//------------------------------------------------------------------------------
//                         ObjectScriptExportScope
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool.
//
// Copyright (c) 2002-2026 United States Government as represented by the
// Administrator of The National Aeronautics and Space Administration.
// All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//------------------------------------------------------------------------------
#ifndef ObjectScriptExportScope_hpp
#define ObjectScriptExportScope_hpp

#include "gmatdefs.hpp"
#include <ios>
#include <limits>
#include <locale>
#include <sstream>

// Explicit object exports include declarations from other source files. The
// request follows nested native serializers on this thread without modifying
// an object's main-script or force-generation flags. Normal script saving
// retains its source-file filtering outside this scope.
class GMAT_API ObjectScriptExportScope
{
public:
   ObjectScriptExportScope() noexcept;
   ~ObjectScriptExportScope() noexcept;
   ObjectScriptExportScope(const ObjectScriptExportScope &) = delete;
   ObjectScriptExportScope &operator=(const ObjectScriptExportScope &) = delete;

   static bool IsActive() noexcept;
   static Integer Precision(Integer usual) noexcept
   {
      const Integer digits = std::numeric_limits<Real>::max_digits10;
      return IsActive() && usual < digits ? digits : usual;
   }
   static void Prepare(std::ios &stream)
   {
      if (!IsActive()) return;
      stream.imbue(std::locale::classic());
      stream.unsetf(std::ios::floatfield);
      if (stream.precision() < std::numeric_limits<Real>::max_digits10)
         stream.precision(std::numeric_limits<Real>::max_digits10);
   }
   static std::string RealLiteral(Real value)
   {
      std::ostringstream stream;
      stream.imbue(std::locale::classic());
      stream.precision(std::numeric_limits<Real>::max_digits10);
      stream << value;
      return stream.str();
   }
   template<class V> static std::string VectorLiteral(const V &value)
   {
      std::ostringstream stream;
      stream.imbue(std::locale::classic());
      stream.precision(std::numeric_limits<Real>::max_digits10);
      stream << "[ ";
      for (Integer i = 0; i < value.GetSize(); ++i)
      {
         if (i) stream << " ";
         stream << value(i);
      }
      stream << " ]";
      return stream.str();
   }

private:
   const bool previous;
};

#endif
