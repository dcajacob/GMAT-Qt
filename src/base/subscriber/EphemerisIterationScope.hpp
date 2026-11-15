// Internal publication context shared by the core and estimation plugin.
#ifndef EphemerisIterationScope_hpp
#define EphemerisIterationScope_hpp
#include "gmatdefs.hpp"

class GMAT_API EphemerisIterationScope
{
public:
   using Acceptance = bool (*)(const void *);
   EphemerisIterationScope(Acceptance accepted, const void *owner);
   ~EphemerisIterationScope();
   EphemerisIterationScope(const EphemerisIterationScope &) = delete;
   EphemerisIterationScope &operator=(const EphemerisIterationScope &) = delete;
   static bool Accepted();
private:
   Acceptance accepted;
   const void *owner;
};
#endif
