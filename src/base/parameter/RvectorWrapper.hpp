// GMAT: General Mission Analysis Tool. Licensed under Apache-2.0.
#ifndef RvectorWrapper_hpp
#define RvectorWrapper_hpp

#include "ElementWrapper.hpp"
#include "Rvector.hpp"

// An evaluated vector owns its data independently of Parameters and invocations.
class GMAT_API RvectorWrapper final : public ElementWrapper
{
public:
   RvectorWrapper();
   explicit RvectorWrapper(const Rvector &value);
   RvectorWrapper(const RvectorWrapper &source) = default;
   RvectorWrapper& operator=(const RvectorWrapper &source);
   ElementWrapper* Clone() const override;
   void RebindClonedReferences(const ElementWrapper &source,
         const std::function<GmatBase*(GmatBase*)> &resolve) override;
   std::string ToString() override;
   Gmat::ParameterType GetDataType() const override;
   Real EvaluateReal() const override;
   bool SetReal(Real value) override;
   const Rvector& EvaluateRvector() const override;
   bool SetRvector(const Rvector &value) override;

protected:
   void SetupWrapper() override;

private:
   Rvector value;
};

#endif
