//------------------------------------------------------------------------------
//                                  CCSDSOEMWriter
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
// Created: 2015.12.14
//
/**
 * Writes a CCSDS Ephemeris Message to a file.
 * This is the base class from which other classes must derive (e.g.
 * a class to handle attitude files)
 */
//------------------------------------------------------------------------------

#include <sstream>
#include "FileUtil.hpp"
#include "FileTypes.hpp"
#include "StringUtil.hpp"

#include "CCSDSOEMWriter.hpp"
#include "CcsdsOemB1Text.hpp"
#include "CcsdsOemB1Chronology.hpp"
#include "UtilityException.hpp"
#include "MessageInterface.hpp"
#include <algorithm>
#include <iomanip>
#include <locale>

namespace
{
std::string OemNumericFields(const Real *values, Integer count)
{
   // CCSDS numbers use a decimal point regardless of the host's LC_NUMERIC
   // or global C++ locale. Retain the existing precision and sign spacing.
   std::ostringstream fields;
   fields.imbue(std::locale::classic());
   fields << std::scientific << std::setprecision(15);
   for (Integer i = 0; i < count; ++i)
   {
      fields << "  ";
      if (!std::signbit(values[i]))
         fields << ' ';
      fields << values[i];
   }
   return fields.str();
}
}

//#define DEBUG_META_COMMENT
//#define DEBUG_META_DATA
//#define DEBUG_DATA_COMMENT
//#define DEBUG_DATA_SEGMENT

// -----------------------------------------------------------------------------
// static data
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// public methods
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// default constructor
// -----------------------------------------------------------------------------
CCSDSOEMWriter::CCSDSOEMWriter(const std::string &versionNumber) :
   CCSDSEMWriter(versionNumber), currentOemSegment(versionNumber), timeSystem("UTC")
{
}

// -----------------------------------------------------------------------------
// copy constructor
// -----------------------------------------------------------------------------
CCSDSOEMWriter::CCSDSOEMWriter(const CCSDSOEMWriter &copy) :
   CCSDSEMWriter(copy), timeSystem(copy.timeSystem)
{
}

// -----------------------------------------------------------------------------
// operator=
// -----------------------------------------------------------------------------
CCSDSOEMWriter& CCSDSOEMWriter::operator=(const CCSDSOEMWriter &copy)
{
   if (&copy == this)
      return *this;

   if (emOutStream.is_open() && timeSystem != copy.timeSystem)
      throw UtilityException("Cannot change CCSDS OEM time system while its file is open");
   
   CCSDSEMWriter::operator=(copy);
   timeSystem = copy.timeSystem;
   
   return *this;
}

// -----------------------------------------------------------------------------
// destructor
// -----------------------------------------------------------------------------
CCSDSOEMWriter::~CCSDSOEMWriter()
{
}

void CCSDSOEMWriter::SetTimeSystem(const std::string &system)
{
   const std::string selected = GmatStringUtil::ToUpper(system);
   const StringArray allowed = {"A1", "TAI", "UTC", "TT", "TDB"};
   if (std::find(allowed.begin(), allowed.end(), selected) == allowed.end())
      throw UtilityException("Unsupported CCSDS OEM output time system: " + system);
   if (emOutStream.is_open() && selected != timeSystem)
      throw UtilityException("Select CCSDS OEM output time system before opening its file");
   timeSystem = selected;
}

std::string CCSDSOEMWriter::FormatEpoch(Real epochInDays)
{
   // Keep the existing default conversion byte-for-byte. The selected formatter
   // is also used by metadata and record counting,including duplicate epochs.
   if (timeSystem == "UTC")
      return A1ModJulianToUtcGregorian(epochInDays, 2);
   if (epochInDays == -999.999)
      return "-999.999";
   Real converted;
   std::string result;
   theTimeConverter->Convert("A1ModJulian", epochInDays, "", timeSystem + "Gregorian",
                            converted, result, 2);
   if (result.empty())
      throw UtilityException("Unable to format CCSDS OEM epoch in " + timeSystem);
   return result;
}

//------------------------------------------------------------------------------
// Returns a clone of this object.
//------------------------------------------------------------------------------
CCSDSOEMWriter* CCSDSOEMWriter::Clone() const
{
   return (new CCSDSOEMWriter(*this));
}


//------------------------------------------------------------------------------
// bool WriteMetaData()
//------------------------------------------------------------------------------
/**
 * Writes meta data to a file and then clears meta data.
 */
//------------------------------------------------------------------------------
bool CCSDSOEMWriter::WriteMetaData()
{
   #ifdef DEBUG_META_DATA
   MessageInterface::ShowMessage("CCSDSOEMWriter::WriteMetaData() entered\n");
   #endif
   
   if (!emOutStream.is_open())
      return false;
   
   currentOemSegment.SetFormatVersion(versionNumber);
   const std::string metadata = currentOemSegment.GetMetaDataForWriting();
   const auto metadataPosition = emOutStream.tellp();
   CcsdsOemB1Chronology::State chronology;
   if (versionNumber == "1.0")
      chronology = CcsdsOemB1Chronology::PreviewMetadata(this, metadataPosition,
            currentOemSegment.GetStringMetaData("OBJECT_ID"));
   emOutStream << metadata;
   emOutStream.flush();
   if (versionNumber == "1.0" && emOutStream.good())
      CcsdsOemB1Chronology::Commit(this, chronology);
   
   // Note that: only clear meta data when data segment is completed because some
   // information in meta data is still needed in writing data segment.
   // The follwoing line is moved to the end of WriteDataSegment() function.
   //ClearMetaData();
   
   #ifdef DEBUG_META_DATA
   MessageInterface::ShowMessage
      ("CCSDSOEMWriter::WriteMetaData() returning true\n");
   #endif
   return true;
}

//------------------------------------------------------------------------------
// bool WriteDataComments()
//------------------------------------------------------------------------------
/**
 * Writes data comments to a file and then clears data comments
 */
//------------------------------------------------------------------------------
bool CCSDSOEMWriter::WriteDataComments()
{
   #ifdef DEBUG_DATA_COMMENTS
   MessageInterface::ShowMessage("CCSDSOEMWriter::WriteDataComments() entered\n");
   #endif
   
   if (!emOutStream.is_open())
      return false;
   
   currentOemSegment.SetFormatVersion(versionNumber);
   const std::string comments = currentOemSegment.GetDataComments();
   emOutStream << comments;
   emOutStream.flush();
   
   ClearDataComments();
   
   #ifdef DEBUG_DATA_COMMENTS
   MessageInterface::ShowMessage("CCSDSOEMWriter::WriteDataComments() returning true\n");
   #endif
   return true;
}

//------------------------------------------------------------------------------
// bool WriteDataSegment()
//------------------------------------------------------------------------------
/**
 * Writes saved data store to a file and then clears data store
 */
//------------------------------------------------------------------------------
bool CCSDSOEMWriter::WriteDataSegment()
{
   #ifdef DEBUG_DATA_SEGMENT
   MessageInterface::ShowMessage("CCSDSOEMWriter::WriteDataSegment() entered\n");
   #endif
   
   if (!emOutStream.is_open())
      return false;

   // B-1 contains exactly epoch and six Cartesian fields. OEM2 extensions
   // must not escape into a file declaring version1.0. Reject before writing
   // any bytes or consuming the buffered records so callers can recover.
   if (versionNumber == "1.0" && (useAcceleration || useCovariance))
      throw UtilityException("CCSDS OEM 1.0 does not support acceleration or covariance output; "
            "select OEM 2.0 for these extensions");
   
   bool retval = true;
   
   Integer numPoints = currentOemSegment.GetNumberOfDataPoints();

   std::vector<std::string> epochStrs(numPoints);
   Real epoch;
   Rvector data;
   for (Integer i = 0; i < numPoints; i++)
   {
      if (currentOemSegment.GetEpochAndData(i, epoch, data))
         epochStrs[i] = FormatEpoch(epoch);
      else
         return false;
   }

   std::string covRefFrame = currentOemSegment.GetRefFrame();

   CcsdsOemB1Chronology::State chronology;
   if (versionNumber == "1.0")
   {
      std::vector<std::string> serializedEpochs;
      for (Integer i = 0; i < numPoints; ++i)
         if (i == numPoints - 1 || epochStrs[i] != epochStrs[i + 1])
            serializedEpochs.push_back(epochStrs[i]);
      chronology = CcsdsOemB1Chronology::Preview(this, serializedEpochs);
   }
   
   for (Integer i = 0; i < numPoints; i++)
   {
      if (i != numPoints - 1 && epochStrs[i] == epochStrs[i + 1])
         continue;

      if (currentOemSegment.GetEpochAndData(i, epoch, data))
      {
         const Real *outState = data.GetDataVector();
         std::string epochStr = epochStrs[i];
         emOutStream << epochStr << OemNumericFields(outState, 6);
         if (useAcceleration)
            emOutStream << OemNumericFields(outState + 27, 3);
         emOutStream << '\n';
      }
      else
      {
         MessageInterface::ShowMessage
            ("*** INTERNAL ERROR *** data index %d out of range\n", i);
         retval = false;
         break;
      }
   }

   if (useCovariance)
   {
      emOutStream << "\n";
      emOutStream << "COVARIANCE_START" << "\n";

      for (Integer i = 0; i < numPoints; i++)
      {
         if (i != numPoints - 1 && epochStrs[i] == epochStrs[i + 1])
            continue;

         if (currentOemSegment.GetEpochAndData(i, epoch, data))
         {
            const Real* outState = data.GetDataVector();
            std::string epochStr = epochStrs[i];
            emOutStream << "EPOCH = " << epochStr << '\n';
            Integer offset = 6;
            for (Integer row = 1; row <= 6; ++row)
            {
               emOutStream << OemNumericFields(outState + offset, row) << '\n';
               offset += row;
            }
         }
         else
         {
            MessageInterface::ShowMessage
            ("*** INTERNAL ERROR *** data index %d out of range\n", i);
            retval = false;
            break;
         }
      }

      emOutStream << "COVARIANCE_STOP" << "\n";
   }
   
   emOutStream.flush();      
   if (versionNumber == "1.0" && retval && emOutStream.good())
      CcsdsOemB1Chronology::Commit(this, chronology);
   
   // Clears data store
   ClearDataStore();

   // Clear meta data
   ClearMetaData();
   #ifdef DEBUG_DATA_SEGMENT
   MessageInterface::ShowMessage
      ("CCSDSOEMWriter::WriteDataSegment() returning %d\n", retval);
   #endif
   return retval;
}

//------------------------------------------------------------------------------
// bool SetMetaDataForWriting(const std::string &fieldName, const std::string &value)
//------------------------------------------------------------------------------
/**
 * Sets meta data for writing. Once meta data is written to a file it will be
 * cleared.
 *
 * @param fieldName Metadata field name to be set
 * @param value Value to set to the field
 */
//------------------------------------------------------------------------------
bool CCSDSOEMWriter::SetMetaDataForWriting(const std::string &fieldName,
                                           const std::string &value)
{
   if (fieldName == "TIME_SYSTEM" && GmatStringUtil::ToUpper(value) != timeSystem)
      throw UtilityException("CCSDS OEM TIME_SYSTEM must match its selected output time system " + timeSystem);
   #ifdef DEBUG_META_DATA
   MessageInterface::ShowMessage("CCSDSOEMWriter::SetMetaDataForWriting() entered\n");
   #endif
   
   currentOemSegment.SetMetaDataForWriting(fieldName, value);
   
   #ifdef DEBUG_META_DATA
   MessageInterface::ShowMessage("CCSDSOEMWriter::SetMetaDataForWriting() returning true\n");
   #endif
   return true;
}

//------------------------------------------------------------------------------
// bool AddMetaComment(const std::string& comment)
//------------------------------------------------------------------------------
/**
 * Adds meta comment for writing. Once meta data is written to a file it will be
 * cleared.
 *
 * @param comment comment to be added to the metadata comments
 */
//------------------------------------------------------------------------------
bool CCSDSOEMWriter::AddMetaComment(const std::string& comment)
{
   #ifdef DEBUG_META_COMMENT
   MessageInterface::ShowMessage("CCSDSOEMWriter::AddMetaComment() entered\n");
   #endif
   
   currentOemSegment.AddMetaComment(comment);
   
   #ifdef DEBUG_META_COMMENT
   MessageInterface::ShowMessage("CCSDSOEMWriter::AddMetaComment() returning true\n");
   #endif
   return true;
}

//------------------------------------------------------------------------------
// bool AddDataComment(const std::string& comment)
//------------------------------------------------------------------------------
/**
 * Adds data comment for writing. Once data segment is written to a file it will be
 * cleared.
 *
 * @param comment comment to be added to the data comments
 */
//------------------------------------------------------------------------------
bool CCSDSOEMWriter::AddDataComment(const std::string& comment)
{
   #ifdef DEBUG_DATA_COMMENT
   MessageInterface::ShowMessage("CCSDSOEMWriter::AddDataComment() entered\n");
   #endif
   
   currentOemSegment.AddDataComment(comment);
   
   #ifdef DEBUG_DATA_COMMENT
   MessageInterface::ShowMessage("CCSDSOEMWriter::AddDataComment() leaving\n");
   #endif
   return true;
}

//------------------------------------------------------------------------------
// Integer GetNumberOfSerializedDataPoints()
//------------------------------------------------------------------------------
/**
 * Counts records in the complete pending segment using the same epoch format
 * and adjacent-duplicate suppression as WriteDataSegment(). The store includes
 * all subscriber buffer flushes and is cleared after the segment is written.
 */
//------------------------------------------------------------------------------
Integer CCSDSOEMWriter::GetNumberOfSerializedDataPoints()
{
   Integer count = 0;
   std::string previousEpoch;
   Real epoch;
   Rvector data;
   for (Integer i = 0; i < currentOemSegment.GetNumberOfDataPoints(); ++i)
   {
      if (!currentOemSegment.GetEpochAndData(i, epoch, data))
         throw UtilityException("Unable to read pending CCSDS OEM data point");
      const std::string epochString = FormatEpoch(epoch);
      if (i == 0 || epochString != previousEpoch)
         ++count;
      previousEpoch = epochString;
   }
   return count;
}

//------------------------------------------------------------------------------
// bool AddDataForWriting(Real epoch, Rvector data)
//------------------------------------------------------------------------------
/**
 * Adds data to a data store for writing. Once data segment is written to a
 * file the data store will be cleared.
 *
 * @epoch Epoch of data to be added to the CCSDS data store
 * @data Data to be added to the CCSDS data store
 * @accel Acceleration to be added to the data store (may be zero is not used)
 */
 //------------------------------------------------------------------------------
bool CCSDSOEMWriter::AddDataForWriting(Real epoch, Rvector& data)
{
   if (versionNumber == "1.0")
      CcsdsOemB1Text::RequireState(epoch, data.GetDataVector(), data.GetSize());

   #ifdef DEBUG_DATA_SEGMENT
   MessageInterface::ShowMessage("CCSDSOEMWriter::AddDataForWriting() entered\n");
   #endif
   
   currentOemSegment.AddDataForWriting(epoch, data);
   
   #ifdef DEBUG_DATA_SEGMENT
   MessageInterface::ShowMessage("CCSDSOEMWriter::AddDataForWriting() exit\n");
   #endif
   return true;
}

//------------------------------------------------------------------------------
// void ClearMetaComments()
//------------------------------------------------------------------------------
/**
 * Clears meta data comments
 */
//------------------------------------------------------------------------------
void CCSDSOEMWriter::ClearMetaComments()
{
   currentOemSegment.ClearMetaComments();
}

//------------------------------------------------------------------------------
// void ClearDataComments()
//------------------------------------------------------------------------------
void CCSDSOEMWriter::ClearDataComments()
{
   currentOemSegment.ClearDataComments();
}

//------------------------------------------------------------------------------
// void ClearMetaData()
//------------------------------------------------------------------------------
/**
 * Clears meta comments and data
 */
//------------------------------------------------------------------------------
void CCSDSOEMWriter::ClearMetaData()
{
   #ifdef DEBUG_META_DATA
   MessageInterface::ShowMessage("CCSDSOEMWriter::ClearMetaData() entered\n");
   #endif
   
   // ClearMetaData() also clears meta comments
   currentOemSegment.ClearMetaData();
   
   #ifdef DEBUG_META_DATA
   MessageInterface::ShowMessage("CCSDSOEMWriter::ClearMetaData() leaving\n");
   #endif
}

//------------------------------------------------------------------------------
// void ClearDataStore()
//------------------------------------------------------------------------------
/**
 * Clears data store and comments.
 */
//------------------------------------------------------------------------------
void CCSDSOEMWriter::ClearDataStore()
{
   #ifdef DEBUG_DATA_SEGMENT
   MessageInterface::ShowMessage("CCSDSOEMWriter::ClearDataStore() entered\n");
   #endif
   
   // ClearDataStore() also clears data comments
   currentOemSegment.ClearDataStore();
   
   #ifdef DEBUG_DATA_SEGMENT
   MessageInterface::ShowMessage("CCSDSOEMWriter::ClearDataStore() leaving\n");
   #endif
}

// -----------------------------------------------------------------------------
// protected methods
// -----------------------------------------------------------------------------

