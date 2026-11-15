// GMAT: General Mission Analysis Tool. Licensed under Apache-2.0.
#ifndef FunctionValueContext_hpp
#define FunctionValueContext_hpp

#include "gmatdefs.hpp"
#include <map>
#include <memory>

class GmatBase;

// A value's dependencies have deterministic resource lifetime. The context
// never owns its public root; root-to-dependency backlinks remain non-owning.
class GMAT_API FunctionValueContext
{
public:
   virtual ~FunctionValueContext() = default;
   virtual const ObjectArray& GetPrivateObjects() const = 0;
   virtual bool RetainsReference(const GmatBase *object) const = 0;
   virtual void RebindCopy(const GmatBase &source, GmatBase &target) const = 0;
   virtual void RebindCaller(GmatBase &target,
         const std::shared_ptr<const void> &epoch,
         const std::map<const GmatBase*, GmatBase*> &bindings) const = 0;
};

#endif
