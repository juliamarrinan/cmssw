/*----------------------------------------------------------------------
This is a generic main that can be used with any plugin and a
PSet script.   See notes in EventProcessor.cpp for details about it.
----------------------------------------------------------------------*/

#include "FWCore/AbstractServices/interface/TimingServiceBase.h"
#include "FWCore/Framework/interface/CmsRunParser.h"
#include "FWCore/Framework/interface/EventProcessor.h"
#include "FWCore/Framework/interface/defaultCmsRunServices.h"
#include "FWCore/MessageLogger/interface/ExceptionMessages.h"
#include "FWCore/MessageLogger/interface/JobReport.h"
#include "FWCore/MessageLogger/interface/MessageDrop.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ProcessDesc.h"
#include "FWCore/ParameterSet/interface/ThreadsInfo.h"
#include "FWCore/PluginManager/interface/PluginManager.h"
#include "FWCore/PluginManager/interface/PresenceFactory.h"
#include "FWCore/PluginManager/interface/standard.h"
#include "FWCore/ParameterSetReader/interface/ParameterSetReader.h"
#include "FWCore/ServiceRegistry/interface/ServiceRegistry.h"
#include "FWCore/ServiceRegistry/interface/ServiceToken.h"
#include "FWCore/ServiceRegistry/interface/ServiceWrapper.h"
#include "FWCore/Concurrency/interface/setNThreads.h"
#include "FWCore/Concurrency/interface/ThreadsController.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/Utilities/interface/EDMException.h"
#include "FWCore/Utilities/interface/ConvertException.h"
#include "FWCore/Utilities/interface/Presence.h"
#include "FWCore/Utilities/interface/thread_safety_macros.h"

#include "TError.h"

#include "oneapi/tbb/task_arena.h"

#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// -----------------------------------------------
namespace {
  class EventProcessorWithSentry {
  public:
    explicit EventProcessorWithSentry() : ep_(nullptr), callEndJob_(false) {}
    explicit EventProcessorWithSentry(std::unique_ptr<edm::EventProcessor> ep)
        : ep_(std::move(ep)), callEndJob_(false) {}
    ~EventProcessorWithSentry() {
      if (callEndJob_ && ep_.get()) {
        //  See the message in catch clause
        CMS_SA_ALLOW try { ep_->endJob(); } catch (...) {
          edm::LogSystem("MoreExceptions")
              << "After a fatal primary exception was caught, there was an attempt to run\n"
              << "endJob methods. Another exception was caught while endJob was running\n"
              << "and we give up trying to run endJob.";
        }
      }
      edm::clearMessageLog();
    }
    EventProcessorWithSentry(EventProcessorWithSentry const&) = delete;
    EventProcessorWithSentry const& operator=(EventProcessorWithSentry const&) = delete;
    EventProcessorWithSentry(EventProcessorWithSentry&&) = default;             // Allow Moving
    EventProcessorWithSentry& operator=(EventProcessorWithSentry&&) = default;  // Allow moving

    void on() { callEndJob_ = true; }
    void off() { callEndJob_ = false; }
    edm::EventProcessor* operator->() { return ep_.get(); }
    edm::EventProcessor* get() { return ep_.get(); }

  private:
    std::unique_ptr<edm::EventProcessor> ep_;
    bool callEndJob_;
  };

  class TaskCleanupSentry {
  public:
    TaskCleanupSentry(edm::EventProcessor* ep) : ep_(ep) {}
    ~TaskCleanupSentry() { ep_->taskCleanup(); }

  private:
    edm::EventProcessor* ep_;
  };
}  // namespace

int main(int argc, const char* argv[]) {
  edm::TimingServiceBase::jobStarted();

  int returnCode = 0;
  std::string context;
  bool alwaysAddContext = true;
  //Default to only use 1 thread. We define this early (before parsing the command line options
  // and python configuration) since the plugin system or message logger may be using TBB.
  auto tsiPtr = std::make_unique<edm::ThreadsController>(edm::s_defaultNumberOfThreads,
                                                         edm::s_defaultSizeOfStackForThreadsInKB * 1024);
  std::shared_ptr<edm::Presence> theMessageServicePresence;
  std::unique_ptr<std::ofstream> jobReportStreamPtr;
  std::shared_ptr<edm::serviceregistry::ServiceWrapper<edm::JobReport>> jobRep;
  EventProcessorWithSentry proc;

  try {
    returnCode = edm::convertException::wrap([&]() -> int {

    // NOTE: MacOs X has a lower rlimit for opened file descriptor than Linux (256
    // in Snow Leopard vs 512 in SLC5). This is a problem for some of the workflows
    // that open many small root datafiles.  Notice that this is safe to do also
    // for Linux, but we agreed not to change the behavior there for the moment.
    // Also the limits imposed by ulimit are not affected and still apply, if
    // there.
#ifdef __APPLE__
      context = "Setting file descriptor limit";
      struct rlimit limits;
      getrlimit(RLIMIT_NOFILE, &limits);
      limits.rlim_cur = (OPEN_MAX < limits.rlim_max) ? OPEN_MAX : limits.rlim_max;
      setrlimit(RLIMIT_NOFILE, &limits);
#endif

      context = "Initializing plug-in manager";
      edmplugin::PluginManager::configure(edmplugin::standard::config());

      context = "Initializing message logger";
      // Load the message service plug-in
      theMessageServicePresence =
          std::shared_ptr<edm::Presence>(edm::PresenceFactory::get()->makePresence("SingleThreadMSPresence").release());

      context = "Processing command line arguments";
      edm::CmsRunParser parser(argv[0]);

      const auto& parserOutput = parser.parse(argc, argv);
      //return with exit code from parser
      if (edm::CmsRunParser::hasExit(parserOutput))
        return edm::CmsRunParser::getExit(parserOutput);
      auto vm = edm::CmsRunParser::getVM(parserOutput);

      std::string cmdString;
      std::string fileName;
      if (vm.count(edm::CmsRunParser::kCmdOpt)) {
        cmdString = vm[edm::CmsRunParser::kCmdOpt].as<std::string>();
        if (vm.count(edm::CmsRunParser::kParameterSetOpt)) {
          edm::LogAbsolute("CommandLineProcessing") << "cmsRun: Error while trying to process command line arguments:\n"
                                                    << "cannot use '-c [command line input]' with 'config_file'\n"
                                                    << "For usage and an options list, please do 'cmsRun --help'.";
          edm::HaltMessageLogging();
          return edm::errors::CommandLineProcessing;
        }
      } else if (!vm.count(edm::CmsRunParser::kParameterSetOpt)) {
        edm::LogAbsolute("ConfigFileNotFound") << "cmsRun: No configuration file given.\n"
                                               << "For usage and an options list, please do 'cmsRun --help'.";
        edm::HaltMessageLogging();
        return edm::errors::ConfigFileNotFound;
      } else
        fileName = vm[edm::CmsRunParser::kParameterSetOpt].as<std::string>();
      std::vector<std::string> pythonOptValues;
      if (vm.count(edm::CmsRunParser::kPythonOpt)) {
        pythonOptValues = vm[edm::CmsRunParser::kPythonOpt].as<std::vector<std::string>>();
      }
      pythonOptValues.insert(pythonOptValues.begin(), fileName);

      if (vm.count(edm::CmsRunParser::kStrictOpt)) {
        //edm::setStrictParsing(true);
        edm::LogSystem("CommandLineProcessing") << "Strict configuration processing is now done from python";
      }

      context = "Creating the JobReport Service";
      // Decide whether to enable creation of job report xml file
      //  We do this first so any errors will be reported
      std::string jobReportFile;
      if (vm.count(edm::CmsRunParser::kJobreportOpt)) {
        jobReportFile = vm[edm::CmsRunParser::kJobreportOpt].as<std::string>();
      } else if (vm.count(edm::CmsRunParser::kEnableJobreportOpt)) {
        jobReportFile = "FrameworkJobReport.xml";
      }
      jobReportStreamPtr = jobReportFile.empty() ? nullptr : std::make_unique<std::ofstream>(jobReportFile.c_str());

      //NOTE: JobReport must have a lifetime shorter than jobReportStreamPtr so that when the JobReport destructor
      // is called jobReportStreamPtr is still valid
      auto jobRepPtr = std::make_unique<edm::JobReport>(jobReportStreamPtr.get());
      jobRep = std::make_shared<edm::serviceregistry::ServiceWrapper<edm::JobReport>>(std::move(jobRepPtr));
      edm::ServiceToken jobReportToken = edm::ServiceRegistry::createContaining(jobRep);

      if (!fileName.empty()) {
        context = "Processing the python configuration file named ";
        context += fileName;
      } else {
        context = "Processing the python configuration from command line ";
        context += cmdString;
      }
      std::shared_ptr<edm::ProcessDesc> processDesc;
      try {
        std::unique_ptr<edm::ParameterSet> parameterSet;
        if (!fileName.empty())
          parameterSet = edm::readConfig(fileName, pythonOptValues);
        else
          edm::makeParameterSets(cmdString, parameterSet);
        processDesc = std::make_shared<edm::ProcessDesc>(std::move(parameterSet));
      } catch (edm::Exception const&) {
        throw;
      } catch (cms::Exception& iException) {
        //check for "SystemExit: 0" on second line
        const std::string& sysexit0("SystemExit: 0");
        const auto& msg = iException.message();
        size_t pos2 = msg.find('\n');
        if (pos2 != std::string::npos and (msg.size() - (pos2 + 1)) > sysexit0.size() and
            msg.compare(pos2 + 1, sysexit0.size(), sysexit0) == 0)
          return 0;

        edm::Exception e(edm::errors::ConfigFileReadError, "", iException);
        throw e;
      }

      // Determine the number of threads to use, and the per-thread stack size:
      //   - from the command line
      //   - from the "options" ParameterSet, if it exists
      //   - from default values (currently, 1 thread and 10 MB)
      //
      // Since TBB has already been initialised with the default values, re-initialise
      // it only if different values are discovered.
      //
      // Finally, reflect the values being used in the "options" top level ParameterSet.
      context = "Setting up number of threads";
      unsigned int nThreads = 0;
      {
        // check the "options" ParameterSet
        std::shared_ptr<edm::ParameterSet> pset = processDesc->getProcessPSet();
        auto threadsInfo = threadOptions(*pset);

        // check the command line options
        if (vm.count(edm::CmsRunParser::kNumberOfThreadsOpt)) {
          threadsInfo.nThreads_ = vm[edm::CmsRunParser::kNumberOfThreadsOpt].as<unsigned int>();
        }
        if (vm.count(edm::CmsRunParser::kSizeOfStackForThreadOpt)) {
          threadsInfo.stackSize_ = vm[edm::CmsRunParser::kSizeOfStackForThreadOpt].as<unsigned int>();
        }

        // if needed, re-initialise TBB
        if (threadsInfo.nThreads_ != edm::s_defaultNumberOfThreads or
            threadsInfo.stackSize_ != edm::s_defaultSizeOfStackForThreadsInKB) {
          threadsInfo.nThreads_ = edm::setNThreads(threadsInfo.nThreads_, threadsInfo.stackSize_, tsiPtr);
        }
        nThreads = threadsInfo.nThreads_;

        // update the numberOfThreads and sizeOfStackForThreadsInKB in the "options" ParameterSet
        setThreadOptions(threadsInfo, *pset);
      }

      context = "Initializing default service configurations";

      // Default parameters will be used for the default services
      // if they are not overridden from the configuration files.
      processDesc->addServices(edm::defaultCmsRunServices());

      context = "Setting MessageLogger defaults";
      // Decide what mode of hardcoded MessageLogger defaults to use
      if (vm.count(edm::CmsRunParser::kJobModeOpt)) {
        std::string jobMode = vm[edm::CmsRunParser::kJobModeOpt].as<std::string>();
        edm::MessageDrop::instance()->jobMode = jobMode;
      }

      oneapi::tbb::task_arena arena(nThreads);
      arena.execute([&]() {
        context = "Constructing the EventProcessor";
        EventProcessorWithSentry procTmp(
            std::make_unique<edm::EventProcessor>(processDesc, jobReportToken, edm::serviceregistry::kTokenOverrides));
        proc = std::move(procTmp);
        TaskCleanupSentry sentry{proc.get()};

        alwaysAddContext = false;

        proc.on();
        context = "Calling beginJob";
        proc->beginJob();

        processDesc.reset();

        context =
            "Calling EventProcessor::runToCompletion (which does almost everything after beginJob and before endJob)";
        auto status = proc->runToCompletion();
        if (status == edm::EventProcessor::epSignal) {
          returnCode = edm::errors::CaughtSignal;
        }

        proc.off();
        context = "Calling endJob and endStream";
        proc->endJob();
      });
      return returnCode;
    });
  }
  // All exceptions which are not handled before propagating
  // into main will get caught here.
  catch (cms::Exception& ex) {
    returnCode = ex.returnCode();
    if (!context.empty()) {
      if (alwaysAddContext) {
        ex.addContext(context);
      } else if (ex.context().empty()) {
        ex.addContext(context);
      }
    }
    if (!ex.alreadyPrinted()) {
      if (jobRep.get() != nullptr) {
        edm::printCmsException(ex, &(jobRep->get()), returnCode);
      } else {
        edm::printCmsException(ex);
      }
    }
  }
  // Disable Root Error Handler.
  SetErrorHandler(DefaultErrorHandler);
  return returnCode;
}
