// Internal STM reference ownership and a read-only historical frame state scope.
// No public spacecraft layout or virtual method order changes.
#ifndef OrbitStmContext_hpp
#define OrbitStmContext_hpp

#include "Spacecraft.hpp"
#include "ODEModel.hpp"
#include "ParameterException.hpp"
#include <cstdint>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

namespace OrbitStmContext
{
// Internal optional query on the existing acceleration route. No public ODE
// interface or spacecraft layout changes are needed to retain a linearization.
class GradientQuery
{
public:
   explicit GradientQuery(Spacecraft *object) : value(3,6), target(object), previous(Active())
   { Active() = this; }
   ~GradientQuery() { Active() = previous; }
   GradientQuery(const GradientQuery &) = delete;
   GradientQuery &operator=(const GradientQuery &) = delete;
   static GradientQuery *For(Spacecraft *object)
   { auto *query = Active(); return query && query->target == object ? query : nullptr; }
   bool available = false;
   Rmatrix value;
private:
   static GradientQuery *&Active() { static thread_local GradientQuery *query = nullptr; return query; }
   Spacecraft *target;
   GradientQuery *previous;
};
struct Sample
{
   std::uint64_t generation = 0;
   Rvector6 state;
   bool accelerationAvailable = false;
   Rvector3 acceleration;
   SpacePoint *accelerationOrigin = nullptr;
   std::shared_ptr<const Rmatrix> accelerationGradient;
};
struct Reference
{
   GmatTime epoch;
   bool precise = false;
   std::shared_ptr<Sample> self;
   std::map<SpaceObject*, std::shared_ptr<Sample>> world;
};
struct Value
{
   std::shared_ptr<const Reference> reference;
   bool advanced = false;
};
struct Entry
{
   std::uint64_t generation = 0;
   Value value;
};
struct Registry
{
   std::mutex mutex;
   std::uint64_t next = 1;
   std::map<const SpaceObject*, Entry> objects;
};
inline Registry &History() { static auto *value = new Registry; return *value; }
inline GmatTime Epoch(SpaceObject *object)
{ return object->HasPrecisionTime() ? object->GetEpochGT() : GmatTime(object->GetEpoch()); }
inline Value Get(const SpaceObject *object)
{
   auto &history = History(); std::lock_guard<std::mutex> lock(history.mutex);
   const auto found = history.objects.find(object);
   return found == history.objects.end() ? Value{} : found->second.value;
}
inline std::uint64_t Generation(const SpaceObject *object)
{
   auto &history = History(); std::lock_guard<std::mutex> lock(history.mutex);
   const auto found = history.objects.find(object);
   return found == history.objects.end() ? 0 : found->second.generation;
}
inline void Register(SpaceObject *object, const SpaceObject *source = nullptr)
{
   auto &history = History(); std::lock_guard<std::mutex> lock(history.mutex);
   Value value;
   const auto from = history.objects.find(source);
   if (from != history.objects.end()) value = from->second.value;
   history.objects.emplace(object, Entry{history.next++, value});
}
inline void Forget(SpaceObject *object)
{
   auto &history = History(); std::lock_guard<std::mutex> lock(history.mutex);
   history.objects.erase(object);
}
inline void Commit(SpaceObject *object, Value value)
{
   auto &history = History(); std::lock_guard<std::mutex> lock(history.mutex);
   const auto found = history.objects.find(object);
   if (found == history.objects.end()) throw ParameterException("STM reference owner is not live.\n");
   found->second.value = std::move(value);
}
inline bool Identity(const Rmatrix &matrix)
{
   if (matrix.GetNumRows() < 6 || matrix.GetNumRows() != matrix.GetNumColumns()) return false;
   for (Integer i = 0; i < matrix.GetNumRows(); ++i)
      for (Integer j = 0; j < matrix.GetNumColumns(); ++j)
         if (matrix(i,j) != (i == j ? 1.0 : 0.0)) return false;
   return true;
}
inline bool Identity(Spacecraft *object) { return Identity(object->GetRmatrixParameter("OrbitSTM")); }
inline bool SameState(SpaceObject *object, const Sample &sample)
{
   const auto &state = object->GetState();
   if (state.GetSize() < 6) return false;
   for (Integer i = 0; i < 6; ++i) if (state[i] != sample.state[i]) return false;
   return true;
}
inline bool Acceleration(Spacecraft *object, Rvector3 &value, SpacePoint *&origin,
      std::shared_ptr<const Rmatrix> *gradient = nullptr)
{
   origin = object->GetJ2000Body();
   if (!origin) return false;
   try
   {
      GradientQuery query(gradient ? object : nullptr);
      const bool ready = ODEModel::GetBoundRelativeAcceleration(object, origin, A1Mjd(object->GetEpoch()), value);
      if (ready && gradient && query.available) *gradient = std::make_shared<Rmatrix>(query.value);
      return ready;
   }
   catch (BaseException &) { return false; } // The provider may not yet be initialized.
}
inline Value Capture(Spacecraft *owner)
{
   auto result = std::make_shared<Reference>(); result->epoch = Epoch(owner); result->precise = owner->HasPrecisionTime();
   std::vector<std::pair<SpaceObject*, std::uint64_t>> objects;
   {
      auto &history = History(); std::lock_guard<std::mutex> lock(history.mutex);
      for (const auto &entry : history.objects)
         objects.emplace_back(const_cast<SpaceObject*>(entry.first), entry.second.generation);
   }
   // Native object construction/propagation is synchronous. Keep registry locks
   // out of force queries, which have their own context lock.
   for (const auto &entry : objects)
   {
      auto *object = dynamic_cast<Spacecraft*>(entry.first);
      if (!object || Epoch(object) != result->epoch || object->GetState().GetSize() < 6) continue;
      auto sample = std::make_shared<Sample>(); sample->generation = entry.second;
      bool finite = true;
      for (Integer i = 0; i < 6; ++i)
      { sample->state[i] = object->GetState()[i]; finite = finite && std::isfinite(sample->state[i]); }
      if (!finite) continue;
      sample->accelerationAvailable = Acceleration(object, sample->acceleration, sample->accelerationOrigin,
            object == owner ? &sample->accelerationGradient : nullptr);
      result->world.emplace(object, sample);
      if (object == owner) result->self = sample;
   }
   if (!result->self) throw ParameterException("STM reference requires a finite six-component owner state.\n");
   return Value{result, false};
}
inline void Ensure(Spacecraft *owner, bool assembly = false)
{
   auto value = Get(owner);
   if (!value.reference)
   {
      if (Identity(owner)) Commit(owner, Capture(owner));
      return;
   }
   if (!value.advanced && (assembly || value.reference->epoch != Epoch(owner) || !SameState(owner, *value.reference->self)))
      Commit(owner, Capture(owner));
}
inline void Advanced(Spacecraft *owner)
{
   auto value = Get(owner);
   if (value.reference && (value.reference->epoch != Epoch(owner) || !Identity(owner)))
   { value.advanced = true; Commit(owner, std::move(value)); }
}
inline void FillAccelerations(Spacecraft *object)
{
   std::vector<std::shared_ptr<Sample>> pending;
   std::set<Sample*> seen;
   const auto now = Epoch(object); const auto generation = Generation(object);
   std::vector<std::pair<Spacecraft*, Value>> values;
   {
      auto &history = History(); std::lock_guard<std::mutex> lock(history.mutex);
      for (const auto &entry : history.objects)
         if (auto *owner = dynamic_cast<Spacecraft*>(const_cast<SpaceObject*>(entry.first))) values.emplace_back(owner, entry.second.value);
   }
   for (const auto &entry : values)
   {
      const auto &value = entry.second;
      // A later provider must not manufacture an unavailable historical rate.
      if (!value.reference || value.advanced || value.reference->epoch != now || Epoch(entry.first) != now) continue;
      const auto found = value.reference->world.find(object);
      if (found == value.reference->world.end()) continue;
      const auto &sample = found->second;
      if (sample->generation == generation && !sample->accelerationAvailable && SameState(object, *sample) && seen.insert(sample.get()).second)
         pending.push_back(sample);
   }
   if (pending.empty()) return;
   Rvector3 acceleration; SpacePoint *origin = nullptr;
   std::shared_ptr<const Rmatrix> gradient;
   if (!Acceleration(object, acceleration, origin, &gradient)) return;
   for (auto &sample : pending)
   {
      sample->acceleration = acceleration; sample->accelerationOrigin = origin;
      sample->accelerationAvailable = true; sample->accelerationGradient = gradient;
   }
}
inline Sample Historical(const Value &owner, SpaceObject *object)
{
   const auto generation = Generation(object);
   if (!generation || !owner.reference) throw ParameterException("STM historical frame reference is not live.\n");
   const auto found = owner.reference->world.find(object);
   if (found != owner.reference->world.end() && found->second->generation == generation) return *found->second;
   const auto own = Get(object);
   if (own.reference && own.reference->epoch == owner.reference->epoch)
   { auto sample = *own.reference->self; sample.generation = generation; return sample; }
   throw ParameterException("No retained STM frame-reference state at its initial epoch for " + object->GetName() + ".\n");
}
inline Value Rebound(const Value &source, const std::function<GmatBase*(GmatBase*)> &resolve)
{
   if (!source.reference) return source;
   auto result = std::make_shared<Reference>(*source.reference); result->world.clear();
   for (const auto &entry : source.reference->world)
   {
      if (Generation(entry.first) != entry.second->generation) continue;
      auto *mapped = dynamic_cast<SpaceObject*>(resolve(entry.first));
      if (!mapped || !Generation(mapped)) continue;
      auto sample = std::make_shared<Sample>(*entry.second); sample->generation = Generation(mapped);
      if (sample->accelerationOrigin) sample->accelerationOrigin = dynamic_cast<SpacePoint*>(resolve(sample->accelerationOrigin));
      result->world[mapped] = sample;
   }
   return Value{result, source.advanced};
}

class Scope
{
public:
   Scope(const Value &reference, const std::vector<SpaceObject*> &objects) : previous(Active()), epoch(reference.reference->epoch)
   { for (auto *object : objects) samples.emplace(object, Historical(reference, object)); Active() = this; }
   ~Scope() { Active() = previous; }
   Scope(const Scope &) = delete;
   Scope &operator=(const Scope &) = delete;
   static const Rvector6 *Find(const SpaceObject *object, const GmatTime &at)
   { auto *scope = Active(); if (!scope || scope->epoch != at) return nullptr; auto i = scope->samples.find(object); return i == scope->samples.end() ? nullptr : &i->second.state; }
   static const Rvector6 *Find(const SpaceObject *object, Real mjd)
   { auto *scope = Active(); return scope && scope->epoch.GetMjd() == mjd ? Find(object, scope->epoch) : nullptr; }
   static bool EpochFor(const SpaceObject *object, Real mjd, GmatTime &at)
   { auto *scope = Active(); if (!scope || scope->epoch.GetMjd() != mjd || !scope->samples.count(object)) return false; at = scope->epoch; return true; }
   static bool Has(const SpaceObject *object) { auto *scope = Active(); return scope && scope->samples.count(object); }
   static const Sample &SampleOf(const SpaceObject *object)
   {
      auto *scope = Active();
      if (!scope || !scope->samples.count(object)) throw ParameterException("STM frame state snapshot is unavailable.\n");
      return scope->samples.at(object);
   }
   static const Rmatrix &GradientOf(const SpaceObject *object)
   {
      const auto &sample = SampleOf(object);
      if (!sample.accelerationGradient)
         throw ParameterException("A captured acceleration gradient is required for this owner-dependent STM frame.\n");
      return *sample.accelerationGradient;
   }
   static Rvector3 AccelerationOf(const SpaceObject *object, SpacePoint *&origin)
   {
      auto *scope = Active();
      if (!scope || !scope->samples.count(object)) throw ParameterException("STM initial frame acceleration requires its retained reference object.\n");
      const auto &sample = scope->samples.at(object);
      if (!sample.accelerationAvailable || !sample.accelerationOrigin)
         throw ParameterException("STM initial frame acceleration was unavailable at its reference epoch for " + object->GetName() + ".\n");
      origin = sample.accelerationOrigin; return sample.acceleration;
   }
private:
   static const Scope *&Active() { static thread_local const Scope *value = nullptr; return value; }
   const Scope *previous;
   GmatTime epoch;
   std::map<const SpaceObject*, Sample> samples;
};
}
#endif
