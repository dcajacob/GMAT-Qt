//------------------------------------------------------------------------------
//                              Formation
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool
//
// Copyright (c) 2002-2026 United States Government as represented by the
// Administrator of The National Aeronautics and Space Administration.
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
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under contract
// number NNG04CI63P
//
// Author: Darrel J. Conway, Thinking Systems, Inc.
// Created: 2004/7/24
// Modified: 
//    2010.03.25 Thomas Grubb 
//      - Modified IsParameterReadOnly to correctly set CartesianState and
//        A1Epoch to true
//
/**
 * Implements the class used for formations. 
 */
//------------------------------------------------------------------------------


#include "Formation.hpp"
#include "StringUtil.hpp"
#include <algorithm>          // for find()
#include <limits>
#include <cmath>
#include <memory>
#include <functional>
#include <typeinfo>
#include <utility>
#include <stdio.h>            // for sprintf()

//#define DEBUG_FORMATION
//#define DEBUG_FORMATION_ACTIONS
//#define DEBUG_REF_OBJ
//#define DEBUG_SET_STRING

#include "MessageInterface.hpp"

namespace
{
Integer MemberStateSize(const std::vector<SpaceObject *> &members,
                        const std::string &formationName)
{
   Integer size = 0;
   for (auto *member : members)
   {
      if (!member || !member->IsOfType(Gmat::SPACECRAFT))
         throw SpaceObjectException("Invalid spacecraft reference in formation \"" +
            formationName + "\".");
      const Integer memberSize = member->GetState().GetSize();
      if (memberSize < 6 || memberSize > std::numeric_limits<Integer>::max() - size)
         throw SpaceObjectException("Invalid or overflowing state size in formation \"" +
            formationName + "\".");
      size += memberSize;
   }
   return size;
}

void CheckMemberEpochs(const std::vector<SpaceObject *> &members,
                       const std::string &formationName)
{
   if (members.empty()) return;
   auto *first = members.front();
   for (auto *member : members)
      if (member->GetEpoch() != first->GetEpoch() ||
          (first->HasPrecisionTime() && member->HasPrecisionTime() &&
           member->GetEpochGT() != first->GetEpochGT()))
         throw SpaceObjectException("Epochs are not synchronized in formation \"" +
            formationName + "\" (" + first->GetName() + ", " + member->GetName() + ").");
}

std::vector<SpaceObject *> OrderedMembers(const StringArray &names,
      const std::vector<SpaceObject *> &members, const std::string &formationName)
{
   if (names.empty() || names.size() != members.size())
      throw SpaceObjectException("Bind every configured spacecraft before building formation \"" +
         formationName + "\".");
   std::vector<SpaceObject *> ordered;
   ordered.reserve(names.size());
   for (const auto &name : names)
   {
      auto match = std::find_if(members.begin(), members.end(),
         [&](SpaceObject *member) { return member && member->GetName() == name; });
      if (match == members.end() || std::find(ordered.begin(), ordered.end(), *match) != ordered.end())
         throw SpaceObjectException("Missing or duplicate spacecraft \"" + name +
            "\" in formation \"" + formationName + "\".");
      ordered.push_back(*match);
   }
   return ordered;
}

void CheckMemberOrder(const StringArray &names,
      const std::vector<SpaceObject *> &members, const std::string &formationName)
{
   if (names.empty() || names.size() != members.size())
      throw SpaceObjectException("Incomplete member layout in formation \"" + formationName + "\".");
   for (UnsignedInt i = 0; i < names.size(); ++i)
      if (!members[i] || members[i]->GetName() != names[i])
         throw SpaceObjectException("Rebuild the configured member order in formation \"" +
            formationName + "\".");
}

Real &MemberElement(const StringArray &names, const std::vector<SpaceObject *> &members,
      Integer dimension, Integer index, const std::string &formationName)
{
   const Integer size = MemberStateSize(members, formationName);
   CheckMemberOrder(names, members, formationName);
   CheckMemberEpochs(members, formationName);
   if (dimension != size || index < 0 || index >= size)
      throw SpaceObjectException("Rebuild the shared member layout in formation \"" + formationName + "\".");
   for (auto *member : members)
   {
      const Integer memberSize = member->GetState().GetSize();
      if (index < memberSize) return member->GetState()[index];
      index -= memberSize;
   }
   throw SpaceObjectException("Invalid shared formation state index.");
}

template <typename Epoch>
Rvector6 SharedCenter(const StringArray &names, const std::vector<SpaceObject *> &members,
      const std::string &referenceName, const std::string &formationName, const Epoch &epoch)
{
   MemberStateSize(members, formationName);
   CheckMemberOrder(names, members, formationName);
   CheckMemberEpochs(members, formationName);
   Rvector6 center;
   for (auto *member : members)
   {
      if (member->GetJ2000BodyName() != referenceName)
         throw SpaceObjectException("Shared formation \"" + formationName +
            "\" has inconsistent J2000 references.");
      const Rvector6 value = member->GetMJ2000State(epoch);
      for (Integer i = 0; i < 6; ++i)
      {
         if (!std::isfinite(value[i]))
            throw SpaceObjectException("Nonfinite member state in shared formation \"" + formationName + "\".");
         center[i] += value[i];
      }
   }
   center /= members.size();
   for (Integer i = 0; i < 6; ++i)
      if (!std::isfinite(center[i]))
         throw SpaceObjectException("Nonfinite shared formation center for \"" + formationName + "\".");
   return center;
}
}


//---------------------------------
// static data
//---------------------------------
const std::string
Formation::PARAMETER_TEXT[FormationParamCount - SpaceObjectParamCount] =
{
   "Add",
   "Remove",
   "Clear",
   "STM",
   "AllowSharedMembers",
   "CartesianState",
};


const Gmat::ParameterType
Formation::PARAMETER_TYPE[FormationParamCount - SpaceObjectParamCount] =
{
   Gmat::OBJECTARRAY_TYPE,
   Gmat::OBJECT_TYPE,     
   Gmat::BOOLEAN_TYPE,
   Gmat::RMATRIX_TYPE,      // FORMATION_STM,
   Gmat::BOOLEAN_TYPE,     // ALLOW_SHARED_MEMBERS,
   Gmat::REAL_TYPE,        // FORMATION_CARTESIAN_STATE,
};


//---------------------------------
// public members
//---------------------------------

//------------------------------------------------------------------------------
// Formation(UnsignedInt typeId, const std::string &typeStr,
//           const std::string &instName)
//------------------------------------------------------------------------------
/**
 * Default constructor.
 *
 * @param <typeId>   UnsignedInt of the constructed object.
 * @param <typeStr>  String describing the type of object created.
 * @param <instName> Name of the constructed instance.
 */
//------------------------------------------------------------------------------
Formation::Formation(UnsignedInt typeId, const std::string &typeStr,
                     const std::string &instName) :
   FormationInterface(typeId, typeStr, instName),
   dimension         (0),
   satCount          (0),
   allowSharedMembers(false)
{
   objectTypes.push_back(Gmat::FORMATION);
   objectTypeNames.push_back("Formation");

   parameterCount = FormationParamCount;
}


//------------------------------------------------------------------------------
// ~Formation()
//------------------------------------------------------------------------------
/**
 * Destructor.
 */
//------------------------------------------------------------------------------
Formation::~Formation()
{
}


//------------------------------------------------------------------------------
// Formation(const Formation& orig)
//------------------------------------------------------------------------------
/**
 * Copy constructor.
 *
 * @param <orig> Formation that is copied onto this one.
 */
//------------------------------------------------------------------------------
Formation::Formation(const Formation& orig) : Formation(orig, true)
{
}

Formation::Formation(const Formation& orig, bool sampleMembers) :
   FormationInterface   (orig),
   componentNames       (orig.componentNames),
   dimension            (orig.dimension),
   satCount             (orig.satCount),
   allowSharedMembers   (orig.allowSharedMembers)
{
   parameterCount = FormationParamCount;
   if (sampleMembers && orig.HasLiveMembers())
   {
      // Capture a current snapshot without adopting the source's bindings.
      // A dormant alias may have an older packed state than its spacecraft.
      components = orig.components;
      BuildState();
      components.clear();
   }
}


//------------------------------------------------------------------------------
// Formation& operator=(const Formation& orig)
//------------------------------------------------------------------------------
/**
 * Assignment operator.
 *
 * @param <so> Formation that is copied onto this one.
 *
 * @return this instance, configured like the input instance.
 */
//------------------------------------------------------------------------------
Formation& Formation::operator=(const Formation& orig)
{
   if (this != &orig)
   {
      const bool restoring = GmatBase::IsNativeRestorationCopy();
      const bool ready = isInitialized;
      bool complete = false;
      const auto onFailure = [&](void*) { if (restoring && !complete) isInitialized = false; };
      const std::unique_ptr<void, decltype(onFailure)> restorationState(this, onFailure);
      if (restoring)
      {
         if (componentNames != orig.componentNames || dimension != orig.dimension ||
             satCount != orig.satCount || allowSharedMembers != orig.allowSharedMembers)
            throw SpaceObjectException("Prepare formation declarations before changing an active restoration graph.");
         if (!components.empty())
         {
            CheckMemberOrder(componentNames, components, instanceName);
            if (components.size() != size_t(satCount) || MemberStateSize(components, instanceName) != dimension)
               throw SpaceObjectException("The active formation member layout has changed.");
         }
      }
      std::unique_ptr<Formation> current;
      const Formation *source = &orig;
      if (orig.HasLiveMembers())
      {
         current.reset(new Formation(orig));
         source = current.get();
      }
      FormationInterface::operator=(*source);
      
      componentNames = source->componentNames;
      dimension      = source->dimension;
      satCount       = source->satCount;
      allowSharedMembers = source->allowSharedMembers;
      if (restoring) isInitialized = ready;
      complete = true;
      
      // Don't clear components here because the assignment operator is used 
      // DURING propagation to evaluate stopping conditions 
      // components.clear();
   }
   #ifdef DEBUG_FORMATION
      MessageInterface::ShowMessage(
         "Formation::operator= : Formation '%s' has %d components\n", 
         instanceName.c_str(), components.size());
   #endif
   
   return *this;
}


//------------------------------------------------------------------------------
// Utility methods used by the CoordinateSystem code
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// const Rvector6 GetMJ2000State(const A1Mjd &atTime)
//------------------------------------------------------------------------------
/**
 * Access the MJ2000 state for this Formation.
 *
 * @param <atTime> Epoch for the state data.
 *
 * @return The Cartesian MJ2000 state.
 *
 * @todo Implement GetMJ2000State in the derived classes, and remove this
 *       implementation.
 */
//------------------------------------------------------------------------------
const Rvector6 Formation::GetMJ2000State(const A1Mjd &atTime)
{
   if (allowSharedMembers)
   {
      if (!j2000Body) throw SpaceObjectException("MJ2000 body not yet set for " + instanceName);
      return SharedCenter(componentNames, components, GetJ2000BodyName(), instanceName, atTime);
   }
   satCount = components.size();
   if (satCount)
   {
      CheckMemberOrder(componentNames, components, instanceName);
      const Integer size = MemberStateSize(components, instanceName);
      if (dimension <= 0 || state.GetSize() != dimension || dimension != size)
         throw SpaceObjectException("Build the current member state before requesting formation \"" +
            instanceName + "\" state.");
   }
   
   if (j2000Body == NULL)
      throw SpaceObjectException("MJ2000 body not yet set for " +
         instanceName);

   // First calculate the geometric center of the formation
   Rvector6 centerState;

   GmatState &ps = GetState();
   Real *st = ps.GetState();
   
   if (satCount == 0)
   {
      MessageInterface::ShowMessage(
         "Warning: Attempting to find MJ2000 state for an empty formation\n");
      return centerState;
   }

   // BuildState packs complete member states. Cartesian data are the first
   // six elements of each packet, whose full size determines the next offset.
   Integer packetOffset = 0;
   for (auto *member : components)
   {
      for (Integer j = 0; j < 6; ++j)
         centerState[j] += st[packetOffset + j];
      packetOffset += member->GetState().GetSize();
   }
   centerState /= satCount;

   // Then calculate the J2000 data based on that state
   Rvector6 bodyState = j2000Body->GetMJ2000State(atTime);

   // If origin is NULL, assume it is set at the J2000 origin.
   if (origin)
   {
      Rvector6 offset = origin->GetMJ2000State(atTime);
      bodyState -= offset;
   }

   return centerState - bodyState;
}



//------------------------------------------------------------------------------
// const Rvector6 GetMJ2000State(const GmatTime &atTimeGT)
//------------------------------------------------------------------------------
/**
 * Access the MJ2000 state for this Formation.
 *
 * @param <atTimeGT>       Epoch for the state data.
 *
 * @return The Cartesian MJ2000 state.
 *
 */
 //------------------------------------------------------------------------------
const Rvector6 Formation::GetMJ2000State(const GmatTime &atTimeGT)
{
   if (allowSharedMembers)
   {
      if (!j2000Body) throw SpaceObjectException("MJ2000 body not yet set for " + instanceName);
      return SharedCenter(componentNames, components, GetJ2000BodyName(), instanceName, atTimeGT);
   }
   satCount = components.size();
   if (satCount)
   {
      CheckMemberOrder(componentNames, components, instanceName);
      const Integer size = MemberStateSize(components, instanceName);
      if (dimension <= 0 || state.GetSize() != dimension || dimension != size)
         throw SpaceObjectException("Build the current member state before requesting formation \"" +
            instanceName + "\" state.");
   }

   if (j2000Body == NULL)
      throw SpaceObjectException("MJ2000 body not yet set for " +
         instanceName);

   // First calculate the geometric center of the formation
   Rvector6 centerState;

   GmatState &ps = GetState();
   Real *st = ps.GetState();

   if (satCount == 0)
   {
      MessageInterface::ShowMessage(
         "Warning: Attempting to find MJ2000 state for an empty formation\n");
      return centerState;
   }

   // BuildState packs complete member states. Cartesian data are the first
   // six elements of each packet, whose full size determines the next offset.
   Integer packetOffset = 0;
   for (auto *member : components)
   {
      for (Integer j = 0; j < 6; ++j)
         centerState[j] += st[packetOffset + j];
      packetOffset += member->GetState().GetSize();
   }
   centerState /= satCount;

   // Then calculate the J2000 data based on that state
   Rvector6 bodyState = j2000Body->GetMJ2000State(atTimeGT);

   // If origin is NULL, assume it is set at the J2000 origin.
   if (origin)
   {
      Rvector6 offset = origin->GetMJ2000State(atTimeGT);
      bodyState -= offset;
   }

   return centerState - bodyState;
}


//------------------------------------------------------------------------------
// Accessor methods
//------------------------------------------------------------------------------

bool Formation::HasLiveMembers() const
{
   return allowSharedMembers && !components.empty() && components.size() == componentNames.size();
}

void Formation::CheckLiveMembers() const
{
   MemberStateSize(components, instanceName);
   CheckMemberOrder(componentNames, components, instanceName);
   CheckMemberEpochs(components, instanceName);
}

Real Formation::GetEpoch()
{
   if (HasLiveMembers())
   {
      CheckLiveMembers();
      return components.front()->GetEpoch();
   }
   return FormationInterface::GetEpoch();
}

GmatTime Formation::GetEpochGT()
{
   if (HasLiveMembers())
   {
      CheckLiveMembers();
      return components.front()->GetEpochGT();
   }
   return FormationInterface::GetEpochGT();
}

Real Formation::SetEpoch(const Real epoch)
{
   if (HasLiveMembers()) return SetRealParameter(EPOCH_PARAM, epoch);
   return FormationInterface::SetEpoch(epoch);
}

GmatTime Formation::SetEpochGT(const GmatTime epoch)
{
   if (HasLiveMembers()) return SetGmatTimeParameter(EPOCH_PARAM, epoch);
   return FormationInterface::SetEpochGT(epoch);
}

GmatTime Formation::GetGmatTimeParameter(const Integer id) const
{
   if (id == EPOCH_PARAM) return const_cast<Formation *>(this)->GetEpochGT();
   return FormationInterface::GetGmatTimeParameter(id);
}

GmatTime Formation::GetGmatTimeParameter(const std::string &label) const
{
   return GetGmatTimeParameter(GetParameterID(label));
}

GmatTime Formation::SetGmatTimeParameter(const Integer id, const GmatTime value)
{
   if (id == EPOCH_PARAM) MemberStateSize(components, instanceName);
   const GmatTime result = FormationInterface::SetGmatTimeParameter(id, value);
   if (id == EPOCH_PARAM)
   {
      // Match the existing Real-epoch coupling, retaining the full native time
      // rather than leaving members at the preceding rounded Real epoch.
      for (auto *member : components)
      {
         const GmatTime actual = member->SetGmatTimeParameter(EPOCH_PARAM, value);
         if (actual != value) throw SpaceObjectException("Formation constituent returned incorrect precision epoch.");
      }
   }
   return result;
}

GmatTime Formation::SetGmatTimeParameter(const std::string &label, const GmatTime value)
{
   return SetGmatTimeParameter(GetParameterID(label), value);
}

//------------------------------------------------------------------------------
//  bool RenameRefObject(const UnsignedInt type,
//                       const std::string &oldName, const std::string &newName)
//------------------------------------------------------------------------------
/**
 * Renames referenced objects.
 *
 * @param <type> Type of the object that is renamed.
 * @param <oldName> The current name for the object.
 * @param <newName> The name the object has when this operation is complete.
 *
 * @return true on success.
 */
//------------------------------------------------------------------------------
bool Formation::RenameRefObject(const UnsignedInt type,
                                const std::string &oldName,
                                const std::string &newName)
{
   #if DEBUG_RENAME
      MessageInterface::ShowMessage
         ("Formation::RenameRefObject() type=%s, oldName=%s, newName=%s\n",
          GetObjectTypeString(type).c_str(), oldName.c_str(), newName.c_str());
   #endif
   
   // Formation needs to know about spacecraft or other formation only
   if (type != Gmat::SPACECRAFT && type != Gmat::FORMATION )
      return true;
   
   //@todo replace any object name here
   for (UnsignedInt i=0; i<componentNames.size(); i++)
   {
      if (componentNames[i] == oldName)
         componentNames[i] = newName;
   }
   
   return true;
}


//------------------------------------------------------------------------------
//  GmatBase* Clone() const
//------------------------------------------------------------------------------
/**
 * This method returns a clone of the Achieve.
 *
 * @return clone of the Achieve.
 */
//------------------------------------------------------------------------------
GmatBase* Formation::Clone() const
{
   std::unique_ptr<Formation> value(new Formation(*this));
   value->RebindFunctionValueContext(*this);
   return value.release();
}


//---------------------------------------------------------------------------
// void Copy(const GmatBase* orig)
//---------------------------------------------------------------------------
/**
 * Sets this object to match another one.
 * 
 * @param orig The original that is being copied.
 */
//---------------------------------------------------------------------------
void Formation::Copy(const GmatBase* orig)
{
   const auto *source = dynamic_cast<const Formation*>(orig);
   if (!source) throw SpaceObjectException("Cannot copy an incompatible resource into a Formation.");
   if (source == this) return;
   const auto previous = GetFunctionValueContext();
   operator=(*source);
   // Native whole-object Copy transfers member bindings; native operator=
   // retains destination bindings for propagation stop-condition work.
   if (!GmatBase::IsNativeRestorationCopy()) components = source->components;
   RebindFunctionValueContext(*source);
}


//------------------------------------------------------------------------------
// RebindClonedReferences
//------------------------------------------------------------------------------
void Formation::RebindClonedReferences(const FormationInterface &original,
      const std::function<GmatBase*(GmatBase*)> &resolve, bool bindMembers)
{
   const auto *source = dynamic_cast<const Formation*>(&original);
   if (!source || source == this)
      throw SpaceObjectException("Incompatible Formation reference-copy source.");

   if (!bindMembers)
   {
      // Native Clone creates a dormant propagation buffer. Retain its body
      // safely while keeping the native empty member bindings and packed data.
      SpacePoint *body = dynamic_cast<SpacePoint*>(resolve(source->GetJ2000Body()));
      if (source->GetJ2000Body() && !body)
         throw SpaceObjectException("Incompatible Formation J2000 reference.");
      SetJ2000Body(body);
      return;
   }

   // Stage the complete name/pointer packet before changing this Formation.
   // Sequential RenameRefObject/SetRefObject cannot handle name permutations,
   // and Clear/Add/BuildState would discard an authoritative packed snapshot.
   StringArray names = source->componentNames;
   std::vector<SpaceObject*> members;
   members.reserve(source->components.size());
   for (UnsignedInt i = 0; i < names.size(); ++i)
   {
      auto found = std::find_if(source->components.begin(), source->components.end(),
            [&](SpaceObject *member) { return member && member->GetName() == source->componentNames[i]; });
      if (found == source->components.end()) continue; // an unresolved configured member
      auto *member = dynamic_cast<SpaceObject*>(resolve(*found));
      if (!member || !member->IsOfType(Gmat::SPACECRAFT) ||
          member->GetState().GetSize() != (*found)->GetState().GetSize())
         throw SpaceObjectException("Incompatible retained member in Formation \"" + instanceName + "\".");
      if (std::find(members.begin(), members.end(), member) != members.end())
         throw SpaceObjectException("Duplicate retained spacecraft in Formation \"" + instanceName + "\".");
      members.push_back(member);
      names[i] = member->GetName();
   }
   if (members.size() != source->components.size())
      throw SpaceObjectException("Formation has an unconfigured or duplicate bound member: " + source->GetName());
   for (UnsignedInt i = 0; i < names.size(); ++i)
      if (std::find(names.begin(), names.begin() + i, names[i]) != names.begin() + i)
         throw SpaceObjectException("Ambiguous retained member names in Formation \"" + instanceName + "\".");
   if (source->allowSharedMembers && members.size() == names.size())
      CheckMemberEpochs(members, instanceName);

   SpacePoint *body = dynamic_cast<SpacePoint*>(resolve(source->GetJ2000Body()));
   if (source->GetJ2000Body() && !body)
      throw SpaceObjectException("Incompatible Formation J2000 reference.");

   componentNames.swap(names);
   components.swap(members);
   satCount = components.size();
   SetJ2000Body(body);
   // Keep this native copy's dimension, packed state, epoch, STM, precision
   // flag and AllowSharedMembers. Binding never pushes values into members.
}

std::unique_ptr<FormationRestorationPlan> Formation::PrepareRestorationLayout(
      const FormationInterface &snapshot, const ObjectArray &members)
{
   const auto *source = dynamic_cast<const Formation*>(&snapshot);
   if (!source) throw SpaceObjectException("Incompatible formation restoration value.");
   if (typeid(*this) != typeid(Formation) || typeid(*source) != typeid(Formation))
      return FormationInterface::PrepareRestorationLayout(snapshot, members);
   std::vector<SpaceObject*> targets;
   targets.reserve(members.size());
   for (GmatBase *object : members)
   {
      auto *member = dynamic_cast<SpaceObject*>(object);
      if (!member || !member->IsOfType(Gmat::SPACECRAFT) ||
            std::find(targets.begin(), targets.end(), member) != targets.end())
         throw SpaceObjectException("Invalid captured native formation member.");
      targets.push_back(member);
   }
   CheckMemberOrder(source->componentNames, targets, instanceName);
   if (source->componentNames.size() != targets.size() || source->satCount != targets.size() ||
         MemberStateSize(targets, instanceName) != source->dimension ||
         source->state.GetSize() != source->dimension)
      throw SpaceObjectException("Captured formation state and member layouts disagree.");
   bool sameState = true;
   try { state.ValidateRestorationLayout(source->state); }
   catch (const BaseException &) { sameState = false; }
   if (sameState && componentNames == source->componentNames && components == targets &&
         dimension == source->dimension && satCount == source->satCount &&
         allowSharedMembers == source->allowSharedMembers) return {};
   struct Layout
   {
      // Retire the displaced layout while its actual previous owner lives.
      std::shared_ptr<const FunctionValueContext> previousOwner;
      std::unique_ptr<Formation> value;
   };
   auto saved = std::make_shared<Layout>();
   saved->previousOwner = GetFunctionValueContext();
   saved->value = std::make_unique<Formation>(*source);
   saved->value->components.swap(targets);
   struct Plan final : FormationRestorationPlan
   {
      std::function<void()> apply;
      void Apply() noexcept override { apply(); }
   };
   auto plan = std::make_unique<Plan>();
   plan->apply = [this, saved]() noexcept
   {
      Formation &prepared = *saved->value;
      using std::swap;
      componentNames.swap(prepared.componentNames);
      components.swap(prepared.components);
      swap(dimension, prepared.dimension);
      swap(satCount, prepared.satCount);
      swap(allowSharedMembers, prepared.allowSharedMembers);
      state.Swap(prepared.state);
   };
   return plan;
}

GmatBase *Formation::CloneRestorationProbe() const
{
   if (typeid(*this) != typeid(Formation))
      return FormationInterface::CloneRestorationProbe();
   // The current member selection can have been replaced or retired. A probe
   // copies its packed data/configuration without reading those member objects.
   return new Formation(*this, false);
}

//------------------------------------------------------------------------------
// void ParametersHaveChanged(bool flag)
//------------------------------------------------------------------------------
/**
 * Used to set or clear the parmsChanged flag from outside of the Formation.
 * 
 * @param <flag>  The new value for the flag.
 */
//------------------------------------------------------------------------------
void Formation::ParametersHaveChanged(bool flag)
{
   parmsChanged = flag;
   // Set the flag for all of the members
   for (std::vector<SpaceObject*>::iterator i = components.begin(); 
        i != components.end(); ++i)
      (*i)->ParametersHaveChanged(flag);
}



//------------------------------------------------------------------------------
//  std::string  GetParameterText(const Integer id) const
//------------------------------------------------------------------------------
/**
 * This method returns the parameter text, given the input parameter ID.
 *
 * @param <id> Id for the requested parameter text.
 *
 * @return parameter text for the requested parameter.
 */
//------------------------------------------------------------------------------
std::string Formation::GetParameterText(const Integer id) const
{
   if (id >= SpaceObjectParamCount && id < FormationParamCount)
      return PARAMETER_TEXT[id - SpaceObjectParamCount];
   
   if ((id >= FORMATION_CARTESIAN_STATE) && 
       (id < FORMATION_CARTESIAN_STATE + dimension))
      return PARAMETER_TEXT[FORMATION_CARTESIAN_STATE - SpaceObjectParamCount];
      
   return FormationInterface::GetParameterText(id);
}


//------------------------------------------------------------------------------
//  Integer  GetParameterID(const std::string &str) const
//------------------------------------------------------------------------------
/**
 * This method returns the parameter ID, given the input parameter string.
 *
 * @param <str> string for the requested parameter.
 *
 * @return ID for the requested parameter.
 */
//------------------------------------------------------------------------------
Integer Formation::GetParameterID(const std::string &str) const
{
   for (Integer i = SpaceObjectParamCount; i < FormationParamCount; i++)
   {
      if (str == PARAMETER_TEXT[i - SpaceObjectParamCount])
         return i;
   }
   
   return FormationInterface::GetParameterID(str);
}


//------------------------------------------------------------------------------
//  Gmat::ParameterType  GetParameterType(const Integer id) const
//------------------------------------------------------------------------------
/**
 * This method returns the parameter type, given the input parameter ID.
 *
 * @param <id> ID for the requested parameter.
 *
 * @return parameter type of the requested parameter.
 */
//------------------------------------------------------------------------------
Gmat::ParameterType Formation::GetParameterType(const Integer id) const
{
   if (id >= SpaceObjectParamCount && id < FormationParamCount)
      return PARAMETER_TYPE[id - SpaceObjectParamCount];

   if ((id >= FORMATION_CARTESIAN_STATE) && 
       (id < FORMATION_CARTESIAN_STATE + dimension))
      return PARAMETER_TYPE[FORMATION_CARTESIAN_STATE - SpaceObjectParamCount];
      
   return FormationInterface::GetParameterType(id);
}

//---------------------------------------------------------------------------
//  bool IsParameterReadOnly(const Integer id) const
//---------------------------------------------------------------------------
/**
 * Checks to see if the requested parameter is read only.
 *
 * @param <id> Description for the parameter.
 *
 * @return true if the parameter is read only, false (the default) if not,
 *         throws if the parameter is out of the valid range of values.
 */
//---------------------------------------------------------------------------
bool Formation::IsParameterReadOnly(const Integer id) const
{
   // STM is a computed propagation output, not writable configuration. Its
   // propagation item is currently unimplemented (GetPropItem returns null).
   // Do not advertise it as a script-persistent matrix with no setter/getter.
   if (id == EPOCH_PARAM || id == FORMATION_STM)
      return true;

   if ((id == REMOVED_SPACECRAFT) || (id == CLEAR_NAMES))
      return true;
   
   if ((id >= FORMATION_CARTESIAN_STATE) && 
       (id <= FORMATION_CARTESIAN_STATE + dimension))
      return true;

   if (id == ORBIT_COLOR || id == TARGET_COLOR)
      return true;
   
   return FormationInterface::IsParameterReadOnly(id);
}

bool Formation::IsParameterEqualToDefault(const Integer id) const
{
   if (id == ALLOW_SHARED_MEMBERS) return !allowSharedMembers;
   return FormationInterface::IsParameterEqualToDefault(id);
}

bool Formation::IsParameterEqualToDefault(const std::string &label) const
{
   return IsParameterEqualToDefault(GetParameterID(label));
}

bool Formation::IsParameterCommandModeSettable(const Integer id) const
{
   // Sharing is structural configuration, validated across the resource graph.
   if (id == ALLOW_SHARED_MEMBERS) return false;
   return FormationInterface::IsParameterCommandModeSettable(id);
}

//------------------------------------------------------------------------------
// bool GetBooleanParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Retrieve a boolean parameter.
 *
 * @param <id> The id for the parameter.
 *
 * @return the boolean value for this parameter.
 */
//------------------------------------------------------------------------------
bool Formation::GetBooleanParameter(const Integer id) const
{
   switch (id)
   {
      case ALLOW_SHARED_MEMBERS:
         return allowSharedMembers;
      case CLEAR_NAMES:
         return false;
      default:
         return FormationInterface::GetBooleanParameter(id);
   }
}

//------------------------------------------------------------------------------
// bool GetBooleanParameter(const std::string &label) const
//------------------------------------------------------------------------------
/**
 * Retrieve a boolean parameter.
 *
 * @param <label> The (string) label for the parameter.
 *
 * @return the boolean value for this parameter.
 */
//------------------------------------------------------------------------------
bool Formation::GetBooleanParameter(const std::string &label) const
{
   return GetBooleanParameter(GetParameterID(label));
}

//------------------------------------------------------------------------------
// bool SetBooleanParameter(const Integer id, const bool value)
//------------------------------------------------------------------------------
/**
 * Sets the value for a boolean parameter.
 *
 * This method is used to clear the Formation's list of spacecraft.
 *
 * @param <id> The id for the parameter.
 * @param <value> The new value for the parameter.
 *
 * @return the boolean value for this parameter.
 */
//------------------------------------------------------------------------------
bool Formation::SetBooleanParameter(const Integer id, const bool value)
{
   #ifdef DEBUG_FORMATION_ACTIONS
      MessageInterface::ShowMessage(
         "Formation::SetBooleanParameter called with id = %d\n", id);
   #endif
 
   switch (id)
   {
      case ALLOW_SHARED_MEMBERS:
         // Freeze a current snapshot before leaving shared-view mode.
         if (allowSharedMembers && !value && HasLiveMembers()) UpdateState();
         allowSharedMembers = value;
         return true;
      case CLEAR_NAMES:
         return ClearSpacecraftList();
      default:
         return FormationInterface::SetBooleanParameter(id, value);
   }
}

//------------------------------------------------------------------------------
// bool SetBooleanParameter(const std::string &label, const bool value)
//------------------------------------------------------------------------------
/**
 * Sets the value for a boolean parameter.
 *
 * @param <label> The (string) label for the parameter.
 * @param <value> The new value for the parameter.
 *
 * @return the boolean value for this parameter.
 */
//------------------------------------------------------------------------------
bool Formation::SetBooleanParameter(const std::string &label,
                                    const bool value)
{
   #ifdef DEBUG_FORMATION_ACTIONS
      MessageInterface::ShowMessage(
         "Formation::SetBooleanParameter called with label = %s\n",
         label.c_str());
   #endif

   return SetBooleanParameter(GetParameterID(label), value);
}

//------------------------------------------------------------------------------
//  std::string  GetParameterTypeString(const Integer id) const
//------------------------------------------------------------------------------
/**
 * This method returns the parameter type string, given the input parameter ID.
 *
 * @param <id> ID for the requested parameter.
 *
 * @return parameter type string of the requested parameter.
 */
//------------------------------------------------------------------------------
std::string Formation::GetParameterTypeString(const Integer id) const
{
   return GmatBase::PARAM_TYPE_STRING[GetParameterType(id)];
}


//------------------------------------------------------------------------------
//  std::string  SetStringParameter(const Integer id, const std::string value)
//------------------------------------------------------------------------------
/**
 * This method sets the string parameter value, given the input
 * parameter ID.
 *
 * @param <id>    ID for the requested parameter.
 * @param <value> string value for the requested parameter.
 *
 * @return  success flag.
 */
//------------------------------------------------------------------------------
void Formation::SetField(const Integer id, const StringArray &value)
{
   if (id != ADDED_SPACECRAFT) { FormationInterface::SetField(id, value); return; }
   StringArray next(value);
   for (size_t i = 0; i < next.size(); ++i)
   {
      if (!GmatStringUtil::IsValidIdentity(next[i]) || next[i] == instanceName)
         throw SpaceObjectException("Invalid Formation member name: " + next[i]);
      if (std::find(next.begin(), next.begin() + i, next[i]) != next.begin() + i)
         throw SpaceObjectException("Repeated Formation member: " + next[i]);
   }
   if (next == componentNames) return;
   // A changed definition is dormant until its actual members are bound.
   // Match native Clear's placeholder without publishing names before allocation.
   GmatState empty(state);
   empty.SetSize(6);
   const Real zeros[6] = {};
   if (!empty.SetState(zeros, 6)) throw SpaceObjectException("Cannot clear Formation state.");
   const auto previous = GetFunctionValueContext();
   state = empty;
   componentNames.swap(next);
   components.clear();
   dimension = 0;
   satCount = 0;
   parmsChanged = true;
}

UnsignedInt Formation::GetPropertyObjectType(const Integer id) const
{
   return id == ADDED_SPACECRAFT ? Gmat::SPACECRAFT : FormationInterface::GetPropertyObjectType(id);
}

bool Formation::SetStringParameter(const Integer id, const std::string &value)
{
#ifdef DEBUG_SET_STRING
   MessageInterface::ShowMessage("Formation::SetStringParameter(id = %d, value = <%s>) enter\n", id, value.c_str());
#endif

   if (id == ADDED_SPACECRAFT)
   {
      if (find(componentNames.begin(), componentNames.end(), value) != 
              componentNames.end())
         return false;
      componentNames.push_back(value);
      dimension = 0;

      #ifdef DEBUG_FORMATION_ACTIONS
         MessageInterface::ShowMessage("%s%s%s\n",
            "Formation \"", instanceName.c_str(),
            "\" now consists of these spacecraft names:");
         for (StringArray::iterator i = componentNames.begin(); 
              i != componentNames.end(); ++i)
            MessageInterface::ShowMessage("    \"%s\"\n", i->c_str());
      #endif

      parmsChanged = true;
      return true;
   }
   if (id == REMOVED_SPACECRAFT)
   {
      return RemoveSpacecraft(value);
   }
   return FormationInterface::SetStringParameter(id, value);
}


//------------------------------------------------------------------------------
//  Real  SetRealParameter(const Integer id, const Real value)
//------------------------------------------------------------------------------
/**
 * This method sets the Real parameter value, given the input parameter ID.
 *
 * @param <id>    ID for the parameter whose value to change.
 * @param <value> value for the parameter.
 *
 * @return Real value of the requested parameter.
 */
//------------------------------------------------------------------------------
Real Formation::SetRealParameter(const Integer id, const Real value)
{
   if (id == EPOCH_PARAM) MemberStateSize(components, instanceName);
   Real retval = -1.0;

   if ((id >= FORMATION_CARTESIAN_STATE) && 
       (id < FORMATION_CARTESIAN_STATE + dimension))
   {
      if (HasLiveMembers())
      {
         // Refresh the complete packet before changing one element so a
         // dormant alias cannot later overwrite newer, untouched member data.
         UpdateState();
         MemberElement(componentNames, components, dimension,
            id - FORMATION_CARTESIAN_STATE, instanceName) = value;
      }
      state[id - FORMATION_CARTESIAN_STATE] = value;
      retval = value;
   }
   else
   {
      retval = FormationInterface::SetRealParameter(id, value);
         
      if (id == EPOCH_PARAM)
      {
         if (retval != value)
            throw SpaceObjectException(
               "Formation update returned incorrect epoch");
         // Update the epoch on the constituent pieces
         for (std::vector<SpaceObject*>::iterator i = components.begin();
              i != components.end(); ++i)
         {
            retval = (*i)->SetRealParameter(id, value);
            if (retval != value)
               throw SpaceObjectException(
                  "Formation constituent returned incorrect epoch");
         }
      }
   }   

   return retval;
}


//------------------------------------------------------------------------------
//  Real  SetRealParameter(const std::string &label, const Real value)
//------------------------------------------------------------------------------
/**
 * This method sets the Real parameter value, given the input parameter ID.
 *
 * @param <label> String description for the parameter value.
 * @param <value> value for the parameter.
 *
 * @return Real value of the requested parameter.
 */
//------------------------------------------------------------------------------
Real Formation::SetRealParameter(const std::string &label, const Real value)
{
   return SetRealParameter(GetParameterID(label), value);
}


//------------------------------------------------------------------------------
//  bool SetStringParameter(const Integer id, const std::string &value,
//                          const Integer index)
//------------------------------------------------------------------------------
/**
 * Change the value of a string parameter.
 *
 * @param <id>    The integer ID for the parameter.
 * @param <value> The new string for this parameter.
 * @param <index> Index for parameters in arrays. 
 *
 * @return true if the string is stored, false if not.
 */
//------------------------------------------------------------------------------
bool Formation::SetStringParameter(const Integer id, const std::string &value,
                                   const Integer index)
{
#ifdef DEBUG_SET_STRING
   MessageInterface::ShowMessage("Formation::SetStringParameter(id = %d, value = <%s>, index = %d) enter\n", id, value.c_str(), index);
#endif

   if (id == ADDED_SPACECRAFT)
   {
      // Add an empty list
      if (index == -1)
      {
         parmsChanged = false;
         return true;
      }

      // verify input value
      if (!GmatStringUtil::IsValidIdentity(value))
         throw GmatBaseException("Error: The value \'" + value 
         + "' is invalid to set to " + GetName() + ".Add parameter.\n");

      if ((0 <= index) && (index < componentNames.size()))
      {
         if (componentNames[index] == value)
            parmsChanged = false;
         else
         {
            if (std::find(componentNames.begin(), componentNames.end(), value) != componentNames.end())
               throw SpaceObjectException("Duplicate spacecraft \"" + value + "\" in formation \"" + instanceName + "\".");
            componentNames[index] = value;
            dimension = 0;
            parmsChanged = true;
         }
      }
      else
      {
         if (index < 0)
            throw SpaceObjectException("Invalid formation member index.");
         if (std::find(componentNames.begin(), componentNames.end(), value) != componentNames.end())
            return false;
         componentNames.push_back(value);
         dimension = 0;
         parmsChanged = true;
      }
      return true;
   }
   if (id == REMOVED_SPACECRAFT)
   {
      return RemoveSpacecraft(value);
   }
   return FormationInterface::SetStringParameter(id, value, index);
}


//------------------------------------------------------------------------------
//  std::string  GetStringParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
 * This method returns the string parameter value, given the input
 * parameter ID.
 *
 * @param <id> ID for the requested parameter.
 *
 * @return  string value of the requested parameter.
 */
//------------------------------------------------------------------------------
std::string  Formation::GetStringParameter(const Integer id) const
{
   if (id == REMOVED_SPACECRAFT)
   {
      return "";
   }
   return FormationInterface::GetStringParameter(id);
}

//------------------------------------------------------------------------------
// std::string  GetStringParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
 * This method returns the string parameter value, given the input
 * parameter ID.
 *
 * @param <id>    ID for the requested parameter.
 * @param <index> Index for the requested parameter.
 *
 * @return  string value of the requested parameter.
 */
//------------------------------------------------------------------------------
std::string  Formation::GetStringParameter(const Integer id,
                                           const Integer index) const
{
   if (id == REMOVED_SPACECRAFT)
   {
      return "";
   }
   return FormationInterface::GetStringParameter(id, index);
}



//------------------------------------------------------------------------------
//  const StringArray& GetStringArrayParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Access an array of string data.
 *
 * @param <id> The integer ID for the parameter.
 *
 * @return The requested StringArray; throws if the parameter is not a
 *         StringArray.
 */
//------------------------------------------------------------------------------
const StringArray& Formation::GetStringArrayParameter(const Integer id) const
{
   if (id == ADDED_SPACECRAFT)
      return componentNames;
   return FormationInterface::GetStringArrayParameter(id);
}


//------------------------------------------------------------------------------
// const StringArray& GetStringArrayParameter(const std::string &label) const
//------------------------------------------------------------------------------
/**
 * Access an array of string data.
 *
 * @param label The script string for the parameter.
 *
 * @return The requested StringArray; throws if the parameter is not a
 *         StringArray.
 */
//------------------------------------------------------------------------------
const StringArray& 
         Formation::GetStringArrayParameter(const std::string &label) const
{
   if (label == "Add")
      return componentNames;
   return FormationInterface::GetStringArrayParameter(label);
}


//------------------------------------------------------------------------------
// const ObjectTypeArray& GetRefObjectTypeArray()
//------------------------------------------------------------------------------
/**
* Retrieves the list of reference object types used by this class.
*
* @return the list of object types.
*
*/
//------------------------------------------------------------------------------
const ObjectTypeArray& Formation::GetRefObjectTypeArray()
{
   #ifdef DEBUG_REF_OBJ
   MessageInterface::ShowMessage("Formation::GetRefObjectTypeArray() enter\n");
   #endif

   refObjectTypes.clear();
   refObjectTypes.push_back(Gmat::SPACECRAFT);
   return refObjectTypes;
}


//------------------------------------------------------------------------------
//  const StringArray& GetRefObjectNameArray(const Gmat::ObjectType type)
//------------------------------------------------------------------------------
/**
 * This method returns an array with the names of the referenced objects.
 *
 * @return a vector with the names of objects of the requested type.
 */
//------------------------------------------------------------------------------
const StringArray& Formation::GetRefObjectNameArray(const UnsignedInt type)
{
   return componentNames;
}


//------------------------------------------------------------------------------
// GmatBase* GetRefObject(type, name)
//------------------------------------------------------------------------------
GmatBase* Formation::GetRefObject(const UnsignedInt type, const std::string &name)
{
   if (type == Gmat::UNKNOWN_OBJECT || type == Gmat::SPACECRAFT || type == Gmat::SPACEOBJECT)
      for (SpaceObject *member : components)
         if (member && member->GetName() == name) return member;
   return FormationInterface::GetRefObject(type, name);
}

//------------------------------------------------------------------------------
// GmatBase* Formation::GetRefObject(const UnsignedInt type,
//                                   const std::string &name,
//                                   const Integer index)
//------------------------------------------------------------------------------
/**
 * Accessor for referenced objects.
 *
 * @param <type>  reference object type.
 * @param <name>  reference object name.
 * @param <index> Which reference object array to search.
 *
 * @return Pointer to the reference object.
 *
 * @note This method is implemented to ensure that polymorphism doen't confuse
 *       some compilers.
 */
//------------------------------------------------------------------------------
GmatBase* Formation::GetRefObject(const UnsignedInt type,
                                  const std::string &name,
                                  const Integer index)
{
   return FormationInterface::GetRefObject(type, name, index);
}


//------------------------------------------------------------------------------
// bool Formation::SetRefObject(GmatBase *obj, const UnsignedInt type,
//                              const std::string &name)
//------------------------------------------------------------------------------
/**
 * Accessor used to set or replace a referenced object.
 *
 * @param <obj>   the reference object.
 * @param <type>  reference object type.
 * @param <name>  reference object name.
 *
 * @return true on success.
 */
//------------------------------------------------------------------------------
bool Formation::SetRefObject(GmatBase *obj, const UnsignedInt type,
                             const std::string &name)
{
   if (!obj) return false;
   if (type == Gmat::FORMATION)
      throw SpaceObjectException("GMAT does not allow Formations of Formations: \"" +
         instanceName + "\".");
   if (type != Gmat::SPACECRAFT)
      return FormationInterface::SetRefObject(obj, type, name);
   if (!obj->IsOfType(Gmat::SPACECRAFT)) return false;

   auto *spacecraft = static_cast<SpaceObject *>(obj);
   if (std::find(componentNames.begin(), componentNames.end(), spacecraft->GetName()) == componentNames.end())
      return false;

   // Rebinding a same-name spacecraft replaces its old pointer. Build the
   // candidate in Add order, independent of reference-resolution order.
   std::vector<SpaceObject *> next;
   next.reserve(componentNames.size());
   for (const auto &configured : componentNames)
   {
      if (configured == spacecraft->GetName())
         next.push_back(spacecraft);
      else
      {
         auto prior = std::find_if(components.begin(), components.end(),
            [&](SpaceObject *member) { return member && member->GetName() == configured; });
         if (prior != components.end()) next.push_back(*prior);
      }
   }
   MemberStateSize(next, instanceName);
   CheckMemberEpochs(next, instanceName);
   auto *first = next.front();
   const bool changed = next != components || state.GetEpoch() != first->GetEpoch() ||
      state.GetEpochGT() != first->GetEpochGT();
   components.swap(next);
   satCount = components.size();
   if (changed)
   {
      // A copied dimension is not the size of the newly resolved layout.
      // Keep the copied state for buffers, but require BuildState for new refs.
      dimension = 0;
      parmsChanged = true;
   }
   state.SetEpoch(first->GetEpoch());
   state.SetEpochGT(first->GetEpochGT());
   state.SetPrecisionTimeFlag(state.HasPrecisionTime() || first->HasPrecisionTime());
   return true;
}


//------------------------------------------------------------------------------
// bool Formation::SetRefObject(GmatBase *obj, const UnsignedInt type,
//                              const std::string &name)
//------------------------------------------------------------------------------
/**
 * Accessor used to set or replace a referenced object.
 *
 * @param <obj>   the reference object.
 * @param <type>  reference object type.
 * @param <name>  reference object name.
 * @param <index> index specifying which array is used.
 *
 * @return true on success.
 *
 * @note This method is implemented to ensure that polymorphism doen't confuse
 *       some compilers.
 */
//------------------------------------------------------------------------------
bool Formation::SetRefObject(GmatBase *obj, const UnsignedInt type,
                                    const std::string &name,
                                    const Integer index)
{
   return FormationInterface::SetRefObject(obj, type, name, index);
}


//------------------------------------------------------------------------------
//  ObjectArray& GetRefObjectArray(const UnsignedInt type)
//------------------------------------------------------------------------------
/**
 * Obtains an array of GmatBase pointers by type.
 *
 * @param <type> The type of objects requested
 *
 * @return Reference to the array.
 */
//------------------------------------------------------------------------------
ObjectArray& Formation::GetRefObjectArray(const UnsignedInt type)
{
   static ObjectArray oa;
   oa.clear();
   if ((type == Gmat::SPACECRAFT) || (type == Gmat::FORMATION))
   {
      for (std::vector<SpaceObject *>::iterator i = components.begin();
           i != components.end(); ++i)
         if ((*i)->GetType() == type)
            oa.push_back(*i);
      return oa;
   }

   if (type == Gmat::SPACEOBJECT)
   {
      for (std::vector<SpaceObject *>::iterator i = components.begin();
           i != components.end(); ++i)
         oa.push_back(*i);
      return oa;
   }
      
   return FormationInterface::GetRefObjectArray(type);
}


//------------------------------------------------------------------------------
//  ObjectArray& GetRefObjectArray(const std::string& typeString)
//------------------------------------------------------------------------------
/**
 * Obtains an array of GmatBase pointers by type.
 *
 * @param <typeString> The type name of objects requested
 *
 * @return Reference to the array.
 */
//------------------------------------------------------------------------------
ObjectArray& Formation::GetRefObjectArray(const std::string& typeString)
{
   UnsignedInt id = Gmat::UNKNOWN_OBJECT;
   if (typeString == "Spacecraft") 
      id = Gmat::SPACECRAFT;
   if (typeString == "Formation") 
      id = Gmat::FORMATION;
   if (typeString == "SpaceObject") 
      id = Gmat::SPACEOBJECT;
      
   if (id != Gmat::UNKNOWN_OBJECT)
      return GetRefObjectArray(id);
      
   return FormationInterface::GetRefObjectArray(typeString);
}


//------------------------------------------------------------------------------
// void BuildState()
//------------------------------------------------------------------------------
/**
 * Constructs a GmatState for a Formation.
 */
//------------------------------------------------------------------------------
void Formation::BuildState()
{
   auto ordered = OrderedMembers(componentNames, components, instanceName);
   const Integer newDimension = MemberStateSize(ordered, instanceName);
   if (newDimension > std::numeric_limits<Integer>::max() - FORMATION_CARTESIAN_STATE)
      throw SpaceObjectException("Formation Cartesian parameter indices overflow for \"" + instanceName + "\".");
   CheckMemberEpochs(ordered, instanceName);

   // Recompute from resolved members, never from a copied or previously
   // accumulated dimension. All bytes are initialized and exactly sized.
   std::vector<Real> data(newDimension);
   Integer offset = 0;
   for (auto *member : ordered)
   {
      GmatState &memberState = member->GetState();
      const Integer size = memberState.GetSize();
      std::copy(memberState.GetState(), memberState.GetState() + size, data.begin() + offset);
      offset += size;
   }
   GmatState next(state);
   next.SetSize(newDimension);
   if (!next.SetState(data.data(), newDimension))
      throw SpaceObjectException("Error building formation state for \"" + instanceName + "\".");
   next.SetEpoch(ordered.front()->GetEpoch());
   next.SetEpochGT(ordered.front()->GetEpochGT());

   state = next;
   components.swap(ordered);
   dimension = newDimension;
   satCount = components.size();
}


//------------------------------------------------------------------------------
// void UpdateElements()
//------------------------------------------------------------------------------
/**
 * Updates the member SpaceObjects using the data in the Formation GmatState.
 */
//------------------------------------------------------------------------------
void Formation::UpdateElements()
{
   const Integer size = MemberStateSize(components, instanceName);
   CheckMemberOrder(componentNames, components, instanceName);
   if (dimension != size || state.GetSize() != size)
      throw SpaceObjectException("Rebuild formation \"" + instanceName +
         "\" before updating its members after a layout change.");

   // Check the entire layout before writing any member, avoiding a partial
   // update or an out-of-bounds memcpy after membership or state-size changes.
   Integer offset = 0;
   for (auto *member : components)
   {
      GmatState &memberState = member->GetState();
      const Integer memberSize = memberState.GetSize();
      std::copy(state.GetState() + offset, state.GetState() + offset + memberSize,
         memberState.GetState());
      offset += memberSize;
   }
}


//------------------------------------------------------------------------------
// void UpdateState()
//------------------------------------------------------------------------------
/**
 * Updates the internal GmatState data from the member SpaceObjects.
 */
//------------------------------------------------------------------------------
void Formation::UpdateState()
{
   // Propagation stop-condition buffers deliberately have copied state and
   // epoch but no member bindings. Preserve that snapshot; do not set epoch 0.
   if (components.empty()) return;
   const Integer size = MemberStateSize(components, instanceName);
   if (dimension != size || state.GetSize() != size)
   {
      BuildState();
      return;
   }
   CheckMemberOrder(componentNames, components, instanceName);
   CheckMemberEpochs(components, instanceName);
   Integer offset = 0;
   for (auto *member : components)
   {
      GmatState &memberState = member->GetState();
      const Integer memberSize = memberState.GetSize();
      std::copy(memberState.GetState(), memberState.GetState() + memberSize,
         state.GetState() + offset);
      offset += memberSize;
   }
   state.SetEpoch(components.front()->GetEpoch());
   state.SetEpochGT(components.front()->GetEpochGT());
}

//------------------------------------------------------------------------------
// bool Formation::IsManeuvering()
//------------------------------------------------------------------------------
/**
 * Checks to see if any formation members are maneuvering
 *
 * @return true if a member is maneuvering, false if not
 */
//------------------------------------------------------------------------------
bool Formation::IsManeuvering()
{
   bool retval = false;

   for (UnsignedInt i = 0; i < components.size(); ++i)
      if (components[i]->IsManeuvering())
         retval = true;

   return retval;
}

//------------------------------------------------------------------------------
// const StringArray& GetManeuveringMembers()
//------------------------------------------------------------------------------
/**
 * Retrieves the list of maneuvering Formation members
 *
 * @return The list
 */
//------------------------------------------------------------------------------
const StringArray& Formation::GetManeuveringMembers()
{
   maneuveringMembers.clear();

   for (UnsignedInt i = 0; i < components.size(); ++i)
      if (components[i]->IsManeuvering())
         maneuveringMembers.push_back(components[i]->GetName());

   return maneuveringMembers;
}


//------------------------------------------------------------------------------
// bool TakeAction(const std::string &action, const std::string &actionData)
//------------------------------------------------------------------------------
/**
 * This method performs action.
 *
 * @param <action> action to perform
 * @param <actionData> action data associated with action
 *
 * @return true if action successfully performed
 */
//------------------------------------------------------------------------------
bool Formation::TakeAction(const std::string &action,
                           const std::string &actionData)
{
   if (action == "Clear")
   {
      return ClearSpacecraftList();
   }
   else if (action == "Remove")
   {
      return RemoveSpacecraft(actionData);
   }
   
   return false;
}


void Formation::ClearLastStopTriggered()
{
   lastStopTriggered.clear();
   // Clear it on all members
   for (std::vector <SpaceObject *>::iterator i = components.begin(); 
        i != components.end(); ++i)
      (*i)->ClearLastStopTriggered();
      
   #ifdef DEBUG_STOPCONDITION_TRACKING
      MessageInterface::ShowMessage(
         "Cleared stop identifier from Formation \"%s\"\n", 
         instanceName.c_str());
   #endif
}


//------------------------------------------------------------------------------
// bool ClearSpacecraftList()
//------------------------------------------------------------------------------
/*
 * Clears the list of spacecraft.
 *
 * @return true if the list was cleared.
 */
//------------------------------------------------------------------------------
bool Formation::ClearSpacecraftList()
{
   #ifdef DEBUG_FORMATION_ACTIONS
      MessageInterface::ShowMessage(
         "Formation::ClearSpacecraftList() called\n");
   #endif
   componentNames.clear();
   components.clear();
   dimension = 0;
   satCount = 0;
   parmsChanged = true;
   state.SetSize(6); // SpaceObject retains its empty six-element placeholder.
   const Real emptyState[6] = {};
   state.SetState(emptyState, 6);
   return true;
}


//------------------------------------------------------------------------------
// bool RemoveSpacecraft(const std::string &name)
//------------------------------------------------------------------------------
/*
 * Removes spacecraft from the spacecraft list
 *
 * @param <name> spacecraft name to be removed from the list
 *
 * @return true if spacecraft was removed from the list, false otherwise
 */
//------------------------------------------------------------------------------
bool Formation::RemoveSpacecraft(const std::string &name)
{
   #ifdef DEBUG_FORMATION_ACTIONS
      MessageInterface::ShowMessage("%s%s\"\n",
         "Formation::RemoveSpacecraft called to remove \"", name.c_str());
   #endif

   StringArray::iterator scPos = 
      find(componentNames.begin(), componentNames.end(), name);
   
   if (scPos != componentNames.end())
   {
      componentNames.erase(scPos);

      // Now remove the pointer from the component list
      std::vector<SpaceObject *>::iterator j;
      SpaceObject *current;
      for (j = components.begin(); j < components.end(); ++j)
      {
         current = *j;
         if (current->GetName() == name)
         {
            components.erase(j);
            break;
         }
      }
      dimension = 0;
      satCount = components.size();

      #ifdef DEBUG_FORMATION_ACTIONS
         MessageInterface::ShowMessage("%s%s%s\n",
            "Formation \"", instanceName.c_str(),
            "\" now consists of these spacecraft names:");
         for (StringArray::iterator i = componentNames.begin(); 
              i != componentNames.end(); ++i)
            MessageInterface::ShowMessage("    \"%s\"\n", i->c_str());
     
         MessageInterface::ShowMessage("%s%s%s\n",
            "Formation \"", instanceName.c_str(),
            "\" now consists of these spacecraft:");
         for (j = components.begin(); j < components.end(); ++j)
         {
            MessageInterface::ShowMessage(
               "    \"%s\"\n", (*j)->GetName().c_str());
         }
      #endif

      parmsChanged = true;
      return true;
   }

   #ifdef DEBUG_FORMATION_ACTIONS
      MessageInterface::ShowMessage("%s%s\"\n",
         "Formation::RemoveSpacecraft Did not find \"", name.c_str());
   #endif

   return false;
}


Integer Formation::SetPropItem(const std::string &propItem)
{
   if (propItem == "CartesianState")
      return Gmat::CARTESIAN_STATE;
   if (propItem == "STM")
      return Gmat::ORBIT_STATE_TRANSITION_MATRIX;
   
   return FormationInterface::SetPropItem(propItem);
}


StringArray Formation::GetDefaultPropItems()
{
   StringArray defaults = FormationInterface::GetDefaultPropItems();
   defaults.push_back("CartesianState");
   return defaults;
}


Real* Formation::GetPropItem(const Integer item)
{
   Real* retval = NULL;
   switch (item)
   {
      case Gmat::CARTESIAN_STATE:
         retval = state.GetState();
         break;
         
      case Gmat::ORBIT_STATE_TRANSITION_MATRIX:
//         retval = stm;
         break;
         
      case Gmat::MASS_FLOW:
         // todo: Access tanks for mass information to handle mass flow
         break;
         
      // All other values call up the class heirarchy
      default:
         retval = FormationInterface::GetPropItem(item);
   }
   
   return retval;
}

Integer Formation::GetPropItemSize(const Integer item)
{
   Integer retval = -1;
   switch (item)
   {
      case Gmat::CARTESIAN_STATE:
         retval = dimension;
         break;
         
      case Gmat::ORBIT_STATE_TRANSITION_MATRIX:
         retval = 36 * satCount;
         break;
         
      case Gmat::MASS_FLOW:
         // todo: Access tanks for mass information to handle mass flow
         break;
         
      // All other values call up the heirarchy
      default:
         retval = FormationInterface::GetPropItemSize(item);
   }
   
   return retval;
}











//---------------------------------------------------------------------------
//  Real GetRealParameter(const Integer id) const
//---------------------------------------------------------------------------
/**
 * Retrieve the value for a Real parameter.
 *
 * @param <id> The integer ID for the parameter.
 *
 * @return The parameter's value.
 */
Real Formation::GetRealParameter(const Integer id) const
{
   #ifdef DEBUG_GET_REAL
      MessageInterface::ShowMessage(
      "In Formation::GetReal, asking for parameter %d, whose string is \"%s\"\n", 
      id, (GetParameterText(id)).c_str());
      //for (Integer i=0; i<6;i++)
      //   MessageInterface::ShowMessage("   state(%d) = %.12f\n",
      //   i, state[i]);
      //MessageInterface::ShowMessage("    and stateType = %s\n",
      //   stateType.c_str());
   #endif

   if (id == EPOCH_PARAM) return const_cast<Formation *>(this)->GetEpoch();
   if (id >= FORMATION_CARTESIAN_STATE && id - FORMATION_CARTESIAN_STATE < dimension)
   {
      if (HasLiveMembers())
         return MemberElement(componentNames, components, dimension,
            id - FORMATION_CARTESIAN_STATE, instanceName);
      return state[id - FORMATION_CARTESIAN_STATE];
   }
   
   return FormationInterface::GetRealParameter(id);
}

//---------------------------------------------------------------------------
//  Real GetRealParameter(const std::string &label) const
//---------------------------------------------------------------------------
/**
 * Retrieve the value for a Real parameter.
 *
 * @param <label> The label of the parameter.
 *
 * @return The parameter's value.
 */
Real Formation::GetRealParameter(const std::string &label) const
{
   // Performance!
    if (label == "A1Epoch")
       return const_cast<Formation *>(this)->GetEpoch();
   
    return GetRealParameter(GetParameterID(label));
}

////---------------------------------------------------------------------------
////  Real SetRealParameter(const Integer id, const Real value)
////---------------------------------------------------------------------------
///**
// * Set the value for a Real parameter.
// *
// * @param <id> The integer ID for the parameter.
// * @param <value> The new parameter value.
// *
// * @return the parameter value at the end of this call, or 
// *         REAL_PARAMETER_UNDEFINED if the parameter id is invalid or the 
// *         parameter type is not Real.
// */
//Real Formation::SetRealParameter(const Integer id, const Real value)
//{
//   #ifdef DEBUG_SPACECRAFT_SET
//   MessageInterface::ShowMessage("In Formation::SetRealParameter, "
//         "id = %d and value = %.12f\n", id, value);
//   #endif
//   if (id >= CARTESIAN_STATE )
//   {
//      state[id - FORMATION_CARTESIAN_STATE] = value;
//      return state[id - FORMATION_CARTESIAN_STATE];
//   }
//   
//   return FormationInterface::SetRealParameter(id, value);
//}
//
////---------------------------------------------------------------------------
////  Real SetRealParameter(const std::string &label, const Real value)
////---------------------------------------------------------------------------
///**
// * Set the value for a Real parameter.
// *
// * @param <label> The label of the parameter.
// * @param <value> The new parameter value.
// *
// * @return the parameter value at the end of this call, or 
// *         REAL_PARAMETER_UNDEFINED if the parameter id is invalid or the 
// *         parameter type is not Real.
// */
//Real Formation::SetRealParameter(const std::string &label, const Real value)
//{
//   #ifdef DEBUG_SPACECRAFT_SET
//   MessageInterface::ShowMessage
//      ("In Formation::SetRealParameter(label), label = %s and value = %.12f\n",
//       label.c_str(), value);
//   #endif
//
//   
//   return FormationInterface::SetRealParameter(label, value);
//}



