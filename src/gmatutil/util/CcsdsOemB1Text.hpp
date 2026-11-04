// Internal serialization checks for CCSDS 502.0-B-1, sections 4.3.1, 4.3.3(h),
// and 4.3.6. These helpers do not change the stored identifiers or comments.
#ifndef CcsdsOemB1Text_hpp
#define CcsdsOemB1Text_hpp

#include "UtilityException.hpp"
#include <string>

namespace CcsdsOemB1Text
{
   static inline std::string MetadataLine(const std::string &field, const std::string &value)
   {
      return field + std::string(field.size() < 21 ? 21-field.size() : 1, ' ') + "= " + value + "\n";
   }

   static inline void RequireValue(const std::string &field,
                                  const std::string &value, bool required = true)
   {
      for (unsigned char c : value)
         if (c < 32 || c > 126)
            throw UtilityException("CCSDS OEM 1.0 " + field +
               " must contain only printable ASCII characters (32-126)");
      // B-1 treats underscores as blanks in text values.
      if (required && value.find_first_not_of(" _") == std::string::npos)
         throw UtilityException("CCSDS OEM 1.0 " + field + " must have a non-null value");
   }

   static inline void CheckLines(const std::string &text)
   {
      std::size_t start = 0;
      while (start < text.size())
      {
         const auto end = text.find('\n', start);
         const auto line = text.substr(start, end == std::string::npos ? end : end-start);
         RequireValue("output line", line, false);
         if (line.size() > 254)
            throw UtilityException("CCSDS OEM 1.0 output line exceeds 254 ASCII characters: " +
               line.substr(0, line.find_first_of(" =")));
         if (end == std::string::npos) break;
         start = end+1;
      }
   }

   static inline std::string Comment(const std::string &value)
   {
      std::string result;
      std::size_t start = 0;
      for (;;)
      {
         const auto end = value.find_first_of("\r\n", start);
         const auto line = value.substr(start, end == std::string::npos ? end : end-start);
         RequireValue("COMMENT", line, false);
         const std::string formatted = "COMMENT " + line + "\n";
         CheckLines(formatted);
         result += formatted;
         if (end == std::string::npos) break;
         start = end+1;
         if (start < value.size() && value[start] != value[end] &&
             (value[start] == '\r' || value[start] == '\n')) ++start;
      }
      return result;
   }
}
#endif
