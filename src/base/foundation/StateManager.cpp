//$Id$
//------------------------------------------------------------------------------
//                                  StateManager
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool.
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
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under contract
// number NNG06CA54C
//
// Author: Darrel J. Conway, Thinking Systems, Inc.
// Created: 2008/12/15
//
/**
 * Imnplementation of the StateManager base class.  This is the class for state 
 * managers used in GMAT's propagators and solvers.
 */
//------------------------------------------------------------------------------

#include "StateManager.hpp"
#include <set>
#include "MessageInterface.hpp"
#include "GmatBase.hpp"
#include "Spacecraft.hpp"
#include <sstream>
#include <memory>
#include "GmatBaseException.hpp"

namespace
{
struct StagedStateMap
{
   std::vector<std::unique_ptr<ListItem>> owners;
   std::vector<ListItem*> entries;
   explicit StagedStateMap(const std::vector<ListItem*> &source)
   {
      owners.reserve(source.size());
      entries.reserve(source.size());
      for (const ListItem *item : source)
      {
         owners.emplace_back(item ? new ListItem(*item) : nullptr);
         entries.push_back(owners.back().get());
      }
   }
   void Adopt(std::vector<ListItem*> &target)
   {
      target.swap(entries);
      for (auto &owner : owners) owner.release();
   }
};
}


//#define DEBUG_STATE_ACCESS
//#define DEBUG_PUBLISHED_INFO
//#define DEBUG_PUBLISHED_DATA

//------------------------------------------------------------------------------
// StateManager(Integer size)
//------------------------------------------------------------------------------
/**
 * Constructor
 *
 * @param size The size of the state vector
 */
//------------------------------------------------------------------------------
StateManager::StateManager(Integer size) :
   stateSize      (size),
   state          (size),
   current        (NULL)
{
   objects.clear();
   epochIDs.clear();
   elements.clear();
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
/**
 */
//------------------------------------------------------------------------------
StateManager::~StateManager()
{
   ClearOwnedLists();
}

void StateManager::ClearStateMap()
{
   for (ListItem *item : stateMap) delete item;
   stateMap.clear();
}

void StateManager::ClearOwnedLists()
{
   for (const auto &entry : elements) delete entry.second;
   elements.clear();
   ClearStateMap();
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
/**
 */
//------------------------------------------------------------------------------
StateManager::StateManager(const StateManager& sm) :
   stateSize   (sm.stateSize),
   state       (sm.state),
   objectNames (sm.objectNames),
   current     (NULL)
{
   StagedStateMap copied(sm.stateMap);
   copied.Adopt(stateMap);
   objects.clear();
   epochIDs.clear();
   elements.clear();
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
/**
 */
//------------------------------------------------------------------------------
StateManager& StateManager::operator=(const StateManager& sm)
{
   if (this != &sm)
   {
      StagedStateMap copied(sm.stateMap);
      GmatState copiedState(sm.state);
      StringArray copiedNames(sm.objectNames);
      state = copiedState;
      ClearOwnedLists();
      // Preserve the native copy-start-empty object-registration policy.
      objects.clear();
      epochIDs.clear();
      current = NULL;
      stateSize = sm.stateSize;
      objectNames.swap(copiedNames);
      copied.Adopt(stateMap);
   }
   return *this;
}

void StateManager::RebindClonedObjectReferences(const StateManager &source,
      const std::function<GmatBase*(GmatBase*)> &resolve)
{
   if (&source == this) return;
   if (source.objects.size() != source.epochIDs.size())
      throw GmatBaseException("Inconsistent retained state-manager epoch registrations.");
   GmatState copiedState(source.state);
   StringArray names(source.objectNames);
   IntegerArray epochs(source.epochIDs);
   ObjectArray mappedObjects;
   std::map<GmatBase*, StringArray*> mappedProperties;
   std::map<GmatBase*, std::unique_ptr<StringArray>> owners;
   GmatBase *mappedCurrent = NULL;
   for (GmatBase *original : source.objects)
   {
      if (!original) throw GmatBaseException("Cannot retain a null propagation object.");
      GmatBase *object = resolve(original);
      if (!object || object->GetTypeName() != original->GetTypeName())
         throw GmatBaseException("Incompatible retained propagation object.");
      const auto properties = source.elements.find(original);
      if (properties == source.elements.end() || !properties->second)
         throw GmatBaseException("Missing retained propagation-property declaration.");
      auto value = std::make_unique<StringArray>(*properties->second);
      if (!owners.emplace(object, std::move(value)).second)
         throw GmatBaseException("Distinct propagation objects map to one retained target.");
      mappedObjects.push_back(object);
      mappedProperties.emplace(object, owners.at(object).get());
      if (original == source.current) mappedCurrent = object;
   }
   if (source.current && !mappedCurrent)
      throw GmatBaseException("Unregistered current object in retained state manager.");
   state = copiedState;
   ClearOwnedLists();
   // Prepared maps carry parameter/index/cache associations from the previous
   // engine frame. Keep numeric state and declarations; rebuild maps natively.
   objects.swap(mappedObjects);
   elements.swap(mappedProperties);
   objectNames.swap(names);
   epochIDs.swap(epochs);
   current = mappedCurrent;
   stateSize = source.stateSize;
   for (auto &owner : owners) owner.second.release();
}

//------------------------------------------------------------------------------
// Integer GetCount(Gmat::StateElementId elementType)
//------------------------------------------------------------------------------
/**
 * Returns the number of objects that support the specified type
 * 
 * This default version just returns the total number of unique objects managed
 * by the StateManager
 * 
 * @param elementType ID for the type of state element that is being queried.  
 *                    Gmat::UNKNOWN_STATE, the default, counts the total number
 *                    of objects being managed.
 * 
 * @return The count of the number of objects supporting the type specified
 */
//------------------------------------------------------------------------------
Integer StateManager::GetCount(Gmat::StateElementId elementType)
{
   ObjectArray pObjects;
 
   GetStateObjects(pObjects, Gmat::UNKNOWN_OBJECT);
   Integer count = pObjects.size();

   #ifdef DEBUG_STATE_ACCESS
      MessageInterface::ShowMessage(
            "StateManager::GetCount found %d objects supporting type %d\n",
            count, elementType);
   #endif
      
   return count;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
/**
 */
//------------------------------------------------------------------------------
bool StateManager::UpdateState() 
{
   return true;
}


//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
/**
 */
//------------------------------------------------------------------------------
GmatState* StateManager::GetState()
{
   return &state;
}


Integer StateManager::GetStateSize()
{
   return state.GetSize();
}


//------------------------------------------------------------------------------
// bool GetStateObjects(ObjectArray& pObjects, UnsignedInt type)
//------------------------------------------------------------------------------
/**
 * Fills in the pointers to the objects
 *
 * @param pObjects Array that will receive the object pointers
 * @param type The type of objects requested
 *
 * @return true if at least one object pointer was set
 */
//------------------------------------------------------------------------------
bool StateManager::GetStateObjects(ObjectArray& pObjects, UnsignedInt type)
{
   bool retval = false;
   
   if (type == Gmat::UNKNOWN_OBJECT)
   {
      for (ObjectArray::iterator i = objects.begin(); i != objects.end(); ++i)
      {
         if (find(pObjects.begin(), pObjects.end(), (*i)) == pObjects.end())
         {
            pObjects.push_back(*i);
            retval = true;
         }
      }
   }
   else
   {
      for (ObjectArray::iterator i = objects.begin(); i != objects.end(); ++i)
      {
         if ((*i)->IsOfType(type))
         {
            if (find(pObjects.begin(), pObjects.end(), (*i)) == pObjects.end())
            {
               pObjects.push_back(*i);
               retval = true;
            }
         }
      }
   }
   
   return retval;
}


const StringArray& StateManager::GetObjectList(std::string ofType)
{
   return objectNames;
}


const std::vector<ListItem*>* StateManager::GetStateMap()
{
   return &stateMap;
}



Integer StateManager::PrepareStateInfoToPublish(StringArray &ownerNames, StringArray &elementNames)
{
   #ifdef DEBUG_PUBLISHED_INFO
      MessageInterface::ShowMessage("Before: elementNems.size() = %d\n", elementNames.size());
      for (Integer i = 0; i < elementNames.size(); ++i)
         MessageInterface::ShowMessage("ownerNames[%d] = <%s>   elementNames[%d] = <%s>\n",
            i, ownerNames[i].c_str(), i, elementNames[i].c_str());
   #endif

   // Prepare for publishing all elements in propagation state managers
   // Note that: each PropSetup propapagtor[i] has its own propagation state manager

   StringArray stateNames;
   stateNames.push_back("X"); stateNames.push_back("Y"); stateNames.push_back("Z");
   stateNames.push_back("Vx"); stateNames.push_back("Vy"); stateNames.push_back("Vz");
   
   #ifdef DEBUG_PUBLISHED_INFO
      MessageInterface::ShowMessage("   stateMap.size() = %d\n", stateMap.size());
   #endif

   for (Integer i = 0; i < stateMap.size(); ++i)
   {
      ListItem *item = stateMap[i];

      #ifdef DEBUG_PUBLISHED_INFO
         MessageInterface::ShowMessage("item %d: object = <%s>   element = <%s>  subelement = %d\n", 
            i, item->objectName.c_str(), item->elementName.c_str(), item->subelement);
      #endif

         // Add owner's name
         //ownerNames.push_back(item->objectName);
         // Add element name of the owner
         std::stringstream ss;
         if (item->length == 1)
            ss << item->objectName << "." << item->elementName;
         if (item->object->IsOfType(Gmat::FORMATION) && (item->elementName == "CartesianState"))
         {
            Integer componentIndex = (item->subelement - 1) / 6;
            Integer componentElementIndex = item->subelement - 1;
            ownerNames.push_back(item->object->GetStringArrayParameter("Add")[componentIndex]);
            if (componentElementIndex > 5)
               componentElementIndex = componentElementIndex % 6;
            ss << item->object->GetStringArrayParameter("Add")[componentIndex] << "." << stateNames[componentElementIndex];
            //Nothing
         }
         else
         {
            ownerNames.push_back(item->objectName);
            if ((item->object->IsOfType(Gmat::SPACECRAFT)) && (item->elementName == "CartesianState"))
               ss << item->objectName << "." << stateNames[item->subelement - 1];
            else
               ss << item->objectName << "." << item->elementName << "." << item->subelement;
         }
         elementNames.push_back(ss.str());
   }

   // This is an example content of ownerNames and elementNames at this point:
   // ownerNames        elementNames
   // ------------------------------
   // EstSat1           EstSat1.X
   // EstSat1           EstSat1.Y
   // ....              ...
   // EstSat1           EstSat1.VZ
   // EstSat1           EstSat1.CR
   // EstSat2           EstSat2.X
   // EstSat2           EstSat2.Y
   // ....              ...
   // EstSat2           EstSat2.VZ

   Integer numOfElements = elementNames.size();

   #ifdef DEBUG_PUBLISHED_INFO
      MessageInterface::ShowMessage("After: elementNems.size() = %d\n", elementNames.size());
      for (Integer i = 0; i < elementNames.size(); ++i)
         MessageInterface::ShowMessage("ownerNames[%d] = <%s>   elementNames[%d] = <%s>\n", 
            i, ownerNames[i].c_str(), i, elementNames[i].c_str());
   #endif
   
   return numOfElements;
}



std::vector<StateManager::PublishedSpacecraft> StateManager::GetPublishedSpacecraft() const
{
   std::vector<PublishedSpacecraft> result;
   std::set<const GmatBase*> seen, formations;
   auto add = [&](GmatBase *object, const std::string &role)
   {
      if (!object || !object->IsOfType(Gmat::SPACECRAFT))
         throw GmatBaseException("Published spacecraft layout has an invalid member.\n");
      if (seen.insert(object).second) result.push_back({object, role});
   };
   for (const auto *item : stateMap)
   {
      if (!item || !item->object)
         throw GmatBaseException("Published state layout has a NULL object.\n");
      if (item->object->IsOfType(Gmat::SPACECRAFT)) add(item->object, item->objectName);
      else if (item->object->IsOfType(Gmat::FORMATION) && formations.insert(item->object).second)
      {
         // Copy native static-return storage before inspecting other references.
         const ObjectArray members = item->object->GetRefObjectArray(Gmat::SPACECRAFT);
         const StringArray roles = item->object->GetStringArrayParameter("Add");
         if (members.size() != roles.size())
            throw GmatBaseException("Published Formation member roles and references differ.\n");
         for (size_t i = 0; i < members.size(); ++i) add(members[i], roles[i]);
      }
   }
   return result;
}

Integer StateManager::PrepareCovAndAccelerationInfoToPublish(StringArray &ownerNames, StringArray &elementNames)
{
   const auto spacecraft = GetPublishedSpacecraft();
   const char *components[] = {"X", "Y", "Z", "Vx", "Vy", "Vz"};
   for (const auto &entry : spacecraft)
      for (Integer row = 0; row < 6; ++row)
         for (Integer col = 0; col <= row; ++col)
         {
            ownerNames.push_back(entry.role);
            elementNames.push_back(entry.role + ".C" + components[row] + components[col]);
         }
   for (const auto &entry : spacecraft)
      for (const auto *component : {"AccelerationX", "AccelerationY", "AccelerationZ"})
      {
         ownerNames.push_back(entry.role);
         elementNames.push_back(entry.role + "." + component);
      }
   return elementNames.size();
}

//------------------------------------------------------------------------------
// Integer PrepareAttitudeInfoToPublish(StringArray& ownerNames,
//                                      StringArray& elementNames)
//------------------------------------------------------------------------------
/**
 * Method to generate the info for the attitude elements of the published data
 *
 * @param ownerNames Names of the owners of the attitude elements
 * @param elementNames The names of the attitude elements [Q1, Q2, Q3, Q4]
 * 
 * @return The number of attitude elements for which info was generated
 *
 */
//------------------------------------------------------------------------------
Integer StateManager::PrepareAttitudeInfoToPublish(StringArray& ownerNames,
                                                   StringArray& elementNames)
{
   for (const auto &entry : GetPublishedSpacecraft())
      for (Integer i = 1; i <= 4; ++i)
      {
         ownerNames.push_back(entry.role);
         elementNames.push_back(entry.role + ".Q" + std::to_string(i));
      }
   return elementNames.size();
}


bool StateManager::PrepareStateDataToPublish(Real* publishData, Integer publishDataSize, StringArray& elementNames)
{
   StringArray stateNames;
   stateNames.push_back("X"); stateNames.push_back("Y"); stateNames.push_back("Z");
   stateNames.push_back("Vx"); stateNames.push_back("Vy"); stateNames.push_back("Vz");

   //Integer Index = len;

   // Publish value of each element handling by state manager sm 
   Integer size = state.GetSize();
   for (Integer i = 0; i < stateMap.size(); ++i)
   {
      ListItem *item = stateMap[i];

      #ifdef DEBUG_PUBLISHED_DATA
      MessageInterface::ShowMessage("item %d: object = <%s>   element = <%s>  subelement = %d\n",
         i, item->objectName.c_str(), item->elementName.c_str(), item->subelement);
      #endif

      std::stringstream ss;
      if (item->length == 1)
         ss << item->objectName << "." << item->elementName;
      if (item->object->IsOfType(Gmat::FORMATION) && (item->elementName == "CartesianState"))
      {
         Integer componentIndex = (item->subelement - 1) / 6;
         Integer componentElementIndex = item->subelement - 1;
         if (componentElementIndex > 5)
            componentElementIndex = componentElementIndex % 6;
         ss << item->object->GetStringArrayParameter("Add")[componentIndex] << "." << stateNames[componentElementIndex];
      }
      else
      {
         if ((item->object->IsOfType(Gmat::SPACECRAFT)) && (item->elementName == "CartesianState"))
            ss << item->objectName << "." << stateNames[item->subelement - 1];
         else
            ss << item->objectName << "." << item->elementName << "." << item->subelement;
      }
      std::string name = ss.str();
      
      bool found = false;
      Integer idx = 0;
      for (; idx < elementNames.size(); ++idx)
      {
         if (name == elementNames[idx])
         {
            found = true;
            break;
         }
      }

      if (found)
         publishData[idx] = state[i];
      // Skip filling data to publishData when element is not in publishing list.


      //if (index >= publishDataSize)
      //{
      //   std::stringstream ss;
      //   ss << "Error: Data is overflowed in StateManager::PrepareStateDataToPublish()"
      //      << " function when data are filled publishData array: publishDataSize = " 
      //      << publishDataSize << ". Fill data for stateMap[" << i 
      //      << "]: object name = <" << stateMap[i]->object->GetName()
      //      << ">     elementName = <" << stateMap[i]->elementName << ">   subelement = <" 
      //      << stateMap[i]->subelement << ">\n";
      //   throw GmatBaseException(ss.str());
      //}
      //
      //publishData[index] = state[i];
      //++index;
   }
   //len = index;

   #ifdef DEBUG_PUBLISHED_DATA
   for (Integer i = len; i < index; ++i)
      MessageInterface::ShowMessage("publishData[%d] = %.12lf\n", i, publishData[i]);
   #endif
   return true;
}



bool StateManager::PrepareCovAndAccelerationDataToPublish(Real* publishData, Integer publishDataSize,
   StringArray& elementNames, Integer covStartIndex, bool hasPrecisionTime)
{
   const auto spacecraft = GetPublishedSpacecraft();
   if (!publishData || covStartIndex < 0 || publishDataSize < covStartIndex ||
       spacecraft.size() > static_cast<size_t>(publishDataSize - covStartIndex) / 24)
      throw GmatBaseException("Published covariance/acceleration storage is incomplete.\n");
   // The declaration layout is all 21-element covariance blocks, followed by
   // all three-element acceleration blocks. Keep values in that exact order.
   const size_t accelerationStart = static_cast<size_t>(covStartIndex) + 21 * spacecraft.size();
   for (size_t i = 0; i < spacecraft.size(); ++i)
   {
      auto *sc = static_cast<Spacecraft*>(spacecraft[i].object);
      const Rmatrix phi = sc->GetRmatrixParameter("FullSTM");
      const Rmatrix covt0 = sc->GetRmatrixParameter("OrbitErrorCovariance");
      Rmatrix Pt0;
      Pt0.SetSize(phi.GetNumRows(), phi.GetNumColumns(), true);
      if (Pt0.GetNumRows() < 6 || Pt0.GetNumColumns() < 6)
         throw GmatBaseException("Published spacecraft covariance has fewer than six Cartesian components.\n");
      for (Integer row = 0; row < phi.GetNumRows(); ++row)
         for (Integer col = 0; col < phi.GetNumColumns(); ++col)
            if (row < covt0.GetNumRows() && col < covt0.GetNumColumns()) Pt0(row, col) = covt0(row, col);
      // Preserve the native covariance value policy: Pt0 already contains P(t).
      // This layout repair adds no STM multiplication or covariance transform.
      size_t index = static_cast<size_t>(covStartIndex) + 21 * i;
      for (Integer row = 0; row < 6; ++row)
         for (Integer col = 0; col <= row; ++col) publishData[index++] = Pt0(row, col);
      const Rvector3 acceleration = hasPrecisionTime ? sc->GetMJ2000Acceleration(state.GetEpochGT()) :
         sc->GetMJ2000Acceleration(state.GetEpoch());
      for (Integer j = 0; j < 3; ++j) publishData[accelerationStart + 3*i + j] = acceleration[j];
   }
   return true;
}

//------------------------------------------------------------------------------
// bool PrepareAttitudeDataToPublish(Real* publishData,
//                                   Integer publishDataSize,
//                                   StringArray& elementNames,
//                                   Integer attStartIndex,
//                                   bool hasPrecisionTime)
//------------------------------------------------------------------------------
/**
 * Method to prepare and add attitude data to the pusblish data array
 * 
 * @param publishData      The data array that attitude data will be added to
 * @param publishDataSize  The size of the data array
 * @param elementNames     The names for each of the data elements
 * @param attStartIndex    The index in the data array for which attitude data 
 *                         will be added to
 * @param hasPrecisionTime Whether GMAT precision time is in use
 * 
 * @return Whether data was successfully added
 *
 */
//------------------------------------------------------------------------------
bool StateManager::PrepareAttitudeDataToPublish(Real* publishData,
                                                Integer publishDataSize,
                                                StringArray& elementNames,
                                                Integer attStartIndex,
                                                bool hasPrecisionTime)
{
   const auto spacecraft = GetPublishedSpacecraft();
   if (!publishData || attStartIndex < 0 || publishDataSize < attStartIndex ||
       spacecraft.size() > static_cast<size_t>(publishDataSize - attStartIndex) / 4)
      throw GmatBaseException("Published attitude storage is incomplete.\n");
   for (size_t i = 0; i < spacecraft.size(); ++i)
   {
      auto *sc = static_cast<Spacecraft*>(spacecraft[i].object);
      auto *attitude = dynamic_cast<Attitude*>(sc->GetRefObject(Gmat::ATTITUDE, "Attitude"));
      if (!attitude) throw GmatBaseException("Published spacecraft has no attitude.\n");
      const Rvector quaternion = attitude->GetQuaternion(hasPrecisionTime ? state.GetEpochGT().GetMjd() : state.GetEpoch());
      if (quaternion.GetSize() < 4) throw GmatBaseException("Published attitude quaternion is incomplete.\n");
      for (Integer j = 0; j < 4; ++j) publishData[static_cast<size_t>(attStartIndex) + 4*i + j] = quaternion[j];
   }
   return true;
}
