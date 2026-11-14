// Native propagation definitions owned by estimation/simulation resources.
#ifndef NavigationPropagationCopies_hpp
#define NavigationPropagationCopies_hpp

#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "ODEModel.hpp"
#include "SolverException.hpp"
#include <algorithm>
#include <memory>
#include <map>
#include <set>
#include <typeinfo>
#include <utility>

namespace NavigationPropagationCopies
{
using Values = std::vector<std::unique_ptr<PropSetup>>;

inline std::unique_ptr<PropSetup> Clone(PropSetup &source, GmatBase &owner,
      bool followDefinition, const std::set<GmatBase*> &forbidden)
{
   GmatBase::FunctionValueCopyGuard nativeChildren;
   std::unique_ptr<GmatBase> value(source.Clone());
   if (value && (value.get() == &source || value.get() == &owner || forbidden.count(value.get())))
   {
      value.release();
      throw SolverException("Propagation definition Clone returned a borrowed object.\n");
   }
   auto *copy = dynamic_cast<PropSetup*>(value.get());
   if (!copy || typeid(*copy) != typeid(source) || copy->GetTypeName() != source.GetTypeName())
      throw SolverException("Cannot clone the native propagation definition type.\n");
   if (followDefinition) copy->BindCloneDefinition(source, &owner);
   else copy->SetCloneOwner(&owner);
   copy->SetCloneNotificationsEnabled(false);
   // Native copies retain the existing frame/body dependency lease. A full
   // independent function value graph still needs its owning graph provider.
   copy->SetFunctionValueContext(source.GetFunctionValueContext());
   if (copy->GetPropagator() && source.GetPropagator())
      copy->GetPropagator()->SetFunctionValueContext(source.GetPropagator()->GetFunctionValueContext());
   if (copy->GetODEModel() && source.GetODEModel())
      copy->GetODEModel()->SetFunctionValueContext(source.GetODEModel()->GetFunctionValueContext());
   value.release();
   return std::unique_ptr<PropSetup>(copy);
}

inline Values CloneAll(const std::vector<PropSetup*> &source, GmatBase &owner,
      const std::vector<PropSetup*> &existing = {})
{
   std::set<GmatBase*> forbidden{&owner};
   for (auto *value : source) forbidden.insert(value);
   for (auto *value : existing) forbidden.insert(value);
   Values result;
   result.reserve(source.size());
   for (auto *definition : source)
   {
      auto value = definition ? Clone(*definition, owner, false, forbidden) : nullptr;
      if (value) forbidden.insert(value.get());
      result.push_back(std::move(value));
   }
   // A source edge inside this owned list must follow the corresponding new
   // node, rather than borrowing the old parent's soon-retired component.
   std::map<const GmatBase*, GmatBase*> copies;
   for (size_t i = 0; i < source.size(); ++i)
      if (source[i] && result[i])
      {
         copies[source[i]] = result[i].get();
         if (source[i]->GetPropagator()) copies[source[i]->GetPropagator()] = result[i]->GetPropagator();
         if (source[i]->GetODEModel()) copies[source[i]->GetODEModel()] = result[i]->GetODEModel();
      }
   for (const auto &entry : copies)
   {
      const auto found = copies.find(entry.first->GetCloneSource());
      if (entry.second && found != copies.end()) entry.second->SetCloneSource(found->second);
   }
   return result;
}

inline std::vector<PropSetup*> Borrow(const Values &values)
{
   std::vector<PropSetup*> result;
   result.reserve(values.size());
   for (const auto &value : values) result.push_back(value.get());
   return result;
}

inline void Release(Values &values)
{
   for (auto &value : values) value.release();
}

inline void Retarget(PropSetup &retired, PropSetup &replacement)
{
   GmatBase::RetargetCloneSources(retired, replacement);
   if (retired.GetPropagator() && replacement.GetPropagator())
      GmatBase::RetargetCloneSources(*retired.GetPropagator(), *replacement.GetPropagator());
   if (retired.GetODEModel() && replacement.GetODEModel())
      GmatBase::RetargetCloneSources(*retired.GetODEModel(), *replacement.GetODEModel());
}

inline void Publish(std::vector<PropSetup*> &destination, Values &values)
{
   auto prepared = Borrow(values);
   // Match actual definitions, not ordinal positions or equal resource names.
   for (auto *retired : destination)
   {
      if (!retired || !retired->GetCloneSource()) continue;
      PropSetup *replacement = nullptr;
      bool ambiguous = false;
      for (auto *candidate : prepared)
         if (candidate && candidate->GetCloneSource() == retired->GetCloneSource())
         {
            if (replacement) ambiguous = true;
            replacement = candidate;
         }
      if (replacement && !ambiguous) Retarget(*retired, *replacement);
   }
   destination.swap(prepared);
   Release(values);
   for (auto *retired : prepared) delete retired;
}

inline void Validate(const std::vector<PropSetup*> &values)
{
   for (auto *value : values)
      if (!value || !value->GetPropagator())
         throw SolverException("A navigation propagation definition is unresolved.\n");
}

inline bool Apply(const std::vector<PropSetup*> &definitions, GmatBase *source,
      Integer parameterId)
{
   if (!source) return false;
   bool changed = false;
   std::vector<std::pair<GmatBase*, Integer>> updates{{source, parameterId}};
   std::set<GmatBase*> visited;
   for (size_t index = 0; index < updates.size(); ++index)
   {
      const auto current = updates[index];
      if (!visited.insert(current.first).second) continue;
      for (auto *definition : definitions)
      {
         if (!definition || !definition->ApplyCloneUpdate(current.first, current.second)) continue;
         changed = true;
         // A setup can explicitly derive from another owned setup. Forward a
         // root setup change after updating that actual intermediate definition.
         // Component notifications already follow their own source ancestry.
         if (definition != current.first && current.first->IsOfType(Gmat::PROP_SETUP))
         {
            const Integer localId = current.second < 0 ? -1 :
               definition->GetParameterID(current.first->GetParameterText(current.second));
            updates.emplace_back(definition, localId);
         }
      }
   }
   return changed;
}
}
#endif
