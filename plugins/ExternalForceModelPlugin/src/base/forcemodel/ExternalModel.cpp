//$Id$
//------------------------------------------------------------------------------
//                              ExternalModel
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
// Developed jointly by NASA/GSFC and Emergent Space Technologies, Inc. 
//
// Author: Alex Campbell
// Created: 07/25/2022
///

#include "ExternalModel.hpp"
#include "GmatBaseException.hpp"
#include <typeinfo>
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include "OrbitStmForceGradient.hpp"
#include "Spacecraft.hpp"
#include <sstream>                      // For stringstream
#include "MessageInterface.hpp"
#include "GmatConstants.hpp"
#include "GmatDefaults.hpp"
#include "PropagationStateManager.hpp"
#include "InterfaceException.hpp"
#include "FileManager.hpp"
#include "StringUtil.hpp"
#include "ObjectScriptExportScope.hpp"
#include "ScriptStringLiteral.hpp"

//#define DEBUG_INITIALIZATION
//#define DEBUG_REGISTRATION


//---------------------------------
// static data
//---------------------------------
const std::string
ExternalModel::PARAMETER_TEXT[ExternalParamCount - PhysicalModelParamCount] =
{
    "ScriptFileName",
    "ExcludeOtherForces",
    "DerivativesFunction", 
};

const Gmat::ParameterType
ExternalModel::PARAMETER_TYPE[ExternalParamCount - PhysicalModelParamCount] =
{
   Gmat::FILENAME_TYPE,  
   Gmat::BOOLEAN_TYPE,
   Gmat::STRING_TYPE,  
};
           
//---------------------------------
// public
//---------------------------------

//------------------------------------------------------------------------------
// ExternalModel(const std::string &name)
//------------------------------------------------------------------------------
/**
 * Constructor
 * 
 * @param name Name of the force object
 */
//------------------------------------------------------------------------------
ExternalModel::ExternalModel(const std::string &name) :
   PhysicalModel       (Gmat::PHYSICAL_MODEL, "ExternalModel", name),   
   scriptFilename           (""),
   fullFilePath              (""),
   excludeForces            (false),
   entryPoint               ("GetDerivatives"),
   torqueEntryPoint         (""),
   pythonIf                 (NULL),
   satCount                 (0)
{
   parameterCount = ExternalParamCount;
   derivativeIds.push_back(Gmat::CARTESIAN_STATE);
   objectTypeNames.push_back("ExternalModel");
   objectTypeNames.push_back("External");

   isConservative = false;
   hasMassJacobian = true;
   initializedMessagePrinted = false;
}

//------------------------------------------------------------------------------
// ExternalModel(ExternalModel &srp)
//------------------------------------------------------------------------------
/**
 * Copy constructor
 * 
 * @param external The force copied to this one.
 */
//------------------------------------------------------------------------------
ExternalModel::ExternalModel(const ExternalModel &external) :
    PhysicalModel           (external),
	scriptFilename          (external.scriptFilename),
   fullFilePath            (external.fullFilePath),
	excludeForces           (external.excludeForces),
	entryPoint              (external.entryPoint),
	torqueEntryPoint        (external.torqueEntryPoint),
	pythonIf                (external.pythonIf),
	satCount                (external.satCount)
{
   parameterCount = ExternalParamCount;
   initializedMessagePrinted = external.initializedMessagePrinted;
   isInitialized = false;
}

//------------------------------------------------------------------------------
// ExternalModel& operator=(ExternalModel &srp)
//------------------------------------------------------------------------------
/**
 * Assignment operator
 * 
 * @param external The force passing values to this one
 *
 * @return This external force, set to match the external input model
 */
//------------------------------------------------------------------------------
ExternalModel& ExternalModel::operator=(const ExternalModel &external)
{
   if (this != &external)
   {
      const bool restoring = GmatBase::IsNativeRestorationCopy();
      if (restoring && (scriptFilename != external.scriptFilename || fullFilePath != external.fullFilePath ||
          entryPoint != external.entryPoint || torqueEntryPoint != external.torqueEntryPoint ||
          excludeForces != external.excludeForces || satCount != external.satCount ||
          (pythonIf && external.pythonIf && pythonIf != external.pythonIf)))
         throw ODEModelException("Prepare the external-force module before restoring changed entry points or bindings.");
      PhysicalModel::operator=(external);
      scriptFilename = external.scriptFilename;
      fullFilePath = external.fullFilePath;
      excludeForces = external.excludeForces;
      entryPoint = external.entryPoint;
	   torqueEntryPoint = external.torqueEntryPoint;
      if (!restoring) pythonIf = external.pythonIf;
      satCount = external.satCount;
      initializedMessagePrinted = external.initializedMessagePrinted;
      if (!restoring) isInitialized = false;
   }
   
   return *this;
}

//------------------------------------------------------------------------------
// ExternalModel::~ExternalModel(void)
//------------------------------------------------------------------------------
/**
 * Destructor
 */
//------------------------------------------------------------------------------
ExternalModel::~ExternalModel()
{ 
    
}

//------------------------------------------------------------------------------
//  GmatBase* Clone() const
//------------------------------------------------------------------------------
/**
 * This method returns a clone of the ExternalModel.
 *
 * @return clone of the ExternalModel.
 *
 */
//------------------------------------------------------------------------------
GmatBase* ExternalModel::Clone() const
{
   return (new ExternalModel(*this));
}

const std::string& ExternalModel::GetGeneratingString(Gmat::WriteMode mode,
      const std::string &prefix, const std::string &useName)
{
   // The owned model marks all fields read-only for ordinary serialization.
   // Preserve its configuration explicitly, creating the External contributor
   // before writing its dependent fields when a saved force model is rebuilt.
   const std::string suffix = "External.";
   if (mode == Gmat::OWNED_OBJECT && prefix.size() >= suffix.size() &&
       prefix.compare(prefix.size() - suffix.size(), suffix.size(), suffix) == 0)
   {
      const auto assignment = [this](Integer id, const std::string &lhs,
            const std::string &value)
      {
         return GetAttributeCommentLine(id) + lhs + " = " + value + ";" +
            GetInlineAttributeComment(id) + "\n";
      };
      generatingString = assignment(SCRIPT_FILENAME,
         prefix.substr(0, prefix.size() - suffix.size()) + "External",
         "'" + scriptFilename + "'");
      generatingString += assignment(ENTRY_POINT, prefix + "DerivativesFunction",
         "'" + entryPoint + "'");
      generatingString += assignment(EXCLUDE_OTHER_FORCES,
         prefix + "ExcludeOtherForces", excludeForces ? "true" : "false");
      return generatingString;
   }
   PhysicalModel::GetGeneratingString(mode, prefix, useName);
   // The ordinary flags keep these fields owned by ForceModel. Preserve them
   // when Save explicitly exports the named external contributor itself.
   if (ObjectScriptExportScope::IsActive() &&
         (mode == Gmat::SCRIPTING || mode == Gmat::SHOW_SCRIPT))
   {
      const auto object = (useName.empty() ? GetName() : useName) + ".";
      if (!scriptFilename.empty())
         generatingString += object + "ScriptFileName = " + GmatScriptString::Quote(scriptFilename) + ";\n";
      generatingString += object + "DerivativesFunction = " + GmatScriptString::Quote(entryPoint) + ";\n";
      generatingString += object + "ExcludeOtherForces = " + (excludeForces ? "true" : "false") + ";\n";
   }
   return generatingString;
}

//------------------------------------------------------------------------------
// std::string GetParameterText(const Integer id) const
//------------------------------------------------------------------------------
/**
 * @see GmatBase
 */
//------------------------------------------------------------------------------
std::string ExternalModel::GetParameterText(const Integer id) const
{
   if (id >= SCRIPT_FILENAME && id < ExternalParamCount)
      return ExternalModel::PARAMETER_TEXT[id - PhysicalModelParamCount];
   else
      return PhysicalModel::GetParameterText(id);
}

//------------------------------------------------------------------------------
// Integer GetParameterID(const std::string str) const
//------------------------------------------------------------------------------
/**
 * @see GmatBase
 */
//------------------------------------------------------------------------------
Integer ExternalModel::GetParameterID(const std::string &str) const
{
   for (int i = SCRIPT_FILENAME; i < ExternalParamCount; i++)
   {
      if (str == ExternalModel::PARAMETER_TEXT[i - PhysicalModelParamCount])
         return i;
   }
   return PhysicalModel::GetParameterID(str);
}

//------------------------------------------------------------------------------
// Gmat::ParameterType GetParameterType(const Integer id) const
//------------------------------------------------------------------------------
/**
 * @see GmatBase
 */
//------------------------------------------------------------------------------
Gmat::ParameterType ExternalModel::GetParameterType(const Integer id) const
{
   if (id >= SCRIPT_FILENAME && id < ExternalParamCount)
      return PARAMETER_TYPE[id - PhysicalModelParamCount];
   else
      return PhysicalModel::GetParameterType(id);
}

//------------------------------------------------------------------------------
// std::string GetParameterTypeString(const Integer id) const
//------------------------------------------------------------------------------
/**
 * @see GmatBase
 */
//------------------------------------------------------------------------------
std::string ExternalModel::GetParameterTypeString(const Integer id) const
{
   if (id >= SCRIPT_FILENAME && id < ExternalParamCount)
      return GmatBase::PARAM_TYPE_STRING[GetParameterType(id)];
    else
      return PhysicalModel::GetParameterTypeString(id);
}

//------------------------------------------------------------------------------
// bool IsParameterReadOnly(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Checks to see if the parameter is user settable
 *
 * @param id The parameter's ID
 *
 * @return true
 */
 //------------------------------------------------------------------------------
bool ExternalModel::IsParameterReadOnly(const Integer id) const
{     
   return true;
}

//------------------------------------------------------------------------------
// bool GetBooleanParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
 * @see GmatBase
 */
//------------------------------------------------------------------------------
bool ExternalModel::GetBooleanParameter(const Integer id) const
{
    if (id == EXCLUDE_OTHER_FORCES)
        return excludeForces;
   
    return PhysicalModel::GetBooleanParameter(id);
}

//------------------------------------------------------------------------------
// bool SetBooleanParameter(const Integer id, const bool value)
//------------------------------------------------------------------------------
/**
 * @see GmatBase
 */
//------------------------------------------------------------------------------
bool ExternalModel::SetBooleanParameter(const Integer id, const bool value)
{
   if (id == EXCLUDE_OTHER_FORCES)
   {
	  excludeForces = value;
      return excludeForces;
   }  
       
   return PhysicalModel::SetBooleanParameter(id, value);
}


//------------------------------------------------------------------------------
// std::string GetStringParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Accessor method used to obtain a parameter value
 *
 * @param id    Integer ID for the requested parameter
 */
 //------------------------------------------------------------------------------
std::string ExternalModel::GetStringParameter(const Integer id) const
{
	if (id == SCRIPT_FILENAME) return scriptFilename;
	else if (id == ENTRY_POINT) return entryPoint;
	return PhysicalModel::GetStringParameter(id);
}

//------------------------------------------------------------------------------
// bool SetStringParameter(const Integer id, const std::string value)
//------------------------------------------------------------------------------
/**
 * Accessor method used to set a parameter value
 *
 * @param    id  Integer ID for the parameter
 * @param    val The new value for the parameter
 */
 //------------------------------------------------------------------------------
bool ExternalModel::SetStringParameter(const Integer id,
	const std::string &value)
{
	if (id == SCRIPT_FILENAME)
	{
      std::string val = value;
      if (GmatStringUtil::IsEnclosedWith(val, "'"))
      {
         val = GmatStringUtil::RemoveEnclosingString(val, "'");
      }
		scriptFilename = val;

		return true;
	}
	if (id == ENTRY_POINT)
	{
      std::string val = value;
      if (GmatStringUtil::IsEnclosedWith(val, "'"))
      {
         val = GmatStringUtil::RemoveEnclosingString(val, "'");
      }
		entryPoint = val;
		return true;
	}
	return PhysicalModel::SetStringParameter(id, value);
}

//------------------------------------------------------------------------------
// std::string GetStringParameter(const std::string &label) const
//------------------------------------------------------------------------------
/**
 * Accessor method used to obtain a parameter value
 *
 * @param label    string ID for the requested parameter
 */
 //------------------------------------------------------------------------------
std::string ExternalModel::GetStringParameter(const std::string &label) const
{	
	return GetStringParameter(GetParameterID(label));
}

//------------------------------------------------------------------------------
// bool SetStringParameter(const std::string &label, const char *value)
//------------------------------------------------------------------------------
bool ExternalModel::SetStringParameter(const std::string &label,
                                                const char *value)
{
   return SetStringParameter(GetParameterID(label), std::string(value));
}

//------------------------------------------------------------------------------
// bool SetStringParameter(const std::string &label, const std::string &value)
//------------------------------------------------------------------------------
bool ExternalModel::SetStringParameter(const std::string &label,
                                                const std::string &value)
{
   return SetStringParameter(GetParameterID(label), value);
}


//------------------------------------------------------------------------------
// bool ExternalModel::Initialize()
//------------------------------------------------------------------------------
/**
 * Prepares the external model for use
 */
//------------------------------------------------------------------------------
bool ExternalModel::Initialize()
{
   bool retval = PhysicalModel::Initialize();  

   if (!retval)
      return false; 
      
   isInitialized = true;   
   try
   {
	   pythonIf = PythonInterface::PyInstance();
	   FileManager *fileMgr = FileManager::Instance();

#ifdef DEBUG_INITIALIZATION
	   MessageInterface::ShowMessage("  pythonIf:  %p\n", pythonIf);
#endif
	   //Initialize Python engine
	   pythonIf->PyInitialize();
	   // Get all Python module paths from the startup file
	   StringArray paths = fileMgr->GetAllPythonModulePaths();

#ifdef DEBUG_INITIALIZATION
	   MessageInterface::ShowMessage("  Adding %d python paths\n", paths.size());
#endif
	   pythonIf->PyAddModulePath(paths);
   }   
   catch (...)
   {
	   throw InterfaceException("An unhandled Python exception was thrown during "
		   "initialization");
   }  
   
  


   return isInitialized;
}

//------------------------------------------------------------------------------
// bool ExternalModel::GetDerivatives()(Real * state, Real dt, Integer order)
//------------------------------------------------------------------------------
/**
 * Converts the given parametrs to what is needed to be sent to the python script. 
 * Converts what is returned from the python script into what the parent ODEModel wants. 
 *
 * @param state  The current state vector
 * @param dt     The time offset for the calculations
 * @param order  Order of the derivative being calculated
 */
 //------------------------------------------------------------------------------
bool ExternalModel::GetDerivatives(Real *state, Real dt, Integer order,
   const Integer id)
{
   if (!state || !deriv || dimension <= 0)
      throw ODEModelException("External force requires initialized state and derivative storage.");
   const Real atEpoch = epoch + (elapsedTime + dt) / GmatTimeConstants::SECS_PER_DAY;
   RealArray privateState(state,state+dimension);
   const auto result = EvaluatePython(privateState.data(),atEpoch,GetStateDateTypes(),order);
   // Publish only a complete, checked callback result.
   std::copy(result.begin(),result.end(),deriv);
   return true;
}

RealArray ExternalModel::EvaluatePython(Real *state, Real atEpoch,
      const StringArray &description, Integer order)
{
   if (!pythonIf || !Py_IsInitialized())
      throw ODEModelException("External-force Python interface is not initialized.");
   if (!state || dimension <= 0 || static_cast<Integer>(description.size()) != dimension)
      throw ODEModelException("External-force state and description dimensions differ.");
   struct PythonLock
   {
      PyGILState_STATE state = PyGILState_Ensure();
      ~PythonLock() { PyGILState_Release(state); }
   } lock;
   if (!initializedMessagePrinted)
   {
      fullFilePath = pythonIf->PyExternalGetPythonPath(scriptFilename);
      MessageInterface::ShowMessage("External Force Model: \"%s\" initialized from \"%s\", entry point \"%s\".\n",
         instanceName.c_str(),fullFilePath.c_str(),entryPoint.c_str());
      initializedMessagePrinted = true;
   }
   auto release = [](PyObject *object) { Py_XDECREF(object); };
   using PythonObject = std::unique_ptr<PyObject,decltype(release)>;
   PythonObject result(pythonIf->PyExternalFunctionWrapper(scriptFilename,entryPoint,
      state,atEpoch,description,order,dimension,4),release);
   PythonObject values(result ? PySequence_Fast(result.get(),"External force must return a numeric sequence.") : nullptr,release);
   auto fail = [&]()
   {
      std::string message;
      pythonIf->PyErrorMsg(nullptr,nullptr,nullptr,message);
      throw InterfaceException("External-force callback failed: " + message);
   };
   if (!values) fail();
   if (PySequence_Fast_GET_SIZE(values.get()) != dimension)
      throw InterfaceException("External-force callback returned a derivative count different from the state dimension.");
   RealArray output(dimension);
   for (Integer index = 0; index < dimension; ++index)
   {
      output[index] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(values.get(),index));
      if (PyErr_Occurred()) fail();
      if (!std::isfinite(output[index]))
         throw InterfaceException("External-force callback returned a nonfinite derivative.");
   }
   return output;
}

Real* ExternalModel::PythonDerivatives(Real *state, Real atEpoch,
      StringArray description, Integer order, const Integer id)
{
   const auto values = EvaluatePython(state,atEpoch,description,order);
   std::unique_ptr<Real[]> result(new Real[values.size()]);
   std::copy(values.begin(),values.end(),result.get());
   return result.release();
}

Rvector6 ExternalModel::GetDerivativesForSpacecraft(Spacecraft *sc)
{
   if (!sc || !psm || !forceOrigin)
      throw ODEModelException("External-force sampling requires a spacecraft and its propagation state manager.");
   auto *aggregate = psm->GetState();
   const auto &mapping = *psm->GetStateMap();
   if (!aggregate || aggregate->GetSize() != dimension || static_cast<Integer>(mapping.size()) != dimension)
      throw ODEModelException("External-force sampling state layout is inconsistent.");
   const GmatTime atEpoch = hasPrecisionTime ? sc->GetEpochGT() : GmatTime(sc->GetEpoch());
   const Rvector6 origin = hasPrecisionTime ? forceOrigin->GetMJ2000State(atEpoch) :
      forceOrigin->GetMJ2000State(atEpoch.GetMjd());
   RealArray state(aggregate->GetState(),aggregate->GetState()+dimension);
   Integer ownerIndex = -1;
   for (Integer index = 0; index < dimension; ++index)
   {
      if (mapping[index]->elementID != Gmat::CARTESIAN_STATE || mapping[index]->subelement != 1)
         continue;
      auto *owner = dynamic_cast<Spacecraft*>(mapping[index]->object);
      if (!owner || index > dimension-6)
         throw ODEModelException("External-force Cartesian state has no spacecraft owner.");
      if (owner->GetEpoch() != sc->GetEpoch() ||
            (hasPrecisionTime && owner->GetEpochGT() != atEpoch))
         throw ODEModelException("External-force sampling requires coeval spacecraft states.");
      for (Integer component = 0; component < 6; ++component)
         state[index+component] = owner->GetState()[component]-origin[component];
      if (owner == sc) ownerIndex = index;
   }
   if (ownerIndex < 0)
      throw ODEModelException("External-force query spacecraft is absent from the propagation state.");
   const auto description = GetStateDateTypes();
   auto evaluate = [&](Integer component, Real offset)
   {
      RealArray trial(state);
      trial[ownerIndex+component] += offset;
      return EvaluatePython(trial.data(),atEpoch.GetMjd(),description,1);
   };
   const auto nominal = evaluate(0,0);
   if (OrbitStmForceGradient::Request::For(this))
   {
      Rmatrix gradient(3,6);
      for (Integer column = 0; column < 6; ++column)
      {
         Real step = column < 3 ? 1.0 : 0.01;
         Rvector3 previous(0,0,0);
         bool accepted = false;
         for (Integer attempt = 0; attempt < 5; ++attempt)
         {
            const auto minusTwo = evaluate(column,-2*step);
            const auto minusOne = evaluate(column,-step);
            const auto plusOne = evaluate(column,step);
            const auto plusTwo = evaluate(column,2*step);
            Rvector3 current;
            accepted = attempt > 0;
            for (Integer row = 0; row < 3; ++row)
            {
               const Integer index = ownerIndex+3+row;
               current[row] = (8*(plusOne[index]-minusOne[index])-(plusTwo[index]-minusTwo[index]))/(12*step);
               const Real scale = std::max(std::abs(current[row]),std::abs(previous[row]));
               const Real rounding = 512*std::numeric_limits<Real>::epsilon()*
                  std::max({std::abs(nominal[index]),std::abs(minusTwo[index]),std::abs(plusTwo[index])})/step;
               const Real tolerance = std::max(rounding,scale*1e-6);
               const Real forward = (-3*nominal[index]+4*plusOne[index]-plusTwo[index])/(2*step);
               const Real backward = (3*nominal[index]-4*minusOne[index]+minusTwo[index])/(2*step);
               accepted = accepted && std::isfinite(current[row]) &&
                  std::abs(current[row]-previous[row]) <= tolerance &&
                  std::abs(forward-backward) <= tolerance;
            }
            if (accepted)
            {
               for (Integer row = 0; row < 3; ++row) gradient(row,column) = current[row];
               break;
            }
            previous = current;
            step /= 2;
         }
         if (!accepted)
            throw ODEModelException("External-force sampling did not converge to a smooth finite gradient.");
      }
      OrbitStmForceGradient::Request::Capture(this,gradient);
   }
   return Rvector6(nominal[ownerIndex],nominal[ownerIndex+1],nominal[ownerIndex+2],
      nominal[ownerIndex+3],nominal[ownerIndex+4],nominal[ownerIndex+5]);
}

//------------------------------------------------------------------------------
// bool IsUnique(const std::string &forBody)
//------------------------------------------------------------------------------
/**
 * Specifies if the External force is the one and only External force allowed
 *
 * @param forBody The body at which the force must be unique (unused for External)
 *
 * @return true for External.
 */
//------------------------------------------------------------------------------
bool ExternalModel::IsUnique(const std::string& forBody)
{
   return true;
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
bool ExternalModel::SupportsDerivative(Gmat::StateElementId id)
{
   #ifdef DEBUG_REGISTRATION
      MessageInterface::ShowMessage(
            "ExternalModel checking for support for id %d\n", id);
   #endif
      
   if (id == Gmat::CARTESIAN_STATE)
      return true;
   
   if (id == Gmat::ORBIT_STATE_TRANSITION_MATRIX)
      return true;
   
   if (id == Gmat::ORBIT_A_MATRIX)
      return true;

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
 * @param id           State Element ID for the derivative type
 * @param index        Starting index in the state vector for this type of derivative
 * @param quantity     Number of objects that supply this type of data
 * @param totalSize    For sizable types, the size to use.  For example, for STM,
 *                     this is the number of elements of STM
 * 
 * @return true if the type is supported, false otherwise. 
 */
//------------------------------------------------------------------------------
bool ExternalModel::SetStart(Gmat::StateElementId id, Integer index, 
                        Integer quantity, Integer totalSize)
{
   #ifdef DEBUG_REGISTRATION
      MessageInterface::ShowMessage("ExternalModel setting start data "
            "for id = %d to index %d; %d objects identified\n", id, index, 
            quantity);
   #endif
   
   bool retval = false;
   
   switch (id)
   {
      case Gmat::CARTESIAN_STATE:
         satCount = quantity;
         cartesianCount = quantity;
         cartesianStart = index;
         fillCartesian = true;
         retval = true;
         break;
         
      case Gmat::ORBIT_STATE_TRANSITION_MATRIX:
         stmCount = quantity;
         stmStart = index;
         fillSTM = true;
         totalSTMSize = totalSize;
         retval = true;
         break;
         
      case Gmat::ORBIT_A_MATRIX:
         aMatrixCount = quantity;
         aMatrixStart = index;
         fillAMatrix = true;
         totalSTMSize = totalSize;
         retval = true;
         break;

      default:
         break;
   }
   
   return retval;
}

//------------------------------------------------------------------------------
// Rvector3 GetTorquesForSpacecraft(Spacecraft *sc)
//------------------------------------------------------------------------------
/**
 * Function used to determine the torque on the spacecraft
 *
 * @return The 3 element torque vector in Nm.
 */
//------------------------------------------------------------------------------
Rvector3 ExternalModel::GetTorquesForSpacecraft(Spacecraft *sc)
{
   //Work in Progress
   Rvector3 torque; 
   if (torqueEntryPoint == "")
	   return torque;

   Real *j2kState = sc->GetState().GetState();
   Real state[6];
   Real now = sc->GetEpoch();
   GmatTime nowgt = sc->GetEpochGT();
   PyObject* pyRet;  
   StringArray stateDescription = GetStateDateTypes();
   try
   {
	   if (hasPrecisionTime)
	   {
		   BuildModelStateGT(nowgt, state, j2kState);
		   pyRet = pythonIf->PyExternalFunctionWrapper(scriptFilename, torqueEntryPoint, state, nowgt.GetMjd(), stateDescription, 1, dimension, 3);
	   }
	   else
	   {
		   BuildModelState(now, state, j2kState);
		   pyRet = pythonIf->PyExternalFunctionWrapper(scriptFilename, torqueEntryPoint, state, now, stateDescription, 1, dimension, 3);
	   }
	  
   }
   catch (BaseException &ex)
   {
	   throw CommandException("Error in the CallPython command torque execution:\n" +
		   ex.GetFullMessage());
   }
   catch (...)
   {
	   throw InterfaceException("An unhandled Python exception was thrown during Torque execution");
   }

   if (pyRet) {
	   for (Integer i = 0; i < 3; ++i)
	   {
		   PyObject* pyItem = PyList_GetItem(pyRet, i);
		   torque[i] = PyFloat_AsDouble(pyItem);
	   }
   }

   return torque;
}

//------------------------------------------------------------------------------
// StringArray GetStateDateTypes()
//------------------------------------------------------------------------------
/**
 * Loops through the state map and renames everything to user friendly names
 * This is then outputted to the python call
 * Returns an StringArray with the names of all of the data types in the state
 * @TODO This should be reworked to not write out the string every single python
 * call which is incredibly expensive. Probably better as an API call or something
 * That can be turned on and off
 *
 */
 //------------------------------------------------------------------------------
StringArray ExternalModel::GetStateDateTypes()
{
   StringArray elements;
   if (!psm || static_cast<Integer>(psm->GetStateMap()->size()) != dimension)
      throw ODEModelException("External-force state descriptions require a matching propagation state manager.");
   std::vector<ListItem*> stateMap = *(psm->GetStateMap());
   int cartesianCount = 0;
   //GmatState* psmState = psm->GetState();
   //StringArray stateDescription = psmState->GetElementDescriptions();
   
   //This
   for (Integer i = 0; i < stateMap.size(); i++)
   {
      //Handles CartesianState values
      std::string elementStr = "";
      if (stateMap[i]->elementName == "CartesianState")
      {
         std::string elementOne;
         cartesianCount++;

         //Starts at one as it preincrements, could go either way
         switch (cartesianCount % 6)
         {
         case 1: elementOne = "X";
            break;
         case 2: elementOne = "Y";
            break;
         case 3: elementOne = "Z";
            break;
         case 4: elementOne = "Vx";
            break;
         case 5: elementOne = "Vy";
            break;
         case 0: elementOne = "Vz";
            break;
         }
         elementStr = "CartesianState." + elementOne;
      }
      else
      {
         //Handles Covariance values
         if ((stateMap[i]->elementName == "Covariance") || (stateMap[i]->elementName == "STM"))
         {
            std::string elementOne;
            std::string elementTwo;
            switch (stateMap[i]->rowIndex)
            {
            case 0: elementOne = "X";
               break;
            case 1: elementOne = "Y";
               break;
            case 2: elementOne = "Z";
               break;
            case 3: elementOne = "Vx";
               break;
            case 4: elementOne = "Vy";
               break;
            case 5: elementOne = "Vz";
               break;
            default:
               elementOne = psm->GetSTMParameterName(stateMap[i]->rowIndex);
               break;
            }
            switch (stateMap[i]->colIndex)
            {
            case 0: elementTwo = "X";
               break;
            case 1: elementTwo = "Y";
               break;
            case 2: elementTwo = "Z";
               break;
            case 3: elementTwo = "Vx";
               break;
            case 4: elementTwo = "Vy";
               break;
            case 5: elementTwo = "Vz";
               break;
            default:
               elementTwo = psm->GetSTMParameterName(stateMap[i]->colIndex);
               break;
            }
            if (stateMap[i]->elementName == "Covariance")
            {
               elementStr = "Covariance." + elementOne + elementTwo;
            }
            else if (stateMap[i]->elementName == "STM")
            {
               elementStr = "STM." + elementOne + elementTwo;
            }
         }
         else
         {
            //Handles extra cases on a case by case basis
            if (stateMap[i]->elementName == "MassFlow")
            {
               elementStr = "TotalMass";
            }
            else 
            {
               elementStr = stateMap[i]->elementName;
            }
         }
      }
      if (stateMap[i]->object->IsOfType("Formation"))
      {
         int index = (cartesianCount-1) / 6;
         elementStr = stateMap[i]->object->GetRefObjectNameArray(0)[index] + "." + elementStr;
      }
      elementStr = stateMap[i]->objectName + "." + elementStr;
      elements.push_back(elementStr);
   }
   return elements;
}

// Reset/copy through the inherited polymorphic contract without slicing.
void ExternalModel::Copy(const GmatBase *original)
{
   const auto *source = dynamic_cast<const ExternalModel*>(original);
   if (!source || typeid(*source) != typeid(ExternalModel) ||
       typeid(*this) != typeid(ExternalModel))
      throw GmatBaseException("Cannot copy incompatible object into ExternalModel");
   if (source == this)
      return;
   operator=(*source);
}
