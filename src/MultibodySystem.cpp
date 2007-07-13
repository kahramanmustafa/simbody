/* Portions copyright (c) 2006-7 Stanford University and Michael Sherman.
 * Contributors:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, 
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**@file
 *
 * Implementation of MultibodySystem, a concrete System.
 */

#include "simbody/internal/common.h"
#include "simbody/internal/MultibodySystem.h"

#include "MultibodySystemRep.h"
#include "DecorationSubsystemRep.h"

namespace SimTK {


    //////////////////////
    // MULTIBODY SYSTEM //
    //////////////////////

/*static*/ bool 
MultibodySystem::isInstanceOf(const System& s) {
    return MultibodySystemRep::isA(s.getRep());
}
/*static*/ const MultibodySystem&
MultibodySystem::downcast(const System& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const MultibodySystem&>(s);
}
/*static*/ MultibodySystem&
MultibodySystem::updDowncast(System& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<MultibodySystem&>(s);
}

const MultibodySystemRep& 
MultibodySystem::getRep() const {
    return dynamic_cast<const MultibodySystemRep&>(*rep);
}
MultibodySystemRep&       
MultibodySystem::updRep() {
    return dynamic_cast<MultibodySystemRep&>(*rep);
}

// Create generic multibody system by default.
MultibodySystem::MultibodySystem() {
    rep = new MultibodySystemRep();
    rep->setMyHandle(*this);
    updRep().setGlobalSubsystem();
}

MultibodySystem::MultibodySystem(SimbodyMatterSubsystem& m)
{
    rep = new MultibodySystemRep();
    rep->setMyHandle(*this);
    updRep().setGlobalSubsystem();
    setMatterSubsystem(m);
}

// This is a protected constructor for use by derived classes which
// allocate a more specialized MultibodySystemRep.
MultibodySystem::MultibodySystem(MultibodySystemRep* rp) {
    rep = rp;
    rep->setMyHandle(*this);
    updRep().setGlobalSubsystem();
}

bool MultibodySystem::project(State& s, Vector& y_err, 
             const Real& tol,
             const Real& dontProjectFac,
             const Real& targetTol
             ) const
{
    return MultibodySystemRep::downcast(*rep).project(
                s,y_err,tol,dontProjectFac,targetTol);
}


int MultibodySystem::setMatterSubsystem(SimbodyMatterSubsystem& m) {
    return MultibodySystemRep::downcast(*rep).setMatterSubsystem(m);
}
int MultibodySystem::addForceSubsystem(ForceSubsystem& f) {
    return MultibodySystemRep::downcast(*rep).addForceSubsystem(f);
}
int MultibodySystem::setDecorationSubsystem(DecorationSubsystem& m) {
    return MultibodySystemRep::downcast(*rep).setDecorationSubsystem(m);
}

const SimbodyMatterSubsystem&       
MultibodySystem::getMatterSubsystem() const {
    return MultibodySystemRep::downcast(*rep).getMatterSubsystem();
}
SimbodyMatterSubsystem&       
MultibodySystem::updMatterSubsystem() {
    return MultibodySystemRep::downcast(*rep).updMatterSubsystem();
}

const DecorationSubsystem&       
MultibodySystem::getDecorationSubsystem() const {
    return MultibodySystemRep::downcast(*rep).getDecorationSubsystem();
}
DecorationSubsystem&       
MultibodySystem::updDecorationSubsystem() {
    return MultibodySystemRep::downcast(*rep).updDecorationSubsystem();
}

const Real&                
MultibodySystem::getPotentialEnergy(const State& s, Stage g) const {
    return getRep().getPotentialEnergy(s,g);
}
const Real&                
MultibodySystem::getKineticEnergy(const State& s, Stage g) const {
    return getRep().getKineticEnergy(s,g);
}

const Vector_<SpatialVec>& 
MultibodySystem::getRigidBodyForces(const State& s, Stage g) const {
    return getRep().getRigidBodyForces(s,g);
}
const Vector_<Vec3>&       
MultibodySystem::getParticleForces(const State& s, Stage g) const {
    return getRep().getParticleForces(s,g);
}
const Vector&              
MultibodySystem::getMobilityForces(const State& s, Stage g) const {
    return getRep().getMobilityForces(s,g);
}

Real&                
MultibodySystem::updPotentialEnergy(const State& s, Stage g) const {
    return getRep().updPotentialEnergy(s,g);
}
Real&                
MultibodySystem::updKineticEnergy(const State& s, Stage g) const {
    return getRep().updKineticEnergy(s,g);
}

Vector_<SpatialVec>& 
MultibodySystem::updRigidBodyForces(const State& s, Stage g) const {
    return getRep().updRigidBodyForces(s,g);
}
Vector_<Vec3>&       
MultibodySystem::updParticleForces(const State& s, Stage g) const {
    return getRep().updParticleForces(s,g);
}
Vector&              
MultibodySystem::updMobilityForces(const State& s, Stage g) const {
    return getRep().updMobilityForces(s,g);
}


    //////////////////////////
    // MULTIBODY SYSTEM REP //
    //////////////////////////

void MultibodySystemRep::realizeTopologyImpl(State& s) const {
    assert(globalSub.isValid());
    assert(matterSub.isValid());

    // We do Matter subsystem first here in case any of the GlobalSubsystem
    // topology depends on Matter topology. That's unlikely though since
    // we don't know sizes until Model stage.
    getMatterSubsystem().getRep().realizeSubsystemTopology(s);
    getGlobalSubsystem().getRep().realizeSubsystemTopology(s);
    for (int i=0; i < (int)forceSubs.size(); ++i)
        getForceSubsystem(forceSubs[i]).getRep().realizeSubsystemTopology(s);

    if (hasDecorationSubsystem())
        getDecorationSubsystem().getRep().realizeSubsystemTopology(s);
}
void MultibodySystemRep::realizeModelImpl(State& s) const {

    // Here it is essential to do the Matter subsystem first because the
    // force accumulation arrays in the Global subsystem depend on the
    // Stage::Model dimensions of the Matter subsystem.
    getMatterSubsystem().getRep().realizeSubsystemModel(s);
    getGlobalSubsystem().getRep().realizeSubsystemModel(s);
    for (int i=0; i < (int)forceSubs.size(); ++i)
        getForceSubsystem(forceSubs[i]).getRep().realizeSubsystemModel(s);

    if (hasDecorationSubsystem())
        getDecorationSubsystem().getRep().realizeSubsystemModel(s);
}
void MultibodySystemRep::realizeInstanceImpl(const State& s) const {
    getGlobalSubsystem().getRep().realizeSubsystemInstance(s);
    getMatterSubsystem().getRep().realizeSubsystemInstance(s);
    for (int i=0; i < (int)forceSubs.size(); ++i)
        getForceSubsystem(forceSubs[i]).getRep().realizeSubsystemInstance(s);

    if (hasDecorationSubsystem())
        getDecorationSubsystem().getRep().realizeSubsystemInstance(s);
}
void MultibodySystemRep::realizeTimeImpl(const State& s) const {
    getGlobalSubsystem().getRep().realizeSubsystemTime(s);
    getMatterSubsystem().getRep().realizeSubsystemTime(s);
    for (int i=0; i < (int)forceSubs.size(); ++i)
        getForceSubsystem(forceSubs[i]).getRep().realizeSubsystemTime(s);

    if (hasDecorationSubsystem())
        getDecorationSubsystem().getRep().realizeSubsystemTime(s);
}
void MultibodySystemRep::realizePositionImpl(const State& s) const {
    getGlobalSubsystem().getRep().realizeSubsystemPosition(s);
    getMatterSubsystem().getRep().realizeSubsystemPosition(s);
    for (int i=0; i < (int)forceSubs.size(); ++i)
        getForceSubsystem(forceSubs[i]).getRep().realizeSubsystemPosition(s);

    if (hasDecorationSubsystem())
        getDecorationSubsystem().getRep().realizeSubsystemPosition(s);
}
void MultibodySystemRep::realizeVelocityImpl(const State& s) const {
    getGlobalSubsystem().getRep().realizeSubsystemVelocity(s);
    getMatterSubsystem().getRep().realizeSubsystemVelocity(s);
    for (int i=0; i < (int)forceSubs.size(); ++i)
        getForceSubsystem(forceSubs[i]).getRep().realizeSubsystemVelocity(s);

    if (hasDecorationSubsystem())
        getDecorationSubsystem().getRep().realizeSubsystemVelocity(s);
}
void MultibodySystemRep::realizeDynamicsImpl(const State& s) const {
    getGlobalSubsystem().getRep().realizeSubsystemDynamics(s);
    // note order: forces first (TODO: does that matter?)
    for (int i=0; i < (int)forceSubs.size(); ++i)
        getForceSubsystem(forceSubs[i]).getRep().realizeSubsystemDynamics(s);
    getMatterSubsystem().getRep().realizeSubsystemDynamics(s);

    if (hasDecorationSubsystem())
        getDecorationSubsystem().getRep().realizeSubsystemDynamics(s);
}
void MultibodySystemRep::realizeAccelerationImpl(const State& s) const {
    getGlobalSubsystem().getRep().realizeSubsystemAcceleration(s);
    // note order: forces first (TODO: does that matter?)
    for (int i=0; i < (int)forceSubs.size(); ++i)
        getForceSubsystem(forceSubs[i]).getRep().realizeSubsystemAcceleration(s);
    getMatterSubsystem().getRep().realizeSubsystemAcceleration(s);

    if (hasDecorationSubsystem())
        getDecorationSubsystem().getRep().realizeSubsystemAcceleration(s);
}
void MultibodySystemRep::realizeReportImpl(const State& s) const {
    getGlobalSubsystem().getRep().realizeSubsystemReport(s);
    // note order: forces first (TODO: does that matter?)
    for (int i=0; i < (int)forceSubs.size(); ++i)
        getForceSubsystem(forceSubs[i]).getRep().realizeSubsystemReport(s);
    getMatterSubsystem().getRep().realizeSubsystemReport(s);

    if (hasDecorationSubsystem())
        getDecorationSubsystem().getRep().realizeSubsystemReport(s);
}


    ///////////////////////////////////////
    // MULTIBODY SYSTEM GLOBAL SUBSYSTEM //
    ///////////////////////////////////////


/*static*/ bool 
MultibodySystemGlobalSubsystem::isInstanceOf(const Subsystem& s) {
    return MultibodySystemGlobalSubsystemRep::isA(s.getRep());
}
/*static*/ const MultibodySystemGlobalSubsystem&
MultibodySystemGlobalSubsystem::downcast(const Subsystem& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const MultibodySystemGlobalSubsystem&>(s);
}
/*static*/ MultibodySystemGlobalSubsystem&
MultibodySystemGlobalSubsystem::updDowncast(Subsystem& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<MultibodySystemGlobalSubsystem&>(s);
}

const MultibodySystemGlobalSubsystemRep& 
MultibodySystemGlobalSubsystem::getRep() const {
    return dynamic_cast<const MultibodySystemGlobalSubsystemRep&>(*rep);
}
MultibodySystemGlobalSubsystemRep&       
MultibodySystemGlobalSubsystem::updRep() {
    return dynamic_cast<MultibodySystemGlobalSubsystemRep&>(*rep);
}

    ////////////////////////////////
    // MOLECULAR MECHANICS SYSTEM //
    ////////////////////////////////

class DuMMForceFieldSubsystem;

/*static*/ bool 
MolecularMechanicsSystem::isInstanceOf(const System& s) {
    return MolecularMechanicsSystemRep::isA(s.getRep());
}
/*static*/ const MolecularMechanicsSystem&
MolecularMechanicsSystem::downcast(const System& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<const MolecularMechanicsSystem&>(s);
}
/*static*/ MolecularMechanicsSystem&
MolecularMechanicsSystem::updDowncast(System& s) {
    assert(isInstanceOf(s));
    return reinterpret_cast<MolecularMechanicsSystem&>(s);
}

const MolecularMechanicsSystemRep& 
MolecularMechanicsSystem::getRep() const {
    return dynamic_cast<const MolecularMechanicsSystemRep&>(*rep);
}
MolecularMechanicsSystemRep&       
MolecularMechanicsSystem::updRep() {
    return dynamic_cast<MolecularMechanicsSystemRep&>(*rep);
}

MolecularMechanicsSystem::MolecularMechanicsSystem() 
  : MultibodySystem(new MolecularMechanicsSystemRep())
{
}

MolecularMechanicsSystem::MolecularMechanicsSystem
   (SimbodyMatterSubsystem& matter, DuMMForceFieldSubsystem& mm)
  : MultibodySystem(new MolecularMechanicsSystemRep())
{
    setMatterSubsystem(matter);
    setMolecularMechanicsForceSubsystem(mm);
}

int MolecularMechanicsSystem::setMolecularMechanicsForceSubsystem(DuMMForceFieldSubsystem& mm) {
    return MolecularMechanicsSystemRep::downcast(*rep).setMolecularMechanicsForceSubsystem(mm);
}

const DuMMForceFieldSubsystem&       
MolecularMechanicsSystem::getMolecularMechanicsForceSubsystem() const {
    return MolecularMechanicsSystemRep::downcast(*rep).getMolecularMechanicsForceSubsystem();
}

DuMMForceFieldSubsystem&       
MolecularMechanicsSystem::updMolecularMechanicsForceSubsystem() {
    return MolecularMechanicsSystemRep::downcast(*rep).updMolecularMechanicsForceSubsystem();
}

} // namespace SimTK

