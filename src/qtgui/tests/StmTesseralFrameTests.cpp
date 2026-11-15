#include "NativeMessageCapture.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "OrbitStmParameters.hpp"
#include "CoordinateSystem.hpp"
#include "PropSetup.hpp"
#include "ODEModel.hpp"
#include "GravityField.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "OrbitStmForceGradient.hpp"
#include "OrbitStmFrame.hpp"
#include "CoordinateConverter.hpp"
#include "Propagator.hpp"
#include "GmatCommand.hpp"
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
#include <memory>
#include <stdexcept>

namespace {
void require(bool condition, const char *message)
{
   if (!condition)
      throw std::runtime_error(message);
}

void save(const QString &path, const QByteArray &value)
{
   QFile file(path);
   require(file.open(QIODevice::WriteOnly) && file.write(value) == value.size(),
           "Cannot write tesseral STM evidence");
}

QByteArray read(const QString &path)
{
   QFile file(path);
   require(file.open(QIODevice::ReadOnly), "Cannot read tesseral STM fixture");
   return file.readAll();
}

QJsonArray matrix(const Rmatrix &value)
{
   QJsonArray result;
   for (int i = 0; i < value.GetNumRows(); ++i)
   {
      QJsonArray row;
      for (int j = 0; j < value.GetNumColumns(); ++j)
         row.append(value(i, j));
      result.append(row);
   }
   return result;
}

QJsonArray state(Spacecraft *spacecraft)
{
   QJsonArray result;
   for (int i = 0; i < 6; ++i)
      result.append(spacecraft->GetState()[i]);
   return result;
}

QJsonObject snapshot(Spacecraft *spacecraft)
{
   return {{"state", state(spacecraft)},
           {"stm", matrix(spacecraft->GetRmatrixParameter("OrbitSTM"))},
           {"epoch", spacecraft->GetEpoch()},
           {"precise", spacecraft->HasPrecisionTime()}};
}

std::unique_ptr<Parameter> parameter(int index, Spacecraft *owner,
                                   CoordinateSystem *frame, Moderator *engine)
{
   const auto prefix = owner->GetName() + "." + frame->GetName() + ".";
   std::unique_ptr<Parameter> result;
   switch (index)
   {
      case 0: result.reset(new OrbitStm(prefix + "OrbitSTM", owner)); break;
      case 1: result.reset(new OrbitStmA(prefix + "OrbitSTMA", owner)); break;
      case 2: result.reset(new OrbitStmB(prefix + "OrbitSTMB", owner)); break;
      case 3: result.reset(new OrbitStmC(prefix + "OrbitSTMC", owner)); break;
      case 4: result.reset(new OrbitStmD(prefix + "OrbitSTMD", owner)); break;
   }
   require(result != nullptr, "Unknown tesseral STM parameter");
   result->SetSolarSystem(engine->GetSolarSystemInUse());
   result->SetInternalCoordSystem(owner->GetInternalCoordSystem());
   result->SetRefObjectName(Gmat::COORDINATE_SYSTEM, frame->GetName());
   require(result->SetRefObject(frame, Gmat::COORDINATE_SYSTEM, frame->GetName()) &&
           result->Initialize(), "Tesseral STM parameter did not bind");
   return result;
}
}

int main(int argc, char **argv)
{
   QCoreApplication app(argc, argv);
   try
   {
      require(argc == 3, "Supply startup and frozen tesseral cases");
      const auto output = qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      require(!output.isEmpty(), "Supply tesseral evidence directory");
      const auto plan = QJsonDocument::fromJson(read(argv[2])).object();
      const auto startup = QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      NativeMessageCapture messages;
      auto *engine = Moderator::Instance();
      require(engine->Initialize(startup.toStdString(), false),
              "Tesseral runtime did not initialize");

      const auto ready = qEnvironmentVariable("GMAT_STM_MAP_READY");
      const auto ack = qEnvironmentVariable("GMAT_STM_MAP_ACK");
      if (!ready.isEmpty())
      {
         save(ready, QByteArray::number(QCoreApplication::applicationPid()));
         QElapsedTimer timer;
         timer.start();
         while (!QFile::exists(ack) && timer.elapsed() < 10000)
            QThread::msleep(5);
         require(QFile::exists(ack), "Tesseral mapped images were not acknowledged");
      }

      const auto potential = QDir(output).filePath("STM-tesseral.cof");
      const double mu = plan["mu"].toDouble();
      const double radius = plan["potential_radius_km"].toDouble();
      QString coefficients = "COMMENT Synthetic full degree-two STM fixture\n";
      coefficients += QString("POTFIELD  2  2  1 %1 %2 1.0\n")
         .arg(mu * 1e9, 0, 'E', 14).arg(radius * 1e3, 0, 'E', 14);
      for (int order = 0; order <= 2; ++order)
      {
         const double cosine = order == 0 ? -plan["j2"].toDouble() / std::sqrt(5.0)
            : plan[QString("c2%1").arg(order)].toDouble();
         const double sine = order == 0 ? 0.0
            : plan[QString("s2%1").arg(order)].toDouble();
         coefficients += "RECOEF    2" + QString::number(order).rightJustified(3) + "   " +
            QString::number(cosine, 'E', 14).rightJustified(21) +
            QString::number(sine, 'E', 14).rightJustified(21) + "\n";
      }
      save(potential, (coefficients + "END\n").toUtf8());
      const auto tideFile = QDir(output).filePath("isotropic.tide");
      if (plan.contains("love_k2"))
      {
         const auto love = QString::number(plan["love_k2"].toDouble(),'g',17);
         save(tideFile,("k 2 0 "+love+"\nk 2 1 "+love+"\nk 2 2 "+love+"\n").toUtf8());
      }

      QString script = "Earth.Mu = " + QString::number(mu, 'g', 17) +
         ";\nCreate Spacecraft Sat Ref;\n";
      const QStringList fields{"X", "Y", "Z", "VX", "VY", "VZ"};
      for (const auto &name : {QString("Sat"), QString("Ref")})
      {
         const double orbitRadius = plan[name == "Sat" ? "radius" : "reference_radius"].toDouble();
         const double initial[] = {orbitRadius, 0, 0, 0, std::sqrt(mu / orbitRadius), 0};
         script += name + ".CoordinateSystem = EarthMJ2000Eq;\n" +
            name + ".DisplayStateType = Cartesian;\n" +
            name + ".DateFormat = A1ModJulian;\n" + name + ".Epoch = '21545.125';\n";
         for (int i = 0; i < 6; ++i)
            script += name + "." + fields[i] + " = " + QString::number(initial[i], 'g', 17) + ";\n";
      }
      for (const auto &entry : plan["cases"].toArray())
      {
         const auto spec = entry.toObject();
         const auto name = spec["key"].toString();
         script += "Create CoordinateSystem " + name + ";\n" +
            name + ".Origin = " + spec["origin"].toString() + ";\n" +
            name + ".Axes = ObjectReferenced;\n" +
            name + ".Primary = " + spec["primary"].toString() + ";\n" +
            name + ".Secondary = Sat;\n" + name + ".XAxis = " + spec["x_axis"].toString() + ";\n" +
            name + ".ZAxis = N;\n";
      }
      script += "Create ForceModel Gravity;\nGravity.PrimaryBodies = {Earth};\n"
         "Gravity.PointMasses = {};\nGravity.GravityField.Earth.PotentialFile = '" + potential + "';\n"
         "Gravity.GravityField.Earth.Degree = 2;\nGravity.GravityField.Earth.Order = 2;\n"
         "Gravity.GravityField.Earth.StmLimit = 2;\nGravity.Drag = None;\nGravity.SRP = Off;\n"
         "Gravity.RelativisticCorrection = Off;\nCreate Propagator P;\nP.FM = Gravity;\n"
         "P.Type = RungeKutta89;\nP.Accuracy = 1e-13;\nP.InitialStepSize = 5;\nP.MinStep = 5;\n"
         "P.MaxStep = 5;\nBeginMissionSequence;\n"
         "Propagate P(Sat, Ref, 'STM') {Sat.ElapsedSecs = 60, StopTolerance = 1e-6};\n";
      const auto path = output + "/mission.script";
      if (plan.contains("love_k2"))
         script.replace("BeginMissionSequence;\n", "Gravity.GravityField.Earth.TideFile = '" + tideFile +
            "';\nGravity.GravityField.Earth.TideModel = 'Solid';\nBeginMissionSequence;\n");
      save(path, script.toUtf8());
      const bool built = engine->InterpretScript(path.toStdString());
      save(output + "/build-messages.txt", QByteArray::fromStdString(messages.text));
      require(built, "Tesseral mission did not build");
      messages.text.clear();
      const int status = engine->RunMission();
      save(output + "/run-messages.txt", QByteArray::fromStdString(messages.text));
      save(output + "/native-status.json", QJsonDocument(QJsonObject{
         {"build_status", built}, {"run_status", status}}).toJson());
      require(status == 1, "Tesseral mission did not complete");

      auto *sat = dynamic_cast<Spacecraft*>(engine->GetInternalObject("Sat"));
      auto *ref = dynamic_cast<Spacecraft*>(engine->GetInternalObject("Ref"));
      require(sat && ref, "Tesseral propagated spacecraft missing");
      GravityField *field = nullptr;
      double duration = 0;
      for (auto *command = engine->GetFirstCommand(); command; command = command->GetNext())
      {
         if (command->GetTypeName() != "Propagate")
            continue;
         auto *setup = dynamic_cast<PropSetup*>(command->GetClone(0));
         require(setup && setup->GetPropagator(), "Tesseral propagator missing");
         duration = setup->GetPropagator()->GetTime();
         auto *model = setup->GetODEModel();
         require(model && model->GetNumForces() == 1, "Expected one tesseral gravity provider");
         field = dynamic_cast<GravityField*>(model->GetForce(0));
      }
      require(field != nullptr, "Actual tesseral GravityField missing");
      std::string body;
      Real nativeMu = 0;
      require(field->GetBodyAndMu(body, nativeMu), "Tesseral field constants missing");
      QJsonObject provider{
         {"type", QString::fromStdString(field->GetTypeName())},
         {"body", QString::fromStdString(body)}, {"mu", nativeMu},
         {"degree", field->GetIntegerParameter("Degree")},
         {"order", field->GetIntegerParameter("Order")},
         {"stm_limit", field->GetIntegerParameter("StmLimit")},
         {"potential", QString::fromStdString(field->GetStringParameter("PotentialFileFullPath"))}
      };
      if (plan.contains("love_k2"))
      {
         provider["tide_model"] = QString::fromStdString(field->GetStringParameter("TideModel"));
         provider["tide_file"] = QString::fromStdString(field->GetStringParameter("TideFile"));
      }
      save(output + "/force-provider.json", QJsonDocument(provider).toJson());

      auto *input = dynamic_cast<CoordinateSystem*>(field->GetRefObject(
         Gmat::COORDINATE_SYSTEM, field->GetStringParameter("InputCoordinateSystem")));
      auto *fixed = dynamic_cast<CoordinateSystem*>(field->GetRefObject(
         Gmat::COORDINATE_SYSTEM, field->GetStringParameter("FixedCoordinateSystem")));
      require(input && fixed && input->GetBaseSystem() == "FK5" &&
              input->AreAxesOfType("MJ2000EqAxes"), "Unexpected tesseral input frame");
      OrbitStmFrame::DetachedFrames detached;
      input = detached.Copy(input);
      fixed = detached.Copy(fixed);
      fixed->SetForGravityUpdate(true);
      CoordinateConverter converter;
      QJsonArray poses;
      const int intervals = plan["pose_intervals"].toInt();
      require(intervals > 0, "Invalid tesseral pose sampling interval");
      for (int i = 0; i <= intervals; ++i)
      {
         const double seconds = duration * i / intervals;
         Rvector6 zero(0, 0, 0, 0, 0, 0), unused;
         require(converter.Convert(A1Mjd(21545.125 + seconds / 86400), zero,
                 input, unused, fixed, true, true), "Tesseral pose sample failed");
         QJsonObject pose{{"seconds", seconds},
            {"input_to_fixed", matrix(converter.GetLastRotationMatrix())}};
         if (plan.contains("love_k2"))
         {
            const GmatTime atEpoch(21545.125+seconds/86400);
            auto *earth = engine->GetSolarSystemInUse()->GetBody("Earth");
            QJsonArray bodies;
            for (const auto &name : {"Sun","Luna"})
            {
               auto *body = engine->GetSolarSystemInUse()->GetBody(name);
               require(body && earth,"Tide-generating body missing");
               const auto position = body->GetMJ2000State(atEpoch)-earth->GetMJ2000State(atEpoch);
               bodies.append(QJsonObject{{"name",name},{"mu",body->GetGravitationalConstant()},
                  {"position",QJsonArray{position[0],position[1],position[2]}}});
            }
            pose["tide_bodies"] = bodies;
         }
         poses.append(pose);
      }
      save(output + "/force-poses.json", QJsonDocument(QJsonObject{
         {"samples", poses}, {"input_base", "FK5"}, {"input_axes", "MJ2000Eq"},
         {"sampled_detached_frames", true}}).toJson());

      if (plan.contains("love_k2"))
      {
         const auto originalEpoch = field->GetRealParameter(field->GetParameterID("Epoch"));
         const auto originalTime = field->GetTime();
         struct RestoreClock
         {
            GravityField *field;
            Real time;
            ~RestoreClock()
            {
               field->SetTime(time);
            }
         } restore{field,originalTime};
         QJsonArray samples;
         for (const auto &offset : plan["clock_offsets_seconds"].toArray())
         {
            field->SetTime(originalTime+offset.toDouble());
            const auto beforeEpoch = field->GetRealParameter(field->GetParameterID("Epoch"));
            const auto beforeTime = field->GetTime();
            OrbitStmForceGradient::Request request(field);
            const auto acceleration = field->GetDerivativesForSpacecraft(sat);
            samples.append(QJsonObject{{"offset",offset},{"available",request.available},
               {"acceleration",QJsonArray{acceleration[3],acceleration[4],acceleration[5]}},
               {"gradient",matrix(request.value)},
               {"epoch_before",beforeEpoch},{"epoch_after",field->GetRealParameter(field->GetParameterID("Epoch"))},
               {"time_before",beforeTime},{"time_after",field->GetTime()},
               {"clock_unchanged",field->GetTime()==beforeTime &&
                  field->GetRealParameter(field->GetParameterID("Epoch"))==beforeEpoch}});
         }
         field->SetTime(originalTime);
         require(field->GetRealParameter(field->GetParameterID("Epoch"))==originalEpoch,
                 "Tide clock probe did not restore the effective epoch");
         save(output + "/sampled-force-clocks.json",QJsonDocument(samples).toJson());
      }

      const QStringList types{"OrbitSTM", "OrbitSTMA", "OrbitSTMB", "OrbitSTMC", "OrbitSTMD"};
      QJsonArray observations;
      for (const auto &entry : plan["cases"].toArray())
      {
         const auto key = entry.toObject()["key"].toString();
         auto *frame = dynamic_cast<CoordinateSystem*>(engine->GetInternalObject(key.toStdString()));
         require(frame && frame->GetSecondaryObject() == sat, "Tesseral frame owner differs");
         for (int index = 0; index < types.size(); ++index)
         {
            auto original = parameter(index, sat, frame, engine);
            std::unique_ptr<Parameter> clone(dynamic_cast<Parameter*>(original->Clone()));
            std::unique_ptr<Parameter> copy(dynamic_cast<Parameter*>(original->Clone()));
            require(clone && copy, "Tesseral parameter clone missing");
            copy->Copy(original.get());
            for (const auto &provider : {QString("original"), QString("clone"), QString("copy")})
            {
               auto *value = provider == "original" ? original.get()
                  : provider == "clone" ? clone.get() : copy.get();
               const auto before = snapshot(sat);
               const auto guideBefore = snapshot(ref);
               const auto cache = matrix(value->GetRmatrix());
               bool rejected = false;
               QString diagnostic;
               QJsonArray actual;
               try { actual = matrix(value->EvaluateRmatrix()); }
               catch (BaseException &error)
               {
                  rejected = true;
                  diagnostic = QString::fromStdString(error.GetFullMessage());
               }
               observations.append(QJsonObject{
                  {"case", key}, {"provider", provider}, {"type", types[index]},
                  {"actual", actual}, {"rejected", rejected}, {"diagnostic", diagnostic},
                  {"owner_unchanged", before == snapshot(sat)},
                  {"guide_unchanged", guideBefore == snapshot(ref)},
                  {"cache_atomic", !rejected || cache == matrix(value->GetRmatrix())},
                  {"readonly", !value->IsSettable()},
                  {"owner_bound", value->GetRefObject(Gmat::SPACECRAFT, "Sat") == sat},
                  {"frame_bound", value->GetRefObject(Gmat::COORDINATE_SYSTEM, key.toStdString()) == frame}
               });
            }
         }
      }
      save(output + "/proof.json", QJsonDocument(QJsonObject{
         {"status", "procedure-completed"}, {"integration_seconds", duration},
         {"owner_state", state(sat)}, {"guide_state", state(ref)},
         {"native_stm", matrix(sat->GetRmatrixParameter("OrbitSTM"))},
         {"guide_stm", matrix(ref->GetRmatrixParameter("OrbitSTM"))},
         {"observations", observations}, {"native_missions", 1}
      }).toJson());
      std::cout << "Tesseral STM mission and 45 parameter reads completed\n";
      return 0;
   }
   catch (BaseException &error) { std::cerr << error.GetFullMessage(); }
   catch (std::exception &error) { std::cerr << error.what(); }
   return 1;
}
