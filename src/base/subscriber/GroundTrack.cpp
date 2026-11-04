//------------------------------------------------------------------------------
//                           GroundTrack
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool
//
// Copyright (c) 2002-2026 United States Government as represented by the
// Administrator of the National Aeronautics and Space Administration.
// All Other Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// You may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
// http://www.apache.org/licenses/LICENSE-2.0.
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied.   See the License for the specific language
// governing permissions and limitations under the License.
//
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under the FDSS III
// contract
//
// Author: Claire R. Conway, Thinking Systems, Inc.
// Created: Sep 12, 2025
/**
 * Base code subscriber for the new ground track component/
 *
 * This implementation is ported from Astrodynamics Workbench(R).
 */
//------------------------------------------------------------------------------

#include "SolverBranchCommand.hpp"
#include "GroundTrack.hpp"
#include "PlotInterface.hpp"         // for XY plot
#include "SubscriberException.hpp"
#include "StringUtil.hpp"            // for GmatStringUtil::ReplaceName(), GetArrayName()
#include "MessageInterface.hpp"      // for ShowMessage()
#include "SpaceObject.hpp"
#include <cstring>
#include <cmath>
#include <limits>
#include <iomanip>
#include <sstream>                   // for <<
#include "AngleUtil.hpp"

//---------------------------------
// static data
//---------------------------------

const std::string
GroundTrack::PARAMETER_TEXT[GroundTrackParamCount - SubscriberParamCount] =
      {
         "CentralBody",   // CENTRAL_BODY
         "TextureMap",
         "Add",
         "ShowName",
         "LineWidth",
         "DataCollectFrequency",
         "UpdatePlotFrequency",
         "NumPointsToRedraw",
         "MaxPlotPoints",
         "ShowPlot",
         // Hidden for now:
         "ShowFootPrints",
         "UseGrid",            // USE_GRID,
         "LatitudeLineCount",  // LATLINE_COUNT,
         "LongitudeLineCount", // LONGLINE_COUNT
         "IntegrationStepsToSkip"
      };

const Gmat::ParameterType
GroundTrack::PARAMETER_TYPE[GroundTrackParamCount - SubscriberParamCount] =
      {
         Gmat::OBJECT_TYPE,
         Gmat::FILENAME_TYPE,
         Gmat::OBJECTARRAY_TYPE,
         Gmat::BOOLEANARRAY_TYPE,
         Gmat::INTEGER_TYPE,
         Gmat::INTEGER_TYPE,
         Gmat::INTEGER_TYPE,
         Gmat::INTEGER_TYPE,
         Gmat::INTEGER_TYPE,
         Gmat::BOOLEAN_TYPE,
         Gmat::STRING_TYPE,
         Gmat::BOOLEAN_TYPE,
         Gmat::INTEGER_TYPE,
         Gmat::INTEGER_TYPE,
         Gmat::INTEGER_TYPE
      };

void GroundTrack::SetRunState(Gmat::RunState state)
{
   const auto next = SolverBranchCommand::GetPlotRunState(state);
   if (next != runstate) previousSample = BoundarySample{};
   Subscriber::SetRunState(next);
}

//------------------------------------------------------------------------------
// GroundTrack(const std::string &name)
//------------------------------------------------------------------------------
/**
 * The default constructor
 */
//------------------------------------------------------------------------------
GroundTrack::GroundTrack(const std::string &name) :
   Subscriber("GroundTrack", name),
   theBodyName       ("Earth"),
   theSS             (nullptr),
   theBody           (nullptr),
   mapFile           (""),
   lineWidth         (2),
   collectFrequency  (1),
   samplesUntilCollect (0),
   updateFrequency   (50),
   redrawCount       (0),
   maxData           (20000),
   showPlot          (true),
   footprintType     ("None"),
   useGrid           (true),
   latLineCount      (6),        // 30 degrees apart
   longLineCount     (12),       // 30 degrees apart
   viewCoordinates   (nullptr),
   inertialSystem    (nullptr),
   bodyRadius        (6378.1363),
   bodyFlattening    (0.0),
   longlat           (nullptr),
   pointlonglat      (nullptr)
{
   // GmatBase data
   objectTypes.push_back(Gmat::GROUNDTRACK);
   objectTypeNames.push_back("GroundTrack");
   objectTypeNames.push_back("GroundTrackPlot");

   parameterCount = GroundTrackParamCount;
}

//------------------------------------------------------------------------------
// ~GroundTrack()
//------------------------------------------------------------------------------
/**
 * Destructor
 */
//------------------------------------------------------------------------------
GroundTrack::~GroundTrack()
{
   ClearRuntimeResources();
}

//------------------------------------------------------------------------------
// GroundTrack(const GroundTrack& gt)
//------------------------------------------------------------------------------
/**
 * The copy constructor
 *
 * @param gt The GroundTrack being copied
 */
//------------------------------------------------------------------------------
GroundTrack::GroundTrack(const GroundTrack& gt) :
   Subscriber(gt),
   viewCoordinates(nullptr),
   inertialSystem(nullptr),
   longlat(nullptr),
   pointlonglat(nullptr)
{
   *this = gt;
}

//------------------------------------------------------------------------------
// GroundTrack& operator=(const GroundTrack& gt)
//------------------------------------------------------------------------------
/**
 * The assignment operator
 *
 * @param gt The GroundTrack being copied
 */
//------------------------------------------------------------------------------
GroundTrack& GroundTrack::operator=(const GroundTrack& gt)
{
   if (this != &gt)
   {
      ClearRuntimeResources();
      Subscriber::operator=(gt);
      isInitialized = false;

      theBodyName = gt.theBodyName;
      theSS = gt.theSS;
      theBody = gt.theBody;
      mapFile = gt.mapFile;
      lineWidth = gt.lineWidth;
      collectFrequency = gt.collectFrequency;
      integrationStepsToSkip = gt.integrationStepsToSkip;
      integrationSampler.Reset();
      previousSample = BoundarySample{};
      samplesUntilCollect = 0;
      updateFrequency = gt.updateFrequency;
      redrawCount = gt.redrawCount;
      maxData = gt.maxData;
      showPlot = gt.showPlot;
      useGrid = gt.useGrid;
      latLineCount = gt.latLineCount;
      longLineCount = gt.longLineCount;
      bodyRadius = gt.bodyRadius;
      bodyFlattening = gt.bodyFlattening;

      theObjects = gt.theObjects;
      showName = gt.showName;
      footprintType = gt.footprintType;
      ccvt = gt.ccvt;
      theSats = gt.theSats;
      theStations = gt.theStations;
      theBodies = gt.theBodies;
      thePoints = gt.thePoints;
      stateMap = gt.stateMap;
      stateIndices = gt.stateIndices;
   }
   return *this;
}

// Release only resources owned by this subscriber. References to spacecraft,
// stations and solar-system bodies belong to the configuration or sandbox.
void GroundTrack::ClearRuntimeResources()
{
   delete [] longlat;
   delete [] pointlonglat;
   delete viewCoordinates;
   delete inertialSystem;
   longlat = nullptr;
   pointlonglat = nullptr;
   viewCoordinates = nullptr;
   inertialSystem = nullptr;
}

//------------------------------------------------------------------------------
// GmatBase* Clone() const
//------------------------------------------------------------------------------
/**
 * This method returns a clone of the GroundTrack.
 *
 * @return clone of the GroundTrack.
 *
 */
//------------------------------------------------------------------------------
GmatBase* GroundTrack::Clone() const
{
   return new GroundTrack(*this);
}

//------------------------------------------------------------------------------
// void Copy(const GmatBase* orig)
//------------------------------------------------------------------------------
/**
 * The default copy constructor
 */
//------------------------------------------------------------------------------
void GroundTrack::Copy(const GmatBase* orig)
{
   if (orig->IsOfType("GroundTrack"))
      operator=(*((GroundTrack*)orig));
}

//------------------------------------------------------------------------------
// bool Initialize()
//------------------------------------------------------------------------------
/**
 * Prepares the subscriber for use
 */
bool GroundTrack::Initialize()
{
   integrationSampler.Reset();
   previousSample = BoundarySample{};
   Publisher::SetIntegrationStepConsumer(this, integrationStepsToSkip >= 0);
   bool retval = false;
   Integer stationCount = 0;

   ClearRuntimeResources();
   isInitialized = false;
   samplesUntilCollect = 0;
   theBody = nullptr;
   if (theSS)
      theBody = theSS->GetBody(theBodyName);

   if (theBody && theSS)
   {
      CelestialBody *j2000Body = theSS->GetBody("Earth");

      bodyRadius = theBody->GetEquatorialRadius();
      bodyFlattening = theBody->GetFlattening();

      // Check that the objects are set matching the object list
      int count = theSats.size() + theStations.size();

      if (count == theObjects.size())
         retval = true;

      // Access spacecraft state origin and ensure it matches for all spacecraft
      std::string scOrigin = "Earth";

      if (!theSats.empty())
      {
         scOrigin = theSats[0]->GetOriginName();

         for (int i = 1; i < theSats.size(); ++i)
         {
             if (scOrigin != theSats[i]->GetOriginName())
                throw SubscriberException("Spacecraft origins must match for "
                      "GroundTrack displays");
         }

      }

      for (int i = 0; i < theStations.size(); ++i)
      {
         if (theBodyName != theStations[i]->GetStringParameter("CentralBody"))
            MessageInterface::ShowMessage("Omitting ground station %s; "
                      "GroundTracks only show stations on the ground track "
                      "central body (%s for %s)\n",
                      theStations[i]->GetName().c_str(), theBodyName.c_str(),
                      instanceName.c_str());
         else
            ++stationCount;
      }

      count += theBodies.size();

      // Setup the buffer used for data passing - reset for groundtrack
      longlat = new double[2 * theSats.size()];

      pointlonglat = new double[2 * stationCount];

      #ifdef DEBUG_INITIALIZATION
         MessageInterface::ShowMessage("Groundtrack %s Sats:\n",
               instanceName.c_str());
         for (int i = 0; i < theSats.size(); ++i)
            MessageInterface::ShowMessage("   %s\n",
                  theSats[i]->GetName().c_str());

         MessageInterface::ShowMessage("Groundtrack %s Stations:\n",
               instanceName.c_str());
         for (int i = 0; i < theStations.size(); ++i)
            MessageInterface::ShowMessage("   %s\n",
                  theStations[i]->GetName().c_str());
      #endif

      viewCoordinates  = CoordinateSystem::CreateLocalCoordinateSystem(
            instanceName + "FixedCS", "BodyFixed", theBody, nullptr, nullptr,
            j2000Body, theSS);

      inertialSystem = CoordinateSystem::CreateLocalCoordinateSystem(
            instanceName + "StateCS", "MJ2000Eq", j2000Body, nullptr, nullptr,
            j2000Body, theSS);

      #ifdef DEBUG_INITIALIZATION
         MessageInterface::ShowMessage("%s Initialized\n\n",
               (bodyFound ? "" : "Not"));
      #endif
   }
   else
   {
      MessageInterface::ShowMessage("Cannot find the body %s for %s\n",
            theBodyName.c_str(), instanceName.c_str());
      return false;
   }

   // Now interact with the GUI
   if (showPlot && PlotInterface::CreateGroundTrackWindow(instanceName,
          "", mPlotUpperLeft[0], mPlotUpperLeft[1],
          mPlotSize[0], mPlotSize[1], isMaximized))
   {
      // The body's texture may be outside TEXTURE_PATH. Keep its resolved
      // path, and leave the scripted setting unchanged for subsequent runs.
      std::string backgroundMap = mapFile;
      if (backgroundMap.empty() && theBody)
         backgroundMap = theBody->GetStringParameter(
               theBody->GetParameterID("TextureMapFullPath"));
      // Pass in GroundTrack parameters
      PlotInterface::SetGroundTrackOption(instanceName, "TextureMap", backgroundMap);

      // Remove stale data if present - we might be reusing the window
      PlotInterface::TakeGroundTrackAction(instanceName, "Reinitialize");
      PlotInterface::TakeGroundTrackAction(instanceName, "ClearData");

      // A solver may start before the first propagator sets its data labels.
      // Name the empty curves now so its Current-mode break can be anchored
      // before trial data arrives. This publishes no trajectory samples.
      std::string satellites = "Satellites=";
      for (auto *satellite : theSats)
      {
         if (satellites.back() != '=') satellites += '|';
         satellites += satellite->GetName();
      }
      PlotInterface::TakeGroundTrackAction(instanceName, satellites);

      if (updateFrequency > 0)
      {
         std::string theAction = "PlotUpdateFrequency=" +
               std::to_string(updateFrequency);
         PlotInterface::TakeGroundTrackAction(instanceName, theAction);
      }

      if (useGrid)
      {
         PlotInterface::TakeGroundTrackAction(instanceName, "UseGrid=True");

         std::string theAction = "LatitudeLineCount=" +
               std::to_string(latLineCount);
         PlotInterface::TakeGroundTrackAction(instanceName, theAction);
         theAction = "LongitudeLineCount=" +
               std::to_string(longLineCount);
         PlotInterface::TakeGroundTrackAction(instanceName, theAction);
      }
      else
         PlotInterface::TakeGroundTrackAction(instanceName, "UseGrid=False");

      if (stationCount > 0)
      {
         for (int i = 0; i < theStations.size(); ++i)
         {
             if (theBodyName == theStations[i]->GetStringParameter("CentralBody"))
             {
                std::string theAction = "AddStation=" + theStations[i]->GetName();
                PlotInterface::TakeGroundTrackAction(instanceName, theAction);
             }
         }
      }

      PlotInterface::TakeGroundTrackAction(instanceName, "Refresh");
   }

   isInitialized = retval;
   return retval;
}

//------------------------------------------------------------------------------
// bool IsParameterReadOnly(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Check scripting interface for writing
 *
 * @param id The parameter ID
 *
 * @retval True if the parameter is written on save
 */
bool GroundTrack::IsParameterReadOnly(const Integer id) const
{
   if ((id == TEXTURE_MAP) && (mapFile == ""))
      return true;

   // Add supplies true name flags by default. Preserve an explicit false
   // flag on export; hiding the setting must not erase its configured value.
   if (id == SHOWNAME)
   {
      for (bool flag : showName)
         if (!flag) return false;
      return true;
   }

   // Hidden controls still configure the native plot. Keep defaults implicit,
   // but serialize nondefaults so Save/reload does not reset the grid.
   if (id == USE_GRID) return useGrid;
   if (id == LATLINE_COUNT) return latLineCount == 6;
   if (id == LONGLINE_COUNT) return longLineCount == 12;

   if ((id == LINE_WIDTH) && (lineWidth == 2))
      return true;

   // Hidden and not currently used setting
   if (id == SHOW_FOOTPRINTS)
      return true;

   return Subscriber::IsParameterReadOnly(id);;
}

//------------------------------------------------------------------------------
// bool IsParameterValid(const Integer id, const std::string &value)
//------------------------------------------------------------------------------
/**
 * Checks parameter update validity (currently unused in this code)
 *
 * @param id The parameter ID
 * @param value The value for the update being checked
 *
 * @retval True if the parameter value is valid
 */
bool GroundTrack::IsParameterValid(const Integer id, const std::string &value)
{
   bool retval = true;
   return retval;
}

//------------------------------------------------------------------------------
// bool IsParameterValid(const std::string &label, const std::string &value)
//------------------------------------------------------------------------------
/**
 * Checks parameter update validity (currently unused in this code)
 *
 * @param label The parameter script label
 * @param value The value for the update being checked
 *
 * @retval True if the parameter value is valid
 */
bool GroundTrack::IsParameterValid(const std::string &label,
      const std::string &value)
{
   return IsParameterValid(GetParameterID(label), value);
}

//------------------------------------------------------------------------------
// bool IsParameterCommandModeSettable(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Method used to check command mode update availability
 *
 * @param id The parameter ID
 *
 * @retval True if the parameter value can be changed during a run
 */
bool GroundTrack::IsParameterCommandModeSettable(const Integer id) const
{
   // Override for base class setting
   if ((id == SOLVER_ITERATIONS) )
      return true;

   return Subscriber::IsParameterCommandModeSettable(id);
}

//------------------------------------------------------------------------------
// std::string GetParameterText(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Retries hte string associated with a parameter ID
 *
 * @param id The parameter ID
 *
 * @retval The string
 */
std::string GroundTrack::GetParameterText(const Integer id) const
{
   if (id >= SubscriberParamCount && id < GroundTrackParamCount)
      return PARAMETER_TEXT[id - SubscriberParamCount];

   return Subscriber::GetParameterText(id);
}

//------------------------------------------------------------------------------
// Integer GetParameterID(const std::string &str) const
//------------------------------------------------------------------------------
/**
 * Retrieves the parameter ID associated with a string
 *
 * @param str The string
 *
 * @retval The parameter ID
 */
Integer GroundTrack::GetParameterID(const std::string &str) const
{
   for (int i = SubscriberParamCount; i < GroundTrackParamCount; ++i)
   {
      if (str == PARAMETER_TEXT[i - SubscriberParamCount])
         return i;
   }

   return Subscriber::GetParameterID(str);
}

//------------------------------------------------------------------------------
// Gmat::ParameterType GetParameterType(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Retrieve the enumerated type of the object.
 *
 * @param <id> The integer ID for the parameter.
 *
 * @return The enumeration for the type of the parameter, or
 *         UNKNOWN_PARAMETER_TYPE.
 */
Gmat::ParameterType GroundTrack::GetParameterType(const Integer id) const
{
   if (id >= SubscriberParamCount && id < GroundTrackParamCount)
      return PARAMETER_TYPE[id - SubscriberParamCount];

   return Subscriber::GetParameterType(id);
}

//------------------------------------------------------------------------------
// std::string GetParameterTypeString(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Retrieve the string associated with a parameter.
 *
 * @param <id> The integer ID for the parameter.
 *
 * @return Text description for the type of the parameter, or the empty
 *         string ("").
 */
std::string GroundTrack::GetParameterTypeString(const Integer id) const
{
   return GmatBase::PARAM_TYPE_STRING[GetParameterType(id)];
}

//------------------------------------------------------------------------------
// std::string GetStringParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Retrieve a string parameter.
 *
 * @param id The integer ID for the parameter.
 *
 * @return The string stored for this parameter, or the empty string if there
 *         is no string association.
 */
std::string GroundTrack::GetStringParameter(const Integer id) const
{
   switch (id)
   {
   case CENTRAL_BODY:
      return theBodyName;

   case TEXTURE_MAP:
      {
         if (mapFile == "" && theBody)
            return theBody->GetStringParameter(
                  theBody->GetParameterID("TextureMapFullPath"));

         return mapFile;
      }

   case SHOW_FOOTPRINTS:
      return footprintType;

   default:
      break;
   }

   return Subscriber::GetStringParameter(id);
}

//------------------------------------------------------------------------------
// bool SetStringParameter(const Integer id, const std::string &value)
//------------------------------------------------------------------------------
/**
 * Change the value of a string parameter.
 *
 * @param id The integer ID for the parameter.
 * @param value The new string for this parameter.
 *
 * @return true if the string is stored, throw if the parameter is not stored.
 */
bool GroundTrack::SetStringParameter(const Integer id, const std::string &value)
{
   switch (id)
   {
   case CENTRAL_BODY:
      theBodyName = value;
      return true;

   case TEXTURE_MAP:
      mapFile = value;
      return true;

   case ADD:
      if (find(theObjects.begin(), theObjects.end(), value) == theObjects.end())
      {
         if (value != theBodyName)
         {
            theObjects.push_back(value);
            showName.push_back(true);
         }
      }
      return true;

   case SHOW_FOOTPRINTS:
      if ((value == "None") || (value == "All"))
         footprintType = value;
      else
      {
         MessageInterface::ShowMessage("The value of \"%s\" for field "
               "\"ShowFootPrints\" on object \"%s\" is not an allowed value.\n"
               "The allowed values are: [None ,All]\n", value.c_str(),
               instanceName.c_str());
         return false;
      }
      return true;

   default:
      break;
   }

   return Subscriber::SetStringParameter(id, value);
}

//------------------------------------------------------------------------------
// std::string GetStringParameter(const Integer id, const Integer index) const
//------------------------------------------------------------------------------
/**
 * Retrieve a string parameter from a string array
 *
 * @param id The integer ID for the parameter.
 * @param index The index into the array.
 *
 * @return The string stored for this parameter, or the empty string if the
 * index is out of bounds.
 */
std::string GroundTrack::GetStringParameter(const Integer id,
                                const Integer index) const
{
   if (id == ADD)
   {
      if (index < theObjects.size() && index >= 0)
         return theObjects[index];

      return "";
   }

   // Calling to GmatBase because Subscriber has no indexed implementation
   return GmatBase::GetStringParameter(id, index);
}

//------------------------------------------------------------------------------
// bool SetStringParameter(const Integer id, const std::string &value,
//                         const Integer index)
//------------------------------------------------------------------------------
/**
 * Change the value of a string parameter in a string array
 *
 * @param id The integer ID for the parameter.
 * @param value The new string for this parameter.
 * @param index The index into the array.
 *
 * @return true if the string is stored, throw if the parameter is not stored.
 */
bool GroundTrack::SetStringParameter(const Integer id,
                                const std::string &value,
                                const Integer index)
{
   if (id == ADD)
   {
      bool retval = false;
      if (index == theObjects.size())
      {
         theObjects.push_back(value);
         showName.push_back(true);
         retval = true;
      }
      else if (index < theObjects.size() && index >= 0)
      {
         theObjects[index] = value;
         retval = true;
      }

      return retval;
   }

   return Subscriber::SetStringParameter(id, value, index);
}

//------------------------------------------------------------------------------
// std::string GetStringParameter(const std::string &label,
//.                               const Integer index) const
//------------------------------------------------------------------------------
/**
 * Retrieve a string parameter from a string array
 *
 * @param label The string for this string array parameter.
 * @param index The index into the array.
 *
 * @return The string stored for this parameter, or the empty string if the
 * index is out of bounds.
 */
std::string GroundTrack::GetStringParameter(const std::string &label,
                                const Integer index) const
{
   return GetStringParameter(GetParameterID(label), index);
}


//------------------------------------------------------------------------------
// bool SetStringParameter(const std::string &label,
//                         const std::string &value,
//                         const Integer index)
//------------------------------------------------------------------------------
/**
 * Change the value of a string parameter in a string array
 *
 * @param label The string for this string array parameter.
 * @param value The new string for this parameter.
 * @param index The index into the array.
 *
 * @return true if the string is stored, throw if the parameter is not stored.
 */
bool GroundTrack::SetStringParameter(const std::string &label,
                                const std::string &value,
                                const Integer index)
{
   return SetStringParameter(GetParameterID(label), value, index);
}


//------------------------------------------------------------------------------
// std::string GetStringParameter(const std::string &label) const
//------------------------------------------------------------------------------
/**
 * Retrieve the value of a string parameter.
 *
 * @param label The string for this parameter.
 *
 * @return The string
 */
std::string GroundTrack::GetStringParameter(const std::string &label) const
{
   return GetStringParameter(GetParameterID(label));
}

//------------------------------------------------------------------------------
// bool SetStringParameter(const std::string &label, const std::string &value)
//------------------------------------------------------------------------------
/**
 * Change the value of a string parameter.
 *
 * @param label The script string for the parameter.
 * @param value The new string for this parameter.
 *
 * @return true if the string is stored, throw if the parameter is not stored.
 */
bool GroundTrack::SetStringParameter(const std::string &label, const std::string &value)
{
   return SetStringParameter(GetParameterID(label), value);
}

//------------------------------------------------------------------------------
// const StringArray& GetStringArrayParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Access an array of string data.
 *
 * @param id The integer ID for the parameter.
 *
 * @return The requested StringArray; throws if the parameter is not a
 *         StringArray.
 */
const StringArray& GroundTrack::GetStringArrayParameter(const Integer id) const
{
   if (id == ADD)
      return theObjects;

   return Subscriber::GetStringArrayParameter(id);
}

//------------------------------------------------------------------------------
// const StringArray& GetStringArrayParameter(const std::string &label) const
//------------------------------------------------------------------------------
/**
 * Access an array of string data.
 *
 * @param label The scripting for the parameter.
 *
 * @return The requested StringArray; throws if the parameter is not a
 *         StringArray.
 */
const StringArray& GroundTrack::GetStringArrayParameter(const std::string &label) const
{
   return GetStringArrayParameter(GetParameterID(label));
}

//------------------------------------------------------------------------------
// Real GetRealParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Retrieve the value for a Real parameter.
 *
 * @param id The integer ID for the parameter.
 *
 * @return The parameter's value.
 */
Real GroundTrack::GetRealParameter(const Integer id) const
{
   return Subscriber::GetRealParameter(id);
}

//------------------------------------------------------------------------------
// Real SetRealParameter(const Integer id, const Real value)
//------------------------------------------------------------------------------
/**
 * Set the value for a Real parameter.
 *
 * @param id The integer ID for the parameter.
 * @param value The new parameter value.
 *
 * @return the parameter value at the end of this call, or throw an exception
 *         if the parameter id is invalid or the parameter type is not Real.
 */
Real GroundTrack::SetRealParameter(const Integer id, const Real value)
{
   return Subscriber::SetRealParameter(id, value);
}

//------------------------------------------------------------------------------
// Real GetRealParameter(const std::string &label) const
//------------------------------------------------------------------------------
/**
 * Retrieve the value for a Real parameter.
 *
 * @param label The scripting for the parameter.
 *
 * @return The parameter's value.
 */
Real GroundTrack::GetRealParameter(const std::string &label) const
{
   return GetRealParameter(GetParameterID(label));
}

//------------------------------------------------------------------------------
// Real SetRealParameter(const std::string &label, const Real value)
//------------------------------------------------------------------------------
/**
 * Set the value for a Real parameter.
 *
 * @param label The scripting for the parameter.
 * @param value The new parameter value.
 *
 * @return the parameter value at the end of this call, or throw an exception
 *         if the parameter id is invalid or the parameter type is not Real.
 */
Real GroundTrack::SetRealParameter(const std::string &label, const Real value)
{

   return SetRealParameter(GetParameterID(label), value);
}

//------------------------------------------------------------------------------
// Integer GetIntegerParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
* Retrieve the value for an Integer parameter.
 *
 * @param id The integer ID for the parameter.
 *
 * @return The parameter's value.
 */
Integer GroundTrack::GetIntegerParameter(const Integer id) const
{
   if (id == INTEGRATION_STEPS_TO_SKIP) return integrationStepsToSkip;
   if (id == LINE_WIDTH)
      return lineWidth;

   if (id == DATA_COLLECT_FREQUENCY)
      return collectFrequency;

   if (id == UPDATE_PLOT_FREQUENCY)
      return updateFrequency;

   if (id == NUM_POINTS_TO_REDRAW)
      return redrawCount;

   if (id == MAX_DATA)
      return maxData;

   if (id == LATLINE_COUNT)
      return latLineCount;

   if (id == LONGLINE_COUNT)
      return longLineCount;

   return Subscriber::GetIntegerParameter(id);
}

//------------------------------------------------------------------------------
// Integer SetIntegerParameter(const Integer id, const Integer value)
//------------------------------------------------------------------------------
/**
* Set the value for an Integer parameter.
 *
 * @param id The integer ID for the parameter.
 * @param value The new parameter value.
 *
 * @return the parameter value at the end of this call, or
 *         INTEGER_PARAMETER_UNDEFINED if the parameter id is invalid or the
 *         parameter type is not an Integer.
 */
Integer GroundTrack::SetIntegerParameter(const Integer id, const Integer value)
{
   if (id == INTEGRATION_STEPS_TO_SKIP)
   {
      if (value < -1) throw SubscriberException("IntegrationStepsToSkip must be -1 (publication sampling) or nonnegative");
      integrationStepsToSkip = value;
      integrationSampler.Reset();
      previousSample = BoundarySample{};
      if (isInitialized) Publisher::SetIntegrationStepConsumer(this, value >= 0);
      return value;
   }
   if (id == LINE_WIDTH)
   {
      if (value > 0)
         lineWidth = value;
      return lineWidth;
   }

   if (id == DATA_COLLECT_FREQUENCY)
   {
      if (value <= 0)
         throw SubscriberException("DataCollectFrequency must be a positive integer for " + instanceName);
      previousSample = BoundarySample{};
      collectFrequency = value;
      samplesUntilCollect = 0;
      return collectFrequency;
   }

   if (id == UPDATE_PLOT_FREQUENCY)
   {
      if (value <= 0)
         throw SubscriberException("UpdatePlotFrequency must be a positive integer for " + instanceName);
      updateFrequency = value;
      return updateFrequency;
   }

   if (id == NUM_POINTS_TO_REDRAW)
   {
      if (value < 0)
         throw SubscriberException("NumPointsToRedraw must be a nonnegative integer for " + instanceName);
      redrawCount = value;
      return redrawCount;
   }

   if (id == MAX_DATA)
   {
      MessageInterface::ShowMessage("Note: The GroundTrack component does not "
            "have a data size limitation.\n");
      if (value > 0)
         maxData = value;
      return maxData;
   }

   if (id == LATLINE_COUNT)
   {
      latLineCount = value;
      return latLineCount;
   }

   if (id == LONGLINE_COUNT)
   {
      longLineCount = value;
      return longLineCount;
   }

   return Subscriber::SetIntegerParameter(id, value);
}

//------------------------------------------------------------------------------
// Integer GetIntegerParameter(const std::string &label) const
//------------------------------------------------------------------------------
/**
* Retrieve the value for an Integer parameter.
 *
 * @param label The scripting for the parameter.
 *
 * @return The parameter's value.
 */
Integer GroundTrack::GetIntegerParameter(const std::string &label) const
{
   return GetIntegerParameter(GetParameterID(label));
}

//------------------------------------------------------------------------------
// Integer SetIntegerParameter(const std::string &label, const Integer value)
//------------------------------------------------------------------------------
/**
* Set the value for an Integer parameter.
 *
 * @param label The scripting for the parameter.
 * @param value The new parameter value.
 *
 * @return the parameter value at the end of this call, or
 *         INTEGER_PARAMETER_UNDEFINED if the parameter id is invalid or the
 *         parameter type is not an Integer.
 */
Integer GroundTrack::SetIntegerParameter(const std::string &label, const Integer value)
{
   return SetIntegerParameter(GetParameterID(label), value);
}

//------------------------------------------------------------------------------
// bool GetBooleanParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Retrieve a boolean parameter.
 *
 * @param  id  The integer ID for the parameter.
 *
 * @return the boolean value for this parameter, or throw an exception if the
 *         parameter access in invalid.
 */
bool GroundTrack::GetBooleanParameter(const Integer id) const
{
   if (id == SHOW_PLOT)
      return showPlot;

   if (id == USE_GRID)
      return useGrid;

   return Subscriber::GetBooleanParameter(id);
}

//------------------------------------------------------------------------------
// bool GetBooleanParameter(const std::string &label) const
//------------------------------------------------------------------------------
/**
 * Retrieve a boolean parameter.
 *
 * @param  label  The scripting for the parameter.
 *
 * @return the boolean value for this parameter, or throw an exception if the
 *         parameter access in invalid.
 */
bool GroundTrack::GetBooleanParameter(const std::string &label) const
{
   return GetBooleanParameter(GetParameterID(label));
}

//------------------------------------------------------------------------------
// bool SetBooleanParameter(const Integer id, const bool value)
//------------------------------------------------------------------------------
/**
 * Sets the value for a boolean parameter.
 *
 * @param id The integer ID for the parameter.
 * @param value The new value.
 *
 * @return the boolean value for this parameter, or throw an exception if the
 *         parameter is invalid or not boolean.
 */
bool GroundTrack::SetBooleanParameter(const Integer id, const bool value)
{
   if (id == SHOW_PLOT)
   {
      showPlot = value;
      return showPlot;
   }

   if (id == USE_GRID)
   {
      useGrid = value;
      return useGrid;
   }

   return Subscriber::SetBooleanParameter(id, value);
}

//------------------------------------------------------------------------------
// bool SetBooleanParameter(const std::string &label, const bool value)
//------------------------------------------------------------------------------
/**
 * Sets the value for a boolean parameter.
 *
 * @param label The scripting for the parameter.
 * @param value The new value.
 *
 * @return the boolean value for this parameter, or throw an exception if the
 *         parameter is invalid or not boolean.
 */
bool GroundTrack::SetBooleanParameter(const std::string &label, const bool value)
{
   return SetBooleanParameter(GetParameterID(label), value);
}

//------------------------------------------------------------------------------
// bool GetBooleanParameter(const Integer id, const Integer index) const
//------------------------------------------------------------------------------
/**
 * Retrieve a boolean parameter.
 *
 * @param id The integer ID for the parameter.
 * @param index Index for parameters in arrays.  Use -1 or the index free
 *              version to add the value to the end of the array.
 *
 * @return the boolean value for this parameter, or false if the parameter is
 *         not boolean.
 */
bool GroundTrack::GetBooleanParameter(const Integer id, const Integer index) const
{
   if (id == SHOWNAME)
      if (index < showName.size())
         return showName[index];

   // Note:  Subscriber lacks override
   return GmatBase::GetBooleanParameter(id, index);
}

//------------------------------------------------------------------------------
// bool SetBooleanParameter(const Integer id, const bool value,
//       const Integer index)
//------------------------------------------------------------------------------
/**
 * Sets the value for a boolean parameter.
 *
 * @param id The integer ID for the parameter.
 * @param value The new value for the parameter.
 * @param index Index for parameters in arrays.  Use -1 or the index free
 *              version to add the value to the end of the array.
 *
 * @return the boolean value for this parameter, or false if the parameter is
 *         not boolean.
 */
bool GroundTrack::SetBooleanParameter(const Integer id, const bool value,
      const Integer index)
{
   if (id == SHOWNAME)
      if (index < showName.size())
      {
         showName[index] = value;
         return showName[index];
      }

   // Note:  Subscriber lacks override
   return GmatBase::SetBooleanParameter(id, value, index);
}

//------------------------------------------------------------------------------
// bool GetBooleanParameter(const std::string &label,
//       const Integer index) const
//------------------------------------------------------------------------------
/**
 * Retrieves the value for a boolean parameter.
 *
 * @param label The scripting for the parameter.
 * @param index Index for parameters in arrays.  Use -1 or the index free
 *              version to add the value to the end of the array.
 *
 * @return the boolean value for this parameter, or false if the parameter is
 *         not boolean.
 */
bool GroundTrack::GetBooleanParameter(const std::string &label,
      const Integer index) const
{
   return GetBooleanParameter(GetParameterID(label), index);
}

//------------------------------------------------------------------------------
// bool SetBooleanParameter(const std::string &label,
//       const bool value, const Integer index)
//------------------------------------------------------------------------------
/**
 * Sets the value for a boolean parameter.
 *
 * @param label The scripting for the parameter.
 * @param value The new value for the parameter.
 * @param index Index for parameters in arrays.  Use -1 or the index free
 *              version to add the value to the end of the array.
 *
 * @return the boolean value for this parameter, or false if the parameter is
 *         not boolean.
 */
bool GroundTrack::SetBooleanParameter(const std::string &label,
      const bool value, const Integer index)
{
   return SetBooleanParameter(GetParameterID(label), value, index);
}

//------------------------------------------------------------------------------
// const BooleanArray& GetBooleanArrayParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Retrieve the value for a BooleanArray parameter.
 *
 * @param id The integer ID for the parameter.
 *
 * @return The parameter's value.
 */
const BooleanArray& GroundTrack::GetBooleanArrayParameter(const Integer id) const
{
   if (id == SHOWNAME)
      return showName;

   return Subscriber::GetBooleanArrayParameter(id);
}

//------------------------------------------------------------------------------
// const BooleanArray& GetBooleanArrayParameter(const std::string &label) const
//------------------------------------------------------------------------------
/**
 * Retrieve the value for a BooleanArray parameter.
 *
 * @param label The string label for the parameter.
 *
 * @return The parameter's value.
 */
const BooleanArray& GroundTrack::GetBooleanArrayParameter(const std::string &label) const
{
   return GetBooleanArrayParameter(GetParameterID(label));
}

//------------------------------------------------------------------------------
// bool SetBooleanArrayParameter(const Integer id,
//       const BooleanArray &valueArray)
//------------------------------------------------------------------------------
/**
 * Sets the value for a BooleanArray parameter.
 *
 * @param id The integer ID for the parameter.
 * @param valueArray The string of new value list enclosed with brackets,
 *                  such as [true false]
 *
 * @return true if value set successfully, or throw an exception if the
 *         parameter is invalid or not boolean.
 */
bool GroundTrack::SetBooleanArrayParameter(const Integer id,
      const BooleanArray &valueArray)
{
   if (id == SHOWNAME)
   {
      showName = valueArray;
      return true;
   }

   return Subscriber::SetBooleanArrayParameter(id, valueArray);
}

//------------------------------------------------------------------------------
// bool SetBooleanArrayParameter(const std::string &label,
//       const BooleanArray &valueArray)
//------------------------------------------------------------------------------
/**
 * Sets the value for a BooleanArray parameter.
 *
 * @param label The string label for the parameter.
 * @param valueArray The string of new value list enclosed with brackets,
 *                  such as [true false]
 *
 * @return true if value set successfully, or throw an exception if the
 *         parameter is invalid or not boolean.
 */
bool GroundTrack::SetBooleanArrayParameter(const std::string &label,
      const BooleanArray &valueArray)
{
   return SetBooleanArrayParameter(GetParameterID(label), valueArray);
}


// Reference object methods
//------------------------------------------------------------------------------
// bool HasRefObjectTypeArray()
//------------------------------------------------------------------------------
/**
 * Returns flag indicating whether GetRefObjectTypeArray() is implemented or not.
 *
 * @retval true: Ground track uses a ref object type array
 */
bool GroundTrack::HasRefObjectTypeArray()
{
   return true;
}

//------------------------------------------------------------------------------
// const ObjectTypeArray& GetRefObjectTypeArray()
//------------------------------------------------------------------------------
/**
 * Returns the types of the reference object.
 *
 * @return The types of the reference object.
 */
const ObjectTypeArray& GroundTrack::GetRefObjectTypeArray()
{
   refObjectTypes.clear();

   refObjectTypes.push_back(Gmat::CELESTIAL_BODY);
   refObjectTypes.push_back(Gmat::SPACECRAFT);
   refObjectTypes.push_back(Gmat::SPACE_POINT);
   refObjectTypes.push_back(Gmat::GROUND_STATION);

   #ifdef DEBUG_INITIALIZATION
      MessageInterface::ShowMessage("Identifying %d ref object types\n",
            refObjectTypes.size());
   #endif

   return refObjectTypes;
}

//------------------------------------------------------------------------------
// std::string GetRefObjectName(const UnsignedInt type) const
//------------------------------------------------------------------------------
/**
 * Returns the name of the reference object.
 *
 * @param <type> reference object type.
 *
 * @return The name of the reference object.
 */
std::string GroundTrack::GetRefObjectName(const UnsignedInt type) const
{
   return Subscriber::GetRefObjectName (type);
}

//------------------------------------------------------------------------------
// const StringArray& GetRefObjectNameArray(const UnsignedInt type)
//------------------------------------------------------------------------------
/**
 * Returns the names of the reference object.
 *
 * @param <type> reference object type.
 *
 * @return The names of the reference object.
 */
const StringArray& GroundTrack::GetRefObjectNameArray(const UnsignedInt type)
{
   refObjectNames.clear();
   refObjectNames = Subscriber::GetRefObjectNameArray(type);

   if (type == Gmat::UNKNOWN_OBJECT || (type == Gmat::SPACE_POINT) ||
       type == Gmat::CELESTIAL_BODY)
   {
      refObjectNames.push_back(theBodyName);
   }

   if ((type == Gmat::SPACE_POINT) || (type == Gmat::UNKNOWN_OBJECT))
      refObjectNames.insert(refObjectNames.end(), theObjects.begin(),
            theObjects.end());

   return refObjectNames;
}



//------------------------------------------------------------------------------
// bool SetRefObjectName(const UnsignedInt type,
//       const std::string &name)
//------------------------------------------------------------------------------
/**
 * Sets the name of the reference object.
 *
 * @param <type> type of the reference object.
 * @param <name> name of the reference object.
 *
 * @return success of the operation.
 */
bool GroundTrack::SetRefObjectName(const UnsignedInt type,
                                   const std::string &name)
{
   return Subscriber::SetRefObjectName(type, name);
}

//------------------------------------------------------------------------------
// bool SetRefObject(GmatBase *obj, const UnsignedInt type,
//       const std::string &name)
//------------------------------------------------------------------------------
/**
 * Sets the reference object.
 *
 * @param <obj> reference object pointer.
 * @param <type> type of the reference object.
 * @param <name> name of the reference object.
 *
 * @return success of the operation.
 */
bool GroundTrack::SetRefObject(GmatBase *obj, const UnsignedInt type,
                                  const std::string &name)
{
   if (obj->IsOfType(Gmat::CELESTIAL_BODY))
   {
      if (name == theBodyName)
      {
         theBody = (CelestialBody*)obj;
//         mapFile = theBody->GetStringParameter(theBody->GetParameterID("TextureMapFullPath"));
      }
      else
      {
         for (auto &reference : theBodies)
         {
            if (reference->GetName() == obj->GetName())
            {
               reference = (SpacePoint*)obj;
               return true;
            }
         }
         theBodies.push_back((SpacePoint*)obj);
      }
      return true;
   }
   else if (obj->IsOfType(Gmat::SPACECRAFT))
   {
      for (auto &reference : theSats)
      {
         if (reference->GetName() == obj->GetName())
         {
            reference = (Spacecraft*)obj;
            return true;
         }
      }
      theSats.push_back((Spacecraft*)obj);
      return true;
   }
   else if (obj->IsOfType(Gmat::GROUND_STATION))
   {
      for (auto &reference : theStations)
      {
         if (reference->GetName() == obj->GetName())
         {
            reference = (GroundstationInterface*)obj;
            return true;
         }
      }
      theStations.push_back((GroundstationInterface*)obj);
      return true;
   }

   return Subscriber::SetRefObject(obj, type, name);
}

//------------------------------------------------------------------------------
// bool SetRefObject(GmatBase *obj, const UnsignedInt type,
//       const std::string &name, const Integer index)
//------------------------------------------------------------------------------
/**
 * Sets the reference object.
 *
 * @param obj reference object pointer.
 * @param type type of the reference object.
 * @param name name of the reference object.
 * @param index Index into the object array.
 *
 * @return success of the operation.
 */
bool GroundTrack::SetRefObject(GmatBase *obj, const UnsignedInt type,
                                  const std::string &name,
                                  const Integer index)
{
   return Subscriber::SetRefObject(obj, type, name, index);
}

//------------------------------------------------------------------------------
// bool RenameRefObject(const UnsignedInt type,
//       const std::string &oldName, const std::string &newName)
//------------------------------------------------------------------------------
/**
 * Interface used to support user actions.
 *
 * @param <type> reference object type.
 * @param <oldName> object name to be renamed.
 * @param <newName> new object name.
 *
 * @return true if object name changed, false if not.
 */
bool GroundTrack::RenameRefObject(const UnsignedInt type,
                                     const std::string &oldName,
                                     const std::string &newName)
{
   MessageInterface::ShowMessage("Renaming %s to %s\n", oldName.c_str(), 
         newName.c_str());
   
   if (type == Gmat::SPACECRAFT || type == Gmat::GROUND_STATION ||
       type == Gmat::CALCULATED_POINT)
   {
      for (UnsignedInt i = 0; i < theObjects.size(); ++i)
      {
         if (theObjects[i] == oldName)
            theObjects[i] = newName;
      }
   }

   if (type == Gmat::CELESTIAL_BODY)
   {
      if (theBodyName == oldName)
         theBodyName = newName;
   }
   
   return Subscriber::RenameRefObject(type, oldName, newName);
}

//------------------------------------------------------------------------------
// void SetSolarSystem(SolarSystem *ss)
//------------------------------------------------------------------------------
/**
 * Sets the internal solar system pointer for objects that have one.
 *
 * @note Derived classes that need the solar system must override this method.
 */
void GroundTrack::SetSolarSystem(SolarSystem *ss)
{
   theSS = ss;
}

//------------------------------------------------------------------------------
// void SetDataLabels(const StringArray& elements)
//------------------------------------------------------------------------------
/**
 * Sets the labels for data elements
 *
 * @param elements The labels
 */
void GroundTrack::SetDataLabels(const StringArray& elements)
{
   #ifdef DEBUG_STATE_ORDER
      for (unsigned int i = 0; i < elements.size(); ++i)
         MessageInterface::ShowMessage("   %3d:  %s\n", i, elements[i].c_str());
   #endif

   stateMap.clear();
   stateIndices.assign(theSats.size(), IntegerArray(6, -1));
   const char *components[] = {"X", "Y", "Z", "Vx", "Vy", "Vz"};
   const char *upperComponents[] = {"X", "Y", "Z", "VX", "VY", "VZ"};
   for (unsigned int i = 0; i < theSats.size(); ++i)
   {
      const std::string prefix = theSats[i]->GetName() + ".";
      for (int component = 0; component < 6; ++component)
      {
         for (unsigned int j = 0; j < elements.size(); ++j)
         {
            if (elements[j] == prefix + components[component] ||
                elements[j] == prefix + upperComponents[component])
            {
               stateIndices[i][component] = j;
               break;
            }
         }
      }
      if (stateIndices[i][0] >= 0)
         stateMap[theSats[i]] = stateIndices[i][0];
   }

   #ifdef DEBUG_STATE_ORDER
      for (std::map<Spacecraft*, int>::iterator i = stateMap.begin();
            i != stateMap.end(); ++i)
         MessageInterface::ShowMessage("   %s: %i\n",
               i->first->GetName().c_str(), i->second);
   #endif

   Subscriber::SetDataLabels(elements);

   std::string theAction = "";
   std::string theColors = "";
   for (int i = 0; i < theSats.size(); i++)
   {
      if (theAction != "")
      {
         theAction = theAction + "|";
         theColors = theColors + "|";
      }
      theAction = theAction + theSats[i]->GetName();
      theColors = theColors + theSats[i]->GetOrbitColorString();
   }
   theAction = "Satellites=" + theAction;
   PlotInterface::TakeGroundTrackAction(instanceName, theAction);

   #ifdef DEBUG_GROUNDTRACK_ACTION
      MessageInterface::ShowMessage("Setting colors next\n");
   #endif

   theAction = "SatColors=" + theColors;
   PlotInterface::TakeGroundTrackAction(instanceName, theAction);
}

//------------------------------------------------------------------------------
// Segment colors are display metadata; receivers may handle this optional
// action without changing state publication or geodetic calculations.
void GroundTrack::SetSegmentOrbitColor(GmatBase *, bool overrideColor,
                                      UnsignedInt orbitColor, const StringArray &objNames)
{
   std::ostringstream action;
   action << "SegmentColor=" << (overrideColor ? "On" : "Off") << '|' << orbitColor;
   for (const auto &name : objNames) action << '|' << name;
   PlotInterface::TakeGroundTrackAction(instanceName, action.str());
}

// In a decimated regular trajectory a Propagate command echoes its initial
// state even when the preceding command just delivered that very sample.
// Only that explicit command-start echo is ineligible for the countdown.
// Ordinary publications and solver trial packets retain their original phase.
bool GroundTrack::RepeatsPropagationStart(const Real *data, Integer count)
{
   bool initial = false;
   const auto *roots = Publisher::GetPublishedPropagationObjects(data, count, initial);
   if (!roots || roots->empty() || runstate != Gmat::RUNNING || theDataLabels.empty())
   {
      previousSample = BoundarySample{};
      return false;
   }
   // Equal numeric packets from other runtime spacecraft are distinct. Include
   // formation members and each object's clock, even for multi-provider packets.
   ObjectArray objects;
   for (auto *root : *roots)
   {
      objects.push_back(root);
      if (root && root->IsOfType(Gmat::FORMATION))
      {
         const auto &members = root->GetRefObjectArray(Gmat::SPACECRAFT);
         objects.insert(objects.end(), members.begin(), members.end());
      }
   }
   std::vector<GmatTime> epochs;
   epochs.reserve(objects.size());
   for (auto *object : objects)
   {
      auto *space = dynamic_cast<SpaceObject*>(object);
      if (!space)
      {
         previousSample = BoundarySample{};
         return false;
      }
      epochs.push_back(space->HasPrecisionTime() ? space->GetEpochGT() : GmatTime(space->GetEpoch()));
   }
   const bool repeated = initial && objects == previousSample.objects &&
      epochs == previousSample.epochs && propDirection == previousSample.direction &&
      theDataLabels[0] == previousSample.labels && previousSample.packet.size() == size_t(count) &&
      std::memcmp(previousSample.packet.data(), data, size_t(count) * sizeof(Real)) == 0;
   previousSample.objects = std::move(objects);
   previousSample.epochs = std::move(epochs);
   previousSample.labels = theDataLabels[0];
   previousSample.packet.assign(data, data + count);
   previousSample.direction = propDirection;
   return repeated;
}

//------------------------------------------------------------------------------
// bool Distribute(const double * dat, Integer len)
//------------------------------------------------------------------------------
/**
 * Pass data to the widget for display
 *
 * For now, override the base class method so that we aren't showing all
 * SpacePoints.
 *
 * @param dat The data
 * @param len The number of data points in dat
 *
 * @return true if the data was handled
 */
bool GroundTrack::Distribute(const double * dat, Integer len)
{
   bool retval = false;

   if (!active || !showPlot)
      return true;

   #ifdef DEBUG_GROUNDTRACK_PUBLISH
      MessageInterface::ShowMessage("Distributing ground track data\n");
   #endif

   if (isEndOfReceive)
   {
      // FlushData also ends individual propagation/solver blocks. Only the
      // engine's SetEndOfRun notification completes the retained display.
      PlotInterface::TakeGroundTrackAction(instanceName,
            isEndOfRun ? "RunComplete" :
            integrationStepsToSkip >= 0 ? "RefreshPreservingCadence" : "Refresh");

      // if targetting and draw target is None, just return
      if (mSolverIterations == "None" &&
          ((runstate == Gmat::TARGETING) || (runstate == Gmat::OPTIMIZING) ||
                (runstate == Gmat::SOLVING)))
      {
         return true;
      }

      if (active)
      {
         return true;
      }
   }

   if (len <= 0 || dat == NULL)
      return true;

   // Solver iterations are display data, not part of the accepted trajectory.
   // Match the OrbitPlot/XY subscriber's None filtering before sampling.
   const bool solving = runstate == Gmat::TARGETING ||
         runstate == Gmat::OPTIMIZING || runstate == Gmat::SOLVING;
   if (mSolverIterations == "None" && solving)
      return true;

   if (integrationStepsToSkip >= 0)
   {
      const auto *steps = Publisher::GetPublishedIntegrationSteps(dat, len);
      if (!steps) return true; // Nonintegrating providers have no step identity.
      const StringArray originalLabels = theDataLabels.empty() ? StringArray{} : theDataLabels[0];
      std::map<const GmatBase*,int> packetSpacecraft;
      for (const auto &record : *steps)
         for (const auto &sample : record.states)
            packetSpacecraft[sample.object] = 3; // Present in this packet, not necessarily this step.
      try
      {
         for (const auto &step : *steps)
         {
            StringArray labels(1, "Epoch");
            RealArray values(1, step.epoch.GetMjd());
            const char *components[] = {".X", ".Y", ".Z", ".Vx", ".Vy", ".Vz"};
            auto selection = packetSpacecraft;
            for (const auto &sample : step.states)
            {
               bool newArc = false;
               if (std::find(theSats.begin(), theSats.end(), sample.object) == theSats.end()) continue;
               if (!integrationSampler.Select(step, sample, integrationStepsToSkip, &newArc))
               {
                  selection[sample.object] = 3; // Present but intentionally uncollected.
                  continue;
               }
               selection[sample.object] = newArc ? 2 : 1;
               for (Integer c = 0; c < 6; ++c)
               {
                  labels.push_back(sample.object->GetName() + components[c]);
                  values.push_back(sample.state[c]);
               }
            }
            if (values.size() == 1) continue;
            SetDataLabels(labels);
            std::ostringstream mask;
            mask.imbue(std::locale::classic());
            mask << "PlotStepAvailability:" << std::setprecision(17) << values[0] << ' ' << theSats.size();
            for (const auto *sat : theSats)
               mask << ' ' << std::quoted(sat->GetName()) << ' ' << selection[sat];
            PlotInterface::TakeGroundTrackAction(instanceName, mask.str());
            if (!PlotSample(values.data(), values.size(), solving))
            { PlotInterface::TakeGroundTrackAction(instanceName, "ClearStepAvailability"); SetDataLabels(originalLabels); return false; }
         }
      }
      catch (...) { PlotInterface::TakeGroundTrackAction(instanceName, "ClearStepAvailability"); SetDataLabels(originalLabels); throw; }
      SetDataLabels(originalLabels);
      return true;
   }

   if (collectFrequency > 1 && RepeatsPropagationStart(dat, len)) return true;

   // Keep the initial point and then every collectFrequency-th eligible publication.
   // A bounded countdown avoids overflow during long missions.
   if (samplesUntilCollect > 0)
   {
      --samplesUntilCollect;
      return true;
   }
   samplesUntilCollect = collectFrequency - 1;
   return PlotSample(dat, len, solving);
}

bool GroundTrack::PlotSample(const Real *dat, Integer len, bool solving)
{
   bool retval = false;
   // A publication may contain only a subset of the tracked spacecraft.
   // Preserve curve slots and explicitly mark absent states for the receiver.
   const double absent = std::numeric_limits<double>::quiet_NaN();
   bool hasPosition = false;
   for (unsigned int i = 0; i < theSats.size(); ++i)
   {
      longlat[2*i] = longlat[2*i + 1] = absent;
      double state[6], bodyState[6];
      bool complete = i < stateIndices.size();
      for (int component = 0; component < 6 && complete; ++component)
      {
         const int index = stateIndices[i][component];
         complete = index > 0 && index < len;
         if (complete)
         {
            state[component] = dat[index];
            // The finite SPICE unavailable marker must not become a
            // plausible latitude/longitude after coordinate conversion.
            complete = std::isfinite(state[component]) &&
                  std::abs(state[component])<(std::numeric_limits<double>::max)();
         }
      }
      if (!complete || !std::isfinite(dat[0]))
         continue;

      ccvt.Convert(dat[0], state, inertialSystem, bodyState, viewCoordinates);
      CalculateLatLong(bodyState, longlat[2*i], longlat[2*i + 1]);
      hasPosition = true;
   }
   if (!hasPosition)
   {
      // Notify the receiver of absent slots even when every spacecraft is
      // unavailable, so the next valid sample cannot bridge this interval.
      return PlotInterface::UpdateGroundTrackData(instanceName,
            dat[0], longlat, theSats.size());
   }

   // Colors belong to the live sandbox objects, not the configuration copies.
   // Include them as display metadata so runtime assignments and late-opened
   // views use the current default/solver palettes. Segment overrides remain
   // a separate callback and do not modify spacecraft color properties.
   std::ostringstream colors;
   colors << "SpacecraftColors=";
   for (unsigned int i = 0; i < theSats.size(); ++i)
   {
      if (i > 0) colors << '|';
      colors << theSats[i]->GetName() << '|' << theSats[i]->GetCurrentOrbitColor()
             << '|' << theSats[i]->GetCurrentTargetColor();
   }
   PlotInterface::TakeGroundTrackAction(instanceName, colors.str());

   // The ground-track callback has no run-state argument. Supply it as an
   // optional action for receivers that distinguish trial and accepted arcs.
   PlotInterface::TakeGroundTrackAction(instanceName,
         solving ? "SolverData=On" : "SolverData=Off");
   retval = PlotInterface::UpdateGroundTrackData(instanceName,
         dat[0], longlat, theSats.size());

   return retval;
}

//------------------------------------------------------------------------------
// virtual bool TakeAction(const std::string &action,
//                         const std::string &actionData = "");
//------------------------------------------------------------------------------
/**
 * This method performs action.
 *
 * @param action action to perform
 * @param actionData action data associated with action
 * @return true if action successfully performed
 *
 */
//------------------------------------------------------------------------------
bool GroundTrack::TakeAction(const std::string &action,
                                 const std::string &actionData)
{
   integrationSampler.Action(action, actionData);
   if (action == "ToggleOn" || action == "ToggleOff" || action == "MarkBreak" ||
       action == "ClearFromBreak" || action == "Clear") previousSample = BoundarySample{};
   if (action == "Clear")
   {
      theObjects.clear();
      showName.clear();
      theSats.clear();
      theBodies.clear();
      thePoints.clear();
      theStations.clear();
      stateMap.clear();
      return true;
   }
   if (action == "ToggleOn" || action == "ToggleOff")
   {
      // ReceiveData skips inactive subscribers, so the receiver cannot infer
      // the disabled interval from samples. Keep retained arcs separated when
      // Toggle resumes without changing the sampled states or coordinates.
      PlotInterface::TakeGroundTrackAction(instanceName, action);
      return true;
   }
   if (action == "PenUp")
   {
      PlotInterface::TakeGroundTrackAction(instanceName, "PenUp");
      return true;
   }
   if (action == "PenDown")
   {
      PlotInterface::TakeGroundTrackAction(instanceName, "PenDown");
      return true;
   }
   if (action == "SolverScope")
   {
      PlotInterface::TakeGroundTrackAction(instanceName, "SolverScope=" + actionData);
      return true;
   }
   if (action == "MarkBreak" || action == "ClearFromBreak")
   {
      if (mSolverIterations == "Current")
         PlotInterface::TakeGroundTrackAction(instanceName,
               action == "MarkBreak" ? action : action + "=" + actionData);
      return true;
   }

   return Subscriber::TakeAction(action, actionData);
}

//------------------------------------------------------------------------------
// void CalculateLatLong(const double *state, double &longitude, double &latitude)
//------------------------------------------------------------------------------
/**
 * Computes latitude and longitude.  Reworked from GMAT's CalculationUtil code.
 *
 * @param state     State in the planet's body fixed coordinate system
 * @param longitude Longitude output parameter
 * @param latitude  Latitude output parameter
 */
void GroundTrack::CalculateLatLong(const double *state, double &longitude,
      double &latitude)
{
   double cFactor, oldlat, sinlat;

   // The input state is in the origin-centered BodyFixed Coordinate System
   longitude = atan2(state[1], state[0]) * GmatMathConstants::DEG_PER_RAD;
   longitude = AngleUtil::PutAngleInDegRange(longitude, -180.0, 180.0);

   // Planetodetic latitude, Vallado algorithm 12 (Vallado, 2nd ed, p. 177)
   double rxy = sqrt(state[0]*state[0] + state[1]*state[1]);
   latitude = atan2(state[2], rxy);

   double delta = 1.0;
   double geodeticTolerance = 1.0e-7;    // Better than 0.0001 degrees
   double ecc2 = 2.0 * bodyFlattening - bodyFlattening * bodyFlattening;

   while (delta > geodeticTolerance)
   {
      oldlat   = latitude;
      sinlat   = sin(oldlat);
      cFactor  = bodyRadius / sqrt(1.0 - ecc2 * sinlat * sinlat);
      latitude = atan2(state[2] + cFactor*ecc2*sinlat, rxy);
      delta    = fabs(latitude - oldlat);
   }

   latitude = latitude * GmatMathConstants::DEG_PER_RAD;
   latitude = AngleUtil::PutAngleInDegRange(latitude, -90.0, 90.0);

//   // Altitude
//   sinlat = sin(latitude);
//   cFactor = originRadius / sqrt(1.0 - ecc2 * sinlat * sinlat);
//   double altitude =  rxy / cos(latitude) - cFactor;
}
