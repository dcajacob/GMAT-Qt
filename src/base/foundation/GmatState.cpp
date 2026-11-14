//$Id$
//------------------------------------------------------------------------------
//                                  GmatState
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
 * Implementation of the GmatState class.  This is the class for state data used 
 * in GMAT's propagators and solvers.
 */
//------------------------------------------------------------------------------

#include "GmatState.hpp"
#include "GmatBaseException.hpp"
#include "GmatConstants.hpp"
#include "RealUtilities.hpp"

#include <sstream>
#include <algorithm>
#include <cstring>
#include <memory>
#include <utility>
#include "MessageInterface.hpp"

//#define DEBUG_STATE_LOADING


//------------------------------------------------------------------------------
// public methods
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// GmatState(Integer size)
//------------------------------------------------------------------------------
/**
 * Default constructor for the GmatState.
 *
 * @param size Size of the requested state vector.
 */
//------------------------------------------------------------------------------
GmatState::GmatState(Integer size) :
   theEpoch          (GmatTimeConstants::MJD_OF_J2000),
   theEpochGT        (GmatTimeConstants::MJD_OF_J2000),
   hasPrecisionTime  (false),
   theData           (nullptr),
   dataIDs           (nullptr),
   associatedElements(nullptr),
   stateSize         (0),
   layoutIdentity    (std::make_shared<const int>(0))
{
   if (size < 0) throw GmatBaseException("GmatState requested a negative size.");
   if (size > 0) Resize(size, false);
}

//------------------------------------------------------------------------------
// ~GmatState()
//------------------------------------------------------------------------------
/**
 * Destructor for the GmatState.
 */
//------------------------------------------------------------------------------
GmatState::~GmatState()
{
   if (theData != NULL)
      delete [] theData;

   if (dataIDs != NULL)
      delete [] dataIDs;
   if (associatedElements != NULL)
      delete [] associatedElements;
}

//------------------------------------------------------------------------------
// GmatState(const GmatState& gs)
//------------------------------------------------------------------------------
/**
 * Copy constructor for the GmatState.
 * 
 * @param gs The state that is copied here
 */
//------------------------------------------------------------------------------
GmatState::GmatState(const GmatState& gs) : GmatState(gs.stateSize)
{
   theEpoch = gs.theEpoch;
   theEpochGT = gs.theEpochGT;
   hasPrecisionTime = gs.hasPrecisionTime;
   if (stateSize)
   {
      std::memcpy(theData, gs.theData, stateSize * sizeof(Real));
      std::memcpy(dataIDs, gs.dataIDs, stateSize * sizeof(Integer));
      std::memcpy(associatedElements, gs.associatedElements, stateSize * sizeof(Integer));
   }
   dataTypes = gs.dataTypes;
}

//------------------------------------------------------------------------------
// GmatState& operator=(const GmatState& gs)
//------------------------------------------------------------------------------
/**
 * Assignment operator for the GmatState.
 * 
 * @param gs The state that is copied here
 */
//------------------------------------------------------------------------------
GmatState& GmatState::operator=(const GmatState& gs)
{
   if (this != &gs)
   {
      // All allocation/copy work completes before the old storage is retired.
      GmatState next(gs);
      Swap(next);
   }
   return *this;
}

std::shared_ptr<const void> GmatState::GetLayoutIdentity() const
{
   return layoutIdentity;
}

void GmatState::Swap(GmatState &other) noexcept
{
   if (&other == this) return;
   using std::swap;
   swap(theEpoch, other.theEpoch);
   swap(theEpochGT, other.theEpochGT);
   swap(hasPrecisionTime, other.hasPrecisionTime);
   swap(theData, other.theData);
   swap(dataIDs, other.dataIDs);
   swap(associatedElements, other.associatedElements);
   dataTypes.swap(other.dataTypes);
   swap(stateSize, other.stateSize);
   layoutIdentity.swap(other.layoutIdentity);
}


//------------------------------------------------------------------------------
//       OPERATOR OVERLOADS
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Real& operator[](const Integer index)
//------------------------------------------------------------------------------
/**
 * Array element accessor used to set element values
 * 
 * @param index The index to the desired element
 */
//------------------------------------------------------------------------------
Real& GmatState::operator[](const Integer index)
{
   if ((index < 0 ) || (index >= stateSize))
      throw GmatBaseException("GmatState array index out of bounds");
   
   return theData[index];
}


//------------------------------------------------------------------------------
// Real operator[](const Integer index) const
//------------------------------------------------------------------------------
/**
 * Array element accessor used to retrieve element values
 * 
 * @param index The index to the desired element
 */
//------------------------------------------------------------------------------
Real GmatState::operator[](const Integer index) const
{
   if ((index < 0 ) || (index >= stateSize))
      throw GmatBaseException("GmatState array index out of bounds");
   
   return theData[index];
}


//------------------------------------------------------------------------------
//       PUBLIC METHODS
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// void SetSize(const Integer size)
//------------------------------------------------------------------------------
/**
 */
//------------------------------------------------------------------------------
void GmatState::SetSize(const Integer size)
{
   if (size > 0)
      Resize(size);
   else
      throw GmatBaseException("State resizing to a value less than or equal "
            "to zero is not allowed");
}


//------------------------------------------------------------------------------
// Integer GetSize() const
//------------------------------------------------------------------------------
/**
 */
//------------------------------------------------------------------------------
Integer GmatState::GetSize() const
{
   return stateSize;
}


//------------------------------------------------------------------------------
// Real* GetState()
//------------------------------------------------------------------------------
/**
 */
//------------------------------------------------------------------------------
Real* GmatState::GetState()
{
   return theData;
}


//------------------------------------------------------------------------------
// bool SetState(const Real *data, const Integer size, const Integer start)

//------------------------------------------------------------------------------
/**
 */
//------------------------------------------------------------------------------
bool GmatState::SetState(const Real *data, const Integer size, 
                           const Integer start)
{
   if (start < 0)
      throw GmatBaseException(
            "Cannot set state data -- starting index is out of range");
   if (start + size > stateSize)
      throw GmatBaseException(
            "Cannot set state data -- data span is out of range");
   
   memcpy(&(theData[start]), data, size*sizeof(Real));
   
   return true;
}


//------------------------------------------------------------------------------
// GmatEpoch GetEpoch() const
//------------------------------------------------------------------------------
/**
 */
//------------------------------------------------------------------------------
GmatEpoch GmatState::GetEpoch() const
{
   return theEpoch;
}


GmatTime GmatState::GetEpochGT() const
{
   return theEpochGT;
}


//------------------------------------------------------------------------------
// GmatEpoch SetEpoch(const GmatEpoch ep)
//------------------------------------------------------------------------------
/**
 */
//------------------------------------------------------------------------------
GmatEpoch GmatState::SetEpoch(const GmatEpoch ep)
{
   theEpoch = ep;
   return theEpoch;
}


GmatTime GmatState::SetEpochGT(const GmatTime ep)
{
   theEpochGT = ep;
   return theEpochGT;
}


bool GmatState::SetPrecisionTimeFlag(bool onOff)
{
   std::shared_ptr<const void> nextIdentity(layoutIdentity);
   if (onOff != hasPrecisionTime) nextIdentity = std::make_shared<const int>(0);

   if ((hasPrecisionTime == false)&& (onOff == true))
      if (!GmatMathUtil::IsEqual(theEpochGT.GetMjd(), theEpoch, 1e-6))
         theEpochGT = theEpoch; // only set epoch if they differ by more than 1 us
      
   hasPrecisionTime = onOff;
   layoutIdentity.swap(nextIdentity);
   return hasPrecisionTime;
}


bool GmatState::HasPrecisionTime() const
{
   return hasPrecisionTime;
}


bool GmatState::SetElementProperties(const Integer index, const Integer id, 
                                     const std::string &textId, 
                                     const Integer associate)
{
   #ifdef DEBUG_STATE_LOADING
      MessageInterface::ShowMessage(
            "Setting element %d, id = %d, desc = %s, assoc = %d\n", index, id, 
            textId.c_str(), associate);
   #endif
      
   if ((index < 0) || (index >= stateSize))
      throw GmatBaseException("Cannot set state element properties: "
                  "index out of range.");
   
   if (dataIDs[index] == id && dataTypes[index] == textId &&
         associatedElements[index] == associate) return true;
   std::string nextText(textId);
   std::shared_ptr<const void> nextIdentity = std::make_shared<const int>(0);
   dataIDs[index] = id;
   dataTypes[index].swap(nextText);
   associatedElements[index] = associate;
   layoutIdentity.swap(nextIdentity);
   
   return true;
}

const StringArray& GmatState::GetElementDescriptions()
{
   return dataTypes;
}

//------------------------------------------------------------------------------
// void Resize(Integer newSize, bool withCopy)
//------------------------------------------------------------------------------
/**
 */
//------------------------------------------------------------------------------
void GmatState::Resize(Integer newSize, bool withCopy)
{
   if (newSize == stateSize) return;
   if (newSize <= 0) throw GmatBaseException("GmatState Resize requested an invalid size");
   std::unique_ptr<Real[]> nextData(new Real[newSize]());
   std::unique_ptr<Integer[]> nextIDs(new Integer[newSize]);
   std::unique_ptr<Integer[]> nextAssociates(new Integer[newSize]);
   std::fill_n(nextIDs.get(), newSize, -1);
   std::fill_n(nextAssociates.get(), newSize, -1);
   StringArray nextTypes(newSize, "");
   std::shared_ptr<const void> nextIdentity = std::make_shared<const int>(0);
   if (withCopy)
   {
      const Integer count = std::min(newSize, stateSize);
      if (count)
      {
         std::memcpy(nextData.get(), theData, count * sizeof(Real));
         std::memcpy(nextIDs.get(), dataIDs, count * sizeof(Integer));
         std::memcpy(nextAssociates.get(), associatedElements, count * sizeof(Integer));
         for (Integer i = 0; i < count; ++i) nextTypes[i] = dataTypes[i];
      }
   }
   std::unique_ptr<Real[]> oldData(theData);
   std::unique_ptr<Integer[]> oldIDs(dataIDs), oldAssociates(associatedElements);
   theData = nextData.release();
   dataIDs = nextIDs.release();
   associatedElements = nextAssociates.release();
   dataTypes.swap(nextTypes);
   stateSize = newSize;
   layoutIdentity.swap(nextIdentity);
}


Integer GmatState::GetAssociateIndex(const Integer id)
{
   #ifdef DEBUG_ASSOCIATE_INDICES
      MessageInterface::ShowMessage("GmatState::GetAssociateIndex State "
            "Size = %d, id = %d\n", stateSize, id);
   #endif

   if ((id < 0) || (id >= stateSize))
   {
      std::stringstream msg;
      msg << "Requested element association index " << id
          << " is out of bounds for the GMAT state vector, which has size "
          << stateSize;
      throw GmatBaseException(msg.str());
   }
   return associatedElements[id];
}


//------------------------------------------------------------------------------
//       PROTECTED METHODS
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// void Zero(Integer begin, UnsignedInt length)
//------------------------------------------------------------------------------
/**
 */
//------------------------------------------------------------------------------
void GmatState::Zero(Integer begin, UnsignedInt length)
{
   if ((begin < 0) || ((Integer)(begin + length) > stateSize))
   {
      std::stringstream errmsg;
      errmsg << "GmatState request to zero " << length
             << " elements starting at element " << begin 
             << " exceeds the state size, which is " << stateSize;
      throw GmatBaseException(errmsg.str());
   }
   
   for (Integer i = begin; i < (Integer)(begin + length); ++i)
   {
      theData[i]    = 0.0;
   }
}

void GmatState::ValidateRestorationLayout(const GmatState &source) const
{
   if (stateSize != source.stateSize || dataTypes != source.dataTypes ||
       hasPrecisionTime != source.hasPrecisionTime ||
       (stateSize && (!theData || !source.theData || !dataIDs || !source.dataIDs ||
                     !associatedElements || !source.associatedElements)))
   {
      std::stringstream message;
      message << "Restoration requires the same native state layout. Source has "
              << source.stateSize << " elements (precision time "
              << source.hasPrecisionTime << "); destination has " << stateSize
              << " elements (precision time " << hasPrecisionTime << ").";
      throw GmatBaseException(message.str());
   }
   for (Integer i = 0; i < stateSize; ++i)
      if (dataIDs[i] != source.dataIDs[i] || associatedElements[i] != source.associatedElements[i])
         throw GmatBaseException("Restoration cannot change an active state element role.");
}

void GmatState::RestoreValues(const GmatState &source)
{
   if (&source == this) return;
   ValidateRestorationLayout(source);
   for (Integer i = 0; i < stateSize; ++i) theData[i] = source.theData[i];
   theEpoch = source.theEpoch;
   theEpochGT = source.theEpochGT;
   // State, ID and association buffers retain their existing storage.
}
