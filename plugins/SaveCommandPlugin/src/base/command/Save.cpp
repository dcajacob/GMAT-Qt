//------------------------------------------------------------------------------
//                                  Save
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool.
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
// Author: Darrel J. Conway
// Created: 2004/02/26
//
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under contract
// number S-67573-G
//
/**
 * Class implementation for the save command
 */
//------------------------------------------------------------------------------

#include "Save.hpp"
#include "FileManager.hpp"      // for GetPathname()
#include "MessageInterface.hpp"
#include "FileUtil.hpp"
#include "GmatGlobal.hpp"       // for GetDataPrecision()
#include "CoordinateSystem.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "ODEModel.hpp"
#include "ObjectScriptExportScope.hpp"
#include "ScriptStringLiteral.hpp"
#include <functional>
#include <memory>
#include <map>
#include <set>
#include <sstream>
#include <vector>


//#define DEBUG_SAVE_INIT
//#define DEBUG_SAVE_EXEC
//#define DEBUG_SAVE_OUTPUT

namespace
{
// Export attached runtime clones rather than configured prototypes, whose fuel,
// power, plate and sensor settings can differ from the running spacecraft.
ObjectArray SaveObjectsWithAttachments(const ObjectArray &roots)
{
   ObjectArray ordered;
   std::map<std::string, GmatBase*> objects;
   std::set<GmatBase*> active;
   std::function<void(GmatBase*)> collect = [&](GmatBase *object)
   {
      if (!object)
         throw CommandException("Save encountered a missing attached object");
      if (active.count(object))
         throw CommandException("Save encountered a cyclic attachment graph");
      const std::string name = object->GetName();
      const auto previous = objects.find(name);
      if (previous != objects.end())
      {
         if (previous->second != object &&
             (previous->second->GetTypeName() != object->GetTypeName() ||
              previous->second->GetGeneratingString(Gmat::NO_COMMENTS) !=
                 object->GetGeneratingString(Gmat::NO_COMMENTS)))
            throw CommandException("Save cannot export objects named \"" + name +
               "\" with different runtime states into one script");
         return;
      }
      objects[name] = object;
      active.insert(object);
      if (object->IsOfType(Gmat::SPACECRAFT))
      {
         for (auto *tank : object->GetRefObjectArray(Gmat::FUEL_TANK)) collect(tank);
         for (auto *thruster : object->GetRefObjectArray(Gmat::THRUSTER)) collect(thruster);
         for (auto *power : object->GetRefObjectArray(Gmat::POWER_SYSTEM)) collect(power);
         for (auto *plate : object->GetRefObjectArray(Gmat::PLATE)) collect(plate);
         for (auto *hardware : object->GetRefObjectArray(Gmat::HARDWARE)) collect(hardware);
      }
      else if (object->IsOfType(Gmat::GROUND_STATION))
      {
         // Include authored station attachments without exporting its
         // file-generated horizon-mask imager as a second resource.
         for (const auto type : {Gmat::HARDWARE,Gmat::ERROR_MODEL})
         {
            const StringArray names=object->GetStringArrayParameter(
                  type==Gmat::HARDWARE ? "AddHardware" : "ErrorModels");
            for (const auto &name : names)
            {
               auto *attached=object->GetRefObject(type,name);
               if (!attached)
                  throw CommandException("Save cannot find station attachment \"" + name + "\" on " + object->GetName());
               collect(attached);
            }
         }
      }
      else if (object->IsOfType(Gmat::THRUSTER))
      {
         for (auto *tank : object->GetRefObjectArray(Gmat::HARDWARE)) collect(tank);
      }
      else if (object->IsOfType(Gmat::HARDWARE))
      {
         // Use base interfaces so Save does not depend on optional hardware
         // plugins. Follow device attachments, such as a primary antenna,
         // modeled field of view or receiver error models, before the device.
         const ObjectTypeArray types = object->GetRefObjectTypeArray();
         for (const auto type : types)
         {
            if (type != Gmat::HARDWARE && type != Gmat::FIELD_OF_VIEW &&
                  type != Gmat::ERROR_MODEL)
               continue;
            // Some implementations return shared scratch arrays. Recursion
            // must not invalidate the names being traversed here.
            StringArray names;
            if (type == Gmat::HARDWARE && object->IsOfType(Gmat::RF_HARDWARE))
            {
               // RFHardware's reference-name getter also validates that an
               // antenna was assigned. Save must be able to export an unused,
               // partially configured RF resource without initializing it.
               // A nonempty name still has to resolve to an attached object.
               const std::string antenna = object->GetStringParameter("PrimaryAntenna");
               if (!antenna.empty())
                  names.push_back(antenna);
            }
            else
               names = object->GetRefObjectNameArray(type);
            for (const auto &attachedName : names)
            {
               GmatBase *attached = object->GetRefObject(type, attachedName);
               if (!attached)
                  throw CommandException("Save cannot find attachment \"" +
                        attachedName + "\" on object \"" + name + "\"");
               collect(attached);
            }
         }
      }
      else if (auto *prop = dynamic_cast<PropSetup*>(object))
      {
         if (prop->GetPropagator() && prop->GetPropagator()->UsesODEModel() &&
             prop->GetODEModel())
            collect(prop->GetODEModel());
      }
      active.erase(object);
      ordered.push_back(object);
   };
   // Prefer spacecraft-owned runtime values if the same attachment is also
   // explicitly selected. Repeated references produce one definition.
   for (auto *object : roots)
      if (object && object->IsOfType(Gmat::SPACECRAFT)) collect(object);
   // Numerical propagators serialize their owned runtime force model. Visit
   // them before explicit force-model roots, just as for spacecraft hardware.
   for (auto *object : roots)
      if (object && object->IsOfType(Gmat::PROP_SETUP)) collect(object);
   for (auto *object : roots)
      if (!object || objects.find(object->GetName()) == objects.end()) collect(object);
   return ordered;
}
}

//---------------------------------
//  static data
//---------------------------------
const std::string
Save::PARAMETER_TEXT[SaveParamCount - GmatCommandParamCount] = 
{
   "ObjectNames",
   "FilenameStyle",
   "Filename",
};

const Gmat::ParameterType
Save::PARAMETER_TYPE[SaveParamCount - GmatCommandParamCount] =
{
   Gmat::STRINGARRAY_TYPE,   // "ObjectNames",
   Gmat::ENUMERATION_TYPE,   // "FilenameStyle",
   Gmat::FILENAME_TYPE,      // "Filename",
};


//------------------------------------------------------------------------------
// Save()
//------------------------------------------------------------------------------
/**
 * Default constructor.
 */
//------------------------------------------------------------------------------
Save::Save() :
   GmatCommand   ("Save"),
   appendData    (false),
   wasWritten    (false),
   writeVerbose  (false)
{
   fileArray = NULL;
   parameterCount = SaveParamCount;
}


//------------------------------------------------------------------------------
// ~Save()
//------------------------------------------------------------------------------
/**
 * Destructor.
 */
//------------------------------------------------------------------------------
Save::~Save()
{
   if (fileArray)
      delete [] fileArray;
}


//------------------------------------------------------------------------------
// Save(const Save& sv)
//------------------------------------------------------------------------------
/**
 * Copy constructor.
 * 
 * @param sv The instance that is copied.
 */
//------------------------------------------------------------------------------
Save::Save(const Save& sv) :
   GmatCommand   (sv),
   fileNameArray (sv.fileNameArray),
   appendData    (sv.appendData),     // should be false...
   wasWritten    (sv.wasWritten),
   objNameArray  (sv.objNameArray),
   writeVerbose  (sv.writeVerbose)
{
   objArray.clear();
   fileArray = NULL;
   fileNameArray.clear();
   filenameStyle=sv.filenameStyle;
   outputFilename=sv.outputFilename;
}


//------------------------------------------------------------------------------
// Save& operator=(const Save& sv)
//------------------------------------------------------------------------------
/**
 * Assignmant operator.
 * 
 * @param sv The instance that is copied.
 *
 * @return this instance, with internal data set to match the input command.
 */
//------------------------------------------------------------------------------
Save& Save::operator=(const Save& sv)
{
   if (this == &sv)
      return *this;
        
   GmatCommand::operator=(sv);
   delete [] fileArray;
   fileArray = NULL;
   fileNameArray.clear();
   appendData    = sv.appendData;
   wasWritten    = sv.wasWritten;
   objNameArray  = sv.objNameArray;
   writeVerbose  = sv.writeVerbose;
   filenameStyle = sv.filenameStyle;
   outputFilename = sv.outputFilename;
   combinedFile = false;
   objArray.clear();
   
   return *this;
}


//------------------------------------------------------------------------------
// std::string GetParameterText(const Integer id) const
//------------------------------------------------------------------------------
std::string Save::GetParameterText(const Integer id) const
{
   if (id >= GmatCommandParamCount && id < SaveParamCount)
      return PARAMETER_TEXT[id - GmatCommandParamCount];
   else
      return GmatCommand::GetParameterText(id);
}


//------------------------------------------------------------------------------
// Integer GetParameterID(const std::string &str) const
//------------------------------------------------------------------------------
Integer Save::GetParameterID(const std::string &str) const
{
   for (int i=GmatCommandParamCount; i<SaveParamCount; i++)
   {
      if (str == PARAMETER_TEXT[i - GmatCommandParamCount])
         return i;
   }
   
   return GmatCommand::GetParameterID(str);
}


//------------------------------------------------------------------------------
// Gmat::ParameterType GetParameterType(const Integer id) const
//------------------------------------------------------------------------------
Gmat::ParameterType Save::GetParameterType(const Integer id) const
{
   if (id >= GmatCommandParamCount && id < SaveParamCount)
      return PARAMETER_TYPE[id - GmatCommandParamCount];
   else
      return GmatCommand::GetParameterType(id);
}


//------------------------------------------------------------------------------
// std::string GetParameterTypeString(const Integer id) const
//------------------------------------------------------------------------------
std::string Save::GetParameterTypeString(const Integer id) const
{
   if (id >= GmatCommandParamCount && id < SaveParamCount)
      return GmatBase::PARAM_TYPE_STRING[GetParameterType(id)];
   else
      return GmatCommand::GetParameterTypeString(id);
}


//------------------------------------------------------------------------------
//  bool  SetStringParameter(const Integer id, const std::string value)
//------------------------------------------------------------------------------
/**
 * This method sets the string parameter value, given the input
 * parameter ID.
 *
 * @param <id> ID for the requested parameter.
 * @param <value> string value for the requested parameter.
 *
 * @exception <CommandException> thrown if value is already in the list.
 *
 * @return  success flag.
 *
 */
//------------------------------------------------------------------------------
std::string Save::GetStringParameter(const Integer id) const
{
   if (id==FILENAME_STYLE) return filenameStyle;
   if (id==OUTPUT_FILENAME) return outputFilename;
   return GmatCommand::GetStringParameter(id);
}

std::string Save::GetStringParameter(const std::string &label) const
{
   return GetStringParameter(GetParameterID(label));
}

const StringArray& Save::GetPropertyEnumStrings(const Integer id) const
{
   static const StringArray styles={"Worksheet","LegacyData"};
   if (id==FILENAME_STYLE) return styles;
   return GmatCommand::GetPropertyEnumStrings(id);
}

bool Save::SetStringParameter(const Integer id, const std::string &value)
{
   if (id==FILENAME_STYLE)
   {
      if (value!="Worksheet" && value!="LegacyData")
         throw CommandException("Save FilenameStyle must be Worksheet or LegacyData");
      filenameStyle=value;return true;
   }
   if (id==OUTPUT_FILENAME)
   {
      for (unsigned char ch : value)
         if (ch<32 || ch==127) throw CommandException("Save Filename cannot contain control characters");
      outputFilename=value;return true;
   }
   if (id == OBJECT_NAMES)
   {
      Integer sz = objNameArray.size();
      for (Integer ii = 0; ii < sz; ii++)
      {
         if (objNameArray[ii] == value)
         {
            std::string ex = "Attempting to add """;
            ex += value + """ more than once to list of objects.\n";
            throw CommandException(ex);
         }
      }
      
      objNameArray.push_back(value);
      return true;
   }
   
   return GmatCommand::SetStringParameter(id, value);
}


//------------------------------------------------------------------------------
//  bool SetStringParameter(const std::string &label, const std::string &value)
//------------------------------------------------------------------------------
/**
 * Sets the value for a std::string parameter.
 *
 * @param <label> The (string) label for the parameter.
 * @param <value> New value for the parameter.
 *
 * @return The string stored for this parameter.
 */
//------------------------------------------------------------------------------
bool Save::SetStringParameter(const std::string &label, const std::string &value)
{
   return SetStringParameter(GetParameterID(label), value);
}


//------------------------------------------------------------------------------
//  std::string  GetStringParameter(const Integer id, const Integer index)
//------------------------------------------------------------------------------
/**
 * This method returns the string parameter value, given the input
 * parameter ID and the index into the array.
 *
 * @param <id> ID for the requested parameter.
 * @param <index> index into the StringArray parameter.
 *
 * @exception <CommandException> thrown if value is out of range
 *
 * @return  string value at index 'index'.
 *
 */
//------------------------------------------------------------------------------
std::string Save::GetStringParameter(const Integer id,
                                     const Integer index) const
{
   if (id == OBJECT_NAMES)
   {
      if ((index < 0) || (index >= ((Integer) objNameArray.size())))
         throw CommandException
            ("Index out of bounds when attempting to return object name\n");
      return objNameArray.at(index);
   }
   
   return GmatCommand::GetStringParameter(id, index);
}


//------------------------------------------------------------------------------
//  std::string GetStringParameter(const std::string &label,
//                                 const Integer index) const
//------------------------------------------------------------------------------
/**
 * Retrieve a string parameter.
 *
 * @param <label> The (string) label for the parameter.
 * @param <index> array index for the parameter.
 *
 * @return The string stored for this parameter.
 */
//------------------------------------------------------------------------------
std::string Save::GetStringParameter(const std::string &label,
                                     const Integer index) const
{
   return GetStringParameter(GetParameterID(label), index);
}


//------------------------------------------------------------------------------
//  const StringArray&  GetStringArrayParameter(const Integer id)
//------------------------------------------------------------------------------
/**
 * This method returns the string array value, given the input
 * parameter ID .
 *
 * @param <id> ID for the requested parameter.
 *
  * @return  string array.
 *
 */
//------------------------------------------------------------------------------
const StringArray& Save::GetStringArrayParameter(const Integer id) const
{
   if (id == OBJECT_NAMES)
      return objNameArray;
   
   return GmatCommand::GetStringArrayParameter(id);
}


//------------------------------------------------------------------------------
// bool GetRefObjectName(const UnsignedInt type)
//------------------------------------------------------------------------------
/**
 * Retrieves the reference object names.
 * 
 * @param type The type of the reference object.
 * 
 * @return the name of the object.
 */
//------------------------------------------------------------------------------
std::string Save::GetRefObjectName(const UnsignedInt type) const
{
   if (objNameArray.size() == 0)
      return "";
   else
      return objNameArray[0];
}


//------------------------------------------------------------------------------
// const StringArray& GetRefObjectNameArray(const UnsignedInt type)
//------------------------------------------------------------------------------
/**
 * Accesses arrays of names for referenced objects.
 * 
 * @param type Type of object requested.
 * 
 * @return the StringArray containing the referenced object names.
 */
//------------------------------------------------------------------------------
const StringArray& Save::GetRefObjectNameArray(const UnsignedInt type)
{
   // it can be any object, so ignore object type
   return objNameArray;
}


//------------------------------------------------------------------------------
// bool SetRefObjectName(const UnsignedInt type, const std::string &name)
//------------------------------------------------------------------------------
/**
 * Sets the reference objects that get saved.
 * 
 * @param type The type of the reference object.
 * @param name The name of the reference object.
 * 
 * @return true on success, false on failure.
 */
//------------------------------------------------------------------------------
bool Save::SetRefObjectName(const UnsignedInt type,
                            const std::string &name)
{
   if (name == "")
      return false;
      
   // Save works for all types, so we don't check the type parameter
   objNameArray.push_back(name);
   return true;
}


//------------------------------------------------------------------------------
// bool Initialize()
//------------------------------------------------------------------------------
/**
 * Connects up the object associations prior to saving them.
 * 
 * @return true on success
 */
//------------------------------------------------------------------------------
bool Save::Initialize()
{
   const bool retval=GmatCommand::Initialize();
   wasWritten=false;appendData=false;
   delete [] fileArray;fileArray=NULL;
   fileNameArray.clear();objArray.clear();
   if (objNameArray.empty()) throw CommandException("Save command has no objects to save");
   for (const auto &name : objNameArray)
   {
      auto *object=FindObject(name);
      if (!object && solarSys)
         object=name=="SolarSystem" ? static_cast<GmatBase *>(solarSys) : solarSys->GetBody(name);
      if (!object) throw CommandException("Save command cannot find object \"" + name + "\"");
      objArray.push_back(object);
   }
   const std::string path=FileManager::Instance()->GetAbsPathname(FileManager::OUTPUT_PATH);
   combinedFile=!outputFilename.empty() || filenameStyle=="LegacyData";
   if (!outputFilename.empty())
      fileNameArray.push_back(GmatFileUtil::IsPathRelative(outputFilename) ? path+outputFilename : outputFilename);
   else if (combinedFile)
   {
      fileNameArray.push_back(path+objNameArray.front());
      for (UnsignedInt i=1;i<objNameArray.size();++i) UpdateOutputFileNames(i,objNameArray[i]);
      fileNameArray.front() += objArray.size()==1 ? "."+objArray.front()->GetTypeName()+".data" : ".data";
   }
   else
      for (auto *object : objArray)
         fileNameArray.push_back(path+object->GetName()+"."+object->GetTypeName());
   fileArray=new std::ofstream[fileNameArray.size()];
   return retval;
}

//------------------------------------------------------------------------------
// bool Execute()
//------------------------------------------------------------------------------
/**
 * Write objects to a text file.
 *
 * @return true if the Command runs to completion, false if an error
 *         occurs.
 */
//------------------------------------------------------------------------------
bool Save::Execute()
{
   #ifdef DEBUG_SAVE_EXEC
   MessageInterface::ShowMessage("Save::Execute() entered\n");
   #endif
   
   Integer prec = GmatGlobal::Instance()->GetDataPrecision();
   
   if (objArray.empty())
      throw CommandException("Save command has no objects to save");
   
   if (!objArray[0])
      throw CommandException("Object not set for Save command");
   
   // Build and render every export before opening/truncating any file. Native
   // serializers can throw even after the attachment graph is valid.
   std::vector<std::string> rendered;
   {
      // Save exports explicitly selected resources, including declarations in
      // included/function files. Keep the request active for attachment-state
      // comparisons and nested owned serializers, and restore it before I/O.
      ObjectScriptExportScope exportScope;
      std::vector<ObjectArray> exports;
      if (combinedFile) exports.push_back(SaveObjectsWithAttachments(objArray));
      else for (auto *object : objArray)
         exports.push_back(SaveObjectsWithAttachments(ObjectArray(1, object)));

      rendered.reserve(exports.size());
      for (const auto &objects : exports)
      {
         std::ostringstream stream;
         stream.precision(prec);
         ObjectScriptExportScope::Prepare(stream);
         for (auto *object : objects)
            WriteObject(stream, object);
         if (!stream)
            throw CommandException("Save command failed preparing object output");
         rendered.push_back(stream.str());
      }
   }

   try
   {
      for (UnsignedInt i = 0; i < fileNameArray.size(); ++i)
      {
         fileArray[i].clear();
         const bool append = appendData || wasWritten;
         fileArray[i].open(fileNameArray[i].c_str(), append ? std::ios::app : std::ios::out);
         if (!fileArray[i].is_open())
            throw CommandException("Save command cannot open output file \"" + fileNameArray[i] + "\"");
         fileArray[i].precision(prec);
      }
      for (UnsignedInt i = 0; i < rendered.size(); ++i)
         fileArray[i] << rendered[i];
      for (UnsignedInt i = 0; i < fileNameArray.size(); ++i)
      {
         fileArray[i].flush();
         if (!fileArray[i])
            throw CommandException("Save command failed writing output file \"" + fileNameArray[i] + "\"");
         fileArray[i].close();
         if (fileArray[i].fail())
            throw CommandException("Save command failed closing output file \"" + fileNameArray[i] + "\"");
      }
      wasWritten = true;
   }
   catch (...)
   {
      for (UnsignedInt i = 0; i < fileNameArray.size(); ++i)
         if (fileArray[i].is_open()) fileArray[i].close();
      throw;
   }

   BuildCommandSummary(true);
   
   return true;
}


//------------------------------------------------------------------------------
// void RunComplete()
//------------------------------------------------------------------------------
void Save::RunComplete()
{
   fileNameArray.clear();
   objArray.clear();
   GmatCommand::RunComplete();
}


//------------------------------------------------------------------------------
//  GmatBase* Clone() const
//------------------------------------------------------------------------------
/**
 * This method returns a clone of the Save.
 *
 * @return clone of the Save.
 */
//------------------------------------------------------------------------------
GmatBase* Save::Clone() const
{
   return (new Save(*this));
}


//------------------------------------------------------------------------------
//  const std::string& GetGeneratingString()
//------------------------------------------------------------------------------
/**
 * Method used to retrieve the string that was parsed to build this GmatCommand.
 *
 * This method is used to retrieve the GmatCommand string from the script that
 * was parsed to build the GmatCommand.  It is used to save the script line, so
 * that the script can be written to a file without inverting the steps taken to
 * set up the internal object data.  As a side benefit, the script line is
 * available in the GmatCommand structure for debugging purposes.
 *
 * @param mode    Specifies the type of serialization requested.  (Not yet used
 *                in commands)
 * @param prefix  Optional prefix appended to the object's name.  (Not yet used
 *                in commands)
 * @param useName Name that replaces the object's name.  (Not yet used in
 *                commands)
 *
 * @return The script line that, when interpreted, defines this Save command.
 */
//------------------------------------------------------------------------------
const std::string& Save::GetGeneratingString(Gmat::WriteMode mode,
                                            const std::string &prefix,
                                            const std::string &useName)
{
   // Build the local string
   generatingString = prefix + "Save";
   for (StringArray::iterator i = objNameArray.begin(); i != objNameArray.end(); ++i)
      generatingString += " " + *i;
   if (filenameStyle!="Worksheet" || !outputFilename.empty())
   {
      generatingString += " {FilenameStyle = " + filenameStyle;
      if (!outputFilename.empty())
      {
         std::string quoted;
         for (char ch : outputFilename) {quoted+=ch;if (ch=='\'') quoted+='\'';}
         generatingString += ", Filename = '" + quoted + "'";
      }
      generatingString += "}";
   }
   generatingString += ";";

   // Then call the base class method
   return GmatCommand::GetGeneratingString(mode, prefix, useName);
}


//------------------------------------------------------------------------------
// virtual bool TakeAction(const std::string &action,  
//                         const std::string &actionData = "");
//------------------------------------------------------------------------------
/**
 * This method performs action.
 *
 * @param <action> action to perform
 * @param <actionData> action data associated with action
 * @return true if action successfully performed
 *
 */
//------------------------------------------------------------------------------
bool Save::TakeAction(const std::string &action, const std::string &actionData)
{
   #if DEBUG_TAKE_ACTION
   MessageInterface::ShowMessage
      ("Save::TakeAction() action=%s, actionData=%s\n",
       action.c_str(), actionData.c_str());
   #endif
   
   if (action == "Clear")
   {
      objNameArray.clear();
      return true;
   }

   return false;
}


//------------------------------------------------------------------------------
//  bool RenameRefObject(const UnsignedInt type,
//                       const std::string &oldName, const std::string &newName)
//------------------------------------------------------------------------------
/**
 * This method updates object names when the user changes them.
 *
 * @param type Type of object that is renamed.
 * @param oldName Old name for the object.
 * @param newName New name for the object.
 *
 * @return true on success.
 */
//------------------------------------------------------------------------------
bool Save::RenameRefObject(const UnsignedInt type,
                           const std::string &oldName,
                           const std::string &newName)
{
   for (Integer index = 0; index < (Integer)objNameArray.size(); ++index)
   {
      if (objNameArray[index] == oldName)
         objNameArray[index] = newName;
   }
   
   return true;
}


//------------------------------------------------------------------------------
// void UpdateOutputFileNames(Integer index, const std::string objName)
//------------------------------------------------------------------------------
void Save::UpdateOutputFileNames(Integer index, const std::string objName)
{
   if (combinedFile && index>0) fileNameArray[0] += "_"+objName;
}


//------------------------------------------------------------------------------
// void Save::WriteObject()
//------------------------------------------------------------------------------
/**
 * Writes out the script snippet that is needed to recreate an object.
 */
//------------------------------------------------------------------------------
void Save::WriteObject(std::ostream &stream, GmatBase *o)
{
   #ifdef DEBUG_SAVE_OUTPUT
      MessageInterface::ShowMessage("Save: %s <a %s> has %d parameters\n",
         o->GetName().c_str(), o->GetTypeName().c_str(),
         o->GetParameterCount());
   #endif
   
   // Built-in coordinate systems are supplied by the interpreter and reject
   // every assignment on reload. Their immutable definition is identified by
   // the reserved name: emit its declaration without illegal assignments.
   // Custom systems continue through the complete native serializer below.
   auto *coordinate = dynamic_cast<CoordinateSystem*>(o);
   if (coordinate && coordinate->IsBuiltIn())
   {
      stream << "Create CoordinateSystem " << o->GetName() << ";\n";
      return;
   }

   // The graph walker writes each numerical propagator's force model once.
   // Suppress the propagator serializer's embedded copy on a detached clone;
   // never change the live include-ODE-model flag used by later commands.
   std::unique_ptr<GmatBase> runtimePropagator;
   if (auto *prop = dynamic_cast<PropSetup*>(o))
   {
      if (prop->GetPropagator() && prop->GetPropagator()->UsesODEModel() &&
          prop->GetODEModel())
      {
         runtimePropagator.reset(prop->Clone());
         runtimePropagator->TakeAction("ExcludeODEModel");
         o = runtimePropagator.get();
      }
   }

   // StringVar serializes its configured expression rather than the current
   // mission value. Export a detached literal snapshot without changing the
   // live object's expression or dependency used by later mission commands.
   std::unique_ptr<GmatBase> runtimeString;
   if (o->GetTypeName() == "String")
   {
      runtimeString.reset(o->Clone());
      runtimeString->SetStringParameter("Expression", o->GetStringParameter("Value"));
      runtimeString->SetStringParameter("DepObject", "");
      o = runtimeString.get();
   }

   std::string objectname = o->GetName();
   std::string prefix = "";

   /**
    * @note:
    * Since we are using GetGeneratingString() we don't need to write "Create"
    * except Variables and Strings.  Variables and Strings are special case that
    * they are written in group in the ScriptInterpreter.  For example,
    * "Create Variable var1 var2 var3;" are written when save script.
    * For Arrays, it needs to know the dimension of array so it is handled in the
    * Array::GetGeneratingString().
    */
   
   // "Create Propagator" creates a PropSetup.  This code handles
   // that special case.
   //std::string tname = o->GetTypeName();
   //if (tname == "PropSetup")
   //   tname = "Propagator";   
   //file << "Create " << tname << " " << o->GetName() << "\n";
   
   if (o->GetTypeName() == "Variable" || o->GetTypeName() == "String")
      prefix = "Create " + o->GetTypeName() + " " + o->GetName() + "\n";

   struct RestoreComments
   {
      GmatBase *object;
      bool preface, inLine;
      ~RestoreComments() { object->SetShowPrefaceComment(preface); object->SetShowInlineComment(inLine); }
   } restore{o, o->GetShowPrefaceComment(), o->GetShowInlineComment()};
   // Do not write comments
   o->SetShowPrefaceComment(false);
   o->SetShowInlineComment(false);
   
   // A runtime snapshot must include empty strings and zero variables too.
   // Ordinary script serialization can omit defaults or retain cached text;
   // an appended snapshot must explicitly replace the previous saved value.
   const Gmat::WriteMode mode = (runtimeString || o->GetTypeName() == "Variable") ?
      Gmat::SHOW_SCRIPT : Gmat::SCRIPTING;
   stream << prefix << o->GetGeneratingString(mode, "", "");
   // BodyName is normally supplied by the containing ForceModel. A named
   // standalone contributor has no such owner when its Save file is rebuilt.
   if (o->IsOfType(Gmat::PHYSICAL_MODEL) && !o->IsOfType(Gmat::ODE_MODEL) &&
         o->IsParameterReadOnly(o->GetParameterID("BodyName")))
   {
      const auto body = o->GetStringParameter("BodyName");
      if (!body.empty())
         stream << objectname << ".BodyName = " << GmatScriptString::Quote(body) << ";\n";
   }
   if (o->GetTypeName() == "Array")
   {
      // The native Array serializer omits zero cells as defaults. Repeated
      // Save output can already contain nonzero assignments for those cells,
      // so the snapshot must explicitly reset them on reload.
      const Rmatrix &matrix = o->GetRmatrixParameter("RmatValue");
      const bool keyword = GmatGlobal::Instance()->IsWritingGmatKeyword();
      for (Integer row = 0; row < matrix.GetNumRows(); ++row)
         for (Integer col = 0; col < matrix.GetNumColumns(); ++col)
            if (matrix(row, col) == 0.0)
            {
               if (keyword)
                  stream << "GMAT ";
               stream << o->GetName() << "(" << row + 1 << ", " << col + 1
                      << ") = " << matrix(row, col) << ";\n";
            }
   }
   stream << std::endl;
   
   // The guard restores the original comment settings.
      
   #ifdef DEBUG_SAVE_OUTPUT
      MessageInterface::ShowMessage("Save:WriteObject() leaving\n");
   #endif
      
}


