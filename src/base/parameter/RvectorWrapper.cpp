// GMAT: General Mission Analysis Tool. Licensed under Apache-2.0.
#include "RvectorWrapper.hpp"
#include "ParameterException.hpp"

RvectorWrapper::RvectorWrapper() : value(0)
{
   wrapperType = Gmat::VECTOR_WT;
}

RvectorWrapper::RvectorWrapper(const Rvector &initialValue) : value(initialValue)
{
   wrapperType = Gmat::VECTOR_WT;
}

RvectorWrapper& RvectorWrapper::operator=(const RvectorWrapper &source)
{
   if (&source == this) return *this;
   ElementWrapper::operator=(source);
   SetRvector(source.value);
   return *this;
}

ElementWrapper* RvectorWrapper::Clone() const
{
   return new RvectorWrapper(*this);
}

void RvectorWrapper::RebindClonedReferences(const ElementWrapper &source,
      const std::function<GmatBase*(GmatBase*)> &)
{
   CheckCloneType(source);
}

std::string RvectorWrapper::ToString()
{
   return EvaluateRvector().ToString();
}

Gmat::ParameterType RvectorWrapper::GetDataType() const
{
   return Gmat::RVECTOR_TYPE;
}

Real RvectorWrapper::EvaluateReal() const
{
   throw ParameterException("Cannot evaluate a vector as a scalar.\n");
}

bool RvectorWrapper::SetReal(Real)
{
   throw ParameterException("Cannot set a vector wrapper to a scalar.\n");
}

const Rvector& RvectorWrapper::EvaluateRvector() const
{
   if (!value.IsSized())
      throw ParameterException("Cannot evaluate an unsized vector result.\n");
   return value;
}

bool RvectorWrapper::SetRvector(const Rvector &newValue)
{
   if (&value == &newValue) return true;
   value.SetSize(newValue.GetSize());
   value = newValue;
   return true;
}

void RvectorWrapper::SetupWrapper()
{
   // Description labels a captured value; it does not name a borrowed object.
   refObjectNames.clear();
}
