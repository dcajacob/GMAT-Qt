//$Id$
//------------------------------------------------------------------------------
//                             UserDefinedFunction
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
 * Implements UserDefinedFunction class. All user defined function classes
 * such as GmatFunction are derived from this class.
 */
//------------------------------------------------------------------------------

#include "UserDefinedFunction.hpp"
#include "NativeValueProvider.hpp"

namespace
{
const NativeValueProviderRegistration userDefinedFunctionValueProvider({
   UserDefinedFunction::NativeValueProviderDepth,
   [](const GmatBase *object) { return dynamic_cast<const UserDefinedFunction*>(object) != nullptr; },
   [](const GmatBase *object) { return dynamic_cast<const UserDefinedFunction*>(object)->GetFunctionValueOwnedObjectsImpl(); },
   nullptr,
   nullptr,
   nullptr,
   nullptr
});
}
#include "FunctionException.hpp"    // for exception
#include "StringUtil.hpp"           // for GmatStringUtil::
#include "Parameter.hpp"            // for GetOwner()
#include "MessageInterface.hpp"
#include "CommandUtil.hpp"
#include <set>
#include <typeinfo>
#include <exception>

struct FunctionDefinitionObjects
{
   struct BorrowedAutomatic
   {
      GmatBase *object;
      std::weak_ptr<const void> identity;
      bool invocation;
   };
   std::vector<std::unique_ptr<GmatBase>> owned;
   std::map<const GmatBase*, GmatBase*> identities;
   // Invocation storage borrows the actual function, including self aliases.
   const GmatBase *definitionOwner = nullptr;
   std::vector<std::unique_ptr<FunctionDefinitionObjects>> retained;
   // Borrow identity belongs to a declaration slot, not just an address that
   // a later allocation could reuse for a different automatic object.
   std::map<std::string, BorrowedAutomatic> borrowedAutomatic;
   std::vector<BorrowedAutomatic> borrowedVariants;
   std::set<const GmatBase*> automaticOwners;
   const BorrowedAutomatic *FindBorrow(const std::string &name, const GmatBase *object) const
   {
      const auto found = borrowedAutomatic.find(name);
      if (found != borrowedAutomatic.end()) return &found->second;
      for (const auto &entry : borrowedVariants)
         if (entry.object == object && GmatBase::IsLiveReference(object, entry.identity)) return &entry;
      return nullptr;
   }
   ~FunctionDefinitionObjects()
   {
      // Current prototypes can borrow a value retained by an older definition.
      for (auto &object : owned) object.reset();
      retained.clear();
   }
   bool Owns(const GmatBase *object) const
   {
      for (const auto &entry : owned) if (entry.get() == object) return true;
      for (const auto &prior : retained) if (prior->Owns(object)) return true;
      return false;
   }
   void Collect(ObjectArray &objects) const
   {
      for (const auto &entry : owned) objects.push_back(entry.get());
      for (const auto &prior : retained) prior->Collect(objects);
   }
   void CollectAutomatic(ObjectArray &objects) const
   {
      for (const auto *object : automaticOwners)
         objects.push_back(const_cast<GmatBase*>(object));
      for (const auto &prior : retained) prior->CollectAutomatic(objects);
   }
};

bool UserDefinedFunction::OwnsDefinitionObject(const GmatBase *object) const
{
   return definitionObjects && object && definitionObjects->Owns(object);
}

bool UserDefinedFunction::IsBorrowedAutomaticObject(const GmatBase *object) const
{
   if (definitionObjects)
   {
      for (const auto &entry : definitionObjects->borrowedAutomatic)
         if (entry.second.object == object) return true;
      for (const auto &entry : definitionObjects->borrowedVariants)
         if (entry.object == object) return true;
   }
   return false;
}

bool UserDefinedFunction::IsLiveAutomaticObject(const std::string &name,
      const GmatBase *object) const
{
   if (!object) return false;
   if (definitionObjects)
   {
      const auto *record = definitionObjects->FindBorrow(name, object);
      if (record)
         return record->object == object && GmatBase::IsLiveReference(object, record->identity);
      if (IsBorrowedAutomaticObject(object)) return false;
   }
   return true; // Native owned prototypes retain their ordinary ownership.
}

void UserDefinedFunction::RebuildBorrowedAutomaticObjects()
{
   ObjectArray globals;
   if (definitionObjects)
   {
      for (const auto &entry : definitionObjects->borrowedAutomatic)
         if (!entry.second.invocation &&
               GmatBase::IsLiveReference(entry.second.object, entry.second.identity) &&
               std::find(globals.begin(), globals.end(), entry.second.object) == globals.end())
            globals.push_back(entry.second.object);
      for (const auto &entry : definitionObjects->borrowedVariants)
         if (!entry.invocation && GmatBase::IsLiveReference(entry.object, entry.identity) &&
               std::find(globals.begin(), globals.end(), entry.object) == globals.end())
            globals.push_back(entry.object);
   }
   sandboxObjects.swap(globals);
}

void UserDefinedFunction::PruneExpiredAutomaticReferences()
{
   if (!definitionObjects) return;
   auto &borrowed = definitionObjects->borrowedAutomatic;
   for (auto entry = borrowed.begin(); entry != borrowed.end();)
   {
      if (GmatBase::IsLiveReference(entry->second.object, entry->second.identity))
      { ++entry; continue; }
      const auto automatic = automaticObjectMap.find(entry->first);
      if (automatic != automaticObjectMap.end() && automatic->second == entry->second.object)
         automatic->second = nullptr;
      const auto lookup = validatorStore.find(entry->first);
      if (lookup != validatorStore.end() && lookup->second == entry->second.object)
         validatorStore.erase(lookup);
      objectsToDelete.erase(std::remove(objectsToDelete.begin(), objectsToDelete.end(),
            entry->second.object), objectsToDelete.end());
      entry = borrowed.erase(entry);
   }
   auto &variants = definitionObjects->borrowedVariants;
   variants.erase(std::remove_if(variants.begin(), variants.end(),
         [](const FunctionDefinitionObjects::BorrowedAutomatic &entry)
         { return !GmatBase::IsLiveReference(entry.object, entry.identity); }), variants.end());
   RebuildBorrowedAutomaticObjects();
}

void UserDefinedFunction::RetireAutomaticReferences(ObjectMap *store)
{
   PruneExpiredAutomaticReferences();
   std::set<GmatBase*> retiring;
   if (store)
      for (const auto &entry : *store)
         if (entry.second && (!entry.second->IsGlobal() || entry.second->IsLocal()))
            retiring.insert(entry.second);
   if (definitionObjects)
   {
      auto &borrowed = definitionObjects->borrowedAutomatic;
      for (auto entry = borrowed.begin(); entry != borrowed.end();)
      {
         if (!retiring.count(entry->second.object)) { ++entry; continue; }
         auto automatic = automaticObjectMap.find(entry->first);
         if (automatic != automaticObjectMap.end() && automatic->second == entry->second.object)
            automatic->second = nullptr;
         entry = borrowed.erase(entry);
      }
      auto &variants = definitionObjects->borrowedVariants;
      variants.erase(std::remove_if(variants.begin(), variants.end(),
            [&](const FunctionDefinitionObjects::BorrowedAutomatic &entry)
            { return retiring.count(entry.object); }), variants.end());
      // These are symbolic prototypes; returned value copies have separate
      // owners. Preserve lexical roles and value/history data, not FOS caches.
      ObjectArray prototypes;
      definitionObjects->CollectAutomatic(prototypes);
      std::exception_ptr firstError;
      for (auto *object : prototypes)
         if (auto *parameter = dynamic_cast<Parameter*>(object))
            try { parameter->RetirePrototypeReferences(); }
            catch (...) { if (!firstError) firstError = std::current_exception(); }
      RebuildBorrowedAutomaticObjects();
      if (firstError) std::rethrow_exception(firstError);
   }
}

void UserDefinedFunction::RetireSandboxReferences(const ObjectMap &objects)
{
   // Parsed declarations can borrow system Parameters from the sandbox.
   // Drop those aliases while their owner is still alive; a later destructor
   // must not inspect a Parameter that the sandbox has already deleted.
   std::set<GmatBase*> retiring;
   for (const auto &entry : objects)
      if (entry.second && entry.second != this && !OwnsDefinitionObject(entry.second))
         retiring.insert(entry.second);
   for (ObjectMap *map : {&functionObjectMap, &automaticObjectMap, &validatorStore})
      for (auto entry = map->begin(); entry != map->end();)
         if (retiring.count(entry->second)) entry = map->erase(entry);
         else ++entry;
   auto retire = [&](GmatBase *object) { return retiring.count(object) != 0; };
   objectsToDelete.erase(std::remove_if(objectsToDelete.begin(), objectsToDelete.end(), retire), objectsToDelete.end());
   sandboxObjects.erase(std::remove_if(sandboxObjects.begin(), sandboxObjects.end(), retire), sandboxObjects.end());
   if (definitionObjects)
   {
      auto &borrowed = definitionObjects->borrowedAutomatic;
      for (auto entry = borrowed.begin(); entry != borrowed.end();)
         if (retire(entry->second.object)) entry = borrowed.erase(entry);
         else ++entry;
      auto &variants = definitionObjects->borrowedVariants;
      variants.erase(std::remove_if(variants.begin(), variants.end(),
            [&](const FunctionDefinitionObjects::BorrowedAutomatic &entry)
            { return retire(entry.object); }), variants.end());
   }
}

void UserDefinedFunction::RetireFunctionBindings(ObjectMap *store)
{
   if (!store) return;
   // Different managers can share a compiled recursive function. Finalizing a
   // command can revisit this sequence; its outer retirement owns the walk.
   static thread_local std::set<const UserDefinedFunction*> retiring;
   if (!retiring.insert(this).second) return;
   struct RetirementGuard
   {
      std::set<const UserDefinedFunction*> &functions;
      const UserDefinedFunction *function;
      ~RetirementGuard() { functions.erase(function); }
   } guard{retiring, this};
   const auto commands = GmatCommandUtil::GetCommandSequenceNodes(fcs);
   bool matches = objectStore == store;
   for (auto *command : commands)
      if (command->GetObjectMap() == store) matches = true;
   if (matches) fcsInitialized = false;
   // Native ownership edges define retirement order. BranchEnd navigation
   // backlinks are borrowed, so they do not form ownership cycles. Retire
   // children before any command-owned solver snapshots or local parameters.
   std::vector<GmatCommand*> ordered;
   std::set<GmatCommand*> visited, visiting;
   std::function<void(GmatCommand*)> order = [&](GmatCommand *command)
   {
      if (!command || visited.count(command)) return;
      if (!visiting.insert(command).second)
         throw FunctionException("Cyclic command ownership during function retirement.\n");
      for (const auto &link : command->GetDefinitionCommandLinks())
         if (link.second) order(link.first);
      visiting.erase(command);
      visited.insert(command);
      ordered.push_back(command);
   };
   for (auto *command : commands) order(command);
   std::exception_ptr firstError;
   for (auto *command : ordered)
      try { command->RetireFunctionBindings(store); }
      catch (...) { if (!firstError) firstError = std::current_exception(); }
   if (objectStore == store) validatorStore.clear();
   if (firstError) std::rethrow_exception(firstError);
}

ObjectArray UserDefinedFunction::GetFunctionValueOwnedObjectsImpl() const
{
   ObjectArray objects;
   std::set<GmatBase*> seen;
   auto borrowed = [this](GmatBase *object)
   {
      return IsBorrowedAutomaticObject(object) ||
            std::find(sandboxObjects.begin(), sandboxObjects.end(), object) != sandboxObjects.end();
   };
   auto add = [&](GmatBase *object)
   {
      if (object && object != this &&
            (!definitionObjects || object != definitionObjects->definitionOwner) &&
            !borrowed(object) && seen.insert(object).second)
         objects.push_back(object);
   };
   if (definitionObjects)
   {
      ObjectArray retained;
      definitionObjects->Collect(retained);
      for (auto *object : retained) add(object);
   }
   std::set<GmatBase*> declared;
   for (const auto &entry : functionObjectMap)
   {
      auto *object = entry.second;
      if (!object) continue;
      declared.insert(object);
      if (object != this && !borrowed(object) && !object->IsOfType("GmatFunction") &&
            object->IsLocal() && !object->IsOfType(Gmat::CELESTIAL_BODY)) add(object);
   }
   for (const auto &entry : automaticObjectMap) if (!declared.count(entry.second)) add(entry.second);
   for (auto *object : objectsToDelete) if (!declared.count(object)) add(object);
   for (auto *command : GmatCommandUtil::GetCommandSequenceNodes(fcs))
   {
      add(command);
      for (auto *object : command->GetCommandOwnedObjects()) add(object);
   }
   return objects;
}

void UserDefinedFunction::CopyDefinitionGraph(const UserDefinedFunction &source,
      UserDefinedFunction *definitionOwner)
{
   if (fcs) throw FunctionException("A compiled function definition must be rebuilt before replacement.\n");
   if (!definitionOwner) definitionOwner = this;
   auto objects = std::make_unique<FunctionDefinitionObjects>();
   objects->definitionOwner = definitionOwner;
   objects->identities.emplace(&source, definitionOwner);
   std::set<const GmatBase*> originals;
   std::vector<GmatBase*> owners;
   auto borrowed = [&source](GmatBase *object)
   {
      return source.IsBorrowedAutomaticObject(object) ||
            std::find(source.sandboxObjects.begin(), source.sandboxObjects.end(), object) !=
            source.sandboxObjects.end();
   };
   auto addOwner = [&](GmatBase *object)
   {
      if (object && object != &source && !borrowed(object) && originals.insert(object).second)
         owners.push_back(object);
   };
   // Previously retained/retired prototypes can still be referenced by a
   // command definition even after its declaration table is edited or cleared.
   if (source.definitionObjects)
   {
      ObjectArray retained;
      source.definitionObjects->Collect(retained);
      for (auto *object : retained) addOwner(object);
   }
   std::set<GmatBase*> declared;
   for (const auto &entry : source.functionObjectMap)
   {
      GmatBase *object = entry.second;
      if (!object) continue;
      declared.insert(object);
      // Compare sandbox identities before dereferencing a borrowed prototype.
      if (!borrowed(object) && object != &source &&
            !object->IsOfType("GmatFunction") && object->IsLocal() &&
            !object->IsOfType(Gmat::CELESTIAL_BODY)) addOwner(object);
   }
   for (const auto &entry : source.automaticObjectMap)
      if (!declared.count(entry.second)) addOwner(entry.second);
   for (auto *object : source.objectsToDelete)
      if (!declared.count(object)) addOwner(object);

   objects->owned.reserve(owners.size());
   std::set<GmatBase*> allocated;
   for (auto *owner : owners)
   {
      GmatBase *raw = owner->Clone();
      if (!raw || raw == &source || raw == this || originals.count(raw) || allocated.count(raw))
         throw FunctionException("Function prototype Clone returned a source, peer or null value.\n");
      std::unique_ptr<GmatBase> copy(raw);
      if (typeid(*copy) != typeid(*owner) || copy->GetTypeName() != owner->GetTypeName())
         throw FunctionException("Function prototype Clone returned an incompatible value.\n");
      allocated.insert(raw);
      objects->identities.emplace(owner, raw);
      ObjectArray automaticOwners;
      if (source.definitionObjects) source.definitionObjects->CollectAutomatic(automaticOwners);
      bool automatic = std::find(automaticOwners.begin(), automaticOwners.end(), owner) != automaticOwners.end();
      for (const auto &entry : source.automaticObjectMap)
         if (entry.second == owner) automatic = true;
      if (automatic)
      {
         objects->automaticOwners.insert(raw);
         if (auto *parameter = dynamic_cast<Parameter*>(raw))
            parameter->RetirePrototypeReferences();
      }
      objects->owned.push_back(std::move(copy));
   }
   auto mapped = [&objects](GmatBase *object)
   {
      const auto found = objects->identities.find(object);
      return found == objects->identities.end() ? object : found->second;
   };
   ObjectMap declarations, automatic;
   for (const auto &entry : source.functionObjectMap)
      declarations.emplace(entry.first, mapped(entry.second));
   ObjectArray sandbox;
   for (const auto &entry : source.automaticObjectMap)
   {
      GmatBase *object = entry.second;
      const auto *record = source.definitionObjects ?
            source.definitionObjects->FindBorrow(entry.first, object) : nullptr;
      const bool invocation = record && record->invocation;
      // A native cold definition does not retain another invocation's locals.
      // Keep the declaration key for reconstruction by ordinary validation.
      if (invocation || !source.IsLiveAutomaticObject(entry.first, object)) object = nullptr;
      automatic.emplace(entry.first, mapped(object));
      if (object && record)
      {
         objects->borrowedAutomatic.emplace(entry.first, *record);
         if (std::find(sandbox.begin(), sandbox.end(), object) == sandbox.end()) sandbox.push_back(object);
      }
   }

   if (source.definitionObjects)
      for (const auto &entry : source.definitionObjects->borrowedVariants)
         if (!entry.invocation && GmatBase::IsLiveReference(entry.object, entry.identity))
         {
            objects->borrowedVariants.push_back(entry);
            if (std::find(sandbox.begin(), sandbox.end(), entry.object) == sandbox.end())
               sandbox.push_back(entry.object);
         }
   std::unique_ptr<GmatCommand> sequence(GmatCommandUtil::CloneCommandSequence(source.fcs));
   for (auto *command : GmatCommandUtil::GetCommandSequenceNodes(sequence.get()))
      command->GmatCommand::SetCurrentFunction(definitionOwner);
   objects->retained.reserve(1);
   // Publish the complete cold definition only after every allocation succeeds.
   functionObjectMap.swap(declarations);
   automaticObjectMap.swap(automatic);
   sandboxObjects.swap(sandbox);
   if (definitionObjects) objects->retained.push_back(std::move(definitionObjects));
   definitionObjects = std::move(objects);
   fcs = sequence.release();
   fcsInitialized = false;
   fcsFinalized = false;
   objectsInitialized = false;
   isInitialized = false;
}

void UserDefinedFunction::SwapDefinitionState(UserDefinedFunction &other) noexcept
{
   using std::swap;
   swap(fcs, other.fcs);
   swap(fcsInitialized, other.fcsInitialized);
   swap(fcsFinalized, other.fcsFinalized);
   functionObjectMap.swap(other.functionObjectMap);
   automaticObjectMap.swap(other.automaticObjectMap);
   swap(validator, other.validator);
   validatorStore.swap(other.validatorStore);
   sandboxObjects.swap(other.sandboxObjects);
   wrappersToDelete.swap(other.wrappersToDelete);
   objectsToDelete.swap(other.objectsToDelete);
   swap(objectsInitialized, other.objectsInitialized);
   definitionObjects.swap(other.definitionObjects);
}

std::unique_ptr<FunctionInvocationState> UserDefinedFunction::SuspendInvocationState()
{
   // This private storage owner is neither registered nor executed as a
   // function resource. Only the actual callee can supply its concrete Clone.
   struct DefinitionStorage final : UserDefinedFunction
   {
      using UserDefinedFunction::UserDefinedFunction;
      GmatBase *Clone() const override
      {
         throw FunctionException("Invocation definition storage is not a function value.\n");
      }
   };
   struct DefinitionFrame
   {
      std::unique_ptr<UserDefinedFunction> definition;
      bool armed = false;
   };
   auto saved = std::make_shared<DefinitionFrame>();
   saved->definition = std::make_unique<DefinitionStorage>(GetTypeName(), GetName());
   // Stage an independent cold graph before detaching any live field. Its
   // commands belong to the actual callee, never to this temporary owner.
   saved->definition->CopyDefinitionGraph(*this, this);
   auto restore = [this, saved]()
   {
      if (!saved->armed) return;
      std::exception_ptr firstError;
      // Retire the inner commands through the concrete function's ordinary
      // completion path while its own prototype tables are still installed.
      try { Finalize(false); }
      catch (...) { firstError = std::current_exception(); }
      SwapDefinitionState(*saved->definition);
      if (firstError) std::rethrow_exception(firstError);
   };
   auto state = std::make_unique<FunctionInvocationState>(*this, std::move(restore),
         ObjectManagedFunction::SuspendInvocationState());
   SwapDefinitionState(*saved->definition);
   isInitialized = false;
   saved->armed = true;
   // The frame now owns the enclosing sequence and all its prototype tables.
   // Restoration swaps them back intact; the ordinary cold owner's destructor
   // retires the inner commands before enclosing environment restoration.
   return state;
}

void UserDefinedFunction::ClearFunctionControlSequence(bool callRunComplete)
{
   if (!fcs) return;
   std::unique_ptr<GmatCommand> sequence(fcs);
   fcs = nullptr;
   const bool complete = callRunComplete && fcsInitialized && !fcsFinalized;
   fcsInitialized = false;
   fcsFinalized = false;
   objectsInitialized = false;
   isInitialized = false;
   // A copied definition has not run. Avoid executing cleanup callbacks just
   // because its owner is destroyed during capture/exception unwinding.
   // Keep ownership of the root if a child RunComplete callback throws.
   GmatCommandUtil::ClearCommandSeq(sequence.get(), true, complete);
}

//#define DEBUG_FUNCTION_SET
//#define DEBUG_FUNCTION_FINALIZE

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
//  UserDefinedFunction(std::string typeStr, std::string name)
//------------------------------------------------------------------------------
/**
 * Constructs the UserDefinedFunction object (default constructor).
 * 
 * @param <typeStr> String text identifying the object type
 * @param <name>   Name for the object
 */
//------------------------------------------------------------------------------
UserDefinedFunction::UserDefinedFunction(const std::string &typeStr, const std::string &name) :
   ObjectManagedFunction(typeStr, name),
   fcs                (NULL),
   fcsInitialized     (false),
   fcsFinalized       (false),
   validator          (NULL),
   objectsInitialized (false)
{
   if (typeStr != "")
      objectTypeNames.push_back(typeStr);
   objectTypeNames.push_back("UserDefinedFunction");
}


//------------------------------------------------------------------------------
//  ~UserDefinedFunction(void)
//------------------------------------------------------------------------------
/**
 * Destroys the UserDefinedFunction object (destructor).
 */
//------------------------------------------------------------------------------
UserDefinedFunction::~UserDefinedFunction()
{
   #ifdef DEBUG_FUNCTION_FINALIZE
   MessageInterface::ShowMessage
      ("UserDefinedFunction::~UserDefinedFunction() <%p>[%s]'%s' entered\n", this, GetTypeName().c_str(),
       GetName().c_str());
   #endif
   
   // Output wrappers can refer to these original objects. Release the wrappers
   // while their resources still exist; input wrappers belong to the manager.
   ClearOutputWrappers();
   ClearFunctionControlSequence(false);
   try { RetireAutomaticReferences(nullptr); }
   catch (BaseException &error)
   { MessageInterface::ShowMessage("Automatic prototype cleanup: %s\n", error.GetFullMessage().c_str()); }
   catch (...)
   { MessageInterface::ShowMessage("Automatic prototype cleanup reported an error.\n"); }
   // Clear original objects
   ClearAutomaticObjects();
   ClearFunctionObjects();
   
   #ifdef DEBUG_FUNCTION_FINALIZE
   MessageInterface::ShowMessage
      ("UserDefinedFunction::~UserDefinedFunction() <%p>[%s]'%s' leaving\n", this, GetTypeName().c_str(),
       GetName().c_str());
   #endif
}


//------------------------------------------------------------------------------
//  UserDefinedFunction(const UserDefinedFunction &f)
//------------------------------------------------------------------------------
/**
 * Constructs the UserDefinedFunction object (copy constructor).
 * 
 * @param <f> Object that is copied
 */
//------------------------------------------------------------------------------
UserDefinedFunction::UserDefinedFunction(const UserDefinedFunction &f) :
   ObjectManagedFunction (f),
   fcs                (NULL),
   fcsInitialized     (false),
   fcsFinalized       (false),
   functionObjectMap  (),
   validator          (f.validator),
   objectsInitialized (false)
{
   CopyDefinitionGraph(f);
   // Compiled syntax and independent prototypes are retained; invocation
   // stores, output/input wrapper bindings and execution cursors stay cold.
   isInitialized = false;
   //parameterCount = UserDefinedFunctionParamCount;
}


//------------------------------------------------------------------------------
//  UserDefinedFunction& operator=(const UserDefinedFunction &f)
//------------------------------------------------------------------------------
/**
 * Sets one UserDefinedFunction object to match another (assignment operator).
 * 
 * @param <f> The object that is copied.
 * 
 * @return this object, with the parameters set as needed.
 */
//------------------------------------------------------------------------------
UserDefinedFunction& UserDefinedFunction::operator=(const UserDefinedFunction &f)
{
   if (this == &f)
      return *this;

   // Match the cached-source rule used by GmatFunction's path/format setters.
   // Even an idle FCS has bound callers and manager-owned argument stores.
   // Retain an identical definition, including its live invocation state;
   // changing it requires the normal mission/function rebuild lifecycle.
   if (fcs)
   {
      if (HasSameDefinition(f)) return *this;
      throw FunctionException("Rebuild GmatFunction '" + GetName() +
            "' before assigning a different function definition; its source has already been interpreted.");
   }

   ClearOutputWrappers();
   ClearAutomaticObjects();
   ClearFunctionObjects();
   automaticObjectMap.clear();
   functionObjectMap.clear();
   validatorStore.clear();
   sandboxObjects.clear();
   objectsToDelete.clear();
   wrappersToDelete.clear(); // Legacy non-owning storage; base owns outputs.
   
   ObjectManagedFunction::operator=(f);
   
   fcs                = NULL;
   fcsInitialized     = false;
   fcsFinalized       = false;
   validator          = f.validator;
   objectsInitialized = false;
   isInitialized      = false;
   solarSys           = NULL;
   internalCoordSys   = NULL;
   forces             = NULL;
   errorFound         = false;
   CopyDefinitionGraph(f);
   return *this;
}

bool UserDefinedFunction::HasSameDefinition(const UserDefinedFunction &other) const
{
   // Input/output name/type caches are derived from the source. Copying those
   // caches from an unparsed value must not overwrite a valid compiled interface.
   return GetTypeName() == other.GetTypeName() &&
         functionPath == other.functionPath && functionName == other.functionName &&
         sourceFormat == other.sourceFormat &&
         requireOutputBrackets == other.requireOutputBrackets;
}

//------------------------------------------------------------------------------
// bool Initialize(ObjectInitializer *objInit, bool reinitialize = false)
//------------------------------------------------------------------------------
bool UserDefinedFunction::Initialize(ObjectInitializer *objInit,
                                     bool reinitialize,
                                     bool resetLocalObjects)
{
   validator = Validator::Instance();
   ObjectManagedFunction::Initialize(objInit, reinitialize, resetLocalObjects);
   return true;
}


//------------------------------------------------------------------------------
// bool UserDefinedFunction::Execute(ObjectInitializer *objInit)  [default implementation]
//------------------------------------------------------------------------------
bool UserDefinedFunction::Execute(ObjectInitializer *objInit, bool reinitialize)
{
   return true;
}


//------------------------------------------------------------------------------
// bool UserDefinedFunction::Finalize(bool cleanUp = false)  [default implementation]
//------------------------------------------------------------------------------
void UserDefinedFunction::Finalize(bool cleanUp)
{
   PruneExpiredAutomaticReferences();
   #ifdef DEBUG_FUNCTION_FINALIZE
   MessageInterface::ShowMessage
      ("\nUserDefinedFunction::Finalize() entered, fcsFinalized=%d\n", fcsFinalized);
   #endif
   
   if (sandboxObjects.size() == 0)
   {
      #ifdef DEBUG_FUNCTION_FINALIZE
      MessageInterface::ShowMessage
         ("UserDefinedFunction::Finalize() leaving, there is nothing to do since snadboxObjects are empty\n\n");
      #endif
      return;
   }
   
   if (globalObjectStore == NULL)
   {
      #ifdef DEBUG_FUNCTION_FINALIZE
      MessageInterface::ShowMessage
         ("UserDefinedFunction::Finalize() leaving, there is nothing to do since globalObjectStore is NULL\n\n");
      #endif
      return;
   }
   
   #ifdef DEBUG_FUNCTION_FINALIZE
   ShowObjects("In UserDefinedFunction::Finalize()");
   #endif
   
   // Set ref Parameter back to original ref Parameter for global ReportFile and XyPlot
   // since these subscribers uses Parameters for reporting and plotting
   GmatBase *sandboxObj = NULL;
   GmatBase *globalObj = NULL;
   GmatBase *autoObj = NULL;
   std::string sandboxObjName;
   std::string globalObjName;
   std::string autoObjName;
   
   #ifdef DEBUG_FUNCTION_FINALIZE
   MessageInterface::ShowMessage
      ("Going through sandboxObjects to see if global Parameters in global "
       "Subscribers need to point back to sandbox pointer\n");
   #endif
   // Go through already managed sandbox objects
   for (UnsignedInt i = 0; i < sandboxObjects.size(); i++)
   {
      sandboxObj = sandboxObjects[i];
      #ifdef DEBUG_FUNCTION_FINALIZE
      MessageInterface::ShowMessage
         ("   sandboxObj=<%p>[%s]'%s'\n", sandboxObj,
          sandboxObj ? sandboxObj->GetTypeName().c_str() : "NULL",
          sandboxObj ? sandboxObj->GetName().c_str() : "NULL");
      #endif
      
      if (sandboxObj == NULL)
         continue;
      
      sandboxObjName = sandboxObj->GetName();
      if (sandboxObj->IsOfType(Gmat::PARAMETER))
      {
         GmatBase *paramOwner = ((Parameter*)sandboxObj)->GetOwner();
         #ifdef DEBUG_FUNCTION_FINALIZE
         MessageInterface::ShowMessage
            ("   sandboxObj is Parameter, paramOwner = <%p>[%s]'%s'\n", paramOwner,
             paramOwner ? paramOwner->GetTypeName().c_str() : "NULL",
             paramOwner ? paramOwner->GetName().c_str() : "NULL");
         #endif
         
         // Go through global ReportFile or XYPlot and reset ref Parameter
         for (ObjectMap::iterator i = globalObjectStore->begin(); i != globalObjectStore->end(); ++i)
         {
            try
            {
               globalObj = i->second;
               globalObjName = i->first;
               #ifdef DEBUG_FUNCTION_FINALIZE
               MessageInterface::ShowMessage(GmatBase::WriteObjectInfo("   ", globalObj));
               MessageInterface::ShowMessage("Checking if it is ReportFile or XYPlot\n");
               #endif
               if (globalObj && 
                   (globalObj->IsOfType("ReportFile") || globalObj->IsOfType("XYPlot")))
               {
                  #ifdef DEBUG_FUNCTION_FINALIZE
                  MessageInterface::ShowMessage
                     ("   Found global <%p>[%s]'%s', so calling globalObj->SetRefObject(%s)\n",
                      globalObj, globalObj->GetTypeName().c_str(), globalObjName.c_str(),
                      sandboxObjName.c_str());
                  #endif
                  
                  // We can use globalObj->GetRefObjectNameArray() and go through this list
                  // or ignore exception. Ignoring exception for now.
                  try
                  {
                     globalObj->SetRefObject(sandboxObj, Gmat::PARAMETER, sandboxObjName);
                  }
                  catch (BaseException &be)
                  {
                     // Ignore exception
                     #ifdef DEBUG_FUNCTION_FINALIZE
                     MessageInterface::ShowMessage("   Ignoring exception: %s\n", be.GetFullMessage().c_str());
                     #endif
                  }
               }
            }
            catch (BaseException &be)
            {
               #ifdef DEBUG_FUNCTION_FINALIZE
               MessageInterface::ShowMessage
                  ("Caught exception: %s\n", be.GetFullMessage().c_str());
               #endif
               // If cleanup mode, ignore exception
               if (!cleanUp)
                  throw;
            }
         }
      }
   }
   
   ObjectManagedFunction::Finalize();
   
   #ifdef DEBUG_FUNCTION_FINALIZE
   MessageInterface::ShowMessage("UserDefinedFunction::Finalize() leaving\n\n");
   #endif
}

//------------------------------------------------------------------------------
// bool IsFcsFinalized()
//------------------------------------------------------------------------------
bool UserDefinedFunction::IsFcsFinalized()
{
   return fcsFinalized;
}

//------------------------------------------------------------------------------
// bool IsFunctionControlSequenceSet()
//------------------------------------------------------------------------------
bool UserDefinedFunction::IsFunctionControlSequenceSet()
{
   if (fcs != NULL) return true;
   return false;
}

//------------------------------------------------------------------------------
// bool SetFunctionControlSequence(GmatCommand *cmd)
//------------------------------------------------------------------------------
bool UserDefinedFunction::SetFunctionControlSequence(GmatCommand *cmd)
{
   if (cmd == fcs) return true;
   if (fcs)
      throw FunctionException("Rebuild GmatFunction '" + GetName() +
            "' before replacing its interpreted control sequence.");
   #ifdef DEBUG_FUNCTION_SET
      if (!cmd) MessageInterface::ShowMessage("Trying to set FCS on %s, but it is NULL!!!\n",
            functionName.c_str());
      else
      {
         MessageInterface::ShowMessage("Setting FCS for function %s with FCS pointer = %p\n",
               functionName.c_str(), cmd);  
         MessageInterface::ShowMessage("First command is a %s\n", (cmd->GetTypeName()).c_str());
      }
   #endif
   fcs = cmd;
   fcsInitialized = false;
   fcsFinalized = false;
   objectsInitialized = false;
   isInitialized = false;
   return true;
}

//------------------------------------------------------------------------------
// GmatBase* GetFunctionControlSequence()
//------------------------------------------------------------------------------
GmatCommand* UserDefinedFunction::GetFunctionControlSequence()
{
   return fcs;
}


//------------------------------------------------------------------------------
// void ClearFunctionObjects()
//------------------------------------------------------------------------------
void UserDefinedFunction::ClearFunctionObjects()
{
   std::set<GmatBase*> release;
   for (const auto &entry : functionObjectMap)
   {
      auto *object = entry.second;
      if (!object || object == this ||
            (definitionObjects && object == definitionObjects->definitionOwner) ||
            OwnsDefinitionObject(object) ||
            std::find(sandboxObjects.begin(), sandboxObjects.end(), object) != sandboxObjects.end()) continue;
      if (!object->IsOfType("GmatFunction") && object->IsLocal() &&
            !object->IsOfType(Gmat::CELESTIAL_BODY)) release.insert(object);
   }
   // Clear all aliases before deleting any prototype; a later key must never
   // inspect an already deleted object to decide whether it owned that value.
   for (auto &entry : functionObjectMap)
      if (release.count(entry.second) || OwnsDefinitionObject(entry.second)) entry.second = nullptr;
   for (auto &entry : automaticObjectMap)
      if (release.count(entry.second)) entry.second = nullptr;
   objectsToDelete.erase(std::remove_if(objectsToDelete.begin(), objectsToDelete.end(),
         [&](GmatBase *object) { return release.count(object) || OwnsDefinitionObject(object); }), objectsToDelete.end());
   for (auto *object : release) delete object;
}

//------------------------------------------------------------------------------
// void AddFunctionObject(GmatBase *obj)
//------------------------------------------------------------------------------
void UserDefinedFunction::AddFunctionObject(GmatBase *obj)
{
   #ifdef DEBUG_FUNCTION_OBJ
   MessageInterface::ShowMessage
      ("UserDefinedFunction::AddFunctionObject() entered, obj=<%p>[%s]'%s'\n", obj,
       obj ? obj->GetTypeName().c_str() : "NULL",
       obj ? obj->GetName().c_str() : "NULL");
   //ShowObjects("Entered UserDefinedFunction::AddFunctionObject()");
   #endif
   
   if (obj && obj->GetName() != "")
   {
      std::string objName = obj->GetName();
      if (functionObjectMap.find(objName) == functionObjectMap.end())
         functionObjectMap.insert(std::make_pair(objName, obj));
   }
   #ifdef DEBUG_FUNCTION_OBJ
   MessageInterface::ShowMessage("UserDefinedFunction::AddFunctionObject() leaving\n");
   ShowObjectMap(&functionObjectMap, "Leaving AddFunctionObject()", "functionObjectMap");
   #endif
}


//------------------------------------------------------------------------------
// GmatBase* FindFunctionObject(const std::string &name)
//------------------------------------------------------------------------------
GmatBase* UserDefinedFunction::FindFunctionObject(const std::string &name)
{
   #ifdef DEBUG_FIND_OBJECT
   MessageInterface::ShowMessage
      ("UserDefinedFunction::FindFunctionObject() entered, name='%s'\n", name.c_str());
   //ShowObjects("In Function::FindFunctionObject()");
   #endif
   
   // Ignore array index
   std::string newName = name;
   std::string::size_type index = name.find_first_of("([");
   if (index != name.npos)
      newName = name.substr(0, index);
   
   #ifdef DEBUG_FIND_OBJECT
   MessageInterface::ShowMessage("   newName='%s'\n", newName.c_str());
   #endif
   
   GmatBase *obj = NULL;
   
   if (functionObjectMap.find(newName) != functionObjectMap.end())
      obj = functionObjectMap[newName];
   
   // try SolarSystem if obj is still NULL
   if (obj == NULL)
      if (const auto *system = GetSolarSystem())
         // The native lookup predates const accessors; the observed service
         // was bound from a mutable SolarSystem and this only finds its body.
         obj = const_cast<SolarSystem*>(system)->GetBody(newName);
   
   #ifdef DEBUG_FIND_OBJECT
   MessageInterface::ShowMessage
      ("UserDefinedFunction::FindFunctionObject() returning <%p>\n", obj);
   #endif
   
   return obj;
}

//------------------------------------------------------------------------------
// ObjectMap* GetFunctionObjectMap()
//------------------------------------------------------------------------------
ObjectMap* UserDefinedFunction::GetFunctionObjectMap()
{
   return &functionObjectMap;
}

//------------------------------------------------------------------------------
// void ClearAutomaticObjects()
//------------------------------------------------------------------------------
void UserDefinedFunction::ClearAutomaticObjects()
{
   // Ownership is by actual object identity, independent of declaration keys.
   std::set<GmatBase*> borrowed(sandboxObjects.begin(), sandboxObjects.end());
   if (definitionObjects)
   {
      for (const auto &entry : definitionObjects->borrowedAutomatic) borrowed.insert(entry.second.object);
      for (const auto &entry : definitionObjects->borrowedVariants) borrowed.insert(entry.object);
   }
   for (const auto &entry : functionObjectMap) if (entry.second) borrowed.insert(entry.second);
   std::set<GmatBase*> release;
   auto collect = [&](GmatBase *object)
   {
      if (object && object != this &&
            (!definitionObjects || object != definitionObjects->definitionOwner) &&
            !borrowed.count(object) && !OwnsDefinitionObject(object)) release.insert(object);
   };
   for (const auto &entry : automaticObjectMap) collect(entry.second);
   // This also handles the optional __COLLECT_AUTO_OBJECTS__ producer without
   // deleting repeated entries or prototypes shared with the declaration map.
   for (auto *object : objectsToDelete) collect(object);
   automaticObjectMap.clear();
   objectsToDelete.clear();
   if (definitionObjects)
   {
      definitionObjects->borrowedAutomatic.clear();
      definitionObjects->borrowedVariants.clear();
   }
   sandboxObjects.clear();
   for (auto *object : release) delete object;
}

//------------------------------------------------------------------------------
// void AddAutomaticObject(const std::string &withName, GmatBase *obj,
//                         bool alreadyManaged)
//------------------------------------------------------------------------------
/*
 * Adds automatic objects such as Parameter, e.g. sat.X, to automatic object map.
 * The GmatFunction::Initialize() calls Validator to validate and to create
 * ElementWrappers for commands in the FCS. The Validator creates automatic
 * Parameters if necessary and calls this method for adding. Since function can be
 * initialized more than one time multiple automatic Parameters can be created
 * which is the case for nested and recursive call.
 *
 * @param  withName  Automatic object name
 * @param  obj  Automatic object pointer
 * @param  alreadyManaged  true if the current lookup already held the object
 *
 * @note Make sure that old automatic objects are deleted properly
 */
//------------------------------------------------------------------------------
void UserDefinedFunction::AddAutomaticObject(const std::string &withName, GmatBase *obj,
                                  bool alreadyManaged)
{
   if (!obj || obj == this)
      throw FunctionException("Cannot register a null or self automatic function object.\n");
   PruneExpiredAutomaticReferences();
   if (!definitionObjects) definitionObjects.reset(new FunctionDefinitionObjects);
   auto contains = [obj](const ObjectMap *map)
   {
      if (map) for (const auto &entry : *map) if (entry.second == obj) return true;
      return false;
   };
   const bool localStore = contains(objectStore);
   const bool globalStore = contains(globalObjectStore);
   const bool declared = contains(&functionObjectMap);
   auto found = automaticObjectMap.find(withName);
   GmatBase *old = found == automaticObjectMap.end() ? nullptr : found->second;
   const bool canonical = old == obj && !IsBorrowedAutomaticObject(obj) &&
         !localStore && !globalStore;
   const bool owned = OwnsDefinitionObject(obj) || declared || canonical ||
         std::find(objectsToDelete.begin(), objectsToDelete.end(), obj) != objectsToDelete.end();
   // 'Already managed' describes a lookup hit, not its lifetime owner. A
   // persistent prototype can be found again in the temporary validator map.
   const bool borrow = !owned && (localStore || globalStore || alreadyManaged);
   if (!borrow && !OwnsDefinitionObject(obj) && !declared)
   {
      definitionObjects->owned.reserve(definitionObjects->owned.size() + 1);
      definitionObjects->automaticOwners.insert(obj);
      definitionObjects->owned.emplace_back(obj);
      objectsToDelete.erase(std::remove(objectsToDelete.begin(), objectsToDelete.end(), obj), objectsToDelete.end());
   }
   if (found == automaticObjectMap.end())
      found = automaticObjectMap.emplace(withName, nullptr).first;
   if (borrow)
   {
      bool recorded = false;
      for (const auto &entry : definitionObjects->borrowedVariants)
         if (entry.object == obj && GmatBase::IsLiveReference(obj, entry.identity)) recorded = true;
      if (!recorded)
         definitionObjects->borrowedVariants.push_back(
               {obj, obj->GetLifetimeToken(), localStore && (!obj->IsGlobal() || obj->IsLocal())});
   }
   // Keep an installed persistent prototype. A new managed FOS variant is a
   // runtime binding; a newly allocated variant has its own pool owner.
   if (!found->second)
   {
      if (borrow)
      {
         const bool invocation = localStore && (!obj->IsGlobal() || obj->IsLocal());
         definitionObjects->borrowedAutomatic.insert_or_assign(withName,
               FunctionDefinitionObjects::BorrowedAutomatic{obj, obj->GetLifetimeToken(), invocation});
      }
      else definitionObjects->borrowedAutomatic.erase(withName);
      found->second = obj;
   }
   RebuildBorrowedAutomaticObjects();
}

//------------------------------------------------------------------------------
// GmatBase* FindAutomaticObject(const std::string &name)
//------------------------------------------------------------------------------
GmatBase* UserDefinedFunction::FindAutomaticObject(const std::string &name)
{
   PruneExpiredAutomaticReferences();
   const auto found = automaticObjectMap.find(name);
   return found == automaticObjectMap.end() ? nullptr : found->second;
}


//------------------------------------------------------------------------------
// ObjectMap* GetAutomaticObjectMap()
//------------------------------------------------------------------------------
ObjectMap* UserDefinedFunction::GetAutomaticObjectMap()
{
   return &automaticObjectMap;
}


//---------------------------------
// protected
//---------------------------------
//------------------------------------------------------------------------------
// GmatBase* FindObject(const std::string &name)
//------------------------------------------------------------------------------
GmatBase* UserDefinedFunction::FindObject(const std::string &name)
{
   #ifdef DEBUG_FIND_OBJECT
   MessageInterface::ShowMessage
      ("UserDefinedFunction::FindObject() <%p>'%s' entered, name = '%s'\n", this,
       functionName.c_str(), name.c_str());
   ShowObjectMap(objectStore, "In FindObject()", "objectStore");
   ShowObjectMap(globalObjectStore, "In FindObject()", "globalObjectStore");
   ShowObjectMap(&functionObjectMap, "In FindObject()", "functionObjectMap");
   #endif
   
   std::string newName = name;
   GmatBase *obj = NULL;
   
   // Ignore array indexing of Array
   std::string::size_type index = name.find('(');
   if (index != name.npos)
      newName = name.substr(0, index);

   // Check for the object in the Local Object Store (LOS) first
   if (objectStore && objectStore->find(newName) != objectStore->end())
      obj = (*objectStore)[newName];
   
   // If not found in the LOS, check the Global Object Store (GOS)
   else if (globalObjectStore && globalObjectStore->find(newName) != globalObjectStore->end())
      obj = (*globalObjectStore)[newName];
   
   // Let's try SolarSystem (loj: 2008.06.12)
   else if (solarSys && solarSys->GetBody(newName))
      obj = (GmatBase*)(solarSys->GetBody(newName));
   
   // If still not found, try functionObjectMap
   // Should this moved to top?
   else if (functionObjectMap.find(newName) != functionObjectMap.end())
      obj = functionObjectMap[newName];
   
   #ifdef DEBUG_FIND_OBJECT
   MessageInterface::ShowMessage
      ("UserDefinedFunction::FindObject() <%p>'%s' returning <%p> for '%s'\n", this,
       functionName.c_str(), obj, name.c_str());
   #endif
   return obj;
}


//------------------------------------------------------------------------------
//bool IsAutomaticObjectGlobal(const std::string &autoObjName, GmatBase *owner)
//------------------------------------------------------------------------------
/**
 * Checks if the owner of automatic object such as Parameter is global object.
 *
 * @param autoObjName  Input object name
 * @param *owner        Owner of the automatic object if owner is found
 */
//------------------------------------------------------------------------------
bool UserDefinedFunction::IsAutomaticObjectGlobal(const std::string &autoObjName,
                                       GmatBase **owner)
{
   #ifdef DEBUG_AUTO_OBJ
   MessageInterface::ShowMessage
      ("UserDefinedFunction::IsAutomaticObjectGlobal() entered, autoObjName='%s'\n",
       autoObjName.c_str());
   #endif
   
   *owner = NULL;
   std::string type, ownerName, dep;
   GmatStringUtil::ParseParameter(autoObjName, type, ownerName, dep);
   #ifdef DEBUG_AUTO_OBJ
   MessageInterface::ShowMessage("   ownerName='%s'\n", ownerName.c_str());
   #endif
   
   bool isParamOwnerGlobal = false;

   // First check if we are using a local version, then check global
   if (objectStore->find(ownerName) != objectStore->end())
      *owner = (objectStore->find(ownerName))->second;
   else if (globalObjectStore->find(ownerName) != globalObjectStore->end())
   {
      *owner = (globalObjectStore->find(ownerName))->second;
      isParamOwnerGlobal = true;
   }
   
   #ifdef DEBUG_AUTO_OBJ
   MessageInterface::ShowMessage
      ("UserDefinedFunction::IsAutomaticObjectGlobal() leaving, autoObjName='%s' is %s, ",
       autoObjName.c_str(), isParamOwnerGlobal ? "global" : "not global");
   MessageInterface::ShowMessage(GmatBase::WriteObjectInfo("owner=", (*owner)));
   #endif
   
   return isParamOwnerGlobal;
}

//------------------------------------------------------------------------------
// void ShowObjects(const std::string &title)
//------------------------------------------------------------------------------
void UserDefinedFunction::ShowObjects(const std::string &title)
{
   MessageInterface::ShowMessage("%s\n", title.c_str());
   MessageInterface::ShowMessage("this=<%p>, functionName='%s'\n", this, functionName.c_str());
   MessageInterface::ShowMessage("========================================\n");
   MessageInterface::ShowMessage("solarSys          = <%p>\n", solarSys);
   MessageInterface::ShowMessage("internalCoordSys  = <%p>\n", internalCoordSys);
   MessageInterface::ShowMessage("forces            = <%p>\n", forces);
   MessageInterface::ShowMessage("sandboxObjects.size() = %d\n", sandboxObjects.size());
   for (UnsignedInt i = 0; i < sandboxObjects.size(); i++)
   {
      GmatBase *obj = sandboxObjects[i];
      MessageInterface::ShowMessage
         ("   <%p>[%s]'%s'", obj, obj ? obj->GetTypeName().c_str() : "NULL",
          obj ? obj->GetName().c_str() : "NULL");
      if (obj->IsOfType(Gmat::PARAMETER))
      {
         GmatBase *paramOwner = ((Parameter*)obj)->GetOwner();
         MessageInterface::ShowMessage
            ("   paramOwner = <%p>[%s]'%s'\n", paramOwner,
             paramOwner ? paramOwner->GetTypeName().c_str() : "NULL",
             paramOwner ? paramOwner->GetName().c_str() : "NULL");
      }
      else
      {
         MessageInterface::ShowMessage("\n");
      }
   }
   ShowObjectMap(&functionObjectMap, "ShowObjects()", "functionObjectMap");
   ShowObjectMap(&automaticObjectMap, "ShowObjects()", "automaticObjectMap");
   ShowObjectMap(objectStore, "ShowObjects()", "objectStore");
   ShowObjectMap(globalObjectStore, "ShowObjects()", "globalObjectStore");
   MessageInterface::ShowMessage("========================================\n");
}

