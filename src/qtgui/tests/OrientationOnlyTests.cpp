#include "CoordinateConverter.hpp"
#include "CoordinateSystem.hpp"
#include "ObjectReferencedAxes.hpp"
#include "SpacePoint.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
void require(bool condition, const char *message)
{
   if (!condition) throw std::runtime_error(message);
}

void equal(const Rmatrix33 &actual, const Rmatrix33 &expected)
{
   for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
         require(std::isfinite(actual(i,j)) &&
                 std::abs(actual(i,j) - expected(i,j)) < 3e-13,
                 "Orientation differs from prescribed geometric/vector oracle");
}

template <typename Function> void rejects(Function function)
{
   bool rejected = false;
   try { function(); } catch (BaseException &) { rejected = true; }
   require(rejected, "Invalid orientation request did not reject");
}

// Prescribed geometry; asking for acceleration is always an error. Neither
// these state vectors nor the independent basis below use a native axes method.
class Point : public SpacePoint
{
public:
   Rvector6 state;
   int accelerations = 0;
   bool failState = false;
   Point(const std::string &name, const Rvector6 &value)
      : SpacePoint(Gmat::SPACE_POINT, "OrientationPoint", name), state(value) {}
   GmatBase *Clone() const override { return new Point(*this); }
   bool HasLocalClones() override { return false; }
   bool RenameRefObject(const UnsignedInt, const std::string &, const std::string &) override { return false; }
   const Rvector6 GetMJ2000State(const A1Mjd &) override
   {
      if (failState) throw std::runtime_error("prescribed state failure");
      return state;
   }
   const Rvector6 GetMJ2000State(const GmatTime &epoch) override
   { return GetMJ2000State(A1Mjd(epoch.GetMjd())); }
   const Rvector3 GetMJ2000Position(const A1Mjd &epoch) override
   { return GetMJ2000State(epoch).GetR(); }
   const Rvector3 GetMJ2000Position(const GmatTime &epoch) override
   { return GetMJ2000State(epoch).GetR(); }
   const Rvector3 GetMJ2000Velocity(const A1Mjd &epoch) override
   { return GetMJ2000State(epoch).GetV(); }
   const Rvector3 GetMJ2000Velocity(const GmatTime &epoch) override
   { return GetMJ2000State(epoch).GetV(); }
   const Rvector3 GetMJ2000Acceleration(const A1Mjd &) override
   {
      ++accelerations;
      throw std::runtime_error("orientation requested acceleration");
   }
   const Rvector3 GetMJ2000Acceleration(const GmatTime &epoch) override
   { return GetMJ2000Acceleration(A1Mjd(epoch.GetMjd())); }
};

class FixedAxes : public AxisSystem
{
public:
   Rmatrix33 prescribed;
   int a1Calls = 0, preciseCalls = 0;
   A1Mjd a1Epoch;
   GmatTime preciseEpoch;
   FixedAxes(const Rmatrix33 &rotation, const std::string &base = "FK5")
      : AxisSystem("OrientationFixed"), prescribed(rotation) { baseSystem = base; }
   GmatBase *Clone() const override { return new FixedAxes(*this); }
   void prepare()
   {
      // This prescribed test double has no celestial references to initialize.
      // Supply the matrix views normally bound by AxisSystem::Initialize.
      rotData = rotMatrix.GetDataVector(); rotDotData = rotDotMatrix.GetDataVector();
   }
   void CalculateRotationMatrix(const A1Mjd &epoch, bool force) override
   {
      require(force, "Fallback did not force current-epoch computation");
      ++a1Calls; a1Epoch = epoch; rotMatrix = prescribed; prepare();
   }
   void CalculateRotationMatrix(const GmatTime &epoch, bool force) override
   {
      require(force, "Precise fallback did not force current-epoch computation");
      ++preciseCalls; preciseEpoch = epoch; rotMatrix = prescribed; prepare();
   }
};

class DerivedAxes : public ObjectReferencedAxes
{
public:
   int calls = 0;
   DerivedAxes() : ObjectReferencedAxes("OrientationDerived", "Derived") {}
   GmatBase *Clone() const override { return new DerivedAxes(*this); }
   void CalculateRotationMatrix(const A1Mjd &, bool) override
   {
      ++calls;
      rotMatrix.Set(0,-1,0, 1,0,0, 0,0,1);
      rotData = rotMatrix.GetDataVector(); rotDotData = rotDotMatrix.GetDataVector();
   }
};

void attach(CoordinateSystem &coordinate, AxisSystem &axes)
{
   require(coordinate.SetRefObject(&axes, Gmat::AXIS_SYSTEM, axes.GetName()),
           "Could not attach prescribed axes");
}

Rmatrix33 prescribedVNB()
{
   // r=(1,0,0), v=(0,3,4): V=(0,.6,.8), N=(0,-.8,.6), B=(1,0,0).
   return Rmatrix33(0,0,1, .6,-.8,0, .8,.6,0);
}

void geometry(ObjectReferencedAxes &axes, Point &primary, Point &secondary)
{
   axes.SetPrimaryObject(&primary); axes.SetSecondaryObject(&secondary);
   axes.ResetAxes(); axes.SetXAxis("V"); axes.SetYAxis("N");
}

void composition()
{
   Point primary("Anchor", Rvector6(10,20,30, 1,2,3));
   Point secondary("Target", Rvector6(11,20,30, 1,5,7));
   ObjectReferencedAxes object("Local"); geometry(object, primary, secondary);
   CoordinateSystem local("CoordinateSystem", "Local"), fixed("CoordinateSystem", "Fixed"); attach(local, object);
   const Rmatrix33 target(0,-1,0, 1,0,0, 0,0,1);
   FixedAxes fixedAxes(target); attach(fixed, fixedAxes);
   CoordinateConverter converter;
   const A1Mjd epoch(21545.125); const GmatTime precise(epoch.Get());
   // The expected transform has noncommuting factors and is written explicitly.
   const Rmatrix33 expected(.6,-.8,0, 0,0,-1, .8,.6,0);
   equal(converter.GetRotationMatrixOnly(epoch, &local, &fixed), expected);
   equal(converter.GetRotationMatrixOnly(precise, &local, &fixed), expected);
   equal(converter.GetRotationMatrixOnly(epoch.Get(), &local, &fixed), expected);
   equal(converter.GetRotationMatrixOnly(epoch, &fixed, &local), expected.Transpose());
   equal(converter.GetRotationMatrixOnly(epoch, &local, &local), Rmatrix33(true));
   require(primary.accelerations == 0 && secondary.accelerations == 0,
           "Geometry queried prescribed acceleration");

   // Seed a genuine last conversion, then verify success AND error calls leave
   // converter R/Rdot untouched. The API intentionally returns a value instead.
   CoordinateSystem other("CoordinateSystem", "Other"); FixedAxes otherAxes(Rmatrix33(true)); attach(other, otherAxes);
   const Real input[6] = {1,2,3,4,5,6}; Real output[6];
   converter.Convert(epoch, input, &fixed, output, &other, true, true);
   const auto savedR = converter.GetLastRotationMatrix();
   const auto savedD = converter.GetLastRotationDotMatrix();
   equal(savedR, target);
   equal(converter.GetRotationMatrixOnly(epoch, &local, &fixed), expected);
   rejects([&] { converter.GetRotationMatrixOnly(epoch, nullptr, &local); });
   rejects([&] { converter.GetRotationMatrixOnly(precise, &local, nullptr); });
   rejects([&] { converter.GetRotationMatrixOnly(epoch.Get(), nullptr, nullptr); });
   require(converter.GetLastRotationMatrix() == savedR &&
           converter.GetLastRotationDotMatrix() == savedD, "Orientation replaced last-conversion data");
}

void invalidGeometry()
{
   Point primary("Anchor", Rvector6(0,0,0, 0,0,0));
   Point secondary("Target", Rvector6(1,0,0, 0,3,4));
   ObjectReferencedAxes axes("Local"); geometry(axes, primary, secondary);
   const auto cachedR = axes.GetLastRotationMatrix(), cachedD = axes.GetLastRotationDotMatrix();
   const A1Mjd epoch(21545);
   auto unchanged = [&] {
      require(axes.GetLastRotationMatrix() == cachedR && axes.GetLastRotationDotMatrix() == cachedD,
              "Failed geometry changed cached matrices");
   };
   ObjectReferencedAxes missing("Missing");
   rejects([&] { missing.GetRotationMatrixOnly(epoch); });
   axes.SetZAxis("R"); rejects([&] { axes.GetRotationMatrixOnly(epoch); }); unchanged();
   axes.ResetAxes(); axes.SetXAxis("V"); axes.SetYAxis("V");
   rejects([&] { axes.GetRotationMatrixOnly(epoch); }); unchanged();
   geometry(axes, primary, secondary);
   const auto valid = secondary.state;
   for (const auto &bad : {Rvector6(0,0,0,0,3,4), Rvector6(1,0,0,0,0,0), Rvector6(1,0,0,2,0,0),
                          Rvector6(std::numeric_limits<double>::quiet_NaN(),0,0,0,3,4),
                          Rvector6(1,0,0,0,std::numeric_limits<double>::infinity(),4)})
   {
      secondary.state = bad;
      rejects([&] { axes.GetRotationMatrixOnly(epoch); }); unchanged();
   }
   secondary.state = valid; secondary.failState = true; bool propagated = false;
   try { axes.GetRotationMatrixOnly(epoch); }
   catch (const std::runtime_error &error) { propagated = std::string(error.what()) == "prescribed state failure"; }
   require(propagated, "Geometry swallowed a state-provider failure"); unchanged();
   secondary.failState = false;
   equal(axes.GetRotationMatrixOnly(epoch), prescribedVNB()); unchanged();
   require(primary.accelerations == 0 && secondary.accelerations == 0,
           "Invalid geometry queried acceleration");
}

void dispatch()
{
   CoordinateSystem first("CoordinateSystem", "First"), second("CoordinateSystem", "Second");
   FixedAxes input(Rmatrix33(true)), output(Rmatrix33(0,-1,0,1,0,0,0,0,1));
   attach(first, input); attach(second, output);
   auto *from = dynamic_cast<FixedAxes *>(first.GetAxisSystem());
   auto *to = dynamic_cast<FixedAxes *>(second.GetAxisSystem());
   require(from && to, "Axes clone lost its dynamic type");
   CoordinateConverter converter; const A1Mjd epoch(21545.2);
   GmatTime precise(epoch.Get()); precise.AddSeconds(.0000001);
   equal(converter.GetRotationMatrixOnly(epoch, &first, &second), output.prescribed.Transpose());
   equal(converter.GetRotationMatrixOnly(precise, &first, &second), output.prescribed.Transpose());
   require(from->a1Calls == 1 && to->a1Calls == 1 && from->preciseCalls == 1 && to->preciseCalls == 1 &&
           from->a1Epoch.Get() == epoch.Get() && to->a1Epoch.Get() == epoch.Get() &&
           from->preciseEpoch == precise && to->preciseEpoch == precise,
           "Fallback lost virtual dispatch or precise epoch");
   DerivedAxes derived; rejects([&] { derived.GetRotationMatrixOnly(epoch); });
   CoordinateSystem specialized("CoordinateSystem", "Specialized"); attach(specialized, derived);
   equal(converter.GetRotationMatrixOnly(epoch, &specialized, &first), output.prescribed);
   auto *actual = dynamic_cast<DerivedAxes *>(specialized.GetAxisSystem());
   require(actual && actual->calls == 1, "Derived object axes bypassed their virtual calculation");
   CoordinateSystem unknown("CoordinateSystem", "Unknown"); FixedAxes unsupported(Rmatrix33(true), "Unsupported"); attach(unknown, unsupported);
   rejects([&] { converter.GetRotationMatrixOnly(epoch, &unknown, &first); });
   rejects([&] { converter.GetRotationMatrixOnly(precise, &first, &unknown); });
}

void baseAlignment()
{
   CoordinateSystem fk5("CoordinateSystem", "FK5"), icrf("CoordinateSystem", "ICRF");
   FixedAxes from(Rmatrix33(0,0,1,1,0,0,0,1,0));
   FixedAxes to(Rmatrix33(0,-1,0,1,0,0,0,0,1), "ICRF");
   attach(fk5, from); attach(icrf, to);
   CoordinateConverter converter; const A1Mjd epoch(21545.25); const GmatTime precise(epoch.Get());
   // Compare each returned matrix column to an actual native state conversion,
   // including base alignment. GetLastRotationMatrix on that legacy conversion
   // omits base alignment, so it is deliberately NOT the oracle here.
   for (bool reverse : {false, true})
   {
      auto *input = reverse ? &icrf : &fk5; auto *output = reverse ? &fk5 : &icrf;
      const auto rotation = converter.GetRotationMatrixOnly(epoch, input, output);
      equal(converter.GetRotationMatrixOnly(precise, input, output), rotation);
      equal(converter.GetRotationMatrixOnly(epoch.Get(), input, output), rotation);
      for (int column = 0; column < 3; ++column)
      {
         Real vector[6] = {}; vector[column] = 1; Real converted[6];
         converter.Convert(epoch, vector, input, converted, output, true, true);
         for (int row = 0; row < 3; ++row)
         {
            if (std::abs(rotation(row,column) - converted[row]) >= 3e-13)
               std::cerr << "Base alignment reverse=" << reverse << " row=" << row
                         << " column=" << column << " rotation=" << rotation(row,column)
                         << " vector=" << converted[row] << '\n';
            require(std::abs(rotation(row,column) - converted[row]) < 3e-13,
                    "Rotation-only matrix disagrees with base-aligned vector conversion");
         }
      }
   }
}
}

int main(int argc, char **argv)
{
   try
   {
      require(argc == 3, "Supply startup and orientation case");
      const auto startup = std::filesystem::absolute(argv[1]);
      std::filesystem::current_path(startup.parent_path());
      FileManager::Instance()->ReadStartupFile(startup.string());
      const std::string key = argv[2];
      if (key == "Composition") composition();
      else if (key == "InvalidGeometry") invalidGeometry();
      else if (key == "Dispatch") dispatch();
      else if (key == "BaseAlignment") baseAlignment();
      else throw std::runtime_error("Unknown orientation case");
      std::cout << "PASS orientation-only " << key << '\n'; return 0;
   }
   catch (BaseException &error) { std::cerr << error.GetFullMessage() << '\n'; }
   catch (const std::exception &error) { std::cerr << error.what() << '\n'; }
   return 1;
}
