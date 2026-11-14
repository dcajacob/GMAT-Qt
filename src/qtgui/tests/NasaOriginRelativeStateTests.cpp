#include "Spacecraft.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "CoordinateSystem.hpp"
#include "CoordinateConverter.hpp"
#include "CartesianParameters.hpp"
#include "Publisher.hpp"
#include "Subscriber.hpp"
#include "BaseException.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

static int checks = 0;
static void require(bool value, const char *message)
{
   if (!value) throw std::runtime_error(message);
   ++checks;
}
class InputProbe : public Spacecraft
{
public:
   InputProbe() : Spacecraft("OriginInputProbe") {}
   void input(CoordinateSystem *frame, Rvector6 value)
   {
      coordinateSystem = frame;
      SetStateFromRepresentation("Cartesian", value);
   }
};
// A distinct ephemeris is a deliberate native metadata fixture, not a science oracle.
class ShiftedBody : public CelestialBody
{
public:
   Rvector6 value;
   bool fail = false;
   ShiftedBody(const CelestialBody &body, const Rvector6 &state)
      : CelestialBody(body), value(state) { value[0] = std::nextafter(value[0],
           std::numeric_limits<double>::infinity()); }
   const Rvector6 &GetState(GmatTime) override { if (fail) throw std::runtime_error("intentional ephemeris failure"); return value; }
   GmatBase *Clone() const override { return new ShiftedBody(*this); }
};
class SamplePublisher : public Publisher {};
class ParameterSink : public Subscriber
{
public:
   CartX &parameter;
   Real sourceValue, sampleValue;
   int deliveries = 0;
   ParameterSink(CartX &p, Real source, Real sample)
      : Subscriber("LocalSampleSink", "LocalSampleSink"), parameter(p),
        sourceValue(source), sampleValue(sample) {}
   GmatBase *Clone() const override { return new ParameterSink(*this); }
   bool ReceiveData(const char*) override { return true; }
   bool ReceiveData(const Real *data, const Integer count = 0) override
   {
      require(parameter.EvaluateReal() == sourceValue,
              "Live parameter did not retain local state outside report scope");
      {
         Publisher::ParameterStateScope scope(data, count);
         require(parameter.EvaluateReal() == sampleValue,
                 "Local cache overrode canonical report publication sample");
      }
      require(parameter.EvaluateReal() == sourceValue,
              "Report scope changed live local parameter");
      ++deliveries;
      return true;
   }
};
int main(int argc, char **argv)
{
   try
   {
      require(argc == 2, "Supply origin-state contract startup file");
      const auto startup = std::filesystem::absolute(argv[1]);
      std::filesystem::current_path(startup.parent_path());
      auto *moderator = Moderator::Instance();
      require(moderator->Initialize(startup.string(), false), "Native initialization failed");
      auto *solar = moderator->GetSolarSystemInUse();
      auto *earth = solar->GetBody("Earth");
      auto *pluto = solar->GetBody("Pluto");
      auto *uranus = solar->GetBody("Uranus");
      auto *internal = moderator->GetInternalCoordinateSystem();
      require(earth && pluto && uranus && internal &&
              internal->AreAxesOfType("MJ2000EqAxes"), "Native frame fixture absent");
      std::unique_ptr<CoordinateSystem> relative(CoordinateSystem::CreateLocalCoordinateSystem(
         "ContractPluto", "MJ2000Eq", pluto, nullptr, nullptr, earth, solar));
      std::unique_ptr<CoordinateSystem> ecliptic(CoordinateSystem::CreateLocalCoordinateSystem(
         "ContractPlutoEc", "MJ2000Ec", pluto, nullptr, nullptr, earth, solar));
      InputProbe spacecraft;
      spacecraft.SetInternalCoordSystem(internal);
      const GmatTime epoch = spacecraft.GetEpochGT();
      const Rvector6 local(1000.123456789, -0.000000123456789, 3.141592653589793,
                           0.125123456789, -0.03123456789, 0.0000001234567);
      const Rvector6 sentinel(-1, -2, -3, -4, -5, -6);
      Rvector6 result = sentinel;
      require(!spacecraft.GetOriginRelativeState(pluto, epoch, result) && result == sentinel,
              "Empty cache changed output");
      spacecraft.input(relative.get(), local);
      for (bool precise : {true, false})
      {
         spacecraft.SetPrecisionTimeFlag(precise);
         require(spacecraft.GetState("Cartesian") == local &&
                 spacecraft.GetOriginRelativeState(pluto, epoch, result) && result == local,
                 "Epoch precision toggle lost the retained local input");
      }
      const Rvector6 high(spacecraft.GetState().GetState());
      require(spacecraft.GetOriginRelativeState(pluto, epoch, result) && result == local,
              "Configured local input lost precision");
      require(spacecraft.GetState("Cartesian") == local,
              "Configured local readback used rounded absolute coordinates");
      CoordinateConverter converter;
      Rvector6 rounded;
      converter.Convert(epoch, high, internal, rounded, relative.get());
      require(rounded != local, "Large-origin rounding fixture did not expose information loss");
      require(Rvector6(spacecraft.GetState().GetState()) == high,
              "Local readback changed the ordinary state");
      result = sentinel;
      GmatTime otherEpoch = epoch; otherEpoch.AddSeconds(1e-8);
      require(!spacecraft.GetOriginRelativeState(uranus, epoch, result) && result == sentinel,
              "Wrong origin returned a state");
      require(!spacecraft.GetOriginRelativeState(pluto, otherEpoch, result) && result == sentinel,
              "Wrong precise epoch returned a state");
      require(spacecraft.GetOriginRelativeState(pluto, epoch, result) && result == local,
              "Unrelated query discarded a valid state");
      std::unique_ptr<GmatBase> sameName(pluto->Clone());
      require(!spacecraft.GetOriginRelativeState(dynamic_cast<SpacePoint*>(sameName.get()),
                                               epoch, result),
              "Same-name distinct origin matched cached identity");
      CartX parameter("OriginInputProbe.ContractPluto.X", &spacecraft);
      parameter.SetSolarSystem(solar);
      parameter.SetInternalCoordSystem(internal);
      parameter.SetRefObjectName(Gmat::COORDINATE_SYSTEM, relative->GetName());
      require(parameter.AddRefObject(relative.get(), true) && parameter.Initialize(),
              "Cartesian report parameter initialization failed");
      require(parameter.EvaluateReal() == local[0],
              "Actual Cartesian parameter lost local input precision");
      Rvector6 sampleState = high; sampleState[0] += 123.0;
      GmatTime sampleEpoch = epoch; sampleEpoch.AddSeconds(20e-9);
      Rvector6 sampleResult;
      converter.Convert(sampleEpoch.GetMjd(), sampleState, internal,
                        sampleResult, relative.get(), true);
      SamplePublisher publisher;
      ParameterSink sink(parameter, local[0], sampleResult[0]);
      require(publisher.Subscribe(&sink), "Parameter sample subscriber failed");
      const auto stream = publisher.RegisterPublishedData(&spacecraft, -1,
         StringArray{"OriginInputProbe", "OriginInputProbe"},
         StringArray{"Epoch", "OriginInputProbe.X"});
      Real packet[2] = {epoch.GetMjd(), high[0]};
      require(publisher.PublishWithSamples(&spacecraft, stream, packet, 2, epoch,
                 std::vector<Publisher::CartesianSample>{{&spacecraft, sampleEpoch, sampleState}})
              && sink.deliveries == 1, "Report sample precedence publication failed");
      publisher.Unsubscribe(&sink);
      require(Rvector6(spacecraft.GetState().GetState()) == high &&
              spacecraft.GetOriginRelativeState(pluto, epoch, result) && result == local,
              "Publication changed live high or local state");
      std::unique_ptr<GmatBase> earthClone(earth->Clone());
      require(internal->SetRefObject(earthClone.get(), Gmat::SPACE_POINT, earth->GetName()),
              "Internal-origin clone rebind fixture failed");
      require(spacecraft.GetOriginRelativeState(pluto, epoch, result) && result == local,
              "Equivalent cloned internal origin discarded local state");
      require(internal->SetRefObject(earth, Gmat::SPACE_POINT, earth->GetName()) &&
              spacecraft.GetOriginRelativeState(pluto, epoch, result) && result == local,
              "Restored internal origin discarded local state");
      auto *plutoClone = dynamic_cast<SpacePoint*>(sameName.get());
      require(relative->SetRefObject(plutoClone, Gmat::SPACE_POINT, pluto->GetName()) &&
              spacecraft.GetOriginRelativeState(plutoClone, epoch, result) && result == local,
              "Configured-frame cloned origin discarded local state");
      require(relative->SetRefObject(pluto, Gmat::SPACE_POINT, pluto->GetName()) &&
              spacecraft.GetOriginRelativeState(pluto, epoch, result) && result == local,
              "Restored configured-frame origin discarded local state");
      ShiftedBody changedEarth(*earth, earth->GetState(epoch));
      require(internal->SetRefObject(&changedEarth, Gmat::SPACE_POINT, earth->GetName()),
              "Changed internal-origin fixture failed");
      result = sentinel;
      require(!spacecraft.GetOriginRelativeState(pluto, epoch, result) && result == sentinel,
              "Different ephemeris with the same internal-origin name was accepted");
      require(internal->SetRefObject(earth, Gmat::SPACE_POINT, earth->GetName()) &&
              !spacecraft.GetOriginRelativeState(pluto, epoch, result),
              "Restored origin resurrected a rejected ephemeris cache");
      require(spacecraft.SetOriginRelativeState(pluto, epoch, local), "Cache setup failed");
      ShiftedBody changedPluto(*pluto, pluto->GetState(epoch));
      require(relative->SetRefObject(&changedPluto, Gmat::SPACE_POINT, pluto->GetName()),
              "Changed configured-origin fixture failed");
      result = sentinel;
      require(!spacecraft.GetOriginRelativeState(&changedPluto, epoch, result) && result == sentinel,
              "Different ephemeris with the same configured-origin name was accepted");
      require(relative->SetRefObject(pluto, Gmat::SPACE_POINT, pluto->GetName()) &&
              spacecraft.GetOriginRelativeState(pluto, epoch, result) && result == local,
              "Rejected unrelated origin destroyed valid original-body state");
      ShiftedBody failedEphemeris(*pluto, pluto->GetState(epoch));
      failedEphemeris.fail = true;
      bool ephemerisThrew = false;
      try { spacecraft.SetOriginRelativeState(&failedEphemeris, epoch, local); }
      catch (const std::runtime_error &error)
      { ephemerisThrew = std::string(error.what()) == "intentional ephemeris failure"; }
      require(ephemerisThrew && spacecraft.GetOriginRelativeState(pluto, epoch, result) &&
              result == local, "Ephemeris exception partially changed cached metadata");
      failedEphemeris.fail = false;
      const auto finiteEphemeris = failedEphemeris.value;
      for (int i = 0; i < 6; ++i)
      {
         failedEphemeris.value = finiteEphemeris;
         failedEphemeris.value[i] = std::numeric_limits<double>::quiet_NaN();
         require(!spacecraft.SetOriginRelativeState(&failedEphemeris, epoch, local) &&
                 spacecraft.GetOriginRelativeState(pluto, epoch, result) && result == local,
                 "Nonfinite ephemeris accepted or partially changed cached metadata");
      }
      Spacecraft copied(spacecraft), assigned("Assigned"); assigned = spacecraft;
      std::unique_ptr<GmatBase> cloned(spacecraft.Clone());
      for (auto *copy : {&copied, &assigned, dynamic_cast<Spacecraft*>(cloned.get())})
         require(copy && copy->GetOriginRelativeState(pluto, epoch, result) && result == local,
                 "Copy, assignment or Clone lost a valid local state");
      for (int i = 0; i < 6; ++i)
      {
         require(spacecraft.SetOriginRelativeState(pluto, epoch, local), "Cache setup failed");
         spacecraft.GetState()[i] = std::nextafter(high[i], std::numeric_limits<double>::infinity());
         result = sentinel;
         require(!spacecraft.GetOriginRelativeState(pluto, epoch, result) && result == sentinel,
                 "Mutable backing component did not invalidate cache");
         spacecraft.GetState()[i] = high[i];
         require(!spacecraft.GetOriginRelativeState(pluto, epoch, result),
                 "Restored backing resurrected an invalid cache");
      }
      require(spacecraft.SetOriginRelativeState(pluto, epoch, local), "Cache setup failed");
      spacecraft.GetState().SetEpochGT(otherEpoch);
      require(!spacecraft.GetOriginRelativeState(pluto, otherEpoch, result),
              "Changed epoch retained a stale local state");
      spacecraft.GetState().SetEpochGT(epoch);
      require(!spacecraft.GetOriginRelativeState(pluto, epoch, result),
              "Restored epoch resurrected stale local state");
      require(spacecraft.SetOriginRelativeState(pluto, epoch, local), "Cache setup failed");
      spacecraft.SetInternalCoordSystem(ecliptic.get());
      spacecraft.SetInternalCoordSystem(internal);
      require(!spacecraft.GetOriginRelativeState(pluto, epoch, result),
              "Frame changes retained stale local state");
      require(spacecraft.SetOriginRelativeState(pluto, epoch, local), "Cache setup failed");
      for (int i = 0; i < 6; ++i)
         for (double invalid : {std::numeric_limits<double>::quiet_NaN(),
                                std::numeric_limits<double>::infinity(),
                                -std::numeric_limits<double>::infinity()})
         {
            Rvector6 bad = local; bad[i] = invalid;
            require(!spacecraft.SetOriginRelativeState(pluto, epoch, bad) &&
                    spacecraft.GetOriginRelativeState(pluto, epoch, result) && result == local,
                    "Nonfinite input accepted or valid cache changed on rejection");
         }
      require(!spacecraft.SetOriginRelativeState(nullptr, epoch, local) &&
              !spacecraft.SetOriginRelativeState(earth, epoch, local) &&
              !spacecraft.SetOriginRelativeState(pluto, otherEpoch, local) &&
              spacecraft.GetOriginRelativeState(pluto, epoch, result) && result == local,
              "Invalid origin or epoch changed the cache");
      spacecraft.SetState(high);
      require(!spacecraft.GetOriginRelativeState(pluto, epoch, result),
              "Explicit ordinary-state setter retained stale local information");
      spacecraft.input(ecliptic.get(), local);
      Rvector6 rotated;
      converter.Convert(epoch, local, ecliptic.get(), rotated, internal, false, true);
      require(spacecraft.GetOriginRelativeState(pluto, epoch, result) && result == rotated,
              "Input frame rotation lost its origin-relative state");
      const auto readback = spacecraft.GetState("Cartesian");
      for (int i = 0; i < 6; ++i)
         require(std::abs(readback[i] - local[i]) <= 8e-13 * std::max(1.0, std::abs(local[i])),
                 "Rotated local input readback disagrees");
      spacecraft.input(internal, local);
      require(Rvector6(spacecraft.GetState().GetState()) == local &&
              !spacecraft.GetOriginRelativeState(earth, epoch, result),
              "Same-origin input changed ordinary state or populated cache");
      std::cout << "NASA_CHECK {\"checkpoint\":\"origin-relative-state-contract\","
                << "\"outcome\":\"passed\",\"checks\":" << checks
                << ",\"scope\":\"Local Cartesian input/readback,rotation,copy/assignment/Clone,"
                   "origin identity,precise epoch,all six mutable backing components,"
                   "frame and explicit-state invalidation,nonfinite atomic rejection,canonical report sample priority;"
                   "no whole-domain physics or GUI credit\"}" << std::endl;
      // Local frame lifetimes exceed every spacecraft that refers to them.
      return 0;
   }
   catch (BaseException &error) { std::cerr << error.GetFullMessage() << std::endl; }
   catch (std::exception &error) { std::cerr << error.what() << std::endl; }
   return 1;
}
