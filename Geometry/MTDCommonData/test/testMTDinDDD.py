import FWCore.ParameterSet.Config as cms

import Configuration.Geometry.defaultPhase2ConditionsEra_cff as _settings
import Geometry.MTDCommonData.defaultMTDConditionsEra_cff as _mtdgeo
_mtdgeo.check_mtdgeo()
_PH2_GLOBAL_TAG, _PH2_ERA = _settings.get_era_and_conditions(_mtdgeo.MTD_DEFAULT_VERSION)

process = cms.Process("CompareGeometryTest", _PH2_ERA)

process.source = cms.Source("EmptySource")
process.maxEvents = cms.untracked.PSet(
        input = cms.untracked.int32(1)
        )

process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.MessageLogger.cerr.threshold = cms.untracked.string('INFO')
process.MessageLogger.cerr.INFO = cms.untracked.PSet(
    limit = cms.untracked.int32(0)
)
process.MessageLogger.cerr.TestMTDIdealGeometry = cms.untracked.PSet(
    # limit = cms.untracked.int32(-1)
    limit = cms.untracked.int32(0)
)
process.MessageLogger.cerr.TestMTDNumbering = cms.untracked.PSet(
    # limit = cms.untracked.int32(-1)
    limit = cms.untracked.int32(0)
)
process.MessageLogger.cerr.TestMTDPath = cms.untracked.PSet(
    # limit = cms.untracked.int32(-1)
    limit = cms.untracked.int32(0)
)
process.MessageLogger.cerr.TestMTDPosition = cms.untracked.PSet(
    # limit = cms.untracked.int32(-1)
    limit = cms.untracked.int32(0)
)
process.MessageLogger.files.mtdCommonDataDDD = cms.untracked.PSet(
    DEBUG = cms.untracked.PSet(
        limit = cms.untracked.int32(0)
    ),
    ERROR = cms.untracked.PSet(
        limit = cms.untracked.int32(0)
    ),
    FWKINFO = cms.untracked.PSet(
        limit = cms.untracked.int32(0)
    ),
    INFO = cms.untracked.PSet(
        limit = cms.untracked.int32(0)
    ),
    MTDUnitTest = cms.untracked.PSet(
        limit = cms.untracked.int32(-1)
    ),
    WARNING = cms.untracked.PSet(
        limit = cms.untracked.int32(0)
    ),
    noLineBreaks = cms.untracked.bool(True),
    threshold = cms.untracked.string('INFO')
)

process.load('Geometry.MTDCommonData.GeometryExtendedRun4MTDDefault_cff')

process.testBTL = cms.EDAnalyzer("TestMTDIdealGeometry",
                               ddTopNodeName = cms.untracked.string('BarrelTimingLayer')
                               )

process.testETL = cms.EDAnalyzer("TestMTDIdealGeometry",
                               ddTopNodeName = cms.untracked.string('EndcapTimingLayer')
                               )

process.Timing = cms.Service("Timing")

process.p1 = cms.Path(cms.wait(process.testBTL)+process.testETL)
