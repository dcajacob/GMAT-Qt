// Two-end Cartesian STM transport, keeping live spacecraft state untouched.
#ifndef OrbitStmFrame_hpp
#define OrbitStmFrame_hpp

#include "OrbitStmContext.hpp"
#include "OrbitStmOwnerFrame.hpp"
#include "CoordinateConverter.hpp"
#include "CoordinateSystem.hpp"
#include "AxisSystem.hpp"
#include "PlotOriginScope.hpp"

namespace OrbitStmFrame
{
class DetachedFrames
{
public:
   DetachedFrames(SpacePoint *original = nullptr, SpacePoint *replacement = nullptr) :
      original(original), replacement(replacement) {}

   CoordinateSystem *Copy(CoordinateSystem *source)
   {
      const auto found = frames.find(source);
      if (found != frames.end())
      {
         if (building.count(source)) throw ParameterException("Cyclic STM frame dependencies.\n");
         return found->second.get();
      }
      std::unique_ptr<CoordinateSystem> value(dynamic_cast<CoordinateSystem*>(source->Clone()));
      if (!value) throw ParameterException("Cannot clone STM frame axes.\n");
      auto *result = value.get(); frames.emplace(source, std::move(value)); building.insert(source);
      // Native coordinate Clone deliberately clears borrowed base references.
      // Rebind them before Initialize establishes private rotation buffers.
      result->SetSolarSystem(source->GetSolarSystem());
      result->SetOrigin(Rebind(source->GetOrigin())); result->SetJ2000Body(source->GetJ2000Body());
      if (source->GetPrimaryObject()) result->SetPrimaryObject(Rebind(source->GetPrimaryObject()));
      if (source->GetSecondaryObject()) result->SetSecondaryObject(Rebind(source->GetSecondaryObject()));
      if (source->GetReferenceObject()) result->SetReferenceObject(Rebind(source->GetReferenceObject()));
      auto *axes = source->GetAxisSystem(); auto *copy = result->GetAxisSystem();
      if (axes && copy)
      {
         const StringArray names = axes->GetRefObjectNameArray(Gmat::COORDINATE_SYSTEM);
         for (const auto &name : names)
            if (auto *frame = dynamic_cast<CoordinateSystem*>(axes->GetRefObject(Gmat::COORDINATE_SYSTEM, name)))
               if (!copy->SetRefObject(Copy(frame), Gmat::COORDINATE_SYSTEM, name))
                  throw ParameterException("Cannot bind a detached STM frame dependency.\n");
      }
      if (!result->Initialize()) throw ParameterException("Cannot initialize detached STM frame axes.\n");
      building.erase(source); return result;
   }
private:
   SpacePoint *Rebind(SpacePoint *object) const
   {
      return original && object == original ? replacement : object;
   }
   SpacePoint *original;
   SpacePoint *replacement;
   std::map<CoordinateSystem*, std::unique_ptr<CoordinateSystem>> frames;
   std::set<CoordinateSystem*> building;
};
inline std::vector<SpaceObject*> References(CoordinateSystem *from, CoordinateSystem *to, Spacecraft *owner)
{
   if (to->GetOrigin() == owner)
      throw ParameterException("An STM relative to its own spacecraft origin has a singular Cartesian state map.\n");
   std::vector<SpaceObject*> objects;
   PlotOriginScope::CollectReferences(from, objects);
   PlotOriginScope::CollectReferences(to, objects);
   return objects;
}
template<class Epoch>
inline Rmatrix66 Jacobian(const Epoch &epoch, CoordinateSystem *from, CoordinateSystem *to)
{
   // Axes keep rotation caches. Use detached axes values at each endpoint so
   // a rejected historical conversion cannot leave a live frame at t0.
   DetachedFrames frames; auto *input = frames.Copy(from); auto *output = frames.Copy(to);
   CoordinateConverter converter; Rvector6 state;
   if (input->GetBaseSystem() != output->GetBaseSystem())
   {
      // GetLastRotationMatrix/Rate describe the two axes only; they omit the
      // intermediate FK5/ICRF alignment used by Convert. With translation
      // omitted, conversion is linear in Cartesian state. Its six unit images
      // give the exact native Jacobian, including alignment on both rate terms.
      // This also preserves the native base-alignment velocity convention.
      Rmatrix66 result(false);
      for (Integer j = 0; j < 6; ++j)
      {
         Rvector6 basis(0,0,0,0,0,0); basis[j] = 1.0;
         if (!converter.Convert(epoch, basis, input, state, output, true, true))
            throw ParameterException("STM base-frame conversion failed.\n");
         for (Integer i = 0; i < 6; ++i)
         {
            if (!std::isfinite(state[i]))
               throw ParameterException("STM base-frame Jacobian is nonfinite.\n");
            result(i,j) = state[i];
         }
      }
      return result;
   }
   converter.Convert(epoch, Rvector6(0,0,0,0,0,0), input, state, output, true, true);
   const auto rotation = converter.GetLastRotationMatrix();
   const auto rate = converter.GetLastRotationDotMatrix();
   Rmatrix66 result(false);
   for (Integer i = 0; i < 3; ++i)
      for (Integer j = 0; j < 3; ++j)
      {
         if (!std::isfinite(rotation(i,j)) || !std::isfinite(rate(i,j)))
            throw ParameterException("STM frame rotation/rate is nonfinite.\n");
         result(i,j) = result(i+3,j+3) = rotation(i,j);
         result(i+3,j) = rate(i,j);
      }
   return result;
}
inline Rmatrix66 Convert(Spacecraft *owner, CoordinateSystem *from, CoordinateSystem *to, const Rmatrix &matrix)
{
   if (!owner || !from || !to || matrix.GetNumRows() < 6 || matrix.GetNumColumns() < 6)
      throw ParameterException("STM frame conversion requires a resolved owner, frames and six Cartesian rows/columns.\n");
   Rmatrix66 native(false);
   for (Integer i = 0; i < 6; ++i)
      for (Integer j = 0; j < 6; ++j)
      {
         if (!std::isfinite(matrix(i,j))) throw ParameterException("STM contains a nonfinite Cartesian component.\n");
         native(i,j) = matrix(i,j);
      }
   // Preserve the native unqualified MJ2000Eq route, including other origins:
   // independent origin translations do not change this Cartesian derivative.
   if (from->GetBaseSystem() == to->GetBaseSystem() && from->AreAxesOfType("MJ2000EqAxes") && to->AreAxesOfType("MJ2000EqAxes"))
   {
      if (to->GetOrigin() == owner) throw ParameterException("An STM relative to its own spacecraft origin has a singular Cartesian state map.\n");
      return native;
   }
   const auto objects = References(from, to, owner);
   const auto now = OrbitStmContext::Epoch(owner);
   for (auto *object : objects)
      if (OrbitStmContext::Epoch(object) != now)
         throw ParameterException("STM final frame references require coeval spacecraft states: " + object->GetName() + ".\n");
   const auto previousReference = OrbitStmContext::Get(owner);
   auto reference = previousReference;
   // Resolve a possible initial snapshot privately. A rejected conversion must
   // not establish or replace the spacecraft's retained reference history.
   if ((!reference.reference && OrbitStmContext::Identity(owner)) ||
       (reference.reference && !reference.advanced &&
        (reference.reference->epoch != now || !OrbitStmContext::SameState(owner, *reference.reference->self))))
      reference = OrbitStmContext::Capture(owner);
   const auto accept = [&](const Rmatrix66 &result)
   {
      if (reference.reference != previousReference.reference)
         OrbitStmContext::Commit(owner, reference);
      return result;
   };
   const bool fixed = from->GetBaseSystem() == to->GetBaseSystem() && from->AreAxesOfType("InertialAxes") && to->AreAxesOfType("InertialAxes");
   if (!reference.reference && !fixed)
      throw ParameterException("The STM initial state/epoch is unavailable for a time-dependent frame.\n");
   if (std::find(objects.begin(),objects.end(),owner)!=objects.end())
   {
      if (!reference.reference) throw ParameterException("The owner-dependent STM initial snapshot is unavailable.\n");
      // A private current snapshot and the retained initial snapshot provide
      // consistent state/acceleration values without moving any live object.
      const auto current = OrbitStmContext::Capture(owner);
      OrbitStmOwnerFrame::Matrix final, initial;
      {
         OrbitStmContext::Scope scope(current,objects);
         final = owner->HasPrecisionTime() ? OrbitStmOwnerFrame::Jacobian(owner,from,to,owner->GetEpochGT()) :
               OrbitStmOwnerFrame::Jacobian(owner,from,to,A1Mjd(owner->GetEpoch()));
      }
      {
         OrbitStmContext::Scope scope(reference,objects);
         initial = reference.reference->precise ? OrbitStmOwnerFrame::Jacobian(owner,from,to,reference.reference->epoch) :
               OrbitStmOwnerFrame::Jacobian(owner,from,to,A1Mjd(reference.reference->epoch.GetMjd()));
      }
      return accept(OrbitStmOwnerFrame::Transport(final,native,initial));
   }
   const auto final = owner->HasPrecisionTime() ? Jacobian(owner->GetEpochGT(), from, to) : Jacobian(A1Mjd(owner->GetEpoch()), from, to);
   Rmatrix66 initial(final);
   if (reference.reference)
   {
      OrbitStmContext::Scope scope(reference, objects);
      initial = reference.reference->precise ? Jacobian(reference.reference->epoch, from, to) :
            Jacobian(A1Mjd(reference.reference->epoch.GetMjd()), from, to);
   }
   const auto result = final * native * initial.Inverse();
   for (Integer i = 0; i < 6; ++i)
      for (Integer j = 0; j < 6; ++j)
         if (!std::isfinite(result(i,j))) throw ParameterException("Converted STM is nonfinite.\n");
   return accept(result);
}
}
#endif
