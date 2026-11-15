#include "EphemerisIterationScope.hpp"
#include <vector>

namespace { thread_local std::vector<const EphemerisIterationScope*> scopes; }

EphemerisIterationScope::EphemerisIterationScope(Acceptance predicate, const void *object)
   : accepted(predicate), owner(object)
{
   scopes.push_back(this);
}

EphemerisIterationScope::~EphemerisIterationScope()
{
   scopes.pop_back();
}

bool EphemerisIterationScope::Accepted()
{
   for (const auto *scope : scopes)
      if (!scope->accepted(scope->owner)) return false;
   return true;
}
