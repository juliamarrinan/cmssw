//
// Authors of this file:        Dustin Stolp (dostolp@ucdavis.edu)
//                              Sushil S. Chauhan (schauhan@cern.ch)
// Creation date: 01.22.2015
//
// -------------------------------------------------------------------
//
#include "SimG4Core/CustomPhysics/interface/CMSDarkPairProduction.h"
#include "G4PhysicalConstants.hh"
#include <CLHEP/Units/SystemOfUnits.h>
#include "G4PairProductionRelModel.hh"

using namespace std;

CMSDarkPairProduction::CMSDarkPairProduction(const G4ParticleDefinition* p, G4double df, const G4String& nam)
    : G4PairProductionRelModel(p, nam), dark_factor(df) {}

G4double CMSDarkPairProduction::ComputeCrossSectionPerAtom(
    const G4ParticleDefinition* p, G4double e, G4double Z, G4double e1, G4double e2, G4double e3) {
  return dark_factor * G4PairProductionRelModel::ComputeCrossSectionPerAtom(p, e, Z, e1, e2, e3);
}
