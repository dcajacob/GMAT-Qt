// SPDX-License-Identifier: Apache-2.0
// GMAT expression definition binding. Containers own their storage; the enclosing
// retained value/context owns referenced GMAT resources. No model execution here.
#ifndef GMAT_EXPRESSION_BINDING_HPP
#define GMAT_EXPRESSION_BINDING_HPP
#include "GmatBase.hpp"
#include "MathException.hpp"
#include "PhysicalModel.hpp"
#include <map>
#include <memory>
#include <functional>
#include <set>
#include <typeinfo>

using ExpressionResolver = std::function<GmatBase*(GmatBase*)>;

template<class T> T *BindExpressionReference(T *source, const ExpressionResolver &resolve)
{
   if (!source) return nullptr;
   T *value = dynamic_cast<T*>(resolve(source));
   if (!value || typeid(*value) != typeid(*source))
      throw MathException("Incompatible actual expression reference.\n");
   return value;
}

struct ExpressionScopeCopies
{
   std::map<const ObjectMap*, std::unique_ptr<ObjectMap>> maps;
   std::set<const ObjectMap*> failedMaps;
   std::map<const std::vector<PhysicalModel*>*, std::unique_ptr<std::vector<PhysicalModel*>>> forces;
   std::set<const std::vector<PhysicalModel*>*> failedForces;
   // One ticket classifies only the immediate resolver call as a scope-table
   // backlink. Descendant computational references must use their normal rules.
   inline static thread_local bool scopeReference = false;
   struct ScopeReference
   {
      bool previous;
      ScopeReference() : previous(scopeReference) { scopeReference = true; }
      ~ScopeReference() { scopeReference = previous; }
   };
   static bool ConsumeScopeReference()
   {
      const bool value = scopeReference;
      scopeReference = false;
      return value;
   }
   ObjectMap *CopyMap(const ObjectMap *source, const ExpressionResolver &resolve)
   {
      if (!source) return nullptr;
      if (failedMaps.count(source)) throw MathException("Expression scope mapping previously failed.\n");
      auto found = maps.find(source);
      if (found != maps.end()) return found->second.get();
      std::unique_ptr<ObjectMap> map(new ObjectMap);
      ObjectMap *value = map.get();
      maps.emplace(source, std::move(map));
      // Publish the container before mapping entries: scope-table aliases may
      // refer back to this table while another resource is passively bound.
      try
      {
         for (const auto &entry : *source)
         {
            ScopeReference role;
            GmatBase *object = entry.second ? resolve(entry.second) : nullptr;
            if (entry.second && !object) throw MathException("Missing expression scope reference.\n");
            value->emplace(entry.first, object);
         }
      }
      catch (...) { failedMaps.insert(source); throw; }
      return value;
   }
   std::vector<PhysicalModel*> *CopyForces(const std::vector<PhysicalModel*> *source,
         const ExpressionResolver &resolve)
   {
      if (!source) return nullptr;
      if (failedForces.count(source)) throw MathException("Expression force mapping previously failed.\n");
      auto found = forces.find(source);
      if (found != forces.end()) return found->second.get();
      std::unique_ptr<std::vector<PhysicalModel*>> table(new std::vector<PhysicalModel*>);
      auto *value = table.get();
      forces.emplace(source, std::move(table));
      try
      {
         for (auto *force : *source)
         {
            ScopeReference role; // Runtime environment membership, not a calculation edge.
            value->push_back(BindExpressionReference(force, resolve));
         }
      }
      catch (...) { failedForces.insert(source); throw; }
      return value;
   }
   bool OwnsForces(const std::vector<PhysicalModel*> *table) const
   {
      if (!table) return false;
      for (const auto &entry : forces) if (entry.second.get() == table) return true;
      return false;
   }
   bool OwnsMap(const ObjectMap *map) const
   {
      if (!map) return false;
      for (const auto &entry : maps) if (entry.second.get() == map) return true;
      return false;
   }
};

struct ExpressionBindingScope
{
   inline static thread_local std::shared_ptr<ExpressionScopeCopies> active;
   inline static thread_local bool preserveReadiness = true;
   std::shared_ptr<ExpressionScopeCopies> previous, scopes;
   bool previousReadiness;
   explicit ExpressionBindingScope(bool preserve = true, bool fresh = false) :
      previous(active), scopes(active && !fresh ? active : std::make_shared<ExpressionScopeCopies>()),
      previousReadiness(preserveReadiness)
   {
      active = scopes;
      preserveReadiness = preserveReadiness && preserve;
   }
   explicit ExpressionBindingScope(const std::shared_ptr<ExpressionScopeCopies> &owned) :
      previous(active), scopes(owned), previousReadiness(preserveReadiness)
   {
      if (!scopes) throw MathException("Missing expression scope storage.\n");
      active = scopes;
   }
   ~ExpressionBindingScope()
   {
      active = previous;
      preserveReadiness = previousReadiness;
   }
};
#endif
