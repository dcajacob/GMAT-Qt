// Native restoration of a fixed owned graph without replacing its identities.
#ifndef NativeRestoration_hpp
#define NativeRestoration_hpp

#include "GmatBase.hpp"
#include "GmatBaseException.hpp"
#include "FunctionValueContext.hpp"
#include <map>
#include <set>
#include <typeinfo>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace NativeRestoration
{
class PreparedAction final : public NativeRestorationPlan
{
   std::function<void()> action;
public:
   explicit PreparedAction(std::function<void()> value) : action(std::move(value)) {}
   void Apply() noexcept override { action(); }
};

inline std::unique_ptr<NativeRestorationPlan> Combine(
      std::vector<std::unique_ptr<NativeRestorationPlan>> plans)
{
   plans.erase(std::remove_if(plans.begin(), plans.end(),
         [](const std::unique_ptr<NativeRestorationPlan> &plan) { return !plan; }), plans.end());
   if (plans.empty()) return {};
   if (plans.size() == 1) return std::move(plans.front());
   auto saved = std::make_shared<std::vector<std::unique_ptr<NativeRestorationPlan>>>(std::move(plans));
   return std::make_unique<PreparedAction>([saved]() noexcept
   {
      for (auto &plan : *saved) plan->Apply();
   });
}

// Only native owners with exclusive, unexposed cache pointers use these
// helpers. Public/borrowed interpolator storage keeps its typed fixed-layout
// contract; an incompatible native type never gets a replacement fallback.
template<class T>
bool NeedsPrivateInterpolatorPreparation(T *target, const T *source)
{
   if (!target || !source) return target != source;
   try { target->ValidateRestorationLayout(*source); }
   catch (const BaseException &) { return true; }
   return false;
}

template<class T>
std::unique_ptr<NativeRestorationPlan> PreparePrivateInterpolator(T *&target,
      const T *source)
{
   if (!NeedsPrivateInterpolatorPreparation(target, source)) return {};
   if ((target && typeid(*target) != typeid(T)) ||
         (source && typeid(*source) != typeid(T)))
      throw GmatBaseException("Private interpolation preparation requires its native type.");
   struct Storage { std::unique_ptr<T> incoming; };
   auto saved = std::make_shared<Storage>();
   if (source)
   {
      source->ValidateRestorationLayout(*source);
      saved->incoming = std::make_unique<T>(*source);
      saved->incoming->ValidateRestorationLayout(*source);
   }
   return std::make_unique<PreparedAction>([&target, saved]() noexcept
   {
      T *incoming = saved->incoming.release();
      saved->incoming.reset(target);
      target = incoming;
   });
}

inline void Validate(const ObjectArray &source, const ObjectArray &target,
      std::map<const GmatBase*, const GmatBase*> &pairs,
      std::map<const GmatBase*, const GmatBase*> &reverse)
{
   if (source.size() != target.size())
      throw GmatBaseException("Restoration cannot change an active owned component collection.");
   for (size_t i = 0; i < source.size(); ++i)
   {
      auto *from = source[i];
      auto *to = target[i];
      if (!from && !to) continue;
      if (!from || !to || typeid(*from) != typeid(*to) ||
          from->GetTypeName() != to->GetTypeName() || from->GetName() != to->GetName())
         throw GmatBaseException("Restoration cannot change an active native component role or type.");
      const auto pair = pairs.emplace(from, to);
      if (!pair.second)
      {
         if (pair.first->second != to)
            throw GmatBaseException("Conflicting owned restoration component correspondence.");
         continue;
      }
      const auto prior = reverse.emplace(to, from);
      if (!prior.second && prior.first->second != from)
         throw GmatBaseException("Distinct captured components cannot restore one active component.");
      Validate(from->GetFunctionValueOwnedObjects(), to->GetFunctionValueOwnedObjects(), pairs, reverse);
   }
}

inline void Validate(const ObjectArray &source, const ObjectArray &target)
{
   std::map<const GmatBase*, const GmatBase*> pairs, reverse;
   Validate(source, target, pairs, reverse);
}

inline void RebindCovariance(GmatBase &target, const ObjectArray &source,
      const ObjectArray &destination)
{
   std::map<const GmatBase*, const GmatBase*> pairs, reverse;
   Validate(source, destination, pairs, reverse);
   if (auto *covariance = target.GetCovariance())
      for (const auto &pair : pairs)
         covariance->RebindOwner(pair.first, const_cast<GmatBase*>(pair.second));
}

inline void Copy(const ObjectArray &source, const ObjectArray &target)
{
   std::map<const GmatBase*, const GmatBase*> pairs, reverse;
   Validate(source, target, pairs, reverse);
   std::vector<std::unique_ptr<NativeRestorationPlan>> plans;
   if (GmatBase::IsNativeRestorationCopy())
      for (const auto &pair : pairs)
         plans.push_back(const_cast<GmatBase*>(pair.second)->PrepareRestoration(*pair.first));
   for (auto &plan : plans) if (plan) plan->Apply();
   std::set<GmatBase*> applied;
   for (size_t i = 0; i < source.size(); ++i)
   {
      if (!target[i] || !applied.insert(target[i]).second) continue;
      const auto previous = target[i]->GetFunctionValueContext();
      const auto name = target[i]->GetName();
      const auto fullName = target[i]->GetFullName();
      target[i]->Copy(source[i]);
      if (!target[i]->SetName(name))
         throw GmatBaseException("Cannot retain an active restoration component name.");
      target[i]->SetFullName(fullName);
      RebindCovariance(*target[i], source[i]->GetFunctionValueOwnedObjects(),
            target[i]->GetFunctionValueOwnedObjects());
      if (previous)
         target[i]->RetainFunctionValueReferences(previous, previous->GetPrivateObjects());
   }
}
}
#endif
