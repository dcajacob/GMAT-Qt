// Captured membership of a borrowed native transient-force collection.
#ifndef TransientForceSelection_hpp
#define TransientForceSelection_hpp

#include "PhysicalModel.hpp"
#include "CommandException.hpp"
#include <memory>
#include <vector>

class TransientForceSelection
{
   struct Entry
   {
      PhysicalModel *object;
      std::weak_ptr<const void> identity;
   };
   std::vector<PhysicalModel*> *context = nullptr;
   std::vector<Entry> entries;
   bool captured = false;
public:
   bool IsCaptured() const noexcept { return captured; }

   void Clear() noexcept
   {
      entries.clear();
      context = nullptr;
      captured = false;
   }

   void Capture(std::vector<PhysicalModel*> *source)
   {
      std::vector<Entry> values;
      if (source)
      {
         values.reserve(source->size());
         for (auto *force : *source)
         {
            if (!force) throw CommandException("Cannot capture a NULL transient force.");
            values.push_back({force, force->GetLifetimeToken()});
         }
      }
      entries.swap(values);
      context = source;
      captured = true;
   }

   bool Matches(const std::vector<PhysicalModel*> *source) const
   {
      if (!captured || source != context) return false;
      if (!source) return true;
      if (source->size() != entries.size()) return false;
      for (size_t i = 0; i < entries.size(); ++i)
         if ((*source)[i] != entries[i].object ||
               !GmatBase::IsLiveReference(entries[i].object, entries[i].identity))
            return false;
      return true;
   }

   // Stage before changing any live checkpoint target. Membership is borrowed:
   // neither cloning a force nor resolving another object by name is valid.
   std::vector<PhysicalModel*> PrepareRestoration(
         std::vector<PhysicalModel*> *destination) const
   {
      if (!captured || destination != context)
         throw CommandException("The captured transient-force context is no longer selected.");
      std::vector<PhysicalModel*> result;
      result.reserve(entries.size());
      for (const auto &entry : entries)
      {
         if (!GmatBase::IsLiveReference(entry.object, entry.identity))
            throw CommandException("The original captured transient force is no longer alive.");
         result.push_back(entry.object);
      }
      return result;
   }
};
#endif
