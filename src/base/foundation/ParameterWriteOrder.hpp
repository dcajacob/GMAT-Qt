//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool.
// Copyright (c) 2002-2026 United States Government as represented by the
// Administrator of the National Aeronautics and Space Administration.
// Licensed under the Apache License, Version 2.0.
//------------------------------------------------------------------------------
#ifndef ParameterWriteOrder_hpp
#define ParameterWriteOrder_hpp

#include "gmatdefs.hpp"
#include "GmatBaseException.hpp"
#include <string>
#include <vector>

namespace GmatSerialization
{
// Derived classes may order only their own properties. Complete that order
// with the current native IDs without caching it on the live object. Preserve
// authored dependencies (for example, thruster axes before direction values).
inline IntegerArray CompleteParameterOrder(Integer count,
      const IntegerArray &preferred, const std::string &objectName)
{
   if (count < 0)
      throw GmatBaseException("Cannot serialize object \"" + objectName +
         "\": negative parameter count.");

   IntegerArray order(preferred);
   std::vector<bool> seen(static_cast<std::size_t>(count), false);
   for (const Integer id : order)
   {
      if (id < 0 || id >= count)
         throw GmatBaseException("Cannot serialize object \"" + objectName +
            "\": parameter write order contains out-of-range ID " +
            std::to_string(id) + ".");
      if (seen[id])
         throw GmatBaseException("Cannot serialize object \"" + objectName +
            "\": parameter write order repeats ID " + std::to_string(id) + ".");
      seen[id] = true;
   }
   for (Integer id = 0; id < count; ++id)
      if (!seen[id])
         order.push_back(id);
   return order;
}
}

#endif
