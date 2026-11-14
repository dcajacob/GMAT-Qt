//$Id$
//------------------------------------------------------------------------------
//                                  Subscriber
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
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under contract
// number S-67573-G
//
// *** File Name : Subscriber.cpp
// *** Created   : May 20, 2003
// **************************************************************************
// ***  Developed By  :  Thinking Systems, Inc. (www.thinksysinc.com)     ***
// ***  For:  Flight Dynamics Analysis Branch (Code 595)                  ***
// ***  Under Contract:  P.O.  GSFC S-67573-G                             ***
// ***                                                                    ***
// ***  Header Version: July 12, 2002                                     ***
// **************************************************************************
// Module Type               : ANSI C++ Source
// Development Environment   : Visual C++ 7.0
// Modification History      : 5/20/2003 - D. Conway, Thinking Systems, Inc.
//                             Original delivery
//
//                           : 10/01/2003 - L. Ruley, Missions Applications Branch
//                             Updated style using GMAT cpp style guide
// **************************************************************************
//
/**
 * Implementation code for the Subscriber base class
 */
//------------------------------------------------------------------------------

#include "Subscriber.hpp"
#include "NativeValueProvider.hpp"

namespace
{
const NativeValueProviderRegistration subscriberValueProvider({
   Subscriber::NativeValueProviderDepth,
   [](const GmatBase *object) { return dynamic_cast<const Subscriber*>(object) != nullptr; },
   [](const GmatBase *object) { return dynamic_cast<const Subscriber*>(object)->GetFunctionValueOwnedObjectsImpl(); },
   [](GmatBase *object, const GmatBase &source, SolarSystem *system, const std::function<GmatBase*(GmatBase*)> &resolve) { return dynamic_cast<Subscriber*>(object)->RebindFunctionValueReferencesImpl(source, system, resolve); },
   nullptr,
   nullptr,
   nullptr
});
}
#include "Publisher.hpp"
#include "SubscriberException.hpp"
#include "Parameter.hpp"
#include "VariableWrapper.hpp"
#include "StringUtil.hpp"          // for Replace()
#include "MessageInterface.hpp"
#include "Function.hpp"
#include "PlotInterface.hpp"
#include "SubscriberBindingState.hpp"
#include "FunctionManager.hpp"
#include <atomic>
#include <limits>
#include <algorithm>
#include <map>
#include <set>
#include <typeinfo>

// Uncomment this line if cloning wrapper code is ready
#define __ENABLE_CLONING_WRAPPERS__

//#define DEBUG_SUBSCRIBER
//#define DEBUG_SUBSCRIBER_PARAM
//#define DEBUG_SUBSCRIBER_LABELS
//#define DEBUG_RECEIVE_DATA
//#define DEBUG_RENAME
//#define DEBUG_WRAPPER_CODE
//#define DEBUG_SKIP_DATA
//#define DEBUG_SUBSCRIBER_PUT
//#define DEBUG_SUBSCRIBER_SET
//#ifndef DEBUG_MEMORY
//#define DEBUG_MEMORY
//#endif

#ifdef DEBUG_MEMORY
#include "MemoryTracker.hpp"
#endif

namespace
{
std::atomic<std::uint64_t> nextOutputIdentity{0};
std::uint64_t NewOutputIdentity()
{
   auto current = nextOutputIdentity.load();
   do
   {
      if (current == std::numeric_limits<std::uint64_t>::max())
         throw SubscriberException("Subscriber output identity cannot be represented.\n");
   } while (!nextOutputIdentity.compare_exchange_weak(current, current + 1));
   return current + 1;
}
thread_local SharedSubscriberBindings *activeSubscriberBindings = nullptr;
}

struct SubscriberValueCopyState
{
   std::shared_ptr<const FunctionValueContext> oldContext, sourceContext;
   std::shared_ptr<SubscriberValueCopyState> previous;
   // Retire wrappers before their context/older captured dependencies.
   std::vector<ElementWrapper::InvocationLease> wrappers;
};

struct SharedSubscriberBindings::State
{
   struct Entry
   {
      Subscriber *subscriber;
      std::weak_ptr<const void> identity;
      std::unique_ptr<FunctionInvocationState> bindings;
      bool committed = false;
   };
   std::vector<Entry> entries;
   bool restored = false, successful = true;
};

SharedSubscriberBindings::SharedSubscriberBindings(ObjectMap *local, ObjectMap *global) :
   state(std::make_unique<State>()), previous(activeSubscriberBindings)
{
   activeSubscriberBindings = this;
   std::set<Subscriber*> captured;
   try
   {
      for (const auto *store : {local, global})
         if (store)
            for (const auto &entry : *store)
               if (auto *subscriber = dynamic_cast<Subscriber*>(entry.second))
                  if (captured.insert(subscriber).second)
                     state->entries.push_back({subscriber, subscriber->GetLifetimeToken(),
                           subscriber->SuspendFunctionBindings(), false});
   }
   catch (...) { Restore(); throw; }
}

void SharedSubscriberBindings::Unlink() noexcept
{
   auto **link = &activeSubscriberBindings;
   while (*link)
   {
      if (*link == this) { *link = previous; return; }
      link = &(*link)->previous;
   }
}

void SharedSubscriberBindings::Commit(Subscriber &subscriber) noexcept
{
   for (auto *scope = activeSubscriberBindings; scope; scope = scope->previous)
      for (auto &entry : scope->state->entries)
         if (!entry.committed && entry.subscriber == &subscriber &&
               GmatBase::IsLiveReference(entry.subscriber, entry.identity))
         {
            entry.committed = true;
            if (entry.bindings) entry.bindings->Dismiss();
         }
}

bool SharedSubscriberBindings::Restore() noexcept
{
   if (state->restored) return state->successful;
   state->restored = true;
   for (auto entry = state->entries.rbegin(); entry != state->entries.rend(); ++entry)
      if (entry->bindings && !entry->bindings->Restore()) state->successful = false;
   // Destructors can trigger nested cleanup. Retired entries must not remain
   // visible in the registry while their captured owners are being released.
   Unlink();
   state->entries.clear();
   return state->successful;
}

SharedSubscriberBindings::~SharedSubscriberBindings() noexcept
{
   Restore();
}

void Subscriber::CommitFunctionValueBindings() noexcept
{
   SharedSubscriberBindings::Commit(*this);
}

//---------------------------------
// static data
//---------------------------------

/// Available show foot print options
StringArray Subscriber::solverIterOptions;

const std::string
Subscriber::SOLVER_ITER_OPTION_TEXT[SolverIterOptionCount] =
{
   "All", "Current", "None",
};

const std::string
Subscriber::PARAMETER_TEXT[SubscriberParamCount - GmatBaseParamCount] =
{
   "SolverIterations",
   "TargetStatus",
   "UpperLeft",
   "Size",
   "RelativeZOrder",
   "Minimized",
   "Maximized",
};

const Gmat::ParameterType
Subscriber::PARAMETER_TYPE[SubscriberParamCount - GmatBaseParamCount] =
{
   Gmat::ENUMERATION_TYPE,  // "SolverIterations"
   Gmat::ON_OFF_TYPE,       // "TargetStatus"
   Gmat::RVECTOR_TYPE,      // "UpperLeft"
   Gmat::RVECTOR_TYPE,      // "Size"
   Gmat::INTEGER_TYPE,      // "RelativeZOrder"
   Gmat::BOOLEAN_TYPE,      // "Minimized"
   Gmat::BOOLEAN_TYPE,      // "Maximized"
};


//---------------------------------
//  public methods
//---------------------------------

//------------------------------------------------------------------------------
// Subscriber(const std::string &typeStr, const std::string &nomme)
//------------------------------------------------------------------------------
Subscriber::Subscriber(const std::string &typeStr, const std::string &nomme) :
   GmatBase (Gmat::SUBSCRIBER, typeStr, nomme),
   data                  (NULL),
   next                  (NULL),
   theInternalCoordSystem(NULL),
   theDataCoordSystem    (NULL),
   theDataMJ2000EqOrigin (NULL),
   theSolarSystem        (NULL),
   currentProvider       (NULL),
   active                (true),
   isManeuvering         (false),
   isEndOfReceive        (false),
   isEndOfDataBlock      (false),
   isEndOfRun            (false),
   isInitialized         (false),
   isFinalized           (false),
   isDataOn              (true),
   isDataStateChanged    (false),
   relativeZOrder        (0),
   isMaximized           (false),
   isMinimized			    (false),
   runstate              (Gmat::IDLE),
   prevRunState          (Gmat::IDLE),
   currProviderId        (0),
   mPlotUpperLeft        (2,0.0,0.0),
   mPlotSize             (2,0.0,0.0),
   propDirection         (1.0)
{
   runtimeOutputIdentity = NewOutputIdentity();
   objectTypes.push_back(Gmat::SUBSCRIBER);
   objectTypeNames.push_back("Subscriber");
   
   mSolverIterations = "Current";
   mSolverIterOption = SI_CURRENT;
   
   wrappersCopied = false;
   
   solverIterOptions.clear();
   for (UnsignedInt i = 0; i < SolverIterOptionCount; i++)
      solverIterOptions.push_back(SOLVER_ITER_OPTION_TEXT[i]);

}


//------------------------------------------------------------------------------
// Subscriber(const Subscriber &copy)
//------------------------------------------------------------------------------
Subscriber::Subscriber(const Subscriber &copy) :
   GmatBase(copy),
   data                  (NULL),
   next                  (NULL),
   theInternalCoordSystem(copy.theInternalCoordSystem),
   theDataCoordSystem    (copy.theDataCoordSystem),
   theDataMJ2000EqOrigin (copy.theDataMJ2000EqOrigin),
   theSolarSystem        (copy.theSolarSystem),
   currentProvider       (NULL),
   active                (copy.active),
   isManeuvering         (copy.isManeuvering),
   isEndOfReceive        (copy.isEndOfReceive),
   isEndOfDataBlock      (copy.isEndOfDataBlock),
   isEndOfRun            (copy.isEndOfRun),
   isInitialized         (false),
   isFinalized           (false),
   isDataOn              (copy.isDataOn),
   isDataStateChanged    (copy.isDataStateChanged),
   relativeZOrder        (copy.relativeZOrder),
   isMaximized           (copy.isMaximized),
   isMinimized			    (copy.isMinimized),
   runstate              (copy.runstate),
   prevRunState          (copy.prevRunState),
   currProviderId        (copy.currProviderId),
   propDirection         (copy.propDirection),
   xWrapperObjectNames   (copy.xWrapperObjectNames),
   yWrapperObjectNames   (copy.yWrapperObjectNames),
   allWrapperObjectNames (copy.allWrapperObjectNames)
{
   GmatBase::isInitialized = false;
   runtimeOutputIdentity = NewOutputIdentity();
   mSolverIterations = copy.mSolverIterations;
   mSolverIterOption = copy.mSolverIterOption;
   wrappersCopied    = true;
   
   mPlotUpperLeft    = Rvector(2,copy.mPlotUpperLeft[0], copy.mPlotUpperLeft[1]);
   mPlotSize         = Rvector(2,copy.mPlotSize[0],      copy.mPlotSize[1]);
   
   #ifdef DEBUG_WRAPPER_CODE
   MessageInterface::ShowMessage
      ("Subscriber(copy) entered, copy.xParamWrappers.size()=%d, "
       "copy.yParamWrappers.size()=%d\n", copy.xParamWrappers.size(),
       copy.yParamWrappers.size());
   #endif
   
#ifdef __ENABLE_CLONING_WRAPPERS__
   CopyWrapperGraph(copy);
   #ifdef DEBUG_WRAPPER_CODE
   MessageInterface::ShowMessage("Subscriber(copy) cloned wrappers\n");
   WriteWrappers();
   #endif
#else
   // Copy wrappers
   xParamWrappers = copy.xParamWrappers;
   yParamWrappers = copy.yParamWrappers;
   #ifdef DEBUG_WRAPPER_CODE
   MessageInterface::ShowMessage("Subscriber(copy) copied wrappers\n");
   WriteWrappers();
   #endif
#endif
}


//------------------------------------------------------------------------------
// Subscriber& Subscriber::operator=(const Subscriber& rhs)
//------------------------------------------------------------------------------
/**
 * The assignment operator
 */
//------------------------------------------------------------------------------
Subscriber& Subscriber::operator=(const Subscriber& rhs)
{
   if (this == &rhs)
      return *this;
   const auto previousContext = GetFunctionValueContext();
   functionValueRolesBound = false;
   
   GmatBase::operator=(rhs);
   
   data = nullptr;
   // The destination keeps its actual subscriber-chain membership.
   
   theInternalCoordSystem = rhs.theInternalCoordSystem;
   theDataCoordSystem = rhs.theDataCoordSystem;
   theDataMJ2000EqOrigin = rhs.theDataMJ2000EqOrigin;
   theSolarSystem = rhs.theSolarSystem;
   currentProvider = NULL;
   
   active = rhs.active;
   isManeuvering = rhs.isManeuvering;
   isEndOfReceive = rhs.isEndOfReceive;
   isEndOfDataBlock = rhs.isEndOfDataBlock;
   isEndOfRun = rhs.isEndOfRun;
   // Readiness belongs to this actual resource and its output, not its peer.
   isInitialized = false;
   GmatBase::isInitialized = false;
   isFinalized = false;
   isDataOn = rhs.isDataOn;
   isDataStateChanged = rhs.isDataStateChanged;

   mPlotUpperLeft     = rhs.mPlotUpperLeft;
   mPlotSize          = rhs.mPlotSize;
   relativeZOrder     = rhs.relativeZOrder;
   isMinimized        = rhs.isMinimized;
   isMaximized        = rhs.isMaximized;

   runstate = rhs.runstate;
   prevRunState = rhs.prevRunState;
   currProviderId = rhs.currProviderId;
   propDirection = rhs. propDirection;
   
   xWrapperObjectNames = rhs.xWrapperObjectNames;
   yWrapperObjectNames = rhs.yWrapperObjectNames;
   allWrapperObjectNames = rhs.allWrapperObjectNames;
   mSolverIterations = rhs.mSolverIterations;
   mSolverIterOption = rhs.mSolverIterOption;
   wrappersCopied = true;
   
#ifdef __ENABLE_CLONING_WRAPPERS__
   // Stage the whole graph before retiring either axis. Aliases across axes
   // must continue to select one independent copied wrapper.
   CopyWrapperGraph(rhs);
   #ifdef DEBUG_WRAPPER_CODE
   MessageInterface::ShowMessage("Subscriber(=) cloned wrappers\n");
   WriteWrappers();
   #endif
#else
   // Copy wrappers
   xParamWrappers = rhs.xParamWrappers;
   yParamWrappers = rhs.yParamWrappers;
   #ifdef DEBUG_WRAPPER_CODE
   MessageInterface::ShowMessage("Subscriber(=) copied wrappers\n");
   WriteWrappers();
   #endif
#endif
   
   invocationWrapperLeases.clear();
   return *this;
}


//------------------------------------------------------------------------------
// ~Subscriber()
//------------------------------------------------------------------------------
Subscriber::~Subscriber()
{
   Publisher::SetIntegrationStepConsumer(this, false);
#ifdef __ENABLE_CLONING_WRAPPERS__
   ClearWrappers(true, true);
#else
   #ifdef DEBUG_WRAPPER_CODE
   MessageInterface::ShowMessage
      ("~Subscriber() <%p>'%s' entered, wrappers%s\n", this, GetName().c_str(),
       wrappersCopied ? " copied" : " did not copied");
   #endif
   
   // Since we just copies wrappers, we can only delete if it is not a cloned
   if (!wrappersCopied)
   {
      ClearWrappers(true, true);
   }
   else
   {
      //@todo We should delete wrappers here
      //Func_BallisticMassParamTest.script leaves memory trace
      //If we clear wrappers, APT_AttitudeTest.script crashes
      //ClearWrappers();
   }
#endif
   for (const auto &entry : runtimeOutputNames) PlotInterface::ForgetPlotDisplayName(entry.second);
}


//------------------------------------------------------------------------------
// bool Initialize()
//------------------------------------------------------------------------------
void Subscriber::ConfigureRuntimeOutput(bool privateOutput)
{
   if (runtimeOutputStarted) return;
   if (privateOutput)
   {
      if (!runtimeOutputIdentity) runtimeOutputIdentity = NewOutputIdentity();
   }
   else
   {
      for (const auto &entry : runtimeOutputNames) PlotInterface::ForgetPlotDisplayName(entry.second);
      runtimeOutputNames.clear();
      runtimeOutputIdentity = 0;
   }
}

const std::string& Subscriber::GetRuntimePlotName() const
{
   return GetRuntimePlotName(instanceName);
}

const std::string& Subscriber::GetRuntimePlotName(const std::string &logicalName) const
{
   static const std::string empty;
   if (logicalName.empty()) return empty;
   if (!runtimeOutputIdentity)
   {
      // A cold copy never owned the source object's previous window name.
      if (!runtimeOutputCreated && logicalName != instanceName) return empty;
      return logicalName;
   }
   auto found = runtimeOutputNames.find(logicalName);
   if (found == runtimeOutputNames.end())
      found = runtimeOutputNames.emplace(logicalName,
            logicalName + "::gmat-output-" + std::to_string(runtimeOutputIdentity)).first;
   PlotInterface::SetPlotDisplayName(found->second, logicalName);
   return found->second;
}

bool Subscriber::InitializeForRun()
{
   runtimeOutputStarted = true;
   try
   {
      const bool initialized = Initialize();
      isInitialized = initialized;
      GmatBase::isInitialized = initialized;
      runtimeOutputCreated = runtimeOutputCreated || initialized;
      return initialized;
   }
   catch (...)
   {
      isInitialized = false;
      GmatBase::isInitialized = false;
      throw;
   }
}

bool Subscriber::HasSubscriberValueReferenceProvider() const
{
   return false;
}

bool Subscriber::HasFunctionValueBindings()
{
   if (!GetFunctionValueContext() || !HasSubscriberValueReferenceProvider() || !functionValueRolesBound) return false;
   GetWrapperObjectNameArray(); // Refresh role lists from the actual declaration.
   if (functionValueXRoles != xWrapperObjectNames || functionValueYRoles != yWrapperObjectNames) return false;
   auto matches = [](const StringArray &names, const WrapperArray &wrappers)
   {
      if (names.size() != wrappers.size()) return false;
      for (size_t i = 0; i < names.size(); ++i)
         if (!wrappers[i]) return false;
      return true;
   };
   return matches(xWrapperObjectNames, xParamWrappers) &&
         matches(yWrapperObjectNames, yParamWrappers) && HasNativeSubscriberBindings();
}

bool Subscriber::HasNativeSubscriberBindings() const
{
   return true;
}

bool Subscriber::EnsureFunctionValueOutput()
{
   if (isInitialized || !GetFunctionValueContext() || !HasSubscriberValueReferenceProvider()) return true;
   if (!HasFunctionValueBindings())
      throw SubscriberException("Copied subscriber declaration needs wrapper rebuilding.\n");
   return InitializeForRun();
}

Subscriber::SubscriberValueCopyToken Subscriber::BeginSubscriberValueCopy(const Subscriber &source)
{
   if (&source == this || IsNativeFunctionValueCopy()) return {};
   auto saved = std::make_shared<SubscriberValueCopyState>();
   saved->oldContext = GetFunctionValueContext();
   saved->sourceContext = source.GetFunctionValueContext();
   saved->previous = copyRetirements;
   const WrapperArray *arrays[] = {&xParamWrappers, &yParamWrappers, &oldWrappersToDelete,
         &source.xParamWrappers, &source.yParamWrappers, &source.oldWrappersToDelete};
   std::set<ElementWrapper*> seen;
   for (const auto *array : arrays) for (auto *wrapper : *array)
      if (wrapper && seen.insert(wrapper).second)
      {
         auto lease = ElementWrapper::RetainInvocationWrapper(wrapper);
         if (ElementWrapper::GetInvocationWrapper(lease) != wrapper)
            throw SubscriberException("Cannot copy a retired subscriber wrapper.\n");
         saved->wrappers.push_back(std::move(lease));
      }
   copyRetirements = saved;
   return saved;
}

void Subscriber::ReconnectSubscriberCopy(const Subscriber &source)
{
   if (&source == this || IsNativeFunctionValueCopy()) return;
   if (source.GetFunctionValueContext()) RebindFunctionValueContext(source);
   else
   {
      // Reuse the complete retained graph builder for native copies as well:
      // nested wrapper-owned resources disclose their own owned components.
      FunctionManager::RebindSubscriberCopy(source, *this, source.theSolarSystem);
   }
}

void Subscriber::CompleteSubscriberValueCopy(const Subscriber &source,
      const SubscriberValueCopyToken &transaction)
{
   if (&source == this || IsNativeFunctionValueCopy()) return;
   ReconnectSubscriberCopy(source);
   CommitFunctionValueBindings();
   // Detach before retirement callbacks. A nested copy owns its own token and
   // must not have its captured state removed by the enclosing completion.
   if (transaction && copyRetirements == transaction) copyRetirements.reset();
}

ObjectArray Subscriber::GetFunctionValueOwnedObjectsImpl() const
{
   ObjectArray objects;
   std::set<ElementWrapper*> seen;
   for (const auto *axis : {&xParamWrappers, &yParamWrappers})
      for (auto *wrapper : *axis)
         if (wrapper && seen.insert(wrapper).second)
            wrapper->CollectOwnedReferences(objects);
   return objects;
}

void Subscriber::GetClonedOwnedObjects(const Subscriber &target,
      ObjectArray &originals, ObjectArray &values) const
{
   if (this == &target || typeid(*this) != typeid(target))
      throw SubscriberException("Incompatible subscriber component inventory.\n");
   std::map<ElementWrapper*, ElementWrapper*> aliases, reverse;
   std::map<GmatBase*, GmatBase*> components;
   std::map<GmatBase*, GmatBase*> componentSources;
   for (const auto &axes : {std::make_pair(&xParamWrappers, &target.xParamWrappers),
         std::make_pair(&yParamWrappers, &target.yParamWrappers)})
   {
      if (axes.first->size() != axes.second->size())
         throw SubscriberException("Copied subscriber wrapper collection changed shape.\n");
      for (size_t i = 0; i < axes.first->size(); ++i)
      {
         auto *from = (*axes.first)[i];
         auto *to = (*axes.second)[i];
         if (!from && !to) continue;
         if (!from || !to || from == to || typeid(*from) != typeid(*to))
            throw SubscriberException("Missing independently copied subscriber wrapper.\n");
         const auto previous = aliases.find(from);
         if (previous != aliases.end())
         {
            if (previous->second != to)
               throw SubscriberException("Copied subscriber split a wrapper alias.\n");
            continue;
         }
         if (!reverse.emplace(to, from).second)
            throw SubscriberException("Copied subscriber merged distinct wrappers.\n");
         aliases.emplace(from, to);
         ObjectArray sourceComponents, targetComponents;
         to->CollectClonedOwnedReferences(*from, sourceComponents, targetComponents);
         if (sourceComponents.size() != targetComponents.size())
            throw SubscriberException("Copied subscriber component collection changed shape.\n");
         for (size_t j = 0; j < sourceComponents.size(); ++j)
         {
            auto *original = sourceComponents[j];
            auto *value = targetComponents[j];
            if (!original || !value || original == value ||
                  original->GetTypeName() != value->GetTypeName())
               throw SubscriberException("Missing independent subscriber component.\n");
            const auto role = componentSources.emplace(value, original);
            if (!role.second && role.first->second != original)
               throw SubscriberException("Copied subscriber merged distinct owned components.\n");
            const auto inserted = components.emplace(sourceComponents[j], targetComponents[j]);
            if (!inserted.second && inserted.first->second != targetComponents[j])
               throw SubscriberException("Copied subscriber split an owned component alias.\n");
         }
      }
   }
   for (const auto &entry : components)
   {
      const auto found = std::find(originals.begin(), originals.end(), entry.first);
      if (found != originals.end())
      {
         if (values.at(found - originals.begin()) != entry.second)
            throw SubscriberException("Conflicting subscriber component copies.\n");
      }
      else { originals.push_back(entry.first); values.push_back(entry.second); }
   }
}

void Subscriber::PrepareClonedSubscriberValue(const Subscriber &source)
{
   if (&source == this || typeid(*this) != typeid(source))
      throw SubscriberException("Incompatible subscriber value copy.\n");
   // Optional builds can use borrowed native wrappers. Retained function
   // values always need an independent, alias-preserving wrapper graph.
   bool borrowed = false;
   for (const auto &axes : {std::make_pair(&source.xParamWrappers, &xParamWrappers),
         std::make_pair(&source.yParamWrappers, &yParamWrappers)})
      if (axes.first->size() == axes.second->size())
         for (size_t i = 0; i < axes.first->size(); ++i)
            borrowed = borrowed || ((*axes.first)[i] && (*axes.first)[i] == (*axes.second)[i]);
   if (borrowed) CopyWrapperGraph(source);
   ObjectArray from, to;
   source.GetClonedOwnedObjects(*this, from, to);
   isInitialized = false;
   GmatBase::isInitialized = false;
   isFinalized = false;
   functionValueRolesBound = false;
}

void Subscriber::RebindSubscriberEnvironment(const Subscriber &source,
      const std::function<GmatBase*(GmatBase*)> &resolve)
{
   if (&source == this || typeid(*this) != typeid(source))
      throw SubscriberException("Incompatible subscriber environment copy.\n");
   auto *internal = MapSubscriberReference(source.theInternalCoordSystem, resolve);
   auto *coordinates = MapSubscriberReference(source.theDataCoordSystem, resolve);
   auto *origin = MapSubscriberReference(source.theDataMJ2000EqOrigin, resolve);
   auto *system = MapSubscriberReference(source.theSolarSystem, resolve);
   ObjectArray from, to;
   source.GetClonedOwnedObjects(*this, from, to);
   std::set<ElementWrapper*> seen;
   for (const auto &axes : {std::make_pair(&source.xParamWrappers, &xParamWrappers),
         std::make_pair(&source.yParamWrappers, &yParamWrappers)})
      for (size_t i = 0; i < axes.first->size(); ++i)
      {
         auto *wrapper = (*axes.second)[i];
         if (wrapper && seen.insert(wrapper).second)
            wrapper->RebindClonedReferences(*(*axes.first)[i], resolve);
      }
   theInternalCoordSystem = internal;
   theDataCoordSystem = coordinates;
   theDataMJ2000EqOrigin = origin;
   theSolarSystem = system;
   // A value does not retain an active publisher packet or linked-list entry.
   data = nullptr;
   currentProvider = nullptr;
   isInitialized = false;
   GmatBase::isInitialized = false;
}

void Subscriber::RebindSubscriberValueReferences(const Subscriber &source,
      const std::function<GmatBase*(GmatBase*)> &resolve)
{
   RebindSubscriberEnvironment(source, resolve);
}

bool Subscriber::RebindFunctionValueReferencesImpl(const GmatBase &original,
      SolarSystem *, const std::function<GmatBase*(GmatBase*)> &resolve)
{
   if (!HasSubscriberValueReferenceProvider()) return false;
   const auto *source = dynamic_cast<const Subscriber*>(&original);
   if (!source || !source->HasSubscriberValueReferenceProvider() ||
         typeid(*source) != typeid(*this))
      throw SubscriberException("Incompatible retained subscriber reference source.\n");
   RebindSubscriberValueReferences(*source, resolve);
   GetWrapperObjectNameArray();
   StringArray xRoles(xWrapperObjectNames), yRoles(yWrapperObjectNames);
   functionValueXRoles.swap(xRoles);
   functionValueYRoles.swap(yRoles);
   functionValueRolesBound = true;
   return true;
}

bool Subscriber::Initialize()
{
   #ifdef DEBUG_INIT
   MessageInterface::ShowMessage
      ("Subscriber::Initialize() <%p>'%s' entered, IsGlobal=%d\n", this,
       GetName().c_str(), IsGlobal());
   #endif
   
   if (!IsGlobal())
   {
      isEndOfReceive = false;
      isEndOfDataBlock = false;
      isEndOfRun = false;
      isInitialized = false;
      isFinalized = false;
      isDataOn = true;
      isDataStateChanged = false;
   }
   
   #ifdef DEBUG_INIT
   MessageInterface::ShowMessage
      ("Subscriber::Initialize() <%p>'%s' returning true\n", this, GetName().c_str());
   #endif
   return true;
}


//------------------------------------------------------------------------------
// bool IsInitialized()
//------------------------------------------------------------------------------
bool Subscriber::IsInitialized()
{
   return isInitialized;
}


//------------------------------------------------------------------------------
// bool ReceiveData(const char *datastream)
//------------------------------------------------------------------------------
bool Subscriber::ReceiveData(const char *datastream)
{
   if (!active)        // Not currently processing data
      return true;
   
   data = datastream;
   return true;
}


//------------------------------------------------------------------------------
// bool ReceiveData(const char *datastream,  const int len)
//------------------------------------------------------------------------------
bool Subscriber::ReceiveData(const char *datastream,  const int len)
{
   #ifdef DEBUG_RECEIVE_DATA
   MessageInterface::ShowMessage
      ("Subscriber::ReceiveData(char*, int) <%p>'%s' entered, active=%d, len=%d\n"
       "data='%s'\n", this, GetName().c_str(), active, len, datastream);
   #endif
   
   // Report command should be able to write headers even when ReportFile is not
   // writing data (ShowReport = false), so commented out (LOJ:2012.06.13 - code QA)
   // if (!active)        // Not currently processing data
   // {
   //    #ifdef DEBUG_RECEIVE_DATA
   //    MessageInterface::ShowMessage
   //       ("Subscriber::ReceiveData() '%s' is not active, so just returning true\n",
   //        GetName().c_str());
   //    #endif
   //    return true;
   // }
   
   
   if (len > 0 && !EnsureFunctionValueOutput()) return false;
   data = datastream;
   if (!Distribute(len))
   {
      data = NULL;
      #ifdef DEBUG_RECEIVE_DATA
      MessageInterface::ShowMessage
         ("Subscriber::ReceiveData() '%s' failed to distribute, so just returning false\n",
          GetName().c_str());
      #endif
      return false;
   }
   
   data = NULL;
   
   #ifdef DEBUG_RECEIVE_DATA
   MessageInterface::ShowMessage
      ("Subscriber::ReceiveData() '%s' was successful, returning true\n",
       GetName().c_str());
   #endif
   
   return true;
}


//------------------------------------------------------------------------------
// bool ReceiveData(const Real *datastream, const int len)
//------------------------------------------------------------------------------
bool Subscriber::ReceiveData(const Real *datastream, const int len)
{
   #ifdef DEBUG_RECEIVE_DATA
   MessageInterface::ShowMessage
      ("Subscriber::ReceiveData(double*) <%p>'%s' entered, active=%d, len=%d\n",
       this, GetName().c_str(), active, len);
   if (len > 0)
      MessageInterface::ShowMessage("   data[0]=%f\n", datastream[0]);
   #endif
   
   if (!active)        // Not currently processing data
   {
      #ifdef DEBUG_RECEIVE_DATA
      MessageInterface::ShowMessage
         ("Subscriber::ReceiveData() '%s' is not active, so just returning true\n",
          GetName().c_str());
      #endif
      return true;
   }
   
   if (len == 0)
      return true;
   if (!EnsureFunctionValueOutput()) return false;
   if (!Distribute(datastream, len))
   {
      return false;
   }
   
   return true;
}


//------------------------------------------------------------------------------
// bool FlushData(bool endOfDataBlock = true)
//------------------------------------------------------------------------------
bool Subscriber::FlushData(bool endOfDataBlock)
{
   isEndOfDataBlock = endOfDataBlock;
   isEndOfReceive = true;
   Distribute(0);
   Distribute(NULL, 0);
   isEndOfReceive = false;
   
   return true;
}


//------------------------------------------------------------------------------
// bool SetEndOfRun()
//------------------------------------------------------------------------------
bool Subscriber::SetEndOfRun()
{
   struct EndRunFlags
   {
      bool &receive, &block, &run;
      ~EndRunFlags() { receive = block = run = false; }
   } reset{isEndOfReceive, isEndOfDataBlock, isEndOfRun};
   isEndOfReceive = true;
   isEndOfDataBlock = true;
   isEndOfRun = true;
   Distribute(0);
   Distribute(NULL, 0);
   return true;
}


//------------------------------------------------------------------------------
// void SetRunState(Gmat::RunState rs)
//------------------------------------------------------------------------------
/**
 * Sets the run state to the input value.
 * 
 * @param rs The new run state.
 */
//------------------------------------------------------------------------------
void Subscriber::SetRunState(Gmat::RunState rs)
{
   #ifdef DEBUG_RUN_STATE
   MessageInterface::ShowMessage
      ("Subscriber::SetRunState() setting new runstate to %d, prevRunState=%d\n",
       rs, prevRunState);
   #endif
   prevRunState = runstate;
   runstate = rs;
}


//------------------------------------------------------------------------------
// void SetManeuvering(GmatBase *originator, bool flag, Real epoch,
//                     const std::string &satName, const std::string &desc)
//------------------------------------------------------------------------------
/**
 * Sets spacecraft maneuvering flag.
 * 
 * @param originator  The maneuver command pointer who is maneuvering
 * @param flag  Set to true if maneuvering
 * @param epoch  Epoch of maneuver
 * @param satName  Name of the maneuvering spacecraft
 * @param desc  Description of maneuver (e.g. impulsive or finite)
 */
//------------------------------------------------------------------------------
void Subscriber::SetManeuvering(GmatBase *originator, bool flag, Real epoch,
                                const std::string &satName,
                                const std::string &desc)
{
   StringArray satNames;
   isManeuvering = flag;
   satNames.push_back(satName);
   HandleManeuvering(originator, flag, epoch, satNames, desc);
}


//------------------------------------------------------------------------------
// void SetManeuvering(GmatBase *originator, bool flag, Real epoch,
//                     const StringArray &satNames, const std::string &desc)
//------------------------------------------------------------------------------
/**
 * Sets spacecraft maneuvering flag.
 * 
 * @param originator  The maneuver command pointer who is maneuvering
 * @param flag Set to true if maneuvering
 * @param epoch Epoch of maneuver
 * @param satNames Names of the maneuvering spacecraft
 * @param desc Description of maneuver (e.g. impulsive or finite)
 */
//------------------------------------------------------------------------------
void Subscriber::SetManeuvering(GmatBase *originator, bool flag, Real epoch,
                                const StringArray &satNames,
                                const std::string &desc)
{
   isManeuvering = flag;
   HandleManeuvering(originator, flag, epoch, satNames, desc);
}


//------------------------------------------------------------------------------
// void SetSpacecraftPropertyChanged(GmatBase *originator, Real epoch,
//                           const std::string &satName, const std::string &desc)
//------------------------------------------------------------------------------
/**
 * Sets spacecraft property change.
 * 
 * @param originator  The assignment command pointer who is setting
 * @param epoch  Epoch of spacecraft at property change
 * @param satName  Name of the spacecraft
 * @param desc  Description of property change
 */
//------------------------------------------------------------------------------
void Subscriber::SetSpacecraftPropertyChanged(GmatBase *originator, Real epoch,
                                              const std::string &satName,
                                              const std::string &desc)
{
   HandleSpacecraftPropertyChange(originator, epoch, satName, desc);
}


//------------------------------------------------------------------------------
// void SetOrbitColorChanged(GmatBase *originator, const std::string &newColor, ...)
//------------------------------------------------------------------------------
/**
 * Sets object orbit color change.
 * 
 * @param originator  The assignment command pointer who is setting
 * @param newColor  New color to be applied to the object
 * @param objName  Name of the object
 * @param desc  Description of property change
 * @param isSpacecraft Set to true if object is a Spcecraft
 */
//------------------------------------------------------------------------------
void Subscriber::SetOrbitColorChanged(GmatBase *originator,
                                      const std::string &newColor,
                                      const std::string &objName,
                                      const std::string &desc)
{
   // Do nothing here
   // Derived classes should handle this if needed
}


//------------------------------------------------------------------------------
// void SetTargetColorChanged(GmatBase *originator, const std::string &newColor, ...)
//------------------------------------------------------------------------------
/**
 * Sets object target color change.
 * 
 * @param originator  The assignment command pointer who is setting
 * @param newColor  New color to be applied to the object
 * @param objName  Name of the object
 * @param desc  Description of property change
 * @param isSpacecraft Set to true if object is a Spcecraft
 */
//------------------------------------------------------------------------------
void Subscriber::SetTargetColorChanged(GmatBase *originator,
                                       const std::string &newColor,
                                       const std::string &objName,
                                       const std::string &desc)
{
   // Do nothing here
   // Derived classes should handle this if needed
}


//------------------------------------------------------------------------------
// virtual void SetSegmentOrbitColor(GmatBase *originator, bool overrideColor,
//                                   UnsignedInt orbitColor)
//------------------------------------------------------------------------------
/**
 * Sets propagation segment orbit color so that subscribers can handle appropriately.
 * 
 * @param originator  The Propagate command pointer who is setting
 * @param overrideColor  The flag indicating whether or not to override orbit color
 * @param orbitColor  New orbit color to be applied to the space object
 */
//------------------------------------------------------------------------------
void Subscriber::SetSegmentOrbitColor(GmatBase *originator, bool overrideColor,
                                      UnsignedInt orbitColor, const StringArray &objNames)
{
   // Do nothing here
   // Derived classes should handle this if needed
}

//------------------------------------------------------------------------------
// Subscriber* Next()
//------------------------------------------------------------------------------
Subscriber* Subscriber::Next()
{
    return next;
}


//------------------------------------------------------------------------------
// bool Add(Subscriber *s)
//------------------------------------------------------------------------------
bool Subscriber::Add(Subscriber *s)
{
   if (next)
      return next->Add(s);

   next = s;
   return true;
}


//------------------------------------------------------------------------------
// bool Remove(Subscriber *s, const bool del)
//------------------------------------------------------------------------------
bool Subscriber::Remove(Subscriber *s, const bool del)
{
   Subscriber *temp = next;

   if (next == s)
   {
      next = next->Next();
      if (del)
         delete temp;
      return true;
   }
   return false;
}


//------------------------------------------------------------------------------
// void Activate(bool state)
//------------------------------------------------------------------------------
bool Subscriber::Activate(bool state)
{
   bool retval = true;

   #ifdef DEBUG_SUBSCRIBER
   MessageInterface::ShowMessage
      ("Subscriber::Activate() entered, state=%d, active=%d, isDataOn=%d, "
       "isDataStateChanged=%d, isInitialized=%d\n", state, active, isDataOn,
       isDataStateChanged, isInitialized);
   #endif
   
   isDataStateChanged = false;
   if (active != state)
   {
      isDataStateChanged = true;
      active = state;
      isDataOn = state;
   }
   
   #ifdef DEBUG_SUBSCRIBER
   MessageInterface::ShowMessage
      ("Subscriber::Activate() leaving, state=%d, active=%d, isDataOn=%d, "
       "isDataStateChanged=%d, isInitialized=%d\n", state, active, isDataOn,
       isDataStateChanged, isInitialized);
   #endif
   
   return retval;
}


//------------------------------------------------------------------------------
// bool IsActive()
//------------------------------------------------------------------------------
bool Subscriber::IsActive()
{
   return active;
}


//------------------------------------------------------------------------------
// void SetProviderId(Integer id)
//------------------------------------------------------------------------------
void Subscriber::SetProviderId(Integer id)
{
   #ifdef DEBUG_SUBSCRIBER_PROVIDER
   MessageInterface::ShowMessage
      ("Subscriber::SetProviderId() <%s> entered, id=%d\n", GetName().c_str(), id);
   #endif
   
   currProviderId = id;
}


//------------------------------------------------------------------------------
// void SetProviderId(Integer id)
//------------------------------------------------------------------------------
Integer Subscriber::GetProviderId()
{
   return currProviderId;
}

//------------------------------------------------------------------------------
// virtual void SetProvider(GmatBase *provider, Real epochInMjd = -999.999)
//------------------------------------------------------------------------------
void Subscriber::SetProvider(GmatBase *provider, Real epochInMjd)
{
   currentProvider = provider;
}

//------------------------------------------------------------------------------
// void SetDataLabels(const StringArray& elements)
//------------------------------------------------------------------------------
void Subscriber::SetDataLabels(const StringArray& elements)
{
   #ifdef DEBUG_SUBSCRIBER_LABELS
   MessageInterface::ShowMessage
      ("==> Subscriber::SetDataLabels() <%p>'%s' entered\n", this, GetName().c_str());
   #endif
   
   // Publisher new code always sets current labels
   if (theDataLabels.empty())
      theDataLabels.push_back(elements);
   else
      theDataLabels[0] = elements;
   
   #ifdef DEBUG_SUBSCRIBER_LABELS
   MessageInterface::ShowMessage
      ("Subscriber::SetDataLabels() <%p>'%s' leaving, theDataLabels.size()=%d, "
       "first label is '%s'\n", this, GetName().c_str(), theDataLabels.size(),
       elements[0].c_str());
   #endif
}

//------------------------------------------------------------------------------
// void SetPropagationDirection(Real propDir)
//------------------------------------------------------------------------------
void Subscriber::SetPropagationDirection(Real propDir)
{
   propDirection = propDir;
}

//------------------------------------------------------------------------------
// void ClearDataLabels()
//------------------------------------------------------------------------------
void Subscriber::ClearDataLabels()
{
   #ifdef DEBUG_SUBSCRIBER_LABELS
   MessageInterface::ShowMessage
      ("Subscriber::ClearDataLabels() <%p>'%s'entered\n", this, GetName().c_str());
   #endif
   
   theDataLabels.clear();
}


//------------------------------------------------------------------------------
// virtual void SetInternalCoordSystem(CoordinateSystem *cs)
//------------------------------------------------------------------------------
void Subscriber::SetInternalCoordSystem(CoordinateSystem *cs)
{
   theInternalCoordSystem = cs;

   // Set data coordinate system to internal coordinate system
   // Internal coordinate system is MJ2000Eq of j2000body as origin.
   // Current j2000body of current internal coordinate system is Earth.
   if (theDataCoordSystem == NULL)
      theDataCoordSystem = cs;
}


//------------------------------------------------------------------------------
// virtual void SetDataCoordSystem(CoordinateSystem *cs)
//------------------------------------------------------------------------------
void Subscriber::SetDataCoordSystem(CoordinateSystem *cs)
{
   #ifdef DEBUG_SUBSCRIBER
   MessageInterface::ShowMessage
      ("Subscriber::SetDataCoordSystem()<%s> set to %s<%p>\n",
       instanceName.c_str(), cs->GetName().c_str(), cs);
   #endif
   
   theDataCoordSystem = cs;
}


//------------------------------------------------------------------------------
// virtual void SetDataMJ2000EqOrigin(CelestialBody *cb)
//------------------------------------------------------------------------------
void Subscriber::SetDataMJ2000EqOrigin(CelestialBody *cb)
{
   theDataMJ2000EqOrigin = cb;
}


//------------------------------------------------------------------------------
// virtual void SetSolarSystem(SolarSystem *ss)
//------------------------------------------------------------------------------
void Subscriber::SetSolarSystem(SolarSystem *ss)
{
   theSolarSystem = ss;
}


//------------------------------------------------------------------------------
// const StringArray& GetWrapperObjectNameArray(bool completeSet = false)
//------------------------------------------------------------------------------
const StringArray& Subscriber::GetWrapperObjectNameArray(bool completeSet)
{
   return allWrapperObjectNames;
}


std::unique_ptr<FunctionInvocationState> Subscriber::WrapFunctionBindings(
      std::shared_ptr<SubscriberBindingState> native,
      std::unique_ptr<FunctionInvocationState> enclosing)
{
   return std::make_unique<FunctionInvocationState>(*this, [this, native]()
   {
      if (!native->Restore())
      {
         isInitialized = false;
         GmatBase::isInitialized = false;
         throw SubscriberException("Cannot restore retired or reconfigured native subscriber bindings.\n");
      }
   }, std::move(enclosing));
}

std::unique_ptr<FunctionInvocationState> Subscriber::SuspendFunctionBindings()
{
   struct BindingFrame
   {
      // A committed value can replace the resource's public context while
      // these enclosing wrappers still await ordered scope retirement.
      std::shared_ptr<const FunctionValueContext> context;
      WrapperArray x, y, queued;
      StringArray xNames, yNames;
      std::map<ElementWrapper*, ElementWrapper::InvocationLease> identities;
      std::vector<ElementWrapper::InvocationLease> retained;
      std::vector<std::unique_ptr<ElementWrapper>> clones;
      bool armed = false, owns = true, copied = false;
      bool ready = false, baseReady = false;
      CoordinateSystem *internal = nullptr;
      SolarSystem *solar = nullptr;
      std::weak_ptr<const void> internalIdentity, solarIdentity;
      ~BindingFrame()
      {
         if (!armed) return;
         auto retire = [&](WrapperArray &array, bool owned)
         {
            for (auto *wrapper : array)
            {
               if (!wrapper) continue;
               for (auto *aliases : {&x, &y, &queued})
                  for (auto &alias : *aliases) if (alias == wrapper) alias = nullptr;
               if (!owned) continue;
               const auto found = identities.find(wrapper);
               if (found == identities.end() ||
                     ElementWrapper::GetInvocationWrapper(found->second) == wrapper)
                  ElementWrapper::ReleaseOwnedWrapper(wrapper);
            }
         };
         retire(x, owns);
         retire(y, owns);
         retire(queued, true);
      }
   };
   auto saved = std::make_shared<BindingFrame>();
   saved->context = GetFunctionValueContext();
   auto native = std::make_shared<SubscriberBindingState>();
   native->Capture(theDataCoordSystem);
   native->Capture(theDataMJ2000EqOrigin);
   native->Capture(currentProvider);
   native->CaptureMetadata(theDataLabels);
   native->CaptureMetadata(currProviderId);
   native->CaptureMetadata(propDirection);
   saved->xNames = xWrapperObjectNames;
   saved->yNames = yWrapperObjectNames;
   saved->copied = wrappersCopied;
#ifndef __ENABLE_CLONING_WRAPPERS__
   saved->owns = !wrappersCopied;
#endif
   saved->ready = isInitialized;
   saved->baseReady = GmatBase::isInitialized;
   saved->internal = theInternalCoordSystem;
   saved->solar = theSolarSystem;
   saved->retained = invocationWrapperLeases;
   if (saved->internal) saved->internalIdentity = saved->internal->GetLifetimeToken();
   if (saved->solar) saved->solarIdentity = saved->solar->GetLifetimeToken();
   for (const auto *array : {&xParamWrappers, &yParamWrappers, &oldWrappersToDelete})
      for (auto *wrapper : *array)
         if (wrapper && !saved->identities.count(wrapper))
         {
            ElementWrapper::InvocationLease lease;
            for (const auto &current : invocationWrapperLeases)
               if (ElementWrapper::GetInvocationWrapper(current) == wrapper) { lease = current; break; }
            if (!lease)
               for (const auto &current : invocationWrapperLeases)
                  if (ElementWrapper::MatchesInvocationWrapper(current, wrapper)) { lease = current; break; }
            if (!lease) lease = ElementWrapper::RetainInvocationWrapper(wrapper);
            if (ElementWrapper::GetInvocationWrapper(lease) != wrapper)
               throw SubscriberException("Cannot suspend a retired subscriber wrapper.\n");
            saved->identities.emplace(wrapper, lease);
            if (std::find(saved->retained.begin(), saved->retained.end(), lease) == saved->retained.end())
               saved->retained.push_back(std::move(lease));
         }
   std::map<ElementWrapper*, ElementWrapper*> copies;
   auto stage = [&](const WrapperArray &from, WrapperArray &to)
   {
      to.reserve(from.size());
      for (auto *wrapper : from)
      {
         if (!wrapper) { to.push_back(nullptr); continue; }
         const auto found = copies.find(wrapper);
         if (found != copies.end()) { to.push_back(found->second); continue; }
         ElementWrapper *raw = wrapper->Clone();
         if (!raw || saved->identities.count(raw) ||
               std::any_of(saved->clones.begin(), saved->clones.end(),
                     [raw](const auto &owned) { return owned.get() == raw; }))
            throw SubscriberException("Subscriber wrapper Clone returned a source, peer or null.\n");
         std::unique_ptr<ElementWrapper> copy(raw);
         if (typeid(*raw) != typeid(*wrapper))
            throw SubscriberException("Subscriber wrapper Clone changed its native type.\n");
         // This frame copies bindings for a live shared output, not its values.
         // VariableWrapper::Clone independently owns its numeric reference;
         // retain the original borrowed scalar identity during the inner call.
         // Deliberately owned wrapper values keep their independent copy.
         if (typeid(*wrapper) == typeid(VariableWrapper) && !wrapper->OwnsRefObject())
            raw->RebindClonedReferences(*wrapper, [](GmatBase *value) { return value; });
         copies.emplace(wrapper, raw);
         to.push_back(raw);
         saved->clones.push_back(std::move(copy));
      }
   };
   stage(xParamWrappers, saved->x);
   stage(yParamWrappers, saved->y);
   auto restore = [this, saved, native]()
   {
      if (!saved->armed) return;
      bool valid = native->Restore();
      for (auto *array : {&saved->x, &saved->y, &saved->queued})
         for (auto &wrapper : *array)
            if (wrapper)
            {
               const auto found = saved->identities.find(wrapper);
               if (found == saved->identities.end() ||
                     ElementWrapper::GetInvocationWrapper(found->second) != wrapper)
               { wrapper = nullptr; valid = false; }
            }
      const bool sameNames = xWrapperObjectNames == saved->xNames &&
            yWrapperObjectNames == saved->yNames;
      xParamWrappers.swap(saved->x);
      yParamWrappers.swap(saved->y);
      oldWrappersToDelete.swap(saved->queued);
      invocationWrapperLeases.swap(saved->retained);
#ifdef __ENABLE_CLONING_WRAPPERS__
      saved->owns = true;
#else
      saved->owns = !wrappersCopied;
#endif
      wrappersCopied = saved->copied;
      theInternalCoordSystem = !saved->internal ||
            GmatBase::IsLiveReference(saved->internal, saved->internalIdentity) ? saved->internal : nullptr;
      theSolarSystem = !saved->solar ||
            GmatBase::IsLiveReference(saved->solar, saved->solarIdentity) ? saved->solar : nullptr;
      if ((saved->internal && !theInternalCoordSystem) || (saved->solar && !theSolarSystem)) valid = false;
      isInitialized = isInitialized && saved->ready;
      GmatBase::isInitialized = GmatBase::isInitialized && saved->baseReady;
      if (!sameNames)
      {
         // Configuration changes persist. Old bindings cannot describe a new
         // list; leave it unbound for the ordinary rebuild lifecycle.
         ClearWrappers(true, true);
         valid = false;
      }
      PruneInvocationWrapperLeases();
      if (!valid)
      {
         isInitialized = false;
         GmatBase::isInitialized = false;
         throw SubscriberException("Cannot restore retired or reconfigured subscriber bindings.\n");
      }
   };
   auto state = std::make_unique<FunctionInvocationState>(*this, std::move(restore));
   xParamWrappers.swap(saved->x);
   yParamWrappers.swap(saved->y);
   oldWrappersToDelete.swap(saved->queued);
   invocationWrapperLeases.clear();
   wrappersCopied = false;
   saved->armed = true;
   for (auto &copy : saved->clones) copy.release();
   return state;
}

void Subscriber::PruneInvocationWrapperLeases() noexcept
{
   std::vector<ElementWrapper::InvocationLease> retained, retired;
   try
   {
      retained.reserve(invocationWrapperLeases.size());
      retired.reserve(invocationWrapperLeases.size());
      for (const auto &lease : invocationWrapperLeases)
      {
         auto *wrapper = ElementWrapper::GetInvocationWrapper(lease);
         bool used = false;
         if (wrapper)
            for (const auto *array : {&xParamWrappers, &yParamWrappers, &oldWrappersToDelete})
               if (std::find(array->begin(), array->end(), wrapper) != array->end()) used = true;
         (used ? retained : retired).push_back(lease);
      }
   }
   catch (...) { return; } // Staging failure leaves the original owners intact.
   invocationWrapperLeases.swap(retained);
   retained.clear(); // Every old lease still has an owner in one staged list.
   // A wrapper destructor can retire a function and reenter this subscriber.
   // Its live lease vector is fully published before releasing orphan owners.
   retired.clear();
}

//------------------------------------------------------------------------------
// bool SetElementWrapper(ElementWrapper* toWrapper, const std::string &name)
//------------------------------------------------------------------------------
bool Subscriber::SetElementWrapper(ElementWrapper* toWrapper,
                                   const std::string &name)
{
   if (!toWrapper) return false;
   auto slot = [&name](const StringArray &names, const WrapperArray &wrappers) -> size_t
   {
      const auto found = std::find(names.begin(), names.end(), name);
      if (found == names.end()) return wrappers.size();
      const size_t index = static_cast<size_t>(found - names.begin());
      if (index >= wrappers.size())
         throw SubscriberException("Subscriber wrapper slots do not match their names.\n");
      return index;
   };
   const size_t x = slot(xWrapperObjectNames, xParamWrappers);
   const size_t y = slot(yWrapperObjectNames, yParamWrappers);
   if (x == xParamWrappers.size() && y == yParamWrappers.size()) return false;
   WrapperArray retired = oldWrappersToDelete;
   retired.reserve(retired.size() + 2);
   auto remember = [&](ElementWrapper *wrapper)
   {
      if (wrapper && wrapper != toWrapper && std::find(retired.begin(), retired.end(), wrapper) == retired.end())
         retired.push_back(wrapper);
   };
   if (x < xParamWrappers.size()) remember(xParamWrappers[x]);
   if (y < yParamWrappers.size()) remember(yParamWrappers[y]);
   // All allocation precedes publication. Destructors see complete new slots
   // and cannot observe or mutate the local queue that owns retired wrappers.
   if (x < xParamWrappers.size()) xParamWrappers[x] = toWrapper;
   if (y < yParamWrappers.size()) yParamWrappers[y] = toWrapper;
   oldWrappersToDelete.clear();
   for (size_t i = 0; i < retired.size(); ++i)
   {
      auto *wrapper = retired[i];
      for (auto &alias : retired) if (alias == wrapper) alias = nullptr;
      if (wrapper && std::find(xParamWrappers.begin(), xParamWrappers.end(), wrapper) == xParamWrappers.end() &&
            std::find(yParamWrappers.begin(), yParamWrappers.end(), wrapper) == yParamWrappers.end())
         ElementWrapper::ReleaseOwnedWrapper(wrapper);
   }
   PruneInvocationWrapperLeases();
   return true;
}


//------------------------------------------------------------------------------
// void ClearWrappers(bool clearX, bool clearY)
//------------------------------------------------------------------------------
/*
 * Deletes and sets all wrapper pointers to NULL but leaves size unchanged.
 */
//------------------------------------------------------------------------------
void Subscriber::ClearWrappers(bool clearX, bool clearY)
{
   WrapperArray x, y, queued;
   if (clearX) x.swap(xParamWrappers);
   if (clearY) y.swap(yParamWrappers);
   if (clearX && clearY) queued.swap(oldWrappersToDelete);
   for (auto *array : {&x, &y, &queued})
      for (auto *wrapper : *array)
      {
         if (!wrapper) continue;
         for (auto *aliases : {&x, &y, &queued})
            for (auto &alias : *aliases) if (alias == wrapper) alias = nullptr;
         if (std::find(xParamWrappers.begin(), xParamWrappers.end(), wrapper) == xParamWrappers.end() &&
               std::find(yParamWrappers.begin(), yParamWrappers.end(), wrapper) == yParamWrappers.end())
         {
            for (auto &alias : oldWrappersToDelete) if (alias == wrapper) alias = nullptr;
            ElementWrapper::ReleaseOwnedWrapper(wrapper);
         }
      }
   PruneInvocationWrapperLeases();
}


//---------------------------------------------------------------------------
// virtual bool RenameRefObject(const UnsignedInt type,
//                              const std::string &oldName,
//                              const std::string &newName)
//---------------------------------------------------------------------------
bool Subscriber::RenameRefObject(const UnsignedInt type,
                                 const std::string &oldName,
                                 const std::string &newName)
{
   #ifdef DEBUG_RENAME
   MessageInterface::ShowMessage
      ("\nSubscriber::RenameRefObject() type=%d, oldName=<%s>, newName=<%s>\n",
       type, oldName.c_str(), newName.c_str());
   #endif
   
   // Go through X wrapper object names
   Integer sz = xWrapperObjectNames.size();
   #ifdef DEBUG_RENAME
   MessageInterface::ShowMessage("   =====> Going through X wrapper object names, size=%d\n", sz);
   #endif
   for (Integer i = 0; i < sz; i++)
   {
      #ifdef DEBUG_RENAME
      MessageInterface::ShowMessage
         ("   Checking X wrapper name: '%s'\n", xWrapperObjectNames[i].c_str());
      #endif
      
      if (xWrapperObjectNames[i].find(oldName) != oldName.npos)
      {
         xWrapperObjectNames[i] =
            GmatStringUtil::ReplaceName(xWrapperObjectNames[i], oldName, newName);
         
         #ifdef DEBUG_RENAME
         MessageInterface::ShowMessage
            ("   After rename, wrapper name: '%s'\n", xWrapperObjectNames[i].c_str());
         #endif
      }
   }
   
   // Go through Y wrapper object names
   sz = yWrapperObjectNames.size();
   #ifdef DEBUG_RENAME
   MessageInterface::ShowMessage("   =====> Going through Y wrapper object names, size=%d\n", sz);
   #endif
   for (Integer i = 0; i < sz; i++)
   {
      #ifdef DEBUG_RENAME
      MessageInterface::ShowMessage
         ("   Checking Y wrapper name: '%s'\n", yWrapperObjectNames[i].c_str());
      #endif
      
      if (yWrapperObjectNames[i].find(oldName) != oldName.npos)
      {
         yWrapperObjectNames[i] =
            GmatStringUtil::ReplaceName(yWrapperObjectNames[i], oldName, newName);
         
         #ifdef DEBUG_RENAME
         MessageInterface::ShowMessage
            ("   After rename, wrapper name: '%s'\n", yWrapperObjectNames[i].c_str());
         #endif
      }
   }
   
   #ifdef DEBUG_RENAME
   MessageInterface::ShowMessage("   =====> New Y wrapper names\n");
   for (Integer i = 0; i < sz; i++)
      MessageInterface::ShowMessage
         ("   <%s>\n", yWrapperObjectNames[i].c_str());
   #endif
   
   // Go through X wrappers
   sz = xParamWrappers.size();
   #ifdef DEBUG_RENAME
   MessageInterface::ShowMessage("   =====> Going through X wrappers, size=%d\n", sz);
   #endif
   for (Integer i = 0; i < sz; i++)
   {
      if (xParamWrappers[i] == NULL)
         throw SubscriberException
            ("Subscriber::RenameRefObject() \"" + GetName() +
             "\" failed to set reference for object named \"" + oldName +
             ".\" The wrapper is NULL.\n");
      
      std::string desc = xParamWrappers[i]->GetDescription();
      #ifdef DEBUG_RENAME
      MessageInterface::ShowMessage
         ("   Checking X wrapper desc: '%s'\n", desc.c_str());
      #endif
      if (desc.find(oldName) != oldName.npos)
      {
         xParamWrappers[i]->RenameObject(oldName, newName);
         desc = xParamWrappers[i]->GetDescription();
         
         #ifdef DEBUG_RENAME
         MessageInterface::ShowMessage
            ("   After rename, wrapper desc: '%s'\n", desc.c_str());
         #endif
      }
   }
   
   // Go through Y wrappers
   sz = yParamWrappers.size();
   #ifdef DEBUG_RENAME
   MessageInterface::ShowMessage("   =====> Going through Y wrappers, size=%d\n", sz);
   #endif
   for (Integer i = 0; i < sz; i++)
   {
      if (yParamWrappers[i] == NULL)
         throw SubscriberException
            ("Subscriber::RenameRefObject() \"" + GetName() +
             "\" failed to set reference for object named \"" + oldName +
             ".\" The wrapper is NULL.\n");
      
      std::string desc = yParamWrappers[i]->GetDescription();
      #ifdef DEBUG_RENAME
      MessageInterface::ShowMessage
         ("   Checking Y wrapper desc = '%s'\n", desc.c_str());
      #endif
      if (desc.find(oldName) != oldName.npos)
      {
         yParamWrappers[i]->RenameObject(oldName, newName);
         desc = yParamWrappers[i]->GetDescription();
         
         #ifdef DEBUG_RENAME
         MessageInterface::ShowMessage
            ("   After rename, wrapper desc = '%s'\n", desc.c_str());
         #endif
      }
   }
   
   #ifdef DEBUG_RENAME
   MessageInterface::ShowMessage("   =====> New Y wrapper descriptions\n");
   for (Integer i = 0; i < sz; i++)
      MessageInterface::ShowMessage
         ("   <%s>\n", yParamWrappers[i]->GetDescription().c_str());
   MessageInterface::ShowMessage
      ("Subscriber::RenameRefObject() returning true\n");
   #endif
   
   return true;
}


//---------------------------------------------------------------------------
//  bool IsParameterReadOnly(const Integer id) const
//---------------------------------------------------------------------------
/**
 * Checks to see if the requested parameter is read only.
 *
 * @param <id> Description for the parameter.
 *
 * @return true if the parameter is read only, false (the default) if not,
 *         throws if the parameter is out of the valid range of values.
 */
//---------------------------------------------------------------------------
bool Subscriber::IsParameterReadOnly(const Integer id) const
{
   if (id == TARGET_STATUS)
      return true;
   if (id == MINIMIZED) return true;  // ******* temporary, until saving of minimized/maximized can be implemented
   
   return GmatBase::IsParameterReadOnly(id);
}


//---------------------------------------------------------------------------
// bool IsParameterVisible(const Integer id) const
//---------------------------------------------------------------------------
bool Subscriber::IsParameterVisible(const Integer id) const
{
   if (id == TARGET_STATUS || id == UPPER_LEFT || id == SIZE ||
       id == RELATIVE_Z_ORDER|| id == MINIMIZED || id == MAXIMIZED)
      return false;
   
   return GmatBase::IsParameterVisible(id);
}


//---------------------------------------------------------------------------
// bool IsParameterVisible(const std::string &label) const
//---------------------------------------------------------------------------
bool Subscriber::IsParameterVisible(const std::string &label) const
{
   return IsParameterVisible(GetParameterID(label));
}

//------------------------------------------------------------------------------
// bool IsParameterCommandModeSettable(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Tests to see if an object property can be set in Command mode
 *
 * @param id The ID of the object property
 *
 * @return true if the property can be set in command mode, false if not.
 */
//------------------------------------------------------------------------------
bool Subscriber::IsParameterCommandModeSettable(const Integer id) const
{
   if ((id == UPPER_LEFT)        || (id == SIZE)       ||
       (id ==  RELATIVE_Z_ORDER) || (id ==  MINIMIZED) ||
       (id == MAXIMIZED) || (id == SOLVER_ITERATIONS))
      return false;

   return GmatBase::IsParameterCommandModeSettable(id);
}


//------------------------------------------------------------------------------
// std::string GetParameterText(const Integer id) const
//------------------------------------------------------------------------------
std::string Subscriber::GetParameterText(const Integer id) const
{
   if (id >= GmatBaseParamCount && id < SubscriberParamCount)
      return PARAMETER_TEXT[id - GmatBaseParamCount];
   else
      return GmatBase::GetParameterText(id);
    
}


//------------------------------------------------------------------------------
// Integer GetParameterID(const std::string &str) const
//------------------------------------------------------------------------------
Integer Subscriber::GetParameterID(const std::string &str) const
{
   for (int i=GmatBaseParamCount; i<SubscriberParamCount; i++)
   {
      if (str == PARAMETER_TEXT[i - GmatBaseParamCount])
         return i;
   }
   
   return GmatBase::GetParameterID(str);
}


//------------------------------------------------------------------------------
// Gmat::ParameterType GetParameterType(const Integer id) const
//------------------------------------------------------------------------------
Gmat::ParameterType Subscriber::GetParameterType(const Integer id) const
{
   if (id >= GmatBaseParamCount && id < SubscriberParamCount)
      return PARAMETER_TYPE[id - GmatBaseParamCount];
   else
      return GmatBase::GetParameterType(id);
}


//------------------------------------------------------------------------------
// std::string GetParameterTypeString(const Integer id) const
//------------------------------------------------------------------------------
std::string Subscriber::GetParameterTypeString(const Integer id) const
{
   return GmatBase::PARAM_TYPE_STRING[GetParameterType(id)];
}

//---------------------------------------------------------------------------
//  Integer GetIntegerParameter(const Integer id) const
//---------------------------------------------------------------------------
/**
* Retrieve the value for an Integer parameter.
 *
 * @param <id> The integer ID for the parameter.
 *
 * @return The parameter's value.
 */
//------------------------------------------------------------------------------
Integer Subscriber::GetIntegerParameter(const Integer id) const
{
   if (id == RELATIVE_Z_ORDER)  return relativeZOrder;

   return GmatBase::GetIntegerParameter(id);
}


//---------------------------------------------------------------------------
//  Integer SetIntegerParameter(const Integer id, const Integer value)
//---------------------------------------------------------------------------
/**
* Set the value for an Integer parameter.
 *
 * @param <id> The integer ID for the parameter.
 * @param <value> The new parameter value.
 *
 * @return the parameter value at the end of this call
 */
//------------------------------------------------------------------------------
Integer Subscriber::SetIntegerParameter(const Integer id, const Integer value)
{
   #ifdef DEBUG_SUBSCRIBER_SET
      MessageInterface::ShowMessage("Now setting value of %d(%s) to %d for object %s\n",
            id, GetParameterText(id).c_str(), value, instanceName.c_str());
   #endif
   if (id == RELATIVE_Z_ORDER)
   {
      relativeZOrder = value;
      return relativeZOrder;
   }
   return GmatBase::SetIntegerParameter(id, value);
}



//------------------------------------------------------------------------------
// std::string GetStringParameter(const Integer id) const
//------------------------------------------------------------------------------
std::string Subscriber::GetStringParameter(const Integer id) const
{
   switch (id)
   {
   case SOLVER_ITERATIONS:
      return mSolverIterations;
   default:
      return GmatBase::GetStringParameter(id);
   }
}


//------------------------------------------------------------------------------
// std::string GetStringParameter(const std::string &label) const
//------------------------------------------------------------------------------
std::string Subscriber::GetStringParameter(const std::string &label) const
{
   #ifdef DEBUG_SUBSCRIBER_PARAM
   MessageInterface::ShowMessage
      ("Subscriber::GetStringParameter() label = %s\n", label.c_str());
   #endif
   
   return GetStringParameter(GetParameterID(label));
}

//------------------------------------------------------------------------------
// bool SetStringParameter(const Integer id, const char *value)
//------------------------------------------------------------------------------
bool Subscriber::SetStringParameter(const Integer id, const char *value)
{
   return SetStringParameter(id, std::string(value));
}

//------------------------------------------------------------------------------
// bool SetStringParameter(const Integer id, const std::string &value)
//------------------------------------------------------------------------------
bool Subscriber::SetStringParameter(const Integer id, const std::string &value)
{
   #ifdef DEBUG_SUBSCRIBER_PARAM
   MessageInterface::ShowMessage
      ("Subscriber::SetStringParameter() id = %d, value = %s \n", id,
       value.c_str());
   #endif
   
   switch (id)
   {
   case SOLVER_ITERATIONS:
      {
         bool itemFound = false;
         int index = -1;
         for (int i=0; i<SolverIterOptionCount; i++)
         {
            if (value == SOLVER_ITER_OPTION_TEXT[i])
            {
               itemFound = true;
               index = i;
               break;
            }
         }
         
         if (itemFound)
         {
            mSolverIterations = value;
            mSolverIterOption = (SolverIterOption)index;
            return true;
         }
         else
         {
            SubscriberException se;
            se.SetDetails(errorMessageFormat.c_str(), value.c_str(),
                          PARAMETER_TEXT[id - GmatBaseParamCount].c_str(),
                          "All, Current, None");
            
            if (value == "On" || value == "Off")
            {
               MessageInterface::ShowMessage
                  ("*** WARNING *** %s.\n", se.GetFullMessage().c_str());
               
               if (value == "On")
               {
                  mSolverIterations = "All";
                  mSolverIterOption = SI_ALL;
                  MessageInterface::ShowMessage
                     ("The value \"On\" has automatically switched to \"All\" but "
                      "this will cause an error in a future build.\n\n");
               }
               else
               {
                  mSolverIterations = "None";
                  mSolverIterOption = SI_NONE;
                  MessageInterface::ShowMessage
                     ("The value \"Off\" has automatically switched to \"None\" but "
                      "this will cause an error in a future build.\n\n");
               }
               
               return true;
            }
            else
            {
               throw se;
            }
         }
      }
//   case UPPER_LEFT:
//   case SIZE:
//      if (value[0] == '[')
//         PutUnsignedIntValue(id, value);
//      return true;
   default:
      return GmatBase::SetStringParameter(id, value);
   }
}


//------------------------------------------------------------------------------
// bool SetStringParameter(const std::string &label, const std::string &value)
//------------------------------------------------------------------------------
bool Subscriber::SetStringParameter(const std::string &label,
                                    const std::string &value)
{
   #ifdef DEBUG_SUBSCRIBER_PARAM
   MessageInterface::ShowMessage
      ("Subscriber::SetStringParameter() label = %s, value = %s \n",
       label.c_str(), value.c_str());
   #endif
   
   return SetStringParameter(GetParameterID(label), value);
}


//------------------------------------------------------------------------------
// virtual bool SetStringParameter(const Integer id, const std::string &value,
//                                 const Integer index)
//------------------------------------------------------------------------------
bool Subscriber::SetStringParameter(const Integer id, const std::string &value,
                                    const Integer index)
{
   #ifdef DEBUG_SUBSCRIBER_PARAM
   MessageInterface::ShowMessage
      ("Subscriber::SetStringParameter() id = %d, value = %s, index = %d\n",
       id, value.c_str(), index);
   #endif
   
   return GmatBase::SetStringParameter(id, value, index);
}


//------------------------------------------------------------------------------
// virtual bool SetStringParameter(const std::string &label,
//                                 const std::string &value,
//                                 const Integer index)
//------------------------------------------------------------------------------
bool Subscriber::SetStringParameter(const std::string &label,
                                    const std::string &value,
                                    const Integer index)
{
   #ifdef DEBUG_SUBSCRIBER_PARAM
   MessageInterface::ShowMessage
      ("Subscriber::SetStringParameter() label = %s, value = %s, index = %d\n",
       label.c_str(), value.c_str(), index);
   #endif
   
   return SetStringParameter(GetParameterID(label), value, index);
}

//---------------------------------------------------------------------------
//  bool GetBooleanParameter(const Integer id) const
//---------------------------------------------------------------------------
/**
 * Retrieve a boolean parameter.
 *
 * @param  id  The integer ID for the parameter.
 *
 * @return the boolean value for this parameter
 */
//------------------------------------------------------------------------------
bool Subscriber::GetBooleanParameter(const Integer id) const
{
	switch (id)
	{
		case MINIMIZED: return isMinimized;
		case MAXIMIZED: return isMaximized;
		default:		return GmatBase::GetBooleanParameter(id);
	}
    
}


//---------------------------------------------------------------------------
//  bool SetBooleanParameter(const Integer id, const bool value)
//---------------------------------------------------------------------------
/**
 * Sets the value for a boolean parameter.
 *
 * @param id The integer ID for the parameter.
 * @param value The new value.
 *
 * @return the boolean value for this parameter
 */
//------------------------------------------------------------------------------
bool Subscriber::SetBooleanParameter(const Integer id, const bool value)
{
	switch (id)
	{
		case MINIMIZED: 
		   {
			  isMinimized = value;
			  return isMinimized;
		   }
		case MAXIMIZED: 
		   {
			  isMaximized = value;
			  return isMaximized;
		   }
		default:		
			return GmatBase::SetBooleanParameter(id, value);
	}
}

bool Subscriber::GetBooleanParameter(const std::string &label) const
{
   return GetBooleanParameter(GetParameterID(label));
}

bool Subscriber::SetBooleanParameter(const std::string &label, const bool value)
{
   return SetBooleanParameter(GetParameterID(label), value);
}

//---------------------------------------------------------------------------
//  std::string GetOnOffParameter(const Integer id) const
//---------------------------------------------------------------------------
std::string Subscriber::GetOnOffParameter(const Integer id) const
{
   switch (id)
   {
   case TARGET_STATUS:
      return mSolverIterations;
   default:
      return GmatBase::GetOnOffParameter(id);
   }
}


//------------------------------------------------------------------------------
// std::string Subscriber::GetOnOffParameter(const std::string &label) const
//------------------------------------------------------------------------------
std::string Subscriber::GetOnOffParameter(const std::string &label) const
{
   return GetOnOffParameter(GetParameterID(label));
}


//---------------------------------------------------------------------------
//  bool SetOnOffParameter(const Integer id, const std::string &value)
//---------------------------------------------------------------------------
bool Subscriber::SetOnOffParameter(const Integer id, const std::string &value)
{
   #ifdef DEBUG_SUBSCRIBER_PARAM
   MessageInterface::ShowMessage
      ("Subscriber::SetOnOffParameter() id = %d, value = %s \n", id,
       value.c_str());
   #endif
   
   // Write only one message per session
   static bool writeTargetStatus = true;
   
   switch (id)
   {
   case TARGET_STATUS:
      if (writeTargetStatus)
      {
         MessageInterface::ShowMessage
            ("*** WARNING *** \"TargetStatus\" is deprecated and will be "
             "removed from a future build; please use \"SolverIterations\" "
             "instead.\n");
         writeTargetStatus = false;
      }
      
      if (value == "On")
      {
         mSolverIterations = "All";
         mSolverIterOption = SI_ALL;
      }
      else
      {
         mSolverIterations = "None";
         mSolverIterOption = SI_NONE;
      }
      
      return true;
      
   default:
      return GmatBase::SetOnOffParameter(id, value);
   }
}


//------------------------------------------------------------------------------
// bool SetOnOffParameter(const std::string &label, const std::string &value)
//------------------------------------------------------------------------------
bool Subscriber::SetOnOffParameter(const std::string &label, 
                                   const std::string &value)
{
   return SetOnOffParameter(GetParameterID(label), value);
}

Real Subscriber::GetRealParameter(const Integer id) const
{
   return GmatBase::GetRealParameter(id);
}

Real Subscriber::SetRealParameter(const Integer id, const Real value)
{
   return GmatBase::SetRealParameter(id, value);
}

Real Subscriber::GetRealParameter(const Integer id, const Integer index) const
{
   if ((index < 0) || (index > 1))
   {
      std::string errmsg = "Index is out-of-bounds for upper left or size for plot ";
      errmsg += instanceName + ".\n";
      throw SubscriberException(errmsg);
   }
   switch (id)
   {
   case UPPER_LEFT:
      return mPlotUpperLeft[index];

   case SIZE:
      return mPlotSize[index];

   default:
      return GmatBase::GetRealParameter(id, index);
   }
}
Real Subscriber::SetRealParameter(const Integer id, const Real value, const Integer index)
{
   if (index < 0 || (index > 1))
      throw SubscriberException
         ("index out of bounds for " + GetParameterText(id));

   switch (id)
   {
   case UPPER_LEFT:
   {
      mPlotUpperLeft[index] = value;
      return mPlotUpperLeft[index];
   }
   case SIZE:
   {
      mPlotSize[index] = value;
      return mPlotSize[index];
   }
   default:
      return GmatBase::GetRealParameter(id, index);
   }
}

const Rvector& Subscriber::GetRvectorParameter(const Integer id) const
{
   switch (id)
   {
   case UPPER_LEFT:
      return mPlotUpperLeft;
   case SIZE:
      return mPlotSize;
   default:
      return GmatBase::GetRvectorParameter(id);
   }
}

//------------------------------------------------------------------------------
// const Rvector& SetRvectorParameter(const Integer id, const Rvector &value)
//------------------------------------------------------------------------------
const Rvector& Subscriber::SetRvectorParameter(const Integer id, const Rvector &value)
{
   #ifdef DEBUG_SUBSCRIBER_SET
   MessageInterface::ShowMessage
      ("Subscriber::SetRvectorParameter() <%p>'%s' entered, id=%d, value=%s",
       this, GetName().c_str(), id, value.ToString().c_str());
   #endif
   
   if (value.GetSize() != 2)
      throw SubscriberException
         ("incorrect size vector for " + GetParameterText(id));
   
   switch (id)
   {
   case UPPER_LEFT:
   {
      mPlotUpperLeft = value;
      return mPlotUpperLeft;
   }
   case SIZE:
   {
      mPlotSize = value;
      return mPlotSize;
   }
   default:
      return GmatBase::SetRvectorParameter(id, value);
   }
}

const Rvector& Subscriber::GetRvectorParameter(const std::string &label) const
{
   return GetRvectorParameter(GetParameterID(label));
}

const Rvector& Subscriber::SetRvectorParameter(const std::string &label, const Rvector &value)
{
   return SetRvectorParameter(GetParameterID(label), value);
}

//------------------------------------------------------------------------------
// const std::string* GetSolverIterOptionList()
//------------------------------------------------------------------------------
const std::string* Subscriber::GetSolverIterOptionList()
{
   return SOLVER_ITER_OPTION_TEXT;
}


//---------------------------------
//  protected methods
//---------------------------------

//------------------------------------------------------------------------------
// bool SetActualElementWrapper(const StringArray &wrapperNames, ...)
//------------------------------------------------------------------------------
bool Subscriber::SetActualElementWrapper(const StringArray &wrapperNames,
                                         WrapperArray &paramWrappers,
                                         ElementWrapper* toWrapper,
                                         const std::string &name)
{
   #ifdef DEBUG_WRAPPER_CODE   
   MessageInterface::ShowMessage
      ("Subscriber::SetActualElementWrapper() entered, wrapperNames.size()=%d, "
       "paramWrappers.size()=%d, toWrapper=<%p>, name='%s'\n", wrapperNames.size(),
       paramWrappers.size(), toWrapper, name.c_str());
   #endif
   
   ElementWrapper *ew;
   Integer sz = wrapperNames.size();
   for (Integer i = 0; i < sz; i++)
   {
      if (wrapperNames.at(i) == name)
      {
         #ifdef DEBUG_WRAPPER_CODE   
         MessageInterface::ShowMessage
            ("   Found wrapper name \"%s\", wrapper=%p\n", name.c_str(), paramWrappers.at(i));
         #endif
         
         if (paramWrappers.at(i) != NULL)
         {
            ew = paramWrappers.at(i);
            if (find(oldWrappersToDelete.begin(), oldWrappersToDelete.end(), ew)
                == oldWrappersToDelete.end())
            {
               #ifdef DEBUG_WRAPPER_CODE
               MessageInterface::ShowMessage("   Adding ew<%p> to wrappers to delete\n", ew);
               #endif
               oldWrappersToDelete.push_back(ew);
            }
            paramWrappers.at(i) = toWrapper;
         }
         else
         {
            paramWrappers.at(i) = toWrapper;
         }
         
         #ifdef DEBUG_WRAPPER_CODE
         MessageInterface::ShowMessage
            ("   Set wrapper name \"%s\" to wrapper=%p\n", name.c_str(), paramWrappers.at(i));
         #endif
         
         return true;
      }
   }
   
   return false;
}


//------------------------------------------------------------------------------
// bool CloneWrappers(WrapperArray &toWrappers, const WrapperArray &fromWrappers);
//------------------------------------------------------------------------------
void Subscriber::CopyWrapperGraph(const Subscriber &source)
{
   WrapperArray x, y;
   std::map<ElementWrapper*, ElementWrapper*> copies;
   std::set<ElementWrapper*> forbidden;
   std::vector<std::unique_ptr<ElementWrapper>> owned;
   const WrapperArray *arrays[] = {&source.xParamWrappers, &source.yParamWrappers,
         &xParamWrappers, &yParamWrappers, &oldWrappersToDelete};
   for (const auto *array : arrays)
      for (auto *wrapper : *array) if (wrapper) forbidden.insert(wrapper);
   auto clone = [&](const WrapperArray &from, WrapperArray &to)
   {
      to.reserve(from.size());
      for (auto *wrapper : from)
      {
         if (!wrapper) { to.push_back(nullptr); continue; }
         const auto found = copies.find(wrapper);
         if (found != copies.end()) { to.push_back(found->second); continue; }
         auto value = ElementWrapper::CloneValue(wrapper);
         if (forbidden.count(value.get()))
         {
            value.release();
            throw SubscriberException("Subscriber Clone returned an existing wrapper.\n");
         }
         forbidden.insert(value.get());
         copies.emplace(wrapper, value.get());
         to.push_back(value.get());
         owned.push_back(std::move(value));
      }
   };
   clone(source.xParamWrappers, x);
   clone(source.yParamWrappers, y);
   ClearWrappers(true, true);
   xParamWrappers.swap(x);
   yParamWrappers.swap(y);
   for (auto &value : owned) value.release();
   wrappersCopied = false;
}

bool Subscriber::CloneWrappers(WrapperArray &toWrappers,
                               const WrapperArray &fromWrappers)
{
   #ifdef DEBUG_WRAPPER_CODE
   MessageInterface::ShowMessage
      ("Subscriber::CloneWrappers() <%p>'%s' entered\n", this, GetName().c_str());
   #endif
   toWrappers.clear();
   for (UnsignedInt i=0; i<fromWrappers.size(); i++)
   {
      if (fromWrappers[i] != NULL)
      {
         ElementWrapper *ew = fromWrappers[i]->Clone();
         toWrappers.push_back(ew);
         #ifdef DEBUG_MEMORY
         MemoryTracker::Instance()->Add
            (ew, ew->GetDescription(), "Subscriber::CloneWrappers()",
             "ew = fromWrappers[i]->Clone()");
         #endif         
      }
      else
      {
         // Need to keep the size, so add NULL (Fix for GMT1552 LOJ: 2015.06.24)
         toWrappers.push_back(NULL);
      }
   }
   
   return true;
}


//------------------------------------------------------------------------------
// bool SetWrapperReference(GmatBase *obj, const std::string &name)
//------------------------------------------------------------------------------
bool Subscriber::SetWrapperReference(GmatBase *obj, const std::string &name)
{
   #ifdef DEBUG_WRAPPER_CODE   
   MessageInterface::ShowMessage
      ("\nSubscriber::SetWrapperReference() <%p>'%s' entered, obj=<%p>, name='%s', "
       "obj->IsGlobal()=%d\n", this, GetName().c_str(), obj, name.c_str(),
       obj->IsGlobal());
   MessageInterface::ShowMessage
      ("   xWrapperObjectNames.size() = %d, yWrapperObjectNames.size() = %d\n",
       xWrapperObjectNames.size(), yWrapperObjectNames.size());
   MessageInterface::ShowMessage
      ("   xParamWrappers.size() = %d, yParamWrappers.size() = %d\n",
       xParamWrappers.size(), yParamWrappers.size());
   for (unsigned int i = 0; i < xWrapperObjectNames.size(); i++)
      MessageInterface::ShowMessage
         ("   xWrapperObjectNames[%d] = '%s'\n", i, xWrapperObjectNames[i].c_str());
   for (unsigned int i = 0; i < yWrapperObjectNames.size(); i++)
      MessageInterface::ShowMessage
         ("   yWrapperObjectNames[%d] = '%s'\n", i, yWrapperObjectNames[i].c_str());
   #endif
   
   // Actually SetActualWrapperReferences() returns true or throws a exception
   bool retval1 = true;
   bool retval2 = true;
   
   // Check X Parameters
   if (xParamWrappers.size() > 0)
      retval1 = SetActualWrapperReference(xParamWrappers, obj, name);
   
   // Check Y Parameters
   if (yParamWrappers.size() > 0)
      retval2 = SetActualWrapperReference(yParamWrappers, obj, name);
   
   if (!retval1 && !retval2)
      throw SubscriberException
         ("Subscriber::SetActualWrapperReference() \"" + GetName() +
          "\" failed to find object named \"" +  name + "\"\n");

   
   #ifdef DEBUG_WRAPPER_CODE   
   MessageInterface::ShowMessage
      ("Subscriber::SetWrapperReference() returning %d\n", (retval1 | retval2));
   #endif
   
   return (retval1 | retval2);
}


//------------------------------------------------------------------------------
// void WriteWrappers()
//------------------------------------------------------------------------------
void Subscriber::WriteWrappers()
{
   MessageInterface::ShowMessage
      ("Subscriber::WriteWrappers() <%p>'%s' has %d xParamWrappers and %d "
       "yParamWrappers\n", this, GetName().c_str(), xParamWrappers.size(),
       yParamWrappers.size());
   
   ElementWrapper *wrapper;
   for (UnsignedInt i = 0; i < xParamWrappers.size(); ++i)
   {
      wrapper = xParamWrappers[i];
      MessageInterface::ShowMessage
         ("   xParamWrapper = <%p>'%s'\n", wrapper,
          wrapper ? wrapper->GetDescription().c_str() : "NULL");
   }
   
   for (UnsignedInt i = 0; i < yParamWrappers.size(); ++i)
   {
      wrapper = yParamWrappers[i];
      MessageInterface::ShowMessage
         ("   yParamWrapper = <%p>'%s'\n", wrapper,
          wrapper ? wrapper->GetDescription().c_str() : "NULL");
   }
}


//------------------------------------------------------------------------------
// Integer FindIndexOfElement(StringArray &labelArray, const std::string &label)
//------------------------------------------------------------------------------
Integer Subscriber::FindIndexOfElement(StringArray &labelArray,
                                       const std::string &label)
{
   std::vector<std::string>::iterator pos;
   pos = find(labelArray.begin(), labelArray.end(),  label);
   if (pos == labelArray.end())
      return -1;
   else
      return distance(labelArray.begin(), pos);
}


//------------------------------------------------------------------------------
// bool ShouldDataBeSkipped(Integer whichWrapper)
//------------------------------------------------------------------------------
/**
 * Determins if data should be skipped. Data needs to be skipped if subscriber
 * is not created inside a function and data is not published inside a function.
 *
 * @param whichWrapper  input wrapper indicator (1 = X wrapper, 2 = Y wrapper)
 */
//------------------------------------------------------------------------------
bool Subscriber::ShouldDataBeSkipped(Integer whichWrapper)
{
   #ifdef DEBUG_SKIP_DATA
   MessageInterface::ShowMessage
      ("Subscriber::ShouldDataBeSkipped() <%p>'%s' entered, whichWrapper=%d, "
       "xParamWrappers.size()=%d, yParamWrappers.size()=%d\n", this, GetName().c_str(),
       whichWrapper, xParamWrappers.size(), yParamWrappers.size());
   #endif
   
   bool skipData = false;
   bool isValidData = true;
   GmatBase *refObj = NULL;
   
   // Check wrapper size first
   if (whichWrapper == 1)
   {
      if (xParamWrappers.empty())
         isValidData = false;
   }
   else if (whichWrapper == 2)
   {
      if (yParamWrappers.empty())
         isValidData = false;
   }
   else
      isValidData = false;
   
   if (!isValidData)
   {
      std::string wrapperName = "Unknown wrappers";
      if (whichWrapper == 1)
         wrapperName = "X wrappers";
      else if (whichWrapper == 2)
         wrapperName = "Y wrappers";
      
      #ifdef DEBUG_SKIP_DATA
      MessageInterface::ShowMessage
         ("Subscriber::ShouldDataBeSkipped() <%p>'%s' returning false, failed to "
          "check valid data for %s\n", this, GetName().c_str(), wrapperName.c_str());
      #endif
      return false;
   }
   
   #ifdef DEBUG_SKIP_DATA
   MessageInterface::ShowMessage
      ("   Data is published from the function, '%s' IsGlobal:%s, IsLocal:%s\n",
       GetName().c_str(), IsGlobal() ? "Yes" : "No", IsLocal() ? "Yes" : "No");
   #endif
   
   // By this time parameter wrappers should not be empty
   if (whichWrapper == 1)
      refObj = xParamWrappers[0]->GetRefObject();
   else
      refObj = yParamWrappers[0]->GetRefObject();
   
   #ifdef DEBUG_SKIP_DATA
   MessageInterface::ShowMessage
      ("   refObj=<%p>[%s]'%s'\n", refObj, refObj ? refObj->GetTypeName().c_str() : "NULL",
       refObj ? refObj->GetName().c_str() : "NULL");
   #endif
   
   if (refObj)
   {
      #ifdef DEBUG_SKIP_DATA
      MessageInterface::ShowMessage
         ("   refObj=<%p>[%s]'%s', IsGlobal=%d, IsLocal=%d\n",
          refObj, refObj->GetTypeName().c_str(), refObj->GetName().c_str(), refObj->IsGlobal(),
          refObj->IsLocal());
      GmatBase *paramOwner = ((Parameter*)refObj)->GetOwner();
      MessageInterface::ShowMessage
         ("   paramOwner=<%p>[%s]'%s', IsGlobal=%d, IsLocal=%d\n", paramOwner,
          paramOwner ? paramOwner->GetTypeName().c_str() : "NULL",
          paramOwner ? paramOwner->GetName().c_str() : "NULL",
          paramOwner ? paramOwner->IsGlobal() : -999,
          paramOwner ? paramOwner->IsLocal() : -999);
      #endif
      
      // Skip if this XyPlot is not a global nor a local object
      if (!IsGlobal() && !IsLocal())
      {
         #ifdef DEBUG_SKIP_DATA
         MessageInterface::ShowMessage
            ("   Skip data since this subscriber is not a global nor a local object\n");
         #endif
         skipData = true;
      }
      
      // Skip if ref Parameter is not a global nor a local object
      if (!(refObj->IsGlobal()) && !(refObj->IsLocal()))
      {
         #ifdef DEBUG_SKIP_DATA
         MessageInterface::ShowMessage
            ("   Skip data since ref Parameter is not a global nor a local object\n");
         #endif
         skipData = true;
      }
      
      // Check all Y wrappers
      if (!skipData)
      {
         int numYParams = yParamWrappers.size();
         for (int i = 0; i < numYParams; i++)
         {
            #ifdef DEBUG_SKIP_DATA
            MessageInterface::ShowMessage
               ("   yParamWrappers[%d]=<%p>\n", i, yParamWrappers[i]);
            #endif
            if (yParamWrappers[i] == NULL)
               break;
            
            refObj = yParamWrappers[i]->GetRefObject();
            
            #ifdef DEBUG_SKIP_DATA
            MessageInterface::ShowMessage
               ("   refObj=<%p>[%s]'%s'\n", refObj, refObj ? refObj->GetTypeName().c_str() : "NULL",
                refObj ? refObj->GetName().c_str() : "NULL");
            #endif
            
            if (refObj)
            {
               #ifdef DEBUG_SKIP_DATA
               MessageInterface::ShowMessage
                  ("   refObj = <%p>[%s]'%s', IsGlobal=%d, IsLocal=%d\n",
                   refObj, refObj->GetTypeName().c_str(), refObj->GetName().c_str(), refObj->IsGlobal(),
                   refObj->IsLocal());
               GmatBase *paramOwner = ((Parameter*)refObj)->GetOwner();
               MessageInterface::ShowMessage
                  ("   paramOwner=<%p>[%s]'%s', IsGlobal=%d, IsLocal=%d\n", paramOwner,
                   paramOwner ? paramOwner->GetTypeName().c_str() : "NULL",
                   paramOwner ? paramOwner->GetName().c_str() : "NULL",
                   paramOwner ? paramOwner->IsGlobal() : -999,
                   paramOwner ? paramOwner->IsLocal() : -999);
               #endif
               
               // Skip if this subscriber is not a global nor a local object
               if (!IsGlobal() && !IsLocal())
               {
                  #ifdef DEBUG_SKIP_DATA
                  MessageInterface::ShowMessage
                     ("   Skip data since this subscriber is not a global nor a local object\n");
                  #endif
                  skipData = true;
                  break;
               }
               // Skip if Parameter is not a global nor a local object
               if (!(refObj->IsGlobal()) && !(refObj->IsLocal()))
               {
                  #ifdef DEBUG_SKIP_DATA
                  MessageInterface::ShowMessage
                     ("   Skip data since ref Parameter is not a global nor a local object\n");
                  #endif
                  skipData = true;
                  break;
               }
            }
         }
      }
   }
   
   #ifdef DEBUG_SKIP_DATA
   MessageInterface::ShowMessage
      ("Subscriber::ShouldDataBeSkipped() this=<%p>'%s' returning %s\n", this,
       GetName().c_str(), skipData ? "true" : "false");
   #endif
   return skipData;
}


//------------------------------------------------------------------------------
// bool Distribute(Integer len)
//------------------------------------------------------------------------------
bool Subscriber::Distribute(Integer len)
{
   return true;
}


//------------------------------------------------------------------------------
// bool Distribute(const Real *dat, Integer len)
//------------------------------------------------------------------------------
bool Subscriber::Distribute(const Real *dat, Integer len)
{
   return true;
}


//------------------------------------------------------------------------------
// virtual void HandleManeuvering(GmatBase *originator, bool maneuvering, Real epoch,
//                                const StringArray &satNames,
//                                const std::string &desc)
//------------------------------------------------------------------------------
/**
 * Handles maneuvering on or off.
 * Derived subscribers should override this method if necessary.
 * 
 * @param originator  The maneuver command pointer who is maneuvering
 * @param maneuvering  Set to true if maneuvering
 * @param epoch  Epoch of maneuver on or off
 * @param satNames  Names of the maneuvering spacecraft
 * @param desc  Description of maneuver (e.g. impulsive or finite)
 */
//------------------------------------------------------------------------------
void Subscriber::HandleManeuvering(GmatBase *originator, bool maneuvering,
                                   Real epoch, const StringArray &satNames,
                                   const std::string &desc)
{
   // do nothing here
}


//------------------------------------------------------------------------------
// virtual void HandleSpacecraftPropertyChange(GmatBase *originator, Real epoch,
//                             const std::string &satName, const std::string &desc)
//------------------------------------------------------------------------------
/**
 * Handles spacecraft epoch change.
 * Derived subscribers should override this method if necessary.
 * 
 * @param originator  The assignment command pointer who is setting
 * @param epoch  Epoch of spacecraft at property change
 * @param satName  Name of the spacecraft
 * @param desc  Description of property change
 */
//------------------------------------------------------------------------------
void Subscriber::HandleSpacecraftPropertyChange(GmatBase *originator, Real epoch,
                                                const std::string &satName,
                                                const std::string &desc)
{
   // do nothing here
}


//------------------------------------------------------------------------------
// void PutUnsignedIntValue(Integer id, const std::string &sval)
//------------------------------------------------------------------------------
void Subscriber::PutUnsignedIntValue(Integer id, const std::string &sval)
{
   #ifdef DEBUG_SUBSCRIBER_PUT
   MessageInterface::ShowMessage
      ("PutUnsignedIntValue() id=%d, sval='%s'\n", id, sval.c_str());
   #endif

   UnsignedIntArray vals = GmatStringUtil::ToUnsignedIntArray(sval);
   for (UnsignedInt i=0; i<vals.size(); i++)
      SetUnsignedIntParameter(id, vals[i], i);
}


//---------------------------------------------------------------------------
// const StringArray& GetPropertyEnumStrings(const Integer id) const
//---------------------------------------------------------------------------
const StringArray& Subscriber::GetPropertyEnumStrings(const Integer id) const
{
   if (id == SOLVER_ITERATIONS)
      return solverIterOptions;
   
   return GmatBase::GetPropertyEnumStrings(id);
}


//---------------------------------------------------------------------------
// const StringArray& GetPropertyEnumStrings(const std::string &label) const
//---------------------------------------------------------------------------
const StringArray& Subscriber::GetPropertyEnumStrings(const std::string &label) const
{
   return GetPropertyEnumStrings(GetParameterID(label));
}

//---------------------------------------------------------------------------
// bool SetActualWrapperReference(const WrapperArray &wrappers, GmatBase *obj,
//         const std::string &name);
//---------------------------------------------------------------------------
bool Subscriber::SetActualWrapperReference(const WrapperArray &wrappers,
                                           GmatBase *obj, const std::string &name)
{
   Integer sz = wrappers.size();
   std::string refname, desc;
   Gmat::WrapperDataType wrapperType;
   bool nameFound = false;
   
   #ifdef DEBUG_WRAPPER_CODE   
   MessageInterface::ShowMessage
      ("\nSubscriber::SetActualWrapperReference() <%p>'%s' entered, obj=<%p>'%s', "
       "name='%s', size=%d\n", this, GetName().c_str(), obj, obj->GetName().c_str(),
       name.c_str(), sz);
   for (unsigned int i = 0; i < wrappers.size(); i++)
      MessageInterface::ShowMessage
         ("   wrappers[%d] = '%s'\n", i, wrappers[i] ? wrappers[i]->GetDescription().c_str() : "NULL");
   #endif
   
   for (Integer i = 0; i < sz; i++)
   {
      if (wrappers[i] == NULL)
      {
         #ifdef DEBUG_WRAPPER_CODE   
         MessageInterface::ShowMessage
            ("===> Subscriber::SetActualWrapperReference() throwing exception, wrappers[%d] is NULL\n", i);
         #endif
         throw SubscriberException
            ("Subscriber::SetActualWrapperReference() \"" + GetName() +
             "\" failed to set reference for object named \"" + name +
             ".\" The wrapper is NULL.\n");
      }
      
      refname = wrappers[i]->GetDescription();
      if (wrappers[i]->GetWrapperType() == Gmat::ARRAY_ELEMENT_WT)
         refname = refname.substr(0, refname.find('('));
      
      #ifdef DEBUG_WRAPPER_CODE   
      MessageInterface::ShowMessage("   refname = '%s'\n", refname.c_str());
      #endif
      
      if (refname == name)
      {
         nameFound = true;
         break;
      }
   }
   
   if (!nameFound)
   {
      // Moved this code to SetWrapperReference() so that both X and Y wrappers
      // can be checked. (LOJ: 2012.08.02)
      //throw SubscriberException
      //   ("Subscriber::SetActualWrapperReference() \"" + GetName() +
      //    "\" failed to find object named \"" +  name + "\"\n");
      #ifdef DEBUG_WRAPPER_CODE   
      MessageInterface::ShowMessage
         ("Subscriber::SetActualWrapperReference() returning false, '%s' not found\n",
          name.c_str());
      #endif
     return false;
   }
   
   #ifdef DEBUG_WRAPPER_CODE   
   MessageInterface::ShowMessage("   setting ref object of wrappers\n");
   #endif
   
   // set ref object of wrappers
   for (Integer i = 0; i < sz; i++)
   {
      desc = wrappers[i]->GetDescription();
      wrapperType = wrappers[i]->GetWrapperType();
      
      #ifdef DEBUG_WRAPPER_CODE   
      MessageInterface::ShowMessage
         ("   wrappers[%d]=\"%s\", wrapperType=%d\n", i, desc.c_str(), wrapperType);
      #endif
      
      refname = desc;
      
      switch (wrapperType)
      {
      case Gmat::OBJECT_PROPERTY_WT:
         if (refname == name)
         {
            Parameter *param = (Parameter*)obj;
            StringArray onames = wrappers[i]->GetRefObjectNames();
            for (UnsignedInt j=0; j<onames.size(); j++)
            {
               wrappers[i]->
                  SetRefObject(param->GetRefObject(param->GetOwnerType(), onames[j]));
            }
            break;
         }
      case Gmat::ARRAY_ELEMENT_WT:
         // for array element, we need to go through all array elements set
         // ref object, so break switch insted of return
         refname = refname.substr(0, refname.find('('));
         if (refname == name)
         {
            #ifdef DEBUG_WRAPPER_CODE   
            MessageInterface::ShowMessage
               ("   Setting array element wrapper '%s'\n", name.c_str());
            #endif
            wrappers[i]->SetRefObject(obj);
         }
         break;
      default:
         // Others, such as VARIABLE, PARAMETER_OBJECT
         if (refname == name)
         {
            #ifdef DEBUG_WRAPPER_CODE   
            MessageInterface::ShowMessage
               ("   Setting other wrapper '%s'\n", name.c_str());
            #endif
            
            wrappers[i]->SetRefObject(obj);
         }
      }
   }
   
   #ifdef DEBUG_WRAPPER_CODE   
   MessageInterface::ShowMessage
      ("Subscriber::SetActualWrapperReference() returning true\n");
   #endif
   
   return true;
}
