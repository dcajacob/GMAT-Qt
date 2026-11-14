// Internal OEM 1.0 serialization history. Public writer layouts are unchanged.
#ifndef CcsdsOemB1Chronology_hpp
#define CcsdsOemB1Chronology_hpp

#include "UtilityException.hpp"
#include <ios>
#include <map>
#include <mutex>
#include <string>
#include <vector>

class CCSDSEMWriter;

namespace CcsdsOemB1Chronology
{
struct State
{
   bool hasMetadata = false;
   std::streamoff metadataPosition = -1;
   std::streamoff lastDataMetadataPosition = -1;
   std::string lastEpoch;
   std::string objectId;
};

struct Registry
{
   std::mutex mutex;
   std::map<const CCSDSEMWriter*, State> writers;
};

inline Registry &History()
{
   // Writers can retire during library shutdown. Individual entries are erased
   // on reopen/destruction; this small registry survives static retirement.
   static auto *registry = new Registry;
   return *registry;
}

inline void Reset(const CCSDSEMWriter *writer)
{
   auto &registry = History();
   std::lock_guard<std::mutex> lock(registry.mutex);
   registry.writers.erase(writer);
}

inline std::string TextKey(const std::string &value)
{
   // B-1 4.3.3(a,g): case is insignificant, underscores represent blanks,
   // and multiple blanks are equivalent to one. Do not alter stored text.
   std::string key;
   bool space = false;
   for (unsigned char c : value)
   {
      if (c == ' ' || c == '_') { space = !key.empty(); continue; }
      if (space) key += ' ';
      space = false;
      key += c >= 'a' && c <= 'z' ? char(c - 'a' + 'A') : char(c);
   }
   return key;
}

inline State PreviewMetadata(const CCSDSEMWriter *writer, std::streampos position,
      const std::string &objectId)
{
   if (position == std::streampos(-1))
      throw UtilityException("Cannot determine CCSDS OEM 1.0 metadata position for epoch ordering");
   auto &registry = History();
   std::lock_guard<std::mutex> lock(registry.mutex);
   State state;
   const auto found = registry.writers.find(writer);
   if (found != registry.writers.end()) state = found->second;
   const auto identity = TextKey(objectId);
   if (!state.objectId.empty() && identity != state.objectId)
      throw UtilityException("CCSDS OEM 1.0 must contain orbit data for a single object; "
            "OBJECT_ID cannot change within its file");
   state.objectId = identity;
   state.hasMetadata = true;
   state.metadataPosition = static_cast<std::streamoff>(position);
   // Rewriting metadata at the same file offset does not start a new block or
   // forget records already serialized. Buffer clearing is independent too.
   return state;
}

inline std::string EpochKey(std::string epoch)
{
   // FormatEpoch emits zero-padded Gregorian records in the selected time
   // system. Normalize fractional trailing zeros only; retain emitted values.
   if (epoch.find('.') != std::string::npos)
   {
      while (!epoch.empty() && epoch.back() == '0') epoch.pop_back();
      if (!epoch.empty() && epoch.back() == '.') epoch.pop_back();
   }
   return epoch;
}

inline State Preview(const CCSDSEMWriter *writer,
      const std::vector<std::string> &epochs)
{
   State next;
   {
      auto &registry = History();
      std::lock_guard<std::mutex> lock(registry.mutex);
      const auto found = registry.writers.find(writer);
      if (found != registry.writers.end()) next = found->second;
   }
   if (!epochs.empty() && !next.hasMetadata)
      throw UtilityException("CCSDS OEM 1.0 record epochs require preceding metadata for ordering");
   for (const auto &epoch : epochs)
   {
      const auto key = EpochKey(epoch);
      if (!next.lastEpoch.empty())
      {
         if (key < next.lastEpoch)
            throw UtilityException("CCSDS OEM 1.0 record epochs are out of order: " +
                  epoch + " precedes " + next.lastEpoch);
         if (key == next.lastEpoch && next.metadataPosition == next.lastDataMetadataPosition)
            throw UtilityException("CCSDS OEM 1.0 record epochs must be strictly increasing "
                  "within a metadata block, including buffer chunks: " + epoch);
      }
      next.lastEpoch = key;
      next.lastDataMetadataPosition = next.metadataPosition;
   }
   return next;
}

inline void Commit(const CCSDSEMWriter *writer, const State &state)
{
   auto &registry = History();
   std::lock_guard<std::mutex> lock(registry.mutex);
   registry.writers[writer] = state;
}
}

#endif
