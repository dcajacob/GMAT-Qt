//$Id$
//------------------------------------------------------------------------------
//                              FiniteThrust
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool
//
// Copyright (c) 2002-2026 United States Government as represented by the
// Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); 
// You may not use this file except in compliance with the License. 
// You may obtain a copy of the License at:
// http://www.apache.org/licenses/LICENSE-2.0 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either 
// express or implied.   See the License for the specific language
// governing permissions and limitations under the License.
//
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under MOMS Task
// Order 124.
//
// Author: Darrel J. Conway, Thinking Systems, Inc.
// Created: 2004/12/20
//
/**
 * Implements the FiniteThrust class used to model the acceleration during a 
 * finite burn. 
 */
//------------------------------------------------------------------------------


#include <memory>
#include <algorithm>
#include <cmath>
#include <limits>
#include "FiniteThrust.hpp"
#include "OrbitStmForceGradient.hpp"
#include "PropagationStateManager.hpp"
#include "GmatBaseException.hpp"
#include <typeinfo>
#ifdef DEBUG_MEMORY
#include "MemoryTracker.hpp"
#endif

//#define DEBUG_FINITETHRUST_INIT
//#define DEBUG_REGISTRATION
//#define DEBUG_FINITETHRUST_EXE
//#define DEBUG_MASS_FLOW
//#define DEBUG_STATE_ALLOCATION

#include "MessageInterface.hpp"
#include "TimeTypes.hpp"
#include <sstream>               // for stringstream


//------------------------------------------------------------------------------
// FiniteThrust(const std::string &name)
//------------------------------------------------------------------------------
/**
 * Constructor for forces from finite burns.
 * 
 * @param name Name of the constructed instance.
 */
//------------------------------------------------------------------------------
FiniteThrust::FiniteThrust(const std::string &name) :
   PhysicalModel        (Gmat::PHYSICAL_MODEL, "FiniteThrust", name),
   satCount             (0),
   cartIndex            (-1),
   fillCartesian        (false),
   satThrustCount       (0),
   mDotIndex            (-1),
   depleteMass          (true)
{
   derivativeIds.push_back(Gmat::CARTESIAN_STATE);
   derivativeIds.push_back(Gmat::ORBIT_STATE_TRANSITION_MATRIX);
   derivativeIds.push_back(Gmat::ORBIT_A_MATRIX);
   objectTypeNames.push_back("FiniteThrust");
   isConservative = false;
}


//------------------------------------------------------------------------------
// ~FiniteThrust()
//------------------------------------------------------------------------------
/**
 * Destructor for forces from finite burns.
 */
//------------------------------------------------------------------------------
FiniteThrust::~FiniteThrust()
{
   #ifdef DEBUG_STATE_ALLOCATION
      MessageInterface::ShowMessage("Deleting FiniteThrust at %p\n", this);
   #endif
}


//------------------------------------------------------------------------------
// FiniteThrust(const FiniteThrust& ft)
//------------------------------------------------------------------------------
/**
 * Copy constructor for forces from finite burns.
 * 
 * @param ft The original that gets copied.
 */
//------------------------------------------------------------------------------
FiniteThrust::FiniteThrust(const FiniteThrust& ft) :
   PhysicalModel           (ft),
   maneuverTargets         (ft.maneuverTargets),
   burnNames               (ft.burnNames),
   mySpacecraft            (ft.mySpacecraft),
   spacecraft              (ft.spacecraft),
   satCount                (ft.satCount),
   cartIndex               (ft.cartIndex),
   fillCartesian           (ft.fillCartesian),
   satThrustCount          (ft.satThrustCount),
   mDotIndex               (ft.mDotIndex),
   depleteMass             (ft.depleteMass)
{
   burns = ft.burns;
   burnLifetimes = ft.burnLifetimes;
   burnReferences = ft.burnReferences;
   scIndices = ft.scIndices;
}


//------------------------------------------------------------------------------
// FiniteThrust& operator=(const FiniteThrust& ft)
//------------------------------------------------------------------------------
/**
 * Assignment operator.
 * 
 * @param ft Instance that provides the parameter values.
 * 
 * @return This instance, set to match the input instance.
 */
//------------------------------------------------------------------------------
FiniteThrust& FiniteThrust::operator=(const FiniteThrust& ft)
{
   if (this != &ft)
   {
      const bool restoring = GmatBase::IsNativeRestorationCopy();
      const bool ready = isInitialized;
      if (restoring)
      {
         RequireLiveBurns();
         ft.RequireLiveBurns();
         if (burnNames != ft.burnNames || mySpacecraft != ft.mySpacecraft ||
             burns != ft.burns || burnReferences != ft.burnReferences ||
             spacecraft != ft.spacecraft || scIndices != ft.scIndices ||
             satCount != ft.satCount || cartIndex != ft.cartIndex ||
             fillCartesian != ft.fillCartesian || satThrustCount != ft.satThrustCount ||
             mDotIndex != ft.mDotIndex || depleteMass != ft.depleteMass ||
             maneuverTargets.size() != ft.maneuverTargets.size())
            throw ODEModelException("Prepare finite-thrust roles before restoring changed active bindings.");
         for (const auto &entry : maneuverTargets)
         {
            const auto source = ft.maneuverTargets.find(entry.first);
            if (!GmatBase::IsLiveReference(entry.second.object, entry.second.lifetime) ||
                source == ft.maneuverTargets.end() || source->second.object != entry.second.object ||
                !GmatBase::IsLiveReference(source->second.object, source->second.lifetime))
               throw ODEModelException("Restoration cannot replace an active finite-thrust target.");
         }
         isInitialized = false;
         PhysicalModel::operator=(ft);
         isInitialized = ready;
         return *this;
      }
      PhysicalModel::operator=(ft);

      burnNames    = ft.burnNames;
      spacecraft   = ft.spacecraft;
      mySpacecraft = ft.mySpacecraft;
      maneuverTargets = ft.maneuverTargets;
      burnLifetimes.clear();
      burnReferences.clear();
      burns.clear();
      scIndices.clear();

      satCount       = ft.satCount;
      cartIndex      = ft.cartIndex;
      fillCartesian  = ft.fillCartesian;
      satThrustCount = ft.satThrustCount;
      mDotIndex      = ft.mDotIndex;
      depleteMass    = ft.depleteMass;
   }
   return *this;
}


//------------------------------------------------------------------------------
bool FiniteThrust::TargetsSpacecraft(const Spacecraft *sc) const
{
   if (!sc) return false;
   if (maneuverTargets.empty()) return PhysicalModel::TargetsSpacecraft(sc);
   for (const auto &entry : maneuverTargets)
      if (entry.second.object == sc && GmatBase::IsLiveReference(sc, entry.second.lifetime)) return true;
   return false;
}

bool FiniteThrust::SharesTargets(const FiniteThrust &other) const
{
   if (!maneuverTargets.empty())
   {
      for (const auto &entry : maneuverTargets)
         if (GmatBase::IsLiveReference(entry.second.object, entry.second.lifetime) &&
             other.TargetsSpacecraft(entry.second.object)) return true;
      return false;
   }
   if (!other.maneuverTargets.empty()) return other.SharesTargets(*this);
   for (const std::string &name : mySpacecraft)
      if (std::find(other.mySpacecraft.begin(), other.mySpacecraft.end(), name) != other.mySpacecraft.end()) return true;
   return false;
}

bool FiniteThrust::UsesBurn(const FiniteBurn *burn) const
{
   if (!burn || std::find(burns.begin(), burns.end(), burn) == burns.end()) return false;
   RequireLiveBurns();
   return true;
}

void FiniteThrust::RequireLiveBurns() const
{
   for (const FiniteBurn *burn : burns)
   {
      const auto lifetime = burnLifetimes.find(burn);
      if (lifetime == burnLifetimes.end() || !GmatBase::IsLiveReference(burn, lifetime->second))
         throw ODEModelException("FiniteThrust references a FiniteBurn outside its live scope.");
   }
}

bool FiniteThrust::operator==(const FiniteThrust& ft) const
{
   if (!SharesTargets(ft)) return false;
   RequireLiveBurns();
   ft.RequireLiveBurns();
   if (burns.empty() || ft.burns.empty())
   {
      for (const std::string &name : burnNames)
         if (std::find(ft.burnNames.begin(), ft.burnNames.end(), name) != ft.burnNames.end()) return true;
      return false;
   }
   // Overlap means the same target and selected thruster, not equal burn labels.
   for (const FiniteBurn *left : burns)
   {
      const StringArray names = left->GetStringArrayParameter(left->GetParameterID("Thrusters"));
      for (const FiniteBurn *right : ft.burns)
      {
         const StringArray other = right->GetStringArrayParameter(right->GetParameterID("Thrusters"));
         for (const std::string &name : names)
            if (std::find(other.begin(), other.end(), name) != other.end()) return true;
      }
   }
   return false;
}


//------------------------------------------------------------------------------
// GmatBase* Clone() const
//------------------------------------------------------------------------------
/**
 * Overridden Clone method used to make copies from a GmatBase pointer.
 * 
 * @return New instance, set to look like this one.
 */
//------------------------------------------------------------------------------
GmatBase* FiniteThrust::Clone() const
{
   std::unique_ptr<FiniteThrust> value(new FiniteThrust(*this));
   value->RebindFunctionValueContext(*this);
   return value.release();
}


//------------------------------------------------------------------------------
// void Clear(const UnsignedInt type)
//------------------------------------------------------------------------------
/**
 * Clears the arrays of elements that get set by the Propagate commands
 * 
 * @param type The type of element that gets cleared.  Set to 
 *             Gmat::UNKNOWN_OBJECT to clear all of the configrable arrays.
 */
//------------------------------------------------------------------------------
void FiniteThrust::Clear(const UnsignedInt type)
{
   if ((type == Gmat::UNKNOWN_OBJECT) || (type == Gmat::SPACECRAFT)) 
   {
      mySpacecraft.clear();
      spacecraft.clear();
      maneuverTargets.clear();
   }
   
   if (type == Gmat::UNKNOWN_OBJECT || type == Gmat::THRUSTER || type == Gmat::FINITE_BURN)
   {
      burnNames.clear();
      burns.clear();
      burnLifetimes.clear();
      burnReferences.clear();
   }
}


//------------------------------------------------------------------------------
// bool SetRefObjectName(const UnsignedInt type, const std::string &name)
//------------------------------------------------------------------------------
/**
 * Sets the names for referenced objects.
 * 
 * FiniteThrust instances use Spacecraft and FiniteThrust objects.  This method
 * sets the names for those objects.
 * 
 * @param type The type of object that has the name.
 * @param name The object's name
 * 
 * @return true on success, false otherwise.
 */
//------------------------------------------------------------------------------
bool FiniteThrust::SetRefObjectName(const UnsignedInt type,
                                  const std::string &name)
{
   if (type == Gmat::SPACECRAFT) 
   {
      if (find(mySpacecraft.begin(), mySpacecraft.end(), name) == mySpacecraft.end())
         mySpacecraft.push_back(name);
      return true;
   }
   
   if (type == Gmat::FINITE_BURN) 
   {
      if (find(burnNames.begin(), burnNames.end(), name) == burnNames.end())
         burnNames.push_back(name);
      return true;
   }
   
   return PhysicalModel::SetRefObjectName(type, name);
}


//------------------------------------------------------------------------------
// const StringArray& GetRefObjectNameArray(const UnsignedInt type)
//------------------------------------------------------------------------------
/**
 * Accesses the names for referenced objects.
 * 
 * @param type The type of object requested.
 * 
 * @return The vector of names.
 */
//------------------------------------------------------------------------------
const StringArray& FiniteThrust::GetRefObjectNameArray(
                                                   const UnsignedInt type)
{
   if (type == Gmat::SPACECRAFT)
      return mySpacecraft;

   if (type == Gmat::FINITE_BURN)
      return burnNames;
      
   return PhysicalModel::GetRefObjectNameArray(type);
}


//------------------------------------------------------------------------------
// bool SetRefObject(GmatBase *obj, const UnsignedInt type,
//                   const std::string &name)
//------------------------------------------------------------------------------
/**
 * Sets referenced object pointers.
 * 
 * @param obj The object.
 * @param type The type of the object.
 * @param name The object's name.
 * 
 * @return true if the object is set, false if not.
 */
//------------------------------------------------------------------------------
bool FiniteThrust::SetRefObject(GmatBase *obj, const UnsignedInt type,
                                const std::string &name)
{
   if (!obj) return false;
   if (type == Gmat::SPACECRAFT)
   {
      auto *sc = dynamic_cast<Spacecraft*>(obj);
      if (!sc) return false;
      const std::string key = name.empty() ? sc->GetName() : name;
      SetRefObjectName(Gmat::SPACECRAFT, key);
      maneuverTargets[key] = {sc, sc->GetLifetimeToken()};
      return true;
   }
   if (type == Gmat::FINITE_BURN) 
   {
      auto *burn = dynamic_cast<FiniteBurn*>(obj);
      if (!burn)
         throw ODEModelException(
            "FiniteThrust::SetRefObject cannot use objects of type " + 
            obj->GetTypeName());
      const std::string key = name.empty() ? burn->GetName() : name;
      StringArray names = burnNames;
      if (std::find(names.begin(), names.end(), key) == names.end()) names.push_back(key);
      auto references = burnReferences;
      references[key] = burn;
      auto lifetimes = burnLifetimes;
      lifetimes[burn] = burn->GetLifetimeToken();
      std::vector<FiniteBurn*> selected;
      for (const std::string &role : names)
      {
         const auto reference = references.find(role);
         if (reference != references.end() &&
             std::find(selected.begin(), selected.end(), reference->second) == selected.end())
            selected.push_back(reference->second);
      }
      for (auto entry = lifetimes.begin(); entry != lifetimes.end(); )
         if (std::find(selected.begin(), selected.end(), entry->first) == selected.end()) entry = lifetimes.erase(entry);
         else ++entry;
      burnNames.swap(names);
      burnReferences.swap(references);
      burnLifetimes.swap(lifetimes);
      burns.swap(selected);
      return true;
   }
   
   return PhysicalModel::SetRefObject(obj, type, name);
}

//------------------------------------------------------------------------------
// bool SetRefObject(GmatBase *obj, const UnsignedInt type,
//       const std::string &name, const Integer index)
//------------------------------------------------------------------------------
/**
 * Sets referenced object pointers in an object array.
 *
 * @param obj The object.
 * @param type The type of the object.
 * @param name The object's name.
 * @param index The index of the object in the array
 *
 * @return true if the object is set, false if not.
 */
//------------------------------------------------------------------------------
bool FiniteThrust::SetRefObject(GmatBase *obj, const UnsignedInt type,
      const std::string &name, const Integer index)
{
   return PhysicalModel::SetRefObject(obj, type, name, index);
}


//------------------------------------------------------------------------------
// bool RenameRefObject(const UnsignedInt type, const std::string &oldName,
//       const std::string &newName)
//------------------------------------------------------------------------------
/**
 * Changes the name for reference objects
 *
 * @param type The type of the object.
 * @param oldName The object's name before the change.
 * @param newName The object's proposed name after the change.
 *
 * @return true if a change was made, false if not
 */
//------------------------------------------------------------------------------
bool FiniteThrust::RenameRefObject(const UnsignedInt type,
      const std::string &oldName, const std::string &newName)
{
   if (type == Gmat::SPACECRAFT)
   {
      if (oldName == newName) return true;
      const auto existing = maneuverTargets.find(oldName);
      const auto collision = maneuverTargets.find(newName);
      if (existing != maneuverTargets.end() && collision != maneuverTargets.end() &&
          collision->second.object != existing->second.object)
         throw ODEModelException("Conflicting configured finite-thrust target names.");
      if (existing != maneuverTargets.end())
      {
         maneuverTargets[newName] = existing->second;
         maneuverTargets.erase(existing);
      }
      for (auto &target : mySpacecraft) if (target == oldName) target = newName;
      return true;
   }
   if (type == Gmat::FINITE_BURN)
   {
      RequireLiveBurns();
      if (oldName == newName) return true;
      const auto existing = burnReferences.find(oldName);
      const auto collision = burnReferences.find(newName);
      if (existing != burnReferences.end() && collision != burnReferences.end() &&
          existing->second != collision->second)
         throw ODEModelException("Conflicting configured finite-burn reference names.");
      if (existing != burnReferences.end())
      {
         burnReferences[newName] = existing->second;
         burnReferences.erase(existing);
      }

      for (UnsignedInt i = 0; i < burnNames.size(); ++i)
      {
         if (burnNames[i] == oldName)
            burnNames[i] = newName;
      }
      for (UnsignedInt i = 0; i < burns.size(); ++i)
      {
         if (burns[i] != NULL)
            if (burns[i]->GetName() == oldName)
               burns[i]->SetName(newName);
      }
      return true;
   }

   return PhysicalModel::RenameRefObject(type, oldName, newName);
}


//------------------------------------------------------------------------------
// GmatBase* GetRefObject(const UnsignedInt type, const std::string &name)
//------------------------------------------------------------------------------
/**
 * Retrieves a pointer to a reference object
 *
 * @param type The type of the object
 * @param name The object's name
 *
 * @return The object pointer
 */
//------------------------------------------------------------------------------
GmatBase* FiniteThrust::GetRefObject(const UnsignedInt type,
      const std::string &name)
{
   if (type == Gmat::SPACECRAFT && !maneuverTargets.empty())
   {
      for (const auto &entry : maneuverTargets)
         if (name.empty() || entry.first == name)
            return GmatBase::IsLiveReference(entry.second.object, entry.second.lifetime) ? entry.second.object : NULL;
      return NULL;
   }
   if (type == Gmat::FINITE_BURN)
   {
      RequireLiveBurns();
      if ((name == "") && (burns.size() > 0))
         return burns[0];
      const auto role = burnReferences.find(name);
      if (role != burnReferences.end()) return role->second;

      for (UnsignedInt i = 0; i < burns.size(); ++i)
      {
         if (burns[i] != NULL)
            if (burns[i]->GetName() == name)
               return burns[i];
      }

      return NULL;
   }

   if (type == Gmat::SPACECRAFT)
   {
      if (name == "")
         if (spacecraft.size() > 0)
            return spacecraft[0];
      for (UnsignedInt i = 0; i < spacecraft.size(); ++i)
         if (spacecraft[i]->GetName() == name)
            return spacecraft[i];
      return NULL;
   }

   return PhysicalModel::GetRefObject(type, name);
}


//------------------------------------------------------------------------------
// GmatBase* GetRefObject(const UnsignedInt type, const std::string &name,
//       const Integer index)
//------------------------------------------------------------------------------
/**
 * Retrieves a pointer to a reference object from an array
 *
 * @param type The type of the object
 * @param name The object's name
 * @param index The index of the object in the array
 *
 * @return The object pointer
 */
//------------------------------------------------------------------------------
GmatBase* FiniteThrust::GetRefObject(const UnsignedInt type,
      const std::string &name, const Integer index)
{
   return PhysicalModel::GetRefObject(type, name, index);
}


//------------------------------------------------------------------------------
// bool IsTransient()
//------------------------------------------------------------------------------
/**
 * Specifies whether the force is transient.
 * 
 * @return true for all FiniteThrust forces.
 */
//------------------------------------------------------------------------------
bool FiniteThrust::IsTransient()
{
   return true;
}

//------------------------------------------------------------------------------
// bool DepletesMass()
//------------------------------------------------------------------------------
/**
 * Detects mass depletion from a PhysicalModel
 *
 * @return true if the model depletes mass, false if it does not
 */
//------------------------------------------------------------------------------
bool FiniteThrust::DepletesMass()
{
   return depleteMass;
}


//------------------------------------------------------------------------------
// void SetPropList(ObjectArray *soList)
//------------------------------------------------------------------------------
/**
 * Sets the list of propagated space objects for transient forces.
 */
//------------------------------------------------------------------------------
void FiniteThrust::SetPropList(ObjectArray *soList)
{
   spacecraft.clear();
   for (ObjectArray::iterator i = soList->begin(); i != soList->end(); ++i)
      spacecraft.push_back(*i);
   
   #ifdef DEBUG_FINITETHRUST_INIT
      MessageInterface::ShowMessage("Spacecraft list contains %d objects\n",
            spacecraft.size());
   #endif
}


//------------------------------------------------------------------------------
// bool Initialize()
//------------------------------------------------------------------------------
/**
 * Set up data structures to manage finite burns
 * 
 * @return              true if the call succeeds, false on failure.
 */
//------------------------------------------------------------------------------
bool FiniteThrust::Initialize()
{
   #ifdef DEBUG_FINITETHRUST_INIT
      MessageInterface::ShowMessage("FiniteThrust::Initialize entered\n");
   #endif

   RequireLiveBurns();
   isInitialized = PhysicalModel::Initialize();

   if (modelState != NULL)
   {
      delete [] modelState;
      modelState = NULL;
      rawState = NULL;
   }
      
   if (!isInitialized)
      throw ODEModelException("Unable to initialize FiniteThrust base");
      
   // set up the indices into the state vector that match spacecraft with active 
   // thrusters
   Integer satIndex, stateIndex;
   #ifdef DEBUG_FINITETHRUST_INIT
      MessageInterface::ShowMessage(
            "   Looping over satNames\n");
   #endif
   for (StringArray::iterator satName = mySpacecraft.begin(); 
        satName != mySpacecraft.end(); ++satName)
   {
      #ifdef DEBUG_FINITETHRUST_INIT
         MessageInterface::ShowMessage(
               "      satName = %s\n", satName->c_str());
      #endif
      satIndex = 0;
      stateIndex = 0;
      
      for (ObjectArray::iterator propSat = spacecraft.begin();
           propSat != spacecraft.end(); ++propSat) 
      {
         #ifdef DEBUG_FINITETHRUST_INIT
            MessageInterface::ShowMessage(
                  "      Looking at satIndex %d; stateIndex = %d\n", satIndex, 
                  stateIndex);
         #endif
         if ((*propSat)->GetName() == *satName) 
         {
            #ifdef DEBUG_FINITETHRUST_INIT
               MessageInterface::ShowMessage(
                  "      FiniteThrust::Initialize Matched up %s\n", 
                  satName->c_str());
            #endif
         }
         #ifdef DEBUG_FINITETHRUST_INIT
            MessageInterface::ShowMessage(
                  "      PropSat is \"%s\"\n", (*satName).c_str());
         #endif
         if ((*propSat)->IsOfType(Gmat::SPACEOBJECT))
         {
            stateIndex += ((SpaceObject*)(*propSat))->GetState().GetSize();
            ++satIndex;
         }
      }
   }

   for (UnsignedInt i = 0; i < burns.size(); ++i)
   {
      // Verify that the burn identifies minimal objects needed
      FiniteBurn *theBurn = burns[i];
      if (theBurn->GetStringArrayParameter(
            theBurn->GetParameterID("Thrusters")).size() == 0)
         throw ODEModelException("The FiniteBurn object \"" +
               theBurn->GetName() + "\" does not identify any Thrusters, and "
               "cannot be used for a finite burn.");
   }
   
   #ifdef DEBUG_FINITETHRUST_INIT
      MessageInterface::ShowMessage(
            "FiniteThrust::Initialize finished\n");
   #endif
   
   isInitialized = true;
   return true;
}


//------------------------------------------------------------------------------
// bool GetDerivatives(Real * state, Real dt, Integer order)
//------------------------------------------------------------------------------
/**
 * Method invoked to calculate derivatives
 * 
 * This method is invoked to fill the deriv array with derivative information 
 * for the system that is being integrated.  It uses the state and elapsedTime 
 * parameters, along with the time interval dt passed in as a parameter, to 
 * calculate the derivative information at time \f$t=t_0+t_{elapsed}+dt\f$.
 *
 * @param dt            Additional time increment for the derivatitive 
 *                      calculation; defaults to 0.
 * @param state         Pointer to the current state data.  This can differ
 *                      from the PhysicalModel state if the subscribing
 *                      integrator samples other state values during 
 *                      propagation.  (For example, the Runge-Kutta integrators 
 *                      do this during the stage calculations.)
 * @param order         The order of the derivative to be taken (first 
 *                      derivative, second derivative, etc)
 *
 * @return              true if the call succeeds, false on failure.
 */
//------------------------------------------------------------------------------
bool FiniteThrust::GetDerivatives(Real * state, Real dt, Integer order, 
      const Integer id)
{
   RequireLiveBurns();
   #ifdef DEBUG_FINITETHRUST_EXE
      MessageInterface::ShowMessage(
            "FiniteThrust::GetDerivatives entered; fillCartesian is %s, "
            "maneuvering %d spacecraft\n", (fillCartesian ? "true" : "false"),
            spacecraft.size());
   #endif

   Real accel[3], mDot, burnData[4];
   Integer i6, mloc = -1;
   Integer i = 0, j = 0;
   SpaceObject *sat;
   
   if (fillCartesian)
   {
      // Loop through the spacecraft list, building accels for active sats
      for (ObjectArray::iterator sc = spacecraft.begin();
           sc != spacecraft.end(); ++sc) 
      {
         i6 = cartIndex + i * 6;
         
         if (1)  // <-- NEED A TEST HERE!
         {
            if (j < satThrustCount)
            {
               mloc = mDotIndex + j;
               ++j;
            }
         }
         else
            mloc = -1;

         if ((*sc)->IsOfType(Gmat::SPACEOBJECT) == false)
            continue;
         
         // Just a convenience
         sat = (SpaceObject*)(*sc);

         #ifdef DEBUG_FINITETHRUST_EXE
            MessageInterface::ShowMessage("   Checking spacecraft %s\n", 
               sat->GetName().c_str());
         #endif

         if (TargetsSpacecraft(dynamic_cast<Spacecraft*>(sat))) 
         {

            #ifdef DEBUG_FINITETHRUST_EXE
               MessageInterface::ShowMessage("   Maneuvering\n");
            #endif
   
            if (sat->GetType() != Gmat::SPACECRAFT)
               throw ODEModelException("FiniteThrust::GetDerivatives Finite "
               "burns cannot maneuver " + sat->GetTypeName() + " objects");
            // Start with zero thrust
            mDot = accel[0] = accel[1] = accel[2] = 0.0;
   
            // Accumulate thrust and mass flow for each active thruster
            for (std::vector <FiniteBurn*>::iterator fb = burns.begin();
                 fb != burns.end(); ++fb)
            {
               // Setting the spacecraft here makes the setting too late for parameters evaluated at maneuver start
               (*fb)->SetSpacecraftToManeuver((Spacecraft*)sat);
               Real now = epoch + (elapsedTime + dt) / GmatTimeConstants::SECS_PER_DAY;
               if ((*fb)->Fire(burnData, now)) 
               {
                  #ifdef DEBUG_FINITETHRUST_EXE
                     MessageInterface::ShowMessage("%s ",(*fb)->GetName().c_str());
                  #endif
                  accel[0] += burnData[0];
                  accel[1] += burnData[1];
                  accel[2] += burnData[2];
                  #ifdef DEBUG_MASS_FLOW
                     MessageInterface::ShowMessage(
                           "   --> direction = [%.12le %.12le %.12le]",
                           accel[0], accel[1], accel[2]);
                  #endif
                  if ((*fb)->DepletesMass())
                  {
                     if (order != 1)
                        throw ODEModelException("Mass depletion cannot be "
                              "performed with the selected propagator.");
                     mDot += burnData[3];
                     #ifdef DEBUG_MASS_FLOW
                        MessageInterface::ShowMessage("  mDot =  %.12lf\n",
                              mDot);
                     #endif
                  }
                  #ifdef DEBUG_MASS_FLOW
                  else
                     MessageInterface::ShowMessage("  mDot =  0.0\n");
                  #endif

               }
               #ifdef DEBUG_FINITETHRUST_EXE
                  MessageInterface::ShowMessage(
                     "\n   Acceleration = [%18le  %18le  %18le]; "
                     "dm/dt = %.18le", accel[0], accel[1], accel[2], mDot);
               #endif
            }
      
            #ifdef DEBUG_FINITETHRUST_EXE
               MessageInterface::ShowMessage("\n");
            #endif
            
            // Apply the burns to the state vector
            if (order == 1) 
            {
               // dr/dt = v
               deriv[i6]     = 
               deriv[1 + i6] = 
               deriv[2 + i6] = 0.0;
               deriv[3 + i6] = accel[0];
               deriv[4 + i6] = accel[1];
               deriv[5 + i6] = accel[2];

               if (mloc >= 0)
               {
                  deriv[mloc+i]   = mDot;  // Is this right???
               }
            } 
            else  
            {
               deriv[ i6 ] = accel[0]; 
               deriv[i6+1] = accel[1]; 
               deriv[i6+2] = accel[2]; 
               deriv[i6+3] = 0.0; 
               deriv[i6+4] = 0.0; 
               deriv[i6+5] = 0.0; 
            }
         }
         else
         {
            // Zero any burn contribution that is not made for this spacecraft
            if (order == 1) 
            {
               // dr/dt = v
               deriv[i6]     =
               deriv[1 + i6] =
               deriv[2 + i6] =
               deriv[3 + i6] =
               deriv[4 + i6] =
               deriv[5 + i6] = 0.0;
            } 
            else  
            {
               deriv[ i6 ] = 0.0; 
               deriv[i6+1] = 0.0; 
               deriv[i6+2] = 0.0; 
               deriv[i6+3] = 0.0; 
               deriv[i6+4] = 0.0; 
               deriv[i6+5] = 0.0; 
            }
         }
         ++i;
      }
   }
   
   for (const auto matrixType : {Gmat::ORBIT_STATE_TRANSITION_MATRIX, Gmat::ORBIT_A_MATRIX})
   {
      const bool stm = matrixType == Gmat::ORBIT_STATE_TRANSITION_MATRIX;
      if (!(stm ? fillSTM : fillAMatrix))
         continue;
      if (!psm || !theState)
         throw ODEModelException("Finite-thrust derivatives require the propagation state map.");
      const auto &mapping = *psm->GetStateMap();
      Integer start = stm ? stmStart : aMatrixStart;
      const Integer count = stm ? stmCount : aMatrixCount;
      GmatTime atEpoch = hasPrecisionTime ? epochGT : GmatTime(epoch);
      atEpoch.AddSeconds(elapsedTime + dt);
      for (Integer index = 0; index < count; ++index)
      {
         if (start < 0 || start >= static_cast<Integer>(mapping.size()))
            throw ODEModelException("Finite-thrust matrix index is outside the propagation state.");
         auto *owner = dynamic_cast<Spacecraft*>(mapping[start]->object);
         if (!owner)
            throw ODEModelException("Finite-thrust matrix has no spacecraft owner.");
         const Integer rows = owner->GetIntegerParameter("FullSTMRowCount");
         const Integer associate = theState->GetAssociateIndex(start);
         if (rows < 6 || rows > (dimension - start) / rows ||
             associate < 0 || associate > dimension - 6)
            throw ODEModelException("Finite-thrust matrix layout is inconsistent.");
         std::fill(deriv + start, deriv + start + rows*rows, 0.0);
         if (TargetsSpacecraft(owner))
         {
            Rmatrix gradient(3,6);
            SampleSpacecraft(owner, state + associate, atEpoch, &gradient);
            for (Integer row = 0; row < 3; ++row)
               for (Integer column = 0; column < 6; ++column)
                  deriv[start + (row+3)*rows + column] = gradient(row,column);
         }
         start += rows*rows;
      }
   }

   #ifdef DEBUG_FINITETHRUST_EXE
      //ShowDerivative("FiniteThrust::GetDerivatives() AFTER compute", state,
      //               satCount);
      MessageInterface::ShowMessage
         ("     deriv[1:3] = [%18le %18le %18le]\n     deriv[4:6] = [%18le %18le %18le]\n",
          deriv[0], deriv[1], deriv[2], deriv[3], deriv[4], deriv[5]);
      MessageInterface::ShowMessage("FiniteThrust::GetDerivatives finished\n");
   #endif
   
   return true;
}


//------------------------------------------------------------------------------
// Rvector6 GetDerivativesForSpacecraft(Spacecraft* sc)
//------------------------------------------------------------------------------
/**
 * Retrieves the Cartesian state vector of derivatives w.r.t. time
 *
 * @param sc The spacecraft that holds the state vector
 *
 * @return The derivative vector
 */
//------------------------------------------------------------------------------
Rvector6 FiniteThrust::GetDerivativesForSpacecraft(Spacecraft* sc)
{
   if (!sc) throw ODEModelException("Finite-thrust sampling requires a spacecraft.");
   const bool capture = OrbitStmForceGradient::Request::For(this);
   Rmatrix gradient(3,6);
   Rvector6 result(0,0,0,0,0,0);
   if (TargetsSpacecraft(sc) && !burns.empty())
      result = SampleSpacecraft(sc, nullptr,
            sc->HasPrecisionTime() ? sc->GetEpochGT() : GmatTime(sc->GetEpoch()),
            capture ? &gradient : nullptr);
   if (capture)
      OrbitStmForceGradient::Request::Capture(this, gradient);
   return result;
}

Rvector6 FiniteThrust::SampleSpacecraft(Spacecraft *sc, const Real *modelState,
      const GmatTime &atEpoch, Rmatrix *gradient)
{
   RequireLiveBurns();

   // Reporting/ephemeris evaluation must not retarget a live shared burn or
   // change the queried spacecraft's throttle/derivative-state caches.
   std::unique_ptr<GmatBase> value(sc->Clone());
   bool borrowed = value.get() == sc;
   for (GmatBase *original : spacecraft) borrowed = borrowed || value.get() == original;
   for (GmatBase *original : scObjs) borrowed = borrowed || value.get() == original;
   if (borrowed)
   {
      value.release();
      throw ODEModelException("Finite-thrust spacecraft sampling Clone returned an existing object.");
   }
   auto *sample = dynamic_cast<Spacecraft*>(value.get());
   if (!sample || sample->GetTypeName() != sc->GetTypeName())
      throw ODEModelException("Incompatible finite-thrust spacecraft sampling clone.");
   if (modelState)
   {
      if (!forceOrigin)
         throw ODEModelException("Finite-thrust stage sampling requires a force origin.");
      const Rvector6 origin = forceOrigin->GetMJ2000State(atEpoch);
      for (Integer component = 0; component < 6; ++component)
         sample->GetState()[component] = modelState[component] + origin[component];
      sample->GetState().SetEpoch(atEpoch.GetMjd());
      sample->GetState().SetEpochGT(atEpoch);
   }
   if (!sample->InitializeClonedResource())
      throw ODEModelException("Cannot initialize finite-thrust scratch spacecraft.");

   Real nominal[6];
   for (Integer component = 0; component < 6; ++component)
      nominal[component] = sample->GetState()[component];
   auto evaluate = [&](Integer component, Real offset)
   {
      for (Integer index = 0; index < 6; ++index)
         sample->GetState()[index] = nominal[index];
      sample->GetState()[component] += offset;
      Rvector3 result(0,0,0);
      for (const FiniteBurn *burn : burns)
      {
         if (!burn) throw ODEModelException("Missing FiniteBurn during spacecraft sampling.");
         Real burnData[4];
         if (burn->EvaluateForSpacecraft(sample, burnData, sample->GetEpoch()))
            for (Integer row = 0; row < 3; ++row)
               result[row] += burnData[row];
      }
      return result;
   };
   const Rvector3 acceleration = evaluate(0,0);
   if (gradient)
   {
      for (Integer column = 0; column < 6; ++column)
      {
         Real step = column < 3 ? 1.0 : 0.01;
         Rvector3 previous(0,0,0);
         bool accepted = false;
         for (Integer attempt = 0; attempt < 5; ++attempt)
         {
            const Rvector3 minusTwo = evaluate(column,-2*step);
            const Rvector3 minusOne = evaluate(column,-step);
            const Rvector3 plusOne = evaluate(column,step);
            const Rvector3 plusTwo = evaluate(column,2*step);
            const Rvector3 current = (8*(plusOne-minusOne)-(plusTwo-minusTwo))/(12*step);
            accepted = attempt > 0;
            for (Integer row = 0; row < 3; ++row)
            {
               const Real scale = std::max(std::abs(current[row]),std::abs(previous[row]));
               const Real rounding = 512*std::numeric_limits<Real>::epsilon()*
                  std::max({std::abs(acceleration[row]),std::abs(minusTwo[row]),std::abs(plusTwo[row])})/step;
               const Real tolerance = std::max(rounding,scale*1e-6);
               const Real forward = (-3*acceleration[row]+4*plusOne[row]-plusTwo[row])/(2*step);
               const Real backward = (3*acceleration[row]-4*minusOne[row]+minusTwo[row])/(2*step);
               accepted = accepted && std::isfinite(current[row]) &&
                  std::abs(current[row]-previous[row]) <= tolerance &&
                  std::abs(forward-backward) <= tolerance;
            }
            if (accepted)
            {
               for (Integer row = 0; row < 3; ++row)
                  (*gradient)(row,column) = current[row];
               break;
            }
            previous = current;
            step /= 2;
         }
         if (!accepted)
            throw ODEModelException("Finite-thrust sampling did not converge to a smooth finite gradient.");
      }
   }
   return Rvector6(0,0,0,acceleration[0],acceleration[1],acceleration[2]);
}


//------------------------------------------------------------------------------
// bool SupportsDerivative(Gmat::StateElementId id)
//------------------------------------------------------------------------------
/**
 * Function used to determine if the physical model supports derivative 
 * information for a specified type.
 * 
 * @param id State Element ID for the derivative type
 * 
 * @return true if the type is supported, false otherwise. 
 */
//------------------------------------------------------------------------------
bool FiniteThrust::SupportsDerivative(Gmat::StateElementId id)
{
   #ifdef DEBUG_REGISTRATION
      MessageInterface::ShowMessage(
            "FiniteThrust checking for support for id %d\n", id);
   #endif
   
   if (id == Gmat::CARTESIAN_STATE || id == Gmat::ORBIT_STATE_TRANSITION_MATRIX ||
       id == Gmat::ORBIT_A_MATRIX)
      return true;
   
   if (id == Gmat::MASS_FLOW)
   {
      #ifdef DEBUG_REGISTRATION
         MessageInterface::ShowMessage("I support mass flow!\n");
      #endif
      return true;
   }
   
   return PhysicalModel::SupportsDerivative(id);
}


//------------------------------------------------------------------------------
// bool SetStart(Gmat::StateElementId id, Integer index, Integer quantity)
//------------------------------------------------------------------------------
/**
 * Function used to set the start point and size information for the state 
 * vector, so that the derivative information can be placed in the correct place 
 * in the derivative vector.
 * 
 * @param id State Element ID for the derivative type
 * @param index Starting index in the state vector for this type of derivative
 * @param quantity Number of objects that supply this type of data
 * 
 * @return true if the type is supported, false otherwise. 
 */
//------------------------------------------------------------------------------
bool FiniteThrust::SetStart(Gmat::StateElementId id, Integer index, 
                      Integer quantity, Integer sizeOfType)
{
   #ifdef DEBUG_REGISTRATION
      MessageInterface::ShowMessage("FiniteThrust setting start data for id = "
            "%d to index %d; %d objects identified\n", id, index, quantity);
   #endif
   
   bool retval = false;
   
   switch (id)
   {
      case Gmat::CARTESIAN_STATE:
         satCount = quantity;
         cartIndex = index;
         fillCartesian = true;
         cartesianCount = quantity;
         cartesianStart = index;
         retval = true;
         break;

      case Gmat::ORBIT_STATE_TRANSITION_MATRIX:
         stmCount = quantity;
         stmStart = index;
         fillSTM = true;
         totalSTMSize = sizeOfType;
         retval = true;
         break;

      case Gmat::ORBIT_A_MATRIX:
         aMatrixCount = quantity;
         aMatrixStart = index;
         fillAMatrix = true;
         totalSTMSize = sizeOfType;
         retval = true;
         break;

      case Gmat::MASS_FLOW:
         // todo: add mass flow bits here
         satThrustCount = quantity;
         mDotIndex = index;
         depleteMass = true;
         retval = true;
         break;
         
      default:
         break;
   }
   
   return retval;
}

// Reset/copy through the inherited polymorphic contract without slicing.
void FiniteThrust::Copy(const GmatBase *original)
{
   const auto *source = dynamic_cast<const FiniteThrust*>(original);
   if (!source || typeid(*source) != typeid(FiniteThrust) ||
       typeid(*this) != typeid(FiniteThrust))
      throw GmatBaseException("Cannot copy incompatible object into FiniteThrust");
   if (source == this)
      return;
   const auto previousContext = GetFunctionValueContext();
   operator=(*source);
   RebindFunctionValueContext(*source);
}

void FiniteThrust::RebindClonedReferences(const FiniteThrust &source,
      const std::function<GmatBase*(GmatBase*)> &resolve)
{
   if (&source == this) return;
   source.RequireLiveBurns();
   for (const auto &entry : source.maneuverTargets)
      if (!GmatBase::IsLiveReference(entry.second.object, entry.second.lifetime))
         throw ODEModelException("Cannot retain an expired configured finite-thrust target.");
   std::map<std::string, ManeuverTarget> targets;
   for (const auto &entry : source.maneuverTargets)
   {
      auto *target = dynamic_cast<Spacecraft*>(resolve(entry.second.object));
      if (!target) throw ODEModelException("Incompatible retained finite-thrust spacecraft.");
      targets.emplace(entry.first, ManeuverTarget{target, target->GetLifetimeToken()});
   }
   std::vector<FiniteBurn*> selected;
   std::map<const FiniteBurn*, FiniteBurn*> mapped;
   std::map<const FiniteBurn*, std::weak_ptr<const void>> lifetimes;
   for (FiniteBurn *original : source.burns)
   {
      auto *burn = dynamic_cast<FiniteBurn*>(resolve(original));
      if (!burn) throw ODEModelException("Incompatible retained finite-thrust burn.");
      mapped.emplace(original, burn);
      if (std::find(selected.begin(), selected.end(), burn) == selected.end()) selected.push_back(burn);
      lifetimes[burn] = burn->GetLifetimeToken();
   }
   std::map<std::string, FiniteBurn*> references;
   for (const auto &entry : source.burnReferences)
   {
      const auto burn = mapped.find(entry.second);
      if (burn == mapped.end()) throw ODEModelException("Unbound retained finite-burn role " + entry.first);
      references.emplace(entry.first, burn->second);
   }
   StringArray names = source.burnNames, spacecraftNames = source.mySpacecraft;
   burns.swap(selected);
   burnLifetimes.swap(lifetimes);
   burnReferences.swap(references);
   maneuverTargets.swap(targets);
   burnNames.swap(names);
   mySpacecraft.swap(spacecraftNames);
   // These belong to a prepared propagator, not to the force definition.
   psm = NULL;
   theState = NULL;
   scObjs.clear();
   spacecraft.clear();
   scIndices.clear();
   satCount = satThrustCount = 0;
   cartIndex = mDotIndex = -1;
   fillCartesian = false;
   PhysicalModel::fillCartesian = false;
   cartesianStart = -1;
   cartesianCount = 0;
   fillSTM = fillAMatrix = fillMassJacobian = fillTimeJacobian = false;
   stmStart = aMatrixStart = -1;
   stmCount = aMatrixCount = 0;
   #ifdef DEBUG_MEMORY
   auto untrack = [](Real *buffer, const std::string &name)
   {
      if (buffer) MemoryTracker::Instance()->Remove(buffer, name,
            "FiniteThrust::RebindClonedReferences()", "retiring evaluator buffer");
   };
   if (rawState != modelState) untrack(rawState, "rawState");
   untrack(modelState, "modelState");
   untrack(deriv, "deriv");
   untrack(massJacobian, "massJacobian");
   untrack(timeJacobian, "timeJacobian");
   #endif
   if (rawState != modelState) delete [] rawState;
   delete [] modelState;
   rawState = modelState = NULL;
   delete [] deriv;
   deriv = NULL;
   delete [] massJacobian;
   massJacobian = NULL;
   delete [] timeJacobian;
   timeJacobian = NULL;
   stateChanged = true;
   isInitialized = false;
}
