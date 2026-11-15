//------------------------------------------------------------------------------
//                                  CCSDSOEMSegment
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
// Author: Linda Jun / NASA
// Created: 2016.01.07
//
/**
 * Stores, validates, and manages meta data and cartesian orbit vector read from,
 * or to be written to, a CCSDS Orbit Ephemeris Message file.
 */
//------------------------------------------------------------------------------
#include "CCSDSOEMSegment.hpp"
#include "MessageInterface.hpp"
#include "UtilityException.hpp"
#include "StringUtil.hpp"
#include "TimeSystemConverter.hpp"
#include "CcsdsOemB1Text.hpp"
#include "HermiteInterpolator.hpp"
#include <algorithm>
#include <sstream>

//#define DEBUG_OEM_VALIDATE
//#define DEBUG_OEM_SET
//#define DEBUG_OEM_DATA
//#define DEBUG_OEM_GET_STATE
//#define DEBUG_ADD_DATA_FOR_WRITING

//------------------------------------------------------------------------------
// static data
//------------------------------------------------------------------------------
// none

//------------------------------------------------------------------------------
// static methods
//------------------------------------------------------------------------------
// none

//------------------------------------------------------------------------------
// public methods
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// constructor (default)
//------------------------------------------------------------------------------
CCSDSOEMSegment::CCSDSOEMSegment(const std::string version, Integer segNum) :
   CCSDSEMSegment(version, segNum)
{
   if (formatVersion == "1.0")
      dataSize = 6;
   else if (formatVersion == "2.0")
      dataSize = 27;
}

//------------------------------------------------------------------------------
// copy constructor
//------------------------------------------------------------------------------
CCSDSOEMSegment::CCSDSOEMSegment(const CCSDSOEMSegment &copy) :
   CCSDSEMSegment(copy)
{
}

//------------------------------------------------------------------------------
// operator=
//------------------------------------------------------------------------------
CCSDSOEMSegment& CCSDSOEMSegment::operator=(const CCSDSOEMSegment &copy)
{
   if (&copy == this)
      return *this;
   
   CCSDSEMSegment::operator=(copy);

   return *this;
}

//------------------------------------------------------------------------------
// destructor
//------------------------------------------------------------------------------
CCSDSOEMSegment::~CCSDSOEMSegment()
{
}

//------------------------------------------------------------------------------
// Returns a clone of this object.
//------------------------------------------------------------------------------
CCSDSEMSegment* CCSDSOEMSegment::Clone() const
{
   return (new CCSDSOEMSegment(*this));
}

//------------------------------------------------------------------------------
// Validates the contents of the handled Cartesian meta data elements.
//------------------------------------------------------------------------------
bool CCSDSOEMSegment::Validate(bool checkData)
{
   #ifdef DEBUG_OEM_VALIDATE
   MessageInterface::ShowMessage
      ("CCSDSOEMSegment::Validate() entered, checkData=%d\n", checkData);
   #endif
   
   // Cartesian type
   const std::string method = GmatStringUtil::ToUpper(interpolationMethod);
   if (method != "LAGRANGE" && method != "HERMITE")
   {
      std::string errmsg = segError;
      errmsg += "Interpolation type \"";
      errmsg += interpolationMethod + "\" is not valid for Orbit type. ";
      errmsg += "Supported values are \"LAGRANGE\" and \"HERMITE\".\n";
      throw UtilityException(errmsg);
   }
   
   #ifdef DEBUG_OEM_VALIDATE
   MessageInterface::ShowMessage
      ("CCSDSOEMSegment::Validate() returning CCSDSEMSegment::Validate()\n");
   #endif
   if (method == "HERMITE" &&
         (interpolationDegree < 0 || interpolationDegree > 9 || (interpolationDegree > 0 && interpolationDegree % 2 == 0)))
      throw UtilityException(segError + "HERMITE interpolation requires an odd degree from 1 to 9.");
   return CCSDSEMSegment::Validate(checkData);
}


//------------------------------------------------------------------------------
// Returns the cartesin orbit vecot at the specified input time.
//------------------------------------------------------------------------------
Rvector6 CCSDSOEMSegment::GetState(Real atEpoch)
{
   #ifdef DEBUG_OEM_GET_STATE
   MessageInterface::ShowMessage
      ("CCSDSOEMSegment::GetState() entered, epoch = %lf, dataStore size = %d\n",
       atEpoch, (Integer) dataStore.size());
   #endif
   
   Rvector rvec = DetermineState(atEpoch);
   Rvector6 cart(rvec[0], rvec[1], rvec[2], rvec[3], rvec[4], rvec[5]);
   
   #ifdef DEBUG_OEM_GET_STATE
   MessageInterface::ShowMessage
      ("CCSDSOEMSegment::GetState() returning\n%s\n", cart.ToString().c_str());
   #endif
   
   return cart;
}

//------------------------------------------------------------------------------
// std::string GetMetaDataForWriting()
//------------------------------------------------------------------------------
std::string CCSDSOEMSegment::GetMetaDataForWriting()
{
   const bool b1 = formatVersion == "1.0";
   const bool hasInterpolation = !interpolationMethod.empty() && interpolationMethod != UNSET_STRING;
   const bool hasDegree = !interpolationDegreeStr.empty() && interpolationDegreeStr != UNSET_STRING;
   const bool hasUsableStart = !usableStartTimeStr.empty() && usableStartTimeStr != UNSET_STRING;
   const bool hasUsableStop = !usableStopTimeStr.empty() && usableStopTimeStr != UNSET_STRING;
   if (b1)
   {
      for (const auto &field : {
         std::make_pair("OBJECT_NAME", objectName), {"OBJECT_ID", objectID},
         {"CENTER_NAME", centerName}, {"REF_FRAME", refFrame},
         {"TIME_SYSTEM", timeSystem}, {"START_TIME", startTimeStr}, {"STOP_TIME", stopTimeStr}})
         CcsdsOemB1Text::RequireValue(field.first, field.second);
      if (hasUsableStart)
         CcsdsOemB1Text::RequireValue("USEABLE_START_TIME", usableStartTimeStr);
      if (hasUsableStop)
         CcsdsOemB1Text::RequireValue("USEABLE_STOP_TIME", usableStopTimeStr);
      if (hasInterpolation)
      {
         if (!hasDegree)
            throw UtilityException("CCSDS OEM 1.0 INTERPOLATION_DEGREE is required when INTERPOLATION is set");
         CcsdsOemB1Text::RequireValue("INTERPOLATION", interpolationMethod);
         CcsdsOemB1Text::RequireValue("INTERPOLATION_DEGREE", interpolationDegreeStr);
      }
      else if (hasDegree)
         CcsdsOemB1Text::RequireValue("INTERPOLATION_DEGREE", interpolationDegreeStr);
   }
   std::stringstream ss("");
   ss << std::endl << "META_START" << std::endl;
   for (unsigned int i = 0; i < metaComments.size(); i++)
   {
      if (b1)
         ss << CcsdsOemB1Text::Comment(metaComments[i]);
      else
         ss << "COMMENT  " << metaComments[i] << std::endl;
   }
   ss << "OBJECT_NAME          = " << objectName << std::endl;
   ss << CcsdsOemB1Text::MetadataLine("OBJECT_ID", objectID);

   if (formatVersion == "2.0")
   {
      ss << "CENTER_NAME          = " << GmatStringUtil::ToUpper(centerName) << std::endl;
      ss << "REF_FRAME            = " << GmatStringUtil::ToUpper(refFrame) << std::endl;
   }
   else
   {
      ss << "CENTER_NAME          = " << centerName << std::endl;
      ss << "REF_FRAME            = " << refFrame << std::endl;
   }

   ss << "TIME_SYSTEM          = " << timeSystem << std::endl;
   ss << "START_TIME           = " << startTimeStr << std::endl;
   if (!b1 || hasUsableStart)
      ss << "USEABLE_START_TIME   = " << usableStartTimeStr << std::endl;
   if (!b1 || hasUsableStop)
      ss << "USEABLE_STOP_TIME    = " << usableStopTimeStr << std::endl;
   ss << "STOP_TIME            = " << stopTimeStr << std::endl;
   if (!b1 || hasInterpolation)
      ss << "INTERPOLATION        = " << interpolationMethod << std::endl;
   if (!b1 || hasDegree)
      ss << "INTERPOLATION_DEGREE = " << interpolationDegreeStr << std::endl;
   ss << "META_STOP" << std::endl << std::endl;
   if (b1)
      CcsdsOemB1Text::CheckLines(ss.str());
   return ss.str();
}

std::string CCSDSOEMSegment::GetDataComments()
{
   if (formatVersion != "1.0")
      return CCSDSEMSegment::GetDataComments();
   std::string comments;
   for (const auto &comment : dataComments)
      comments += CcsdsOemB1Text::Comment(comment);
   if (!dataComments.empty()) comments += "\n";
   return comments;
}

//------------------------------------------------------------------------------
// Sets the corresponding meta data for the input field name.
//------------------------------------------------------------------------------
bool CCSDSOEMSegment::SetMetaData(const std::string &fieldName,
                                  const std::string &value)
{
   return CCSDSEMSegment::SetMetaData(fieldName, value);
}

//------------------------------------------------------------------------------
// bool SetMetaDataForWriting(const std::string &fieldName,
//                            const std::string &value)
//------------------------------------------------------------------------------
/**
 * Sets OEM meta data for writing.
 */
//------------------------------------------------------------------------------
bool CCSDSOEMSegment::SetMetaDataForWriting(const std::string &fieldName,
                                            const std::string &value)
{
   if (fieldName == "OBJECT_NAME" || fieldName == "OBJECT_ID" ||
       fieldName == "CENTER_NAME" || fieldName == "REF_FRAME" ||
       fieldName == "TIME_SYSTEM" || fieldName == "START_TIME" ||
       fieldName == "USEABLE_START_TIME" || fieldName == "USEABLE_STOP_TIME" ||
       fieldName == "STOP_TIME" || fieldName == "INTERPOLATION_DEGREE")
      CCSDSEMSegment::SetMetaDataForWriting(fieldName, value);
   else if (fieldName == "INTERPOLATION")
      interpolationMethod = GmatStringUtil::ToUpper(value);
   else
      throw UtilityException
         ("The field \"" + fieldName + "\" is not valid OEM metadata field.\n"
          "Valid fields are: OBJECT_NAME, OBJECT_ID, CENTER_NAME, REF_FRAME, "
          "TIME_SYSTEM, START_TIME, USEABLE_START_TIME, USEABLE_STOP_TIME, "
          "STOP_TIME, INTERPOLATION, INTERPOLATION_DEGREE");
   
   return true;
}


//------------------------------------------------------------------------------
// Adds an epoch/data pair to the dataStore.
// At this level, it just checks for errors in data size or epoch order.
//------------------------------------------------------------------------------
bool CCSDSOEMSegment::AddData(Real epoch, Rvector data, bool justCheckDataSize)
{
   return AddData(GmatTime(epoch), data, justCheckDataSize);
}

bool CCSDSOEMSegment::AddData(const GmatTime &epoch, Rvector data, bool justCheckDataSize)
{
   if (data.GetSize() != 6 && data.GetSize() != 9)
      throw UtilityException("OEM Cartesian records must contain six or nine data elements.");
   if (justCheckDataSize)
      return true;
   Rvector useData(6, data[0], data[1], data[2], data[3], data[4], data[5]);
   if (!dataStore.empty())
   {
      const auto *last = dataStore.back();
      const GmatTime lastEpoch = last->hasPrecisionEpoch ? last->preciseEpoch : GmatTime(last->epoch);
      if (epoch == lastEpoch)
      {
         if (useData == last->data)
            return true;
         throw UtilityException("Duplicate epochs with different position/velocity data were found within a segment. Ephemeris data can only be discontinuous at segment boundaries.");
      }
   }
   EpochAndData *newData = new EpochAndData();
   newData->epoch = epoch.GetMjd();
   newData->preciseEpoch = epoch;
   newData->hasPrecisionEpoch = true;
   newData->data = useData;
   dataStore.push_back(newData);
   return true;
}

//------------------------------------------------------------------------------
// Adds an epoch/data/accel to the dataStore for writing.
//------------------------------------------------------------------------------
bool CCSDSOEMSegment::AddDataForWriting(Real epoch, Rvector &data)
{
   EpochAndData *newData = new EpochAndData();
   newData->epoch = epoch;
   newData->data = data;
   dataStore.push_back(newData);

   #ifdef DEBUG_ADD_DATA_FOR_WRITING
   for (Integer i = 0; i < newData->data.GetSize(); ++i)
      MessageInterface::ShowMessage("data [%d] = %.15le\n", i, newData->data[i]);
   #endif

   return true;
}

//------------------------------------------------------------------------------
// void ClearMetaData()
//------------------------------------------------------------------------------
/**
 * Clears meta comments and data
 */
//------------------------------------------------------------------------------
void CCSDSOEMSegment::ClearMetaData()
{
   CCSDSEMSegment::ClearMetaData();
}

//------------------------------------------------------------------------------
// protected methods
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Interpolates the data.
//------------------------------------------------------------------------------
Rvector CCSDSOEMSegment::Interpolate(Real atEpoch)
{
   if (GmatStringUtil::ToUpper(interpolationMethod) != "HERMITE")
      return InterpolateLagrange(atEpoch);

   const Integer count = (interpolationDegree + 1) / 2;
   if (lastUsable - firstUsable + 1 < count)
      throw UtilityException(segError + "Insufficient usable data for HERMITE interpolation.");
   if (atEpoch < dataStore[firstUsable]->epoch || atEpoch > dataStore[lastUsable]->epoch)
      throw UtilityException(segError + "Requested HERMITE epoch is outside the usable range.");

   Integer upper = firstUsable;
   while (upper < lastUsable && dataStore[upper]->epoch < atEpoch)
      ++upper;
   const Integer start = std::max(firstUsable,
         std::min(upper - count / 2, lastUsable - count + 1));
   const auto *first = dataStore[start];
   const GmatTime anchor = first->hasPrecisionEpoch ? first->preciseEpoch : GmatTime(first->epoch);
   HermiteInterpolator interpolator("", 6, count);
   for (Integer i = start; i < start + count; ++i)
   {
      const auto *point = dataStore[i];
      const GmatTime epoch = point->hasPrecisionEpoch ? point->preciseEpoch : GmatTime(point->epoch);
      interpolator.AddPoint((epoch - anchor).GetTimeInSec(), point->data.GetDataVector());
   }
   for (Integer i = start; i < start + count; ++i)
   {
      const auto *point = dataStore[i];
      const GmatTime epoch = point->hasPrecisionEpoch ? point->preciseEpoch : GmatTime(point->epoch);
      const Real derivatives[6] = {point->data[3], point->data[4], point->data[5],
            -1.0e100, -1.0e100, -1.0e100};
      interpolator.AddDerivative((epoch - anchor).GetTimeInSec(), derivatives);
   }
   Real state[6];
   if (!interpolator.InterpolateCartesianState((GmatTime(atEpoch) - anchor).GetTimeInSec(), state))
      throw UtilityException(segError + "Unable to interpolate HERMITE state.");
   Rvector result(dataSize);
   for (Integer i = 0; i < 6; ++i)
      result[i] = state[i];
   return result;
}


// A single record has one position and velocity constraint. It can provide
// its exact state even though it spans no interpolation interval.
bool CCSDSOEMSegment::GetUsableIndexRange(Integer &first, Integer &last)
{
   if (interpolationMethod == "HERMITE" && interpolationDegree <= 1 && dataStore.size() == 1)
   {
      first = last = 0;
      return true;
   }
   return CCSDSEMSegment::GetUsableIndexRange(first, last);
}
