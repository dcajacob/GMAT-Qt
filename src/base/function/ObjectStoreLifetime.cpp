// GMAT: General Mission Analysis Tool. Licensed under Apache-2.0.
#include "ObjectStoreLifetime.hpp"
#include "FunctionException.hpp"
#include <map>
#include <mutex>

namespace {
struct StoreRegistry {
   std::mutex mutex;
   std::map<ObjectMap*, std::weak_ptr<const void>> entries;
};
StoreRegistry& Registry() {
   // Sandbox destruction may occur after ordinary static destruction.
   static auto *registry = new StoreRegistry;
   return *registry;
}
}

ObjectStoreLifetime::~ObjectStoreLifetime() noexcept { Reset(); }

void ObjectStoreLifetime::Bind(ObjectMap *next)
{
   if (!next) { Reset(); return; }
   if (store == next && identity) return;
   auto token = std::make_shared<const int>(0);
   auto &registry = Registry();
   {
      std::lock_guard<std::mutex> lock(registry.mutex);
      const auto found = registry.entries.find(next);
      if (found != registry.entries.end() && !found->second.expired())
         throw FunctionException("Object store already has a lifetime owner.\n");
      registry.entries[next] = token;
   }
   Reset();
   store = next;
   identity = std::move(token);
}

void ObjectStoreLifetime::Reset() noexcept
{
   // Invalidate captured views before removing the registry entry. No view
   // owns this token, so it cannot extend the map or referents' lifetimes.
   auto *retired = store;
   store = nullptr;
   identity.reset();
   if (!retired) return;
   try {
      auto &registry = Registry();
      std::lock_guard<std::mutex> lock(registry.mutex);
      const auto found = registry.entries.find(retired);
      if (found != registry.entries.end() && found->second.expired())
         registry.entries.erase(found);
   } catch (...) {} // Destruction still expires every captured weak identity.
}

std::function<bool()> ObjectStoreLifetime::Capture(ObjectMap *store)
{
   if (!store) return {};
   std::weak_ptr<const void> token;
   {
      auto &registry = Registry();
      std::lock_guard<std::mutex> lock(registry.mutex);
      const auto found = registry.entries.find(store);
      if (found == registry.entries.end()) return {};
      if (found->second.expired()) { registry.entries.erase(found); return {}; }
      token = found->second;
   }
   return [token]() { return !token.expired(); };
}
