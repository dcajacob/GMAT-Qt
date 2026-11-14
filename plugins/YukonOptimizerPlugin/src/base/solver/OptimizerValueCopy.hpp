// GMAT: General Mission Analysis Tool. Licensed under Apache-2.0.
#ifndef OptimizerValueCopy_hpp
#define OptimizerValueCopy_hpp

#include "Rvector.hpp"
#include "Rmatrix.hpp"

namespace OptimizerValueCopy
{
// Preserve unsized storage as well as numerical values. C++17 prvalue return
// construction avoids the native copy constructor's unsized-source exception.
inline Rvector Vector(const Rvector &source)
{
   if (source.IsSized()) return Rvector(source);
   return Rvector();
}

inline Rmatrix Matrix(const Rmatrix &source)
{
   if (source.IsSized()) return Rmatrix(source);
   return Rmatrix();
}
}

#endif
