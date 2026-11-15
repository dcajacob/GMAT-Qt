// Private, synchronous request for one force's Cartesian acceleration gradient.
#ifndef OrbitStmForceGradient_hpp
#define OrbitStmForceGradient_hpp

#include "Rmatrix33.hpp"
#include "Rmatrix.hpp"
#include <cmath>

namespace OrbitStmForceGradient
{
class Request
{
public:
   explicit Request(const void *force) :
      value(3,6), available(false),
      provider(force), previous(Active())
   {
      Active() = this;
   }
   ~Request() { Active() = previous; }
   Request(const Request &) = delete;
   Request &operator=(const Request &) = delete;

   static Request *For(const void *force)
   {
      auto *request = Active();
      return request && force && request->provider == force ? request : nullptr;
   }

   static void Capture(const void *force, const Rmatrix33 &gradient)
   {
      if (!For(force)) return;
      Rmatrix stateGradient(3,6);
      for (Integer i = 0; i < 3; ++i)
         for (Integer j = 0; j < 3; ++j)
            stateGradient(i,j) = gradient(i,j);
      Capture(force, stateGradient);
   }

   static void Capture(const void *force, const Rmatrix &gradient)
   {
      if (auto *request = For(force))
      {
         if (gradient.GetNumRows() != 3 || gradient.GetNumColumns() != 6) return;
         for (Integer i = 0; i < 3; ++i)
            for (Integer j = 0; j < 6; ++j)
               if (!std::isfinite(gradient(i,j))) return;
         request->value = gradient;
         request->available = true;
      }
   }

   Rmatrix value;
   bool available;

private:
   static Request *&Active()
   {
      static thread_local Request *request = nullptr;
      return request;
   }
   const void *provider;
   Request *previous;
};
}
#endif
