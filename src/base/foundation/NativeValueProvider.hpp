// Native value extensions preserve the released GMAT object layout and vtables.
#ifndef NativeValueProvider_hpp
#define NativeValueProvider_hpp

#include "GmatBase.hpp"

class Solver;

enum class NativeValueOperation
{
   OwnedObjects, RebindReferences, Restoration, SolverProvider, SolverClone
};

struct NativeValueProvider
{
   unsigned int depth = 0;
   bool (*matches)(const GmatBase*) = nullptr;
   ObjectArray (*ownedObjects)(const GmatBase*) = nullptr;
   bool (*rebindReferences)(GmatBase*, const GmatBase&, SolarSystem*,
         const std::function<GmatBase*(GmatBase*)>&) = nullptr;
   std::unique_ptr<NativeRestorationPlan> (*restoration)(GmatBase*, const GmatBase&) = nullptr;
   bool (*solverProvider)(const GmatBase*) = nullptr;
   void (*solverClone)(GmatBase*, const Solver&) = nullptr;
};

// A plugin unregisters its callbacks before its code is unloaded. Registration
// changes neither the provider object's data members nor any virtual slot.
class GMAT_API NativeValueProviderRegistration
{
public:
   explicit NativeValueProviderRegistration(NativeValueProvider provider);
   ~NativeValueProviderRegistration();
   NativeValueProviderRegistration(const NativeValueProviderRegistration&) = delete;
   NativeValueProviderRegistration& operator=(const NativeValueProviderRegistration&) = delete;
private:
   NativeValueProvider provider;
};

GMAT_API NativeValueProvider FindNativeValueProvider(
      const GmatBase &object, NativeValueOperation operation);

#endif
