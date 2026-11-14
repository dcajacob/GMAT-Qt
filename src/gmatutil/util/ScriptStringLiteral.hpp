// SPDX-License-Identifier: Apache-2.0
#ifndef ScriptStringLiteral_hpp
#define ScriptStringLiteral_hpp

#include "UtilityException.hpp"
#include <string>

// Explicit byte literals keep saved strings on one physical script line.
// Legacy single-quoted strings retain their raw interior, including slashes
// and repeated apostrophes. Decode only at a script-input boundary, never
// while passing an already evaluated string to a native setter.
namespace GmatScriptString
{
inline bool IsEncoded(const std::string &value)
{
   return value.size() >= 2 && value[0] == '@' && value[1] == '\'';
}

inline bool IsLiteral(const std::string &value)
{
   return IsEncoded(value) || (value.size() >= 2 && value.front() == '\'' && value.back() == '\'');
}

inline std::string Decode(const std::string &literal)
{
   if (!IsEncoded(literal))
   {
      if (literal.size() >= 2 && literal.front() == '\'' && literal.back() == '\'')
         return literal.substr(1, literal.size() - 2);
      return literal;
   }
   const auto malformed = []() -> void
   {
      throw UtilityException("Encoded string literals require @' followed by paired hexadecimal digits and a closing apostrophe.\n");
   };
   if (literal.size() < 3 || literal.back() != '\'' || (literal.size() - 3) % 2) malformed();
   const auto nibble = [&](char ch) -> unsigned
   {
      if (ch >= '0' && ch <= '9') return static_cast<unsigned>(ch - '0');
      if (ch >= 'a' && ch <= 'f') return static_cast<unsigned>(ch - 'a' + 10);
      if (ch >= 'A' && ch <= 'F') return static_cast<unsigned>(ch - 'A' + 10);
      malformed(); return 0;
   };
   std::string result;
   result.reserve((literal.size() - 3) / 2);
   for (std::size_t i = 2; i < literal.size() - 1; i += 2)
      result.push_back(static_cast<char>((nibble(literal[i]) << 4) | nibble(literal[i + 1])));
   return result;
}

inline std::string Quote(const std::string &value)
{
   bool encoded = false;
   for (unsigned char ch : value)
      if (ch < 32 || ch >= 127 || ch == '\'' || ch == '[' || ch == ']' ||
          ch == '{' || ch == '}' || ch == ';' || ch == '%' || ch == '=')
      { encoded = true; break; }
   if (!encoded) return "'" + value + "'";
   std::string result = "@'";
   if (value.size() > (result.max_size() - 3) / 2)
      throw UtilityException("String value exceeds the encoded script literal size limit.\n");
   result.reserve(value.size() * 2 + 3);
   constexpr char hex[] = "0123456789ABCDEF";
   for (unsigned char ch : value)
   {
      result.push_back(hex[ch >> 4]); result.push_back(hex[ch & 15]);
   }
   result.push_back('\'');
   return result;
}
}
#endif
