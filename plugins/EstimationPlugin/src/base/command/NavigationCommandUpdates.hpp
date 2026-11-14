// Native definition staging for RunEstimator and RunSimulator.
#ifndef NavigationCommandUpdates_hpp
#define NavigationCommandUpdates_hpp

#include "NavigationPropagationCopies.hpp"
#include "CommandException.hpp"
#include "FunctionValueContext.hpp"
#include "DataFile.hpp"
#include "MeasurementManager.hpp"

namespace NavigationCommandUpdates
{
inline bool Related(PropSetup *setup, GmatBase *source)
{
   if (!setup || !source) return false;
   if (setup == source || setup->IsCloneOf(*source)) return true;
   for (GmatBase *child : {static_cast<GmatBase*>(setup->GetPropagator()),
                          static_cast<GmatBase*>(setup->GetODEModel())})
      if (child && (child == source || child->IsCloneOf(*source))) return true;
   return false;
}

inline void KeepReferences(GmatBase &target, const GmatBase &source, bool whole)
{
   const auto context = source.GetFunctionValueContext();
   if (whole) target.SetFunctionValueContext(context);
   else if (context) target.RetainFunctionValueReferences(context, context->GetPrivateObjects());
}

template<class Resource>
std::unique_ptr<Resource> Clone(Resource &source, GmatBase &owner,
      const GmatBase *definition, const ObjectArray &existing)
{
   std::set<GmatBase*> forbidden{&owner, &source};
   forbidden.insert(existing.begin(), existing.end());
   for (auto *setup : source.GetOwnedPropagationDefinitions())
   {
      forbidden.insert(setup);
      if (setup)
      {
         forbidden.insert(setup->GetPropagator());
         forbidden.insert(setup->GetODEModel());
      }
   }
   GmatBase::FunctionValueCopyGuard nativeCopy;
   std::unique_ptr<GmatBase> value(source.Clone());
   if (value && forbidden.count(value.get()))
   {
      value.release();
      throw CommandException("Navigation resource Clone returned an existing object.\n");
   }
   auto *copy = dynamic_cast<Resource*>(value.get());
   if (!copy || typeid(*copy) != typeid(source) || copy->GetTypeName() != source.GetTypeName())
      throw CommandException("Cannot clone the native navigation resource type.\n");
   copy->SetCloneSource(definition);
   copy->SetCloneOwner(&owner);
   copy->SetCloneNotificationsEnabled(false);
   KeepReferences(*copy, source, true);
   value.release();
   return std::unique_ptr<Resource>(copy);
}

inline void RepairComponentSources(PropSetup &setup)
{
   const auto *definition = dynamic_cast<const PropSetup*>(setup.GetCloneSource());
   auto *nativeDefinition = const_cast<PropSetup*>(definition);
   GmatBase *sources[] = {nativeDefinition ? nativeDefinition->GetPropagator() : nullptr,
                         nativeDefinition ? nativeDefinition->GetODEModel() : nullptr};
   GmatBase *targets[] = {setup.GetPropagator(), setup.GetODEModel()};
   for (size_t i = 0; i < 2; ++i)
      if (targets[i])
      {
         const bool compatible = sources[i] && typeid(*sources[i]) == typeid(*targets[i]) &&
            sources[i]->GetTypeName() == targets[i]->GetTypeName();
         targets[i]->SetCloneSource(compatible ? sources[i] : nullptr);
         targets[i]->SetCloneOwner(&setup);
      }
}

inline void Mirror(PropSetup &target, PropSetup &original, GmatBase &source, Integer id)
{
   GmatBase::FunctionValueCopyGuard nativeCopy;
   if (&source == &original)
   {
      if (id < 0) target.Copy(&original);
      else
      {
         const auto label = source.GetParameterText(id);
         if (label == "FM") target.SetODEModel(original.GetODEModel());
         else if (label == "Type")
         {
            target.SetPropagator(original.GetPropagator(), true);
            target.SetODEModel(original.GetODEModel());
         }
         else target.CopyParameter(source, id);
      }
      KeepReferences(target, original, id < 0);
   }
   else if (&source == original.GetPropagator())
   {
      if (id < 0) target.SetPropagator(original.GetPropagator(), true);
      else target.GetPropagator()->CopyParameter(source, id);
      KeepReferences(*target.GetPropagator(), source, id < 0);
      KeepReferences(*target.GetPropagator(), original, false);
   }
   else if (&source == original.GetODEModel())
   {
      if (id < 0) target.SetODEModel(original.GetODEModel());
      else
      {
         if (!target.GetODEModel()) throw CommandException("The pending engine has no ODE model.\n");
         target.GetODEModel()->CopyParameter(source, id);
      }
      if (!target.GetODEModel())
         throw CommandException("The pending setup no longer uses an ODE model.\n");
      KeepReferences(*target.GetODEModel(), source, id < 0);
      KeepReferences(*target.GetODEModel(), original, false);
   }
   if (target.GetPropagator() && original.GetPropagator())
      KeepReferences(*target.GetPropagator(), *original.GetPropagator(), false);
   if (target.GetODEModel() && original.GetODEModel())
      KeepReferences(*target.GetODEModel(), *original.GetODEModel(), false);
   RepairComponentSources(target);
}

template<class Resource>
std::unique_ptr<Resource> Prepare(Resource *runtime, Resource *pending, Resource *selected,
      const std::vector<PropSetup*> &groups, GmatBase &owner, GmatBase *source,
      Integer id, const ObjectArray &existing)
{
   Resource *base = pending ? pending : (runtime ? runtime : selected);
   if (!base || !source) return nullptr;
   const bool root = source == selected || source == runtime || source == pending ||
      source == base->GetCloneSource();
   bool related = root;
   related |= base->HasOwnedDefinitionSource(source);
   if (runtime) related |= runtime->HasOwnedDefinitionSource(source);
   for (auto *setup : base->GetOwnedPropagationDefinitions()) related |= Related(setup, source);
   if (runtime)
      for (auto *setup : runtime->GetOwnedPropagationDefinitions()) related |= Related(setup, source);
   for (auto *setup : groups) related |= Related(setup, source);
   if (!related) return nullptr;

   auto *input = root && id < 0 ? dynamic_cast<Resource*>(source) : base;
   if (!input) throw CommandException("A navigation definition update has an incompatible resource type.\n");
   const GmatBase *identity = source == selected || base == selected ? selected : base->GetCloneSource();
   auto value = Clone(*input, owner, identity, existing);
   if (root)
   {
      if (id >= 0)
      {
         value->CopyParameter(*source, id);
         KeepReferences(*value, *source, false);
      }
      return value;
   }
   value->ApplyOwnedCloneUpdate(source, id);

   // Direct edits to command-owned definitions/groups have no ambient lookup.
   // Map their actual native definition to the corresponding staged value.
   const auto findValue = [&](Resource &originalOwner, PropSetup &original) -> PropSetup*
   {
      const auto &oldValues = originalOwner.GetOwnedPropagationDefinitions();
      const auto &newValues = value->GetOwnedPropagationDefinitions();
      if (&originalOwner == base)
         for (size_t i = 0; i < oldValues.size(); ++i)
            if (oldValues[i] == &original && i < newValues.size()) return newValues[i];
      const GmatBase *definition = original.GetCloneSource();
      PropSetup *result = nullptr;
      for (auto *candidate : newValues)
         if (candidate && definition && candidate->GetCloneSource() == definition)
         {
            if (result) throw CommandException("Ambiguous pending propagation definition.\n");
            result = candidate;
         }
      if (!result) throw CommandException("Cannot map an edited engine into the pending definition.\n");
      return result;
   };
   const auto direct = [&](PropSetup *setup)
   {
      return setup && (setup == source || setup->GetPropagator() == source || setup->GetODEModel() == source);
   };
   const auto mirror = [&](PropSetup &target, PropSetup &original)
   {
      Mirror(target, original, *source, id);
      value->ApplyPropagationCloneUpdate(&target);
   };
   for (Resource *originalOwner : {base, runtime})
   {
      if (!originalOwner || (originalOwner == runtime && runtime == base)) continue;
      for (auto *setup : originalOwner->GetOwnedPropagationDefinitions())
         if (direct(setup)) mirror(*findValue(*originalOwner, *setup), *setup);
   }
   // The base owner is also needed when there is no distinct runtime owner.
   if (runtime == base)
      for (auto *setup : base->GetOwnedPropagationDefinitions())
         if (direct(setup)) mirror(*findValue(*base, *setup), *setup);
   if (runtime)
      for (auto *group : groups)
         if (direct(group))
         {
            auto *definition = group->GetCloneSource();
            PropSetup *original = nullptr;
            for (auto *setup : runtime->GetOwnedPropagationDefinitions())
               if (setup == definition) original = setup;
            if (!original) throw CommandException("The edited group has no live native definition.\n");
            mirror(*findValue(*runtime, *original), *group);
         }
   return value;
}

inline void AttachStream(MeasurementManager &manager, GmatBase &owner, GmatBase &source, bool ramp)
{
   GmatBase::FunctionValueCopyGuard nativeCopy;
   std::unique_ptr<GmatBase> value(source.Clone());
   if (value.get() == &source || value.get() == &owner)
   {
      value.release();
      throw CommandException("Data stream Clone returned a borrowed object.\n");
   }
   auto *copy = dynamic_cast<DataFile*>(value.get());
   if (!copy || typeid(*copy) != typeid(source) || copy->GetTypeName() != source.GetTypeName())
      throw CommandException("Cannot clone the native data stream definition.\n");
   copy->SetCloneSource(&source);
   copy->SetCloneOwner(&owner);
   copy->SetCloneNotificationsEnabled(false);
   KeepReferences(*copy, source, true);
   if (ramp) manager.SetRampTableDataStreamObject(copy);
   else manager.SetStreamObject(copy);
   value.release(); // MeasurementManager adopts its stream objects.
}
}
#endif
