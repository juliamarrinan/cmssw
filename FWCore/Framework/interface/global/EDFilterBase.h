#ifndef FWCore_Framework_global_EDFilterBase_h
#define FWCore_Framework_global_EDFilterBase_h
// -*- C++ -*-
//
// Package:     FWCore/Framework
// Class  :     EDFilterBase
//
/**\class EDFilterBase EDFilterBase.h "EDFilterBase.h"

 Description: [one line class summary]

 Usage:
    <usage>

*/
//
// Original Author:  Chris Jones
//         Created:  Thu, 18 Jul 2013 11:51:14 GMT
//

// system include files
#include <memory>

// user include files
#include "FWCore/Framework/interface/ProducerBase.h"
#include "FWCore/Framework/interface/EDConsumerBase.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "DataFormats/Provenance/interface/ModuleDescription.h"
#include "FWCore/ParameterSet/interface/ParameterSetfwd.h"
#include "FWCore/Concurrency/interface/WaitingTaskHolder.h"

// forward declarations

namespace edm {
  class ModuleCallingContext;
  class PreallocationConfiguration;
  class StreamID;
  class ActivityRegistry;
  class ThinnedAssociationsHelper;
  class EventForTransformer;
  class ServiceWeakToken;
  class SignallingProductRegistryFiller;

  namespace maker {
    template <typename T>
    class ModuleHolderT;
  }

  namespace global {

    class EDFilterBase : public ProducerBase, public EDConsumerBase {
    public:
      template <typename T>
      friend class edm::maker::ModuleHolderT;
      template <typename T>
      friend class edm::WorkerT;
      typedef EDFilterBase ModuleType;

      EDFilterBase();
      ~EDFilterBase() override;

      static void fillDescriptions(ConfigurationDescriptions& descriptions);
      static void prevalidate(ConfigurationDescriptions& descriptions);
      static const std::string& baseType();

      // Warning: the returned moduleDescription will be invalid during construction
      ModuleDescription const& moduleDescription() const { return moduleDescription_; }

      virtual bool wantsProcessBlocks() const noexcept = 0;
      virtual bool wantsInputProcessBlocks() const noexcept = 0;
      virtual bool wantsGlobalRuns() const noexcept = 0;
      virtual bool wantsGlobalLuminosityBlocks() const noexcept = 0;
      virtual bool wantsStreamRuns() const noexcept = 0;
      virtual bool wantsStreamLuminosityBlocks() const noexcept = 0;

    private:
      bool doEvent(EventTransitionInfo const&, ActivityRegistry*, ModuleCallingContext const*);
      void doAcquire(EventTransitionInfo const&, ActivityRegistry*, ModuleCallingContext const*, WaitingTaskHolder&&);
      void doTransformAsync(WaitingTaskHolder iTask,
                            size_t iTransformIndex,
                            EventPrincipal const& iEvent,
                            ActivityRegistry*,
                            ModuleCallingContext,
                            ServiceWeakToken const&) noexcept;
      //For now this is a placeholder
      /*virtual*/ void preActionBeforeRunEventAsync(WaitingTaskHolder iTask,
                                                    ModuleCallingContext const& iModuleCallingContext,
                                                    Principal const& iPrincipal) const noexcept {}

      void doPreallocate(PreallocationConfiguration const&);
      void doBeginJob();
      void doEndJob();

      void doBeginStream(StreamID id);
      void doEndStream(StreamID id);
      void doStreamBeginRun(StreamID, RunTransitionInfo const&, ModuleCallingContext const*);
      void doStreamEndRun(StreamID, RunTransitionInfo const&, ModuleCallingContext const*);
      void doStreamBeginLuminosityBlock(StreamID, LumiTransitionInfo const&, ModuleCallingContext const*);
      void doStreamEndLuminosityBlock(StreamID, LumiTransitionInfo const&, ModuleCallingContext const*);

      void doBeginProcessBlock(ProcessBlockPrincipal const&, ModuleCallingContext const*);
      void doAccessInputProcessBlock(ProcessBlockPrincipal const&, ModuleCallingContext const*);
      void doEndProcessBlock(ProcessBlockPrincipal const&, ModuleCallingContext const*);
      void doBeginRun(RunTransitionInfo const&, ModuleCallingContext const*);
      void doEndRun(RunTransitionInfo const&, ModuleCallingContext const*);
      void doBeginLuminosityBlock(LumiTransitionInfo const&, ModuleCallingContext const*);
      void doEndLuminosityBlock(LumiTransitionInfo const&, ModuleCallingContext const*);

      void doRespondToCloseOutputFile() { clearInputProcessBlockCaches(); }

      void registerProductsAndCallbacks(EDFilterBase* module, SignallingProductRegistryFiller* reg) {
        registerProducts(module, reg, moduleDescription_);
      }

      virtual bool filter(StreamID, Event&, EventSetup const&) const = 0;
      virtual void beginJob() {}
      virtual void endJob() {}

      virtual void preallocStreams(unsigned int);
      virtual void preallocRuns(unsigned int);
      virtual void preallocRunsSummary(unsigned int);
      virtual void preallocLumis(unsigned int);
      virtual void preallocLumisSummary(unsigned int);
      virtual void preallocate(PreallocationConfiguration const&);
      virtual void doBeginStream_(StreamID id);
      virtual void doEndStream_(StreamID id);
      virtual void doStreamBeginRun_(StreamID id, Run const& rp, EventSetup const& c);
      virtual void doStreamEndRun_(StreamID id, Run const& rp, EventSetup const& c);
      virtual void doStreamEndRunSummary_(StreamID id, Run const& rp, EventSetup const& c);
      virtual void doStreamBeginLuminosityBlock_(StreamID id, LuminosityBlock const& lbp, EventSetup const& c);
      virtual void doStreamEndLuminosityBlock_(StreamID id, LuminosityBlock const& lbp, EventSetup const& c);
      virtual void doStreamEndLuminosityBlockSummary_(StreamID id, LuminosityBlock const& lbp, EventSetup const& c);

      virtual void doBeginProcessBlock_(ProcessBlock const&);
      virtual void doAccessInputProcessBlock_(ProcessBlock const&);
      virtual void doEndProcessBlock_(ProcessBlock const&);
      virtual void doBeginRun_(Run const& rp, EventSetup const& c);
      virtual void doBeginRunSummary_(Run const& rp, EventSetup const& c);
      virtual void doEndRunSummary_(Run const& rp, EventSetup const& c);
      virtual void doEndRun_(Run const& rp, EventSetup const& c);
      virtual void doBeginLuminosityBlock_(LuminosityBlock const& lbp, EventSetup const& c);
      virtual void doBeginLuminosityBlockSummary_(LuminosityBlock const& rp, EventSetup const& c);
      virtual void doEndLuminosityBlockSummary_(LuminosityBlock const& lb, EventSetup const& c);
      virtual void doEndLuminosityBlock_(LuminosityBlock const& lb, EventSetup const& c);

      virtual void doBeginProcessBlockProduce_(ProcessBlock&);
      virtual void doEndProcessBlockProduce_(ProcessBlock&);
      virtual void doBeginRunProduce_(Run& rp, EventSetup const& c);
      virtual void doEndRunProduce_(Run& rp, EventSetup const& c);
      virtual void doBeginLuminosityBlockProduce_(LuminosityBlock& lbp, EventSetup const& c);
      virtual void doEndLuminosityBlockProduce_(LuminosityBlock& lbp, EventSetup const& c);

      virtual size_t transformIndex_(edm::ProductDescription const& iBranch) const noexcept;
      virtual ProductResolverIndex transformPrefetch_(std::size_t iIndex) const noexcept;
      virtual void transformAsync_(WaitingTaskHolder iTask,
                                   std::size_t iIndex,
                                   edm::EventForTransformer& iEvent,
                                   edm::ActivityRegistry* iAct,
                                   ServiceWeakToken const& iToken) const noexcept;

      virtual void clearInputProcessBlockCaches();
      virtual bool hasAcquire() const noexcept { return false; }
      bool hasAccumulator() const noexcept { return false; }

      virtual void doAcquire_(StreamID, Event const&, edm::EventSetup const&, WaitingTaskHolder&&);

      void setModuleDescription(ModuleDescription const& md) { moduleDescription_ = md; }
      ModuleDescription moduleDescription_;
      std::unique_ptr<std::vector<BranchID>[]> previousParentages_;  //Per stream in the future?
      std::unique_ptr<std::vector<BranchID>[]> gotBranchIDsFromAcquire_;
      std::unique_ptr<ParentageID[]> previousParentageIds_;
    };
  }  // namespace global
}  // namespace edm

#endif
