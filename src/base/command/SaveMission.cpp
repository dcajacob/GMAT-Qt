//$Id$
//------------------------------------------------------------------------------
//                                  SaveMission
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
// Author: Linda Jun (NASA/GSFC)
// Created: 2010/08/03
//
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under contract
// number S-67573-G
//
/**
 * Class implementation for the SaveMission command
 */
//------------------------------------------------------------------------------

#include "SaveMission.hpp"
#include "CommandUtil.hpp"         // for GetLastCommand()
#include "FileManager.hpp"         // for GetPathname()
#include "FileUtil.hpp"            // for ParseFileExtension()
#include "StringUtil.hpp"          // for RemoveEnclosingString()
#include "Moderator.hpp"           // for GetScript()
#include "MessageInterface.hpp"    // for ShowMessage()
#include <fstream>                 // for fopen(), <<
#include <filesystem>

//#define DEBUG_SAVEMISSION_IA
//#define DEBUG_SAVEMISSION_EXE

//---------------------------------
//  static data
//---------------------------------
const std::string
SaveMission::PARAMETER_TEXT[SaveMissionParamCount - GmatCommandParamCount] = 
{
   "Filename",
   "ExecutionPolicy",
};

const Gmat::ParameterType
SaveMission::PARAMETER_TYPE[SaveMissionParamCount - GmatCommandParamCount] =
{
   Gmat::FILENAME_TYPE,   // "Filename",
   Gmat::ENUMERATION_TYPE,
};


//------------------------------------------------------------------------------
//  SaveMission()
//------------------------------------------------------------------------------
/**
 * Constructs the SaveMission command (default constructor).
 */
//------------------------------------------------------------------------------
SaveMission::SaveMission() :
    GmatCommand("SaveMission"),
    executionPolicy("LegacyTerminal"),
    executionPolicySpecified(false)
{
   parameterCount = SaveMissionParamCount;
}


//------------------------------------------------------------------------------
//  ~SaveMission()
//------------------------------------------------------------------------------
/**
 * Destroys the SaveMission command (default constructor).
 */
//------------------------------------------------------------------------------
SaveMission::~SaveMission()
{
}


//------------------------------------------------------------------------------
//  SaveMission(const SaveMission& noop)
//------------------------------------------------------------------------------
/**
 * Makes a copy of the SaveMission command (copy constructor).
 *
 * @param noop The original used to set parameters for this one.
 */
//------------------------------------------------------------------------------
SaveMission::SaveMission(const SaveMission& noop) :
    GmatCommand (noop),
    fileName(noop.fileName),
    executionPolicy(noop.executionPolicy),
    executionPolicySpecified(noop.executionPolicySpecified)
{
}


//------------------------------------------------------------------------------
//  SaveMission& operator=(const SaveMission&)
//------------------------------------------------------------------------------
/**
 * Sets this SaveMission to match another one (assignment operator).
 * 
 * @param noop The original used to set parameters for this one.
 *
 * @return this instance.
 */
//------------------------------------------------------------------------------
SaveMission& SaveMission::operator=(const SaveMission& noop)
{
   if (this != &noop)
   {
      GmatCommand::operator=(noop);
      fileName = noop.fileName;
      executionPolicy = noop.executionPolicy;
      executionPolicySpecified = noop.executionPolicySpecified;
   }
   return *this;
}


//------------------------------------------------------------------------------
//  bool Execute()
//------------------------------------------------------------------------------
/**
 * Exports the configured host mission according to the execution policy.
 * Serialization and file errors propagate to the mission runner.
 */
//------------------------------------------------------------------------------
bool SaveMission::Execute()
{
   bool shouldSave = executionPolicy == "Immediate";
   if (!shouldSave && currentFunction == NULL)
   {
      // Script Event closing markers are GUI grouping only. Do not walk a
      // branch's cyclic chain while checking legacy terminal placement.
      GmatCommand *following = GetNextWhileExecuting();
      while (following != NULL && following->GetTypeName() == "EndScript")
         following = following->GetNextWhileExecuting();
      shouldSave = following == NULL;
   }
   if (shouldSave)
   {
      if (fileName.empty())
         throw CommandException("SaveMission requires a nonempty output filename.");
      FileManager *fm = FileManager::Instance();
      std::string outPath = fm->GetAbsPathname(FileManager::OUTPUT_PATH);
      std::string fname = fileName;
      
      // add output path if there is no path
      if (fileName.find("/") == fileName.npos &&
          fileName.find("\\") == fileName.npos)
      {
         fname = outPath + fileName;
      }
      
      // If file name has no extension, add .script
      if (GmatFileUtil::ParseFileExtension(GmatFileUtil::ParseFileName(fname)) == "")
         fname = fname + ".script";
      
      // Call Moderator::GetScript();
      std::string script = Moderator::Instance()->GetScript();
      if (script.empty())
         throw CommandException("SaveMission could not serialize the host mission.");
      
      #ifdef DEBUG_SAVEMISSION_EXE
      MessageInterface::ShowMessage
         ("SaveMission::Execute() '%s' should save mission to a file\n",
          GetGeneratingString(Gmat::NO_COMMENTS).c_str());
      MessageInterface::ShowMessage("   fileName='%s'\n", fname.c_str());
      //MessageInterface::ShowMessage(script);
      #endif
      
      std::ofstream dstream;
      dstream.open(std::filesystem::u8path(fname), std::ios::out | std::ios::trunc);
      if (!dstream.is_open())
         throw CommandException("SaveMission could not open output file '" + fname + "'.");
      dstream << script;
      dstream.flush();
      if (!dstream)
         throw CommandException("SaveMission could not write output file '" + fname + "'.");
      dstream.close();
      if (!dstream)
         throw CommandException("SaveMission could not close output file '" + fname + "'.");
   }
   
   BuildCommandSummary(true);
   return true;
}


//------------------------------------------------------------------------------
//  void InterpretAction()
//------------------------------------------------------------------------------
/**
 * Parses the command string and builds the corresponding command structures.
 *
 * The SaveMission command has the following syntax:
 *
 *     SaveMission 'file name'
 */
//------------------------------------------------------------------------------
bool SaveMission::InterpretAction()
{
   #ifdef DEBUG_SAVEMISSION_IA
   MessageInterface::ShowMessage
      ("SaveMission::InterpretAction(), generatingString = %s\n",
       generatingString.c_str());
   #endif
   
   StringArray chunks = InterpretPreface();
   
   #ifdef DEBUG_SAVEMISSION_IA
   for (UnsignedInt i=0; i<chunks.size(); i++)
      MessageInterface::ShowMessage("   %d: '%s'\n", i, chunks[i].c_str());
   #endif
   
   if (chunks.size() < 2)
      throw CommandException("Missing information for MissionSave command.\n");
   
   executionPolicy = "LegacyTerminal";
   executionPolicySpecified = false;
   const std::string arguments = GmatStringUtil::Trim(chunks[1]);
   std::string parsedName, remaining;
   if (!arguments.empty() && arguments[0] == '\'')
   {
      bool closed = false;
      for (size_t i = 1; i < arguments.size(); ++i)
      {
         if (arguments[i] != '\'')
            parsedName += arguments[i];
         else if (i + 1 < arguments.size() && arguments[i + 1] == '\'')
         {
            parsedName += '\'';
            ++i;
         }
         else
         {
            remaining = GmatStringUtil::Trim(arguments.substr(i + 1));
            closed = true;
            break;
         }
      }
      if (!closed)
         throw CommandException("SaveMission has an unterminated quoted filename.");
   }
   else
   {
      const auto brace = arguments.find('{');
      parsedName = GmatStringUtil::Trim(arguments.substr(0, brace));
      if (brace != std::string::npos) remaining = GmatStringUtil::Trim(arguments.substr(brace));
   }
   SetStringParameter(FILE_NAME, parsedName);
   if (!remaining.empty())
   {
      if (remaining.front() != '{' || remaining.back() != '}')
         throw CommandException("SaveMission options must be enclosed in braces.");
      const auto option = GmatStringUtil::Trim(remaining.substr(1, remaining.size() - 2));
      const auto equal = option.find('=');
      if (equal == std::string::npos || GmatStringUtil::Trim(option.substr(0, equal)) != "ExecutionPolicy")
         throw CommandException("SaveMission supports only the ExecutionPolicy option.");
      SetStringParameter(EXECUTION_POLICY, GmatStringUtil::Trim(option.substr(equal + 1)));
   }
   
   #ifdef DEBUG_SAVEMISSION_IA
   MessageInterface::ShowMessage("   fileName = '%s'\n", fileName.c_str());
   MessageInterface::ShowMessage
      ("SaveMission::InterpretAction() returning true\n");
   #endif
   
   return true;
}


//------------------------------------------------------------------------------
//  GmatBase* Clone(void) const
//------------------------------------------------------------------------------
/**
 * This method returns a clone of the SaveMission.
 *
 * @return clone of the SaveMission.
 */
//------------------------------------------------------------------------------
GmatBase* SaveMission::Clone() const
{
   return (new SaveMission(*this));
}


//------------------------------------------------------------------------------
//  bool RenameRefObject(const UnsignedInt type,
//                       const std::string &oldName, const std::string &newName)
//------------------------------------------------------------------------------
/**
 * @see GmatBase
 */
//------------------------------------------------------------------------------
bool SaveMission::RenameRefObject(const UnsignedInt type,
                                  const std::string &oldName,
                                  const std::string &newName)
{
   // There are no objects to be renamed here
   return true;
}


//------------------------------------------------------------------------------
// std::string GetParameterText(const Integer id) const
//------------------------------------------------------------------------------
std::string SaveMission::GetParameterText(const Integer id) const
{
   if (id >= GmatCommandParamCount && id < SaveMissionParamCount)
      return PARAMETER_TEXT[id - GmatCommandParamCount];
   else
      return GmatCommand::GetParameterText(id);
}


//------------------------------------------------------------------------------
// Integer GetParameterID(const std::string &str) const
//------------------------------------------------------------------------------
Integer SaveMission::GetParameterID(const std::string &str) const
{
   for (int i=GmatCommandParamCount; i<SaveMissionParamCount; i++)
   {
      if (str == PARAMETER_TEXT[i - GmatCommandParamCount])
         return i;
   }
   
   return GmatCommand::GetParameterID(str);
}


//------------------------------------------------------------------------------
// Gmat::ParameterType GetParameterType(const Integer id) const
//------------------------------------------------------------------------------
Gmat::ParameterType SaveMission::GetParameterType(const Integer id) const
{
   if (id >= GmatCommandParamCount && id < SaveMissionParamCount)
      return PARAMETER_TYPE[id - GmatCommandParamCount];
   else
      return GmatCommand::GetParameterType(id);
}


//------------------------------------------------------------------------------
// std::string GetParameterTypeString(const Integer id) const
//------------------------------------------------------------------------------
std::string SaveMission::GetParameterTypeString(const Integer id) const
{
   if (id >= GmatCommandParamCount && id < SaveMissionParamCount)
      return GmatBase::PARAM_TYPE_STRING[GetParameterType(id)];
   else
      return GmatCommand::GetParameterTypeString(id);
}


//------------------------------------------------------------------------------
// std::string GetStringParameter(const Integer id) const
//------------------------------------------------------------------------------
std::string SaveMission::GetStringParameter(const Integer id) const
{
   switch (id)
   {
   case FILE_NAME:
      return fileName;
   case EXECUTION_POLICY:
      return executionPolicy;
   default:
      return GmatCommand::GetStringParameter(id);
   }
}


//------------------------------------------------------------------------------
// std::string GetStringParameter(const std::string &label) const
//------------------------------------------------------------------------------
std::string SaveMission::GetStringParameter(const std::string &label) const
{
   return GetStringParameter(GetParameterID(label));
}


//------------------------------------------------------------------------------
// bool SetStringParameter(const Integer id, const std::string value)
//------------------------------------------------------------------------------
bool SaveMission::SetStringParameter(const Integer id, const std::string &value)
{
   switch (id)
   {
   case FILE_NAME:
      if (value.empty() || value.find('\0') != std::string::npos ||
          value.find_first_of("\r\n") != std::string::npos)
         throw CommandException("SaveMission requires a nonempty single-line filename.");
      fileName = value;
      return true;
   case EXECUTION_POLICY:
      if (value != "LegacyTerminal" && value != "Immediate")
         throw CommandException("SaveMission ExecutionPolicy must be LegacyTerminal or Immediate.");
      executionPolicy = value;
      executionPolicySpecified = true;
      return true;
   default:
      return GmatCommand::SetStringParameter(id, value);
   }
}


//------------------------------------------------------------------------------
// bool SetStringParameter(const std::string &label, const std::string &value)
//------------------------------------------------------------------------------
bool SaveMission::SetStringParameter(const std::string &label, const std::string &value)
{
   return SetStringParameter(GetParameterID(label), value);
}

const StringArray& SaveMission::GetPropertyEnumStrings(const Integer id) const
{
   if (id == EXECUTION_POLICY)
   {
      static const StringArray values = {"LegacyTerminal", "Immediate"};
      return values;
   }
   return GmatCommand::GetPropertyEnumStrings(id);
}


//------------------------------------------------------------------------------
// const std::string& GetGeneratingString(Gmat::WriteMode mode = Gmat::SCRIPTING,
//                    const std::string &prefix = "",
//                    const std::string &useName = "");
//------------------------------------------------------------------------------
const std::string& SaveMission::GetGeneratingString(Gmat::WriteMode mode,
                                                    const std::string &prefix,
                                                    const std::string &useName)
{
   // Build the local string
   generatingString = prefix + "SaveMission";
   std::string escapedName;
   for (char ch : fileName)
   {
      escapedName += ch;
      if (ch == '\'') escapedName += '\'';
   }
   generatingString += " '" + escapedName + "'";
   if (executionPolicy != "LegacyTerminal" || executionPolicySpecified)
      generatingString += " {ExecutionPolicy = " + executionPolicy + "}";
   generatingString += ";";
   
   // Then call the base class method for comments
   return GmatCommand::GetGeneratingString(mode, prefix, useName);
}

