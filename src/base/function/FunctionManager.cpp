//$Id$
//------------------------------------------------------------------------------
//                                 FunctionManager
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
// Author: Wendy C. Shoan
// Created: 2008.03.24
//
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under contract
// number NNG06CCA54C
//
/**
 * Class implementation for the FunctionManager command
 */
//------------------------------------------------------------------------------
#include "ScriptStringLiteral.hpp"
#include <stack>
#include <exception>

#include "FunctionManager.hpp"
#include "ObjectStoreLifetime.hpp"
#include "CommandUtil.hpp"
#include "ExpressionBinding.hpp"
#include "MathTree.hpp"
#include "MathNode.hpp"
#include "FunctionException.hpp"
#include "EventLocator.hpp"
#include "PlanetographicRegion.hpp"
#include "GmatCommand.hpp"
#include "StringUtil.hpp"
#include "Array.hpp"
#include "ElementWrapper.hpp"
#include "NumberWrapper.hpp"
#include "RvectorWrapper.hpp"
#include "ObjectWrapper.hpp"
#include "VariableWrapper.hpp"
#include "ArrayWrapper.hpp"
#include "StringObjectWrapper.hpp"
#include "RealUtilities.hpp"
#include "Variable.hpp"
#include "RvectorVar.hpp"
#include "StringVar.hpp"
#include "UserDefinedFunction.hpp"
#include "Subscriber.hpp"
#include "MessageInterface.hpp"
#include "GmatGlobal.hpp"
#include "EphemerisFile.hpp"
#include "CalculatedPoint.hpp"
#include "FormationInterface.hpp"
#include "GroundstationInterface.hpp"
#include "DragForce.hpp"
#include "Burn.hpp"
#include "FiniteThrust.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "Thruster.hpp"
#include "PowerSystem.hpp"
#include "Attitude.hpp"
#include "Plate.hpp"
#include "StopCondition.hpp"
#include "Solver.hpp"
#include "FunctionValueContext.hpp"
#include "NativeValueProvider.hpp"
#include <exception>
#include <set>
#include <mutex>
#include <typeinfo>
#include <limits>


// Displaced outputs stay with the actual owner of their store. Multiple
// managers can use one callee/store, so the replacing manager need not own it.
struct FunctionBuiltinOutputResources
{
   struct Retired
   {
      GmatBase *object;
      std::weak_ptr<const void> identity;
   };
   std::vector<Retired> retired;

   void Retain(GmatBase *object)
   {
      if (!object || (object->IsGlobal() && !object->IsLocal())) return;
      for (const auto &entry : retired)
         if (entry.object == object && !entry.identity.expired()) return;
      retired.push_back({object, object->GetLifetimeToken()});
   }

   ObjectArray LiveObjects() const
   {
      ObjectArray result;
      result.reserve(retired.size());
      for (const auto &entry : retired)
         if (!entry.identity.expired()) result.push_back(entry.object);
      return result;
   }

   ~FunctionBuiltinOutputResources()
   {
      // Normal Cleanup deduplicates these with all active/recursive stores.
      // The identity also makes fallback destruction safe if another owning
      // map has already deleted an alias, including on exceptional cleanup.
      for (const auto &entry : retired)
         if (!entry.identity.expired() &&
               (!entry.object->IsGlobal() || entry.object->IsLocal()))
            delete entry.object;
   }
};

namespace
{
thread_local size_t activeManagerExecutions = 0;

struct BuiltinOutputStoreRegistry
{
   struct Registration
   {
      std::weak_ptr<FunctionBuiltinOutputResources> owner;
      std::shared_ptr<const void> identity;
   };
   std::mutex mutex;
   std::map<ObjectMap*, Registration> stores;
};

BuiltinOutputStoreRegistry& BuiltinOutputStores()
{
   // Managers can be destroyed during process shutdown after ordinary statics.
   static auto *registry = new BuiltinOutputStoreRegistry;
   return *registry;
}

void RegisterBuiltinOutputStore(ObjectMap *store,
      const std::shared_ptr<FunctionBuiltinOutputResources> &owner)
{
   auto identity = std::make_shared<const int>(0);
   auto &registry = BuiltinOutputStores();
   std::lock_guard<std::mutex> lock(registry.mutex);
   registry.stores[store] = {owner, std::move(identity)};
}

using BuiltinOutputStore = std::pair<std::shared_ptr<FunctionBuiltinOutputResources>,
      std::weak_ptr<const void>>;

BuiltinOutputStore FindBuiltinOutputStore(ObjectMap *store)
{
   auto &registry = BuiltinOutputStores();
   std::lock_guard<std::mutex> lock(registry.mutex);
   const auto found = registry.stores.find(store);
   if (found == registry.stores.end()) return {};
   auto owner = found->second.owner.lock();
   if (!owner)
   {
      registry.stores.erase(found);
      return {};
   }
   return {std::move(owner), found->second.identity};
}

std::shared_ptr<FunctionBuiltinOutputResources> FindBuiltinOutputStoreOwner(ObjectMap *store)
{
   return FindBuiltinOutputStore(store).first;
}

void UnregisterBuiltinOutputStore(ObjectMap *store,
      const std::shared_ptr<FunctionBuiltinOutputResources> &owner)
{
   auto &registry = BuiltinOutputStores();
   std::shared_ptr<FunctionBuiltinOutputResources> current;
   {
      std::lock_guard<std::mutex> lock(registry.mutex);
      const auto found = registry.stores.find(store);
      if (found == registry.stores.end()) return;
      current = found->second.owner.lock();
      if (current == owner) registry.stores.erase(found);
   }
}

bool MatchesBuiltinOutput(GmatBase *object, Gmat::WrapperDataType type,
      Integer rows, Integer columns)
{
   if (!object) return false;
   if (type == Gmat::VARIABLE_WT) return typeid(*object) == typeid(Variable);
   if (type == Gmat::STRING_OBJECT_WT) return typeid(*object) == typeid(StringVar);
   if (type == Gmat::ARRAY_WT)
   {
      auto *array = dynamic_cast<Array*>(object);
      return array && typeid(*object) == typeid(Array) &&
            array->GetRowCount() == rows && array->GetColCount() == columns;
   }
   if (type == Gmat::VECTOR_WT)
   {
      auto *vector = dynamic_cast<RvectorVar*>(object);
      return vector && typeid(*object) == typeid(RvectorVar) &&
            vector->GetIntegerParameter("VectorSize") == (rows == 1 ? columns : rows);
   }
   return false;
}
}

namespace
{
thread_local const GmatBase *functionValueSnapshotSource = NULL;
GmatBase *CloneFunctionValue(const GmatBase *source)
{
   struct SnapshotScope
   {
      const GmatBase *previous;
      explicit SnapshotScope(const GmatBase *source) : previous(functionValueSnapshotSource)
      { functionValueSnapshotSource = source; }
      ~SnapshotScope() { functionValueSnapshotSource = previous; }
   } scope(source);
   return source ? source->Clone() : NULL;
}

thread_local unsigned int functionValueCopyDepth = 0;
struct FunctionValueCopyScope
{
   FunctionValueCopyScope() { ++functionValueCopyDepth; }
   ~FunctionValueCopyScope() { --functionValueCopyDepth; }
};

Rmatrix ReadFunctionArray(ElementWrapper &wrapper)
{
   if (wrapper.GetDataType() == Gmat::RMATRIX_TYPE)
      return wrapper.EvaluateArray();
   if (wrapper.GetDataType() == Gmat::RVECTOR_TYPE)
   {
      // Native Rvectors have no row/column orientation. GMAT function formals
      // use Arrays, so materialize a column without borrowing the wrapper's
      // evaluation buffer across invocation cleanup or recursive restoration.
      const Rvector &value = wrapper.EvaluateRvector();
      Rmatrix column;
      column.MakeOneColumnMatrix(value);
      return column;
   }
   throw FunctionException("Function Array input or output must be a matrix or vector.\n");
}

bool SupportsAuxiliaryFunctionValue(GmatBase *object)
{
   return object && (dynamic_cast<EphemerisFile*>(object) ||
         object->IsOfType(Gmat::FIELD_OF_VIEW) ||
         object->IsOfType(Gmat::ERROR_MODEL) || object->IsOfType(Gmat::DATA_FILTER) ||
         object->IsOfType(Gmat::PARAMETER) || object->IsOfType(Gmat::STOP_CONDITION) ||
         object->IsOfType("ProcessNoiseModel") || object->IsOfType("ProcessNoise") ||
         object->IsOfType("EstimatedParameter") || object->IsOfType("EstimatedParameterModel") ||
         object->IsOfType(Gmat::HARDWARE) || object->IsOfType(Gmat::ATTITUDE) ||
         object->IsOfType(Gmat::PLATE) || object->IsOfType("TrackingFileSet") ||
         typeid(*object) == typeid(PlanetographicRegion) ||
         (dynamic_cast<EventLocator*>(object) &&
          static_cast<EventLocator*>(object)->HasEventLocatorValueReferenceProvider()) ||
         (dynamic_cast<Subscriber*>(object) &&
          static_cast<Subscriber*>(object)->HasSubscriberValueReferenceProvider()) ||
         (dynamic_cast<Function*>(object) &&
          static_cast<Function*>(object)->HasFunctionValueReferenceProvider()) ||
         (dynamic_cast<Solver*>(object) &&
          static_cast<Solver*>(object)->HasSolverValueReferenceProvider()));
}

bool SupportsRetainedFunctionRoot(GmatBase *object)
{
   return object && (dynamic_cast<CoordinateSystem*>(object) ||
         dynamic_cast<CalculatedPoint*>(object) ||
         dynamic_cast<Spacecraft*>(object) || dynamic_cast<ODEModel*>(object) ||
         dynamic_cast<FormationInterface*>(object) || dynamic_cast<Burn*>(object) ||
         dynamic_cast<PropSetup*>(object) || dynamic_cast<GroundstationInterface*>(object) ||
         SupportsAuxiliaryFunctionValue(object));
}

bool OwnsFunctionObject(const GmatBase *object)
{
   return object && (!object->IsGlobal() || object->IsLocal());
}

// A retained function value and its dependencies form one resource graph.
// The source store remains authoritative for ownership, and caller mappings
// use exact source identities. Unrelated equal-name caller objects are ignored.
struct ReturnedEphemerisFrames : public FunctionValueContext,
      public std::enable_shared_from_this<ReturnedEphemerisFrames>
{
   SolarSystem *solarSystem;
   std::set<GmatBase*> privateObjects;
   std::map<GmatBase*, GmatBase*> callerObjects;
   std::map<GmatBase*, GmatBase*> copies;
   std::set<GmatBase*> visiting;
   ObjectArray resolutionPath;
   std::set<GmatBase*> nativeOwned;
   std::set<GmatBase*> reboundOwned;
   ObjectArray objects;
   ObjectArray initializationOrder;
   std::vector<std::unique_ptr<GmatBase>> owned;
   std::vector<std::shared_ptr<const ObjectArray>> inheritedEphemerisOwners;
   std::vector<std::shared_ptr<const FunctionValueContext>> inheritedValues;
   std::set<GmatBase*> externalReferences;
   std::map<const GmatBase*, GmatBase*> origins;
   std::shared_ptr<const void> bindingEpoch;
   const GmatBase *rootIdentity = NULL;
   GmatBase *publicRootValue = NULL;
   std::weak_ptr<const void> publicRootLifetime;
   std::vector<PhysicalModel*> *runtimeForces = NULL;
   bool reportingContext = false;

   const ObjectArray& GetPrivateObjects() const override { return objects; }

   bool RetainsReference(const GmatBase *object) const override
   {
      if (externalReferences.count(const_cast<GmatBase*>(object))) return true;
      for (const auto &entry : copies)
         if (entry.second == object) return true;
      return false;
   }

   void IncludeContext(GmatBase *source)
   {
      const auto context = source->GetFunctionValueContext();
      if (!context) return;
      const ObjectArray &dependencies = context->GetPrivateObjects();
      privateObjects.insert(dependencies.begin(), dependencies.end());
      const auto *owner = dynamic_cast<const ReturnedEphemerisFrames*>(context.get());
      if (owner && owner->publicRootValue && owner->publicRootValue != source)
      {
         // A detached interior value can refer back to the original public
         // root. Snapshot that root too rather than leaving a borrowed pointer
         // into the enclosing value. Contexts never own their public root.
         if (owner->publicRootLifetime.expired())
            throw FunctionException("Cannot detach a function dependency after its public root has expired.\n");
         privateObjects.insert(owner->publicRootValue);
      }
      inheritedValues.push_back(context);
   }

   void RebindCopy(const GmatBase &source, GmatBase &target) const override
   {
      // Native Clone completes the derived object before reconnecting a public
      // root. Dependency clones are instead bound by this enclosing builder.
      if (functionValueCopyDepth) return;
      auto next = std::make_shared<ReturnedEphemerisFrames>();
      next->solarSystem = solarSystem;
      next->bindingEpoch = bindingEpoch;
      next->rootIdentity = rootIdentity;
      next->runtimeForces = runtimeForces;
      next->reportingContext = reportingContext;
      // Native buffer assignment can retain members from an older context
      // while copying a new body. Import the actual source's combined leases.
      next->IncludeContext(const_cast<GmatBase*>(&source));
      const auto previous = target.GetFunctionValueContext();
      if (previous) next->inheritedValues.push_back(previous);
      target.SetFunctionValueContext(next);
      const bool dormantFormation = functionValueSnapshotSource != &source &&
            source.IsOfType(Gmat::FORMATION) &&
            target.GetRefObjectArray(Gmat::SPACECRAFT).empty();
      bool dormantPropSetup = false;
      if (functionValueSnapshotSource != &source && source.IsOfType(Gmat::PROP_SETUP))
      {
         auto *setup = dynamic_cast<PropSetup*>(&target);
         ObjectArray registered;
         if (setup) setup->GetPropStateManager()->GetStateObjects(registered, Gmat::UNKNOWN_OBJECT);
         dormantPropSetup = registered.empty();
      }
      next->BindCopy(const_cast<GmatBase*>(&source), &target, true, dormantFormation, dormantPropSetup);
      next->ForwardOrigins(*this);
      next->Initialize();
      next->inheritedValues.clear();
   }

   void ForwardOrigins(const ReturnedEphemerisFrames &previous)
   {
      for (const auto &entry : previous.origins)
      {
         const auto copy = copies.find(entry.second);
         if (copy != copies.end()) origins[entry.first] = copy->second;
         else if (previous.externalReferences.count(entry.second))
         {
            origins[entry.first] = entry.second;
            externalReferences.insert(entry.second);
         }
      }
   }

   std::shared_ptr<ExpressionScopeCopies> expressionBindings = std::make_shared<ExpressionScopeCopies>();

   ObjectArray NativeChildren(GmatBase *parent)
   {
      const bool hasOwnershipProvider = FindNativeValueProvider(
            *parent, NativeValueOperation::OwnedObjects).ownedObjects != nullptr;
      ObjectArray disclosed = parent->GetFunctionValueOwnedObjects();
      if (reportingContext && dynamic_cast<ODEModel*>(parent))
         disclosed.erase(std::remove_if(disclosed.begin(), disclosed.end(), [](GmatBase *object)
         {
            auto *force = dynamic_cast<PhysicalModel*>(object);
            return force && force->IsTransient();
         }), disclosed.end());
      // An explicitly empty ownership list is authoritative. Falling through
      // would turn borrowed FOV/error-model references into owned components.
      if (hasOwnershipProvider || !disclosed.empty()) return disclosed;
      ObjectArray result;
      if (auto *function = dynamic_cast<Function*>(parent))
         return function->GetFunctionValueOwnedObjects();
      else if (auto *subscriber = dynamic_cast<Subscriber*>(parent))
         return subscriber->GetFunctionValueOwnedObjects();
      else if (auto *setup = dynamic_cast<PropSetup*>(parent))
      {
         result.push_back(setup->GetPropagator());
         result.push_back(setup->GetODEModel());
      }
      else if (auto *station = dynamic_cast<GroundstationInterface*>(parent))
         return station->GetFunctionValueOwnedObjects();
      else if (auto *condition = dynamic_cast<StopCondition*>(parent))
         return condition->GetFunctionValueOwnedObjects();
      else if (auto *model = dynamic_cast<ODEModel*>(parent))
      {
         for (Integer i = 0; i < model->GetOwnedObjectCount(); ++i)
         {
            auto *force = dynamic_cast<PhysicalModel*>(model->GetOwnedObject(i));
            if (force && (!reportingContext || !force->IsTransient())) result.push_back(force);
         }
      }
      else if (auto *spacecraft = dynamic_cast<Spacecraft*>(parent))
      {
         result.push_back(spacecraft->GetOwnedObject(0));
         const ObjectArray power = spacecraft->GetRefObjectArray(Gmat::POWER_SYSTEM);
         result.push_back(power.empty() ? NULL : power.front());
         for (UnsignedInt type : {Gmat::FUEL_TANK, Gmat::THRUSTER,
               Gmat::HARDWARE, Gmat::PLATE})
         {
            const ObjectArray children = spacecraft->GetRefObjectArray(type);
            result.insert(result.end(), children.begin(), children.end());
         }
      }
      else if (auto *frame = dynamic_cast<CoordinateSystem*>(parent))
         result.push_back(frame->GetAxisSystem());
      else if (parent->IsOfType("ProcessNoiseModel") || parent->IsOfType("EstimatedParameter"))
      {
         const UnsignedInt type = GmatType::GetTypeId(parent->IsOfType("ProcessNoiseModel") ?
               "ProcessNoise" : "EstimatedParameterModel");
         result.push_back(parent->GetRefObject(type, ""));
      }
      if (parent->HasRefObjectTypeArray())
      {
         const ObjectTypeArray types = parent->GetRefObjectTypeArray();
         for (UnsignedInt type : types)
            if (type == Gmat::FIELD_OF_VIEW || type == Gmat::ERROR_MODEL)
            {
               const StringArray names = parent->GetRefObjectNameArray(type);
               for (const std::string &name : names)
                  result.push_back(parent->GetRefObject(type, name));
            }
      }
      return result;
   }

   void BindCallerTree(GmatBase *source, GmatBase *target)
   {
      if (!source && !target) return;
      if (!source || !target || source->GetTypeName() != target->GetTypeName())
         throw FunctionException("Incompatible caller resource in function output binding.\n");
      const auto previous = callerObjects.find(source);
      if (previous != callerObjects.end())
      {
         if (previous->second != target)
            throw FunctionException("Conflicting caller component bindings in function outputs.\n");
         return;
      }
      callerObjects[source] = target;
      externalReferences.insert(target);
      const ObjectArray from = NativeChildren(source);
      const ObjectArray to = NativeChildren(target);
      if (from.size() != to.size())
         throw FunctionException("Caller resource changed its owned component collection during function return.\n");
      for (UnsignedInt i = 0; i < from.size(); ++i)
      {
         auto *force = dynamic_cast<PhysicalModel*>(from[i]);
         auto *callerForce = dynamic_cast<PhysicalModel*>(to[i]);
         if (force && callerForce && force->GetBodyName() != callerForce->GetBodyName())
            throw FunctionException("Caller force components have incompatible body roles.\n");
         BindCallerTree(from[i], to[i]);
      }
   }

   void RebindCaller(GmatBase &target, const std::shared_ptr<const void> &epoch,
         const std::map<const GmatBase*, GmatBase*> &bindings) const override
   {
      // Origin addresses are meaningful only within one captured invocation.
      // The shared marker prevents older retained values from matching a reused
      // address in a later function frame.
      if (!epoch || bindingEpoch != epoch) return;
      std::unique_ptr<GmatBase> source(CloneFunctionValue(&target));
      if (source.get() == &target)
      {
         source.release();
         throw FunctionException("Function output rebinding Clone returned its source.\n");
      }
      if (!source) throw FunctionException("Cannot capture a function output for caller binding.\n");
      const auto sourceOwner = source->GetFunctionValueContext();
      const auto *reader = dynamic_cast<const ReturnedEphemerisFrames*>(sourceOwner.get());
      if (!reader || reader->bindingEpoch != epoch)
         throw FunctionException("Missing captured function output binding context.\n");
      auto next = std::make_shared<ReturnedEphemerisFrames>();
      next->solarSystem = solarSystem;
      next->runtimeForces = runtimeForces;
      next->reportingContext = reader->reportingContext;
      next->bindingEpoch = epoch;
      next->rootIdentity = reader->rootIdentity;
      next->privateObjects.insert(reader->objects.begin(), reader->objects.end());
      for (const auto &entry : bindings)
      {
         if (entry.first == reader->rootIdentity) continue;
         const auto origin = reader->origins.find(entry.first);
         if (origin != reader->origins.end()) next->BindCallerTree(origin->second, entry.second);
      }
      const auto previous = target.GetFunctionValueContext();
      if (previous) next->inheritedValues.push_back(previous);
      next->inheritedValues.push_back(sourceOwner);
      target.SetFunctionValueContext(next);
      // Bind against an independent reader. Replacing only references leaves
      // the caller's already-copied owned component addresses stable.
      next->BindCopy(source.get(), &target, true);
      next->ForwardOrigins(*reader);
      next->Initialize();
      next->inheritedValues.clear();
   }

   ~ReturnedEphemerisFrames()
   {
      // Resource roots precede their newly discovered dependencies here.
      // Retire borrowers while their attached objects and other refs are live.
      for (auto &object : owned) object.reset();
   }

   GmatBase *FindNativeOwner(GmatBase *component)
   {
      std::set<GmatBase*> seen;
      ObjectArray candidates(privateObjects.begin(), privateObjects.end());
      for (size_t i = 0; i < candidates.size(); ++i)
      {
         GmatBase *owner = candidates[i];
         if (!owner || !seen.insert(owner).second) continue;
         const ObjectArray children = NativeChildren(owner);
         for (auto *child : children)
         {
            if (child == component) return owner;
            if (child && !seen.count(child)) candidates.push_back(child);
         }
      }
      return NULL;
   }

   GmatBase *Resolve(GmatBase *source)
   {
      const bool scopeReference = ExpressionScopeCopies::ConsumeScopeReference();
      if (!source) return NULL;
      if (nativeOwned.find(source) != nativeOwned.end()) return copies.at(source);
      const auto caller = callerObjects.find(source);
      if (caller != callerObjects.end())
      {
         if (!caller->second || !caller->second->IsOfType(source->GetType()))
            throw FunctionException("Incompatible caller resource for returned ephemeris frame.\n");
         origins[source] = caller->second;
         externalReferences.insert(caller->second);
         return caller->second;
      }
      if (visiting.find(source) != visiting.end())
      {
         if (scopeReference)
         {
            const auto bound = copies.find(source);
            if (bound != copies.end()) return bound->second;
         }
         // Hardware/attitude and axes can refer back to the spacecraft whose
         // detached value is being assembled. It already has a stable identity.
         if (source->IsOfType(Gmat::SPACECRAFT) || source->IsOfType(Gmat::ODE_MODEL) ||
             source->IsOfType(Gmat::GROUND_STATION) || dynamic_cast<Function*>(source) ||
             dynamic_cast<EphemerisFile*>(source) ||
             (dynamic_cast<EventLocator*>(source) &&
              static_cast<EventLocator*>(source)->HasEventLocatorValueReferenceProvider()) ||
             (dynamic_cast<Subscriber*>(source) &&
              static_cast<Subscriber*>(source)->HasSubscriberValueReferenceProvider()))
            return copies.at(source);
         if (source->IsOfType(Gmat::COORDINATE_SYSTEM) ||
             source->IsOfType(Gmat::FORMATION))
         {
            bool afterSource = false;
            for (GmatBase *object : resolutionPath)
            {
               if (object == source) afterSource = true;
               // A frame -> spacecraft -> frame reference loop can depend
               // only on Cartesian position. It need not be a cyclic frame
               // computation. All its pointers are bound before initialization.
               if (afterSource && (object->IsOfType(Gmat::SPACECRAFT) ||
                     object->IsOfType(Gmat::GROUND_STATION)))
                  return copies.at(source);
            }
         }
         throw FunctionException("Circular private dependency in returned ephemeris frame '" +
               source->GetName() + "'.\n");
      }
      const auto existing = copies.find(source);
      if (existing != copies.end()) return existing->second;
      if (privateObjects.find(source) == privateObjects.end())
      {
         if (GmatBase *parent = FindNativeOwner(source))
         {
            // A caller's Copy can replace these addresses. Do not manufacture
            // a borrowed source-component lifetime from a root-name mapping.
            if (callerObjects.find(parent) != callerObjects.end())
               throw FunctionException("Returned function value needs owned-component "
                     "binding after caller resource assignment.\n");
            Resolve(parent);
            const auto component = copies.find(source);
            if (component != copies.end()) return component->second;
            throw FunctionException("Native owner did not expose its copied component.\n");
         }
         origins[source] = source;
         externalReferences.insert(source);
         return source;
      }

      IncludeContext(source);
      auto *frame = dynamic_cast<CoordinateSystem*>(source);
      auto *point = dynamic_cast<CalculatedPoint*>(source);
      auto *spacecraft = dynamic_cast<Spacecraft*>(source);
      auto *formation = dynamic_cast<FormationInterface*>(source);
      auto *physical = dynamic_cast<PhysicalModel*>(source);
      auto *burn = dynamic_cast<Burn*>(source);
      auto *setup = dynamic_cast<PropSetup*>(source);
      auto *station = dynamic_cast<GroundstationInterface*>(source);
      auto *propagator = dynamic_cast<Propagator*>(source);
      const bool auxiliary = SupportsAuxiliaryFunctionValue(source);
      if (!frame && !point && !spacecraft && !formation && !physical && !burn && !setup && !station && !propagator && !auxiliary)
         throw FunctionException("Returned ephemeris frame references local " +
               source->GetTypeName() + " '" + source->GetName() +
               "'. Return that resource with the ephemeris or pass it as an input.\n");
      FunctionValueCopyScope copying;
      std::unique_ptr<GmatBase> value(CloneFunctionValue(source));
      bool borrowedClone = value.get() == source || privateObjects.count(value.get());
      for (const auto &entry : copies)
         borrowedClone = borrowedClone || value.get() == entry.second;
      for (const auto &entry : callerObjects)
         borrowedClone = borrowedClone || value.get() == entry.second;
      if (borrowedClone)
      {
         value.release();
         throw FunctionException("A returned ephemeris frame dependency Clone returned its source.\n");
      }
      if (!value || value->GetTypeName() != source->GetTypeName())
         throw FunctionException("Cannot clone returned ephemeris frame dependency '" +
               source->GetName() + "'.\n");
      GmatBase *target = value.get();
      owned.push_back(std::move(value));
      objects.push_back(target);
      BindCopy(source, target, false);
      return target;
   }

   void RebindCloneSource(GmatBase *source, GmatBase *target, bool existingRoot = false)
   {
      // Construction inherits snapshot roles; assignment to an ordinary public
      // resource keeps that resource ordinary. Snapshot destinations are value
      // references, not live definition-notification edges.
      if (source->HasRestorationTarget() && target->HasRestorationTarget())
      {
         auto *destination = source->GetRestorationTarget();
         if (!destination) throw FunctionException("A retained restoration destination has expired.\n");
         if (!copies.count(destination) && !callerObjects.count(destination) &&
             !nativeOwned.count(destination) && (!destination->IsGlobal() || destination->IsLocal()))
            privateObjects.insert(destination);
         auto *mapped = Resolve(destination);
         if (!mapped) throw FunctionException("Cannot bind a retained restoration destination.\n");
         target->SetRestorationTarget(mapped);
      }
      // Assignment to an existing owned component keeps its declaration edge.
      // Fresh native child/value construction registers its new edge below.
      if (existingRoot && target->GetCloneOwner()) return;
      auto *definition = const_cast<GmatBase*>(source->GetCloneSource());
      if (!definition) { target->SetCloneSource(nullptr); return; }
      // A value copy follows the same actual-identity graph as its ordinary
      // references. Unlisted non-global definitions are private snapshots;
      // explicit caller mappings and shared globals retain their native role.
      if (!copies.count(definition) && !callerObjects.count(definition) &&
          !nativeOwned.count(definition) &&
          (!definition->IsGlobal() || definition->IsLocal()))
         privateObjects.insert(definition);
      target->SetCloneSource(Resolve(definition));
   }

   void BindCopy(GmatBase *source, GmatBase *target, bool publicRoot,
         bool dormantFormation = false, bool dormantPropSetup = false)
   {
      ExpressionBindingScope expressionScope(expressionBindings);
      if (!source || !target || source == target ||
          source->GetTypeName() != target->GetTypeName())
         throw FunctionException("Incompatible retained function resource copy.\n");
      auto *ephemeris = dynamic_cast<EphemerisFile*>(source);
      if (publicRoot) reportingContext = ephemeris != NULL;
      auto *frame = dynamic_cast<CoordinateSystem*>(source);
      auto *point = dynamic_cast<CalculatedPoint*>(source);
      auto *spacecraft = dynamic_cast<Spacecraft*>(source);
      auto *formation = dynamic_cast<FormationInterface*>(source);
      auto *model = dynamic_cast<ODEModel*>(source);
      auto *physical = dynamic_cast<PhysicalModel*>(source);
      auto *burn = dynamic_cast<Burn*>(source);
      auto *setup = dynamic_cast<PropSetup*>(source);
      auto *station = dynamic_cast<GroundstationInterface*>(source);
      auto *propagator = dynamic_cast<Propagator*>(source);
      const bool auxiliary = SupportsAuxiliaryFunctionValue(source);
      if (!ephemeris && !frame && !point && !spacecraft && !formation && !physical && !burn && !setup && !station && !propagator && !auxiliary)
         throw FunctionException("Returned ephemeris frame references local " +
               source->GetTypeName() + " '" + source->GetName() +
               "'. Return that resource with the ephemeris or pass it as an input.\n");
      IncludeContext(source);
      // Cached callable references can point into its manager-owned FOS. Those
      // objects are private dependencies even though the FOS itself is not an
      // owned definition component and is never resumed/checkpointed here.
      if (auto *function = dynamic_cast<ObjectManagedFunction*>(source))
         if (auto *store = function->GetFunctionObjectStore())
            for (const auto &entry : *store)
               if (OwnsFunctionObject(entry.second)) privateObjects.insert(entry.second);
      copies[source] = target;
      origins[source] = target;
      if (publicRoot && !rootIdentity) rootIdentity = source;
      if (publicRoot)
      {
         publicRootValue = target;
         publicRootLifetime = target->GetLifetimeToken();
      }
      callerObjects.erase(source);
      visiting.insert(source);
      resolutionPath.push_back(source);
      if (!burn && !dynamic_cast<Thruster*>(source) && !dynamic_cast<PowerSystem*>(source) &&
            !source->IsOfType(Gmat::PARAMETER) && !dynamic_cast<Function*>(source) &&
            !dynamic_cast<Solver*>(source))
         target->SetSolarSystem(solarSystem);
      if (ephemeris)
      {
         auto *copy = dynamic_cast<EphemerisFile*>(target);
         if (!copy) throw FunctionException("Incompatible ephemeris native copy.\n");
         RegisterSubscriberWrappers(*ephemeris, *copy);
         copy->RebindSubscriberEnvironment(*ephemeris,
               [this](GmatBase *object) {return Resolve(object);});
         const auto retained = ephemeris->GetFunctionCoordinateSystemOwners();
         if (retained)
         {
            inheritedEphemerisOwners.push_back(retained);
            privateObjects.insert(retained->begin(), retained->end());
         }
         // A private ephemeris belongs to this graph already. Its back edge
         // must be weak; a public root retains the ordinary owning alias.
         const std::shared_ptr<const ObjectArray> owners = publicRoot ?
               std::shared_ptr<const ObjectArray>(shared_from_this(), &objects) :
               std::shared_ptr<const ObjectArray>();
         copy->RetainFunctionDynamics(
               [this](GmatBase *object) { return Resolve(object); }, runtimeForces, owners);
         copy->RetainFunctionCoordinateSystems(
               [this](CoordinateSystem *frame) { return dynamic_cast<CoordinateSystem*>(Resolve(frame)); }, owners);
         copy->RetainFunctionSpacecraft(
               [this](GmatBase *object) { return Resolve(object); }, owners);
      }
      else if (station)
      {
         auto *copy = dynamic_cast<GroundstationInterface*>(target);
         if (!copy) throw FunctionException("Incompatible GroundStation native copy.\n");
         copy->RebindClonedReferences(*station, solarSystem,
               [this](GmatBase *object) {return Resolve(object);},
               [this](const ObjectArray &from, const ObjectArray &to)
               {
                  for (UnsignedInt i = 0; i < from.size(); ++i) RegisterOwned(from[i], to.at(i));
                  for (UnsignedInt i = 0; i < from.size(); ++i) RebindOwned(from[i], to.at(i));
               });
      }
      else if (setup)
      {
         auto *copy = dynamic_cast<PropSetup*>(target);
         if (!copy) throw FunctionException("PropSetup Clone has an incompatible native type.\n");
         copy->RebindClonedReferences(*setup, solarSystem,
               [this](GmatBase *object) {return Resolve(object);},
               [this, dormantPropSetup](const ObjectArray &from, const ObjectArray &to)
               {
                  for (UnsignedInt i = 0; i < from.size(); ++i) RegisterOwned(from[i], to.at(i));
                  for (UnsignedInt i = 0; i < from.size(); ++i) RebindOwned(from[i], to.at(i), !dormantPropSetup);
               }, !dormantPropSetup);
      }
      else if (model)
      {
         RebindModelReferences(model, dynamic_cast<ODEModel*>(target));
      }
      else if (propagator)
      {
         RebindPropagatorReferences(propagator, dynamic_cast<Propagator*>(target), false);
      }
      else if (frame)
      {
         auto *copy = dynamic_cast<CoordinateSystem*>(target);
         if (!copy)
            throw FunctionException("CoordinateSystem Clone has an incompatible native type.\n");
         AxisSystem *axes = frame->GetAxisSystem();
         AxisSystem *copiedAxes = copy->GetAxisSystem();
         if (!axes || !copiedAxes)
            throw FunctionException("Returned ephemeris frame has no axes.\n");
         RegisterOwned(axes, copiedAxes);
         copiedAxes->SetSolarSystem(solarSystem);
         Bind(copy, Gmat::SPACE_POINT, frame->GetJ2000BodyName(), frame->GetJ2000Body());
         Bind(copy, Gmat::SPACE_POINT, frame->GetOriginName(), frame->GetOrigin());
         // Native axes reference space points and, for aligned/constrained
         // axes, another coordinate system. Copy static name arrays before
         // recursing into another resource's reference accessors.
         const StringArray names = axes->GetRefObjectNameArray(Gmat::UNKNOWN_OBJECT);
         for (const std::string &name : names)
         {
            if (name.empty()) continue;
            bool bound = false;
            for (UnsignedInt type : {Gmat::COORDINATE_SYSTEM, Gmat::SPACE_POINT})
            {
               GmatBase *reference = NULL;
               try {reference = axes->GetRefObject(type, name);}
               catch (BaseException &) {continue;} // This getter does not expose this type.
               // Axes getters check the declared field name. Its bound
               // object's own name can differ after formal/caller aliasing.
               if (!reference || !reference->IsOfType(type)) continue;
               Bind(copiedAxes, type, name, reference);
               bound = true;
               break;
            }
            if (!bound)
               for (GmatBase *object : privateObjects)
                  if (object->GetName() == name)
                     throw FunctionException("Axes do not expose private reference '" + name +
                           "' needed by a returned ephemeris frame.\n");
         }
      }
      else if (point)
      {
         auto *copy = dynamic_cast<CalculatedPoint*>(target);
         if (!copy)
            throw FunctionException("CalculatedPoint Clone has an incompatible native type.\n");
         const StringArray names = point->GetRefObjectNameArray(Gmat::SPACE_POINT);
         for (const std::string &name : names)
         {
            GmatBase *reference = point->GetRefObject(Gmat::SPACE_POINT, name);
            if (!reference || reference->GetName() != name)
               throw FunctionException("Cannot resolve returned frame point dependency '" + name + "'.\n");
            Bind(copy, Gmat::SPACE_POINT, name, reference);
         }
         SpacePoint *j2000 = dynamic_cast<SpacePoint*>(Resolve(point->GetJ2000Body()));
         copy->SetJ2000Body(j2000);
      }
      else if (spacecraft)
      {
         auto *copy = dynamic_cast<Spacecraft*>(target);
         if (!copy) throw FunctionException("Spacecraft Clone has an incompatible native type.\n");
         copy->RebindClonedReferences(*spacecraft,
               [this](GmatBase *object) {return Resolve(object);},
               [this](const ObjectArray &from, const ObjectArray &to)
               {
                  for (UnsignedInt i = 0; i < from.size(); ++i)
                     RegisterOwned(from[i], to.at(i));
                  for (UnsignedInt i = 0; i < from.size(); ++i)
                     RebindOwned(from[i], to.at(i));
               });
      }
      else if (formation)
      {
         auto *copy = dynamic_cast<FormationInterface*>(target);
         if (!copy) throw FunctionException("Formation Clone has an incompatible native type.\n");
         copy->RebindClonedReferences(*formation,
               [this](GmatBase *object) {return Resolve(object);}, !dormantFormation);
      }
      else if (burn)
      {
         auto *copy = dynamic_cast<Burn*>(target);
         if (!copy) throw FunctionException("Burn Clone has an incompatible native type.\n");
         copy->RebindClonedReferences(*burn, solarSystem,
               [this](GmatBase *object) {return Resolve(object);});
      }
      else if (physical)
      {
         RebindPhysicalReferences(physical, dynamic_cast<PhysicalModel*>(target));
      }
      else
      {
         RegisterEmbedded(source, target);
         RebindReferences(source, target);
      }
      RebindCloneSource(source, target, publicRoot);
      // Complete all bindings before native initialization can query another
      // cloned resource. Dependencies enter this order before their borrowers.
      if (!ephemeris) initializationOrder.push_back(target);
      visiting.erase(source);
      resolutionPath.pop_back();
      if (!publicRoot)
      {
         if (ephemeris) target->SetBorrowedFunctionValueContext(shared_from_this());
         else target->SetFunctionValueContext({});
      }
   }

   void Initialize()
   {
      // Station-origin frames can query body-fixed state during initialization.
      // Prepare geometry before those frames, then initialize station hardware
      // after all frame bindings are ready. This does not associate hardware.
      for (GmatBase *object : initializationOrder)
         if (auto *station = dynamic_cast<GroundstationInterface*>(object))
            if (!station->BodyFixedPoint::Initialize())
               throw FunctionException("Cannot initialize retained station body frames.\n");
      // Match native object initialization: calculated points and coordinate
      // systems are ready before spacecraft/hardware can query their matrices.
      for (Integer phase = 0; phase < 5; ++phase)
      for (GmatBase *object : initializationOrder)
      {
         // Preserve a cold standalone RF value as configuration data. The
         // eventual operational owner must supply its antenna and initialize
         // it, just as for a main-script hardware declaration.
         if (object->IsOfType("RFHardware") &&
               object->GetStringParameter("PrimaryAntenna").empty())
            continue;
         const Integer objectPhase = dynamic_cast<PropSetup*>(object) ? 4 :
               dynamic_cast<CalculatedPoint*>(object) ? 0 :
               dynamic_cast<CoordinateSystem*>(object) ? 1 :
               (dynamic_cast<PhysicalModel*>(object) || dynamic_cast<Burn*>(object)) ? 3 : 2;
         if (phase != objectPhase) continue;
         // Owned engine definitions remain dormant until native preparation;
         // value binding must not open ephemerides or resume integration.
         if (dynamic_cast<Propagator*>(object) || object->IsOfType("ProcessNoise") ||
               object->IsOfType(Gmat::PARAMETER) || object->IsOfType(Gmat::STOP_CONDITION) ||
               dynamic_cast<Function*>(object) || dynamic_cast<Solver*>(object) ||
               dynamic_cast<Subscriber*>(object) || dynamic_cast<EventLocator*>(object) ||
               object->IsOfType("TrackingFileSet")) continue;
         auto *station = dynamic_cast<GroundstationInterface*>(object);
         auto *setup = dynamic_cast<PropSetup*>(object);
         auto *spacecraft = dynamic_cast<Spacecraft*>(object);
         auto *model = dynamic_cast<ODEModel*>(object);
         auto *burn = dynamic_cast<Burn*>(object);
         auto *thruster = dynamic_cast<Thruster*>(object);
         auto *power = dynamic_cast<PowerSystem*>(object);
         auto *attitude = dynamic_cast<Attitude*>(object);
         auto *plate = dynamic_cast<Plate*>(object);
         const bool ready = station ? station->InitializeClonedResource() :
               setup ? setup->InitializeClonedResource() :
               model ? model->InitializeForParameters() :
               spacecraft ? spacecraft->InitializeClonedResource() :
               burn ? burn->InitializeClonedResource() :
               thruster ? thruster->InitializeClonedResource() :
               power ? power->InitializeClonedResource() :
               attitude ? attitude->InitializeClonedResource() :
               plate ? plate->InitializeClonedResource() : object->Initialize();
         if (!ready)
            throw FunctionException("Cannot initialize returned ephemeris dependency '" +
                  object->GetName() + "'.\n");
      }
      // Every private dependency has now been rebound into this graph. Source
      // leases were needed during construction, not for the completed value.
      inheritedEphemerisOwners.clear();
   }

   void RegisterOwned(GmatBase *source, GmatBase *target)
   {
      if (!source && !target) return;
      if (!source || !target || source == target ||
          source->GetTypeName() != target->GetTypeName())
         throw FunctionException("Spacecraft Clone did not copy an owned component.\n");
      const auto previous = copies.find(source);
      if (previous != copies.end() && previous->second != target)
         throw FunctionException("Conflicting owned component copies in returned ephemeris.\n");
      if (!nativeOwned.insert(source).second) return;
      copies[source] = target;
      origins[source] = target;
      RegisterEmbedded(source, target);
   }

   void RegisterSubscriberWrappers(Subscriber &source, Subscriber &target)
   {
      target.PrepareClonedSubscriberValue(source);
      ObjectArray from, to;
      source.GetClonedOwnedObjects(target, from, to);
      for (size_t i = 0; i < from.size(); ++i) RegisterOwned(from[i], to.at(i));
      for (size_t i = 0; i < from.size(); ++i)
      {
         if (SupportsRetainedFunctionRoot(from[i]))
         {
            if (reboundOwned.insert(from[i]).second) BindCopy(from[i], to[i], false);
         }
         else RebindOwned(from[i], to[i]);
      }
   }

   void RegisterValueChildren(GmatBase *source, GmatBase *target)
   {
      const ObjectArray from = source->GetFunctionValueOwnedObjects();
      const ObjectArray to = target->GetFunctionValueOwnedObjects();
      if (from.size() != to.size())
         throw FunctionException("Incompatible native function-value component collection.\n");
      for (size_t i = 0; i < from.size(); ++i) RegisterOwned(from[i], to[i]);
      for (size_t i = 0; i < from.size(); ++i) RebindOwned(from[i], to[i]);
   }

   void RegisterEmbedded(GmatBase *source, GmatBase *target)
   {
      if (auto *subscriber = dynamic_cast<Subscriber*>(source))
      {
         auto *copy = dynamic_cast<Subscriber*>(target);
         if (!copy) throw FunctionException("Incompatible retained subscriber value.\n");
         RegisterSubscriberWrappers(*subscriber, *copy);
         return;
      }
      if (auto *solver = dynamic_cast<Solver*>(source))
      {
         auto *copy = dynamic_cast<Solver*>(target);
         if (!copy) throw FunctionException("Incompatible retained Solver value.\n");
         copy->PrepareClonedSolverValue(*solver);
         RegisterValueChildren(source, target);
         return;
      }
      if (auto *function = dynamic_cast<Function*>(source))
      {
         auto *copy = dynamic_cast<Function*>(target);
         if (!copy) throw FunctionException("Incompatible Function resource copy.\n");
         copy->PrepareClonedFunctionValue(*function);
         ObjectArray from, to;
         function->GetClonedOwnedObjects(*copy, from, to);
         if (from.size() != to.size())
            throw FunctionException("Incompatible owned Function resource collection.\n");
         for (UnsignedInt i = 0; i < from.size(); ++i) RegisterOwned(from[i], to[i]);
         for (UnsignedInt i = 0; i < from.size(); ++i)
         {
            // Native file frames own axes and need the complete frame binder,
            // rather than only the generic embedded-reference pass.
            if (dynamic_cast<CoordinateSystem*>(from[i]))
            {
               if (reboundOwned.insert(from[i]).second) BindCopy(from[i], to[i], false);
            }
            else RebindOwned(from[i], to[i]);
         }
         return;
      }
      if (auto *condition = dynamic_cast<StopCondition*>(source))
      {
         auto *copy = dynamic_cast<StopCondition*>(target);
         if (!copy) throw FunctionException("Incompatible stopping-condition copy.\n");
         copy->PrepareClonedFunctionValue(*condition);
         ObjectArray from, to;
         condition->GetClonedOwnedObjects(*copy, from, to);
         for (UnsignedInt i = 0; i < from.size(); ++i) RegisterOwned(from[i], to.at(i));
         for (UnsignedInt i = 0; i < from.size(); ++i) RebindOwned(from[i], to.at(i));
         return;
      }
      if (auto *frame = dynamic_cast<CoordinateSystem*>(source))
      {
         auto *copy = dynamic_cast<CoordinateSystem*>(target);
         if (!copy) throw FunctionException("Incompatible owned coordinate-system copy.\n");
         RegisterOwned(frame->GetAxisSystem(), copy->GetAxisSystem());
         return;
      }
      if (source->IsOfType("ProcessNoiseModel") || source->IsOfType("EstimatedParameter"))
      {
         const UnsignedInt type = GmatType::GetTypeId(source->IsOfType("ProcessNoiseModel") ?
               "ProcessNoise" : "EstimatedParameterModel");
         RegisterOwned(source->GetRefObject(type, ""), target->GetRefObject(type, ""));
         return;
      }
      RegisterValueChildren(source, target);
      if (!source->HasRefObjectTypeArray()) return;
      const ObjectTypeArray types = source->GetRefObjectTypeArray();
      for (UnsignedInt type : types)
      {
         // These native components clone their configured FOV/error models.
         // Other references (frames, antennas, tanks) remain borrowed bindings.
         if (type != Gmat::FIELD_OF_VIEW && type != Gmat::ERROR_MODEL) continue;
         const StringArray names = source->GetRefObjectNameArray(type);
         for (const std::string &name : names)
         {
            GmatBase *original = source->GetRefObject(type, name);
            GmatBase *copy = target->GetRefObject(type, name);
            if (copy && copy != original) RegisterOwned(original, copy);
         }
      }
   }

   void RebindOwned(GmatBase *source, GmatBase *target, bool bindPropagationObjects = true)
   {
      if (!source || !reboundOwned.insert(source).second) return;
      if (dynamic_cast<PropSetup*>(source) || dynamic_cast<Spacecraft*>(source) ||
          dynamic_cast<GroundstationInterface*>(source) || dynamic_cast<CoordinateSystem*>(source) ||
          dynamic_cast<CalculatedPoint*>(source) || dynamic_cast<FormationInterface*>(source) ||
          dynamic_cast<EphemerisFile*>(source))
      {
         BindCopy(source, target, false, false, !bindPropagationObjects);
         return;
      }
      if (auto *model = dynamic_cast<ODEModel*>(source))
      {
         RebindModelReferences(model, dynamic_cast<ODEModel*>(target), true);
         RebindCloneSource(source, target);
         return;
      }
      if (auto *propagator = dynamic_cast<Propagator*>(source))
      {
         RebindPropagatorReferences(propagator, dynamic_cast<Propagator*>(target), true, bindPropagationObjects);
         RebindCloneSource(source, target);
         return;
      }
      if (!target) throw FunctionException("Missing copied native component.\n");
      IncludeContext(source);
      const auto previous = target->GetFunctionValueContext();
      if (previous && previous.get() != this) inheritedValues.push_back(previous);
      // Their owning EquationWrapper binds all node/map roles after every
      // copied owned identity has been registered by the surrounding owner.
      if (dynamic_cast<MathTree*>(source) || dynamic_cast<MathNode*>(source))
      {
         RebindCloneSource(source, target);
         target->SetFunctionValueContext({});
         return;
      }
      if (auto *force = dynamic_cast<PhysicalModel*>(source))
         RebindPhysicalReferences(force, dynamic_cast<PhysicalModel*>(target));
      else RebindReferences(source, target);
      RebindCloneSource(source, target);
      // The containing native owner retains this scene. An embedded child
      // must not keep its old public-root lease or own that parent through it.
      target->SetFunctionValueContext({});
   }

   void RebindModelReferences(ODEModel *source, ODEModel *target, bool ownedComponent = false)
   {
      if (!target) throw FunctionException("ODEModel Clone has an incompatible native type.\n");
      IncludeContext(source);
      const auto previous = target->GetFunctionValueContext();
      if (previous && previous.get() != this) inheritedValues.push_back(previous);
      target->SetSolarSystem(solarSystem);
      for (CoordinateSystem *internal : source->GetInternalCoordinateSystems())
         if (internal) privateObjects.insert(internal);
      auto resolve = [this](GmatBase *object) {return Resolve(object);};
      auto bindOwned = [this](const ObjectArray &from, const ObjectArray &to)
      {
         for (UnsignedInt i = 0; i < from.size(); ++i) RegisterOwned(from[i], to.at(i));
         for (UnsignedInt i = 0; i < from.size(); ++i) RebindOwned(from[i], to.at(i));
      };
      if (reportingContext) target->RebindClonedReportingReferences(*source, resolve, bindOwned);
      else target->RebindClonedReferences(*source, resolve, bindOwned);
      if (ownedComponent) target->SetFunctionValueContext({});
   }

   void RebindPropagatorReferences(Propagator *source, Propagator *target, bool ownedComponent, bool bindObjects = true)
   {
      if (!target) throw FunctionException("Propagator Clone has an incompatible native type.\n");
      IncludeContext(source);
      const auto previous = target->GetFunctionValueContext();
      if (previous && previous.get() != this) inheritedValues.push_back(previous);
      target->RebindClonedReferences(*source, solarSystem,
            [this](GmatBase *object) {return Resolve(object);}, bindObjects);
      if (ownedComponent) target->SetFunctionValueContext({});
   }

   void RebindPhysicalReferences(PhysicalModel *source, PhysicalModel *target)
   {
      if (!target) throw FunctionException("Incompatible native force copy.\n");
      target->SetSolarSystem(solarSystem);
      target->RebindClonedBodyReferences(*source,
            [this](GmatBase *object) {return Resolve(object);});
      if (auto *drag = dynamic_cast<DragForce*>(source))
      {
         auto *value = dynamic_cast<DragForce*>(target);
         if (!value) throw FunctionException("Incompatible native drag force copy.\n");
         value->RebindClonedCoordinateSystems(*drag,
               [this](GmatBase *object) {return Resolve(object);});
      }
      if (auto *thrust = dynamic_cast<FiniteThrust*>(source))
      {
         auto *value = dynamic_cast<FiniteThrust*>(target);
         if (!value) throw FunctionException("Incompatible retained finite-thrust value.\n");
         value->RebindClonedReferences(*thrust,
               [this](GmatBase *object) {return Resolve(object);});
      }
      // UNKNOWN_OBJECT exposes harmonic frame roles without the mutating
      // typed name accessor that recomputes its FixedCoordinateSystem name.
      StringArray names = source->GetRefObjectNameArray(Gmat::UNKNOWN_OBJECT);
      if (source->IsOfType("DragForce")) names.push_back(source->GetBodyName() + "Fixed");
      for (const std::string &name : names)
      {
         if (name.empty()) continue;
         for (UnsignedInt type : {Gmat::COORDINATE_SYSTEM, Gmat::SPACE_POINT, Gmat::CELESTIAL_BODY})
         {
            GmatBase *reference = NULL;
            try {reference = source->GetRefObject(type, name);}
            catch (BaseException &) {continue;}
            if (!reference || !reference->IsOfType(type)) continue;
            GmatBase *value = Resolve(reference);
            if (!target->SetRefObject(value, type, name))
               throw FunctionException("Cannot bind copied force reference '" + name + "'.\n");
            // Force accessors use declared role labels (e.g. EarthFixed),
            // which need not be the bound caller frame's own resource name.
            break;
         }
      }
   }

   void RebindReferences(GmatBase *source, GmatBase *target)
   {
      if (target->RebindFunctionValueReferences(*source, solarSystem,
            [this](GmatBase *object) {return Resolve(object);}))
      {
         // Ephemeris reporting observes the caller's live force table. Ordinary
         // function values keep the provider's independently mapped snapshot.
         auto *parameter = dynamic_cast<Parameter*>(target);
         if (reportingContext && parameter && parameter->NeedsForces())
            parameter->SetTransientForces(runtimeForces);
         return;
      }
      // Unlike a value-only fallback, a callable definition must explicitly
      // disclose its reference policy. Unimplemented providers stay open.
      if (dynamic_cast<Function*>(source) || dynamic_cast<Solver*>(source) ||
            dynamic_cast<Subscriber*>(source) || dynamic_cast<EventLocator*>(source))
         throw FunctionException("Retained reference binding is unavailable for callable, Solver, Subscriber or EventLocator type '" +
               source->GetTypeName() + "'.\n");
      if (auto *attitude = dynamic_cast<Attitude*>(source))
      {
         auto *value = dynamic_cast<Attitude*>(target);
         if (!value) throw FunctionException("Incompatible retained Attitude.\n");
         value->RebindClonedReferences(*attitude,
               [this](GmatBase *object) {return Resolve(object);});
         return;
      }
      if (auto *plate = dynamic_cast<Plate*>(source))
      {
         auto *value = dynamic_cast<Plate*>(target);
         if (!value) throw FunctionException("Incompatible retained Plate.\n");
         value->RebindClonedReferences(*plate,
               [this](GmatBase *object) {return Resolve(object);},
               [this, plate](const std::string &name)
               {
                  CoordinateSystem *frame = NULL;
                  for (GmatBase *object : privateObjects)
                     if (object->GetName() == name && object->IsOfType(Gmat::COORDINATE_SYSTEM))
                     {
                        if (frame && frame != object) throw FunctionException("Ambiguous private Plate history frame.\n");
                        frame = dynamic_cast<CoordinateSystem*>(object);
                     }
                  if (!frame) frame = dynamic_cast<CoordinateSystem*>(plate->GetConfiguredObject(name));
                  return dynamic_cast<CoordinateSystem*>(Resolve(frame));
               });
         return;
      }
      if (auto *thruster = dynamic_cast<Thruster*>(source))
      {
         auto *value = dynamic_cast<Thruster*>(target);
         if (!value) throw FunctionException("Incompatible retained Thruster.\n");
         value->RebindClonedReferences(*thruster, solarSystem,
               [this](GmatBase *object) {return Resolve(object);});
         return;
      }
      if (auto *power = dynamic_cast<PowerSystem*>(source))
      {
         auto *value = dynamic_cast<PowerSystem*>(target);
         if (!value) throw FunctionException("Incompatible retained PowerSystem.\n");
         value->RebindClonedReferences(*power, solarSystem,
               [this](GmatBase *object) {return Resolve(object);});
         return;
      }
      if (source->IsOfType("ProcessNoiseModel") || source->IsOfType("EstimatedParameter"))
      {
         const UnsignedInt type = GmatType::GetTypeId(source->IsOfType("ProcessNoiseModel") ?
               "ProcessNoise" : "EstimatedParameterModel");
         RebindOwned(source->GetRefObject(type, ""), target->GetRefObject(type, ""));
         return;
      }
      if (!source->HasRefObjectTypeArray()) return;
      const ObjectTypeArray types = source->GetRefObjectTypeArray();
      for (UnsignedInt type : types)
      {
         // A standalone RF value can be declared/returned before it is attached
         // to an antenna. Its required-reference accessor throws for that cold
         // state; there is no antenna edge to copy yet. Operational hardware
         // initialization still enforces the required antenna association.
         if (type == Gmat::HARDWARE && source->IsOfType("RFHardware") &&
               source->GetStringParameter("PrimaryAntenna").empty())
            continue;
         const StringArray names = source->GetRefObjectNameArray(type);
         for (const std::string &name : names)
         {
            if (name.empty()) continue;
            GmatBase *original = source->GetRefObject(type, name);
            if (!original) continue; // an unused optional reference
            GmatBase *copy = Resolve(original);
            GmatBase *current = target->GetRefObject(type, name);
            // An owned FOV/error/model is already cloned. Passing it through a
            // setter that clones again would discard the registered identity.
            if (current != copy && !target->SetRefObject(copy, type, name))
               throw FunctionException("Cannot rebind returned spacecraft component reference '" + name + "'.\n");
            if (original->GetName() != copy->GetName() &&
                !target->RenameRefObject(type, name, copy->GetName()))
               throw FunctionException("Cannot rename returned spacecraft component reference '" + name + "'.\n");
            if (nativeOwned.find(original) != nativeOwned.end()) RebindOwned(original, copy);
         }
      }
   }

   void Bind(GmatBase *target, UnsignedInt type, const std::string &name,
         GmatBase *source)
   {
      if (!source)
         throw FunctionException("Uninitialized reference '" + name + "' in returned ephemeris frame.\n");
      GmatBase *value = Resolve(source);
      // Bind using the existing field name first. CalculatedPoint also uses
      // that name to replace its existing borrowed body-list entry.
      if (!target->SetRefObject(value, type, name))
         throw FunctionException("Cannot bind returned ephemeris frame reference '" + name + "'.\n");
      if (value->GetName() != name &&
          !target->RenameRefObject(type, name, value->GetName()))
         throw FunctionException("Cannot rename returned ephemeris frame reference '" + name + "'.\n");
   }
};

std::exception_ptr FinalizeFunctionSubscriber(Subscriber *subscriber,Publisher *publisher)
{
   std::exception_ptr failure;
   try
   {
      subscriber->ConfigureRuntimeOutput(true);
      if (!subscriber->TakeAction("Finalize") && subscriber->IsOfType("EphemerisFile"))
         throw FunctionException("Cannot finalize local ephemeris '"+subscriber->GetName()+"'");
   }
   catch (BaseException &error)
   {
      // An output failure must fail the call even when the historical plot
      // finalization policy permits a nonfatal diagnostic and continuation.
      if (error.IsFatal() || subscriber->IsOfType("EphemerisFile"))
         failure=std::current_exception();
      else
      {
         try {MessageInterface::ShowMessage("%s\n",error.GetFullMessage().c_str());}
         catch (...) {failure=std::current_exception();}
      }
   }
   catch (...) {failure=std::current_exception();}
   try {if (publisher) publisher->Unsubscribe(subscriber);}
   catch (...) {if (!failure) failure=std::current_exception();}
   return failure;
}
}

//#define DO_NOT_EXECUTE_NESTED_GMAT_FUNCTIONS

// Deleting subscribers seems to work as of 2015.07.27
//#define __DO_NOT_DELETE_SUBSCRIBERS__

//#define DEBUG_FUNCTION_MANAGER
//#define DEBUG_FM_SET
//#define DEBUG_FM_INIT
//#define DEBUG_FM_EXECUTE
//#define DEBUG_FM_REFRESH
//#define DEBUG_FM_EVAL
//#define DEBUG_FM_RESULT
//#define DEBUG_FM_FINALIZE
//#define DEBUG_FM_STACK
//#define DEBUG_OBJECT_MAP
//#define DEBUG_INPUT
//#define DEBUG_WRAPPERS
//#define DEBUG_SUBSCRIBER
//#define DEBUG_CLEANUP
//#define DEBUG_FIND_OBJ
//#define DEBUG_CREATE_OBJ

//#ifndef DEBUG_MEMORY
//#define DEBUG_MEMORY
//#endif
//#ifndef DEBUG_TRACE
//#define DEBUG_TRACE
//#endif

#ifdef DEBUG_MEMORY
#include "MemoryTracker.hpp"
#endif
#ifdef DEBUG_TRACE
#include <ctime>                 // for clock()
#endif

//---------------------------------
// static data
//---------------------------------

//---------------------------------
// public methods
//---------------------------------

//------------------------------------------------------------------------------
// FunctionManager()
//------------------------------------------------------------------------------
/**
 * Constructor
 */
//------------------------------------------------------------------------------
void FunctionManager::RebindSubscriberCopy(const Subscriber &source, Subscriber &target,
      SolarSystem *system)
{
   if (&source == &target || typeid(source) != typeid(target) ||
         !source.HasSubscriberValueReferenceProvider() || !target.HasSubscriberValueReferenceProvider())
      throw FunctionException("Incompatible completed subscriber copy.\n");
   auto context = std::make_shared<ReturnedEphemerisFrames>();
   context->solarSystem = system;
   context->runtimeForces = nullptr;
   context->privateObjects.insert(const_cast<Subscriber*>(&source));
   const auto children = source.GetFunctionValueOwnedObjects();
   context->privateObjects.insert(children.begin(), children.end());
   target.SetFunctionValueContext(context);
   context->BindCopy(const_cast<Subscriber*>(&source), &target, true);
   context->Initialize();
   context->inheritedValues.clear();
}

std::function<bool()> ObjectManagedFunction::CaptureObjectStoreLifetime(ObjectMap *store)
{
   auto nativeOwner = ObjectStoreLifetime::Capture(store);
   if (nativeOwner) return nativeOwner;
   const auto registration = FindBuiltinOutputStore(store);
   if (!registration.first) return {};
   std::weak_ptr<FunctionBuiltinOutputResources> owner(registration.first);
   const auto identity = registration.second;
   return [store, owner, identity]()
   {
      const auto expectedOwner = owner.lock();
      const auto expectedIdentity = identity.lock();
      if (!expectedOwner || !expectedIdentity) return false;
      const auto current = FindBuiltinOutputStore(store);
      return current.first == expectedOwner && current.second.lock() == expectedIdentity;
   };
}

FunctionManager& FunctionManager::ReentrantInvocation()
{
   if (!reentrantInvocation)
   {
      auto frame = std::move(completedInvocation);
      if (frame)
      {
         frame->RetireCompletedInvocation();
         frame->PrepareCallSetup();
         frame->SetExpressionEnvironment(HasLiveFunction() ? currentFunction : nullptr,
               localObjectStore, globalObjectStore);
         frame->expressionBindings = expressionBindings;
         frame->publisher = publisher;
         frame->solarSys = solarSys;
         frame->internalCS = internalCS;
         frame->forces = forces;
         frame->functionName = functionName;
         frame->SetInputs(passedIns);
         frame->SetOutputs(passedOuts);
         frame->deferredFunctionName = deferredFunctionName;
      }
      else frame = std::make_unique<FunctionManager>(*this);
      frame->localObjectStore = localObjectStore;
      reentrantInvocation = std::move(frame);
   }
   reentrantPrepared = true;
   return *reentrantInvocation;
}

void FunctionManager::PrepareCallSetup()
{
   if (!executing && finalizationRequested) Finalize();
}

void FunctionManager::RetireCompletedInvocation()
{
   if (executing) return;
   struct Result
   {
      FunctionManager &manager;
      Real real;
      Rmatrix matrix;
      GmatBase *view;
      std::unique_ptr<GmatBase> object;
      std::string type;
      bool blank;
      bool completed = false;
      explicit Result(FunctionManager &value) : manager(value), real(value.realResult),
         view(value.objectResult), object(std::move(value.ownedObjectResult)), blank(value.blankResult)
      {
         matrix.Swap(manager.matResult);
         type.swap(manager.outputType);
         manager.objectResult = nullptr;
      }
      ~Result()
      {
         manager.realResult = real;
         matrix.Swap(manager.matResult);
         manager.ownedObjectResult = std::move(object);
         manager.blankResult = blank;
         if (completed)
         {
            manager.objectResult = view;
            type.swap(manager.outputType);
         }
      }
   } result(*this);
   // Keep completed value storage alive for a following aliased input, while
   // freeing prior stores/initializers/wrappers before a recursive frame reuses
   // its manager. Completed store histories do not grow with call count.
   Finalize();
   result.completed = true;
}

bool FunctionManager::HasInstalledInvocation() const
{
   if (!HasLiveFunction() || (currentFunction->HasActiveInvocation() && !executing)) return false;
   const auto *store = currentFunction->GetFunctionObjectStore();
   return store && (store == functionObjectStore ||
         std::find(clonedObjectStores.begin(), clonedObjectStores.end(), store) != clonedObjectStores.end());
}

bool FunctionManager::IsActiveCallerStore(const ObjectMap *store) const
{
   if (!store) return false;
   std::set<const FunctionManager*> visited;
   for (const FunctionManager *caller = callingFunction; caller; caller = caller->callingFunction)
   {
      if (!visited.insert(caller).second)
         throw FunctionException("Cyclic function manager caller chain.\n");
      if (caller->executing && caller->functionObjectStore == store) return true;
   }
   return false;
}

void FunctionManager::BindFunctionServices()
{
   if (!HasLiveFunction())
      throw FunctionException("Cannot bind services to a retired Function.\n");
   currentFunction->SetGlobalObjectMap(globalObjectStore);
   currentFunction->SetSolarSystem(solarSys);
   currentFunction->SetInternalCoordSystem(internalCS);
   currentFunction->SetTransientForces(forces);
   if (auto *function = dynamic_cast<UserDefinedFunction*>(currentFunction))
   {
      fcs = function->GetFunctionControlSequence();
      for (auto *command : GmatCommandUtil::GetCommandSequenceNodes(fcs))
      {
         command->SetPublisher(publisher);
         command->SetCallingFunction(this);
         command->SetInternalCoordSystem(internalCS);
      }
   }
}

void FunctionManager::ReleaseStagedWrappers() noexcept
{
   for (auto *wrapper : stagedWrapperDeletions)
   {
      if (!wrapper) continue;
      for (auto &entry : stagedWrapperDeletions) if (entry == wrapper) entry = nullptr;
      bool borrowed = false;
      for (const auto &entry : inputWrapperMap) if (entry.second == wrapper) borrowed = true;
      for (auto *entry : outputWrappers) if (entry == wrapper) borrowed = true;
      if (!borrowed) ElementWrapper::ReleaseOwnedWrapper(wrapper);
   }
   stagedWrapperDeletions.clear();
}

FunctionManager::FunctionManager() :
   publisher           (NULL),
   functionObjectStore (NULL),
   localObjectStore    (NULL),
   globalObjectStore   (NULL),
   solarSys            (NULL),
   forces              (NULL),
   functionName        (""),
   currentFunction     (NULL),
   firstExecution      (true),
   isFinalized         (false),
   reinitializeFunc    (false),
   isObjStoreOwner     (false),
   numVarsCreated      (0),
   validator           (NULL),
   realResult          (-999.99),
   objectResult        (NULL),
   ownedObjectResult   (),
   blankResult         (false),
   outputType          (""),
   objInit             (NULL),
   internalCS          (NULL), 
   fcs                 (NULL),
   callingFunction     (NULL)
{
   #ifdef DEBUG_FUNCTION_MANAGER
   MessageInterface::ShowMessage("FM default constructor entered, this=<%p>\n", this);
   #endif
}


//------------------------------------------------------------------------------
// ~FunctionManager()
//------------------------------------------------------------------------------
/**
 * Destructor
 */
//------------------------------------------------------------------------------
FunctionManager::~FunctionManager()
{
   const auto retiringBindings = expressionBindings;
   const bool previouslyFinalized = isFinalized;
   isFinalized = true;
   finalizationRequested = false;
   auto pending = std::move(reentrantInvocation);
   auto completed = std::move(completedInvocation);
   executing = true;
   pending.reset();
   completed.reset();
   executing = false;
   finalizationRequested = false;
   isFinalized = previouslyFinalized;
   if (!isFinalized && (functionObjectStore || !clonedObjectStores.empty() ||
         !createdLiterals.empty() || !createdOthers.empty() ||
         !inputWrapperMap.empty() || !outputWrappers.empty() || builtinOutputResources))
   {
      try { Finalize(); }
      catch (...)
      {
         // Cleanup retains the first callback error while retiring stores.
         // A destructor cannot propagate it or discard the original exception.
         try
         {
            try { throw; }
            catch (BaseException &error)
            {
               MessageInterface::ShowMessage("Function cleanup during destruction: %s\n",
                     error.GetFullMessage().c_str());
            }
            catch (const std::exception &error)
            {
               MessageInterface::ShowMessage("Function cleanup during destruction: %s\n", error.what());
            }
            catch (...)
            {
               MessageInterface::ShowMessage("Unknown function cleanup error during destruction.\n");
            }
         }
         catch (...) {} // Diagnostics must not replace an exception already unwinding.
      }
   }

   // Destroy a cached local value before its borrowed scope dependencies.
   objectResult = NULL;
   ReleaseStagedWrappers();
   ownedObjectResult.reset();
   pendingInputValues.clear();
   while (!savedLocalObjectStates.empty()) savedLocalObjectStates.pop();
   #ifdef DEBUG_FUNCTION_MANAGER
   MessageInterface::ShowMessage("FM destructor entered, this=<%p>\n", this);
   #endif
   
   if (objInit)
   {
      #ifdef DEBUG_MEMORY
      MemoryTracker::Instance()->Remove
         (objInit, "objInit", "FunctionManager::~FunctionManager()");
      #endif
      delete objInit;
      objInit = NULL;
   }
   
   if (isObjStoreOwner)
   {
      DeleteObjectMap(functionObjectStore, "FOS in Destructor");
   }
   functionObjectStore = NULL;
   ClearInOutWrappers();
}


//------------------------------------------------------------------------------
// FunctionManager(const FunctionManager &fm)
//------------------------------------------------------------------------------
/**
 * Copy constructor.
 * 
 * @param <fm> The instance that gets copied.
 */
//------------------------------------------------------------------------------
FunctionManager::FunctionManager(const FunctionManager &fm) :
   publisher           (fm.publisher),
   functionObjectStore (NULL),
   localObjectStore    (NULL),
   globalObjectStore   (fm.globalObjectStore),
   solarSys            (fm.solarSys),
   forces              (fm.forces),
   functionName        (fm.functionName),
   currentFunction     (fm.HasLiveFunction() ? fm.currentFunction : nullptr),
   currentFunctionIdentity(fm.currentFunctionIdentity),
   passedIns           (fm.passedIns),
   passedOuts          (fm.passedOuts),
   //inputWrapperMap       (fm.inputWrapperMap),
   firstExecution      (true),
   isFinalized         (false),
   reinitializeFunc    (false),
   isObjStoreOwner     (false),
   numVarsCreated      (0),
   validator           (NULL),
   realResult          (-999.99),
   objectResult        (NULL),
   ownedObjectResult   (),
   blankResult         (false),
   outputType          (""),
   objInit             (NULL),
   internalCS          (fm.internalCS), 
   fcs                 (NULL),
   callingFunction     (NULL)
{
   expressionBindings = fm.expressionBindings;
   deferredFunctionName = fm.deferredFunctionName;
   #ifdef DEBUG_FUNCTION_MANAGER
   MessageInterface::ShowMessage("FM copy constructor entered, this=<%p>\n", this);
   #endif
}


//------------------------------------------------------------------------------
// FunctionManager& operator=(const FunctionManager &fm)
//------------------------------------------------------------------------------
/**
 * Assignment operator
 * 
 * @param <fm> The command that gets copied.
 * 
 * @return A reference to this instance.
 */
//------------------------------------------------------------------------------
FunctionManager& FunctionManager::operator=(const FunctionManager &fm)
{
   if (&fm != this)
   {
      if (executing || !callers.empty())
         throw FunctionException("Cannot assign to an executing FunctionManager\n");

      // Wrappers and cached results belong to one invocation. The copy will
      // rebuild its arguments during Initialize(), just as a new manager does.
      if (reentrantInvocation || completedInvocation || functionObjectStore || !createdLiterals.empty() || !createdOthers.empty() || builtinOutputResources)
         Finalize();
      ReleaseStagedWrappers();
      ClearInOutWrappers();
      #ifdef DEBUG_MEMORY
      if (objInit)
         MemoryTracker::Instance()->Remove
            (objInit, "objInit", "FunctionManager::operator=()");
      #endif
      delete objInit;
      objInit = NULL;

      expressionBindings = fm.expressionBindings;
      publisher           = fm.publisher;
      functionObjectStore = NULL;
      functionStoreOwner.reset();
      functionStoreIdentity.reset();
      functionStoreIsManaged = false;
      localObjectStore    = NULL;
      combinedObjectStore.clear();
      globalObjectStore   = fm.globalObjectStore; // is that right?
      solarSys            = fm.solarSys;
      forces              = fm.forces;
      functionName        = fm.functionName;
      currentFunction     = fm.HasLiveFunction() ? fm.currentFunction : nullptr;
      currentFunctionIdentity = fm.currentFunctionIdentity;
      passedIns           = fm.passedIns;
      passedOuts          = fm.passedOuts;
      firstExecution      = true;
      isFinalized         = false;
      reinitializeFunc    = false;
      isObjStoreOwner     = false;
      numVarsCreated      = 0;
      validator           = NULL;
      realResult          = -999.99;
      objectResult        = NULL;
      ownedObjectResult.reset();
      pendingInputValues.clear();
      blankResult         = false;
      outputType.clear();
      objInit             = NULL;  
      internalCS          = fm.internalCS;  // right?
      fcs                 = NULL;
      callingFunction     = NULL;
      finalizationRequested = false;
      contextRetirementRequested = false;
      reentrantPrepared = false;
      deferredFunctionName = fm.deferredFunctionName;
      while (!savedLocalObjectStates.empty())
         savedLocalObjectStates.pop();
   }
   return *this;
}


//------------------------------------------------------------------------------
// virtual void SetPublisher(Publisher *pub)
//------------------------------------------------------------------------------
void FunctionManager::SetPublisher(Publisher *pub)
{
   if (executing) { ReentrantInvocation().SetPublisher(pub); return; }
   PrepareCallSetup();
   #ifdef DEBUG_PUBLISHER
   MessageInterface::ShowMessage
      ("FunctionManager::SetPublisher() '%s' setting publisher <%p>\n",
       functionName.c_str(), pub);
   #endif
   publisher = pub;
}


//------------------------------------------------------------------------------
// virtual Publisher* GetPublisher()
//------------------------------------------------------------------------------
Publisher* FunctionManager::GetPublisher()
{
   if (executing && reentrantPrepared && reentrantInvocation) return reentrantInvocation->GetPublisher();
   return publisher;
}


//------------------------------------------------------------------------------
// void SetObjectMap(std::map<std::string, GmatBase *> *map)
//------------------------------------------------------------------------------
void FunctionManager::SetObjectMap(std::map<std::string, GmatBase *> *map)
{
   if (executing) { ReentrantInvocation().SetObjectMap(map); return; }
   PrepareCallSetup();
   localObjectStore = map;
   #ifdef DEBUG_OBJECT_MAP
   MessageInterface::ShowMessage
      ("FunctionManager::SetObjectMap() localObjectStore=<%p>\n", localObjectStore);
   ShowObjectMap(localObjectStore, "SetObjectMap()");
   #endif
}

//------------------------------------------------------------------------------
// void SetGlobalObjectMap(std::map<std::string, GmatBase *> *map)
//------------------------------------------------------------------------------
void FunctionManager::SetGlobalObjectMap(std::map<std::string, GmatBase *> *map)
{
   if (executing) { ReentrantInvocation().SetGlobalObjectMap(map); return; }
   PrepareCallSetup();
   globalObjectStore = map;
}

//------------------------------------------------------------------------------
// void SetSolarSystem(SolarSystem *ss)
//------------------------------------------------------------------------------
void FunctionManager::SetSolarSystem(SolarSystem *ss)
{
   if (executing) { ReentrantInvocation().SetSolarSystem(ss); return; }
   PrepareCallSetup();
   #ifdef DEBUG_FM_SET
   MessageInterface::ShowMessage("FunctionManager::SetSolarSystem() ss=<%p>\n", ss);
   #endif
   solarSys = ss;
   if (HasLiveFunction() && !currentFunction->HasActiveInvocation() && currentFunction->GetTypeName() == "GmatFunction")
      currentFunction->SetSolarSystem(ss);
}

//------------------------------------------------------------------------------
// void SetTransientForces(std::vector<PhysicalModel*> *tf)
//------------------------------------------------------------------------------
void FunctionManager::SetTransientForces(std::vector<PhysicalModel*> *tf)
{
   if (executing) { ReentrantInvocation().SetTransientForces(tf); return; }
   PrepareCallSetup();
   forces = tf;
   if (HasLiveFunction() && !currentFunction->HasActiveInvocation() && currentFunction->GetTypeName() == "GmatFunction")
      currentFunction->SetTransientForces(tf);
}

//------------------------------------------------------------------------------
// void SetFunctionName(const std::string &itsName)
//------------------------------------------------------------------------------
void FunctionManager::SetFunctionName(const std::string &itsName)
{
   if (executing) { ReentrantInvocation().SetFunctionName(itsName); return; }
   PrepareCallSetup();
   #ifdef DEBUG_FM_SET
      MessageInterface::ShowMessage("Entering FM::SetFunctionName with name = %s\n",
                                    itsName.c_str());
   #endif
   functionName = itsName;
   if (HasLiveFunction() && currentFunction->GetTypeName() == "GmatFunction")
   {
      #ifdef DEBUG_FM_SET
         MessageInterface::ShowMessage("   and setting name to %s on the function\n",
                                       itsName.c_str());
      #endif
      if (currentFunction->HasActiveInvocation()) deferredFunctionName = true;
      else currentFunction->SetStringParameter("FunctionName", itsName);
   }
}

//------------------------------------------------------------------------------
// std::string GetFunctionName() const
//------------------------------------------------------------------------------
std::string FunctionManager::GetFunctionName() const
{
   if (executing && reentrantPrepared && reentrantInvocation) return reentrantInvocation->GetFunctionName();
   // The call-site name remains useful after an external callee retires.
   #ifdef DEBUG_FM_GET
   MessageInterface::ShowMessage("FunctionManager call-site name = '%s'\n", functionName.c_str());
   #endif
   return functionName;
}

//------------------------------------------------------------------------------
// void SetFunction(Function *theFunction)
//------------------------------------------------------------------------------
void FunctionManager::SetFunction(Function *theFunction)
{
   if (executing) { ReentrantInvocation().SetFunction(theFunction); return; }
   PrepareCallSetup();
   if (!theFunction) return; // Legacy null input leaves the binding unchanged.
   auto *function = dynamic_cast<ObjectManagedFunction*>(theFunction);
   if (!function)
   {
      if (theFunction->IsOfType("ObjectManagedFunction"))
         throw FunctionException("Function manager requires an ObjectManagedFunction.\n");
      return;
   }
   SetExpressionEnvironment(function, localObjectStore, globalObjectStore);
   // Keep this legacy API's explicit naming behavior. Expression/native call
   // sites use SetExpressionEnvironment directly and do not rename the callee.
   if (currentFunction->HasActiveInvocation()) deferredFunctionName = true;
   else currentFunction->SetStringParameter("FunctionName", functionName);
}

//------------------------------------------------------------------------------
// Function* GetFunction() const
//------------------------------------------------------------------------------
bool FunctionManager::HasLiveFunction() const
{
   return GmatBase::IsLiveReference(currentFunction, currentFunctionIdentity);
}

Function* FunctionManager::GetFunction() const
{
   if (executing && reentrantPrepared && reentrantInvocation) return reentrantInvocation->GetFunction();
   return HasLiveFunction() ? currentFunction : nullptr;
}

//------------------------------------------------------------------------------
// void AddInput(const std::string &withName, Integer atIndex)
//------------------------------------------------------------------------------
void FunctionManager::AddInput(const std::string &withName, Integer atIndex)
{
   if (executing) { ReentrantInvocation().AddInput(withName, atIndex); return; }
   PrepareCallSetup();
   #ifdef DEBUG_INPUT
   MessageInterface::ShowMessage
      ("FunctionManager::AddInput() entered, name='%s', index=%d\n", withName.c_str(),
       atIndex);
   #endif
   
   // -999 means to put it at the end of the list (default value/behavior)
   if ((atIndex == -999) || (atIndex == (Integer) passedIns.size()))
   {
      passedIns.push_back(withName);
      return;
   }
   
   if ((atIndex < 0) || (atIndex > (Integer) passedIns.size()))
      throw FunctionException("FunctionManager:: input index out of range - unable to set.\n");
   
   // replace an entry
   if (passedIns.at(atIndex) != withName) pendingInputValues.erase(atIndex);
   passedIns.at(atIndex) = withName;
   
   #ifdef DEBUG_INPUT
   for (UnsignedInt i = 0; i < passedIns.size(); i++)
      MessageInterface::ShowMessage("   passedIns[%d] = '%s'\n", i, passedIns[i].c_str());
   MessageInterface::ShowMessage
      ("FunctionManager::AddInput() entered, name='%s', index=%d\n", withName.c_str(),
       atIndex);
   #endif
}

//------------------------------------------------------------------------------
// void AddOutput(const std::string &withName, Integer atIndex)
//------------------------------------------------------------------------------
void FunctionManager::AddOutput(const std::string &withName, Integer atIndex)
{
   if (executing) { ReentrantInvocation().AddOutput(withName, atIndex); return; }
   PrepareCallSetup();
   // -999 means to put it at the end of the list (default value/behavior)
   if ((atIndex == -999) || (atIndex == (Integer) passedOuts.size()))
   {
      passedOuts.push_back(withName);
      return;
   }
   
   if ((atIndex < 0) || (atIndex > (Integer) passedOuts.size()))
      throw FunctionException("FunctionManager:: input index out of range - unable to set.\n");
   // replace an entry
   passedOuts.at(atIndex) = withName;   
}

//------------------------------------------------------------------------------
// void SetInputs(const StringArray &inputs)
//------------------------------------------------------------------------------
void FunctionManager::SetInputs(const StringArray &inputs)
{
   if (executing) { ReentrantInvocation().SetInputs(inputs); return; }
   PrepareCallSetup();
   if (passedIns != inputs) pendingInputValues.clear();
   passedIns = inputs;
}

//------------------------------------------------------------------------------
// void SetOutputs(const StringArray &outputs)
//------------------------------------------------------------------------------
void FunctionManager::SetOutputs(const StringArray &outputs)
{
   if (executing) { ReentrantInvocation().SetOutputs(outputs); return; }
   PrepareCallSetup();
   passedOuts = outputs;
}

//------------------------------------------------------------------------------
// StringArray GetOutputs()
//------------------------------------------------------------------------------
StringArray  FunctionManager::GetOutputs()
{
   if (executing && reentrantPrepared && reentrantInvocation) return reentrantInvocation->GetOutputs();
   return passedOuts;
}

//------------------------------------------------------------------------------
// void SetInternalCoordinateSystem(CoordinateSystem *intCS)
//------------------------------------------------------------------------------
void FunctionManager::SetInternalCoordinateSystem(CoordinateSystem *intCS)
{
   if (executing) { ReentrantInvocation().SetInternalCoordinateSystem(intCS); return; }
   PrepareCallSetup();
   #ifdef DEBUG_FM_SET
   MessageInterface::ShowMessage
      ("in FM:SetInternalCoordinateSystem(), intCS=<%p>\n", intCS);
   #endif
   
   internalCS = intCS;
   //if (HasLiveFunction() && currentFunction->GetTypeName() == "GmatFunction")
   if (HasLiveFunction() && !currentFunction->HasActiveInvocation())
      if (currentFunction->GetTypeName() == "GmatFunction") 
         currentFunction->SetInternalCoordSystem(internalCS);
}


//------------------------------------------------------------------------------
// virtual bool SetInputWrapper(Integer index, ElementWrapper *ew)
//------------------------------------------------------------------------------
bool FunctionManager::SetInputWrapper(Integer index, ElementWrapper *ew)
{
   if (executing) return ReentrantInvocation().SetInputWrapper(index, ew);
   PrepareCallSetup();
   #ifdef DEBUG_INPUT
   MessageInterface::ShowMessage
      ("FunctionManager::SetInputWrapper() entered, index=%d, ew=<%p>\n", index, ew);
   #endif
   
   if (ew == NULL || !HasLiveFunction() || index < 0 ||
       index >= (Integer)passedIns.size())
      return false;

   const StringArray &formalInputs = currentFunction->GetStringArrayParameter(
      currentFunction->GetParameterID("Input"));
   if (index >= (Integer)formalInputs.size())
      return false;
   std::string formalInput = formalInputs[index];
   
   #ifdef DEBUG_INPUT
   Integer numWrappers = inputWrapperMap.size();
   MessageInterface::ShowMessage
      ("   numWrappers=%d, formalInput='%s'\n", numWrappers, formalInput.c_str());
   #endif
   
   #ifdef DEBUG_INPUT
   MessageInterface::ShowMessage
      ("FunctionManager::SetInputWrapper() passed input string=<%s>\n",
       passedIns[index].c_str());
   #endif
   
   ElementWrapper *oldEw = inputWrapperMap[formalInput];
   inputWrapperMap[formalInput] = ew;
   if (oldEw && oldEw != ew)
   {
      bool retained = false;
      for (const auto &entry : inputWrapperMap) if (entry.second == oldEw) retained = true;
      for (auto *entry : outputWrappers) if (entry == oldEw) retained = true;
      if (!retained) ElementWrapper::ReleaseOwnedWrapper(oldEw);
      oldEw = NULL;
   }
   
   #ifdef DEBUG_INPUT
   MessageInterface::ShowMessage
      ("FunctionManager::SetInputWrapper() returning true\n");
   #endif
   return true;
}


//------------------------------------------------------------------------------
// virtual ElementWrapper* GetInputWrapper(Integer index)
//------------------------------------------------------------------------------
ElementWrapper* FunctionManager::GetInputWrapper(Integer index)
{
   if (executing && reentrantPrepared && reentrantInvocation) return reentrantInvocation->GetInputWrapper(index);
   if (!HasLiveFunction() || index < 0 ||
       index >= (Integer)passedIns.size())
      return NULL;
   const StringArray &formalInputs = currentFunction->GetStringArrayParameter(
      currentFunction->GetParameterID("Input"));
   if (index >= (Integer)formalInputs.size())
      return NULL;
   std::string formalInput = formalInputs[index];

   #ifdef DEBUG_INPUT
   Integer numWrappers = inputWrapperMap.size();
   MessageInterface::ShowMessage
      ("FunctionManager::GetInputWrapper() entered, index=%d, numWrappers=%d, "
       "formalInput='%s'\n", index, numWrappers, formalInput.c_str());
   #endif
   
   #ifdef DEBUG_INPUT
   MessageInterface::ShowMessage
      ("FunctionManager::GetInputWrapper() passed input string=<%s>\n",
       passedIns[index].c_str());
   #endif
   
   if (inputWrapperMap.find(formalInput) == inputWrapperMap.end())
   {
      #ifdef DEBUG_INPUT
      MessageInterface::ShowMessage
         ("FunctionManager::GetInputWrapper() returning NULL, "
          "formal input name '%s' not found\n", formalInput.c_str());
      #endif
      return NULL;
   }
   
   ElementWrapper *ew = inputWrapperMap[formalInput];

   #ifdef DEBUG_INPUT
   MessageInterface::ShowMessage
      ("FunctionManager::GetInputWrapper() returning <%p><%s>, wrapperType=%d\n",
       ew, ew ? ew->GetDescription().c_str() : "NULL", ew ? ew->GetWrapperType() : -1);
   #endif

   if (ew && ew->GetWrapperType() == Gmat::NUMBER_WT)
   {      
      #ifdef DEBUG_INPUT
      NumberWrapper *nw = (NumberWrapper*)ew;
      MessageInterface::ShowMessage
         ("FunctionManager::GetInputWrapper() ew value=%f\n", nw->EvaluateReal());
      #endif
   }
   
   return ew;
}


//------------------------------------------------------------------------------
// virtual bool SetPassedInput(Integer index, GmatBase *obj, bool &inputAdded)
//------------------------------------------------------------------------------
bool FunctionManager::SetPassedInput(Integer index, GmatBase *obj, bool &inputAdded)
{
   if (executing) return ReentrantInvocation().SetPassedInput(index, obj, inputAdded);
   PrepareCallSetup();
   inputAdded = false;
   if (!obj || !HasLiveFunction() || index < 0 || index >= (Integer)passedIns.size())
      return false;
   const StringArray &formalInputs = currentFunction->GetStringArrayParameter(
      currentFunction->GetParameterID("Input"));
   if (index >= (Integer)formalInputs.size()) return false;

   // Do not alter a formal object before Execute saves the caller frame and
   // captures every argument. A recursive call can share that very store.
   obj->SetName(passedIns[index]);
   auto &slot = pendingInputValues[index];
   if (slot.get() != obj) slot.reset(obj);
   inputAdded = true;
   return true;
}

//------------------------------------------------------------------------------
// WrapperArray& GetWrappersToDelete()
//------------------------------------------------------------------------------
WrapperArray& FunctionManager::GetWrappersToDelete()
{
   if (executing && reentrantPrepared && reentrantInvocation)
      return reentrantInvocation->GetWrappersToDelete();
   if (!HasLiveFunction())
      throw FunctionException("Cannot access wrappers for an unbound or retired Function.\n");
   if (HasLiveFunction() && !executing && currentFunction->HasActiveInvocation())
      return stagedWrapperDeletions;
   return currentFunction->GetWrappersToDelete();
}

// Sequence methods

//------------------------------------------------------------------------------
// bool PrepareObjectMap()
//------------------------------------------------------------------------------
bool FunctionManager::PrepareObjectMap()
{
   if (executing) return ReentrantInvocation().PrepareObjectMap();
   auto context = Validator::Instance()->CaptureContext(activeManagerExecutions != 0);
   PrepareCallSetup();
   return PrepareObjectMapCore();
}

bool FunctionManager::PrepareObjectMapCore()
{
   #ifdef DEBUG_FM_EXECUTE
   MessageInterface::ShowMessage
      ("FunctionManager::PrepareObjectMap() <%p>'%s' entered, validator=<%p>\n",
       this, functionName.c_str(), validator);
   #endif
   if (!HasLiveFunction())
   {
      std::string errMsg = "FunctionManager:: Unable to execute Function """;
      errMsg += functionName + """ - pointer is NULL\n";
      throw FunctionException(errMsg);
   }
   
   // Initialize the Validator - I think I need to do this each time - or do I?
   //validator.SetSolarSystem(solarSys);
   if (validator == NULL)
      validator = Validator::Instance();
   
   // A prior invocation can replace local/formal roots. Rebuild the borrowed
   // lookup on every entry before evaluating properties or Array expressions.
   combinedObjectStore.clear();
   if (localObjectStore)
      combinedObjectStore.insert(localObjectStore->begin(), localObjectStore->end());
   if (globalObjectStore)
      combinedObjectStore.insert(globalObjectStore->begin(), globalObjectStore->end());
   validator->SetObjectMap(&combinedObjectStore);

   #ifdef DEBUG_FM_EXECUTE
   MessageInterface::ShowMessage
      ("FunctionManager::PrepareObjectMap() <%p>'%s' returning true\n",
       this, functionName.c_str());
   #endif
   
   return true;
}

//------------------------------------------------------------------------------
// virtual bool Initialize()
//------------------------------------------------------------------------------
bool FunctionManager::Initialize()
{
   if (executing) return ReentrantInvocation().Initialize();
   auto context = Validator::Instance()->CaptureContext(activeManagerExecutions != 0);
   PrepareCallSetup();
   std::unique_ptr<FunctionInvocationState> enclosing;
   if (HasLiveFunction() && currentFunction->HasActiveInvocation())
      enclosing = currentFunction->EnterInvocation();
   return InitializeCore();
}

bool FunctionManager::InitializeCore()
{
   if (!HasLiveFunction())
      throw FunctionException("Cannot initialize an unbound or retired Function.\n");
   ForgetRetiredFunctionStore();
   // Direct API initialization can fail before Execute's completion guard.
   // Its partially prepared stores and wrappers still require cleanup.
   isFinalized = false;

   #ifdef DEBUG_TRACE
   static Integer callCount = 0;
   callCount++;
   clock_t t1 = clock();
   ShowTrace(callCount, t1, "FunctionManager::Initialize() entered");
   #endif
   
   #ifdef DEBUG_FM_INIT
   MessageInterface::ShowMessage
      ("=======================================================================\n"
       "Entering FM::Initialize() this FM is <%p>, current function is <%p>'%s'\n   "
       "calling FM is <%p>'%s'\n", this, currentFunction, functionName.c_str(), callingFunction,
       callingFunction ? callingFunction->GetFunctionName().c_str() : "NULL");
   MessageInterface::ShowMessage
      ("FunctionManager::Initialize() clonedObjectStores.size()=%d\n",
       clonedObjectStores.size());
   ShowObjectMap(globalObjectStore, "In FunctionManager::Initialize(), GOS to be used in the function");
   #endif
   
   // clonedObjectStores should not be cleared for recursive call to work
   //clonedObjectStores.clear();
   bool funcInitialized = currentFunction->IsInitialized();
   ObjectMap *previousStore = functionObjectStore;
   const bool previousOwned = isObjStoreOwner;
   auto retainPreviousStore = [&](ObjectMap *replacement)
   {
      const bool retain = previousStore && previousOwned && previousStore != replacement &&
            std::find(clonedObjectStores.begin(), clonedObjectStores.end(), previousStore) == clonedObjectStores.end();
      if (retain) clonedObjectStores.reserve(clonedObjectStores.size() + 1);
      return retain;
   };
   if (!funcInitialized || !currentFunction->GetFunctionObjectStore())
   {
      std::unique_ptr<ObjectMap> store(new ObjectMap);
      auto resources = builtinOutputResources ? builtinOutputResources :
            std::make_shared<FunctionBuiltinOutputResources>();
      const bool retain = retainPreviousStore(store.get());
      RegisterBuiltinOutputStore(store.get(), resources);
      if (retain) clonedObjectStores.push_back(previousStore);
      builtinOutputResources = resources;
      BindFunctionStore(store.release());
      reinitializeFunc = true;
      isObjStoreOwner = true;
   }
   else
   {
      ObjectMap *store = currentFunction->GetFunctionObjectStore();
      const bool retain = retainPreviousStore(store);
      if (retain) clonedObjectStores.push_back(previousStore);
      BindFunctionStore(store);
      reinitializeFunc = false;
   }

   #ifdef DEBUG_MEMORY
   MemoryTracker::Instance()->Add
      (functionObjectStore, "functionObjectStore", "FunctionManager::Initialize()",
       "functionObjectStore = new ObjectMap");
   #endif
   
   #ifdef DEBUG_FM_INIT
   MessageInterface::ShowMessage
      ("   new functionObjectStore <%p> created\n", functionObjectStore);
   #endif
   
   if (callingFunction && !callingFunction->savedLocalObjectStates.empty() &&
         callingFunction->savedLocalObjectStates.top().preserveLiveCaller &&
         IsActiveCallerStore(functionObjectStore))
      throw FunctionException("A function cannot initialize arguments in its active caller's object store.\n");

   if (!ValidateFunctionArguments())
      return false;
   
   if (!CreatePassingArgWrappers())
      return false;

   // Pass in wrappers to function
   currentFunction->SetFunctionObjectMap(functionObjectStore);
   
   #ifdef DEBUG_FM_INIT
   MessageInterface::ShowMessage("Exiting  FM::Initialize() firstExecution set to false\n");
   #endif
   
   firstExecution = false;
   isFinalized = false;
   
   #ifdef DEBUG_TRACE
   ShowTrace(callCount, t1, "FunctionManager::Initialize() exiting", true);
   #endif
   
   return true;
}

//------------------------------------------------------------------------------
// bool Execute(FunctionManager *callingFM)
//------------------------------------------------------------------------------
bool FunctionManager::Execute(FunctionManager *callingFM)
{
   if (executing)
      return InvokeReentrant([callingFM](FunctionManager &frame)
            { return frame.Execute(callingFM); });
   auto validation = Validator::Instance()->CaptureContext(activeManagerExecutions != 0);
   PrepareCallSetup();
   if (activeManagerExecutions == std::numeric_limits<size_t>::max())
      throw FunctionException("Function manager activation depth cannot be represented.\n");
   struct ExecutionScope
   {
      FunctionManager &manager;
      explicit ExecutionScope(FunctionManager &value) : manager(value)
      {
         manager.executing = true;
         ++activeManagerExecutions;
      }
      ~ExecutionScope()
      {
         manager.executing = false;
         --activeManagerExecutions;
         if (manager.contextRetirementRequested) manager.RetireExpressionContext();
      }
   } execution(*this);
   // A failed invocation must never expose a preceding call's typed result.
   // Retain its owned storage until argument binding/output replacement or
   // cleanup: a C++ caller may pass the preceding result as the next input.
   outputType.clear();
   objectResult = NULL;
   if (!HasLiveFunction())
      throw FunctionException("FunctionManager::Execute() Function pointer is NULL");
   
   #ifdef DEBUG_TRACE
   static Integer callCount = 0;
   callCount++;      
   clock_t t1 = clock();
   ShowTrace(callCount, t1, "FunctionManager::Execute() entered");
   #endif
   
   #ifdef DEBUG_FM_EXECUTE
   MessageInterface::ShowMessage
      ("=======================================================================\n"
       "Entering FM::Execute(), this FM is <%p>, current function is <%p>'%s'\n   "
       "calling FM is '%s'\n   solarSys is %p, internalCS is %p, forces is %p\n",
       this, currentFunction, functionName.c_str(),
       callingFM ? callingFM->GetFunctionName().c_str() : "NULL", solarSys,
       internalCS, forces);
   MessageInterface::ShowMessage("   === firstExecution=%d\n", firstExecution);
   #endif
   
   // Solver Parameters read the most recently used clone through a name cache.
   // A private solver declared by this function can shadow a caller solver of
   // the same name. Keep that private result inside this call, including when
   // initialization, execution or result assignment throws. Explicitly Global
   // solvers keep the shared cache and their completed status.
   struct CallerSolverStatuses
   {
      std::map<std::string, Integer> saved;
      ~CallerSolverStatuses()
      {
         for (const auto &entry : saved)
            GmatGlobal::Instance()->SetSolverStatus(entry.first, entry.second);
      }
   } callerSolverStatuses;
   if (currentFunction->IsOfType("UserDefinedFunction") && localObjectStore)
   {
      for (const auto &entry : *localObjectStore)
      {
         auto *object = entry.second;
         if (object && object->IsOfType(Gmat::SOLVER) && !object->IsGlobal())
            callerSolverStatuses.saved[object->GetName()] =
               GmatGlobal::Instance()->GetSolverStatus(object->GetName());
      }
   }

   SharedSubscriberBindings subscriberBindings(localObjectStore, globalObjectStore);

   struct CallerFrame
   {
      FunctionManager *caller;
      size_t maps = 0, values = 0;
      std::shared_ptr<const void> marker;
      std::weak_ptr<const void> storeIdentity;
      bool managed = false;
      explicit CallerFrame(FunctionManager *value) : caller(value)
      {
         if (!caller) return;
         maps = caller->callStack.size();
         values = caller->savedLocalObjectStates.size();
         managed = caller->functionStoreIsManaged;
         storeIdentity = caller->functionStoreIdentity;
      }
      void Remember()
      {
         if (caller && caller->callStack.size() == maps + 1 &&
               caller->savedLocalObjectStates.size() == values + 1)
            marker = caller->savedLocalObjectStates.top().epoch;
      }
      ~CallerFrame() noexcept
      {
         if (!caller || !marker || caller->callStack.size() != maps + 1 ||
               caller->savedLocalObjectStates.size() != values + 1 ||
               caller->savedLocalObjectStates.top().epoch != marker) return;
         try
         {
            if (managed && storeIdentity.expired())
            {
               // Discard only this retired frame, without opening its map.
               caller->callStack.pop();
               caller->savedLocalObjectStates.pop();
               caller->ForgetRetiredFunctionStore();
               return;
            }
            if (!caller->HasLiveFunction()) return;
            caller->validator = Validator::Instance();
            caller->validator->SetSolarSystem(caller->solarSys);
            caller->validator->SetFunction(caller->currentFunction);
            if (!caller->PopFromStack(nullptr, {}, {}, nullptr))
               throw FunctionException("Unable to restore a failed call's caller frame.\n");
         }
         catch (...)
         {
            try { MessageInterface::ShowMessage("Additional error restoring a failed function caller.\n"); }
            catch (...) {}
         }
      }
   } callerFrame(callingFM);

   struct OutputInitializationScope
   {
      Function *function;
      std::weak_ptr<const void> identity;
      std::unique_ptr<FunctionInvocationState> invocation;
      Function::OutputObjectInitializer previous;
      bool restored = false;
      bool restoredSuccessfully = true;
      explicit OutputInitializationScope(Function *callee) :
         function(callee), identity(callee->GetLifetimeToken()),
         invocation(callee->EnterInvocation()),
         previous(callee->ExchangeOutputObjectInitializer({}))
      {
      }
      void Bind(Function::OutputObjectInitializer initialize)
      {
         function->ExchangeOutputObjectInitializer(std::move(initialize));
      }
      bool Restore() noexcept
      {
         if (restored) return restoredSuccessfully;
         restored = true;
         if (GmatBase::IsLiveReference(function, identity))
            function->ExchangeOutputObjectInitializer(std::move(previous));
         if (invocation) restoredSuccessfully = invocation->Restore();
         invocation.reset();
         return restoredSuccessfully;
      }
      ~OutputInitializationScope() noexcept
      {
         Restore();
      }
   } outputInitialization(currentFunction);
   if (deferredFunctionName)
   {
      currentFunction->SetStringParameter("FunctionName", functionName);
      deferredFunctionName = false;
   }

   // Keep failure cleanup inside the caller-solver-status scope so private
   // solver finalization cannot overwrite the restored caller status. Setup,
   // result assignment and stack restoration can fail as well as Execute.
   struct InvocationCompletion
   {
      FunctionManager &manager;
      bool completed = false;

      static void ReportFailure(const char *phase) noexcept
      {
         try
         {
            try
            {
               throw;
            }
            catch (BaseException &error)
            {
               MessageInterface::ShowMessage("Additional function cleanup error (%s): %s\n",
                     phase, error.GetFullMessage().c_str());
            }
            catch (const std::exception &error)
            {
               MessageInterface::ShowMessage("Additional function cleanup error (%s): %s\n",
                     phase, error.what());
            }
            catch (...)
            {
               MessageInterface::ShowMessage("Additional unknown function cleanup error (%s).\n", phase);
            }
         }
         catch (...) {} // Cleanup diagnostics must not replace the original error.
      }

      ~InvocationCompletion()
      {
         if (completed || manager.isFinalized)
            return;
         // Finalizing the FCS can recursively revisit this manager through
         // RunComplete. Retire it during that walk without deleting its object
         // store until finalization and publication cleanup have finished.
         manager.isFinalized = true;
         const bool installed = manager.HasInstalledInvocation();
         if (installed) manager.currentFunction->ClearOutputWrappers();
         try
         {
            if (installed) manager.currentFunction->Finalize(true);
         }
         catch (...) { ReportFailure("function finalization"); }
         manager.isFinalized = false;
         try
         {
            if (manager.publisher && activeManagerExecutions == 1)
               manager.publisher->ClearPublishedData();
         }
         catch (...) { ReportFailure("published data"); }
         try
         {
            manager.Cleanup();
         }
         catch (...) { ReportFailure("invocation state"); }
      }
   } invocation{*this};
   // A prior finalized call must not suppress cleanup of a partially built
   // replacement invocation when argument creation or validation fails.
   isFinalized = false;

   ForgetRetiredFunctionStore();
   PrepareObjectMapCore();
   try { PrepareExecution(callingFM); }
   catch (...) { callerFrame.Remember(); throw; }
   callerFrame.Remember();
   
   if (firstExecution || currentFunction->HasChangingInputType() ||
         !currentFunction->IsInitialized() ||
         currentFunction->GetFunctionObjectStore() != functionObjectStore)
   {
      #ifdef DEBUG_FM_EXECUTE
      MessageInterface::ShowMessage("   First execution, so calling Initialize()\n");
      #endif
      if (!InitializeCore())
         throw FunctionException("Unable to initialize arguments for function \"" +
                                 functionName + "\"\n");
   }
   else
   {
      #ifdef DEBUG_FM_EXECUTE
      MessageInterface::ShowMessage
         ("   NOT First execution, so calling RefreshFOS()\n");
      #endif
      // Need to refresh for nested or recursive function call or
      // function call in math equation for input/output arguments
      //RefreshFOS();
      RefreshFormalInputObjects();
   } // end if not first time through
   
   #ifdef DEBUG_FM_EXECUTE
   MessageInterface::ShowMessage
      ("   Now pass FOS/GOS into the function <%p>'%s'\n", currentFunction,
       GetFunctionName().c_str());
   ShowObjectMap(functionObjectStore, "FOS to pass to function");
   ShowObjectMap(globalObjectStore, "GOS to pass to function");
   #endif
   
   // pass the FOS/GOS and other objects into the function
   BindFunctionServices();
   
   // send all input element wrappers to the function
   std::map<std::string, ElementWrapper *>::iterator ewi;
   for (ewi = inputWrapperMap.begin(); ewi != inputWrapperMap.end(); ++ewi)
   {
      #ifdef DEBUG_FM_EXECUTE
      MessageInterface::ShowMessage
         ("   Calling currentFunction->InputElementWrapper(%s, %p)\n", ewi->first.c_str(),
          ewi->second);
      #endif
      currentFunction->SetInputElementWrapper(ewi->first, ewi->second);
   }
   
   outputInitialization.Bind([this, callee = currentFunction]()
   {
      if (currentFunction != callee)
         throw FunctionException("Function binding changed during output preparation.\n");
      const bool hadStore = functionObjectStore != nullptr;
      ForgetRetiredFunctionStore();
      if (hadStore && !functionObjectStore)
         throw FunctionException("Function output store retired during declaration.\n");
      if (functionObjectStore) RefreshBuiltinOutputObjects();
   });

   RefreshBuiltinOutputObjects();

   // create new ObjectInitializer   
   #ifdef DEBUG_FM_EXECUTE
   MessageInterface::ShowMessage
      ("in FM::Execute (%s), about to create new ObjectInitializer, objInit=<%p>,\n   "
       "solarSys=<%p>, FOS=<%p>, GOS=<%p>, internalCS=<%p>, and true for useGOS\n",
       functionName.c_str(), objInit, solarSys, functionObjectStore, globalObjectStore, internalCS);
   #endif
   
   if (objInit)
   {
      #ifdef DEBUG_MEMORY
      MemoryTracker::Instance()->Remove
         (objInit, "objInit", "FunctionManager::Execute()");
      #endif
      delete objInit;
      objInit = NULL;
   }

   #ifdef DUMP_OBJECT_STORES
   MessageInterface::ShowMessage("FunctionObjectStore:\n");
   for(ObjectMap::iterator it = functionObjectStore->begin(); it != functionObjectStore->end(); ++it)
   {
      GmatBase *obj = it->second;
      MessageInterface::ShowMessage("  %p:  %s ==> %s of type %s\n", obj,
            it->first.c_str(), obj->GetName().c_str(), obj->GetTypeName().c_str());

      if (obj->IsOfType(Gmat::COORDINATE_SYSTEM))
         MessageInterface::ShowMessage("%s\n",
               obj->GetGeneratingString(Gmat::SCRIPTING).c_str());
   }
   MessageInterface::ShowMessage("GlobalObjectStore:\n");
   for(ObjectMap::iterator it = globalObjectStore->begin(); it != globalObjectStore->end(); ++it)
   {
      GmatBase *obj = it->second;
      MessageInterface::ShowMessage("  %p:  %s ==> %s of type %s\n", obj,
            it->first.c_str(), obj->GetName().c_str(), obj->GetTypeName().c_str());

      if (obj->IsOfType(Gmat::COORDINATE_SYSTEM))
         MessageInterface::ShowMessage("%s\n",
               obj->GetGeneratingString(Gmat::SCRIPTING).c_str());
   }
   #endif

   objInit = new ObjectInitializer(solarSys, functionObjectStore,
                                   globalObjectStore, internalCS, true, true);
   
   #ifdef DEBUG_MEMORY
   MemoryTracker::Instance()->Add
      (objInit, "objInit", "FunctionManager::Execute()", "objInit = new ObjectInitializer");
   #endif
   
   #ifdef DEBUG_FM_EXECUTE
   MessageInterface::ShowMessage
      ("FunctionManager::Execute() Now initializing currentFunction '%s'\n",
       currentFunction->GetName().c_str());
   #endif

   if (!(currentFunction->Initialize(objInit, reinitializeFunc)))
   {
      std::string errMsg = "FunctionManager:: Error initializing function \"";
      errMsg += currentFunction->GetStringParameter("FunctionName") + "\"\n";
      throw FunctionException(errMsg);
   }

   RefreshBuiltinOutputObjects();

   reinitializeFunc = false;
   if (!stagedWrapperDeletions.empty())
   {
      auto &wrappers = currentFunction->GetWrappersToDelete();
      wrappers.reserve(wrappers.size() + stagedWrapperDeletions.size());
      wrappers.insert(wrappers.end(), stagedWrapperDeletions.begin(), stagedWrapperDeletions.end());
      stagedWrapperDeletions.clear();
   }

   // Native initialization can rebind nested commands. Restore the complete
   // structural command graph, including branch children, before execution.
   BindFunctionServices();

   #ifdef DEBUG_FM_EXECUTE
   MessageInterface::ShowMessage
      ("in FM::Execute, Now calling currentFunction<%p>'%s'->Execute()\n", currentFunction,
       currentFunction->GetName().c_str());
   MessageInterface::ShowMessage
      ("   Create and Global command may have updated FOS/GOS in the function <%p>'%s'\n",
       currentFunction, GetFunctionName().c_str());
   ShowObjectMap(functionObjectStore, "FOS to be used in the function");
   ShowObjectMap(globalObjectStore, "GOS to be used in the function");
   MessageInterface::ShowMessage
      ("======================================================= Calling currentFunction->Execute()\n");
   #endif
   
   // The invocation guard also handles false returns without assigning results.
   if (!currentFunction->Execute(objInit, reinitializeFunc))
   {
      MessageInterface::ShowMessage
         ("*** ERROR *** FunctionManager \"%s\" finalizing... due to false returned "
          "from currentFunction->Execute()\n", functionName.c_str());
      return false;
   }
   
   #ifdef DEBUG_FM_EXECUTE
   MessageInterface::ShowMessage
      ("======================================================= currentFunction->Execute() DONE\n");
   #endif
   
   // Prepare related object results before any caller assignment can overwrite
   // a source value in an aliased recursive object store.
   OutputObjectValues completedObjects;
   PrepareOutputObjects(completedObjects);
   AssignResult(&completedObjects);
   
   // Now deal with the calling function here
   if (!HandleCallStack(&completedObjects))
      return false;
   if (blankResult && ownedObjectResult)
      if (const auto context = ownedObjectResult->GetFunctionValueContext())
      {
         auto bindings = completedObjects.bindings;
         for (const auto &entry : completedObjects.bindingNames)
         {
            GmatBase *caller = FindObject(entry.second);
            if (!caller) throw FunctionException("Missing caller resource after function return: " + entry.second);
            bindings[entry.first] = caller;
         }
         context->RebindCaller(*ownedObjectResult, completedObjects.epoch, bindings);
      }
   
   #ifdef DEBUG_TRACE
   ShowTrace(callCount, t1, "FunctionManager::Execute() exiting", true);
   #endif
   
   if (!outputInitialization.Restore())
      throw FunctionException("Unable to restore the enclosing function invocation.\n");
   if (!subscriberBindings.Restore())
      throw FunctionException("Unable to restore enclosing subscriber bindings.\n");
   invocation.completed = true;
   return true;
}

//------------------------------------------------------------------------------
// Real FunctionManager::Evaluate(FunctionManager *callingFM)
//------------------------------------------------------------------------------
Real FunctionManager::Evaluate(FunctionManager *callingFM)
{
   if (executing)
      return InvokeReentrant([callingFM](FunctionManager &frame)
            { return frame.Evaluate(callingFM); });
   if (!HasLiveFunction())
   {
      std::string errMsg = "FunctionManager:: Unable to return Real value from Function """;
      errMsg += functionName + """ - pointer is NULL\n";
      throw FunctionException(errMsg);
   }
   
   #ifdef DEBUG_FM_EVAL
   MessageInterface::ShowMessage
      ("Entering FunctionManager::Evaluate() f=<%p><%s>\n", currentFunction,
       currentFunction->GetName().c_str());
   #endif
   
   if (!Execute(callingFM))
      throw FunctionException("FunctionManager: cannot evaluate a failed function call\n");
   
   if (outputType != "Real")
      throw FunctionException("FunctionManager: invalid output type - should be real\n");
   
   #ifdef DEBUG_FM_EVAL
   MessageInterface::ShowMessage
      ("Exiting  FunctionManager::Evaluate() with %f\n", realResult);
   #endif
   
   return realResult;
}

//------------------------------------------------------------------------------
// Rmatrix FunctionManager::MatrixEvaluate(FunctionManager *callingFM)
//------------------------------------------------------------------------------
Rmatrix FunctionManager::MatrixEvaluate(FunctionManager *callingFM)
{
   if (executing)
      return InvokeReentrant([callingFM](FunctionManager &frame)
            { return frame.MatrixEvaluate(callingFM); });
   if (!HasLiveFunction())
   {
      std::string errMsg = "FunctionManager:: Unable to return Rmatrix value from Function """;
      errMsg += functionName + """ - pointer is NULL\n";
      throw FunctionException(errMsg);
   }

   #ifdef DEBUG_FM_EVAL
   MessageInterface::ShowMessage
      ("Entering FunctionManager::MatrixEvaluate() f=<%p><%s>\n", currentFunction,
       currentFunction->GetName().c_str());
   #endif
   
   if (!Execute(callingFM))
      throw FunctionException("FunctionManager: cannot evaluate a failed function call\n");
   
   if (outputType != "Rmatrix" && outputType != "Rvector")
      throw FunctionException("FunctionManager: invalid output type - should be Rmatrix\n");
   
   #ifdef DEBUG_FM_EVAL
   MessageInterface::ShowMessage
      ("Exiting  FunctionManager::MatrixEvaluate() with %s\n", matResult.ToString().c_str());
   #endif
   
   return matResult;
}

Rvector FunctionManager::VectorEvaluate(FunctionManager *callingFM)
{
   if (executing)
      return InvokeReentrant([callingFM](FunctionManager &frame)
            { return frame.VectorEvaluate(callingFM); });
   if (!HasLiveFunction())
      throw FunctionException("Cannot evaluate a vector from a retired function.\n");
   if (!Execute(callingFM))
      throw FunctionException("FunctionManager: cannot evaluate a failed function call\n");
   if (outputType != "Rvector")
      throw FunctionException("FunctionManager: invalid output type - should be Rvector\n");
   return matResult.GetRowOrColumn();
}

//------------------------------------------------------------------------------
// GmatBase* EvaluateObject(FunctionManager *callingFM)
//------------------------------------------------------------------------------
/**
 * Returns reference object pointer so that object can be assigned in a math
 * equation when GmatFunction returns an object. It used only allow GmatFunction
 * returning numeric values in a math equation. For example,
 * "sc = MyFunction(sc)" will work.
 */
//------------------------------------------------------------------------------
GmatBase* FunctionManager::EvaluateObject(FunctionManager *callingFM)
{
   if (executing)
      return InvokeReentrant([callingFM](FunctionManager &frame)
            { return frame.EvaluateObject(callingFM); });
   if (!HasLiveFunction())
   {
      std::string errMsg = "FunctionManager:: Unable to return object from Function """;
      errMsg += functionName + """ - pointer is NULL\n";
      throw FunctionException(errMsg);
   }
   
   #ifdef DEBUG_FM_EVAL
   MessageInterface::ShowMessage
      ("Entering FunctionManager::EvaluateObject() f=<%p><%s>\n", currentFunction,
       currentFunction->GetName().c_str());
   #endif
   
   if (!Execute(callingFM))
      throw FunctionException("FunctionManager: cannot evaluate a failed function call\n");
   
   if (outputType != "Object")
      throw FunctionException("FunctionManager: invalid output type - should be Object\n");
   
   #ifdef DEBUG_FM_EVAL
   MessageInterface::ShowMessage
      ("Exiting  FunctionManager::EvaluateObject() with <%p>'%s'\n", objectResult,
       objectResult ? objectResult->GetName().c_str() : "NULL");
   #endif
   
   return objectResult;
}

//------------------------------------------------------------------------------
// void FunctionManager::Finalize()
//------------------------------------------------------------------------------
void FunctionManager::Finalize()
{
   if (executing)
   {
      finalizationRequested = true;
      return;
   }
   auto context = Validator::Instance()->CaptureContext(activeManagerExecutions != 0);
   finalizationRequested = false;
   ForgetRetiredFunctionStore();
   #ifdef DEBUG_TRACE
   static Integer callCount = 0;
   callCount++;      
   clock_t t1 = clock();
   ShowTrace(callCount, t1, "FunctionManager::Finalize() entered");
   #endif
   
   #ifdef DEBUG_FM_FINALIZE
   MessageInterface::ShowMessage
      ("Entering FM::Finalize, this FM is <%p>, current function is '%s', "
       " calling FM is <%p> '%s'\n", this, functionName.c_str(), callingFunction,
       callingFunction ? callingFunction->GetFunctionName().c_str() : "NULL");
   MessageInterface::ShowMessage
       ("   functionObjectStore=<%p>, localObjectStore=<%p>, clonedObjectStores.size()=%d, "
        "isFinalized=%d\n", functionObjectStore, localObjectStore, clonedObjectStores.size(),
        isFinalized);
   #ifdef DEBUG_OBJECT_MAP
   ShowObjectMap(functionObjectStore, "FOS in Finalize");
   ShowObjectMap(localObjectStore, "LOS in Finalize");
   //ShowObjectMap(clonedObjectStore, "clonedOS in Finalize");
   #endif
   #endif
   
   if (!isFinalized || reentrantInvocation || completedInvocation || !stagedWrapperDeletions.empty())
   {
      Cleanup();
   
      firstExecution = true;
      isFinalized = true;
   }
   
   #ifdef DEBUG_TRACE
   ShowTrace(callCount, t1, "FunctionManager::Finalize() exiting", true, true);
   #endif
}

//------------------------------------------------------------------------------
// bool IsFinalized()
//------------------------------------------------------------------------------
bool FunctionManager::IsFinalized()
{
   return isFinalized && !finalizationRequested &&
         (!reentrantInvocation || reentrantInvocation->IsFinalized()) &&
         (!completedInvocation || completedInvocation->IsFinalized());
//    if (currentFunction)
//       return currentFunction->IsFcsFinalized();
//    return true;
}

//---------------------------------
// protected methods
//---------------------------------

//------------------------------------------------------------------------------
// void PrepareExecution(FunctionManager *callingFM = NULL)
//------------------------------------------------------------------------------
void FunctionManager::PrepareExecution(FunctionManager *callingFM)
{
   #ifdef DEBUG_FM_EXECUTE
   MessageInterface::ShowMessage
      ("Entering FM::PrepareExecution for '%s'\n", functionName.c_str());
   MessageInterface::ShowMessage
      ("   and the calling FM is '%s'\n",
       callingFM? (callingFM->GetFunctionName()).c_str() : "NULL");
   #endif
   
   callers.push(callingFunction);
   callingFunction  = callingFM;
   
   #ifdef DEBUG_FM_EXECUTE
   ShowCallers("After push");
   #endif
   
   if (callingFunction != NULL)
   {
      #ifdef DO_NOT_EXECUTE_NESTED_GMAT_FUNCTIONS
         Finalize(); // Do we need Finalize here? (loj: 2008.09.11)
         std::string noNested = "FunctionManager (";
         noNested += functionName + ") - nested functions not yet supported";
         throw FunctionException(noNested);
      #else
         #ifdef DEBUG_FM_EXECUTE
         ShowObjectMap(localObjectStore, "=====> LOS in FM::PrepareExecution() before push");
         #endif
         losStack.push(localObjectStore);
         const bool privateStore = !currentFunction->IsInitialized() ||
               !currentFunction->GetFunctionObjectStore() ||
               !IsActiveCallerStore(currentFunction->GetFunctionObjectStore());
         callingFunction->PushToStack(callingFunction->executing && privateStore);
         #ifdef DEBUG_FM_EXECUTE
         ShowObjectMap(localObjectStore, "=====> LOS in FM::PrepareExecution() after  push");
         #endif
      #endif
   }
   
   #ifdef DEBUG_FM_EXECUTE
   MessageInterface::ShowMessage("Exiting  FM::PrepareExecution() for '%s'\n", functionName.c_str());
   #endif
}


//------------------------------------------------------------------------------
// bool ValidateFunctionArguments()
//------------------------------------------------------------------------------
/**
 * Checks function input/output arguments against function's formal parameters.
 */
//------------------------------------------------------------------------------
bool FunctionManager::ValidateFunctionArguments()
{
   #ifdef DEBUG_FM_INIT
   MessageInterface::ShowMessage
      ("Entering FM::ValidateFunctionArguments for <%p>'%s'\n", currentFunction,
       functionName.c_str());
   #endif
   if (!firstExecution) return true;
   
   StringArray inFormalNames = currentFunction->GetStringArrayParameter(currentFunction->GetParameterID("Input"));
   StringArray outFormalNames = currentFunction->GetStringArrayParameter(currentFunction->GetParameterID("Output"));
   
   #ifdef DEBUG_FM_INIT
   MessageInterface::ShowMessage
      ("    inFormalNames.size=%d,  passedIns.size=%d\n", inFormalNames.size(), passedIns.size());
   MessageInterface::ShowMessage
      ("   outFormalNames.size=%d, passedOuts.size=%d\n", outFormalNames.size(), passedOuts.size());
   for (unsigned int rr = 0; rr < inFormalNames.size(); rr++)
      MessageInterface::ShowMessage
         ("    inFormalNames[%d] = %s\n", rr, (inFormalNames.at(rr)).c_str());
   if (passedIns.size() == 0)
      MessageInterface::ShowMessage("NOTE - passedIns is empty - is it supposed to be?\n");
   else
      for (unsigned int qq = 0; qq < passedIns.size(); qq++)
         MessageInterface::ShowMessage("        passedIns[%d] = %s\n", qq, (passedIns.at(qq)).c_str());
   for (unsigned int rr = 0; rr < outFormalNames.size(); rr++)
      MessageInterface::ShowMessage
         ("   outFormalNames[%d] = %s\n", rr, (outFormalNames.at(rr)).c_str());
   if (passedOuts.size() == 0)
      MessageInterface::ShowMessage("NOTE - passedOuts is empty - is it supposed to be?\n");
   else
      for (unsigned int qq = 0; qq < passedOuts.size(); qq++)
         MessageInterface::ShowMessage("       passedOuts[%d] = %s\n", qq, (passedOuts.at(qq)).c_str());
   #endif
   
   
   // check input arguments
   // Number of inputs cannot be more than number of formal input arguments,
   // but can we have less than formal input arguments?
   // Can we have default input value?
   // Currently if number of input is less than formal, it is created as String,
   // so number of inputs must match for now.
   if (passedIns.size() != inFormalNames.size())
   {
      #ifdef DEBUG_FM_INIT
      MessageInterface::ShowMessage
         ("**** ERROR **** FM::ValidateFunctionArguments() throwing exception: "
          "Number of inputs doesn't match\n");
      #endif
      FunctionException ex;
      ex.SetDetails("The function '%s' expecting %d input argument(s), but called "
                    "with %d argument(s)", currentFunction->GetFunctionPathAndName().c_str(),
                    inFormalNames.size(), passedIns.size());
      throw ex;
   }
   
   // check output arguments   
   // Number of outputs cannot be more than number of formal output arguments,
   // but can we have less than formal output arguments?
   // Let's just check for more than formal output for now.
   if (passedOuts.size() > outFormalNames.size())
   {
      #ifdef DEBUG_FM_INIT
      MessageInterface::ShowMessage
         ("**** ERROR **** FM::ValidateFunctionArguments() throwing exception: "
          "Number of outputs doesn't match\n");
      #endif
      FunctionException ex;
      ex.SetDetails("The function '%s' expecting %d output argument(s), but called "
                    "with %d argument(s)", currentFunction->GetFunctionPathAndName().c_str(),
                    outFormalNames.size(), passedOuts.size());
      throw ex;
   }
   
   #ifdef DEBUG_FM_INIT
   MessageInterface::ShowMessage
      ("Exiting  FM::ValidateFunctionArguments with true for <%p>'%s'\n",
       currentFunction, functionName.c_str());
   #endif
   
   return true;
}

//------------------------------------------------------------------------------
// CaptureInputObjects / BindCapturedValue / RebindCapturedObjects
//------------------------------------------------------------------------------
FunctionManager::LocalObjectState FunctionManager::CaptureInputObjects()
{
   LocalObjectState captured;
   captured.epoch = std::make_shared<const int>(0);
   const StringArray formalNames = currentFunction->GetStringArrayParameter(
         currentFunction->GetParameterID("Input"));
   if (formalNames.size() != passedIns.size())
      throw FunctionException("Function input count changed during argument capture.\n");
   std::map<Integer, std::unique_ptr<GmatBase>> pending;
   pending.swap(pendingInputValues);
   std::map<const GmatBase*, size_t> occurrences;
   for (UnsignedInt i = 0; i < passedIns.size(); ++i)
   {
      const auto evaluated = pending.find(i);
      GmatBase *source = evaluated == pending.end() ? FindObject(passedIns[i]) : evaluated->second.get();
      const GmatBase *identity = NULL;
      if (source && !source->IsOfType(Gmat::PARAMETER) &&
          !source->IsOfType(Gmat::FUNCTION) && OwnsFunctionObject(source) &&
          evaluated == pending.end())
         identity = source;

      // A system Parameter is an evaluated scalar/matrix/String argument, not
      // a handle back to the caller spacecraft or other parameter owner.
      if (!source || (evaluated == pending.end() && source->IsOfType(Gmat::PARAMETER)))
         source = CreateObject(passedIns[i]);
      if (!source)
         throw FunctionException("Cannot capture function input " + passedIns[i] + ".\n");
      std::unique_ptr<GmatBase> value(CloneFunctionValue(source));
      if (value.get() == source)
      {
         value.release();
         throw FunctionException("Function input capture Clone returned its source.\n");
      }
      if (!value || value->GetTypeName() != source->GetTypeName())
         throw FunctionException("Cannot clone captured function input " + passedIns[i] + ".\n");
      BindCapturedValue(value.get(), source, localObjectStore, captured.epoch);
      if (identity)
      {
         ++occurrences[identity];
         captured.bindingNames[identity] = formalNames[i];
      }
      captured.objects.emplace(formalNames[i], std::move(value));
   }
   // Repeated actual arguments produce independent formal values. A reference
   // from another argument has no unique formal target in that case; retain
   // its independent captured dependency instead of guessing a last binding.
   for (const auto &entry : occurrences)
      if (entry.second != 1) captured.bindingNames.erase(entry.first);
   return captured;
}

void FunctionManager::BindCapturedValue(GmatBase *value, GmatBase *source,
      ObjectMap *scene, const std::shared_ptr<const void> &epoch)
{
   if (!value || !source ||
       (!SupportsRetainedFunctionRoot(source) && !source->IsOfType("EphemerisFile"))) return;
   auto context = std::make_shared<ReturnedEphemerisFrames>();
   context->solarSystem = solarSys;
   context->runtimeForces = forces;
   context->bindingEpoch = epoch;
   for (ObjectMap *store : {scene, globalObjectStore})
      if (store)
         for (const auto &entry : *store)
            if (OwnsFunctionObject(entry.second)) context->privateObjects.insert(entry.second);
   const auto previous = value->GetFunctionValueContext();
   if (previous) context->inheritedValues.push_back(previous);
   value->SetFunctionValueContext(context);
   context->BindCopy(source, value, true);
   context->Initialize();
   context->inheritedValues.clear();
}

void FunctionManager::RebindCapturedObjects(const LocalObjectState &captured,
      const ObjectMap &targets)
{
   std::map<const GmatBase*, GmatBase*> bindings;
   for (const auto &entry : captured.bindingNames)
   {
      const auto target = targets.find(entry.second);
      if (target != targets.end() && target->second) bindings[entry.first] = target->second;
   }
   for (const auto &entry : targets)
      if (entry.second)
         if (const auto context = entry.second->GetFunctionValueContext())
            context->RebindCaller(*entry.second, captured.epoch, bindings);
}

//------------------------------------------------------------------------------
// bool CreatePassingArgWrappers()
//------------------------------------------------------------------------------
/**
 * Creates passing input and output argument wrappers.
 *
 * It finds input from LOS or GOS and clones it and adds it to input wrapper array.
 * If the input string does not refer to an object that can be located in the LOS
 * or GOS, it will try to create an object for it, e.g. numeric literal, string
 * literal. Since Outputs cannot be numeric or string literals or array elements, etc.
 * they must be found in the object store; and we do not need to clone them.
 */
//------------------------------------------------------------------------------
bool FunctionManager::CreatePassingArgWrappers(LocalObjectState *captured)
{
   #ifdef DEBUG_FM_INIT
   MessageInterface::ShowMessage("\n==================================================\n");
   MessageInterface::ShowMessage
      ("FunctionManager::CreatePassingArgWrappers() entered for '%s'\n", functionName.c_str());
   MessageInterface::ShowMessage
      ("currentFunction=<%p>, isBuiltinGmatFunction=%d\n", currentFunction,
       currentFunction ? currentFunction->IsOfType("BuiltinGmatFunction") : -1);
   ShowObjectMap(functionObjectStore, "functionObjectStore in FM:CreatePassingArgWrappers()");
   ShowObjectMap(localObjectStore, "localObjectStore in FM:CreatePassingArgWrappers()");
   ShowObjectMap(globalObjectStore, "globalObjectStore in FM:CreatePassingArgWrappers()");
   #endif
   
   GmatBase *obj, *objFOS;
   std::string formalName, passedName;
   
   StringArray inFormalNames = currentFunction->GetStringArrayParameter(currentFunction->GetParameterID("Input"));
   StringArray outFormalNames = currentFunction->GetStringArrayParameter(currentFunction->GetParameterID("Output"));
   
   validator->SetObjectMap(&combinedObjectStore);
   validator->SetSolarSystem(solarSys);
   
   LocalObjectState ownedCapture;
   if (!captured)
   {
      ownedCapture = CaptureInputObjects();
      captured = &ownedCapture;
   }
   ObjectMap argumentTargets;

   // clear input and out wrappers first
   ClearInOutWrappers();
   numVarsCreated = 0;
   blankResult = false;
   
   #ifdef DEBUG_FM_INIT
   MessageInterface::ShowMessage
      ("   There are %d passed inputs\n", passedIns.size());
   #endif
   // set up the FOS with the input objects
   bool hasChangingInputType = false;
   for (unsigned int ii=0; ii<passedIns.size(); ii++)
   {
      formalName = inFormalNames.at(ii);
      passedName = passedIns.at(ii);
      obj = captured->objects.at(formalName).get();
      objFOS = CloneFunctionValue(obj);

      if (objFOS == obj)
         throw FunctionException("Function input Clone returned its source: " + passedName);
      std::unique_ptr<GmatBase> ownedArgument(objFOS);
      if (!objFOS)
         throw FunctionException("Cannot clone function input \"" + passedName + "\"\n");
      objFOS->SetName(formalName);
      objFOS->SetIsLocal(true);
      objFOS->SetIsGlobal(false);
      
      // If object already exists and is the same type, we can keep the object
      // and just copy the data
      ObjectMap::iterator it = functionObjectStore->find(formalName);
      if (it == functionObjectStore->end())
      {
         functionObjectStore->insert(std::make_pair(formalName, objFOS));
         ownedArgument.release();
      }
      else if (it->second &&
               it->second->GetTypeName() == objFOS->GetTypeName())
      {
         it->second->Copy(objFOS);
         it->second->SetFullName(passedName);
         objFOS = it->second;
      }
      else
      {
         hasChangingInputType = hasChangingInputType || !it->second ||
            it->second->GetTypeName() != objFOS->GetTypeName();
         if (it->second != objFOS)
            DeleteObject(it->second);
         it->second = objFOS;
         ownedArgument.release();
      }
      
      argumentTargets[formalName] = objFOS;

      #ifdef DEBUG_FM_INIT // ------------------------------------------------- debug ---
         MessageInterface::ShowMessage("   Adding object '%s' to the FOS\n", formalName.c_str());
      #endif // -------------------------------------------------------------- end debug ---
      // create an element wrapper for the input
      // Do we neet to set this inside the loop? commented out (loj: 2008.11.21)
      //validator->SetObjectMap(&combinedObjectStore);
      //validator->SetSolarSystem(solarSys);
      // Owned inputs are evaluated through their formal local object. A
      // caller-side array/property wrapper would otherwise remain live on the
      // caller, or point to a temporary clone discarded during object reuse.
      std::string inName = formalName;
      #ifdef DEBUG_WRAPPERS
      MessageInterface::ShowMessage
         ("==========> FM:CreatePassingArgWrappers() '%s' Creating "
          "ElementWrapper for '%s'\n", functionName.c_str(), inName.c_str());
      #endif
      
      std::unique_ptr<ElementWrapper> inWrapper;
      validator->SetObjectMap(functionObjectStore);
      try
      {
         inWrapper.reset(validator->CreateElementWrapper(inName, false, 0));
      }
      catch (...)
      {
         validator->SetObjectMap(&combinedObjectStore);
         throw;
      }
      validator->SetObjectMap(&combinedObjectStore);
      if (!inWrapper)
         throw FunctionException("Cannot create function input wrapper \"" + inName + "\"\n");
      
      #ifdef DEBUG_MORE_MEMORY
      MessageInterface::ShowMessage
         ("+++ FunctionManager::CreatePassingArgWrappers() *inWrapper = validator->"
          "CreateElementWrapper(%s), <%p> '%s'\n", inName.c_str(), inWrapper.get(),
          inWrapper->GetDescription().c_str());
      #endif
      if (!inWrapper->SetRefObject(objFOS))
         throw FunctionException("Cannot bind function input \"" + inName + "\"\n");
      #ifdef DEBUG_FM_INIT // ------------------------------------------------- debug ---
         MessageInterface::ShowMessage("   Created element wrapper of type %d for \"%s\"\n",
               inWrapper->GetWrapperType(), inName.c_str());
      #endif // -------------------------------------------------------------- end debug ---
      inputWrapperMap.insert(std::make_pair(formalName, inWrapper.get()));
      inWrapper.release();
   }
   RebindCapturedObjects(*captured, argumentTargets);
   currentFunction->SetChangingInputType(hasChangingInputType);
   
   // Built-ins write through their formal output objects even when the caller
   // consumes an unnamed result in a math expression. Allocate those objects
   // before choosing whether a caller output wrapper is needed.
   RefreshBuiltinOutputObjects();
   

   // Outputs must refer to caller objects or writable array elements, not
   // numeric/string literals. Resolve the owner; the wrapper validates indices.
   // We do not need to clone these caller objects.
   // Handle the case with one blank output (called from FunctionRunner) first   
   if ((passedOuts.size() == 1) && (passedOuts.at(0) == ""))
   {
      #ifdef DEBUG_FM_INIT
      MessageInterface::ShowMessage("   Has 1 output and it's name is blank\n");
      #endif
      blankResult = true;
   }
   else
   {
      for (unsigned int jj = 0; jj < passedOuts.size(); jj++)
      {
         if (!(obj = FindObject(passedOuts.at(jj), true)))
         {
            std::string errMsg = "Output \"" + passedOuts.at(jj);
            errMsg += " not found for function \"" + functionName + "\"";
            throw FunctionException(errMsg);
         }
         std::string outName = passedOuts.at(jj);         
         #ifdef DEBUG_WRAPPERS
         MessageInterface::ShowMessage
            ("==========> FM:CreatePassingArgWrappers() '%s' Creating "
             "ElementWrapper for '%s'\n", functionName.c_str(), outName.c_str());
         #endif
         
         std::unique_ptr<ElementWrapper> outWrapper(
            validator->CreateElementWrapper(outName));
         if (!outWrapper || !outWrapper->SetRefObject(obj))
            throw FunctionException("Cannot bind function output \"" + outName + "\"\n");
         
         #ifdef DEBUG_MORE_MEMORY
         MessageInterface::ShowMessage
            ("+++ FunctionManager::CreatePassingArgWrappers() *outWrapper = validator->"
             "CreateElementWrapper(%s), <%p> '%s'\n", outName.c_str(), outWrapper.get(),
             outWrapper->GetDescription().c_str());
         #endif
         outputWrappers.push_back(outWrapper.release());
         #ifdef DEBUG_FM_INIT // ------------------------------------------------- debug ---
         MessageInterface::ShowMessage
            ("   Output wrapper created <%p> for %s\n", outputWrappers.back(), outName.c_str());
         #endif // -------------------------------------------------------------- end debug ---
      }
   }
   
   #ifdef DEBUG_FM_INIT
   ShowObjectMap(functionObjectStore, "in CreatePassingArgWrappers()");
   MessageInterface::ShowMessage
      ("FunctionManager::CreatePassingArgWrappers() returning true for '%s'\n",
       functionName.c_str());
   MessageInterface::ShowMessage("==================================================\n");
   #endif
   
   return true;
}

//------------------------------------------------------------------------------
// void RefreshFOS()
//------------------------------------------------------------------------------
/*
 * Deletes all objects in the function object store except formal inputs
 */
//------------------------------------------------------------------------------
void FunctionManager::RefreshFOS()
{
   #ifdef DEBUG_FM_REFRESH
   MessageInterface::ShowMessage
      ("FM:RefreshFOS() entered for '%s', current FOS = <%p>, "
       "has %d objects\n", functionName.c_str(), functionObjectStore, functionObjectStore->size());
   #endif
   
   if (functionObjectStore == NULL)
      throw FunctionException
         ("FunctionManager::RefreshFOS() function pointer is NULL");
   
   // Need to delete all items in the FOS that are not inputs (so that they can 
   // properly be created again in the FCS 
   StringArray toDelete;
   bool        isInput = false;
   std::map<std::string, GmatBase *>::iterator omi;
   StringArray formalNames =
      currentFunction->GetStringArrayParameter(currentFunction->GetParameterID("Input"));
   StringArray outFormalNames =
      currentFunction->GetStringArrayParameter(currentFunction->GetParameterID("Output"));
   
   // combine the lists of inputs/outputs 
   for (Integer ii = 0; ii < outFormalNames.size(); ii++)
      formalNames.push_back(outFormalNames.at(ii));

   #ifdef DEBUG_FM_REFRESH
   MessageInterface::ShowMessage
      ("   Function '%s' has %d inputs and FOS <%p> has %d objects\n",
       functionName.c_str(), formalNames.size(), functionObjectStore, functionObjectStore->size());
   MessageInterface::ShowMessage("  inputs are:\n");
   for (Integer ii = 0; ii < formalNames.size(); ii++)
      MessageInterface::ShowMessage("   %d  %s\n", ii,
                                    formalNames.at(ii).c_str());
   #endif

   for (omi = functionObjectStore->begin(); omi != functionObjectStore->end(); ++omi)
   {
      isInput = false;
      for (unsigned int qq = 0; qq < formalNames.size(); qq++)
         if (omi->first == formalNames.at(qq))
         {
            isInput = true;
            break;
         }
      if (!isInput) 
      {  
         if (omi->second != NULL)
         {
            #ifdef DEBUG_MEMORY
            MemoryTracker::Instance()->Remove
               (omi->second, (omi->second)->GetName(), "FunctionManager::RefreshFOS()");
            #endif
            delete omi->second;
            omi->second = NULL;
         }
         toDelete.push_back(omi->first); 
      }
   }
   for (unsigned int kk = 0; kk < toDelete.size(); kk++)
   {
      #ifdef DEBUG_FM_REFRESH
      MessageInterface::ShowMessage
         ("   erasing '%s' from FOS <%p>\n", toDelete.at(kk).c_str(), functionObjectStore);
      #endif
      functionObjectStore->erase(toDelete.at(kk));
   }

   
   #ifdef DEBUG_FM_REFRESH
   MessageInterface::ShowMessage
      ("FM:RefreshFOS() leaving for '%s', has %d objects in FOS\n",
       functionName.c_str(), functionObjectStore->size());
   #endif
}


//------------------------------------------------------------------------------
// void RefreshFormalInputObjects()
//------------------------------------------------------------------------------
/*
 * Finds object with passing input argument name from LOS or GOS or createdLiterals
 * or createdOthers and set to object in the FOS.
 */
//------------------------------------------------------------------------------
void FunctionManager::RefreshFormalInputObjects()
{
   LocalObjectState captured = CaptureInputObjects();
   // A changed value type needs a matching formal object and typed wrapper.
   // Reuse this capture: reevaluating expressions could change their values.
   for (const auto &entry : captured.objects)
   {
      const auto formal = functionObjectStore->find(entry.first);
      if (formal != functionObjectStore->end() &&
          (!formal->second || formal->second->GetTypeName() != entry.second->GetTypeName()))
      {
         if (!CreatePassingArgWrappers(&captured))
            throw FunctionException("Cannot rebuild changed function input types.\n");
         return;
      }
   }

   ObjectMap argumentTargets;
   for (const auto &entry : captured.objects)
   {
      GmatBase *target = NULL;
      const auto local = functionObjectStore->find(entry.first);
      if (local != functionObjectStore->end())
      {
         target = local->second;
         target->Copy(entry.second.get());
         argumentTargets[entry.first] = target;
      }
      else if (globalObjectStore)
      {
         const auto global = globalObjectStore->find(entry.first);
         if (global != globalObjectStore->end()) target = global->second;
      }
      const auto wrapper = inputWrapperMap.find(entry.first);
      if (!target || wrapper == inputWrapperMap.end() || !wrapper->second ||
          !wrapper->second->SetRefObject(target))
         throw FunctionException("Cannot refresh function input " + entry.first + ".\n");
   }
   // No root Copy occurs after this point. Native component addresses in each
   // formal are now stable for references from the other captured arguments.
   RebindCapturedObjects(captured, argumentTargets);
}


//------------------------------------------------------------------------------
// GmatBase* FindObject(const std::string &name, bool arrayElementsAllowed = false)
//------------------------------------------------------------------------------
GmatBase* FunctionManager::FindObject(const std::string &name, bool arrayElementsAllowed)
{
   std::string key = name;
   const auto index = name.find('(');
   if (index != std::string::npos)
   {
      if (!arrayElementsAllowed) return nullptr;
      key = name.substr(0, index);
   }
   if (localObjectStore)
   {
      const auto found = localObjectStore->find(key);
      if (found != localObjectStore->end()) return found->second;
   }
   if (globalObjectStore)
   {
      const auto found = globalObjectStore->find(key);
      if (found != globalObjectStore->end()) return found->second;
   }
   if (solarSys)
   {
      GmatBase *object = solarSys->GetBody(key);
      if (!object) object = solarSys->GetSpecialPoint(key);
      return object;
   }
   return nullptr;
}

//------------------------------------------------------------------------------
// GmatBase* CreateObject(const std::string &fromString)
//------------------------------------------------------------------------------
/*
 * Materializes a manager-owned input value. Numeric and String literals reuse
 * their snapshots. Other expressions are evaluated by reported value type;
 * dynamic snapshots replace the previous cache only after evaluation succeeds.
 *
 * @param fromString input name of the object to be created
 *
 * @return manager-owned snapshot (possibly an existing literal)
 */
//------------------------------------------------------------------------------
GmatBase* FunctionManager::CreateObject(const std::string &fromString)
{
   if (GmatStringUtil::IsBlank(fromString))
      return NULL;
   const std::string name = GmatStringUtil::Trim(fromString, GmatStringUtil::BOTH);
   std::unique_ptr<GmatBase> value;
   Real number;
   ObjectMap *cache = &createdOthers;

   // Literal snapshots can be reused by multiple formal inputs. Each formal
   // receives its own clone; allocating another uncached literal leaks it.
   const bool numericLiteral = GmatStringUtil::ToReal(name, &number);
   const bool stringLiteral = GmatScriptString::IsLiteral(name);
   if (numericLiteral || stringLiteral)
   {
      cache = &createdLiterals;
      auto existing = cache->find(name);
      if (existing != cache->end() && existing->second)
         return existing->second;
      if (numericLiteral)
      {
         std::unique_ptr<Variable> variable(new Variable(name));
         variable->SetReal(number);
         value = std::move(variable);
      }
      else
      {
         std::unique_ptr<StringVar> text(new StringVar(name));
         text->SetStringParameter("Value", GmatScriptString::Decode(name));
         value = std::move(text);
      }
   }
   else
   {
      validator->SetObjectMap(&combinedObjectStore);
      validator->SetSolarSystem(solarSys);
      auto deleteWrapper = [](ElementWrapper *wrapper)
      {
         #ifdef DEBUG_MEMORY
         if (wrapper)
            MemoryTracker::Instance()->Remove(wrapper, wrapper->GetDescription(),
                                             "FunctionManager::CreateObject()");
         #endif
         delete wrapper;
      };
      std::unique_ptr<ElementWrapper, decltype(deleteWrapper)> wrapper(
         validator->CreateElementWrapper(name, false, 2), deleteWrapper);
      if (!wrapper)
         throw FunctionException("Cannot evaluate input \"" + name +
                                 "\" for function \"" + functionName + "\"\n");
      if (wrapper->GetWrapperType() == Gmat::ARRAY_ELEMENT_WT)
      {
         GmatBase *owner = FindObject(name, true);
         if (!owner || !wrapper->SetRefObject(owner))
            throw FunctionException("Cannot bind Array input \"" + name + "\"\n");
      }

      // A property/Parameter wrapper can hold real, String or matrix data.
      // Its class alone does not determine the value supplied to the function.
      switch (wrapper->GetDataType())
      {
         case Gmat::REAL_TYPE:
         case Gmat::INTEGER_TYPE:
         {
            const Real scalar = wrapper->GetDataType() == Gmat::INTEGER_TYPE ?
               static_cast<Real>(wrapper->EvaluateInteger()) : wrapper->EvaluateReal();
            std::unique_ptr<Variable> variable(new Variable(name));
            variable->SetReal(scalar);
            value = std::move(variable);
            break;
         }
         case Gmat::STRING_TYPE:
         case Gmat::ENUMERATION_TYPE:
         case Gmat::FILENAME_TYPE:
         case Gmat::COLOR_TYPE:
         {
            const std::string scalar = wrapper->EvaluateString();
            std::unique_ptr<StringVar> text(new StringVar(name));
            text->SetStringParameter("Value", scalar);
            value = std::move(text);
            break;
         }
         case Gmat::RVECTOR_TYPE:
         case Gmat::RMATRIX_TYPE:
         {
            const Rmatrix matrix = ReadFunctionArray(*wrapper);
            std::unique_ptr<Array> array(new Array(name));
            if (!array->SetSize(matrix.GetNumRows(), matrix.GetNumColumns()))
               throw FunctionException("Cannot size materialized function Array input.\n");
            array->SetRmatrix(matrix);
            value = std::move(array);
            break;
         }
         default:
            throw FunctionException("Input \"" + name + "\" for function \"" +
               functionName + "\" must evaluate to a real number, String or Array\n");
      }
   }

   GmatBase *result = value.get();
   auto slot = cache->find(name);
   GmatBase *previous = NULL;
   if (slot == cache->end())
      cache->insert(std::make_pair(name, result));
   else
   {
      previous = slot->second;
      slot->second = result;
   }
   value.release();
   ++numVarsCreated;
   #ifdef DEBUG_MEMORY
   MemoryTracker::Instance()->Add(result, name, "FunctionManager::CreateObject()",
                                 "materialized input snapshot");
   #endif

   // Replace dynamic snapshots only after evaluation succeeds. Legacy frames
   // can still own an aliased snapshot (GMT-5717); leave those identities with
   // their live store rather than deleting a caller/formal or shared cache.
   if (previous && previous != result)
   {
      bool retained = false;
      const ObjectMap *stores[] = {functionObjectStore, localObjectStore,
         globalObjectStore, &createdLiterals, &createdOthers};
      for (const auto *store : stores)
         if (store)
            for (const auto &entry : *store)
               if (entry.second == previous)
                  retained = true;
      if (!retained)
      {
         #ifdef DEBUG_MEMORY
         MemoryTracker::Instance()->Remove(previous, previous->GetName(),
                                          "FunctionManager::CreateObject()");
         #endif
         delete previous;
      }
   }
   return result;
} // CreateObject()

//------------------------------------------------------------------------------
// GmatBase* CreateObjectForBuiltinGmatFunction(const std::string outName,
//           Gmat::WrapperDataType wType, Integer numRows, Integer numCols)
//------------------------------------------------------------------------------
void FunctionManager::BindFunctionStore(ObjectMap *store)
{
   const auto registration = FindBuiltinOutputStore(store);
   const auto &owner = registration.first;
   functionObjectStore = store;
   functionStoreOwner = owner;
   functionStoreIdentity = registration.second;
   functionStoreIsManaged = owner != nullptr;
   isObjStoreOwner = owner && owner == builtinOutputResources;
}

void FunctionManager::ForgetRetiredFunctionStore()
{
   if (!functionStoreIsManaged) return;
   const auto owner = functionStoreOwner.lock();
   const auto identity = functionStoreIdentity.lock();
   const auto current = FindBuiltinOutputStore(functionObjectStore);
   if (owner && identity && current.first == owner && current.second.lock() == identity) return;
   // A borrowing manager must not inspect a map whose actual owner retired.
   auto *retired = functionObjectStore;
   functionObjectStore = nullptr;
   functionStoreOwner.reset();
   functionStoreIdentity.reset();
   functionStoreIsManaged = false;
   isObjStoreOwner = false;
   firstExecution = true;
   ClearInOutWrappers();
   if (HasLiveFunction()) currentFunction->RetireFunctionObjectStore(retired);
}

bool FunctionManager::RefreshBuiltinOutputObjects()
{
   if (!HasLiveFunction() || !currentFunction->IsOfType("BuiltinGmatFunction")) return false;
   ForgetRetiredFunctionStore();
   if (!functionObjectStore) throw FunctionException("Built-in outputs require a live function store.\n");
   const StringArray names = currentFunction->GetStringArrayParameter("Output");
   IntegerArray rows, columns;
   const WrapperTypeArray types = currentFunction->GetOutputTypes(rows, columns);
   if (names.size() != types.size() || names.size() != rows.size() || names.size() != columns.size())
      throw FunctionException("Built-in function output metadata does not match its formal outputs.\n");
   struct Replacement
   {
      std::string name;
      std::unique_ptr<GmatBase> value;
      GmatBase *previous;
   };
   std::vector<Replacement> pending;
   std::set<std::string> declared;
   for (size_t i = 0; i < names.size(); ++i)
   {
      if (names[i].empty() || !declared.insert(names[i]).second)
         throw FunctionException("Built-in output formal names must be nonempty and distinct.\n");
      if (rows[i] <= 0 || columns[i] <= 0 ||
            ((types[i] == Gmat::VARIABLE_WT || types[i] == Gmat::STRING_OBJECT_WT) &&
             (rows[i] != 1 || columns[i] != 1)) ||
            (types[i] == Gmat::VECTOR_WT && rows[i] != 1 && columns[i] != 1))
         throw FunctionException("Invalid dimensions for a built-in output formal.\n");
      const auto existing = functionObjectStore->find(names[i]);
      GmatBase *previous = existing == functionObjectStore->end() ? NULL : existing->second;
      if (MatchesBuiltinOutput(previous, types[i], rows[i], columns[i])) continue;
      std::unique_ptr<GmatBase> value(CreateObjectForBuiltinGmatFunction(names[i], types[i], rows[i], columns[i]));
      if (!value) throw FunctionException("Cannot create a typed built-in formal output.\n");
      pending.push_back({names[i], std::move(value), previous});
   }
   if (pending.empty()) return false;

   // Allocate the complete replacement dictionary before publishing any slot.
   // Its address stays stable, and aliases outside the replaced slots retain
   // their actual objects. Inputs can still borrow a displaced output.
   ObjectMap staged(*functionObjectStore);
   for (const auto &entry : pending) staged[entry.name] = entry.value.get();
   const auto owner = FindBuiltinOutputStoreOwner(functionObjectStore);
   if (owner)
      for (const auto &entry : pending) owner->Retain(entry.previous);
   // Unregistered external stores and their displaced objects remain under
   // their caller's ownership; this manager does not adopt that authority.
   functionObjectStore->swap(staged);
   for (auto &entry : pending) entry.value.release();
   currentFunction->ClearOutputWrappers();
   return true;
}

GmatBase* FunctionManager::
CreateObjectForBuiltinGmatFunction(const std::string &outName,
                                   Gmat::WrapperDataType wType,
                                   Integer numRows, Integer numCols)
{
   std::unique_ptr<GmatBase> output;
   switch (wType)
   {
      case Gmat::VARIABLE_WT:
         output.reset(new Variable(outName));
         break;
      case Gmat::ARRAY_WT:
      {
         std::unique_ptr<Array> array(new Array(outName));
         if (!array->SetSize(numRows, numCols))
            throw FunctionException("Cannot size built-in function Array output.\n");
         output = std::move(array);
         break;
      }
      case Gmat::VECTOR_WT:
      {
         if (numRows <= 0 || numCols <= 0 || (numRows != 1 && numCols != 1))
            throw FunctionException("Built-in vector output must have one row or column.\n");
         const Integer count = numRows == 1 ? numCols : numRows;
         output.reset(new RvectorVar(outName, count));
         break;
      }
      case Gmat::STRING_OBJECT_WT:
         output.reset(new StringVar(outName));
         break;
      default:
         break;
   }
   #ifdef DEBUG_MEMORY
   if (output)
      MemoryTracker::Instance()->Add(output.get(), outName,
            "FunctionManager::CreateObjectForBuiltinGmatFunction()", "native formal output");
   #endif
   #ifdef DEBUG_CREATE_OBJ
   MessageInterface::ShowMessage("Created native built-in output <%p> '%s'\n",
         output.get(), outName.c_str());
   #endif
   return output.release();
}

//------------------------------------------------------------------------------
// void AssignResult()
//------------------------------------------------------------------------------
/*
 * Sets result to output wrapper
 */
//------------------------------------------------------------------------------
void FunctionManager::PrepareOutputObjects(OutputObjectValues &values)
{
   values.epoch = std::make_shared<const int>(0);
   const StringArray inputs = currentFunction->GetStringArrayParameter("Input");
   const StringArray names = currentFunction->GetStringArrayParameter("Output");
   auto formal = [this](const std::string &name) -> GmatBase*
   {
      for (ObjectMap *store : {functionObjectStore, globalObjectStore})
         if (store)
         {
            const auto found = store->find(name);
            if (found != store->end()) return found->second;
         }
      return NULL;
   };
   for (UnsignedInt i = 0; i < inputs.size() && i < passedIns.size(); ++i)
      if (GmatBase *object = formal(inputs[i]))
         if (OwnsFunctionObject(object) && !object->IsOfType(Gmat::PARAMETER) &&
             !object->IsOfType(Gmat::FUNCTION))
            if (GmatBase *caller = FindObject(passedIns[i]))
            {
               values.bindings[object] = caller;
               values.bindingNames[object] = passedIns[i];
            }
   // Explicit whole-resource outputs take precedence over input aliases.
   for (UnsignedInt i = 0; i < outputWrappers.size(); ++i)
   {
      ElementWrapper *source = currentFunction->GetOutputArgument(i);
      if (!source || source->GetDataType() != Gmat::OBJECT_TYPE ||
          outputWrappers[i]->GetWrapperType() != Gmat::OBJECT_WT) continue;
      GmatBase *object = source->GetRefObject();
      GmatBase *caller = outputWrappers[i]->GetRefObject();
      if (OwnsFunctionObject(object) && caller)
      {
         values.bindings[object] = caller;
         values.bindingNames[object] = i < passedOuts.size() ? passedOuts[i] : caller->GetName();
      }
   }
   if (blankResult) return;
   for (UnsignedInt i = 0; i < outputWrappers.size(); ++i)
   {
      ElementWrapper *wrapper = currentFunction->GetOutputArgument(i);
      if (!wrapper) throw FunctionException("Missing function output during capture.\n");
      const std::string &name = names.at(i);
      std::unique_ptr<GmatBase> scalar;
      std::unique_ptr<ElementWrapper> frozen;
      if (wrapper->GetDataType() == Gmat::REAL_TYPE)
      {
         auto number = std::make_unique<Variable>(name);
         number->SetReal(wrapper->EvaluateReal());
         scalar = std::move(number);
         frozen.reset(new VariableWrapper);
      }
      else if (wrapper->GetDataType() == Gmat::RVECTOR_TYPE)
      {
         // Preserve the native type so assignment retains its normal vector
         // sizing/orientation rules, including row Array destinations.
         auto vector = std::make_unique<RvectorWrapper>(wrapper->EvaluateRvector());
         vector->SetDescription(name);
         vector->AllowOneDimArraySetting(outputWrappers[i]->IsOneDimArraySettingAllowed());
         values.wrappers[name] = std::move(vector);
         continue;
      }
      else if (wrapper->GetDataType() == Gmat::RMATRIX_TYPE)
      {
         const Rmatrix data = ReadFunctionArray(*wrapper);
         auto array = std::make_unique<Array>(name);
         if (!array->SetSize(data.GetNumRows(), data.GetNumColumns()))
            throw FunctionException("Cannot size captured function Array output.\n");
         array->SetRmatrix(data);
         scalar = std::move(array);
         frozen.reset(new ArrayWrapper);
      }
      else if (wrapper->GetDataType() == Gmat::STRING_TYPE)
      {
         auto string = std::make_unique<StringVar>(name);
         string->SetString(wrapper->EvaluateString());
         scalar = std::move(string);
         frozen.reset(new StringObjectWrapper);
      }
      if (scalar)
      {
         scalar->SetIsLocal(true);
         frozen->AllowOneDimArraySetting(outputWrappers[i]->IsOneDimArraySettingAllowed());
         frozen->SetDescription(name);
         if (!frozen->SetRefObject(scalar.get()))
            throw FunctionException("Cannot bind captured function output wrapper.\n");
         values.objects[name] = std::move(scalar);
         values.wrappers[name] = std::move(frozen);
         continue;
      }
      if (wrapper->GetDataType() != Gmat::OBJECT_TYPE) continue;
      GmatBase *source = wrapper->GetRefObject();
      if (!source || (!source->IsOfType("EphemerisFile") &&
          !SupportsRetainedFunctionRoot(source))) continue;
      if (!OwnsFunctionObject(source) && outputWrappers[i]->GetRefObject() == source)
         continue; // True-global identity operations preserve live writers.
      std::unique_ptr<GmatBase> value(CloneFunctionValue(source));
      if (value.get() == source)
      {
         value.release();
         throw FunctionException("FunctionManager: output Clone returned its source\n");
      }
      if (!value) throw FunctionException("Cannot capture a function object output.\n");
      BindReturnedValue(value.get(), source, &values);
      values.objects[name] = std::move(value);
   }
}

void FunctionManager::BindReturnedValue(GmatBase *value, GmatBase *source,
      const OutputObjectValues *bindings)
{
   if (!value || !source) return;
   if (source->IsOfType("EphemerisFile"))
   {
      BindReturnedEphemeris(value, source, bindings);
      return;
   }
   if (!SupportsRetainedFunctionRoot(source)) return;
   auto context = std::make_shared<ReturnedEphemerisFrames>();
   context->solarSystem = solarSys;
   context->runtimeForces = forces;
   if (bindings) context->bindingEpoch = bindings->epoch;
   for (ObjectMap *store : {functionObjectStore, globalObjectStore})
      if (store)
         for (const auto &entry : *store)
            if (OwnsFunctionObject(entry.second)) context->privateObjects.insert(entry.second);
   const auto previous = value->GetFunctionValueContext();
   if (previous) context->inheritedValues.push_back(previous);
   value->SetFunctionValueContext(context);
   context->BindCopy(source, value, true);
   context->Initialize();
   context->inheritedValues.clear();
}

void FunctionManager::BindReturnedEphemeris(GmatBase *value, GmatBase *source,
      const OutputObjectValues *bindings)
{
   if (!value || !source) return;
   auto *ephemeris = dynamic_cast<EphemerisFile*>(value);
   auto *sourceEphemeris = dynamic_cast<EphemerisFile*>(source);
   if (!ephemeris || !sourceEphemeris)
      throw FunctionException("Incompatible native ephemeris result type.\n");
   auto context = std::make_shared<ReturnedEphemerisFrames>();
   context->solarSystem = solarSys;
   context->runtimeForces = forces;
   if (bindings) context->bindingEpoch = bindings->epoch;
   for (ObjectMap *store : {functionObjectStore, globalObjectStore})
      if (store)
         for (const auto &entry : *store)
            if (OwnsFunctionObject(entry.second)) context->privateObjects.insert(entry.second);
   const auto previous = value->GetFunctionValueContext();
   if (previous) context->inheritedValues.push_back(previous);
   value->SetFunctionValueContext(context);
   context->BindCopy(source, value, true);
   context->Initialize();
   context->inheritedValues.clear();
}

void FunctionManager::AssignResult(const OutputObjectValues *values)
{
   struct OutputCompletion
   {
      ObjectManagedFunction &function;
      ~OutputCompletion() { function.ClearOutputWrappers(); }
   } outputs{*currentFunction};
   if (blankResult)
   {
      SaveLastResult(values);
      return;
   }
   const StringArray names = currentFunction->GetStringArrayParameter("Output");
   auto captured = [&](UnsignedInt i) -> GmatBase*
   {
      if (!values) return NULL;
      const auto found = values->objects.find(names.at(i));
      return found == values->objects.end() ? NULL : found->second.get();
   };
   auto assign = [&](UnsignedInt i, GmatBase *overrideValue = NULL)
   {
      ElementWrapper *source = currentFunction->GetOutputArgument(i);
      ObjectWrapper object;
      if (overrideValue)
      {
         object.SetDescription(overrideValue->GetName());
         if (!object.SetRefObject(overrideValue))
            throw FunctionException("Cannot wrap rebound function ephemeris output.\n");
         source = &object;
      }
      else if (values)
      {
         const auto frozen = values->wrappers.find(names.at(i));
         if (frozen != values->wrappers.end()) source = frozen->second.get();
         else if (GmatBase *value = captured(i))
         {
            object.SetDescription(value->GetName());
            if (!object.SetRefObject(value))
               throw FunctionException("Cannot wrap captured function object output.\n");
            source = &object;
         }
      }
      if (!source || !ElementWrapper::SetValue(outputWrappers.at(i), source,
            solarSys, functionObjectStore, globalObjectStore))
         throw FunctionException("FunctionManager::AssignResult() failed to assign function output\n");
   };
   std::vector<UnsignedInt> ephemerides;
   for (UnsignedInt i = 0; i < outputWrappers.size(); ++i)
   {
      GmatBase *value = captured(i);
      ElementWrapper *source = currentFunction->GetOutputArgument(i);
      if (!value && source && source->GetDataType() == Gmat::OBJECT_TYPE)
         value = source->GetRefObject();
      if (value && value->IsOfType("EphemerisFile")) ephemerides.push_back(i);
      else assign(i);
   }
   if (values)
      for (UnsignedInt i = 0; i < outputWrappers.size(); ++i)
         if (captured(i) && !captured(i)->IsOfType("EphemerisFile") &&
             outputWrappers[i]->GetWrapperType() == Gmat::OBJECT_WT)
         {
            GmatBase *target = outputWrappers[i]->GetRefObject();
            if (target)
               if (const auto context = target->GetFunctionValueContext())
                  context->RebindCaller(*target, values->epoch, values->bindings);
         }
   for (UnsignedInt i : ephemerides)
   {
      std::unique_ptr<GmatBase> rebound;
      if (GmatBase *value = captured(i))
      {
         rebound.reset(CloneFunctionValue(value));
         if (rebound.get() == value)
         {
            rebound.release();
            throw FunctionException("Ephemeris result Clone returned its source.\n");
         }
         if (!rebound) throw FunctionException("Cannot stage ephemeris caller binding.\n");
         if (const auto context = rebound->GetFunctionValueContext())
            context->RebindCaller(*rebound, values->epoch, values->bindings);
      }
      // Keep the original captured graph private for PopFromStack: restoring
      // the caller can replace the components used by this first assignment.
      // Native ephemeris Copy still controls accepted/trial writers.
      assign(i, rebound.get());
   }
}

//------------------------------------------------------------------------------
// bool HandleCallStack()
//------------------------------------------------------------------------------
/*
 * Handles callstack.
 */
//------------------------------------------------------------------------------
bool FunctionManager::HandleCallStack(const OutputObjectValues *values)
{
   #ifdef DEBUG_FM_EXECUTE
   MessageInterface::ShowMessage
      ("Entering FM::HandleCallStack for function '%s'\n", functionName.c_str());
   //ShowStackContents(callStack, "Stack contents at beg. of PushToStack");
   //ShowObjectMap(functionObjectStore, "FOS at beg. of PushToStack");
   #endif
      
   if (callingFunction != NULL)
   {
      #ifdef DEBUG_FM_EXECUTE
      MessageInterface::ShowMessage
         ("Calling function is not NULL, so PopFromStack() with functionObjectStore=<%p>\n",
          functionObjectStore);
      #endif
      
      if (!callingFunction->savedLocalObjectStates.empty() &&
            callingFunction->savedLocalObjectStates.top().preserveLiveCaller)
      {
         if (!callingFunction->PopFromStack(nullptr, {}, {}, nullptr)) return false;
      }
      else
      {
         const StringArray outFormalNames = currentFunction->GetStringArrayParameter("Output");
         ObjectMap *currentFOS = functionObjectStore;
         ObjectMap returnStore;
         if (values && !values->objects.empty())
         {
            returnStore = *functionObjectStore;
            for (const auto &entry : values->objects)
               returnStore[entry.first] = entry.second.get();
            currentFOS = &returnStore;
         }
         if (!callingFunction->PopFromStack(currentFOS, outFormalNames, passedOuts, values))
            return false;
      }
      
      // Pop up local object store from stack
      if (losStack.empty())
      {
         localObjectStore = NULL;
      }
      else
      {
         // restore the localObjectStore
         localObjectStore = losStack.top();

         #ifdef DEBUG_FM_STACK
         MessageInterface::ShowMessage("===> after losStack.top(), functionObjectStore=<%p>\n",
            functionObjectStore);
         MessageInterface::ShowMessage("===> after losStack.top(), localObjectStore=<%p>\n",
            localObjectStore);
         #endif

         losStack.pop();
      }
   }
   else
   {
      // Do not call Finalize() it will delete LOS and nested function will not work
      #ifdef DEBUG_FM_EXECUTE
      MessageInterface::ShowMessage
         ("   calling function <%p>'%s'->Finalize()\n", currentFunction,
          currentFunction->GetName().c_str());
      #endif
      
      // Unsubscribe local subscribers since function run is completed
      UnsubscribeSubscribers(functionObjectStore);
      
      // delete cloned object store
      Integer numClones = clonedObjectStores.size();
      for (Integer i=0; i<numClones; i++)
      {
         DeleteObjectMap(clonedObjectStores[i], "clonedOS in Finalize");
         //clonedObjectStores[i] = NULL;                       // It was done inside DeleteObjectMap() function
      }
      clonedObjectStores.clear();
   }
   
   if (!callers.empty())
   {
      // remove the caller from the stack of callers      
      callingFunction = callers.top();
      callers.pop();
      #ifdef DEBUG_FM_EXECUTE
      ShowCallers("After pop");
      #endif
   }
   
   #ifdef DEBUG_FM_EXECUTE
   MessageInterface::ShowMessage("Exiting  FM::HandleCallStack()\n");
   #endif
   
   return true;
}


//------------------------------------------------------------------------------
// void SaveLastResult()
//------------------------------------------------------------------------------
void FunctionManager::SaveLastResult(const OutputObjectValues *values)
{
   ElementWrapper *ew = currentFunction->GetOutputArgument(0);
   if (!ew) 
   {
      std::string errMsg = "FunctionManager: missing output argument from function \"";
      errMsg += functionName + "\"\n";
      throw FunctionException(errMsg);
   }
   
   Gmat::ParameterType outType = ew->GetDataType();
   
   #ifdef DEBUG_FM_EXECUTE
   MessageInterface::ShowMessage
      ("FunctionManager::SaveLastResult() entered, ew='%s', outType=%d\n",
       ew->GetDescription().c_str(), outType);
   #endif
   
   switch (outType) 
   {
   case Gmat::INTEGER_TYPE:
      realResult = (Real) ew->EvaluateInteger();      
      outputType = "Real";
      break;
   case Gmat::REAL_TYPE:
      realResult = ew->EvaluateReal();
      outputType = "Real";
      #ifdef DEBUG_FM_RESULT
      MessageInterface::ShowMessage("   saved realResult=%f\n", realResult);
      #endif
      break;
   case Gmat::RVECTOR_TYPE:
   case Gmat::RMATRIX_TYPE:
   {
      const Rmatrix value = ReadFunctionArray(*ew);
      matResult.SetSize(value.GetNumRows(), value.GetNumColumns());
      matResult = value;
      outputType = outType == Gmat::RVECTOR_TYPE ? "Rvector" : "Rmatrix";
      #ifdef DEBUG_FM_RESULT
      MessageInterface::ShowMessage
         ("   saved matResult=%s\n", matResult.ToString().c_str());
      #endif
      break;
   }
   case Gmat::OBJECT_TYPE:
   {
      GmatBase *result = ew->GetRefObject();
      if (!result)
         throw FunctionException("FunctionManager: missing object result from function \"" +
               functionName + "\"\n");
      if (OwnsFunctionObject(result))
      {
         // PopFromStack restores/reuses local objects after AssignResult.
         // Keep the returned value before that restoration can overwrite it.
         std::unique_ptr<GmatBase> value(CloneFunctionValue(result));
         if (value.get() == result)
         {
            value.release(); // A broken Clone must not delete its source.
            throw FunctionException("FunctionManager: object result Clone returned its source\n");
         }
         if (!value)
            throw FunctionException("FunctionManager: cannot clone object result from function \"" +
                  functionName + "\"\n");
         BindReturnedValue(value.get(), result, values);
         ownedObjectResult = std::move(value);
         objectResult = ownedObjectResult.get();
      }
      else
      {
         // True globals retain their borrowed identity.
         ownedObjectResult.reset();
         objectResult = result;
      }
      outputType = "Object";
      #ifdef DEBUG_FM_RESULT
      MessageInterface::ShowMessage
         ("   saved objectResult=<%p>'%s'\n", objectResult,
          objectResult ? objectResult->GetName().c_str() : "NULL");
      #endif
      break;
   }
   default:
      throw FunctionException("FunctionManager: Unknown or invalid output data type");
   }
   
   #ifdef DEBUG_FM_EXECUTE
   MessageInterface::ShowMessage
      ("FunctionManager::SaveLastResult() exiting\n");
   #endif
}


//------------------------------------------------------------------------------
// ObjectMap* PushToStack()
//------------------------------------------------------------------------------
void FunctionManager::PushToStack()
{
   PushToStack(false);
}

void FunctionManager::PushToStack(bool preserveLiveCaller)
{
   #ifdef DEBUG_FM_STACK
   MessageInterface::ShowMessage
      ("Entering FM::PushToStack for function '%s'\n", functionName.c_str());
   ShowStackContents(callStack, "Stack contents at beg. of PushToStack");
   ShowObjectMap(functionObjectStore, "FOS at beg. of PushToStack");
   #endif
   
   // A private child leaves the actual caller graph installed. Preserve its
   // identity and execution state; legacy shared stores still need values.
   LocalObjectState saved;
   saved.epoch = std::make_shared<const int>(0);
   saved.preserveLiveCaller = preserveLiveCaller;
   saved.managedStore = functionStoreIsManaged;
   saved.storeIdentity = functionStoreIdentity;
   if (functionObjectStore && !preserveLiveCaller)
   {
      for (const auto &entry : *functionObjectStore)
      {
         GmatBase *object = entry.second;
         if (!object || object->IsGlobal() || object->IsOfType(Gmat::FUNCTION) ||
             object->IsOfType(Gmat::SUBSCRIBER)) continue;
         std::unique_ptr<GmatBase> value(CloneFunctionValue(object));
         if (value.get() == object)
         {
            value.release();
            throw FunctionException("Caller frame Clone returned its source.\n");
         }
         if (!value) throw FunctionException("Cannot capture caller frame value.\n");
         BindCapturedValue(value.get(), object, functionObjectStore, saved.epoch);
         saved.bindingNames[object] = entry.first;
         saved.objects.emplace(entry.first, std::move(value));
      }
   }
   callStack.push(functionObjectStore);
   try { savedLocalObjectStates.push(std::move(saved)); }
   catch (...) { callStack.pop(); throw; }
}

//------------------------------------------------------------------------------
// bool PopFromStack(ObjectMap* cloned, const StringArray &outNames, ...)
//------------------------------------------------------------------------------
/**
 * Pops callstack and sets output to caller's output
 */
//------------------------------------------------------------------------------
bool FunctionManager::PopFromStack(ObjectMap* cloned, const StringArray &outNames,
                                   const StringArray &callingNames)
{
   return PopFromStack(cloned, outNames, callingNames, NULL);
}

bool FunctionManager::PopFromStack(ObjectMap* cloned, const StringArray &outNames,
      const StringArray &callingNames, const OutputObjectValues *values)
{
   #ifdef DEBUG_FM_STACK
   MessageInterface::ShowMessage
      ("Entering FM::PopFromStack for function '%s', cloned=<%p>\n", functionName.c_str(), cloned);
   ShowStackContents(callStack, "Stack contents at beg. of PopFromStack");
   #endif
   if (callStack.empty())
      throw FunctionException(
            "ERROR popping object store from call stack - stack is empty\n");
   if (savedLocalObjectStates.empty())
      throw FunctionException("Missing caller value frame on function return\n");

   const auto &frame = savedLocalObjectStates.top();
   if (frame.preserveLiveCaller)
   {
      // The child has a separate FOS and has already assigned its explicit
      // outputs. Do not Copy untouched caller resources, repeat output writes,
      // replace the installed callee, or reinitialize the caller's live FCS.
      if (callStack.top() != functionObjectStore)
         throw FunctionException("The active caller object store changed during a nested call.\n");
      if (frame.managedStore)
      {
         const auto savedIdentity = frame.storeIdentity.lock();
         const auto registration = FindBuiltinOutputStore(functionObjectStore);
         if (!savedIdentity || registration.second.lock() != savedIdentity ||
               !functionStoreIsManaged || functionStoreIdentity.lock() != savedIdentity)
            throw FunctionException("The active caller object store was retired during a nested call.\n");
      }
      callStack.pop();
      savedLocalObjectStates.pop();
      return true;
   }

   // Preserve callee outputs before restoring an aliased caller map: otherwise
   // restoring 'out' can erase the value that must be copied to 'child'.
   auto findScoped = [this](ObjectMap *local, const std::string &name) -> GmatBase*
   {
      if (local)
      {
         auto found = local->find(name);
         if (found != local->end() && found->second)
            return found->second;
      }
      if (globalObjectStore)
      {
         auto found = globalObjectStore->find(name);
         if (found != globalObjectStore->end())
            return found->second;
      }
      return NULL;
   };
   std::vector<std::shared_ptr<GmatBase> > returnValues;
   std::vector<bool> borrowedReturns;
   for (UnsignedInt i = 0; i < callingNames.size(); ++i)
   {
      GmatBase *value = findScoped(cloned, outNames.at(i));
      if (!value)
         throw FunctionException("Missing function output on stack return\n");
      const bool isGlobalValue = globalObjectStore &&
         globalObjectStore->find(outNames.at(i)) != globalObjectStore->end() &&
         globalObjectStore->at(outNames.at(i)) == value;
      borrowedReturns.push_back(isGlobalValue);
      if (isGlobalValue)
         // Global values are not restored with the local frame; keep their
         // borrowed identity and leave ownership with the global store.
         returnValues.emplace_back(value, [](GmatBase*) {});
      else
         returnValues.push_back(std::shared_ptr<GmatBase>(CloneFunctionValue(value)));
   }

   ObjectMap *topMap = callStack.top();
   callStack.pop();
   LocalObjectState saved = std::move(savedLocalObjectStates.top());
   savedLocalObjectStates.pop();
   BindFunctionStore(topMap);
   ObjectMap restoredTargets;
   for (const auto &entry : saved.objects)
   {
      auto found = functionObjectStore->find(entry.first);
      if (found != functionObjectStore->end() && found->second &&
          found->second->GetTypeName() == entry.second->GetTypeName())
         found->second->Copy(entry.second.get());
      else
      {
         std::unique_ptr<GmatBase> replacement(CloneFunctionValue(entry.second.get()));
         if (replacement.get() == entry.second.get())
         {
            replacement.release();
            throw FunctionException("Caller frame restoration Clone returned its source.\n");
         }
         if (!replacement) throw FunctionException("Cannot restore caller frame value.\n");
         if (found == functionObjectStore->end())
            functionObjectStore->emplace(entry.first, replacement.release());
         else
         {
            DeleteObject(found->second);
            found->second = replacement.release();
         }
      }
      restoredTargets[entry.first] = functionObjectStore->at(entry.first);
   }

   #ifdef DEBUG_FM_STACK
      MessageInterface::ShowMessage("PopFromStack::outNames are: \n");
      for (unsigned int ii=0; ii<outNames.size(); ii++)
         MessageInterface::ShowMessage(" %d)   %s\n", ii, (outNames.at(ii)).c_str());
      MessageInterface::ShowMessage("PopFromStack::callingNames are: \n");
      for (unsigned int ii=0; ii<callingNames.size(); ii++)
         MessageInterface::ShowMessage(" %d)   %s\n", ii, (callingNames.at(ii)).c_str());
      MessageInterface::ShowMessage("PopFromStack::cloned stack entries are: \n");
      ShowObjectMap(cloned, "cloned");
      MessageInterface::ShowMessage("PopFromStack::popped stack entries are: \n");
      ShowObjectMap(functionObjectStore, "FOS");
   #endif
   
   auto assign = [&](UnsignedInt jj)
   {
      if (values)
      {
         const auto frozen = values->wrappers.find(outNames.at(jj));
         if (frozen != values->wrappers.end())
         {
            // Recreate the caller's typed LHS after restoration. Copying a
            // Variable proxy into a system Parameter is not a value setter.
            combinedObjectStore = *globalObjectStore;
            for (const auto &entry : *functionObjectStore)
               combinedObjectStore[entry.first] = entry.second;
            validator->SetFunction(currentFunction);
            validator->SetObjectMap(&combinedObjectStore);
            validator->SetSolarSystem(solarSys);
            std::unique_ptr<ElementWrapper> target(
                  validator->CreateElementWrapper(callingNames.at(jj)));
            if (target) target->AllowOneDimArraySetting(frozen->second->IsOneDimArraySettingAllowed());
            if (!target || !ElementWrapper::SetValue(target.get(), frozen->second.get(),
                  solarSys, functionObjectStore, globalObjectStore))
               throw FunctionException("Cannot assign captured caller output " + callingNames.at(jj));
            return;
         }
      }

      // Find output name from cloned object store
      // If name found then assign to object pointer since [] operator
      // inserts the item if it doesn't exist (LOJ: 2010.05.19)
      GmatBase *clonedObj = NULL;
      clonedObj = returnValues.at(jj).get();
      
      // Find output name from popped object store
      // If name found then assign to object pointer since [] operator
      // inserts the item if it doesn't exist (LOJ: 2010.05.19)
      GmatBase *fosObj = findScoped(functionObjectStore, callingNames.at(jj));
      
      if (clonedObj == NULL)
      {
         std::string errMsg = "PopFromStack::Error getting output named ";
         errMsg += outNames.at(jj) + " from cloned map in function \"";
         errMsg += functionName + "\"\n";
         throw FunctionException(errMsg);
      }
      if (fosObj == NULL && callingNames.at(jj).find('(') != std::string::npos)
      {
         // AssignResult wrote through the caller's element wrapper before
         // restoring its saved frame. Reapply the scalar after restoration,
         // rather than silently dropping an indexed name absent from the map.
         const std::string targetName = callingNames.at(jj);
         const std::string ownerName = targetName.substr(0, targetName.find('('));
         GmatBase *owner = findScoped(functionObjectStore, ownerName);
         if (!owner || !owner->IsOfType(Gmat::ARRAY))
            throw FunctionException("Missing caller Array for output " + targetName);
         Real result;
         if (clonedObj->IsOfType(Gmat::VARIABLE))
            result = clonedObj->GetRealParameter("Value");
         else if (clonedObj->IsOfType(Gmat::ARRAY) &&
                  clonedObj->GetIntegerParameter("NumRows") == 1 &&
                  clonedObj->GetIntegerParameter("NumCols") == 1)
            result = clonedObj->GetRealParameter("SingleValue", 0, 0);
         else
            throw FunctionException("Array-element output requires a scalar: " + targetName);
         combinedObjectStore = *globalObjectStore;
         for (const auto &entry : *functionObjectStore)
            combinedObjectStore[entry.first] = entry.second;
         validator->SetFunction(currentFunction);
         validator->SetObjectMap(&combinedObjectStore);
         validator->SetSolarSystem(solarSys);
         std::unique_ptr<ElementWrapper> target(validator->CreateElementWrapper(targetName));
         if (!target || target->GetWrapperType() != Gmat::ARRAY_ELEMENT_WT ||
             !target->SetRefObject(owner) || !target->SetReal(result))
            throw FunctionException("Cannot assign caller Array-element output " + targetName);
      }
      else if (fosObj == NULL)
      {
         // We don't want to throw exception here (loj: 2008.09.12)
         //std::string errMsg = "PopFromStack::Error setting output named \"";
         //errMsg += callingNames.at(jj) + "\" from nested function on function \"";
         //errMsg += functionName + "\"\n";
         ////throw FunctionException(errMsg);
         #ifdef DEBUG_FM_STACK
         MessageInterface::ShowMessage
            ("PopFromStack:: Found blank output name, realResult=%f\n", realResult);
         #endif
      }
      else
      {
         #ifdef DEBUG_FM_STACK
            MessageInterface::ShowMessage(
               "PopFromStack::Copying value of %s into popped stack element %s\n",
               (outNames.at(jj)).c_str(), (callingNames.at(jj)).c_str());
         #endif
         if (fosObj != clonedObj)
            fosObj->Copy(clonedObj);
      }
   };
   std::vector<UnsignedInt> ephemerides;
   for (UnsignedInt i = 0; i < callingNames.size(); ++i)
      if (returnValues[i] && returnValues[i]->IsOfType("EphemerisFile"))
         ephemerides.push_back(i);
      else assign(i);

   // Returning an ODEModel/spacecraft can replace native components after
   // the saved values were copied. Connect untouched saved roots only after
   // those output Copies, so they cannot borrow the just-retired components.
   RebindCapturedObjects(saved, restoredTargets);

   std::map<const GmatBase*, GmatBase*> bindings;
   if (values)
   {
      bindings = values->bindings;
      for (const auto &entry : values->bindingNames)
      {
         GmatBase *caller = findScoped(functionObjectStore, entry.second);
         if (!caller && solarSys)
         {
            caller = solarSys->GetBody(entry.second);
            if (!caller) caller = solarSys->GetSpecialPoint(entry.second);
         }
         if (!caller) throw FunctionException("Missing caller resource after stack restoration: " + entry.second);
         bindings[entry.first] = caller;
      }
      for (UnsignedInt i = 0; i < callingNames.size(); ++i)
         if (returnValues[i] && !returnValues[i]->IsOfType("EphemerisFile") && !borrowedReturns[i])
         {
            GmatBase *caller = findScoped(functionObjectStore, callingNames[i]);
            if (caller)
               if (const auto context = caller->GetFunctionValueContext())
                  context->RebindCaller(*caller, values->epoch, bindings);
         }
   }
   for (UnsignedInt i : ephemerides)
   {
      if (values && !borrowedReturns[i])
         if (const auto context = returnValues[i]->GetFunctionValueContext())
            context->RebindCaller(*returnValues[i], values->epoch, bindings);
      assign(i);
   }

   #ifdef DEBUG_FM_STACK
      MessageInterface::ShowMessage(
            "PopFromStack::reset object map to the one popped from the stack\n");
      MessageInterface::ShowMessage(
            "PopFromStack::now about to re-initialize the function '%s'\n",
            functionName.c_str());
   #endif
   
   // Set popped FOS to function and re-initialize fcs
   currentFunction->SetFunctionObjectMap(functionObjectStore);
   BindFunctionServices();
   for (const auto &entry : inputWrapperMap)
      currentFunction->SetInputElementWrapper(entry.first, entry.second);
   bool retval = currentFunction->Initialize(objInit, false, false);
   
   #ifdef DEBUG_FM_STACK
      MessageInterface::ShowMessage(
         "Exiting  FM::PopFromStack for function %s with %d\n", functionName.c_str(), retval);
      ShowStackContents(callStack, "Stack contents at end of PopFromStack");
   #endif

   return retval;
}


//------------------------------------------------------------------------------
// void Cleanup()
//------------------------------------------------------------------------------
void FunctionManager::Cleanup()
{
   auto context = Validator::Instance()->CaptureContext(activeManagerExecutions != 0);
   ForgetRetiredFunctionStore();
   if (reentrantInvocation && reentrantInvocation->executing)
   {
      finalizationRequested = true;
      return;
   }
   if (isFinalized && !reentrantInvocation && !completedInvocation && stagedWrapperDeletions.empty())
      return;
   // RunComplete on the FCS can revisit this manager through recursive calls.
   // Retire the invocation before finalizing commands, not after they return.
   isFinalized = true;
   finalizationRequested = false;
   firstExecution = true;
   struct CleanupState
   {
      bool &finalized;
      bool completed = false;
      ~CleanupState() { if (!completed) finalized = false; }
   } cleanupState{isFinalized};
   // Executing calls already own an enclosing subscriber frame. An inactive
   // cached manager can also finalize while another function is running.
   std::unique_ptr<SharedSubscriberBindings> subscriberBindings;
   if (!executing)
      subscriberBindings = std::make_unique<SharedSubscriberBindings>(localObjectStore, globalObjectStore);
   // Reentrant command retirement can detach this manager's copied tables.
   // The callee and its wrappers may still borrow their storage until the
   // remainder of this Cleanup has retired the FOS and argument interface.
   const auto retiringBindings = expressionBindings;
   // Result destruction can itself release function/command resources. Keep
   // the re-entry guard in place and retire snapshots while dependencies live.
   objectResult = NULL;
   outputType.clear();
   auto retiringResult = std::move(ownedObjectResult);
   ReleaseStagedWrappers();
   std::exception_ptr cleanupFailure;
   if (reentrantInvocation)
   {
      auto nested = std::move(reentrantInvocation);
      reentrantPrepared = false;
      try { nested->Finalize(); }
      catch (...) { cleanupFailure = std::current_exception(); }
   }
   if (completedInvocation)
   {
      auto completed = std::move(completedInvocation);
      try { completed->Finalize(); }
      catch (...) { if (!cleanupFailure) cleanupFailure = std::current_exception(); }
   }

   #ifdef DEBUG_CLEANUP
   MessageInterface::ShowMessage
      ("==> FunctionManager::Cleanup() entered, currentFunction=<%p>'%s'\n",
       currentFunction, HasLiveFunction() ? currentFunction->GetName().c_str() : "NULL");
   #endif

   if (HasInstalledInvocation())
   {
      if (functionObjectStore || !clonedObjectStores.empty())
      {
         ////Edwin's MMS script failed, so commented out (loj: 2008.12.03)
         ////We need to delete this somewhere though.
         ////currentFunction->ClearAutomaticObjects();
         // They are deleted in the Function destructor (LOJ: 2014.12.09)
         #ifdef DEBUG_CLEANUP
         MessageInterface::ShowMessage("   Calling currentFunction->Finalize(true)\n");
         #endif
         
         // Set true for cleanup
         try {currentFunction->Finalize(true);}
         catch (...) {if (!cleanupFailure) cleanupFailure=std::current_exception();}
      }
   }
   
   //// now delete all of the items/entries in the FOS - we can do this since they 
   //// are all either locally-created or clones of reference objects or automatic objects
   //if (functionObjectStore)
   //{
   //   #ifdef DEBUG_CLEANUP
   //   MessageInterface::ShowMessage
   //      ("   Deleting functionObjectStore <%p>\n", functionObjectStore);
   //   #endif
   //   DeleteObjectMap(functionObjectStore, "FOS in Finalize");
   //   functionObjectStore = NULL;
   //}
   //
   //Integer numClones = clonedObjectStores.size();
   //for (Integer i=0; i<numClones; i++)
   //{
   //   #ifdef DEBUG_CLEANUP
   //   MessageInterface::ShowMessage
   //      ("   Deleting clonedObjectStores[%d] <%p>\n", i, clonedObjectStores[i]);
   //   #endif
   //   DeleteObjectMap(clonedObjectStores[i], "clonedOS in Finalize");
   //   clonedObjectStores[i] = NULL;
   //}
   //clonedObjectStores.clear();
   

   // RunComplete resets execution state, but compiled definitions must also
   // stop borrowing scopes before their objects are deleted. Do this even if
   // Finalize reported an error, and include owned recursive store snapshots.
   auto retireDefinition = [&](ObjectMap *store)
   {
      if (!store || !HasLiveFunction()) return;
      if (auto *function = dynamic_cast<UserDefinedFunction*>(currentFunction))
      {
         try { function->RetireFunctionBindings(store); }
         catch (...) { if (!cleanupFailure) cleanupFailure = std::current_exception(); }
         try { function->RetireAutomaticReferences(store); }
         catch (...) { if (!cleanupFailure) cleanupFailure = std::current_exception(); }
      }
   };
   if (isObjStoreOwner) retireDefinition(functionObjectStore);
   for (auto *store : clonedObjectStores) retireDefinition(store);

   // Drop value-only snapshots on exceptional unwinding as well as normal
   // PopFromStack. Their ownership is separate from the live object maps.
   while (!savedLocalObjectStates.empty())
      savedLocalObjectStates.pop();
   while (!callStack.empty())
      callStack.pop();
   while (!losStack.empty())
      losStack.pop();
   while (!callers.empty())
      callers.pop();
   callingFunction = NULL;

   ObjectArray toDeleteObjects = builtinOutputResources ? builtinOutputResources->LiveObjects() : ObjectArray{};
   ClearInOutWrappers();
   pendingInputValues.clear();
   retiringResult.reset();
   if (functionObjectStore && isObjStoreOwner)
   {
      if (HasLiveFunction() &&
          currentFunction->GetFunctionObjectStore() == functionObjectStore)
         currentFunction->RetireFunctionObjectStore(functionObjectStore);
      for (ObjectMap::iterator i = (*functionObjectStore).begin(); i != (*functionObjectStore).end(); ++i)
      {
         if (find(toDeleteObjects.begin(), toDeleteObjects.end(), (*i).second) ==
            toDeleteObjects.end())
            toDeleteObjects.push_back((*i).second);
      }
      UnregisterBuiltinOutputStore(functionObjectStore, builtinOutputResources);
      functionObjectStore->clear();
      delete functionObjectStore;
      functionObjectStore = NULL;
   }
   else
      functionObjectStore = NULL;

   Integer numClones = clonedObjectStores.size();
   for (Integer i = 0; i < numClones; i++)
   {
      if (clonedObjectStores[i])
      {
         for (ObjectMap::iterator k = (*clonedObjectStores[i]).begin(); k != (*clonedObjectStores[i]).end(); ++k)
         {
            if (find(toDeleteObjects.begin(), toDeleteObjects.end(), (*k).second) == 
               toDeleteObjects.end())
               toDeleteObjects.push_back((*k).second);
         }

         if (HasLiveFunction() && currentFunction->GetFunctionObjectStore() == clonedObjectStores[i])
            currentFunction->RetireFunctionObjectStore(clonedObjectStores[i]);
         UnregisterBuiltinOutputStore(clonedObjectStores[i], builtinOutputResources);
         clonedObjectStores[i]->clear();
         delete clonedObjectStores[i];
         clonedObjectStores[i] = NULL;
      }
   }
   clonedObjectStores.clear();

   for (ObjectMap::iterator i = createdLiterals.begin(); i != createdLiterals.end(); ++i)
   {
      if (find(toDeleteObjects.begin(), toDeleteObjects.end(), (*i).second) == 
         toDeleteObjects.end())
         toDeleteObjects.push_back((*i).second);
   }
   createdLiterals.clear();

   for (ObjectMap::iterator i = createdOthers.begin(); i != createdOthers.end(); ++i)
   {
      if (find(toDeleteObjects.begin(), toDeleteObjects.end(), (*i).second) == 
         toDeleteObjects.end())
         toDeleteObjects.push_back((*i).second);
   }
   createdOthers.clear();

   // Final metadata can access spacecraft/frame resources. Retire every
   // owned subscriber before deleting those resources, including on error.
   for (auto &obj : toDeleteObjects)
   {
      if (OwnsFunctionObject(obj) && obj->IsOfType(Gmat::SUBSCRIBER))
      {
         const auto failure=FinalizeFunctionSubscriber(static_cast<Subscriber *>(obj),publisher);
         if (!cleanupFailure) cleanupFailure=failure;
         delete obj;
         obj=NULL;
      }
   }
   for (auto *obj : toDeleteObjects) DeleteObject(obj);
   toDeleteObjects.clear();
   if (builtinOutputResources) builtinOutputResources->retired.clear();
   builtinOutputResources.reset();
   functionStoreOwner.reset();
   functionStoreIdentity.reset();
   functionStoreIsManaged = false;
   isObjStoreOwner = false;
   combinedObjectStore.clear();
   ClearInOutWrappers();
   objectResult = NULL;
   outputType.clear();
   blankResult = false;
   // Retiring a value can finalize cached functions. Complete that work before
   // releasing the surrounding shared-subscriber binding frame.
   retiringResult.reset();
   cleanupState.completed = true;
   if (subscriberBindings && !subscriberBindings->Restore() && !cleanupFailure)
      cleanupFailure = std::make_exception_ptr(FunctionException("Unable to restore subscriber bindings after cleanup.\n"));
   if (cleanupFailure) std::rethrow_exception(cleanupFailure);

   #ifdef DEBUG_CLEANUP
   MessageInterface::ShowMessage("==> FunctionManager::Cleanup() exiting\n");
   #endif
}

//------------------------------------------------------------------------------
// void DeleteObject(GmatBase *obj)
//------------------------------------------------------------------------------
/**
 * Used to handle deletion of local or global objects from the GmatFunction
 * including special cases
 *
 * param <obj> The object to be deleted
 */
 //------------------------------------------------------------------------------
void FunctionManager::DeleteObject(GmatBase *obj)
{
   if (!OwnsFunctionObject(obj)) return;
   if (obj->IsOfType(Gmat::SUBSCRIBER))
   {
      const auto failure=FinalizeFunctionSubscriber(static_cast<Subscriber *>(obj),publisher);
      // This single-object route may still have an owning map entry. On
      // failure retain the object for its owner's cleanup, already unsubscribed.
      if (failure) std::rethrow_exception(failure);
   }
   delete obj;
}


//------------------------------------------------------------------------------
// void UnsubscribeSubscribers(ObjectMap *om)
//------------------------------------------------------------------------------
/**
 * Unsubscribes local subscribers. Assumes input object map contains local
 * objects.
 */
//------------------------------------------------------------------------------
void FunctionManager::UnsubscribeSubscribers(ObjectMap *om)
{
   if (!om) return;
   std::exception_ptr firstFailure;
   bool localPlotClosed=false;
   std::set<Subscriber *> visited;
   for (const auto &entry:*om)
   {
      auto *object=entry.second;
      // True globals can be visible in a function without belonging to it.
      if (!OwnsFunctionObject(object) || !object->IsOfType(Gmat::SUBSCRIBER)) continue;
      auto *subscriber=static_cast<Subscriber *>(object);
      if (!visited.insert(subscriber).second) continue;
      const auto failure=FinalizeFunctionSubscriber(subscriber,publisher);
      if (!firstFailure) firstFailure=failure;
      if (subscriber->IsOfType("Plot"))
      {
         try
         {
            MessageInterface::ShowMessage("*** WARNING *** The local %s named '%s' was closed "
                  "upon exit of the function '%s'.\n",subscriber->GetTypeName().c_str(),
                  subscriber->GetName().c_str(),functionName.c_str());
            localPlotClosed=true;
         }
         catch (...) {if (!firstFailure) firstFailure=std::current_exception();}
      }
   }
   if (localPlotClosed)
   {
      try {MessageInterface::ShowMessage("*** Please make plots and drawing objects Global in the main script and "
            "in the function to make plots remain open during the mission run.\n");}
      catch (...) {if (!firstFailure) firstFailure=std::current_exception();}
   }
   if (firstFailure) std::rethrow_exception(firstFailure);
}


//------------------------------------------------------------------------------
// bool EmptyObjectMap(ObjectMap *om, const std::string &mapID)
//------------------------------------------------------------------------------
/**
 * Deletes local object map.
 */
//------------------------------------------------------------------------------
bool FunctionManager::EmptyObjectMap(ObjectMap *om, const std::string &mapID)
{
   #ifdef DEBUG_OBJECT_MAP
   MessageInterface::ShowMessage
      ("FM::EmptyObjectMap() entered, om = <%p>, mapID = '%s'\n", om, mapID.c_str());
   #endif
   if (om == NULL)
   {
      #ifdef DEBUG_OBJECT_MAP
      MessageInterface::ShowMessage
         ("in FM::EmptyObjectMap(), object map '%s' is NULL\n", mapID.c_str());
      #endif
      return true;
   }

   #ifdef DEBUG_CLEANUP
   MessageInterface::ShowMessage
      ("in FM::EmptyObjectMap(), object map '%s' <%p> had %u objects\n", mapID.c_str(),
         om, om->size());
   #endif

   StringArray toDelete;
   std::map<std::string, GmatBase *>::iterator omi;

   // Delete subscribers first since CCSDS EphemerisFile needs to access spacecraft for
   // writing final metadata (LOJ: 2015.07.07)
   for (omi = om->begin(); omi != om->end(); ++omi)
   {
      GmatBase* obj = omi->second;
      if (obj != NULL)
      {
         // Only delete object when it is not global and it is local
         if (!(obj->IsGlobal()) || obj->IsLocal())
         {
            // Finalize and unsubcribe before delete a subscriber
            if (obj->IsOfType(Gmat::SUBSCRIBER))
            {
               //=============================================================
               #ifdef __DO_NOT_DELETE_SUBSCRIBERS__
               //=============================================================
               GmatBase *obj = omi->second;

               #ifdef DEBUG_CLEANUP
               MessageInterface::ShowMessage
               ("   The subscriber <%p>'%s' will remain in the Publisher and will "
                  "not be deleted.\n", functionName.c_str(), obj, obj->GetName().c_str());
               #endif

               // for now, don't delete subscribers as the Publisher still points to them and
               // bad things happen at the end of the run if they disappear
               #ifdef DEBUG_MEMORY
               MessageInterface::ShowMessage
               ("*** FunctionManager::EmptyObjectMap() should delete <%p> <%s> '%s'\n",
                  obj, obj->GetTypeName().c_str(), obj->GetName().c_str());
               MessageInterface::ShowMessage
               ("*** For now, don't delete subscribers as the Publisher still points "
                  "to them and bad things happen at the end of the run if they disappear.\n");
               #endif

               //=============================================================
               #else
               //=============================================================

               // Unsubscribe subscriber before deleting (LOJ: 2009.04.07)
               Subscriber *sub = (Subscriber*)omi->second;

               // Catch any error encountered during subscriber finalization and
               // rethrow if it is fatal else just write error message (Fix for GMT-5204)
               try
               {
                  // Instead of deleting OpenGL plot from the OpenGlPlot destructor
                  // call TakeAction() to delete it (LOJ:2009.04.22)
                  sub->TakeAction("Finalize");
                  //@todo This causes Func_AssigningWholeObjects crash
                  // in Subscriber::ClearWrappers() due to stale wrapper pointers
                  //sub->ClearWrappers();
               }
               catch (BaseException &be)
               {
                  if (be.IsFatal())
                     throw;
                  else
                     MessageInterface::ShowMessage("%s\n", be.GetFullMessage().c_str());
               }

               #ifdef DEBUG_CLEANUP
               MessageInterface::ShowMessage
               ("   '%s' Unsubscribe <%p>'%s' from the publisher <%p>\n",
                  functionName.c_str(), sub, sub->GetName().c_str(), publisher);
               #endif

               if (publisher)
               {
                  publisher->Unsubscribe(sub);
               }
               else
               {
                  #ifdef DEBUG_CLEANUP
                  MessageInterface::ShowMessage
                  ("   '%s' Cannot unsubscribe, the publisher is NULL\n", functionName.c_str());
                  #endif
               }
               //=============================================================
               #endif
               //=============================================================
            }

            // delete the object
            delete omi->second;
            omi->second = NULL;
         }
      }
   }


//   GmatBase *mapObj = NULL;
//   // Delete the rest of objects (LOJ: 2015.07.07)
//   for (omi = om->begin(); omi != om->end(); ++omi)
//   {
//      mapObj = omi->second;
//
//      if (mapObj && (mapObj->IsOfType(Gmat::GROUND_STATION)))
//         continue;
//
//      if (omi->second != NULL)
//      {
//         if (!(omi->second)->IsOfType(Gmat::SUBSCRIBER))
//         {
//            // Do not delete if object is a global and not created inside a function
//            if (mapObj->IsGlobal() && !(mapObj->IsLocal()))
//            {
//#ifdef DEBUG_CLEANUP
//               MessageInterface::ShowMessage
//               ("   '%s' is global and not local, so skip deleting\n",
//                  mapObj->GetName().c_str());
//#endif
//            }
//            else
//            {
//#ifdef DEBUG_MEMORY
//               MemoryTracker::Instance()->Remove
//               (omi->second, (omi->second)->GetName(), "FunctionManager::EmptyObjectMap()",
//                  "deleting obj from ObjectMap");
//#endif
//#ifdef DEBUG_CLEANUP
//               MessageInterface::ShowMessage
//               ("   Deleting <%p>'%s' IsGlobal:%d IsLocal:%dfrom om\n", omi->second,
//                  (omi->first).c_str(), (omi->second)->IsGlobal(), (omi->second)->IsLocal());
//#endif
//               delete omi->second;
//               omi->second = NULL;
//            }
//         }
//      }
//
//      if (omi->second != NULL)
//      {
//         if (mapObj->IsGlobal() && !(mapObj->IsLocal()))
//         {
//#ifdef DEBUG_CLEANUP
//            MessageInterface::ShowMessage
//            ("   '%s' is global and not local, so skip deleting\n",
//               mapObj->GetName().c_str());
//#endif
//         }
//         else
//         {
//#ifdef DEBUG_CLEANUP
//            MessageInterface::ShowMessage("   Adding <%p>'%s' to toDelete\n", omi->second, (omi->first).c_str());
//#endif
//            toDelete.push_back(omi->first);
//         }
//      }
//   }
//#ifdef DEBUG_CLEANUP
//   MessageInterface::ShowMessage("   Before deleting from toDelete.size() = %d\n", toDelete.size());
//#endif
//   for (unsigned int kk = 0; kk < toDelete.size(); kk++)
//   {
//      omi = om->find(toDelete.at(kk));
//#ifdef DEBUG_CLEANUP
//      MessageInterface::ShowMessage
//      ("   Erasing <%p> object with name '%s'\n", omi->second, (omi->first).c_str());
//      //   ("   Erasing <%p> object with name '%s'\n", omi->second, (toDelete.at(kk)).c_str());
//#endif
////om->erase(toDelete.at(kk));
//      delete omi->second;
//   }
//#ifdef DEBUG_CLEANUP
//   MessageInterface::ShowMessage("   After deleting from toDelete.size() = %d\n", toDelete.size());
//#endif


   // empty om
   om->clear();

#ifdef DEBUG_OBJECT_MAP
   MessageInterface::ShowMessage
   ("FM::EmptyObjectMap() returning true, om = <%p>, mapID = '%s'\n", om, mapID.c_str());
#endif
   return true;
}


//------------------------------------------------------------------------------
// bool DeleteObjectMap(ObjectMap *om, const std::string &mapID)
//------------------------------------------------------------------------------
bool FunctionManager::DeleteObjectMap(ObjectMap *om, const std::string &mapID)
{
   if (om == NULL)
   {
      #ifdef DEBUG_CLEANUP
      MessageInterface::ShowMessage
         ("in FM::DeleteObjectMap(), object map %s is NULL\n", mapID.c_str());
      #endif
      return true;
   }
   
   #ifdef DEBUG_CLEANUP
   MessageInterface::ShowMessage
      ("in FM::DeleteObjectMap(), object map %s <%p> had %u objects\n", mapID.c_str(),
       om, om->size());
   #endif

   // Contents can alias another invocation. Keep the actual owned objects in
   // their registered owner's retirement pool instead of deleting them here
   // or losing the last record when this old dictionary is discarded.
   const auto owner = FindBuiltinOutputStoreOwner(om);
   if (owner && owner == builtinOutputResources)
      for (const auto &entry : *om) owner->Retain(entry.second);
   if (HasLiveFunction())
   {
      if (auto *function = dynamic_cast<UserDefinedFunction*>(currentFunction))
         function->RetireFunctionBindings(om);
      currentFunction->RetireFunctionObjectStore(om);
   }
   UnregisterBuiltinOutputStore(om, builtinOutputResources);

   // Crash source for GMT-7836 / GMT-7792
//   EmptyObjectMap(om, mapID);
   
   #ifdef DEBUG_MEMORY
   MemoryTracker::Instance()->Remove
      (om, "om", "FunctionManager::DeleteObjectMap()", "deleting ObjectMap");
   #endif
   delete om;
   om = NULL;
   return true;
}

//------------------------------------------------------------------------------
// bool ClearInOutWrappers()
//------------------------------------------------------------------------------
bool FunctionManager::ClearInOutWrappers()
{
   if (HasInstalledInvocation()) currentFunction->RetireArgumentWrappers(inputWrapperMap, outputWrappers);
   auto release = [&](ElementWrapper *wrapper)
   {
      if (!wrapper) return;
      for (auto &entry : inputWrapperMap) if (entry.second == wrapper) entry.second = NULL;
      for (auto &entry : outputWrappers) if (entry == wrapper) entry = NULL;
      ElementWrapper::ReleaseOwnedWrapper(wrapper);
   };
   for (auto &entry : inputWrapperMap) release(entry.second);
   for (auto *entry : outputWrappers) release(entry);
   inputWrapperMap.clear();
   outputWrappers.clear();
   return true;
}


//------------------------------------------------------------------------------
// bool IsOnStack(ObjectMap *om)
//------------------------------------------------------------------------------
bool FunctionManager::IsOnStack(ObjectMap *om)
{
   if (callStack.empty())  return false;

   ObjectMap *tmp = callStack.top();
   //if (&tmp == &om) return true;  // is om the top element on the LIFO stack?
   if (tmp == om) return true;  // is om the top element on the LIFO stack?
   else           return false;
}

//------------------------------------------------------------------------------
// bool CloneObjectMap(ObjectMap *orig, ObjectMap *cloned)
//------------------------------------------------------------------------------
bool FunctionManager::CloneObjectMap(ObjectMap *orig, ObjectMap *cloned)
{
   if (orig == NULL || cloned == NULL)
      return false;

   ObjectMap::iterator omi;
   GmatBase *objInMap;
   std::string strInMap;
   
   cloned->clear();
   for (omi = orig->begin(); omi != orig->end(); ++omi)
   {
      strInMap = omi->first;
      objInMap = omi->second;
      
      // We don't want to insert blank name, so just commented out (loj: 2008.09.22)
      if (strInMap == "")
      {
         #ifdef DEBUG_FM_STACK
         MessageInterface::ShowMessage("PushToStack:: Found blank output name\n");
         #endif
         continue;
         //throw FunctionException
         //   ("PushToStack:: Cannot insert blank object name to object map\n");
      }
      
      #ifdef DEBUG_FM_STACK
         MessageInterface::ShowMessage(
            "PushToStack::Inserting clone of object \"%s\" into cloned object map\n",
            strInMap.c_str());
      #endif
      
      GmatBase *clonedObj = CloneFunctionValue(objInMap);
      #ifdef DEBUG_MEMORY
      MemoryTracker::Instance()->Add
         (clonedObj, strInMap, "FunctionManager::CloneObjectMap()",
          "clonedObj = objInMap->Clone()");
      #endif
      cloned->insert(std::make_pair(strInMap, clonedObj));
   }
   
   return true;
}

//------------------------------------------------------------------------------
// bool CopyObjectMap(ObjectMap *from, ObjectMap *to)
//------------------------------------------------------------------------------
bool FunctionManager::CopyObjectMap(ObjectMap *from, ObjectMap *to)
{
   if ((from == NULL) || (to == NULL))
      return false;
   
   ObjectMap::iterator omi;   
   to->clear();
   for (omi = from->begin(); omi != from->end(); ++omi)
      to->insert(std::make_pair(omi->first, omi->second));
   
   return true;
}

//------------------------------------------------------------------------------
// void ShowObjectMap(ObjectMap *om, const std::string &mapID)
//------------------------------------------------------------------------------
void FunctionManager::ShowObjectMap(ObjectMap *om, const std::string &mapID)
{
   #ifdef DEBUG_OBJECT_MAP
   
   std::string itsID = mapID;
   if (itsID == "") itsID = "unknown name";
   if (om)
   {
      MessageInterface::ShowMessage
         ("Object Map <%p> '%s' contains %u objects:\n", om, itsID.c_str(), om->size());
      if (om->size() > 0)
      {
         GmatBase *obj = NULL;
         GmatBase *paramOwner = NULL;
         std::string objName;
         std::string isGlobal;
         std::string isLocal;
         std::string paramOwnerType = "";
         std::string paramOwnerName = "";
         for (std::map<std::string, GmatBase *>::iterator i = om->begin();
              i != om->end(); ++i)
         {
            obj = i->second;
            objName = i->first;
            isGlobal = "No";
            isLocal = "No";
            if (obj)
            {
               if (obj->IsGlobal())
                  isGlobal = "Yes";
               if (obj->IsLocal())
                  isLocal = "Yes";
               if (obj->IsOfType(Gmat::PARAMETER))
               {
                  paramOwner = ((Parameter*)obj)->GetOwner();
                  if (paramOwner)
                  {
                     paramOwnerType = paramOwner->GetTypeName();
                     paramOwnerName = paramOwner->GetName();
                  }
               }
            }
            MessageInterface::ShowMessage
               ("   %50s  <%p>  %-16s   IsGlobal:%-3s  IsLocal:%-3s", objName.c_str(), obj,
                obj == NULL ? "NULL" : (obj)->GetTypeName().c_str(), isGlobal.c_str(),
                isLocal.c_str());
            if (paramOwner)
            {
               MessageInterface::ShowMessage
                  ("  ParameterOwner: <%p>[%s]'%s'\n", paramOwner, 
                   (paramOwner == NULL? "NULL": paramOwnerType.c_str()), 
                   paramOwnerName.c_str());
            }
            else
            {
               MessageInterface::ShowMessage("\n");
            }
         }
      }
   }
   else
      MessageInterface::ShowMessage("Object Map '%s' is NULL\n", itsID.c_str());

   #endif
}

//------------------------------------------------------------------------------
// void ShowStackContents(ObjectMapStack omStack, const std::string &stackID)
//------------------------------------------------------------------------------
void FunctionManager::ShowStackContents(ObjectMapStack omStack, const std::string &stackID)
{
   MessageInterface::ShowMessage("Showing stack contents: %s\n", stackID.c_str());
   ObjectMapStack temp;
   ObjectMap      *om;
   Integer        idx = 0;
   std::string    mapID;
   while (!(omStack.empty()))
   {
      mapID = GmatStringUtil::ToString(idx, 3);
      om = omStack.top();
      ShowObjectMap(om, mapID);
      temp.push(om);
      omStack.pop();
      idx++;
      mapID = "";
   }
   // put them back on the stack again
   while (!(temp.empty()))
   {
      om = temp.top();
      omStack.push(om);
      temp.pop();
   }
}

//------------------------------------------------------------------------------
// void ShowCallers(const std::string &label)
//------------------------------------------------------------------------------
/*
 * Shows contents of callers
 */
//------------------------------------------------------------------------------
void FunctionManager::ShowCallers(const std::string &label)
{
   std::stack<FunctionManager*> temp;
   FunctionManager *fm;
   Integer idx = 0;
   
   MessageInterface::ShowMessage("==================== %s\n", label.c_str());
   MessageInterface::ShowMessage
      ("   Call stack for this=<%p>'%s', there are %d FunctionManagers in stack\n",
       this, functionName.c_str(), callers.size());
   
   while (!(callers.empty()))
   {
      fm = callers.top();
      MessageInterface::ShowMessage
         ("   caller %d, FM = <%p> '%s'\n", idx, fm,
          fm ? fm->GetFunctionName().c_str() : "NULL");
      temp.push(fm);
      callers.pop();
      idx++;
   }
   
   // put them back on the stack again
   while (!(temp.empty()))
   {
      fm = temp.top();
      callers.push(fm);
      temp.pop();
   }
   MessageInterface::ShowMessage("===============================\n");
}


//------------------------------------------------------------------------------
// void ShowTrace(Integer count, Integer t1, const std::string &label = "",
//                bool showMemoryTracks = false, bool addEol = false)
//------------------------------------------------------------------------------
void FunctionManager::ShowTrace(Integer count, Integer t1, const std::string &label,
                                bool showMemoryTracks, bool addEol)
{
   // To locally control debug output
   bool showTrace = true;
   bool showTracks = true;
   
   showTracks = showTracks & showMemoryTracks;
   
   if (showTrace)
   {
      #ifdef DEBUG_TRACE
      clock_t t2 = clock();
      MessageInterface::ShowMessage
         (">>>>> CALL TRACE: %s, <%p> '%s' Count = %d, elapsed time: %f sec\n", label.c_str(),
          this, functionName.c_str(), count, (Real)(t2-t1)/CLOCKS_PER_SEC);
      #endif
   }
   
   if (showTracks)
   {
      #ifdef DEBUG_MEMORY
      StringArray tracks = MemoryTracker::Instance()->GetTracks(false, false);
      if (showTrace)
         MessageInterface::ShowMessage
            ("    ==> There are %d memory tracks\n", tracks.size());
      else
         MessageInterface::ShowMessage
            (">>>>> MEMORY TRACK: There are %d memory tracks when %s, '%s'\n",
             tracks.size(), label.c_str(), functionName.c_str());
      
      if (addEol)
         MessageInterface::ShowMessage("\n");
      #endif
   }
}


void FunctionManager::SetExpressionFunctionName(const std::string &name)
{
   if (executing) { ReentrantInvocation().SetExpressionFunctionName(name); return; }
   PrepareCallSetup();
   if (name == functionName) return;
   std::string replacement(name);
   SetExpressionEnvironment(nullptr, localObjectStore, globalObjectStore);
   functionName.swap(replacement);
}

void FunctionManager::SetExpressionEnvironment(Function *function,
      ObjectMap *local, ObjectMap *global)
{
   if (executing) { ReentrantInvocation().SetExpressionEnvironment(function, local, global); return; }
   auto context = Validator::Instance()->CaptureContext(activeManagerExecutions != 0);
   PrepareCallSetup();
   auto *callee = dynamic_cast<ObjectManagedFunction*>(function);
   if (function && !callee)
      throw FunctionException("Expression calls require an ObjectManagedFunction.\n");

   auto *userFunction = dynamic_cast<UserDefinedFunction*>(callee);
   GmatCommand *sequence = userFunction ? userFunction->GetFunctionControlSequence() : nullptr;
   if (callee != currentFunction || (callee && !HasLiveFunction()))
   {
      if (!callers.empty() || !callStack.empty() || !losStack.empty())
         throw FunctionException("Cannot replace the callee of an executing expression.\n");
      // Incoming caller tables must survive retirement of this manager's FOS.
      if (isObjStoreOwner && functionObjectStore &&
            (local == functionObjectStore || global == functionObjectStore))
         throw FunctionException("Cannot retire an expression's caller scope while rebinding it.\n");
      for (auto *store : clonedObjectStores)
         if (store && (local == store || global == store))
            throw FunctionException("Cannot retire an expression's stacked caller scope while rebinding it.\n");

      // Use the native definition assignment to retire wrappers/results/FOS
      // while the old callee is still installed. Keep retained table storage
      // alive across that assignment, including aliases in the new environment.
      const auto previousBindings = expressionBindings;
      FunctionManager definition(*this);
      definition.currentFunction = callee;
      definition.currentFunctionIdentity = callee ? callee->GetLifetimeToken() : std::weak_ptr<const void>();
      operator=(definition);
      expressionBindings = previousBindings;
   }
   // Reentry into the same callee keeps its native invocation stack. Recursive
   // calls may supply another local table; they must not be reset as a new call.
   currentFunction = callee;
   currentFunctionIdentity = callee ? callee->GetLifetimeToken() : std::weak_ptr<const void>();
   localObjectStore = local;
   globalObjectStore = global;
   fcs = sequence;
}

void FunctionManager::RebindExpressionContext(const FunctionManager &source,
      const std::function<GmatBase*(GmatBase*)> &resolve,
      ObjectMap *local, ObjectMap *global)
{
   if (executing || !callers.empty() || functionObjectStore || !inputWrapperMap.empty() || !outputWrappers.empty())
      throw FunctionException("Expression definition binding requires a fresh FunctionManager.\n");
   ExpressionBindingScope binding;
   auto *function = BindExpressionReference(source.HasLiveFunction() ? source.currentFunction : nullptr, resolve);
   auto *system = BindExpressionReference(source.solarSys, resolve);
   auto *coordinateSystem = BindExpressionReference(source.internalCS, resolve);
   // Native runtime forces are an engine service: Begin/EndFiniteBurn must
   // update the caller's table. Only an already value-owned environment copies.
   auto *transients = source.expressionBindings && source.expressionBindings->OwnsForces(source.forces) ?
         binding.scopes->CopyForces(source.forces, resolve) : source.forces;
   // Assign fields directly. Native setters may mutate the callee definition.
   expressionBindings = binding.scopes;
   currentFunction = function;
   currentFunctionIdentity = function ? function->GetLifetimeToken() : std::weak_ptr<const void>();
   solarSys = system;
   internalCS = coordinateSystem;
   forces = transients;
   localObjectStore = local;
   globalObjectStore = global;
   publisher = source.publisher; // engine service, never owned by this manager
   callingFunction = nullptr;
   fcs = nullptr;
   firstExecution = true;
   isFinalized = false;
}

bool FunctionManager::OwnsExpressionMap(const ObjectMap *map) const
{
   return expressionBindings && expressionBindings->OwnsMap(map);
}

void FunctionManager::RetireExpressionContext()
{
   if (executing)
   {
      contextRetirementRequested = true;
      return;
   }
   contextRetirementRequested = false;
   if (reentrantInvocation) reentrantInvocation->RetireExpressionContext();
   if (completedInvocation) completedInvocation->RetireExpressionContext();
   localObjectStore = nullptr;
   globalObjectStore = nullptr;
   combinedObjectStore.clear();
   solarSys = nullptr;
   forces = nullptr;
   internalCS = nullptr;
   callingFunction = nullptr;
   fcs = nullptr;
   expressionBindings.reset();
   // Keep currentFunction/currentFunctionIdentity: reentrant Cleanup still
   // needs the actual callee to detach its own FOS. Do not alter the owned
   // stores, wrappers, stacks, results or Cleanup's finalized guard here.
}
