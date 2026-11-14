// Guarded native measurement/filter values; runtime products are rebuilt.
#ifndef NavigationMeasurementCopies_hpp
#define NavigationMeasurementCopies_hpp

#include "GmatBase.hpp"
#include "MeasurementException.hpp"
#include "FunctionValueContext.hpp"
#include <memory>
#include <algorithm>
#include <set>
#include <map>
#include <typeinfo>

namespace NavigationMeasurementCopies
{
template<class Native>
std::unique_ptr<Native> Clone(Native &source, GmatBase *owner, bool followDefinition,
      const ObjectArray &existing = {})
{
   GmatBase::FunctionValueCopyGuard nativeCopy;
   std::unique_ptr<GmatBase> value(source.Clone());
   if (value && (value.get() == &source || value.get() == owner ||
       std::find(existing.begin(), existing.end(), value.get()) != existing.end()))
   {
      value.release();
      throw MeasurementException("Measurement Clone returned a borrowed object.\n");
   }
   auto *copy = dynamic_cast<Native*>(value.get());
   if (!copy || typeid(*copy) != typeid(source) || copy->GetTypeName() != source.GetTypeName())
      throw MeasurementException("Cannot clone the native measurement definition type.\n");
   if (followDefinition) copy->SetCloneSource(&source);
   copy->SetCloneOwner(owner);
   copy->SetCloneNotificationsEnabled(false);
   copy->SetFunctionValueContext(source.GetFunctionValueContext());
   value.release();
   return std::unique_ptr<Native>(copy);
}

inline void KeepReferences(GmatBase &target, const GmatBase &source, bool whole)
{
   const auto context = source.GetFunctionValueContext();
   if (whole) target.SetFunctionValueContext(context);
   else if (context) target.RetainFunctionValueReferences(context, context->GetPrivateObjects());
}

// Map labels only after an actual object has been resolved. Equal labels from
// distinct source objects are ambiguous even if their targets share a name.
class Aliases
{
public:
   void Add(const GmatBase &source, const GmatBase &target)
   {
      AddName(source, target, source.GetName(), target.GetName());
   }

   void AddFull(GmatBase &source, GmatBase &target)
   {
      AddName(source, target, source.GetFullName(), target.GetFullName());
   }

   std::string Map(const std::string &name) const
   {
      const auto entry = values.find(name);
      return entry == values.end() ? name : entry->second.second;
   }

   std::string MapProperty(const std::string &name) const
   {
      size_t length = 0;
      const std::string *replacement = nullptr;
      for (const auto &entry : values)
         if (!entry.first.empty() && entry.first.size() > length &&
             (name == entry.first || (name.size() > entry.first.size() &&
              name.compare(0, entry.first.size(), entry.first) == 0 && name[entry.first.size()] == '.')))
         {
            length = entry.first.size();
            replacement = &entry.second.second;
         }
      return replacement ? *replacement + name.substr(length) : name;
   }

private:
   void AddName(const GmatBase &source, const GmatBase &target,
         const std::string &from, const std::string &to)
   {
      if (typeid(source) != typeid(target) || source.GetTypeName() != target.GetTypeName())
         throw MeasurementException("Incompatible retained navigation reference.\n");
      const auto entry = values.emplace(from, std::make_pair(&source, to));
      if (!entry.second && (entry.first->second.first != &source || entry.first->second.second != to))
         throw MeasurementException("Ambiguous retained navigation reference label.\n");
   }
   std::map<std::string, std::pair<const GmatBase*, std::string>> values;
};

inline std::map<std::string, StringArray> MapPropagationRoles(
      const std::map<std::string, StringArray> &source,
      const Aliases &propagators, const Aliases &participants)
{
   std::map<std::string, StringArray> result;
   for (const auto &entry : source)
   {
      auto names = entry.second;
      for (auto &name : names) name = participants.Map(name);
      if (!result.emplace(propagators.Map(entry.first), std::move(names)).second)
         throw MeasurementException("Conflicting retained navigation propagation roles.\n");
   }
   return result;
}

template<class Native>
void DeleteValues(std::vector<Native*> &values)
{
   for (size_t i = 0; i < values.size(); ++i)
   {
      auto *value = values[i];
      values[i] = nullptr;
      for (size_t j = i + 1; j < values.size(); ++j)
         if (values[j] == value) values[j] = nullptr;
      delete value;
   }
   values.clear();
}
inline void DeleteOwned(ObjectArray &values) { DeleteValues(values); }

template<class Native>
std::vector<std::unique_ptr<Native>> CloneValues(const std::vector<Native*> &source,
      GmatBase *owner, const ObjectArray &existing = {}, bool frozen = false)
{
   ObjectArray forbidden(existing);
   forbidden.insert(forbidden.end(), source.begin(), source.end());
   std::vector<std::unique_ptr<Native>> result;
   result.reserve(source.size());
   for (auto *object : source)
   {
      if (!object) throw MeasurementException("Cannot copy an unresolved native value.\n");
      auto value = Clone(*object, owner, false, forbidden);
      if (frozen)
      {
         auto *target = object->HasRestorationTarget() ? object->GetRestorationTarget() : object;
         if (!target) throw MeasurementException("The restoration destination has expired.\n");
         value->SetCloneSource(nullptr);
         value->SetRestorationTarget(target);
      }
      forbidden.push_back(value.get());
      result.push_back(std::move(value));
   }
   return result;
}

template<class Native>
std::vector<Native*> BorrowValues(const std::vector<std::unique_ptr<Native>> &values)
{
   std::vector<Native*> result;
   result.reserve(values.size());
   for (const auto &value : values) result.push_back(value.get());
   return result;
}

template<class Native>
void ReleaseValues(std::vector<std::unique_ptr<Native>> &values)
{
   for (auto &value : values) value.release();
}

template<class Native>
void PublishValues(std::vector<Native*> &target, std::vector<std::unique_ptr<Native>> &values)
{
   auto retired = BorrowValues(values);
   target.swap(retired);
   ReleaseValues(values);
   DeleteValues(retired);
}

}
#endif
