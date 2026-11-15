//$Id$
//------------------------------------------------------------------------------
//                            SpiceAttitudeKernelWriter
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
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under
// FDSS III.
//
// Author: Joshua Raymond, Thinking Systems, Inc.
// Created: September 02, 2025
/**
 * Defines the SpiceAttitudeKernelWriter, which writes CK files quaternion
 * inputs. It also generates an SCLK file if needed from the SCLK writer.
 */
 //------------------------------------------------------------------------------
#include <stdio.h>
#include <sstream>
#include <memory>
#include <limits>
#include <exception>
#include <utility>

#include "SpiceAttitudeKernelWriter.hpp"
#include "SpiceInterface.hpp"
#include "SpiceKernelFileRetirement.hpp"
#include "MessageInterface.hpp"
#include "StringUtil.hpp"
#include "TimeTypes.hpp"
#include "TimeSystemConverter.hpp"
#include "UtilityException.hpp"
#include "RealUtilities.hpp"
#include "FileUtil.hpp"

namespace
{
void ReportCKCleanupFailure(const std::string &file) noexcept
{
   try { throw; }
   catch (BaseException &error)
   {
      try { MessageInterface::ShowMessage("*** WARNING *** CK cleanup failed for %s: %s\n", file.c_str(), error.GetFullMessage().c_str()); } catch (...) {}
   }
   catch (const std::exception &error)
   {
      try { MessageInterface::ShowMessage("*** WARNING *** CK cleanup failed for %s: %s\n", file.c_str(), error.what()); } catch (...) {}
   }
   catch (...)
   {
      try { MessageInterface::ShowMessage("*** WARNING *** CK cleanup failed for %s\n", file.c_str()); } catch (...) {}
   }
}
}

//#define DEBUG_SPK_WRITING
//#define DEBUG_SPK_INIT
//#define DEBUG_SPK_KERNELS

//---------------------------------
// static data
//---------------------------------


const std::string   SpiceAttitudeKernelWriter::TMP_TXT_FILE_NAME = "GMATtmpCKcmmnt";

//---------------------------------
// public methods
//---------------------------------
//------------------------------------------------------------------------------
//  SpiceAttitudeKernelWriter(const std::string& objName,
//    Integer objNAIFId, const std::string& fileName, Integer deg,
//    const std::string& frame, const std::string& existingSCLK, bool makeSCLK, bool renameExistingCK)
//------------------------------------------------------------------------------
/**
 * This method constructs a SpiceKernelWriter instance.
 * (constructor)
 *
 * @param    objName       name of the object for which to write the ck kernel
 * @param    objNAIFId     NAIF ID for the object
 * @param    fileName      name of the kernel to generate
 * @param    deg           degree of interpolating polynomials
 * @param    frame         reference frame (default = "J2000")
 * @param    existingSCLK  SCLK file to write to or utilize
 * @param    makeSCLK      whether an SCLK must be freshly generated
 * @param    renameCK      rename existing ck file(s) with same name?
 *                         [true = rename; false = overwrite]
 *
 */
 //------------------------------------------------------------------------------
SpiceAttitudeKernelWriter::SpiceAttitudeKernelWriter(const std::string& objName,
   Integer objNAIFId, const std::string& fileName, Integer deg,
   const std::string& frame, const std::string &existingSCLK, bool makeSCLK, bool renameExistingCK) :
   SpiceKernelWriter(),
   objectName(objName),
   kernelFileName(fileName),
   frameName(frame),
   fileOpen(false),
   fileFinalized(false),
   dataWritten(false),
   tmpTxtFile(NULL),
   tmpFileOK(false),
   appending(false),
   fm(NULL),
   sclk(NULL),
   sclkFileName(existingSCLK),
   generateSCLK(makeSCLK),
   renameCK(renameExistingCK)
{
#ifdef DEBUG_SPK_INIT
   MessageInterface::ShowMessage(
      "Entering constructor for SPKOrbitWriter with fileName = %s, objectName = %s\n",
      fileName.c_str(), objName.c_str());
#endif
   // Check for the default NAIF ID
   if (objNAIFId == SpiceInterface::DEFAULT_NAIF_ID)
   {
      MessageInterface::ShowMessage(
         "*** WARNING *** NAIF ID for object %s is set to the default NAIF ID (%d).  Resulting ck file will contain that value as the object's ID.\n",
         objectName.c_str(), objNAIFId);
   }

   // Get the FileManage pointer
   fm = FileManager::Instance();
   // Create the temporary text file to hold the meta data
   tmpTxtFileName = fm->GetAbsPathname(FileManager::OUTPUT_PATH);
   tmpTxtFileName += TMP_TXT_FILE_NAME + objectName + ".txt";
#ifdef DEBUG_SPK_INIT
   MessageInterface::ShowMessage("temporary SPICE file name is: %s\n", tmpTxtFileName.c_str());
#endif
   tmpTxtFile = fopen(tmpTxtFileName.c_str(), "w");

   if (!tmpTxtFile)
   {
      std::string errmsg = "Error creating or opening temporary text file for ck meta data, for object \"";
      errmsg += objectName + "\".  No meta data will be added to the file.\n";
      MessageInterface::PopupMessage(Gmat::WARNING_, errmsg);
      tmpFileOK = false;
   }
   else
   {
      fclose(tmpTxtFile);
      tmpTxtFile = NULL;
      // remove the temporary text file
      remove(tmpTxtFileName.c_str());
      tmpFileOK = true;
   }

   if (generateSCLK && sclkFileName == "")
   {
      std::string errmsg =
         "No file path was provided for an SCLK file to generate for associated CK file writer \"";
      errmsg += kernelFileName + "\".\n";
      throw UtilityException(errmsg);
   }

   /// set up CSPICE data
   objectNAIFId = objNAIFId;
   kernelNameSPICE = kernelFileName.c_str();
   degree = deg;
   referenceFrame = frameName.c_str();
   handle = -999;

   // first, try to rename an existing file, as SPICE will not overwrite or
   // append to an existing file - this is the most common error returned from spkopn
   GmatSpiceKernelFiles::Unload(this, kernelFileName);
   if (GmatFileUtil::DoesFileExist(kernelFileName))
   {
      if (renameCK)
      {
#ifdef DEBUG_SPK_INIT
         MessageInterface::ShowMessage(
            "SPKOrbitWriter: the file %s exists, will need to rename ... \n",
            kernelFileName.c_str());
#endif
         Integer     fileCounter = 0;
         bool        done = false;
         std::string fileWithBSP = fileName;
         std::string fileNoBSP = fileWithBSP.erase(fileWithBSP.rfind(".bsp"));
         std::stringstream fileRename("");
         Integer     retCode = 0;
         while (!done)
         {
            fileRename.str("");
            fileRename << fileNoBSP << "__" << fileCounter << ".bsp";
#ifdef DEBUG_SPK_INIT
            MessageInterface::ShowMessage(
               "SPKOrbitWriter: renaming %s to %s ... \n",
               kernelFileName.c_str(), fileRename.str().c_str());
#endif
            if (fm->RenameFile(kernelFileName, fileRename.str(), retCode))
            {
               done = true;
            }
            else
            {
               if (retCode == 0) // if no error from system, but not allowed to overwrite
               {
                  fileCounter++;
               }
               else
               {
                  //               reset_c(); // reset failure flag in SPICE
                  std::string errmsg =
                     "Unknown system error occurred when attempting to rename existing ck file \"";
                  errmsg += kernelFileName + "\".\n";
                  throw UtilityException(errmsg);
               }
            }

         }
      }
      else
      {
         // delete the file
         GmatSpiceKernelFiles::Retire(this, kernelFileName);
      }
   }
   else // otherwise, check to make sure the directory is writable
   {
      std::string dirName = GmatFileUtil::ParsePathName(kernelFileName, true);
      if (dirName == "")
         dirName = "./";
      if (!GmatFileUtil::DoesDirectoryExist(dirName, true))
      {
         std::string errmsg =
            "Directory \"";
         errmsg += dirName + "\" does not exist.\n";
         throw UtilityException(errmsg);
      }
   }
   // set up the "basic" meta data here ...
   SetBasicMetaData();

   // make sure that the NAIF Id is associated with the object name  @todo - need to set center's Id as well sometimes?
   ConstSpiceChar* itsName = objectName.c_str();
   boddef_c(itsName, objectNAIFId);        // CSPICE method to set NAIF ID for an object
   if (failed_c())
   {
      std::string errDesc = "Unable to set NAIF Id for object \"" +
         objectName + "\" to the value " +
         GmatStringUtil::ToString(objectNAIFId) + ".";
      GenerateSpiceErrorMessage(errDesc);
   }
}

//------------------------------------------------------------------------------
//  SpiceAttitudeKernelWriter(const SpiceAttitudeKernelWriter &copy)
//------------------------------------------------------------------------------
/**
 * This method constructs a SpiceKernelWriter instance, copying data from the
 * input instance.
 * (copy constructor)
 *
 * @param    copy       object to copy
 *
 */
//------------------------------------------------------------------------------
SpiceAttitudeKernelWriter::SpiceAttitudeKernelWriter(const SpiceAttitudeKernelWriter& copy) :
   SpiceKernelWriter(copy),
   objectName(copy.objectName),
   centralBodyName(copy.centralBodyName),
   kernelFileName(copy.kernelFileName),
   frameName(copy.frameName),
   segStarts(copy.segStarts),
   segEnds(copy.segEnds),
   segEpochs(copy.segEpochs),
   segAttitudes(copy.segAttitudes),
   renameCK(copy.renameCK),
   objectNAIFId(copy.objectNAIFId),
   degree(copy.degree),
   referenceFrame(NULL),
   handle(-999),
   basicMetaData(copy.basicMetaData),
   addedMetaData(copy.addedMetaData),
   fileOpen(false),
   fileFinalized(false),
   dataWritten(copy.dataWritten),
   tmpTxtFileName(copy.tmpTxtFileName),
   tmpTxtFile(NULL),
   tmpFileOK(copy.tmpFileOK),
   appending(false),
   fm(copy.fm),
   sclk(NULL),
   sclkFileName(copy.sclkFileName),
   sclkStartTime(copy.sclkStartTime),
   generateSCLK(copy.generateSCLK)
{
   std::unique_ptr<SpiceSCClockKernelWriter> clock(copy.sclk ? copy.sclk->Clone() : NULL);
   kernelNameSPICE = kernelFileName.c_str();
   referenceFrame = frameName.c_str();
   sclk = clock.release();
}

//------------------------------------------------------------------------------
//  SpiceAttitudeKernelWriter& operator=(const SpiceAttitudeKernelWriter &copy)
//------------------------------------------------------------------------------
/**
 * This method copies data from the input SpiceKernelWriter instance to
 * "this" instance.
 *
 * @param    copy       object whose data to copy
 *
 */
 //------------------------------------------------------------------------------
SpiceAttitudeKernelWriter& SpiceAttitudeKernelWriter::operator=(const SpiceAttitudeKernelWriter& copy)
{
   if (&copy != this)
   {
      // Stage value storage and a private clock before retiring this handle.
      SpiceAttitudeKernelWriter prepared(copy);
      FinalizeKernel();
      SpiceKernelWriter::operator=(copy);
      SwapValue(prepared);
   }

   return *this;
}

void SpiceAttitudeKernelWriter::SwapValue(SpiceAttitudeKernelWriter &other) noexcept
{
   using std::swap;
   objectName.swap(other.objectName); centralBodyName.swap(other.centralBodyName);
   kernelFileName.swap(other.kernelFileName); frameName.swap(other.frameName);
   segStarts.swap(other.segStarts); segEnds.swap(other.segEnds);
   segEpochs.swap(other.segEpochs); segAttitudes.swap(other.segAttitudes);
   swap(renameCK, other.renameCK); swap(objectNAIFId, other.objectNAIFId);
   swap(degree, other.degree); swap(handle, other.handle);
   basicMetaData.swap(other.basicMetaData); addedMetaData.swap(other.addedMetaData);
   swap(fileOpen, other.fileOpen); swap(fileFinalized, other.fileFinalized);
   swap(dataWritten, other.dataWritten); tmpTxtFileName.swap(other.tmpTxtFileName);
   swap(tmpTxtFile, other.tmpTxtFile); swap(tmpFileOK, other.tmpFileOK);
   swap(appending, other.appending); swap(fm, other.fm); swap(sclk, other.sclk);
   sclkFileName.swap(other.sclkFileName); swap(sclkStartTime, other.sclkStartTime);
   swap(generateSCLK, other.generateSCLK);
   kernelNameSPICE = kernelFileName.c_str(); referenceFrame = frameName.c_str();
   other.kernelNameSPICE = other.kernelFileName.c_str(); other.referenceFrame = other.frameName.c_str();
}

//------------------------------------------------------------------------------
//  ~SpiceAttitudeKernelWriter()
//------------------------------------------------------------------------------
/**
 * This method deletes "this" SpiceKernelWriter instance.
 * (destructor)
 *
 */
 //------------------------------------------------------------------------------
SpiceAttitudeKernelWriter::~SpiceAttitudeKernelWriter()
{
   if (!dataWritten)
   {
      //      MessageInterface::ShowMessage(
      //            "*** WARNING *** ck kernel %s not written - no data provided\n",
      //            kernelFileName.c_str());
   }
   try { FinalizeKernel(); }
   catch (...)
   {
      ReportCKCleanupFailure(kernelFileName);
      try { AbortKernel(); }
      catch (...) { ReportCKCleanupFailure(kernelFileName); }
   }
   delete sclk;
   sclk = NULL;
}


//------------------------------------------------------------------------------
//  SpiceAttitudeKernelWriter* Clone(void) const
//------------------------------------------------------------------------------
/**
 * This method clones the object.
 *
 * @return new object, cloned from "this" object.
 *
 */
 //------------------------------------------------------------------------------
SpiceAttitudeKernelWriter* SpiceAttitudeKernelWriter::Clone(void) const
{
   SpiceAttitudeKernelWriter* clonedSKW = new SpiceAttitudeKernelWriter(*this);

   return clonedSKW;
}


//------------------------------------------------------------------------------
//  void SetCKFile(const std::string &kernelFile)
//------------------------------------------------------------------------------
/**
 * This method set the file for the CK kernel. It can be used to switch to a
 * new file when starting a new segment, but the previous must have been
 * properly handled and closed first.
 *
 * @kernelFile The new kernel file to use
 *
 */
//------------------------------------------------------------------------------
void SpiceAttitudeKernelWriter::SetCKFile(const std::string &kernelFile)
{
   if (fileOpen)
   {
      std::string errmsg = "Error setting new CK file, the previous file " +
         kernelFileName + " was not finalized and closed.\n";
      throw UtilityException(errmsg);
   }

   if (kernelFile.empty()) throw UtilityException("Select a nonempty CK output filename.\n");
   std::string prepared(kernelFile);
   kernelFileName.swap(prepared);
   kernelNameSPICE = kernelFileName.c_str();
   handle = -999;
   fileFinalized = false;
}

//------------------------------------------------------------------------------
//  void WriteSegment(const A1Mjd &start, const A1Mjd &end,
//                    const StateArray &states, const EpochArray &epochs)
//------------------------------------------------------------------------------
/**
 * This method stores segment data for a CK file. It will later be written to
 * the file once the SCLK file is created.
 *
 * @param   start    start time of the segment data
 * @param   end      end time of the segment data
 * @param   states   array of quaternions to write to the segment
 * @param   epochs   array of corresponding epochs
 *
 */
 //------------------------------------------------------------------------------
void SpiceAttitudeKernelWriter::StoreSegment(const A1Mjd& start, const A1Mjd& end,
   const std::vector<Rvector>& quats, const EpochArray& epochs)
{
   #ifdef DEBUG_SPK_KERNELS
      MessageInterface::ShowMessage("In SOKW::WriteSegment, start = %12.10, end = %12.10\n",
         start.Get(), end.Get());
      MessageInterface::ShowMessage("   writing %d states\n", (Integer)states.size());
      MessageInterface::ShowMessage("   fileOpen = %s\n", (fileOpen ? "true" : "false"));
   #endif
   if (quats.empty() || quats.size() != epochs.size())
      throw UtilityException("CK segments require matching nonempty epoch and quaternion arrays.\n");
   if (epochs.size() > static_cast<size_t>(std::numeric_limits<Integer>::max()) ||
         epochs.size() > static_cast<size_t>(std::numeric_limits<SpiceInt>::max()))
      throw UtilityException("CK segment size exceeds native index capacity.\n");
   std::vector<A1Mjd> currSegEpochs;
   currSegEpochs.reserve(epochs.size());
   for (Integer ii = 0; ii < epochs.size(); ++ii)
   {
      if (!epochs[ii] || quats[ii].GetSize() != 4)
         throw UtilityException("CK segments require valid epochs and four-element quaternions.\n");
      currSegEpochs.push_back(epochs.at(ii)->Get());
   }
   auto starts = segStarts; auto ends = segEnds;
   auto attitudes = segAttitudes; auto times = segEpochs;
   starts.push_back(start); ends.push_back(end);
   attitudes.push_back(quats); times.push_back(std::move(currSegEpochs));
   segStarts.swap(starts); segEnds.swap(ends);
   segAttitudes.swap(attitudes); segEpochs.swap(times);
   fileFinalized = false;
   dataWritten = false;

   WriteCKData();
}

//------------------------------------------------------------------------------
//  SetBasicMetaData()
//------------------------------------------------------------------------------
/**
 * This method sets the 'basic' (i.e. written to every kernel)  meta data
 * (comments).
 *
 */
 //------------------------------------------------------------------------------
void SpiceAttitudeKernelWriter::SetBasicMetaData()
{
   basicMetaData.clear();
   std::string metaDataLine = "--- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---\n";
   basicMetaData.push_back(metaDataLine);
   metaDataLine = ("CK ATTITUDE kernel for object " + objectName) + "\n";
   basicMetaData.push_back(metaDataLine);
   metaDataLine = "Generated on ";
   metaDataLine += GmatTimeUtil::FormatCurrentTime();
   metaDataLine += "\n";
   basicMetaData.push_back(metaDataLine);
   metaDataLine = "Generated by the General Mission Analysis Tool (GMAT) [Build ";

   metaDataLine += GmatGlobal::Instance()->GetGMATBuildDate();
   metaDataLine += " at ";
   metaDataLine += GmatGlobal::Instance()->GetGMATBuildTime();

   metaDataLine += "]\n";
   basicMetaData.push_back(metaDataLine);
   metaDataLine = "--- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---\n";
   basicMetaData.push_back(metaDataLine);
}

//------------------------------------------------------------------------------
//  FinalizeKernel(bool done = true, writeMetaData = true)
//------------------------------------------------------------------------------
/**
 * This method writes the meta data (comments) to the kernel and then closes it.
 *
 */
//------------------------------------------------------------------------------
void SpiceAttitudeKernelWriter::FinalizeKernel(bool done, bool writeMetaData)
{
#ifdef DEBUG_SPK_WRITING
   MessageInterface::ShowMessage("In FinalizeKernel .... tmpFileOK = %s\n",
      (tmpFileOK ? "true" : "false"));
   MessageInterface::ShowMessage("In FinalizeKernel .... kernelFileName = %s\n",
      kernelFileName.c_str());
   MessageInterface::ShowMessage("In FinalizeKernel .... fileOpen = %s\n",
      (fileOpen ? "true" : "false"));
   MessageInterface::ShowMessage("In FinalizeKernel .... dataWritten = %s\n",
      (dataWritten ? "true" : "false"));
#endif

   if (fileOpen && !dataWritten) // Write the data that's been stored
      WriteCKData();

   if ((fileOpen) && (dataWritten)) // should be both or neither are true
   {
      // write all the meta data to the file
      if (tmpFileOK && (done || writeMetaData)) WriteMetaData();
#ifdef DEBUG_SPK_WRITING
      MessageInterface::ShowMessage("In SOKW::FinalizeKernel ... is it loaded?  %s\n",
         (IsLoaded(kernelFileName) ? "true" : "false"));
#endif
      // close the ck file
      ckcls_c(handle);
      if (failed_c())
      {
         // CKCLS can fail before DAFCLS, including an empty CK. Keep owning
         // the handle so a retry or explicit abort can release it.
         GenerateSpiceErrorMessage("Error closing CK kernel \"" + kernelFileName + "\".");
      }
      fileOpen = false;
      handle = -999;
   }
   if (done)
   {
      basicMetaData.clear();
      addedMetaData.clear();
      fileFinalized = true;
      appending = false;
   }
   else
   {
      appending = true;
   }
}

void SpiceAttitudeKernelWriter::AbortKernel()
{
   if (fileOpen)
   {
      reset_c(); // Cleanup must still close a handle after a native failure.
      dafcls_c(handle);
      if (failed_c()) GenerateSpiceErrorMessage("Error aborting CK kernel \"" + kernelFileName + "\".");
      fileOpen = false;
      handle = -999;
   }
   ClearStoredData();
   basicMetaData.clear(); addedMetaData.clear();
   fileFinalized = true; dataWritten = false; appending = false;
}

//------------------------------------------------------------------------------
// void WriteCKData()
//------------------------------------------------------------------------------
/**
 * Method to write CK attitude data to the file. This is called after all data
 * has been stored in order to ensure the total timespan has been added to
 * create an SCLK as times for attitudes must be provided in ticks.
 *
 */
//------------------------------------------------------------------------------
void SpiceAttitudeKernelWriter::WriteCKData()
{
   if (generateSCLK)
      WriteSCLKData();

   // If the file has not been set up yet, open it if we have data to write
   if (!fileOpen && segStarts.size() != 0)
      OpenFileForWriting();

   for (UnsignedInt segNum = 0; segNum < segStarts.size(); ++segNum)
   {
      const auto count = segAttitudes[segNum].size();
      if (count == 0 || count > static_cast<size_t>(std::numeric_limits<SpiceInt>::max()) ||
            count > std::vector<SpiceDouble>().max_size() / 4)
         throw UtilityException("CK segment size exceeds native storage capacity.\n");
      SpiceInt numAtts = static_cast<SpiceInt>(count);
      if ((Integer)segEpochs[segNum].size() != (Integer)numAtts)
      {
         std::string errmsg = "Error writing segment to CK file \"";
         errmsg += kernelFileName + "\" - size of epoch array does not "
            "match size of attitude array.\n";
         throw UtilityException(errmsg);
      }

      // Convert time to spacecraft clock ticks
      SpiceDouble startTick, endTick, currentTick;
      std::vector<SpiceDouble> tickArray(count);
      sce2c_c(objectNAIFId, A1ToSpiceTime(segStarts[segNum].Get()), &startTick);
      sce2c_c(objectNAIFId, A1ToSpiceTime(segEnds[segNum].Get()), &endTick);
      for (Integer ii = 0; ii < numAtts; ii++)
      {
         sce2c_c(objectNAIFId, A1ToSpiceTime(segEpochs[segNum].at(ii).Get()), &currentTick);
         tickArray[ii] = currentTick;
         #ifdef DEBUG_CK_WRITING
                  MessageInterface::ShowMessage("epochArray[%d] = %12.10f\n", ii, (Real)epochArray[ii]);
         #endif
      }

      if (failed_c())
      {
         std::string errDesc = "Error getting tick values from SCLK file \"" +
            sclkFileName + "\".";
         GenerateSpiceErrorMessage(errDesc);
      }

      SpiceDouble tickRate = 1.0;

      // Put attitudes into SpiceDouble arrays
      std::vector<SpiceDouble> attArray(count * 4);

      for (Integer ii = 0; ii < numAtts; ii++)
      {
         // SPICE quaternion format is [q3, -q0, -q1, -q2]
         attArray[(ii * 4)] = ((segAttitudes[segNum].at(ii)).GetDataVector())[3];
         attArray[(ii * 4) + 1] = -((segAttitudes[segNum].at(ii)).GetDataVector())[0];
         attArray[(ii * 4) + 2] = -((segAttitudes[segNum].at(ii)).GetDataVector())[1];
         attArray[(ii * 4) + 3] = -((segAttitudes[segNum].at(ii)).GetDataVector())[2];
      }

      std::string segmentID = "CK_SEGMENT" + GmatStringUtil::ToString(Integer(segNum));
      ConstSpiceChar* segmentIDSPICE = segmentID.c_str();

      // Create the interpolation intervals
      SpiceInt numInterpInts = 1;
      std::vector<SpiceDouble> intStarts(numInterpInts);
      for (Integer ii = 0; ii < numInterpInts; ++ii)
      {

         intStarts[ii] = tickArray[ii * 2];

      }

      ckw05_c(handle, C05TP1, degree, startTick, endTick, objectNAIFId * 1000,
         referenceFrame, false, segmentIDSPICE, numAtts, tickArray.data(),
         attArray.data(), tickRate, numInterpInts, intStarts.data());

      if (failed_c())
      {
         std::string errDesc = "Error writing ephemeris data to CK file \"" +
            kernelFileName + "\".";
         GenerateSpiceErrorMessage(errDesc);
      }
   }

   dataWritten = true;
   ClearStoredData();

#ifdef DEBUG_SPK_KERNELS
   MessageInterface::ShowMessage("In SOKW::WriteSegment, data has been written\n");
#endif
}

//------------------------------------------------------------------------------
// void WriteSCLKData()
//------------------------------------------------------------------------------
/**
 * Method to generate a clock file for the spacecraft. This file is needed to
 * be able to store and grab attitude data for a spacecraft in SPICE. A generic
 * tick of 1/1000 of a second is used to generate across the stored timespan.
 *
 */
//------------------------------------------------------------------------------
void SpiceAttitudeKernelWriter::WriteSCLKData()
{
   // If no data was collected, return without writing
   if (segEpochs.size() == 0)
      return;

   // Get total timespan covered by spacecraft
   A1Mjd startTime = segStarts.front();
   A1Mjd endTime = segEnds.back();

   Real startTimeSpice = A1ToSpiceTime(startTime.Get());
   Real endTimeSpice = A1ToSpiceTime(endTime.Get());

   // Round to a second
   startTimeSpice = GmatMathUtil::Floor(startTimeSpice);
   endTimeSpice = GmatMathUtil::Ceiling(endTimeSpice);

   Real tickStart = 0;
   if (sclk)
   {
      Rvector3 prevFinal = sclk->GetFinalTimeCoefficients();
      // Accepted estimator event boundaries can repeat an epoch, including
      // the final flush. The existing clock already maps this interval; do
      // not retire it or append duplicate coefficients when it need not grow.
      if (endTimeSpice <= prevFinal(1))
         return;
      startTimeSpice = prevFinal(1);
      tickStart = prevFinal(0);
   }

   // Generate SCLK coefficients
   Real span = endTimeSpice - startTimeSpice;

   A1Mjd modStartTime = SpiceTimeToA1(startTimeSpice);
   A1Mjd modEndTime = SpiceTimeToA1(endTimeSpice);

   std::vector<Rvector3> clockCoeffs;
   if (tickStart == 0)
      clockCoeffs.push_back(Rvector3(tickStart, startTimeSpice, 1.0));
   clockCoeffs.push_back(Rvector3(span * 1000.0 + tickStart, endTimeSpice, 1.0));

   // If this file already existed it needs to be rewritten to account for new time data
   bool reloadKernels = false;
   if (sclk)
   {
      // Both kernels need to be unloaded to provide new data to them
      GmatSpiceKernelFiles::Unload(sclk, sclkFileName);
      GmatSpiceKernelFiles::Unload(this, kernelFileName);
      GmatSpiceKernelFiles::Retire(sclk, sclkFileName);
      reloadKernels = true;
   }

   if (sclk == NULL)
   {
      RealArray modArray, offsetArray;
      modArray.push_back(4294967296.0);
      modArray.push_back(1000.0);
      offsetArray.push_back(0);
      offsetArray.push_back(0);
      sclk = new SpiceSCClockKernelWriter("SpacecraftClockFile", objectNAIFId,
         1000.0, modArray, offsetArray, "1", modStartTime, sclkFileName);
   }

   sclk->AddCoefficientData(clockCoeffs);
   sclk->AddPartitionSegment(modStartTime, modEndTime);

   if (reloadKernels)
      sclk->WriteSCLKCoefficients(true);
   else
      sclk->WriteSCLKCoefficients();

   sclk->LoadKernel(sclkFileName);
}

//------------------------------------------------------------------------------
// void AddMetaData(const std::string& line, bool done)
//------------------------------------------------------------------------------
/**
 * Method to store meta data to later be written to the file, while also
 * checking if we are done and ready to write and finalize the file now.
 * 
 * @param line The line of meta data to store and later write
 * @param done Whether we are done adding data to write to the file
 *
 */
//------------------------------------------------------------------------------
void SpiceAttitudeKernelWriter::AddMetaData(const std::string& line, bool done)
{
   if (fileFinalized)
   {
      std::string errmsg = "Unable to add meta data to CK kernel \"";
      errmsg += kernelFileName + "\".  File has been finalized and closed.\n";
      throw UtilityException(errmsg);
   }
   addedMetaData.push_back(line);

   if (done)
      FinalizeKernel();
}

//------------------------------------------------------------------------------
//  Integer GetMinNumberOfStates()
//------------------------------------------------------------------------------
/**
 * This method returns the minimum number if states required by SPICE to
 * so the interpolation.
 *
 */
 //------------------------------------------------------------------------------
Integer SpiceAttitudeKernelWriter::GetMinNumberOfAttitudes()
{
   return degree + 1;
}


//------------------------------------------------------------------------------
//  WriteMetaData()
//------------------------------------------------------------------------------
/**
 * This method writes the meta data (comments) to the kernel.
 *
 */
 //------------------------------------------------------------------------------
void SpiceAttitudeKernelWriter::WriteMetaData()
{
   // open the temporary file for writing the metadata
   struct CloseText
   {
      void operator()(FILE *file) const noexcept { if (file) fclose(file); }
   };
   std::unique_ptr<FILE, CloseText> text(fopen(tmpTxtFileName.c_str(), "w"));
   if (!text) throw UtilityException("Cannot open CK metadata file \"" + tmpTxtFileName + "\".\n");

   // write the meta data to the temporary file (according to SPICE documentation, must use regular C routines)
   // close the temporary file, when done
   unsigned int basicSize = basicMetaData.size();
   unsigned int addedSize = addedMetaData.size();
   for (unsigned int ii = 0; ii < basicSize; ii++)
      fprintf(text.get(), "%s", (basicMetaData[ii]).c_str());
   fprintf(text.get(), "\n");
   for (unsigned int ii = 0; ii < addedSize; ii++)
      fprintf(text.get(), "%s", (addedMetaData[ii]).c_str());
   fprintf(text.get(), "\n");
   if (fflush(text.get()) != 0 || ferror(text.get()))
      throw UtilityException("Cannot write CK metadata file \"" + tmpTxtFileName + "\".\n");
   if (fclose(text.release()) != 0)
      throw UtilityException("Cannot close CK metadata file \"" + tmpTxtFileName + "\".\n");

   // write the meta data to the ck file comment area by telling it to read the
   // temporary text file
   if (tmpTxtFileName.size() >= static_cast<size_t>(std::numeric_limits<ftnlen>::max()))
      throw UtilityException("CK metadata filename exceeds native text capacity.\n");
   std::vector<char> tmpTxt(tmpTxtFileName.begin(), tmpTxtFileName.end());
   tmpTxt.push_back('\0');
   struct TextUnit
   {
      integer number = 0;
      bool open = false;
      ~TextUnit()
      {
         if (open) { reset_c(); ftncls_c(number); if (failed_c()) reset_c(); }
      }
   } unit;
   char        blank[2] = " ";
   ftnlen      txtLen = static_cast<ftnlen>(tmpTxt.size());
   txtopr_(tmpTxt.data(), &unit.number, txtLen);
   if (failed_c()) GenerateSpiceErrorMessage("Cannot read CK metadata file \"" + tmpTxtFileName + "\".");
   unit.open = true;
   spcac_(&handle, &unit.number, blank, blank, 1, 1);
   if (failed_c())
   {
      std::string errDesc = "Error writing meta data to ck file \"" +
         kernelFileName + "\".";
      GenerateSpiceErrorMessage(errDesc);
   }
   // close the text file
   ftncls_c(unit.number);
   if (failed_c()) GenerateSpiceErrorMessage("Cannot release CK metadata file \"" + tmpTxtFileName + "\".");
   unit.open = false;
   // remove the temporary text file
   remove(tmpTxtFileName.c_str());
}

//------------------------------------------------------------------------------
//  bool OpenFileForWriting()
//------------------------------------------------------------------------------
/**
 * This method sets up and opens the file for writing.
 *
 */
//------------------------------------------------------------------------------
bool SpiceAttitudeKernelWriter::OpenFileForWriting()
{
   // get a file handle here
   SpiceInt        maxChar = MAX_CHAR_COMMENT;
#ifdef DEBUG_SPK_INIT
   MessageInterface::ShowMessage("... attempting to open ck file with  fileName = %s\n",
      kernelFileName.c_str());
   MessageInterface::ShowMessage("... and appending = %s\n", (appending ? "true" : "false"));
   MessageInterface::ShowMessage("    handle = %d\n", (Integer)handle);
#endif

   kernelNameSPICE = kernelFileName.c_str();
   SpiceInt preparedHandle = -999;
   bool fileExists = GmatFileUtil::DoesFileExist(kernelFileName);
   if (!fileExists)
   {
      std::string     internalFileName = "GMAT-generated CK file for " + objectName;
      ConstSpiceChar* internalSPKName = internalFileName.c_str();
      ckopn_c(kernelNameSPICE, internalSPKName, maxChar, &preparedHandle);
   }
   else
   {
      SpiceChar architecture[8], kind[8];
      getfat_c(kernelNameSPICE, sizeof(architecture), sizeof(kind), architecture, kind);
      if (failed_c()) GenerateSpiceErrorMessage("Cannot inspect CK output \"" + kernelFileName + "\".");
      if (!eqstr_c(architecture, "DAF") || !eqstr_c(kind, "CK"))
         throw UtilityException("Cannot append to a file that is not a CK kernel: " + kernelFileName + "\n");
      dafopw_c(kernelNameSPICE, &preparedHandle);
   }
   if (failed_c()) // CSPICE method to detect failure of previous call to CSPICE
   {
      std::string errDesc = "Error getting file handle for ck file \"" +
         kernelFileName + "\".";
      GenerateSpiceErrorMessage(errDesc);
   }

   handle = preparedHandle;
   fileOpen = true;
   fileFinalized = false;
   return true;
}

//------------------------------------------------------------------------------
//  void ClearStoredData()
//------------------------------------------------------------------------------
/**
 * Method to clear the currently stored attitude segment data.
 *
 */
//------------------------------------------------------------------------------
void SpiceAttitudeKernelWriter::ClearStoredData()
{
   segStarts.clear();
   segEnds.clear();
   segAttitudes.clear();
   segEpochs.clear();
}

//------------------------------------------------------------------------------
//  void GenerateSpiceErrorMessage(std::string errDesc)
//------------------------------------------------------------------------------
/**
 * Method to generate and throw a SPICE related error
 *
 * @param errDesc The description of the error from the GMAT perspective which
 *        will have the SPICE error message added onto it.
 */
//------------------------------------------------------------------------------
void SpiceAttitudeKernelWriter::GenerateSpiceErrorMessage(std::string errDesc)
{
   // Clear the problematic data so we can close the file during cleanup
   ClearStoredData();

   ConstSpiceChar option[] = "LONG"; // retrieve long error message
   SpiceInt       numErrChar = MAX_LONG_MESSAGE_VALUE;
   SpiceChar* err = new SpiceChar[MAX_LONG_MESSAGE_VALUE];
   getmsg_c(option, numErrChar, err);
   std::string errStr(err);
   std::string errmsg = errDesc + "  Message received from CSPICE is: ";
   errmsg += errStr + "\n";
   reset_c();
   delete[] err;
   throw UtilityException(errmsg);
}

//---------------------------------
// private methods
//---------------------------------

//------------------------------------------------------------------------------
//  SpiceAttitudeKernelWriter()
//------------------------------------------------------------------------------
/**
 * This method constructs an instance of SpiceAttitudeKernelWriter.
 * (default constructor)
 *
 */
 //------------------------------------------------------------------------------
SpiceAttitudeKernelWriter::SpiceAttitudeKernelWriter()
{
};
