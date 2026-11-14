#pragma once
#include "gmatdefs.hpp"
#include <string>
#include <vector>

// Owned values, independent of MATLAB C libraries. Numbers are column-major.
struct GMAT_API MatlabValue
{
   enum class Type { UNSUPPORTED, NUMERIC, LOGICAL, CHARACTER, STRUCTURE, CELL };
   Type type = Type::UNSUPPORTED;
   Integer rows = 0, columns = 0;
   std::string className, text;
   RealArray numbers;
   StringArray fieldNames;
   std::vector<MatlabValue> children;
};
