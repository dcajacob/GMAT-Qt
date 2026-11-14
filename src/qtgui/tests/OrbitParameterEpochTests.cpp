#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "Parameter.hpp"
#include "CoordinateSystem.hpp"
#include "AxisSystem.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char *message)
{
   if (!condition) throw std::runtime_error(message);
}

void write(const QString &path, const QByteArray &bytes)
{
   QFile file(path);
   require(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size(),
           "Could not retain orbit epoch evidence");
}

// Prescribed rotating frame, not a model of a physical body's rotation.
// Its closed-form R and Rdot make epoch loss observable without using the
// coordinate converter itself to calculate the expected spacecraft state.
class EpochAxes : public AxisSystem
{
public:
   int a1Calls = 0, preciseCalls = 0;
   GmatTime lastEpoch;
   EpochAxes() : AxisSystem("PrescribedEpochAxes") { baseSystem = "FK5"; }
   GmatBase *Clone() const override { return new EpochAxes(*this); }
   void setRotation(const GmatTime &epoch)
   {
      lastEpoch = epoch;
      const double theta = .25 * (epoch - GmatTime(21545.125)).GetTimeInSec();
      const double c = std::cos(theta), s = std::sin(theta);
      rotMatrix.Set(c,-s,0, s,c,0, 0,0,1);
      rotDotMatrix.Set(-.25*s,-.25*c,0, .25*c,-.25*s,0, 0,0,0);
      rotData = rotMatrix.GetDataVector();
      rotDotData = rotDotMatrix.GetDataVector();
   }
   void CalculateRotationMatrix(const A1Mjd &epoch, bool) override
   { ++a1Calls; setRotation(GmatTime(epoch.Get())); }
   void CalculateRotationMatrix(const GmatTime &epoch, bool) override
   { ++preciseCalls; setRotation(epoch); }
};

QJsonArray vector(const Rvector6 &state)
{
   QJsonArray result;
   for (int i = 0; i < 6; ++i) result.append(state[i]);
   return result;
}
}

int main(int argc, char **argv)
{
   QCoreApplication application(argc, argv);
   const QString directory = qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
   QJsonObject proof{{"passed", false}};
   try
   {
      require(argc == 3 && !directory.isEmpty(), "Supply startup, case and evidence directory");
      const QString name = argv[2];
      proof["case"] = name;
      const bool precise = name.startsWith("Precise");
      const bool velocity = name.endsWith("VXIncrement");
      const bool idempotent = name.endsWith("Idempotent");
      require(name == "LegacyXIncrement" || name == "PreciseXIncrement" ||
              name == "PreciseXIdempotent" || name == "LegacyVXIncrement" ||
              name == "PreciseVXIncrement", "Unknown orbit epoch case");
      const auto startup = QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      auto *moderator = Moderator::Instance();
      require(moderator->Initialize(startup.toStdString(), false), "Native runtime failed");
      write(qEnvironmentVariable("GMAT_SOLVER_MAP_READY"), QByteArray::number(QCoreApplication::applicationPid()));
      QElapsedTimer timer; timer.start();
      while (!QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK")) && timer.elapsed() < 10000)
         QThread::msleep(5);
      require(QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK")), "Native maps not acknowledged");
      const QString script = directory + "/mission.script";
      write(script, QByteArray(
         "Create Spacecraft Sat;\n"
         "Sat.DateFormat = A1ModJulian;\nSat.Epoch = '21545.125';\n"
         "Sat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\n"
         "Create CoordinateSystem Tracked;\nTracked.Origin = Earth;\n"
         "Tracked.Axes = MJ2000Eq;\nBeginMissionSequence;\n"));
      require(moderator->InterpretScript(script.toStdString()), "Probe script did not build");
      auto *spacecraft = dynamic_cast<Spacecraft *>(moderator->GetConfiguredObject("Sat"));
      auto *frame = dynamic_cast<CoordinateSystem *>(moderator->GetConfiguredObject("Tracked"));
      require(spacecraft && frame, "Probe spacecraft or frame absent");
      EpochAxes prescribed;
      require(frame->SetRefObject(&prescribed, Gmat::AXIS_SYSTEM, prescribed.GetName()),
              "Could not attach prescribed frame axes");
      require(frame->Initialize(), "Prescribed frame did not initialize");
      auto *axes = dynamic_cast<EpochAxes *>(frame->GetAxisSystem());
      require(axes, "Frame lost prescribed axes type");
      GmatTime epoch(21545.125);
      if (precise) epoch.AddSeconds(.0000001);
      require(!precise || epoch != GmatTime(epoch.GetMjd()), "Precise probe epoch is representable as Real");
      spacecraft->SetPrecisionTimeFlag(precise);
      spacecraft->SetEpochGT(epoch);
      const Rvector6 initial(1000,7000,150, -2,1,.2);
      spacecraft->SetState(initial);
      const std::string field = velocity ? "VX" : "X";
      auto *parameter = moderator->CreateParameter(field, "Sat.Tracked." + field, "Sat", "Tracked", 1);
      require(parameter && parameter->Initialize(), "Native frame parameter did not initialize");
      axes->a1Calls = axes->preciseCalls = 0;
      const double before = parameter->EvaluateReal();
      const double theta = .25 * (epoch - GmatTime(21545.125)).GetTimeInSec();
      const double c = std::cos(theta), s = std::sin(theta);
      // r' = R^T r; v' = R^T (v - Rdot r').
      const double localX = c*initial[0] + s*initial[1];
      const double localY = -s*initial[0] + c*initial[1];
      const double localVX = c*initial[3] + s*initial[4] + .25*localY;
      const double expectedRead = velocity ? localVX : localX;
      const double delta = idempotent ? 0 : 1.5;
      parameter->SetReal(before + delta);
      const Rvector6 actual = spacecraft->GetState("Cartesian");
      Rvector6 expected(initial);
      if (velocity) { expected[3] += c*delta; expected[4] += s*delta; }
      else
      {
         expected[0] += c*delta; expected[1] += s*delta;
         expected[3] -= .25*s*delta; expected[4] += .25*c*delta;
      }
      double maximumError = 0;
      for (int i = 0; i < 6; ++i)
      {
         require(std::isfinite(actual[i]), "Parameter write produced nonfinite state");
         maximumError = std::max(maximumError, std::abs(actual[i] - expected[i]));
      }
      const bool dispatch = precise ? axes->a1Calls == 0 && axes->preciseCalls > 0 && axes->lastEpoch == epoch :
                                     axes->a1Calls > 0 && axes->preciseCalls == 0;
      proof["passed"] = std::abs(before-expectedRead) < 1e-9 && maximumError < 1e-9 && dispatch;
      proof["read"] = before; proof["expected_read"] = expectedRead;
      proof["expected_state"] = vector(expected); proof["actual_state"] = vector(actual);
      proof["maximum_state_error"] = maximumError; proof["tolerance"] = 1e-9;
      proof["a1_calls"] = axes->a1Calls; proof["precise_calls"] = axes->preciseCalls;
      proof["epoch_dispatch_correct"] = dispatch;
      proof["epoch_rounding_seconds"] = (epoch-GmatTime(epoch.GetMjd())).GetTimeInSec();
      proof["oracle"] = "Prescribed Rz(0.25 rad/s) and its derivative; independent closed-form six-state expectation";
   }
   catch (BaseException &error) { proof["error"] = QString::fromStdString(error.GetFullMessage()); }
   catch (std::exception &error) { proof["error"] = QString::fromUtf8(error.what()); }
   write(QDir(directory).filePath("proof.json"), QJsonDocument(proof).toJson());
   std::cout << QJsonDocument(proof).toJson(QJsonDocument::Compact).constData() << std::endl;
   return proof["passed"].toBool() ? 0 : 1;
}
