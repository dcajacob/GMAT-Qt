//$Id$
//------------------------------------------------------------------------------
//                              GmatInterface
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
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under contract
// number S-67573-G
//
// Author: Linda Jun
// Created: 2004/8/30
//
/**
 * Implements class to provide sending scripts from MATLAB to GMAT.
 */
//------------------------------------------------------------------------------

#include "GmatInterface.hpp"
#include "Moderator.hpp"         // for Instance()
#include "MessageInterface.hpp"
#include "InterfaceException.hpp"
#include "MatlabCallbackBridge.hpp"
#include <algorithm>
#include <limits>
#include <atomic>
#include <cstring>

namespace
{
std::atomic<std::size_t> activeMatlabFunctions{0}, activeCallbackScopes{0};
std::atomic<bool> scriptControlActive{false};

void IncrementActivity(std::atomic<std::size_t> &activity)
{
   auto count = activity.load();
   do
   {
      if (count == std::numeric_limits<std::size_t>::max())
         throw InterfaceException("MATLAB interface activity depth is exhausted");
   } while (!activity.compare_exchange_weak(count, count + 1));
}
void RequireReplyName(const std::string &name)
{
   if (name.empty() || name.find('\0') != std::string::npos)
      throw InterfaceException("MATLAB reply name is empty or contains a NUL character");
}
char *BorrowLegacyReply(std::string &storage, std::string value)
{
   if (value.find('\0') != std::string::npos)
      throw InterfaceException("The legacy character reply cannot represent embedded NUL data; use the owned reply API");
   storage = std::move(value);
   return storage.data();
}

void RequireCallbackAllowed()
{
   if (activeMatlabFunctions.load())
      throw InterfaceException("A GMAT-called MATLAB function cannot invoke a GMAT callback");
}
struct ScriptControlScope
{
   ScriptControlScope()
   {
      bool expected = false;
      if (!scriptControlActive.compare_exchange_strong(expected, true))
         throw InterfaceException("Another MATLAB script-control operation is active");
      try
      {
         if (activeMatlabFunctions.load())
            throw InterfaceException("A GMAT-called MATLAB function cannot build, change or run a GMAT script");
         if (activeCallbackScopes.load() || Moderator::Instance()->GetRunState() != Gmat::IDLE)
            throw InterfaceException("MATLAB script control requires an idle GMAT mission and callback service");
      }
      catch (...) { scriptControlActive.store(false); throw; }
   }
   ~ScriptControlScope() { scriptControlActive.store(false); }
   ScriptControlScope(const ScriptControlScope&) = delete;
   ScriptControlScope& operator=(const ScriptControlScope&) = delete;
};
}

GmatInterface::MatlabFunctionScope::MatlabFunctionScope()
{
   IncrementActivity(activeMatlabFunctions);
}
GmatInterface::MatlabFunctionScope::~MatlabFunctionScope() noexcept
{
   activeMatlabFunctions.fetch_sub(1);
}
bool GmatInterface::CanAcceptMatlabCommand(const char *command) noexcept
{
   if (!command) return false;
   if (std::strcmp(command, "Close;") == 0) return true;
   if (activeMatlabFunctions.load()) return false;
   if (std::strcmp(command, "Callback;") == 0 || std::strncmp(command, "CallbackData", 12) == 0)
      return true;
   return !scriptControlActive.load() && activeCallbackScopes.load() == 0;
}

GmatInterface* GmatInterface::instance = NULL;
bool GmatInterface::mPassedInterpreter = false;

//#define DEBUG_GMAT_INTERFACE
//#define DEBUG_TEST_CALLBACK

//---------------------------------
// public methods
//---------------------------------

//------------------------------------------------------------------------------
// static Instance()
//------------------------------------------------------------------------------
GmatInterface* GmatInterface::Instance()
{
   if (instance == NULL)
      instance = new GmatInterface();
   return instance;
}


//------------------------------------------------------------------------------
// void OpenScript()
//------------------------------------------------------------------------------
void GmatInterface::OpenScript()
{
   ScriptControlScope control;
   if (!mInStringStream)
      mInStringStream = new std::istringstream;

}


//------------------------------------------------------------------------------
// void ClearScrip()
//------------------------------------------------------------------------------
void GmatInterface::ClearScript()
{
   ScriptControlScope control;
   auto *ui = Moderator::GetUiInterpreter();
   if (!ui) throw InterfaceException("Clearing a MATLAB-controlled project requires an active UI interpreter");
   mPassedInterpreter = false;
   mStringStream.str(""); mStringStream.clear();
   if (mInStringStream) mInStringStream->clear();
   ui->CloseCurrentProject();
}


//------------------------------------------------------------------------------
// void PutScript(char *str)
//------------------------------------------------------------------------------
/*
 * Appends script to a string stream.
 *
 * @param <str> string to append
 */
//------------------------------------------------------------------------------
void GmatInterface::PutScript(char *str)
{
   ScriptControlScope control;
   if (!str) throw InterfaceException("Cannot append a null MATLAB script fragment");
   mPassedInterpreter = false;
   mStringStream << std::string(str) << std::endl;
   #ifdef DEBUG_GMAT_INTERFACE
   MessageInterface::ShowMessage("GmatInterface::PutScript() str=%s\n", str);
   #endif
}


//------------------------------------------------------------------------------
// void BuildObject()
//------------------------------------------------------------------------------
/*
 * Clears resource and build new objects from a internal string stream.
 */
//------------------------------------------------------------------------------
void GmatInterface::BuildObject()
{
   ScriptControlScope control;
  
   Moderator *moderator = Moderator::Instance();
   mPassedInterpreter = false;
   if (!mInStringStream) mInStringStream = new std::istringstream;
   mInStringStream->clear(); mStringStream.clear(); mStringStream.seekg(0);
   std::streambuf *streamBuf = mStringStream.rdbuf();

   // redirect mInStringStream into mStringStream
   RedirectBuffer(mInStringStream, streamBuf);
   
   #ifdef DEBUG_GMAT_INTERFACE
   //loj: 8/31/04 Why this causes problem for long scripts? buffer overflow?
   //MessageInterface::ShowMessage
   //   ("GmatInterface::BuildObject() mStringStream.str=\n%s", mStringStream.str().c_str());
   //MessageInterface::ShowMessage
   //   ("GmatInterface::BuildObject() mInStringStream.str=\n%s\n", mInStringStream->str().c_str());
   #endif
   
   // flag to clear objects and mission sequence
   mPassedInterpreter = moderator->InterpretScript(mInStringStream, true);
   if (auto *ui = Moderator::GetUiInterpreter()) ui->UpdateView(3);
   
   // empty the buffer, once objects are created
   mStringStream.str("");
}


//------------------------------------------------------------------------------
// void UpdateObject()
//------------------------------------------------------------------------------
/*
 * Build and updates objects from a internal string stream without clearing the
 * resource.
 */
//------------------------------------------------------------------------------
void GmatInterface::UpdateObject()
{
   ScriptControlScope control;
  
   Moderator *moderator = Moderator::Instance();
   mPassedInterpreter = false;
   if (!mInStringStream) mInStringStream = new std::istringstream;
   mInStringStream->clear(); mStringStream.clear(); mStringStream.seekg(0);

   std::streambuf *streamBuf = mStringStream.rdbuf();

   // redirect mInStringStream into mStringStream
   RedirectBuffer(mInStringStream, streamBuf);
   
   #ifdef DEBUG_GMAT_INTERFACE
   //loj: 8/31/04 Why this causes problem for long scripts? buffer overflow?
   //MessageInterface::ShowMessage
   //   ("GmatInterface::UpdateObject() mStringStream.str=\n%s", mStringStream.str().c_str());
   //MessageInterface::ShowMessage
   //   ("GmatInterface::UpdateObject() mInStringStream.str=\n%s\n", mInStringStream->str().c_str());
   #endif
   
   // flag not to clear objects and mission sequence
   mPassedInterpreter = moderator->InterpretScript(mInStringStream, false);
   if (auto *ui = Moderator::GetUiInterpreter()) ui->UpdateView(3);
   
   // empty the buffer, once objects are created
   mStringStream.str("");
}


//------------------------------------------------------------------------------
// void RunScript()
//------------------------------------------------------------------------------
/*
 * Executues commands from existing objects.
 */
//------------------------------------------------------------------------------
void GmatInterface::RunScript()
{
   ScriptControlScope control;
   #ifdef DEBUG_GMAT_INTERFACE
   MessageInterface::ShowMessage
      ("GmatInterface::RunScript() entered. mPassedInterpreter=%d\n",
       mPassedInterpreter);
   #endif

   if (!mPassedInterpreter)
      throw InterfaceException("No successfully built MATLAB-controlled GMAT script is available");
   Moderator::Instance()->RunScript();

   if (auto *ui = Moderator::GetUiInterpreter()) ui->UpdateView(4);
}


//------------------------------------------------------------------------------
// bool ExecuteCallback()
//------------------------------------------------------------------------------
void GmatInterface::ValidateCallbackObject(GmatBase *callbackObject)
{
   if (!callbackObject || callbackObject->IsOfType(Gmat::FUNCTION))
      throw InterfaceException("Optimizer callbacks require a live command or service, not a Function resource");
}

GmatInterface::CallbackScope::CallbackScope()
   : CallbackScope(GmatInterface::Instance()->GetLiveCallback())
{
}

GmatInterface::CallbackScope::CallbackScope(GmatBase *callbackObject)
{
   if (!callbackObject) return;
   GmatInterface::ValidateCallbackObject(callbackObject);
   owner = GmatInterface::Instance();
   if (owner->nextCallbackRegistration == std::numeric_limits<std::size_t>::max())
      throw InterfaceException("Optimizer callback scope identity is exhausted");
   registration = ++owner->nextCallbackRegistration;
   IncrementActivity(activeCallbackScopes);
   try { owner->callbackScopes.push_back({callbackObject, callbackObject->GetLifetimeToken(), registration}); }
   catch (...) { activeCallbackScopes.fetch_sub(1); throw; }
}

GmatInterface::CallbackScope::~CallbackScope() noexcept
{
   if (owner && registration)
   {
      owner->ReleaseCallbackScope(registration);
      activeCallbackScopes.fetch_sub(1);
   }
}

GmatBase *GmatInterface::CallbackScope::GetRetainedCallback() const
{
   if (!owner || !registration)
      throw InterfaceException("No live optimizer callback is selected");
   const auto found = std::find_if(owner->callbackScopes.begin(), owner->callbackScopes.end(),
         [&](const ScopedCallback &value) { return value.registration == registration; });
   if (found == owner->callbackScopes.end() || !GmatBase::IsLiveReference(found->object, found->identity))
      throw InterfaceException("The selected optimizer callback registration has ended");
   return found->object;
}

void GmatInterface::CallbackScope::ValidateRetention() const
{
   GetRetainedCallback();
}

void GmatInterface::CallbackScope::ValidateSelection() const
{
   auto *target = GetRetainedCallback();
   if (owner->GetLiveCallback() != target)
      throw InterfaceException("The selected optimizer callback has changed");
}

void GmatInterface::ReleaseCallbackScope(std::size_t registration) noexcept
{
   callbackScopes.erase(std::remove_if(callbackScopes.begin(), callbackScopes.end(),
         [=](const ScopedCallback &value) { return value.registration == registration; }), callbackScopes.end());
}

GmatBase *GmatInterface::GetLiveCallback()
{
   while (!callbackScopes.empty())
   {
      const auto &value = callbackScopes.back();
      if (GmatBase::IsLiveReference(value.object, value.identity)) return value.object;
      callbackScopes.pop_back();
   }
   if (callbackObj && GmatBase::IsLiveReference(callbackObj, callbackIdentity)) return callbackObj;
   callbackObj = nullptr; callbackIdentity.reset();
   return nullptr;
}

bool GmatInterface::ExecuteCallback()
{
   RequireCallbackAllowed();
   // Check periodically for user interrupt (LOJ: 2014.09.08)
   static Integer checkCount = 0;
   static Integer interruptCheckFrequency = 2;
   
   #ifdef DEBUG_TEST_CALLBACK
      MessageInterface::ShowMessage("GmatInterface::ExecuteCallback being called ...\n");
   #endif
   auto *callback = GetLiveCallback();
   if (callback)
   {
      if (callback->IsCallbackExecuting())
         throw InterfaceException("The selected optimizer callback is already executing");
      CallbackScope active(callback);
      ++checkCount;
      if (checkCount == interruptCheckFrequency)
      {
         // Request UI to set input focus
         CheckUserInterrupt();
         checkCount = 0;
      }
      active.ValidateSelection();
      RequireCallbackAllowed();
      if (callback->IsCallbackExecuting())
         throw InterfaceException("The selected optimizer callback is already executing");
      const bool result = callback->ExecuteCallback();
      active.ValidateSelection();
      return result;
   }
   else
   {
      MessageInterface::ShowMessage("call back object is NULL, so returning false\n");
      return false;
   }
}


//------------------------------------------------------------------------------
// bool RegisterCallbackServer(GmatBase *callbackObject)
//------------------------------------------------------------------------------
bool GmatInterface::RegisterCallbackServer(GmatBase *callbackObject)
{
   ValidateCallbackObject(callbackObject);
   #ifdef DEBUG_TEST_CALLBACK
      MessageInterface::ShowMessage(
      "GmatInterface::RegisterCallbackServer being called with object %s \"%s\"...\n",
      callbackObject->GetTypeName().c_str(), callbackObject->GetName().c_str());
   #endif
   callbackObj = callbackObject;
   callbackIdentity = callbackObject->GetLifetimeToken();
   return true;
}

bool GmatInterface::PublishCallbackIteration(const std::string &data, bool validateOnly)
{
   RequireCallbackAllowed();
   auto *callback = GetLiveCallback();
   if (!callback) return false;
   CallbackScope active(callback);
   const bool result = callback->TakeAction(validateOnly ?
      "ValidateMatlabOptimizerIteration" : "MatlabOptimizerIteration", data);
   active.ValidateSelection();
   return result;
}

void GmatInterface::UnregisterCallbackServer(GmatBase *callbackObject)
{
   if (callbackObj == callbackObject) { callbackObj = nullptr; callbackIdentity.reset(); }
   callbackScopes.erase(std::remove_if(callbackScopes.begin(), callbackScopes.end(),
         [=](const ScopedCallback &value) { return value.object == callbackObject; }), callbackScopes.end());
}


//------------------------------------------------------------------------------
// char* GetCallbackStatus()
//------------------------------------------------------------------------------
/*
 * @return the status of the callback execution ("Executing", "Completed").
 */
//------------------------------------------------------------------------------
char* GmatInterface::GetCallbackStatus()
{
   auto *callback = GetLiveCallback();
   #ifdef DEBUG_TEST_CALLBACK
      MessageInterface::ShowMessage(
      "GmatInterface::GetCallbackStatus being called ...\n");
   #endif
   static char dataString[MAX_PARAM_VAL_STRING];
   static const char *executingStr = "Executing\0";
   static const char *completedStr = "Completed\0";
   if (!callback) // not running a callback - why are you asking?
   {
      sprintf(dataString, "%s", completedStr);
   }
   else
   {
      if (callback->IsCallbackExecuting())
         sprintf(dataString, "%s", executingStr);
      else
         sprintf(dataString, "%s", completedStr);
   }
   #ifdef DEBUG_TEST_CALLBACK
   MessageInterface::ShowMessage
      ("GmatInterface::GetCallbackStatus() dataString=<%s>\n", dataString);
   #endif
   return dataString;
}

//------------------------------------------------------------------------------
// void PutCallbackData(std::string &data)
//------------------------------------------------------------------------------
void  GmatInterface::PutCallbackData(std::string &data)
{
   RequireCallbackAllowed();
   auto *callback = GetLiveCallback();
   #ifdef DEBUG_TEST_CALLBACK
      MessageInterface::ShowMessage(
      "GmatInterface::PutCallbackData being called with data = %s\n", data.c_str());
   #endif
   if (callback && callback->IsCallbackExecuting())
      throw InterfaceException("Cannot replace inputs while the selected optimizer callback is executing");
   if (callback)
   {
      CallbackScope active(callback);
      if (!(callback->PutCallbackData(data)))
         throw InterfaceException(
         "GmatInterface::Error setting callback data on callback server");
      active.ValidateSelection();
   }
}

//------------------------------------------------------------------------------
// void PutCallbackRealData(RealArray &data)
//------------------------------------------------------------------------------
void GmatInterface::PutCallbackRealData(RealArray &data)
{
   RequireCallbackAllowed();
   auto *callback = GetLiveCallback();
   #ifdef DEBUG_TEST_CALLBACK
   MessageInterface::ShowMessage
      ("GmatInterface::PutCallbackRealData being called with data[0] = %f\n", data[0]);
   #endif
   if (callback && callback->IsCallbackExecuting())
      throw InterfaceException("Cannot replace inputs while the selected optimizer callback is executing");
   if (callback)
   {
      CallbackScope active(callback);
      if (!(callback->PutCallbackRealData(data)))
         throw InterfaceException(
         "GmatInterface::Error setting callback real data on callback server");
      active.ValidateSelection();
   }
}

//------------------------------------------------------------------------------
// char* GetCallbackResults()
//------------------------------------------------------------------------------
/*
 * @return the status of the callback execution ("Executing", "Completed").
 */
//------------------------------------------------------------------------------
char* GmatInterface::GetCallbackResults()
{
   static thread_local std::string reply;
   reply.clear();
   if (!GetLiveCallback()) return BorrowLegacyReply(reply, "ERROR!!");
   return BorrowLegacyReply(reply, GetCallbackResultString());
}

std::string GmatInterface::GetCallbackResultString()
{
   auto *callback = GetLiveCallback();
   if (!callback)
      throw InterfaceException("No active GMAT callback server is registered");
   CallbackScope active(callback);
   auto result = callback->GetCallbackResults();
   active.ValidateSelection();
   return result;
}

//------------------------------------------------------------------------------
// char* GetRunState()
//------------------------------------------------------------------------------
/*
 * @return the state of system ("RUNNING", "PAUSED", "IDLE").
 */
//------------------------------------------------------------------------------
char* GmatInterface::GetRunState()
{
   static char dataString[MAX_PARAM_VAL_STRING];
   static const char *runningStr = "Running\0";
   static const char *pausedStr = "Paused\0";
   static const char *idleStr = "Idle\0";
   strcpy(dataString, idleStr);
   
   Gmat::RunState state = Moderator::Instance()->GetRunState();
   
   if (state == Gmat::RUNNING)
      sprintf(dataString, "%s", runningStr);
   else if (state == Gmat::PAUSED)
      sprintf(dataString, "%s", pausedStr);
   else if (state == Gmat::IDLE)
      sprintf(dataString, "%s", idleStr);
   else
      sprintf(dataString, "Unknown");
   
   #ifdef DEBUG_GMAT_INTERFACE
   MessageInterface::ShowMessage
      ("GmatInterface::GetRunState() state=%d, dataString=<%s>\n", state,
       dataString);
   #endif
   
   return dataString;
}


//------------------------------------------------------------------------------
// char* GetParameter(const std::string &name)
//------------------------------------------------------------------------------
/*
 * It retrieves a Parameter pointer from the Sandbox, if it is not found in the
 * Sandbox, it retrieves it from the Configuration.
 *
 * @return string value of the parameter in the Sandbox or in the Configuration.
 */
//------------------------------------------------------------------------------
char* GmatInterface::GetParameter(const std::string &name)
{
   static thread_local std::string reply;
   reply.clear();
   return BorrowLegacyReply(reply, GetParameterString(name));
}

std::string GmatInterface::GetParameterString(const std::string &name)
{
   RequireReplyName(name);
   auto *moderator = Moderator::Instance();
   GmatBase *object = nullptr;
   try { object = moderator->GetInternalObject(name); }
   catch (BaseException &)
   {
      MessageInterface::ShowMessage("*** WARNING *** Could not find '%s' in the Sandbox. Trying Configuration...\n", name.c_str());
   }
   // An actual non-Parameter shadows any configured object of the same name.
   Parameter *parameter = object ? dynamic_cast<Parameter*>(object) : moderator->GetParameter(name);
   if (!parameter)
   {
      MessageInterface::ShowMessage("*** WARNING *** Could not find Parameter '%s' in the current scope\n", name.c_str());
      return "-123456789.123456789";
   }
   return "[" + parameter->ToString() + "]";
}


char* GmatInterface::GetGmatObject(const char *name)
{
   if (!name) throw InterfaceException("Cannot retrieve a MATLAB reply for a null object name");
   return GetGmatObject(std::string(name));
}


//------------------------------------------------------------------------------
// char* GetGmatObject(const std::string &name)
//------------------------------------------------------------------------------
/*
 * It retrieves an object pointer from the Sandbox, if it is not found in the
 * Sandbox, it retrieves it from the Configuration.
 *
 * @return serialized string value of the internal object in the Sandbox or
 *         object in the Configuration
 */
//------------------------------------------------------------------------------
char* GmatInterface::GetGmatObject(const std::string &name)
{
   static thread_local std::string reply;
   reply.clear();
   return BorrowLegacyReply(reply, GetGmatObjectString(name));
}

std::string GmatInterface::GetGmatObjectString(const std::string &name)
{
   RequireReplyName(name);
   auto *moderator = Moderator::Instance();
   GmatBase *object = nullptr;
   try { object = moderator->GetInternalObject(name); }
   catch (BaseException &)
   {
      MessageInterface::ShowMessage("*** WARNING *** Could not find '%s' in the Sandbox. Trying Configuration...\n", name.c_str());
   }
   if (!object) object = moderator->GetConfiguredObject(name);
   if (!object)
   {
      MessageInterface::ShowMessage("*** WARNING *** Could not find '%s' in the Configuration\n", name.c_str());
      return "-123456789.123456789";
   }
   return object->GetGeneratingString(Gmat::MATLAB_STRUCT);
}


//------------------------------------------------------------------------------
// void CheckUserInterrupt()
//------------------------------------------------------------------------------
/*
 * Calls Moderator::SetUserInputFocus() to check if user interrupted the
 * mission sequence.
 */
//------------------------------------------------------------------------------
void GmatInterface::CheckUserInterrupt()
{
   Moderator::Instance()->GetUserInterrupt();
}


//---------------------------------
// private methods
//---------------------------------

//------------------------------------------------------------------------------
// GmatInterface()
//------------------------------------------------------------------------------
GmatInterface::GmatInterface()
{
   mInStringStream = NULL;
   callbackObj     = NULL;
}


//------------------------------------------------------------------------------
// GmatInterface()
//------------------------------------------------------------------------------
GmatInterface::~GmatInterface()
{
   if (mInStringStream)
      delete mInStringStream;
}
  

namespace
{
struct MatlabCallbackContext
{
   std::size_t identity;
   MatlabCallbackBridge::Executor executor;
   std::shared_ptr<void> localState;
};
thread_local std::vector<MatlabCallbackContext> matlabCallbackContexts;
thread_local std::size_t nextMatlabCallbackContext = 0;
}

MatlabCallbackBridge::Scope::Scope() : Scope(Executor{}) {}

MatlabCallbackBridge::Scope::Scope(Executor executor)
{
   if (nextMatlabCallbackContext == std::numeric_limits<std::size_t>::max())
      throw InterfaceException("MATLAB callback bridge identities are exhausted");
   identity = ++nextMatlabCallbackContext;
   matlabCallbackContexts.push_back({identity, std::move(executor), {}});
}

MatlabCallbackBridge::Scope::~Scope() noexcept
{
   matlabCallbackContexts.erase(std::remove_if(matlabCallbackContexts.begin(), matlabCallbackContexts.end(),
         [&](const MatlabCallbackContext &value) { return value.identity == identity; }), matlabCallbackContexts.end());
}

bool MatlabCallbackBridge::IsActive()
{
   return !matlabCallbackContexts.empty() && bool(matlabCallbackContexts.back().executor);
}

MatlabCallbackReply MatlabCallbackBridge::Execute(const MatlabCallbackRequest &request)
{
   if (!IsActive()) throw InterfaceException("No waiting MATLAB callback client is available");
   const auto identity = matlabCallbackContexts.back().identity;
   // A nested callback can grow the context vector while the transport waits.
   auto execute = matlabCallbackContexts.back().executor;
   MatlabCallbackReply result;
   try { result = execute(request); }
   catch (BaseException &) { throw; }
   catch (const std::exception &error) { throw InterfaceException(error.what()); }
   if (!IsActive() || matlabCallbackContexts.back().identity != identity)
      throw InterfaceException("The MATLAB callback bridge scope has changed");
   return result;
}

std::shared_ptr<void> &MatlabCallbackBridge::LocalState()
{
   if (!IsActive()) throw InterfaceException("No MATLAB callback-local state is available");
   return matlabCallbackContexts.back().localState;
}
