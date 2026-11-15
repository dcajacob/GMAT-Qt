#include "NativeMessageCapture.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "OrbitStmParameters.hpp"
#include "CoordinateSystem.hpp"
#include "PropSetup.hpp"
#include "ODEModel.hpp"
#include "RelativisticCorrection.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "OrbitStmForceGradient.hpp"
#include "PropagationStateManager.hpp"
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
           "Cannot write relativistic STM evidence");
}

QByteArray read(const QString &path)
{
   QFile file(path);
   require(file.open(QIODevice::ReadOnly), "Cannot read relativistic STM fixture");
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

QJsonArray centeredState(Spacecraft *spacecraft, CelestialBody *center)
{
   const GmatTime epoch = spacecraft->GetEpochGT();
   const Rvector6 offset = center->GetMJ2000State(epoch) -
      spacecraft->GetJ2000Body()->GetMJ2000State(epoch);
   QJsonArray result;
   for (int i = 0; i < 6; ++i)
      result.append(spacecraft->GetState()[i] - offset[i]);
   return result;
}

QJsonArray bodyFromSun(CelestialBody *body, CelestialBody *sun, const GmatTime &epoch)
{
   const Rvector6 relative = body->GetMJ2000State(epoch) - sun->GetMJ2000State(epoch);
   QJsonArray result;
   for (int i = 0; i < 6; ++i)
      result.append(relative[i]);
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
   require(result != nullptr, "Unknown relativistic STM parameter");
   result->SetSolarSystem(engine->GetSolarSystemInUse());
   result->SetInternalCoordSystem(owner->GetInternalCoordSystem());
   result->SetRefObjectName(Gmat::COORDINATE_SYSTEM, frame->GetName());
   require(result->SetRefObject(frame, Gmat::COORDINATE_SYSTEM, frame->GetName()) &&
           result->Initialize(), "Relativistic STM parameter did not bind");
   return result;
}
}

int main(int argc, char **argv)
{
   QCoreApplication app(argc, argv);
   try
   {
      require(argc == 3, "Supply startup and frozen relativistic cases");
      const auto output = qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      require(!output.isEmpty(), "Supply relativistic evidence directory");
      const auto plan = QJsonDocument::fromJson(read(argv[2])).object();
      const auto startup = QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      NativeMessageCapture messages;
      auto *engine = Moderator::Instance();
      require(engine->Initialize(startup.toStdString(), false),
              "Relativistic runtime did not initialize");

      const auto ready = qEnvironmentVariable("GMAT_STM_MAP_READY");
      const auto ack = qEnvironmentVariable("GMAT_STM_MAP_ACK");
      if (!ready.isEmpty())
      {
         save(ready, QByteArray::number(QCoreApplication::applicationPid()));
         QElapsedTimer timer;
         timer.start();
         while (!QFile::exists(ack) && timer.elapsed() < 10000)
            QThread::msleep(5);
         require(QFile::exists(ack), "Relativistic mapped images were not acknowledged");
      }

      const double mu = plan["mu"].toDouble();
      const QString bodyName = plan["body"].toString("Sun");
      const QString frameName = bodyName + "MJ2000Eq";
      QString script = bodyName + ".Mu = " + QString::number(mu, 'g', 17) + ";\n";
      if (bodyName != "Earth")
         script += "Create CoordinateSystem " + frameName + ";\n" + frameName +
            ".Origin = " + bodyName + ";\n" + frameName + ".Axes = MJ2000Eq;\n";
      script += "Create Spacecraft Sat Ref;\n";
      const QStringList fields{"X", "Y", "Z", "VX", "VY", "VZ"};
      for (const auto &name : {QString("Sat"), QString("Ref")})
      {
         const double orbitRadius = plan[name == "Sat" ? "radius" : "reference_radius"].toDouble();
         const double initial[] = {orbitRadius, 0, 0, 0, std::sqrt(mu / orbitRadius), 0};
         script += name + ".CoordinateSystem = " + frameName + ";\n" +
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
      script += "Create ForceModel Gravity;\nGravity.CentralBody = " + bodyName + ";\n"
         "Gravity.PrimaryBodies = {};\nGravity.PointMasses = {" + bodyName + "};\n"
         "Gravity.Drag = None;\nGravity.SRP = Off;\nGravity.RelativisticCorrection = On;\n"
         "Create Propagator P;\nP.FM = Gravity;\nP.Type = RungeKutta89;\nP.Accuracy = 1e-13;\n"
         "P.InitialStepSize = 5;\nP.MinStep = 5;\nP.MaxStep = 5;\nBeginMissionSequence;\n"
         "Propagate P(Sat, Ref, 'STM') {Sat.ElapsedSecs = 600, StopTolerance = 1e-6};\n";
      const bool matrixOnly = plan["matrix_mode"].toString() == "AMatrix";
      if (matrixOnly)
         script.replace("P(Sat, Ref, 'STM')", "P(Sat, Ref, 'AMatrix')");
      const auto path = output + "/mission.script";
      save(path, script.toUtf8());
      const bool built = engine->InterpretScript(path.toStdString());
      save(output + "/build-messages.txt", QByteArray::fromStdString(messages.text));
      require(built, "Relativistic mission did not build");
      messages.text.clear();
      const int status = engine->RunMission();
      save(output + "/run-messages.txt", QByteArray::fromStdString(messages.text));
      save(output + "/native-status.json", QJsonDocument(QJsonObject{
         {"build_status", built}, {"run_status", status}}).toJson());
      require(status == 1, "Relativistic mission did not complete");

      auto *sat = dynamic_cast<Spacecraft*>(engine->GetInternalObject("Sat"));
      auto *ref = dynamic_cast<Spacecraft*>(engine->GetInternalObject("Ref"));
      require(sat && ref, "Relativistic propagated spacecraft missing");
      RelativisticCorrection *field = nullptr;
      ODEModel *executedModel = nullptr;
      PropagationStateManager *stateManager = nullptr;
      double duration = 0;
      for (auto *command = engine->GetFirstCommand(); command; command = command->GetNext())
      {
         if (command->GetTypeName() != "Propagate")
            continue;
         auto *setup = dynamic_cast<PropSetup*>(command->GetClone(0));
         require(setup && setup->GetPropagator(), "Relativistic propagator missing");
         duration = setup->GetPropagator()->GetTime();
         auto *model = setup->GetODEModel();
         executedModel = model;
         stateManager = setup->GetPropStateManager();
         require(model && model->GetNumForces() == 2, "Expected gravity and relativistic providers");
         for (int index = 0; index < model->GetNumForces(); ++index)
            if (auto *candidate = dynamic_cast<RelativisticCorrection*>(model->GetForce(index)))
               field = candidate;
      }
      require(field != nullptr, "Actual relativistic force missing");
      auto *sun = engine->GetSolarSystemInUse()->GetBody("Sun");
      auto *center = engine->GetSolarSystemInUse()->GetBody(bodyName.toStdString());
      require(sun && center, "Celestial body missing from relativistic mission");
      save(output + "/force-provider.json", QJsonDocument(QJsonObject{
         {"type", QString::fromStdString(field->GetTypeName())},
         {"body", bodyName}, {"mu", center->GetGravitationalConstant()},
         {"radius", center->GetEquatorialRadius()}, {"sun_mu", sun->GetGravitationalConstant()}
      }).toJson());

      std::unique_ptr<CoordinateSystem> input(CoordinateSystem::CreateLocalCoordinateSystem(
         "RelativityInertial", "MJ2000Eq", center, nullptr, nullptr, center->GetJ2000Body(),
         engine->GetSolarSystemInUse()));
      std::unique_ptr<CoordinateSystem> fixed(CoordinateSystem::CreateLocalCoordinateSystem(
         "RelativityFixed", "BodyFixed", center, nullptr, nullptr, center->GetJ2000Body(),
         engine->GetSolarSystemInUse()));
      require(input && fixed, "Relativistic body frames missing");
      CoordinateConverter converter;
      QJsonArray poses;
      const int intervals = plan["pose_intervals"].toInt();
      require(intervals > 0, "Invalid relativistic pose sampling interval");
      for (int i = 0; i <= intervals; ++i)
      {
         const double seconds = duration * i / intervals;
         Rvector6 zero(0, 0, 0, 0, 0, 0), unused;
         require(converter.Convert(A1Mjd(21545.125 + seconds / 86400), zero,
                 fixed.get(), unused, input.get(), true, true), "Relativistic pose sample failed");
         poses.append(QJsonObject{{"seconds", seconds},
            {"body_wrt_sun", bodyFromSun(center, sun, GmatTime(21545.125 + seconds / 86400))},
            {"fixed_to_input", matrix(converter.GetLastRotationMatrix())},
            {"fixed_to_input_dot", matrix(converter.GetLastRotationDotMatrix())}});
      }
      save(output + "/force-poses.json", QJsonDocument(QJsonObject{
         {"samples", poses}, {"input_base", "FK5"}, {"input_axes", "MJ2000Eq"},
         {"sampled_detached_frames", true}}).toJson());

      OrbitStmForceGradient::Request gradientRequest(field);
      const auto queriedAcceleration = field->GetDerivativesForSpacecraft(sat);
      Real modelState[6];
      Rvector6 zero(0, 0, 0, 0, 0, 0), unused;
      if (field->HasPrecisionTime())
      {
         require(field->BuildModelStateGT(sat->GetEpochGT(), modelState,
                 sat->GetState().GetState()), "Relativistic query state conversion failed");
         require(converter.Convert(sat->GetEpochGT(), zero, fixed.get(), unused,
                 input.get(), true, true), "Relativistic query pose conversion failed");
      }
      else
      {
         require(field->BuildModelState(sat->GetEpoch(), modelState,
                 sat->GetState().GetState()), "Relativistic query state conversion failed");
         require(converter.Convert(A1Mjd(sat->GetEpoch()), zero, fixed.get(), unused,
                 input.get(), true, true), "Relativistic query pose conversion failed");
      }
      QJsonArray queryState;
      for (int component = 0; component < 6; ++component)
         queryState.append(modelState[component]);
      QJsonArray acceleration;
      for (int component = 3; component < 6; ++component)
         acceleration.append(queriedAcceleration[component]);
      save(output + "/queried-gradient.json", QJsonDocument(QJsonObject{
         {"available", gradientRequest.available}, {"gradient", matrix(gradientRequest.value)},
         {"acceleration", acceleration}, {"state", queryState},
         {"precise_epoch", field->HasPrecisionTime()},
         {"pose", QJsonObject{{"fixed_to_input", matrix(converter.GetLastRotationMatrix())},
                              {"fixed_to_input_dot", matrix(converter.GetLastRotationDotMatrix())},
                              {"body_wrt_sun", bodyFromSun(center, sun, field->HasPrecisionTime()
                                  ? sat->GetEpochGT() : GmatTime(sat->GetEpoch()))}}}
      }).toJson());

      QJsonArray forceAMatrix;
      if (matrixOnly)
      {
         require(executedModel && stateManager, "Relativistic propagation state missing");
         const int dimension = executedModel->GetDimension();
         std::vector<Real> inputState(executedModel->GetState(), executedModel->GetState() + dimension);
         const auto &mapping = *stateManager->GetStateMap();
         int matrixStart = -1;
         for (int index = 0; index < static_cast<int>(mapping.size()); ++index)
         {
            const auto *item = mapping[index];
            if (item->elementID == Gmat::CARTESIAN_STATE && item->subelement == 1)
            {
               auto *spacecraft = dynamic_cast<Spacecraft*>(item->object);
               require(spacecraft && index + 6 <= dimension, "Relativistic Cartesian association missing");
               require(field->BuildModelState(spacecraft->GetEpoch(), inputState.data() + index,
                       spacecraft->GetState().GetState()), "Relativistic A-matrix state conversion failed");
            }
            if (item->elementID == Gmat::ORBIT_A_MATRIX && item->object == sat && item->subelement == 1)
               matrixStart = index;
         }
         require(matrixStart >= 0 && matrixStart + 36 <= dimension,
                 "Relativistic A-matrix association missing");
         const Real oldTime = field->GetTime();
         field->SetTime(duration);
         try
         {
            require(field->GetDerivatives(inputState.data(), 0.0), "Relativistic A-matrix evaluation failed");
         }
         catch (...)
         {
            field->SetTime(oldTime);
            throw;
         }
         field->SetTime(oldTime);
         const Real *derivatives = field->GetDerivativeArray();
         Rmatrix value(6,6);
         for (int row = 0; row < 6; ++row)
            for (int column = 0; column < 6; ++column)
               value(row,column) = derivatives[matrixStart + row * 6 + column];
         forceAMatrix = matrix(value);
      }

      const QStringList types{"OrbitSTM", "OrbitSTMA", "OrbitSTMB", "OrbitSTMC", "OrbitSTMD"};
      QJsonArray observations;
      for (const auto &entry : matrixOnly ? QJsonArray() : plan["cases"].toArray())
      {
         const auto key = entry.toObject()["key"].toString();
         auto *frame = dynamic_cast<CoordinateSystem*>(engine->GetInternalObject(key.toStdString()));
         require(frame && frame->GetSecondaryObject() == sat, "Relativistic frame owner differs");
         for (int index = 0; index < types.size(); ++index)
         {
            auto original = parameter(index, sat, frame, engine);
            std::unique_ptr<Parameter> clone(dynamic_cast<Parameter*>(original->Clone()));
            std::unique_ptr<Parameter> copy(dynamic_cast<Parameter*>(original->Clone()));
            require(clone && copy, "Relativistic parameter clone missing");
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
         {"owner_state", centeredState(sat, center)}, {"guide_state", centeredState(ref, center)},
         {"raw_owner_state", state(sat)}, {"raw_guide_state", state(ref)},
         {"reported_state_origin", bodyName},
         {"matrix_mode", matrixOnly ? "AMatrix" : "STM"},
         {"native_amatrix", matrix(sat->GetRmatrixParameter("OrbitAMatrix"))},
         {"force_amatrix", forceAMatrix},
         {"native_stm", matrix(sat->GetRmatrixParameter("OrbitSTM"))},
         {"guide_stm", matrix(ref->GetRmatrixParameter("OrbitSTM"))},
         {"observations", observations}, {"native_missions", 1}
      }).toJson());
      std::cout << "Relativistic mission and " << observations.size() << " parameter reads completed\n";
      return 0;
   }
   catch (BaseException &error) { std::cerr << error.GetFullMessage(); }
   catch (std::exception &error) { std::cerr << error.what(); }
   return 1;
}
