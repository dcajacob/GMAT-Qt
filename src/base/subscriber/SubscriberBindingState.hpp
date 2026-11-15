// Native subscriber bindings are borrowed references, never runtime owners.
#ifndef SubscriberBindingState_hpp
#define SubscriberBindingState_hpp

#include "GmatBase.hpp"
#include "Function.hpp"
#include <functional>
#include <map>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

// Capture stable member slots or explicit accessors, rather than addresses of
// vector elements that a nested initializer can invalidate. Actual generations
// distinguish a surviving object from a different object at the same address.
class SubscriberBindingState
{
public:
   template<class T, class Publish>
   void CaptureValue(T *value, Publish publish)
   {
      const auto identity = value ? value->GetLifetimeToken() : std::weak_ptr<const void>{};
      actions.emplace_back([value, identity, publish](bool compatible)
      {
         const bool live = !value || GmatBase::IsLiveReference(value, identity);
         const bool published = publish(compatible && live ? value : nullptr);
         return compatible && live && published;
      });
   }

   template<class T> void Capture(T *&slot)
   {
      CaptureValue(slot, [location = &slot](T *value) { *location = value; return true; });
   }

   template<class T> void Capture(std::vector<T*> &slot)
   {
      auto values = std::make_shared<std::vector<T*>>(slot);
      auto identities = std::make_shared<std::vector<std::weak_ptr<const void>>>();
      identities->reserve(slot.size());
      for (auto *value : slot)
         identities->push_back(value ? value->GetLifetimeToken() : std::weak_ptr<const void>{});
      actions.emplace_back([location = &slot, values, identities](bool compatible)
      {
         bool valid = compatible;
         if (!compatible) values->clear();
         else for (size_t i = 0; i < values->size(); ++i)
            if ((*values)[i] && !GmatBase::IsLiveReference((*values)[i], (*identities)[i]))
            { (*values)[i] = nullptr; valid = false; }
         location->swap(*values);
         return valid;
      });
   }

   template<class T, class V> void Capture(std::map<T*, V> &slot)
   {
      auto values = std::make_shared<std::map<T*, V>>(slot);
      auto identities = std::make_shared<std::map<T*, std::weak_ptr<const void>>>();
      for (const auto &entry : slot)
         if (entry.first) identities->emplace(entry.first, entry.first->GetLifetimeToken());
      actions.emplace_back([location = &slot, values, identities](bool compatible)
      {
         bool valid = compatible;
         if (!compatible) values->clear();
         else for (auto it = values->begin(); it != values->end();)
         {
            if (it->first && !GmatBase::IsLiveReference(it->first, identities->at(it->first)))
            { it = values->erase(it); valid = false; }
            else ++it;
         }
         location->swap(*values);
         return valid;
      });
   }

   // Publication column indices and labels belong to the current binding,
   // rather than to the accumulated numerical or display history.
   template<class T> void CaptureMetadata(T &slot)
   {
      static_assert(std::is_nothrow_default_constructible<T>::value &&
            noexcept(std::swap(std::declval<T&>(), std::declval<T&>())),
            "Subscriber binding metadata must restore without allocation");
      auto value = std::make_shared<T>(slot);
      actions.emplace_back([location = &slot, value](bool compatible)
      {
         if (!compatible) { T empty{}; std::swap(*value, empty); }
         std::swap(*location, *value);
         return compatible;
      });
   }

   void Require(std::function<bool()> unchanged) { guards.push_back(std::move(unchanged)); }

   bool Restore() noexcept
   {
      if (restored) return successful;
      restored = true;
      bool compatible = true;
      for (const auto &guard : guards)
         try { if (!guard()) compatible = false; }
         catch (...) { compatible = false; }
      successful = compatible;
      for (const auto &action : actions)
         try { if (!action(compatible)) successful = false; }
         catch (...) { successful = false; }
      return successful;
   }

private:
   std::vector<std::function<bool()>> guards;
   std::vector<std::function<bool(bool)>> actions;
   bool restored = false, successful = true;
};

#endif
