//------------------------------------------------------------------------------
//                                  CCSDSEphemerisFile
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
// Author: Tuan D. Nguyen / GSFC-NASA, Code 583
// Created: 2021.07.19
//
/**
 * Writes a spacecraft orbit ephemeris to a file in CCSDS format.
 */
//------------------------------------------------------------------------------

#include "CCSDSEphemerisFile.hpp"
#include "CCSDSOEMReader.hpp"
#include "FileUtil.hpp"
#include "FileTypes.hpp"
#include "StringUtil.hpp"
#include "TimeTypes.hpp"             // for FormatCurrentTime()
#include "UtilityException.hpp"
#include "MessageInterface.hpp"
#include "A1Mjd.hpp"
#include "Rvector6.hpp"
#include "GmatGlobal.hpp"
#include <sstream>
#include <limits>
#include <algorithm>

// We want to use std::numeric_limits<std::streamsize>::max()
#ifdef _MSC_VER  // if Microsoft Visual C++
#undef max
#endif

//#define DEBUG_CCSDS_FILE
//#define DEBUG_CCSDS_EPHEMFILE_INIT
//#define DEBUG_INITIALIZE
//#define DEBUG_WRITE_DATA_SEGMENT
//#define DEBUG_FINALIZE
//#define DEBUG_INITIAL_FINAL
//#define DEBUG_WRITE_POSVEL
//#define DEBUG_WRITE_COVPOSVEL
//#define DEBUG_DISTANCEUNIT
//#define DEBUG_VERSION
//#define DEBUG_WRITE_HEADER
//#define DEBUG_WRITE_STRING
//#define DEBUG_SEGMENTING
//#define DUMP_SEGMENT_DATA

//----------------------------
// static data
//----------------------------

//----------------------------
// public methods
//----------------------------

//------------------------------------------------------------------------------
// default constructor
//------------------------------------------------------------------------------
CCSDSEphemerisFile::CCSDSEphemerisFile() :
   ccsdsFileNameForRead  (""),
   stateType             ("OEM"),
   fileReader            (NULL)
{
}

//------------------------------------------------------------------------------
// copy constructor
//------------------------------------------------------------------------------
CCSDSEphemerisFile::CCSDSEphemerisFile(const CCSDSEphemerisFile &copy) :
   ccsdsFileNameForRead  (copy.ccsdsFileNameForRead),
   stateType             (copy.stateType),
   //fileReader            (copy.fileReader)
   fileReader            (NULL)
{
   if (copy.fileReader)
      fileReader = copy.fileReader->Clone();
}

//------------------------------------------------------------------------------
// operator=
//------------------------------------------------------------------------------
CCSDSEphemerisFile& CCSDSEphemerisFile::operator=(const CCSDSEphemerisFile &copy)
{
   if (&copy == this)
      return *this;
   
   Ephemeris::operator=(copy);
   theEphem.clear();
   ephemRecords.clear();
   ccsdsFileNameForRead  = copy.ccsdsFileNameForRead;
   stateType             = copy.stateType;

   //fileReader            = copy.fileReader;
   if (fileReader)
      delete fileReader;
   fileReader = NULL;
   if (copy.fileReader)
      fileReader = copy.fileReader->Clone();

   return *this;
}

//------------------------------------------------------------------------------
// destructor
//------------------------------------------------------------------------------
CCSDSEphemerisFile::~CCSDSEphemerisFile()
{
   if (fileReader != NULL)
      delete fileReader;
}

//------------------------------------------------------------------------------
// void InitializeData()
//------------------------------------------------------------------------------
/**
 * Clears header information.
 */
//------------------------------------------------------------------------------
void CCSDSEphemerisFile::InitializeData()
{
   #ifdef DEBUG_CCSDS_EPHEMFILE_INIT
   MessageInterface::ShowMessage("CCSDSEphemerisFile::InitializeData() entered\n");
   #endif
   
   scenarioEpochA1Mjd  = 0.0;

   warnInterpolationDegradation = true;
}

//------------------------------------------------------------------------------
// bool OpenForRead(const std::string &filename, const std::string &ephemType,
//                  const std::string &ephemCovType)
//------------------------------------------------------------------------------
/**
 * Opens CCSDS ephemeris (.e) file for reading.
 *
 * @filename File name to open
 * @ephemType Ephemeris type to read, at this time only "TimePos" or "TimePosVel"
 *               is allowed
 * @ephemCovType Ephemeris covariance type to read, at this time only "", "TimePos" or "TimePosVel"
 *               is allowed
 */
//------------------------------------------------------------------------------
bool CCSDSEphemerisFile::OpenForRead(const std::string &filename,
   const std::string &ephemType,
   const std::string &ephemCovType)
{
#ifdef DEBUG_CCSDS_FILE
   MessageInterface::ShowMessage
   ("CCSDSEphemerisFile::OpenForRead() entered, ephemType = <%s>  ephemCovType = <%s>  filename='%s'\n",
      ephemType.c_str(), ephemCovType.c_str(), filename.c_str());
#endif
   
   // Check ephem type
   // Currently only TimePos and TimePosVel are allowed
   /// @todo Do we allow TimePos?  ReadDataRecords seems to say no
   if (ephemType != "TimePos" && ephemType != "TimePosVel")
   {
      UtilityException ue;
      ue.SetDetails("CCSDSEphemerisFile::OpenForRead() *** INTERNAL ERROR *** "
         "Only TimePos or TimePosVel is valid for read on CCSDS "
         "ephemeris file '%s'.", filename.c_str());
      throw ue;
   }

   stateType = "OEM";

   // Check covariance ephem type
   // Currently only "", TimePos, and TimePosVel are allowed
   if (ephemCovType != "" && ephemCovType != "TimePos" && ephemCovType != "TimePosVel")
   {
      UtilityException ue;
      ue.SetDetails("CCSDSEphemerisFile::OpenForRead() *** INTERNAL ERROR *** "
         "Only "", TimePos, or TimePosVel is valid to read covariance on CCSDS "
         "ephemeris file '%s'.", filename.c_str());
      throw ue;
   }
   
   ccsdsFileNameForRead = filename;
   return ReadDataRecords();
}


//------------------------------------------------------------------------------
// void CloseForRead()
//------------------------------------------------------------------------------
void CCSDSEphemerisFile::CloseForRead()
{
}

//------------------------------------------------------------------------------
// bool GetInitialAndFinalStates(Real &initialA1Mjd, Real &finalA1Mjd,
//                               Rvector6 &initialState, Rvector6 &finalState,
//                               std::string &cbName, std::string &csName)
//------------------------------------------------------------------------------
/**
 * Retrieves initial and final state from CCSDS ephem file. Assumes ephem file
 * is successfully opened via OpenForRead()
 *
 * @param initialA1Mjd  output initial epoch in a1mjd
 * @param finalA1Mjd    output final epoch in a1mjd
 * @param initialState  output initial cartesian state in MJ2000Eq
 * @param finalState    output final cartesian state in MJ2000Eq
 * @param cbName        output central body name
 * @param csName        output coordinate system name
 *
 * @return true if all output parameter values are assigned, false otherwise
 */
//------------------------------------------------------------------------------
bool CCSDSEphemerisFile::GetInitialAndFinalStates(Real &initialA1Mjd, Real &finalA1Mjd,
                                                Rvector6 &initialState, Rvector6 &finalState,
                                                std::string &cbName, std::string &csName)
{
   #ifdef DEBUG_INITIAL_FINAL
   MessageInterface::ShowMessage
      ("CCSDSEphemerisFile::GetInitialAndFinalState() entered\n");
   #endif
   
   // Set defaults for output
   cbName = "Earth";
   csName = "J2000";

   bool lastStateFound = false;
   
   if (!theEphem.empty() || ReadDataRecords())
   {
      initialA1Mjd = a1StartEpoch;
      finalA1Mjd = a1EndEpoch;
      initialState = InterpolatePoint(theEphem.front().StartEpoch());
      finalState = InterpolatePoint(theEphem.back().EndEpoch());

      lastStateFound = true;

      cbName = GetCentralBody(0);
      csName = GetReferenceFrame(0);
      if (csName == "EME2000")
         csName = "J2000";
   }   
   return lastStateFound;
}


//------------------------------------------------------------------------------
// bool ReadDataRecords(int numRecordsToRead, int logOption)
//------------------------------------------------------------------------------
/**
 * Reads the header and checks for required elements, and loads data records
 *
 * @param logOption Flag used to trigger writing the data to the log and message
 *                  window (not used)
 *
 * @return true if state data was read, false if not
 */
//------------------------------------------------------------------------------
bool CCSDSEphemerisFile::ReadDataRecords(int logOption)
{
   // A rejected replacement file must not leave old samples queryable with
   // the new reader's segment metadata.
   theEphem.clear();
   ephemRecords.clear();
   a1StartEpoch = a1EndEpoch = scenarioEpochA1Mjd = 0.0;
   if (ccsdsFileNameForRead.empty()) return false;
   // An explicit reread must also see repairs to the same file on disk.
   delete fileReader;
   fileReader = nullptr;
   fileReader = new CCSDSOEMReader();
   fileReader->SetFile(ccsdsFileNameForRead);
   const Integer count = fileReader->GetNumberOfSegments();
   const std::string body = GetCentralBody(0);
   const std::string frame = GetReferenceFrame(0);
   if (frame != "EME2000" && !(body == "Earth" &&
         (frame == "GRC" || frame == "TDR" || frame == "TOD")))
      throw UtilityException("Unsupported coordinate system " + frame + " for " + body +
            " in CCSDS-OEM file " + ccsdsFileNameForRead);

   std::vector<Segment> segments;
   std::vector<EphemData> records;
   for (Integer i = 0; i < count; ++i)
   {
      if (GetCentralBody(i) != body || GetReferenceFrame(i) != frame)
         throw UtilityException("CCSDS-OEM propagation requires the same CENTER_NAME and "
               "REF_FRAME in every segment. Segment " + std::to_string(i + 1) +
               " differs in " + ccsdsFileNameForRead);

      auto *source = fileReader->GetSegment(i);
      Segment segment;
      const Integer size = source->GetNumberOfDataPoints();
      if (size == 0)
         throw UtilityException("Empty CCSDS-OEM segment in " + ccsdsFileNameForRead);
      segment.points.reserve(size);
      for (Integer j = 0; j < size; ++j)
      {
         EphemPoint point;
         Rvector state;
         source->GetEpochAndData(j, point.preciseEpoch, state);
         point.theEpoch = point.preciseEpoch.GetMjd();
         point.hasPrecisionEpoch = true;
         point.posvel = Rvector6(state[0], state[1], state[2], state[3], state[4], state[5]);
         segment.points.push_back(point);
      }
      const GmatTime start = std::max(source->GetCoverageStart(), segment.points.front().GetEpoch());
      const GmatTime stop = std::min(source->GetCoverageStop(), segment.points.back().GetEpoch());
      if (start > stop)
         throw UtilityException("No state data covers the usable interval in " + ccsdsFileNameForRead);
      segment.coverage = std::make_pair(start, stop);
      segment.segStart = start.GetMjd();
      segment.segEnd = stop.GetMjd();
      segments.push_back(std::move(segment));
   }
   const GmatTime origin = segments.front().StartEpoch();
   for (const auto &segment : segments)
      for (const auto &point : segment.points)
      {
         EphemData record;
         record.timeFromEpoch = (point.GetEpoch() - origin).GetTimeInSec();
         std::copy_n(point.posvel.GetDataVector(), 6, record.theState);
         records.push_back(record);
      }

   theEphem.swap(segments);
   ephemRecords.swap(records);
   a1StartEpoch = theEphem.front().segStart;
   a1EndEpoch = theEphem.back().segEnd;
   scenarioEpochA1Mjd = a1StartEpoch;
   GetInterpolationInfoFromSegment(0);
   return true;
}


//------------------------------------------------------------------------------
// void GetStartAndEndEpochs(GmatEpoch& startEpoch, GmatEpoch& endEpoch,
//       std::vector<EphemData>** records)
//------------------------------------------------------------------------------
/**
 * Accesses the start and end epochs as contained in the header data
 *
 * @param startEpoch The start epoch
 * @param endEpoch The end epoch
 * @param records The ephem data
 */
//------------------------------------------------------------------------------
void CCSDSEphemerisFile::GetStartAndEndEpochs(GmatEpoch& startEpoch,
      GmatEpoch& endEpoch, std::vector<EphemData>** records)
{
   startEpoch = a1StartEpoch;
   endEpoch   = a1EndEpoch;

   *records   = &ephemRecords;
}

//------------------------------------------------------------------------------
// std::string GetCentralBody(Integer segmentNo)
//------------------------------------------------------------------------------
/**
 * Returns the central body from the file
 *
 * @param segmentNo - segment number you want the central body of
 */
 //------------------------------------------------------------------------------
std::string CCSDSEphemerisFile::GetCentralBody(Integer segmentNo)
{
   // Get segment with index given by segmentNo
   CCSDSEMSegment* segment;
   if (segmentNo == -1)
      segment = fileReader->GetSegment();
   else
      segment = fileReader->GetSegment(segmentNo);

   // Get central body name associate with this data segment
   std::string cbName = segment->GetStringMetaData("CENTER_NAME");
   cbName = GmatStringUtil::ToUpper(cbName.substr(0, 1)) + GmatStringUtil::ToLower(cbName.substr(1));   // convert "LUNA", "EARTH", etc to "Luna", "Earth", etc

   return cbName;
}

//------------------------------------------------------------------------------
// std::string GetReferenceFrame(Integer segmentNo)
//------------------------------------------------------------------------------
/**
 * Returns the refrence frame from the file
 *
 * @param segmentNo - segment number you want the central body of
 */
 //------------------------------------------------------------------------------
std::string CCSDSEphemerisFile::GetReferenceFrame(Integer segmentNo)
{
   // Get segment with index given by segmentNo
   CCSDSEMSegment* segment;
   if (segmentNo == -1)
      segment = fileReader->GetSegment();
   else
      segment = fileReader->GetSegment(segmentNo);

   // Get name of the reference frame associated with this data segment
   std::string frameName = segment->GetStringMetaData("REF_FRAME");

   return frameName;
}

void CCSDSEphemerisFile::GetInterpolationInfoFromSegment(Integer segmentNo)
{
   // Get segment with index given by segmentNo
   CCSDSEMSegment* segment;
   if (segmentNo == -1)
      segment = fileReader->GetSegment();
   else
      segment = fileReader->GetSegment(segmentNo);

   // Get central body name associate with this data segment
   order = segment->GetIntegerMetaData("INTERPOLATION_DEGREE");
   std::string interpolationMethod = segment->GetStringMetaData("INTERPOLATION");
   if (GmatStringUtil::ToLower(interpolationMethod) == "hermite")
      useHermite = true;
   else 
      useHermite = false;
}

