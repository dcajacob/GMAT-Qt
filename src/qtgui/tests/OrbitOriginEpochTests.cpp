#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "Parameter.hpp"
#include "Planet.hpp"
#include "SolarSystem.hpp"
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
#include <QSaveFile>
#include <QThread>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
constexpr double pi = 3.1415926535897932384626433832795;
void require(bool value, const char *message)
{ if (!value) throw std::runtime_error(message); }
void write(const QString &path, const QByteArray &bytes)
{
   QSaveFile file(path);
   require(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit(),
           "Could not retain origin epoch evidence");
}
QJsonArray vector(const Rvector6 &state)
{
   QJsonArray result;
   for (int i = 0; i < 6; ++i) result.append(state[i]);
   return result;
}

// A prescribed linear ephemeris isolates timestamp dispatch. This body is
// a software fixture, not an independent model of any physical planet.
class EpochPlanet : public Planet
{
public:
   int a1States = 0, preciseStates = 0, a1Positions = 0, precisePositions = 0;
   GmatTime lastStateEpoch, lastPositionEpoch;
   EpochPlanet() : Planet("ClockBody") { SetRealParameter("Mu", 1000); }
   GmatBase *Clone() const override { return new EpochPlanet(*this); }
   bool Initialize() override { return true; }
   static Rvector6 state(const GmatTime &epoch)
   {
      const double seconds = (epoch-GmatTime(21545.125)).GetTimeInSec();
      return Rvector6(30000+4*seconds,12000-5*seconds,-5000+2*seconds,4,-5,2);
   }
   const Rvector6 GetMJ2000State(const A1Mjd &epoch) override
   { ++a1States; lastStateEpoch = GmatTime(epoch.Get()); return state(lastStateEpoch); }
   const Rvector6 GetMJ2000State(const GmatTime &epoch) override
   { ++preciseStates; lastStateEpoch = epoch; return state(epoch); }
   const Rvector3 GetMJ2000Position(const A1Mjd &epoch) override
   { ++a1Positions; lastPositionEpoch = GmatTime(epoch.Get()); return state(lastPositionEpoch).GetR(); }
   const Rvector3 GetMJ2000Position(const GmatTime &epoch) override
   { ++precisePositions; lastPositionEpoch = epoch; return state(epoch).GetR(); }
};

class EpochAxes : public AxisSystem
{
public:
   int a1Calls = 0, preciseCalls = 0;
   GmatTime lastEpoch;
   EpochAxes() : AxisSystem("PrescribedEpochAxes") { baseSystem = "FK5"; }
   GmatBase *Clone() const override { return new EpochAxes(*this); }
   void rotation(const GmatTime &epoch)
   {
      lastEpoch = epoch;
      const double theta = .25*(epoch-GmatTime(21545.125)).GetTimeInSec();
      const double c = std::cos(theta), s = std::sin(theta);
      rotMatrix.Set(c,-s,0,s,c,0,0,0,1);
      rotDotMatrix.Set(-.25*s,-.25*c,0,.25*c,-.25*s,0,0,0,0);
      rotData = rotMatrix.GetDataVector(); rotDotData = rotDotMatrix.GetDataVector();
   }
   void CalculateRotationMatrix(const A1Mjd &epoch, bool) override
   { ++a1Calls; rotation(GmatTime(epoch.Get())); }
   void CalculateRotationMatrix(const GmatTime &epoch, bool) override
   { ++preciseCalls; rotation(epoch); }
};

double initialMeanAnomaly()
{
   const double e = .2, f = pi/3;
   const double eccentric = 2*std::atan2(std::sqrt(1-e)*std::sin(f/2),
                                       std::sqrt(1+e)*std::cos(f/2));
   return eccentric-e*std::sin(eccentric);
}

// Independent elliptic perifocal equations and an explicitly written basis.
// This oracle does not invoke StateConversionUtil or a spacecraft element API.
Rvector6 orbit(double a, double meanAnomaly)
{
   const double e = .2, mu = 1000, i = pi/6, node = pi/9, peri = pi/12;
   double eccentric = meanAnomaly;
   for (int n = 0; n < 20; ++n)
      eccentric -= (eccentric-e*std::sin(eccentric)-meanAnomaly)/(1-e*std::cos(eccentric));
   const double c = std::cos(eccentric), s = std::sin(eccentric), q = std::sqrt(1-e*e);
   const double radius = a*(1-e*c), speed = std::sqrt(mu*a)/radius;
   const double x = a*(c-e), y = a*q*s, vx = -speed*s, vy = speed*q*c;
   const double cn = std::cos(node), sn = std::sin(node), cp = std::cos(peri), sp = std::sin(peri);
   const double ci = std::cos(i), si = std::sin(i);
   const double p[3] = {cn*cp-sn*sp*ci,sn*cp+cn*sp*ci,sp*si};
   const double t[3] = {-cn*sp-sn*cp*ci,-sn*sp+cn*cp*ci,cp*si};
   Rvector6 result;
   for (int k = 0; k < 3; ++k)
   { result[k] = p[k]*x+t[k]*y; result[k+3] = p[k]*vx+t[k]*vy; }
   return result;
}
}

int main(int argc, char **argv)
{
   QCoreApplication application(argc, argv);
   const QString directory = qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
   QJsonObject proof{{"passed",false}};
   try
   {
      require(argc == 3 && !directory.isEmpty(), "Supply startup, case and evidence directory");
      const QString name = argv[2]; proof["case"] = name;
      const bool precise = name.startsWith("Precise");
      const QString operation = name.mid(precise ? 7 : 6);
      const bool covariance = operation == "Covariance";
      const bool writeMA = operation == "MAWrite", writeSMA = operation == "SMAWrite";
      const bool beta = operation == "Beta", radius = operation == "RMAG";
      require(name.startsWith("Legacy") || precise, "Unknown epoch mode");
      require(covariance || writeMA || writeSMA || beta || radius || operation == "SMA", "Unknown epoch operation");
      const auto startup = QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      auto *moderator = Moderator::Instance();
      require(moderator->Initialize(startup.toStdString(),false), "Native origin runtime failed");
      write(qEnvironmentVariable("GMAT_SOLVER_MAP_READY"),QByteArray::number(QCoreApplication::applicationPid()));
      QElapsedTimer timer; timer.start();
      while (!QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK")) && timer.elapsed() < 10000) QThread::msleep(5);
      require(QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK")), "Native maps not acknowledged");
      const QString script = directory+"/mission.script";
      write(script,QByteArray("Create Spacecraft Sat;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545.125';\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nCreate CoordinateSystem Tracked;\nTracked.Origin = Earth;\nTracked.Axes = MJ2000Eq;\nBeginMissionSequence;\n"));
      require(moderator->InterpretScript(script.toStdString()), "Native origin script did not build");
      auto *spacecraft = dynamic_cast<Spacecraft *>(moderator->GetConfiguredObject("Sat"));
      require(spacecraft, "Native origin spacecraft absent");
      GmatTime epoch(21545.125); if (precise) epoch.AddSeconds(.0000001);
      require(!precise || epoch != GmatTime(epoch.GetMjd()), "Precise fixture rounded to Real");
      spacecraft->SetPrecisionTimeFlag(precise); spacecraft->SetEpochGT(epoch);
      proof["epoch_rounding_seconds"] = (epoch-GmatTime(epoch.GetMjd())).GetTimeInSec();
      if (covariance)
      {
         auto *frame = dynamic_cast<CoordinateSystem *>(moderator->GetConfiguredObject("Tracked"));
         require(frame, "Covariance frame absent"); EpochAxes prescribed;
         require(frame->SetRefObject(&prescribed,Gmat::AXIS_SYSTEM,prescribed.GetName()) && frame->Initialize(), "Covariance frame did not initialize");
         auto *axes = dynamic_cast<EpochAxes *>(frame->GetAxisSystem()); require(axes, "Covariance axes type lost");
         spacecraft->SetState(Rvector6(1000,7000,150,-2,1,.2));
         Rmatrix diagonal(6,6); const double entries[6] = {1,4,9,16,25,36};
         for (int k = 0; k < 6; ++k) diagonal(k,k) = entries[k];
         spacecraft->SetRmatrixParameter("OrbitErrorCovariance",diagonal);
         auto *parameter = moderator->CreateParameter("OrbitErrorCovariance","Sat.Tracked.OrbitErrorCovariance","Sat","Tracked",1);
         require(parameter && parameter->Initialize(), "Native covariance parameter failed");
         axes->a1Calls = axes->preciseCalls = 0;
         const auto actual = parameter->EvaluateRmatrix();
         const double theta = .25*(epoch-GmatTime(21545.125)).GetTimeInSec(), c = std::cos(theta), s = std::sin(theta);
         double maximum = 0; QJsonArray values, expected;
         for (int row = 0; row < 6; ++row) for (int col = 0; col < 6; ++col)
         {
            double want = 0;
            if (row/3 == col/3)
            {
               const int base = row/3*3, i = row%3, j = col%3;
               if (i == 0 && j == 0) want = c*c*entries[base]+s*s*entries[base+1];
               if (i == 1 && j == 1) want = s*s*entries[base]+c*c*entries[base+1];
               if ((i == 0 && j == 1) || (i == 1 && j == 0)) want = s*c*(entries[base+1]-entries[base]);
               if (i == 2 && j == 2) want = entries[base+2];
            }
            require(std::isfinite(actual(row,col)), "Covariance entry nonfinite");
            maximum = std::max(maximum,std::abs(actual(row,col)-want));
            values.append(actual(row,col)); expected.append(want);
         }
         const bool dispatch = precise ? axes->a1Calls == 0 && axes->preciseCalls > 0 && axes->lastEpoch == epoch : axes->a1Calls > 0 && axes->preciseCalls == 0;
         proof["passed"] = maximum < 1e-12 && dispatch; proof["actual"] = values; proof["expected"] = expected;
         proof["max_entry_error"] = maximum; proof["tolerance"] = 1e-12; proof["epoch_dispatch_correct"] = dispatch;
         proof["a1_calls"] = axes->a1Calls; proof["precise_calls"] = axes->preciseCalls;
         proof["oracle"] = "Closed-form Rz rotation of diagonal covariance using unchanged blockdiag(R,R) convention; timestamp qualification only";
      }
      else
      {
         auto *system = moderator->GetSolarSystemInUse(); auto *body = new EpochPlanet;
         require(system->AddBody(body), "Could not attach prescribed origin");
         const auto relative = orbit(9000,initialMeanAnomaly()), origin = EpochPlanet::state(epoch);
         const Rvector6 initial = relative+origin; spacecraft->SetState(initial);
         const std::string type = writeMA ? "MA" : radius ? "RMAG" : beta ? "BetaAngle" : "SMA";
         auto *parameter = moderator->CreateParameter(type,"Sat.ClockBody."+type,"Sat","ClockBody",1);
         require(parameter && parameter->Initialize(), "Native origin parameter failed");
         double expectedRead = writeMA ? initialMeanAnomaly()*180/pi : radius ? relative.GetR().GetMagnitude() : 9000;
         if (beta)
         {
            const auto sun = system->GetBody("Sun")->GetMJ2000Position(epoch);
            const auto r = relative.GetR(), v = relative.GetV(), direction = sun-origin.GetR();
            const Rvector3 h(r[1]*v[2]-r[2]*v[1],r[2]*v[0]-r[0]*v[2],r[0]*v[1]-r[1]*v[0]);
            expectedRead = std::asin((h*direction)/(h.GetMagnitude()*direction.GetMagnitude()))*180/pi;
         }
         body->a1States = body->preciseStates = body->a1Positions = body->precisePositions = 0;
         const double actualRead = parameter->EvaluateReal(), tolerance = beta || writeMA ? 1e-10 : 1e-8;
         Rvector6 expected(initial);
         if (writeMA || writeSMA)
         {
            parameter->SetReal(actualRead+(writeMA ? 5 : 12.5));
            expected = origin+orbit(writeSMA ? 9012.5 : 9000,initialMeanAnomaly()+(writeMA ? 5*pi/180 : 0));
         }
         const auto actual = spacecraft->GetState("Cartesian"); double positionError = 0, velocityError = 0;
         for (int k = 0; k < 6; ++k)
         {
            require(std::isfinite(actual[k]), "Origin operation produced nonfinite state");
            auto &error = k < 3 ? positionError : velocityError;
            error = std::max(error,std::abs(actual[k]-expected[k]));
         }
         const bool states = precise ? body->a1States == 0 && body->preciseStates > 0 && body->lastStateEpoch == epoch : body->a1States > 0 && body->preciseStates == 0;
         const bool positions = !beta || (precise ? body->a1Positions == 0 && body->precisePositions > 0 && body->lastPositionEpoch == epoch : body->a1Positions > 0 && body->precisePositions == 0);
         proof["passed"] = std::abs(actualRead-expectedRead) < tolerance && positionError < 1e-8 && velocityError < 1e-11 && states && positions;
         proof["actual_read"] = actualRead; proof["expected_read"] = expectedRead; proof["read_tolerance"] = tolerance;
         proof["actual_state"] = vector(actual); proof["expected_state"] = vector(expected);
         proof["position_error_km"] = positionError; proof["velocity_error_km_s"] = velocityError;
         proof["state_epoch_correct"] = states; proof["position_epoch_correct"] = positions;
         proof["a1_states"] = body->a1States; proof["precise_states"] = body->preciseStates;
         proof["a1_positions"] = body->a1Positions; proof["precise_positions"] = body->precisePositions;
         proof["oracle"] = beta ? "Independent angular-momentum/Sun-direction geometry, native Sun ephemeris as shared input; prescribed moving origin" : "Independent elliptic perifocal equations, explicit rotation basis and prescribed moving origin";
      }
   }
   catch (BaseException &error) { proof["error"] = QString::fromStdString(error.GetFullMessage()); }
   catch (std::exception &error) { proof["error"] = QString::fromUtf8(error.what()); }
   write(QDir(directory).filePath("proof.json"),QJsonDocument(proof).toJson());
   std::cout << QJsonDocument(proof).toJson(QJsonDocument::Compact).constData() << std::endl;
   return proof["passed"].toBool() ? 0 : 1;
}
