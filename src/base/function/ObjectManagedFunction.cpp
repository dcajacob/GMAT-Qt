//$Id$
//------------------------------------------------------------------------------
//                             ObjectManagedFunction
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
// number NNG04CC06P.
//
// Author: Linda Jun
// Created: 2016.10.20
//
/**
 * Implements the ObjectManagedFunction base class for BuiltinGmatFunction and
 * UserDefinedFunction
 */
//------------------------------------------------------------------------------

#include "ObjectManagedFunction.hpp"
#include "ExpressionBinding.hpp"
#include "FunctionException.hpp"    // for exception
#include "StringUtil.hpp"           // for GmatStringUtil::
#include "Parameter.hpp"            // for GetOwner()
#include "MessageInterface.hpp"
#include "SolarSystem.hpp"
#include <algorithm>
#include <mutex>

namespace {
struct GlobalStoreView {
   ObjectMap *store = nullptr;
   std::function<bool()> isLive;
   std::weak_ptr<const void> function;
   const SolarSystem *solarSystem = nullptr;
   std::weak_ptr<const void> solarSystemIdentity;
};
struct GlobalViewRegistry {
   std::mutex mutex;
   std::map<const ObjectManagedFunction*, GlobalStoreView> entries;
};
GlobalViewRegistry& GlobalViews() {
   static auto *registry = new GlobalViewRegistry;
   return *registry;
}
void ForgetGlobalView(const ObjectManagedFunction *function) noexcept {
   try {
      auto &registry = GlobalViews();
      std::lock_guard<std::mutex> lock(registry.mutex);
      registry.entries.erase(function);
   } catch (...) {} // Expired function identities still reject reused addresses.
}
void RememberGlobalView(ObjectManagedFunction *function, ObjectMap *store,
      std::function<bool()> isLive, const SolarSystem *system = nullptr) {
   if (!store || !isLive || !isLive()) { ForgetGlobalView(function); return; }
   GlobalStoreView view{store, std::move(isLive), function->GetLifetimeToken()};
   view.solarSystem = system;
   if (view.solarSystem)
      view.solarSystemIdentity = view.solarSystem->GetLifetimeToken();
   auto &registry = GlobalViews();
   std::lock_guard<std::mutex> lock(registry.mutex);
   registry.entries[function] = std::move(view);
}
GlobalStoreView FindGlobalView(const ObjectManagedFunction *function) {
   auto &registry = GlobalViews();
   std::lock_guard<std::mutex> lock(registry.mutex);
   const auto found = registry.entries.find(function);
   return found == registry.entries.end() ? GlobalStoreView{} : found->second;
}
void RestoreGlobalView(const ObjectManagedFunction *function, const GlobalStoreView &view) {
   auto &registry = GlobalViews();
   std::lock_guard<std::mutex> lock(registry.mutex);
   registry.entries[function] = view;
}
}

//#define DEBUG_FUNCTION_SET
//#define DEBUG_FUNCTION_IN_OUT
//#define DEBUG_WRAPPER_CODE
//#define DEBUG_FUNCTION_OBJ
//#define DEBUG_OBJECT_MAP


//#ifndef DEBUG_MEMORY
//#define DEBUG_MEMORY
//#endif

#ifdef DEBUG_MEMORY
#include "MemoryTracker.hpp"
#endif


//---------------------------------
// static data
//---------------------------------


//------------------------------------------------------------------------------
//  ObjectManagedFunction(std::string typeStr, std::string name)
//------------------------------------------------------------------------------
/**
 * Constructs the ObjectManagedFunction object (default constructor).
 * 
 * @param <typeStr> String text identifying the object type
 * @param <name>   Name for the object
 */
//------------------------------------------------------------------------------
ObjectManagedFunction::ObjectManagedFunction(const std::string &typeStr, const std::string &name) :
   Function           (typeStr, name),
   objectStore        (NULL),
   globalObjectStore  (NULL),
   changingInputType  (false)
{
   if (typeStr != "")
      objectTypeNames.push_back(typeStr);
   objectTypeNames.push_back("ObjectManagedFunction");
}


//------------------------------------------------------------------------------
//  ~ObjectManagedFunction(void)
//------------------------------------------------------------------------------
/**
 * Destroys the ObjectManagedFunction object (destructor).
 */
//------------------------------------------------------------------------------
ObjectManagedFunction::~ObjectManagedFunction()
{
   ForgetGlobalView(this);
   RetireInvocationInterfaces();
}


//------------------------------------------------------------------------------
//  ObjectManagedFunction(const ObjectManagedFunction &f)
//------------------------------------------------------------------------------
/**
 * Constructs the ObjectManagedFunction object (copy constructor).
 * 
 * @param <f> Object that is copied
 */
//------------------------------------------------------------------------------
ObjectManagedFunction::ObjectManagedFunction(const ObjectManagedFunction &f) :
   Function           (f),
   objectStore        (NULL),
   globalObjectStore  (NULL),
   changingInputType  (f.changingInputType)
{
   for (const auto &entry : f.inputArgMap)
      inputArgMap[entry.first] = NULL;
   for (const auto &entry : f.outputArgMap)
      outputArgMap[entry.first] = NULL;
}


//------------------------------------------------------------------------------
//  ObjectManagedFunction& operator=(const ObjectManagedFunction &f)
//------------------------------------------------------------------------------
/**
 * Sets one ObjectManagedFunction object to match another (assignment operator).
 * 
 * @param <f> The object that is copied.
 * 
 * @return this object, with the parameters set as needed.
 */
//------------------------------------------------------------------------------
ObjectManagedFunction& ObjectManagedFunction::operator=(const ObjectManagedFunction &f)
{
   if (this == &f)
      return *this;

   const auto previousBindings = objectBindings;
   
   RetireInvocationInterfaces();
   Function::operator=(f);
   
   objectStore        = NULL;
   globalObjectStore  = NULL;
   ForgetGlobalView(this);
   objectBindings = previousBindings && previousBindings->OwnsMap(f.globalObjectStore) ?
         previousBindings : f.objectBindings;
   // Input wrappers remain borrowed from the caller. Copies retain formal
   // names without borrowing another invocation's bindings or output queue.
   ClearInOutArgMaps(false, false);
   wrappersToDelete.clear();
   for (const auto &entry : f.inputArgMap)
      inputArgMap[entry.first] = NULL;
   for (const auto &entry : f.outputArgMap)
      outputArgMap[entry.first] = NULL;
   changingInputType  = f.changingInputType;
   return *this;
}


std::unique_ptr<FunctionInvocationState> ObjectManagedFunction::SuspendInvocationState()
{
   struct Wrappers
   {
      WrapperMap inputs, outputs;
      WrapperArray queued, owned;
      std::vector<ElementWrapper::InvocationLease> retained;
      std::map<ElementWrapper*, ElementWrapper::InvocationLease> identities;
      bool armed = false;
      bool invalid = false;
   };
   auto saved = std::make_shared<Wrappers>();
   saved->inputs = inputArgMap;
   saved->outputs = outputArgMap;
   saved->retained = invocationWrapperLeases;
   auto retain = [&](ElementWrapper *wrapper)
   {
      if (!wrapper || saved->identities.count(wrapper)) return;
      ElementWrapper::InvocationLease lease;
      // Prefer the live generation if this address was legitimately reused.
      for (const auto &current : invocationWrapperLeases)
         if (ElementWrapper::GetInvocationWrapper(current) == wrapper)
         { lease = current; break; }
      if (!lease)
         for (const auto &current : invocationWrapperLeases)
            if (ElementWrapper::MatchesInvocationWrapper(current, wrapper))
            { lease = current; break; }
      if (!lease) lease = ElementWrapper::RetainInvocationWrapper(wrapper);
      saved->identities.emplace(wrapper, lease);
      if (std::find(saved->retained.begin(), saved->retained.end(), lease) == saved->retained.end())
         saved->retained.push_back(std::move(lease));
   };
   for (const auto &entry : inputArgMap) retain(entry.second);
   auto retainOutput = [&](ElementWrapper *wrapper)
   {
      retain(wrapper);
      const bool input = std::any_of(inputArgMap.begin(), inputArgMap.end(),
            [wrapper](const auto &entry) { return entry.second == wrapper; });
      if (wrapper && !input && std::find(saved->owned.begin(), saved->owned.end(), wrapper) == saved->owned.end())
         saved->owned.push_back(wrapper);
   };
   for (const auto &entry : outputArgMap) retainOutput(entry.second);
   for (auto *wrapper : wrappersToDelete) retainOutput(wrapper);
   // Stage cold interfaces with the same formal keys before detaching anything.
   for (auto &entry : saved->inputs) entry.second = nullptr;
   for (auto &entry : saved->outputs) entry.second = nullptr;

   auto *local = objectStore;
   auto *global = globalObjectStore;
   auto localIsLive = CaptureObjectStoreLifetime(local);
   auto globalIsLive = CaptureObjectStoreLifetime(global);
   const auto globalView = FindGlobalView(this);
   auto state = CaptureInvocationFields(Function::SuspendInvocationState(),
         changingInputType, objectBindings);
   auto restoreViews = [this, local, global, localIsLive, globalIsLive, globalView]()
   {
      const bool localAlive = !localIsLive || localIsLive();
      const bool globalAlive = !globalIsLive || globalIsLive();
      objectStore = localAlive ? local : nullptr;
      globalObjectStore = globalAlive ? global : nullptr;
      if (localAlive && globalAlive && GmatBase::IsLiveReference(this, globalView.function))
         RestoreGlobalView(this, globalView);
      else
         ForgetGlobalView(this);
      if (!localAlive || !globalAlive)
      {
         isInitialized = false;
         throw FunctionException("An enclosing function object store was retired.\n");
      }
   };
   // Validate after the base initialization flag is restored, so a retired
   // store cannot become a ready invocation again. Do not revive stale maps.
   state = std::make_unique<FunctionInvocationState>(*this, std::move(restoreViews),
         std::move(state), true);
   auto restoreWrappers = [this, saved]()
   {
      if (!saved->armed) return;
      RetireInvocationInterfaces();
      auto validate = [&](ElementWrapper *&wrapper)
      {
         if (!wrapper) return;
         const auto found = saved->identities.find(wrapper);
         if (found == saved->identities.end() ||
               ElementWrapper::GetInvocationWrapper(found->second) != wrapper)
         {
            wrapper = nullptr;
            saved->invalid = true;
         }
      };
      for (auto &entry : saved->inputs) validate(entry.second);
      for (auto &entry : saved->outputs) validate(entry.second);
      for (auto &wrapper : saved->queued) validate(wrapper);
      inputArgMap.swap(saved->inputs);
      outputArgMap.swap(saved->outputs);
      wrappersToDelete.swap(saved->queued);
      invocationWrapperLeases.swap(saved->retained);
      PruneInvocationWrapperLeases();
   };
   state = std::make_unique<FunctionInvocationState>(*this, std::move(restoreWrappers),
         std::move(state));
   auto validateRestoration = [this, saved]()
   {
      if (saved->invalid)
      {
         isInitialized = false;
         throw FunctionException("An enclosing function argument or result wrapper was retired.\n");
      }
   };
   auto result = std::make_unique<FunctionInvocationState>(*this,
         std::move(validateRestoration), std::move(state), true);
   // Ownership transfer is nonthrowing and occurs after all staging succeeds.
   for (auto *wrapper : saved->owned)
   {
      const auto &lease = saved->identities.at(wrapper);
      if (ElementWrapper::GetInvocationWrapper(lease) == wrapper)
         ElementWrapper::ReleaseOwnedWrapper(wrapper);
      else
         saved->invalid = true;
   }
   inputArgMap.swap(saved->inputs);
   outputArgMap.swap(saved->outputs);
   wrappersToDelete.swap(saved->queued);
   invocationWrapperLeases.clear();
   saved->armed = true;
   // A nested activation creates its own formals/local resources instead of
   // borrowing and overwriting the enclosing call's installed dictionary.
   objectStore = nullptr;
   globalObjectStore = nullptr;
   return result;
}

void ObjectManagedFunction::PruneInvocationWrapperLeases() noexcept
{
   auto unused = [this](const ElementWrapper::InvocationLease &lease)
   {
      auto *wrapper = ElementWrapper::GetInvocationWrapper(lease);
      if (!wrapper) return true;
      for (const auto &entry : inputArgMap) if (entry.second == wrapper) return false;
      for (const auto &entry : outputArgMap) if (entry.second == wrapper) return false;
      return std::find(wrappersToDelete.begin(), wrappersToDelete.end(), wrapper) == wrappersToDelete.end();
   };
   invocationWrapperLeases.erase(std::remove_if(invocationWrapperLeases.begin(),
         invocationWrapperLeases.end(), unused), invocationWrapperLeases.end());
}

void ObjectManagedFunction::RetireInvocationInterfaces() noexcept
{
   ClearOutputWrappers();
   for (auto &entry : inputArgMap) entry.second = nullptr;
   for (auto &entry : outputArgMap) entry.second = nullptr;
   invocationWrapperLeases.clear();
}

void ObjectManagedFunction::RetireArgumentWrappers(const WrapperMap &inputs,
      const WrapperArray &outputs) noexcept
{
   auto retiring = [&](ElementWrapper *wrapper)
   {
      if (!wrapper) return false;
      for (const auto &entry : inputs) if (entry.second == wrapper) return true;
      for (auto *entry : outputs) if (entry == wrapper) return true;
      return false;
   };
   // Release actual borrowed interfaces, including output aliases, without
   // deleting wrappers owned by the retiring manager or touching another one.
   for (auto &entry : inputArgMap) if (retiring(entry.second)) entry.second = NULL;
   for (auto &entry : outputArgMap) if (retiring(entry.second)) entry.second = NULL;
   for (auto &entry : wrappersToDelete) if (retiring(entry)) entry = NULL;
   PruneInvocationWrapperLeases();
}

void ObjectManagedFunction::RetireFunctionObjectStore(ObjectMap *store) noexcept
{
   if (!store || objectStore != store) return;
   // Keep only an owner-tracked observation of the actual global binding.
   // Invocation resources and wrappers are still retired below.
   try { RememberGlobalView(this, globalObjectStore, CaptureObjectStoreLifetime(globalObjectStore), solarSys); }
   catch (...) { ForgetGlobalView(this); }
   // The manager is retiring this actual invocation, not a same-name caller or
   // a store currently owned by another manager of the shared function.
   RetireInvocationInterfaces();
   objectStore = nullptr;
   globalObjectStore = nullptr;
   solarSys = nullptr;
   internalCoordSys = nullptr;
   forces = nullptr;
   changingInputType = false;
   isInitialized = false;
   functionBindings.reset();
   objectBindings.reset();
}

void ObjectManagedFunction::RebindObjectManagedFunctionValue(const ObjectManagedFunction &source,
      const std::function<GmatBase*(GmatBase*)> &resolve)
{
   if (objectStore || !wrappersToDelete.empty())
      throw FunctionException("Retained Function binding requires a cold invocation interface.\n");
   for (const auto &entry : inputArgMap)
      if (entry.second) throw FunctionException("Copied Function still has bound invocation inputs.\n");
   for (const auto &entry : outputArgMap)
      if (entry.second) throw FunctionException("Copied Function still has bound invocation outputs.\n");
   const auto previousBindings = objectBindings;
   ExpressionBindingScope binding;
   RebindFunctionEnvironment(source, resolve);
   const bool valueGlobal = source.globalObjectStore == source.objectStore ||
         (source.objectBindings && source.objectBindings->OwnsMap(source.globalObjectStore)) ||
         (previousBindings && previousBindings->OwnsMap(source.globalObjectStore));
   // Global commands use the caller's native shared store. Already value-owned
   // environments (including local/global aliases) preserve independent storage.
   globalObjectStore = valueGlobal ? binding.scopes->CopyMap(source.globalObjectStore, resolve) :
         source.globalObjectStore;
   objectBindings = binding.scopes;
   changingInputType = false;
}

//------------------------------------------------------------------------------
// bool Initialize(ObjectInitializer *objInit, bool reinitialize = false)
//------------------------------------------------------------------------------
bool ObjectManagedFunction::Initialize(ObjectInitializer *objInit,
                                       bool reinitialize,
                                       bool resetLocalObjects)
{
   Function::Initialize(objInit, reinitialize);

   return true;
}


//------------------------------------------------------------------------------
// bool ObjectManagedFunction::Execute(ObjectInitializer *objInit)  [default implementation]
//------------------------------------------------------------------------------
bool ObjectManagedFunction::Execute(ObjectInitializer *objInit, bool reinitialize)
{
   return true;
}


//------------------------------------------------------------------------------
// bool ObjectManagedFunction::Finalize(bool cleanUp = false)  [default implementation]
//------------------------------------------------------------------------------
void ObjectManagedFunction::Finalize(bool cleanUp)
{
   // Anything to do here?
}

//------------------------------------------------------------------------------
// void SetFunctionObjectMap(ObjectMap *objMap)
//------------------------------------------------------------------------------
void ObjectManagedFunction::SetFunctionObjectMap(ObjectMap *objMap)
{
   #ifdef DEBUG_OBJECT_MAP
   ShowObjectMap(objMap, "In ObjectManagedFunction::SetFunctionObjectMap",
                 "Input Object Map");
   #endif
   objectStore = objMap;
}

//------------------------------------------------------------------------------
// ObjectMap* SetFunctionObjectMap()
//------------------------------------------------------------------------------
ObjectMap* ObjectManagedFunction::GetFunctionObjectStore()
{
   return objectStore;
}

const ObjectMap* ObjectManagedFunction::GetGlobalObjectStore() const
{
   const auto view = FindGlobalView(this);
   const bool validFunction = GmatBase::IsLiveReference(this, view.function);
   if (globalObjectStore)
   {
      if (view.store == globalObjectStore && view.isLive && (!validFunction || !view.isLive()))
         return nullptr;
      return globalObjectStore; // Unregistered active maps retain their caller contract.
   }
   // An unbound nested activation must not borrow the enclosing observation.
   if (HasActiveInvocation()) return nullptr;
   return validFunction && view.isLive && view.isLive() ? view.store : nullptr;
}

const SolarSystem* ObjectManagedFunction::GetObservedSolarSystem() const
{
   if (HasActiveInvocation()) return nullptr;
   const auto view = FindGlobalView(this);
   return GmatBase::IsLiveReference(this, view.function) &&
         view.isLive && view.isLive() &&
         GmatBase::IsLiveReference(view.solarSystem, view.solarSystemIdentity) ?
         view.solarSystem : nullptr;
}

void ObjectManagedFunction::ClearObservedSolarSystem()
{
   auto &registry = GlobalViews();
   std::lock_guard<std::mutex> lock(registry.mutex);
   const auto found = registry.entries.find(this);
   if (found == registry.entries.end()) return;
   found->second.solarSystem = nullptr;
   found->second.solarSystemIdentity.reset();
}

//------------------------------------------------------------------------------
// void SetGlobalObjectMap(ObjectMap *objMap)
//------------------------------------------------------------------------------
void ObjectManagedFunction::SetGlobalObjectMap(ObjectMap *objMap)
{
   globalObjectStore = objMap;
   RememberGlobalView(this, objMap, CaptureObjectStoreLifetime(objMap));
}


//------------------------------------------------------------------------------
// bool SetInputElementWrapper(const std::string &forName, ElementWrapper *wrapper)
//------------------------------------------------------------------------------
bool ObjectManagedFunction::SetInputElementWrapper(const std::string &forName, ElementWrapper *wrapper)
{
   #ifdef DEBUG_FUNCTION_IN_OUT
   MessageInterface::ShowMessage
      ("ObjectManagedFunction::SetInputElementWrapper() entered, wrapper name \"%s\"\n", forName.c_str());
   MessageInterface::ShowMessage
      ("   wrapper=<%p>, wrapper type = %d, inputArgMap.size()=%d\n", wrapper,
       wrapper->GetWrapperType(), inputArgMap.size());
   #endif
   
   if (inputArgMap.find(forName) == inputArgMap.end())
   {
      #ifdef DEBUG_FUNCTION_IN_OUT
      MessageInterface::ShowMessage
         ("**** ERROR **** ObjectManagedFunction::SetInputElementWrapper() throwing exception: "
          "Input name '%s' not found in inputArgMap\n", forName.c_str());
      #endif
      std::string errMsg = "Unknown input argument \"" + forName;
      errMsg += "\" for function \"" + functionName + "\"";
      throw FunctionException(errMsg);
   }
   
   auto lease = ElementWrapper::RetainInvocationWrapper(wrapper);
   if (lease && std::find(invocationWrapperLeases.begin(), invocationWrapperLeases.end(), lease) == invocationWrapperLeases.end())
      invocationWrapperLeases.push_back(std::move(lease));
   inputArgMap[forName] = wrapper;
   
   //@note old inputWrappers are deleted in the FunctionManager::CreateFunctionArgWrappers()
   // before creates new wrappers for input arguments
   
   #ifdef DEBUG_FUNCTION_IN_OUT
   MessageInterface::ShowMessage
      ("ObjectManagedFunction::SetInputElementWrapper() leaving, wrapper name \"%s\"\n", forName.c_str());
   #endif
   
   return true;
}

//------------------------------------------------------------------------------
// virtual ElementWrapper*  GetOutputArgument(Integer argNumber)
//------------------------------------------------------------------------------
/*
 * Implements GMAT FUNCTIONS design 27.2.2.3 GmatFunction Execution
 * step 4 of "Steps Performed on the Firstexecution"
 */
//------------------------------------------------------------------------------
ElementWrapper* ObjectManagedFunction::GetOutputArgument(Integer argNumber)
{
   if ((argNumber < 0) || (argNumber >= (Integer) outputNames.size()))
   {
      std::string errMsg = "Function error: argument number out-of-range\n";
      throw FunctionException(errMsg);
   }
   std::string argName = outputNames.at(argNumber);
   return GetOutputArgument(argName);
}


//------------------------------------------------------------------------------
// ElementWrapper* GetOutputArgument(const std::string &byName)
//------------------------------------------------------------------------------
ElementWrapper* ObjectManagedFunction::GetOutputArgument(const std::string &byName)
{
   #ifdef DEBUG_FUNCTION_IN_OUT
   MessageInterface::ShowMessage
      ("ObjectManagedFunction::GetOutputArgument(%s) entered, outputArgMap.size()=%d\n",
       byName.c_str(), outputArgMap.size());
   #endif
   
   if (outputArgMap.find(byName) == outputArgMap.end())
   {
      std::string errMsg = "Function error: output \"" + byName;
      errMsg += "\" from function \"" + functionName;
      errMsg += "\" does not exist.\n";
      throw FunctionException(errMsg);
   }
   
   ElementWrapper *ew = outputArgMap[byName];
   
   #ifdef DEBUG_FUNCTION_IN_OUT
   MessageInterface::ShowMessage
      ("ObjectManagedFunction::GetOutputArgument(%s) returning <%p>, type=%d\n", byName.c_str(),
       ew, ew ? ew->GetDataType() : -999);
   #endif
   
   return ew;
}


//------------------------------------------------------------------------------
// WrapperArray& GetWrappersToDelete()
//------------------------------------------------------------------------------
WrapperArray& ObjectManagedFunction::GetWrappersToDelete()
{
   return wrappersToDelete;
}


//------------------------------------------------------------------------------
// void ClearOutputWrappers()
//------------------------------------------------------------------------------
void ObjectManagedFunction::ClearOutputWrappers() noexcept
{
   // Neither allocation nor a copied deletion queue is needed. Clear every
   // alias before deleting a wrapper, including outputs not selected by the
   // caller and built-in outputs that are absent from wrappersToDelete.
   auto release = [this](ElementWrapper *wrapper) noexcept
   {
      if (!wrapper)
         return;
      for (auto &entry : outputArgMap)
         if (entry.second == wrapper)
            entry.second = NULL;
      for (auto &pending : wrappersToDelete)
         if (pending == wrapper)
            pending = NULL;
      // Input wrappers remain owned by the invoking FunctionManager.
      for (const auto &entry : inputArgMap)
         if (entry.second == wrapper)
            return;
      ElementWrapper::ReleaseOwnedWrapper(wrapper);
   };
   for (auto &entry : outputArgMap)
      release(entry.second);
   for (auto *pending : wrappersToDelete)
      release(pending);
   wrappersToDelete.clear();
   PruneInvocationWrapperLeases();
}


//------------------------------------------------------------------------------
// void ClearInOutArgMaps(bool deleteInputs, bool deleteOutputs)
//------------------------------------------------------------------------------
void ObjectManagedFunction::ClearInOutArgMaps(bool deleteInputs, bool deleteOutputs)
{
   #ifdef DEBUG_WRAPPER_CODE   
   MessageInterface::ShowMessage
      ("ObjectManagedFunction::ClearInOutArgMaps() this=<%p> '%s' entered\n", this,
       GetName().c_str());
   MessageInterface::ShowMessage
      ("inputArgMap.size()=%d, outputArgMap.size()=%d\n", inputArgMap.size(),
       outputArgMap.size());
   #endif
   
   if (deleteOutputs)
      ClearOutputWrappers();

   std::vector<ElementWrapper *> inputWrappersToDelete;
   std::map<std::string, ElementWrapper *>::iterator ewi;
   
   if (deleteInputs)
   {
      // input wrappers map
      for (ewi = inputArgMap.begin(); ewi != inputArgMap.end(); ++ewi)
      {
         if (ewi->second)
         {         
            if (find(inputWrappersToDelete.begin(), inputWrappersToDelete.end(), ewi->second) ==
                inputWrappersToDelete.end())
               inputWrappersToDelete.push_back(ewi->second);
         }
      }
   }
   
   #ifdef DEBUG_WRAPPER_CODE   
   MessageInterface::ShowMessage
      ("   There are %d wrappers to delete\n", inputWrappersToDelete.size());
   #endif
   
   // Detach every role before releasing an owned input. The same interface can
   // also occur in an output/deletion slot or a suspended invocation.
   inputArgMap.clear();
   outputArgMap.clear();
   // Delete old ElementWrappers (loj: 2008.11.20)
   for (std::vector<ElementWrapper*>::iterator ewi = inputWrappersToDelete.begin();
        ewi < inputWrappersToDelete.end(); ewi++)
   {
      for (auto &pending : wrappersToDelete) if (pending == *ewi) pending = nullptr;
      ElementWrapper::ReleaseOwnedWrapper(*ewi);
      (*ewi) = NULL;
   }
   PruneInvocationWrapperLeases();
}

//------------------------------------------------------------------------------
// void SetChangingInputType(bool hasChangingInput)
//------------------------------------------------------------------------------
/**
 * Set whether this function will have changing types of inputs. This info is
 * utilized for knowing when reinitialization is necessary.
 *
 * @param <hasChangingInput> Whether the function input type will change
 *
 */
//------------------------------------------------------------------------------
void ObjectManagedFunction::SetChangingInputType(bool hasChangingInput)
{
   changingInputType = hasChangingInput;
}

//------------------------------------------------------------------------------
// bool HasChangingInputType()
//------------------------------------------------------------------------------
/**
 * Get whether this function will have changing types of inputs. This info is
 * utilized for knowing when reinitialization is necessary.
 *
 */
//------------------------------------------------------------------------------
bool ObjectManagedFunction::HasChangingInputType()
{
   return changingInputType;
}

//------------------------------------------------------------------------------
// virtual bool TakeAction(const std::string &action,
//                         const std::string &actionData = "")
//------------------------------------------------------------------------------
/**
 * This method performs action.
 *
 * @param <action> action to perform
 * @param <actionData> action data associated with action ("")
 * @return true if action successfully performed
 *
 */
//------------------------------------------------------------------------------
bool ObjectManagedFunction::TakeAction(const std::string &action,
                                       const std::string &actionData)
{
   #ifdef DEBUG_FUNCTION_ACTION
   MessageInterface::ShowMessage
      ("ObjectManagedFunction::TakeAction() action=%s, actionData=%s\n", action.c_str(),
       actionData.c_str());
   #endif
   
   return Function::TakeAction(action, actionData);
}


//------------------------------------------------------------------------------
//  bool SetStringParameter(const Integer id, const char *value)
//------------------------------------------------------------------------------
/**
 * @see SetStringParameter(const Integer id, const std::string &value)
 */
//------------------------------------------------------------------------------
bool ObjectManagedFunction::SetStringParameter(const Integer id, const char *value)
{
   return SetStringParameter(id, std::string(value));
}


//------------------------------------------------------------------------------
//  bool SetStringParameter(const Integer id, const std::string &value)
//------------------------------------------------------------------------------
/**
 * Sets the value for a std::string parameter.
 * 
 * @param <id> Integer ID of the parameter.
 * @param <value> New value for the parameter.
 * 
 * @return If value of the parameter was set.
 */
//------------------------------------------------------------------------------
bool ObjectManagedFunction::SetStringParameter(const Integer id, const std::string &value)
{
   #ifdef DEBUG_FUNCTION_SET
   MessageInterface::ShowMessage
      ("ObjectManagedFunction::SetStringParameter() entered, id=%d, value='%s'\n", id, value.c_str());
   #endif
   
   switch (id)
   {
   case FUNCTION_INPUT:
      {
         #ifdef DEBUG_FUNCTION_SET
         MessageInterface::ShowMessage("   Adding function input name '%s'\n", value.c_str());
         #endif
         
         // Ignore () as input, so that "function MyFunction()" will work as no input (LOJ: 2015.08.19)
         if (value == "()")
            return true;
         
         if (inputArgMap.find(value) == inputArgMap.end())
         {
            inputNames.push_back(value);
            inputArgMap.insert(std::make_pair(value,(ElementWrapper*) NULL));
         }
         else
            throw FunctionException
               ("In function file \"" + functionPath + "\": "
                "The input argument \"" + value + "\" already exists");
         
         return Function::SetStringParameter(id, value);
      }
   case FUNCTION_OUTPUT:
      {
         #ifdef DEBUG_FUNCTION_SET
         MessageInterface::ShowMessage("   Adding function output name '%s'\n", value.c_str());
         #endif
         if (outputArgMap.find(value) == outputArgMap.end())
         {
            outputNames.push_back(value);
            outputArgMap.insert(std::make_pair(value,(ElementWrapper*) NULL));
         }
         else
            throw FunctionException
               ("In function file \"" + functionPath + "\": "
                "The output argument \"" + value + "\" already exists");
         
         return Function::SetStringParameter(id, value);
      }
   default:
      return Function::SetStringParameter(id, value);
   }
}


//------------------------------------------------------------------------------
// bool SetStringParameter(const std::string &label, const char *value)
//------------------------------------------------------------------------------
bool ObjectManagedFunction::SetStringParameter(const std::string &label,
                                               const char *value)
{
   return SetStringParameter(GetParameterID(label), std::string(value));
}

//------------------------------------------------------------------------------
// bool SetStringParameter(const std::string &label, const std::string &value)
//------------------------------------------------------------------------------
bool ObjectManagedFunction::SetStringParameter(const std::string &label,
                                               const std::string &value)
{
   return SetStringParameter(GetParameterID(label), value);
}

//---------------------------------
// protected
//---------------------------------

//------------------------------------------------------------------------------
// void ShowObjectMap(ObjectMap *objMap, const std::string &title,
//                    const std::string &mapName)
//------------------------------------------------------------------------------
void ObjectManagedFunction::ShowObjectMap(ObjectMap *objMap, const std::string &title,
                                          const std::string &mapName)
{
   MessageInterface::ShowMessage("%s\n", title.c_str());
   MessageInterface::ShowMessage("this=<%p>, functionName='%s'\n", this, functionName.c_str());
   if (objMap == NULL)
   {
      MessageInterface::ShowMessage("%s is NULL\n", mapName.c_str());
      return;
   }
   
   std::string objMapName = mapName;
   GmatBase *obj = NULL;
   std::string objName;
   std::string objTypeName = "NULL";
   std::string isGlobal;
   std::string isLocal;
   GmatBase *paramOwner = NULL;
   bool isParameter = false;
   std::string paramOwnerType;
   std::string paramOwnerName;
   
   if (objMapName == "")
      objMapName = "object map";
   
   MessageInterface::ShowMessage("========================================\n");
   MessageInterface::ShowMessage
      ("Here is %s <%p>, it has %d objects\n", objMapName.c_str(), objMap, objMap->size());
   for (ObjectMap::iterator i = objMap->begin(); i != objMap->end(); ++i)
   {
      obj = i->second;
      objName = i->first;
      objTypeName = "";
      isGlobal = "No";
      isLocal = "No";
      paramOwner = NULL;
      isParameter = false;
      paramOwnerType = "";
      paramOwnerName = "";
      
      if (obj)
      {
         objTypeName = obj->GetTypeName();
         if (obj->IsGlobal())
            isGlobal = "Yes";
         if (obj->IsLocal())
            isLocal = "Yes";
         if (obj->IsOfType(Gmat::PARAMETER))
         {
            isParameter = true;
            paramOwner = ((Parameter*)obj)->GetOwner();
            if (paramOwner)
            {
               paramOwnerType = paramOwner->GetTypeName();
               paramOwnerName = paramOwner->GetName();
            }
         }
      }
      MessageInterface::ShowMessage
         ("   %60s  <%p>  %-16s  IsGlobal:%-3s  IsLocal:%-3s", objName.c_str(), obj,
          objTypeName.c_str(), isGlobal.c_str(), isLocal.c_str());
      if (isParameter)
         MessageInterface::ShowMessage
            ("  ParameterOwner: <%p>[%s]'%s'\n", paramOwner, paramOwnerType.c_str(),
             paramOwnerName.c_str());
      else
         MessageInterface::ShowMessage("\n");
   }
}

