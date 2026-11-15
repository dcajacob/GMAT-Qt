// GMAT: General Mission Analysis Tool. Licensed under Apache-2.0.
#ifndef ObjectStoreLifetime_hpp
#define ObjectStoreLifetime_hpp

#include "gmatdefs.hpp"
#include <functional>
#include <memory>

// An owner registers the lifetime of its actual map, without retaining any of
// that map's objects. Reset must precede clearing or replacing those objects.
// A subsequent Bind at the same address has a distinct lifetime identity.
class GMAT_API ObjectStoreLifetime
{
public:
   ObjectStoreLifetime() = default;
   ~ObjectStoreLifetime() noexcept;
   ObjectStoreLifetime(const ObjectStoreLifetime&) = delete;
   ObjectStoreLifetime& operator=(const ObjectStoreLifetime&) = delete;
   void Bind(ObjectMap *store);
   void Reset() noexcept;
   // Empty means that this registry does not own the map's lifetime.
   static std::function<bool()> Capture(ObjectMap *store);
private:
   ObjectMap *store = nullptr;
   std::shared_ptr<const void> identity;
};
#endif
