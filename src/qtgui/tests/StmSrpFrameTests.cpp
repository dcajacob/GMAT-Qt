#include "NativeMessageCapture.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "OrbitStmParameters.hpp"
#include "CoordinateSystem.hpp"
#include "PropSetup.hpp"
#include "ODEModel.hpp"
#include "SolarRadiationPressure.hpp"
#include "Plate.hpp"
#include "DragForce.hpp"
#include "AtmosphereModel.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "OrbitStmForceGradient.hpp"
#include "PropagationStateManager.hpp"
#include "FileManager.hpp"
#include "OrbitStmFrame.hpp"
#include "CoordinateConverter.hpp"
#include "Propagator.hpp"
#include "GmatCommand.hpp"
#include "CommandExecutionObserver.hpp"
#include "Publisher.hpp"
#include "Subscriber.hpp"
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
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
class FiniteThrustObserver final : public Subscriber, public CommandExecutionObserver
{
public:
   FiniteThrustObserver() : Subscriber("FiniteThrustObserver", "FiniteThrustObserver") {}
   ~FiniteThrustObserver() override { Publisher::Instance()->Unsubscribe(this); }
   GmatBase *Clone() const override { return new FiniteThrustObserver(*this); }
   void BeforeExecution(GmatCommand *command) override
   {
      if (beforeCommand)
         beforeCommand(command);
      if (armed || command->GetTypeName() != "Propagate")
         return;
      auto *publisher = Publisher::Instance();
      SetInternalCoordSystem(publisher->GetInternalCoordSystem());
      if (auto *frame = publisher->GetDataCoordSystem())
         SetDataCoordSystem(frame);
      armed = publisher->Subscribe(this);
   }
   bool ReceiveData(const Real *data, Integer count = 0) override
   {
      bool initial = false;
      if (observed || !error.isEmpty() || !sample ||
          !Publisher::GetPublishedPropagationObjects(data, count, initial) ||
          !data || count < 1 || (data[0]-21545.125)*86400 < 59.99999)
         return true;
      try
      {
         sample();
         observed = true;
      }
      catch (BaseException &failure) { error = QString::fromStdString(failure.GetFullMessage()); }
      catch (std::exception &failure) { error = QString::fromUtf8(failure.what()); }
      return true;
   }
   std::function<void()> sample;
   std::function<void(GmatCommand*)> beforeCommand;
   bool armed = false;
   bool observed = false;
   QString error;
protected:
   bool Distribute(Integer) override { return true; }
   bool Distribute(const Real *, Integer) override { return true; }
};

void require(bool condition, const char *message)
{
   if (!condition)
      throw std::runtime_error(message);
}

void save(const QString &path, const QByteArray &value)
{
   QFile file(path);
   require(file.open(QIODevice::WriteOnly) && file.write(value) == value.size(),
           "Cannot write spherical SRP STM evidence");
}

QByteArray read(const QString &path)
{
   QFile file(path);
   require(file.open(QIODevice::ReadOnly), "Cannot read spherical SRP STM fixture");
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

QJsonArray forceSunPosition(CelestialBody *sun, CelestialBody *earth, const GmatTime &epoch,
                           bool precise)
{
   const Rvector6 sunState = precise ? sun->GetState(epoch) : sun->GetState(epoch.GetMjd());
   const Rvector6 earthState = precise ? earth->GetState(epoch) : earth->GetState(epoch.GetMjd());
   QJsonArray result;
   for (int i=0;i<3;++i) result.append(sunState[i] - earthState[i]);
   return result;
}

QJsonArray bodyRotation(CelestialBody *body, SolarSystem *solarSystem, const GmatTime &epoch)
{
   std::unique_ptr<CoordinateSystem> inertial(CoordinateSystem::CreateLocalCoordinateSystem(
      "CubeInertial", "MJ2000Eq", body, nullptr, nullptr, body, solarSystem));
   std::unique_ptr<CoordinateSystem> fixed(CoordinateSystem::CreateLocalCoordinateSystem(
      "CubeFixed", "BodyFixed", body, nullptr, nullptr, body, solarSystem));
   CoordinateConverter converter;
   const auto rotation = converter.GetRotationMatrixOnly(A1Mjd(epoch.GetMjd()), inertial.get(), fixed.get());
   QJsonArray values;
   for (int row=0;row<3;++row)
      for (int column=0;column<3;++column)
         values.append(rotation(row,column));
   return values;
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
   require(result != nullptr, "Unknown spherical SRP STM parameter");
   result->SetSolarSystem(engine->GetSolarSystemInUse());
   result->SetInternalCoordSystem(owner->GetInternalCoordSystem());
   result->SetRefObjectName(Gmat::COORDINATE_SYSTEM, frame->GetName());
   require(result->SetRefObject(frame, Gmat::COORDINATE_SYSTEM, frame->GetName()) &&
           result->Initialize(), "Spherical SRP STM parameter did not bind");
   return result;
}
}

int main(int argc, char **argv)
{
   QCoreApplication app(argc, argv);
   try
   {
      require(argc == 3, "Supply startup and frozen spherical SRP cases");
      const auto output = qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      require(!output.isEmpty(), "Supply spherical SRP evidence directory");
      const auto plan = QJsonDocument::fromJson(read(argv[2])).object();
      const auto startup = QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      NativeMessageCapture messages;
      auto *engine = Moderator::Instance();
      require(engine->Initialize(startup.toStdString(), false),
              "Spherical SRP runtime did not initialize");

      const auto ready = qEnvironmentVariable("GMAT_STM_MAP_READY");
      const auto ack = qEnvironmentVariable("GMAT_STM_MAP_ACK");
      if (!ready.isEmpty())
      {
         save(ready, QByteArray::number(QCoreApplication::applicationPid()));
         QElapsedTimer timer;
         timer.start();
         while (!QFile::exists(ack) && timer.elapsed() < 10000)
            QThread::msleep(5);
         require(QFile::exists(ack), "Spherical SRP mapped images were not acknowledged");
      }

      auto *sun = engine->GetSolarSystemInUse()->GetBody("Sun");
      auto *earth = engine->GetSolarSystemInUse()->GetBody("Earth");
      require(sun && earth, "Spherical SRP bodies missing");
      const Rvector3 sunlight = sun->GetMJ2000Position(21545.125) - earth->GetMJ2000Position(21545.125);
      const Rvector3 along = sunlight / sunlight.GetMagnitude();
      Rvector3 across = Cross(Rvector3(0,0,1), along);
      across /= across.GetMagnitude();
      const double mu = plan["mu"].toDouble();
      const QString shape = plan["shape"].toString("Spherical");
      const bool dragCase = plan["force_type"].toString() == "DragForce";
      const bool polyhedronCase = plan["force_type"].toString() == "PolyhedronGravityModel";
      const bool externalCase = plan["force_type"].toString() == "ExternalModel";
      const bool fileThrustCase = plan["force_type"].toString() == "FileThrust";
      const bool finiteCase = plan["force_type"].toString() == "FiniteThrust" || fileThrustCase;
      const QString densityFile = output + "/density.txt";
      const QString gravityFile = output + "/monopole.cof";
      const QString thrustFile = output + "/constant-acceleration.thf";
      const QString polyhedronFile = output + "/uniform-cube.txt";
      if (polyhedronCase)
      {
         QString shape = QString::number(plan["polyhedron_vertices_km"].toArray().size()) + "\n";
         int index = 0;
         for (const auto &vertex : plan["polyhedron_vertices_km"].toArray())
         {
            shape += QString::number(++index);
            for (const auto &value : vertex.toArray()) shape += " " + QString::number(value.toDouble(),'g',17);
            shape += "\n";
         }
         shape += QString::number(plan["polyhedron_faces"].toArray().size()) + "\n";
         index = 0;
         for (const auto &face : plan["polyhedron_faces"].toArray())
         {
            shape += QString::number(++index);
            for (const auto &value : face.toArray()) shape += " " + QString::number(value.toInt());
            shape += "\n";
         }
         save(polyhedronFile, shape.toUtf8());
      }
      if (fileThrustCase)
      {
         const auto vector = plan["file_acceleration_m_s2"].toArray();
         const QString acceleration = QString::number(vector[0].toDouble(),'g',17) + " " +
            QString::number(vector[1].toDouble(),'g',17) + " " +
            QString::number(vector[2].toDouble(),'g',17);
         save(thrustFile, ("BeginThrust {ThrustSegment1}\nStart_Epoch = 01 Jan 2000 00:00:00.000\n"
            "Thrust_Vector_Coordinate_System = " + plan["thrust_axes"].toString("EarthMJ2000Eq") + "\n"
            "Thrust_Vector_Interpolation_Method = Linear\n"
            "Mass_Flow_Rate_Interpolation_Method = None\nModelAccelOnly\n0 " + acceleration +
            "\n172800 " + acceleration + "\nEndThrust {ThrustSegment1}\n").toUtf8());
      }
      if (dragCase)
         save(densityFile, QString("0, %1, %2\n10000, %3, %2\n")
            .arg(plan["reference_density"].toDouble(),0,'g',17)
            .arg(plan["scale_height_km"].toDouble(),0,'g',17)
            .arg(plan["reference_density"].toDouble()*std::exp(-10000/plan["scale_height_km"].toDouble()),0,'g',17).toUtf8());
      if (plan["controlled_gravity"].toBool())
      {
         QString potential = QString("COMMENT Synthetic monopole for drag STM comparison\n"
            "POTFIELD  2  2  1 %1 %2 1.0\n")
            .arg(mu*1e9,0,'E',17).arg(plan["earth_radius_km"].toDouble()*1e3,0,'E',17);
         for (int order = 0; order <= 2; ++order)
            potential += "RECOEF  " + QString::number(2).rightJustified(3) +
               QString::number(order).rightJustified(3) + "   " +
               QString::number(0.0,'E',14).rightJustified(21) +
               QString::number(0.0,'E',14).rightJustified(21) + "\n";
         potential += "END\n";
         save(gravityFile,potential.toUtf8());
      }
      const QString spad = output + "/constant-vector.spo";
      if (shape == "SPADFile")
      {
         const auto templatePath = QString::fromStdString(
            FileManager::Instance()->GetFullPathname("VEHICLE_EPHEM_SPK_PATH")) +
            "../../spad/SphericalModel.spo";
         const auto source = read(templatePath);
         const auto marker = source.indexOf(" ------- -------");
         require(marker >= 0, "SPAD template columns missing");
         const auto dataStart = source.indexOf('\n',marker) + 1;
         QByteArray grid = source.left(dataStart);
         const auto vector = plan["spad_body_area_m2"].toArray();
         const auto azimuthSlope = plan["spad_azimuth_slope"].toArray();
         const auto elevationSlope = plan["spad_elevation_slope"].toArray();
         const auto azimuthQuadratic = plan["spad_azimuth_quadratic"].toArray();
         const auto elevationQuadratic = plan["spad_elevation_quadratic"].toArray();
         for (int azimuth=-180;azimuth<=180;azimuth+=5)
            for (int elevation=-90;elevation<=90;elevation+=5)
            {
               double area[3];
               for (int component=0;component<3;++component)
                  area[component] = vector[component].toDouble() +
                     (azimuthSlope.isEmpty() ? 0 : azimuthSlope[component].toDouble()*azimuth) +
                     (elevationSlope.isEmpty() ? 0 : elevationSlope[component].toDouble()*elevation) +
                     (azimuthQuadratic.isEmpty() ? 0 : azimuthQuadratic[component].toDouble()*azimuth*azimuth) +
                     (elevationQuadratic.isEmpty() ? 0 : elevationQuadratic[component].toDouble()*elevation*elevation);
               grid += QString("%1 %2 %3 %4 %5\n").arg(azimuth).arg(elevation)
                  .arg(area[0],0,'g',17).arg(area[1],0,'g',17)
                  .arg(area[2],0,'g',17).toUtf8();
            }
         save(spad,grid);
      }
      QString script = "Earth.Mu = " + QString::number(mu,'g',17) + ";\n";
      if (plan["spherical_atmosphere"].toBool())
         script += "Earth.Flattening = 0;\nEarth.EquatorialRadius = " +
            QString::number(plan["earth_radius_km"].toDouble(),'g',17) + ";\n";
      script += "Create Spacecraft Sat Ref;\n";
      const QStringList fields{"X", "Y", "Z", "VX", "VY", "VZ"};
      QJsonObject initialStates;
      for (const auto &name : {QString("Sat"), QString("Ref")})
      {
         const auto overrides = name == "Ref" ? plan["reference_ballistics"].toObject() : QJsonObject();
         const double radius = plan[name == "Sat" ? "radius" : "reference_radius"].toDouble();
         const double sign = plan["sunward"].toBool() ? 1 : -1;
         const Rvector3 position = sign*radius*along + plan["transverse_offset_km"].toDouble()*across;
         const Rvector3 velocity = plan["transverse_speed_km_s"].toDouble()*
            std::sqrt(plan["radius"].toDouble()/radius)*across;
         const double initial[] = {position[0],position[1],position[2],velocity[0],velocity[1],velocity[2]};
         script += name + ".CoordinateSystem = EarthMJ2000Eq;\n" +
            name + ".DisplayStateType = Cartesian;\n" + name + ".DateFormat = A1ModJulian;\n" +
            name + ".Epoch = '21545.125';\n";
         QJsonArray values;
         for (int i=0; i<6; ++i)
         {
            script += name + "." + fields[i] + " = " + QString::number(initial[i],'g',17) + ";\n";
            values.append(initial[i]);
         }
         const double fuelMass = finiteCase ? plan["fuel_mass_kg"].toDouble() : 0;
         script += name + ".DryMass = " + QString::number(overrides["mass_kg"].toDouble(plan["mass_kg"].toDouble())-fuelMass,'g',17) + ";\n" +
            name + ".SRPArea = " + QString::number(plan["area_m2"].toDouble(),'g',17) + ";\n" +
            name + ".Cr = " + QString::number(plan["cr"].toDouble(),'g',17) + ";\n";
         if (dragCase)
            script += name + ".Cd = " + QString::number(overrides["cd"].toDouble(plan["cd"].toDouble()),'g',17) + ";\n" +
               name + ".DragArea = " + QString::number(overrides["drag_area_m2"].toDouble(plan["drag_area_m2"].toDouble()),'g',17) + ";\n" +
               name + ".AtmosDensityScaleFactor = " + QString::number(overrides["density_scale"].toDouble(plan["density_scale"].toDouble(1)),'g',17) + ";\n";
         if (finiteCase && !fileThrustCase && name == "Sat")
         {
            script += "Create ChemicalTank Fuel;\nFuel.FuelMass = " + QString::number(fuelMass,'g',17) + ";\n"
               "Create ChemicalThruster Engine;\nEngine.Tank = {Fuel};\nEngine.DecrementMass = " +
               QString(plan["decrement_mass"].toBool() ? "true" : "false") + ";\n"
               "Engine.CoordinateSystem = Local;\nEngine.Origin = Earth;\nEngine.Axes = SpacecraftBody;\n"
               "Engine.C1 = " + QString::number(plan["thrust_newtons"].toDouble(),'g',17) + ";\n";
            if (plan["decrement_mass"].toBool())
               script += "Engine.K1 = " + QString::number(plan["isp_s"].toDouble(),'g',17) + ";\n"
                  "Engine.GravitationalAccel = " + QString::number(plan["gravity_m_s2"].toDouble(),'g',17) + ";\n";
            for (int component=0;component<3;++component)
               script += "Engine.ThrustDirection" + QString::number(component+1) + " = " +
                  QString::number(plan["thrust_direction"].toArray()[component].toDouble(),'g',17) + ";\n";
            script += "Sat.Tanks = {Fuel};\nSat.Thrusters = {Engine};\n"
               "Create FiniteBurn Burn;\nBurn.Thrusters = {Engine};\n";
         }
         if (finiteCase && !fileThrustCase && name == "Ref")
            script += "Ref.Tanks = {Fuel};\n";
         if (shape == "SPADFile")
         {
            const QString prefix = dragCase ? ".SPADDrag" : ".SPADSRP";
            script += name + prefix + "File = '" + spad + "';\n" +
               name + prefix + "ScaleFactor = " + QString::number(plan["spad_scale"].toDouble(),'g',17) + ";\n" +
               name + prefix + "InterpolationMethod = " + plan["interpolation"].toString("Bilinear") + ";\n";
         }
         if (plan["attitude"].toString() == "NadirPointing")
         {
            script += name + ".Attitude = NadirPointing;\n" +
               name + ".AttitudeReferenceBody = Earth;\n" +
               name + ".AttitudeConstraintType = " + plan["attitude_constraint"].toString("OrbitNormal") + ";\n";
            for (int component=0;component<3;++component)
            {
               const QString axis = QString("XYZ")[component];
               script += name + ".BodyAlignmentVector" + axis + " = " +
                  QString::number(plan["attitude_alignment"].toArray()[component].toDouble(),'g',17) + ";\n" +
                  name + ".BodyConstraintVector" + axis + " = " +
                  QString::number(plan["attitude_body_constraint"].toArray()[component].toDouble(),'g',17) + ";\n";
            }
         }
         else if (shape != "Spherical")
            script += name + ".Attitude = CoordinateSystemFixed;\n" +
               name + ".AttitudeCoordinateSystem = EarthMJ2000Eq;\n" +
               name + ".AttitudeDisplayStateType = Quaternion;\n" +
               name + ".Q1 = 0;\n" + name + ".Q2 = 0;\n" + name + ".Q3 = 0;\n" + name + ".Q4 = 1;\n";
         if (shape == "NPlate")
         {
            const auto plate = name + "Mirror";
            const auto normal = plan["plate_normal"].toArray(QJsonArray{0,0,1});
            script += "Create Plate " + plate + ";\n" + plate + ".Type = " +
               plan["plate_type"].toString("SunFacing") + ";\n" +
               plate + ".Area = " + QString::number(plan["plate_area_m2"].toDouble(),'g',17) + ";\n" +
               plate + ".AreaCoefficient = " + QString::number(plan["plate_area_coefficient"].toDouble(),'g',17) + ";\n" +
               plate + ".SpecularFraction = " + QString::number(plan["plate_specular"].toDouble(1),'g',17) + ";\n" +
               plate + ".DiffuseFraction = " + QString::number(plan["plate_diffuse"].toDouble(0),'g',17) + ";\n" +
               plate + ".LitFraction = " + QString::number(plan["plate_lit_fraction"].toDouble(1),'g',17) + ";\n" +
               plate + ".PlateNormal = [" + QString::number(normal[0].toDouble(),'g',17) + " " +
               QString::number(normal[1].toDouble(),'g',17) + " " + QString::number(normal[2].toDouble(),'g',17) +
               "];\n" + name + ".AddPlates = {" + plate + "};\n";
         }
         initialStates.insert(name,values);
      }
      save(output + "/initial-states.json",QJsonDocument(initialStates).toJson());
      if (fileThrustCase && plan["file_owner_frame"].toBool())
         script += "Create CoordinateSystem ThrustRTN;\nThrustRTN.Origin = Earth;\n"
            "ThrustRTN.Axes = ObjectReferenced;\nThrustRTN.Primary = Earth;\n"
            "ThrustRTN.Secondary = Sat;\nThrustRTN.XAxis = R;\nThrustRTN.ZAxis = N;\n";
      if (fileThrustCase)
         script += "Create ThrustSegment ThrustSegment1;\nCreate ThrustHistoryFile History;\n"
            "History.FileName = '" + thrustFile + "';\nHistory.AddThrustSegment = {ThrustSegment1};\n";
      for (const auto &entry : plan["cases"].toArray())
      {
         const auto spec = entry.toObject();
         const auto name = spec["key"].toString();
         script += "Create CoordinateSystem " + name + ";\n" +
            name + ".Origin = " + spec["origin"].toString() + ";\n" +
            name + ".Axes = ObjectReferenced;\n" +
            name + ".Primary = " + spec["primary"].toString() + ";\n" +
            name + ".Secondary = Sat;\n";
         if (spec.contains("axis_labels"))
         {
            const QStringList fields{"XAxis","YAxis","ZAxis"};
            const auto labels = spec["axis_labels"].toArray();
            for (int axis=0;axis<3;++axis)
               if (!labels[axis].toString().isEmpty())
                  script += name + "." + fields[axis] + " = " + labels[axis].toString() + ";\n";
         }
         else
            script += name + ".XAxis = " + spec["x_axis"].toString() + ";\n" + name + ".ZAxis = N;\n";
      }
      script += "Create ForceModel Gravity;\n";
      if (externalCase)
         script += "Gravity.PrimaryBodies = {};\nGravity.PointMasses = {};\nGravity.Drag = None;\nGravity.SRP = Off;\n"
            "Gravity.External = '" + plan["external_module"].toString() + "';\n"
            "Gravity.DerivativesFunction = 'GetDerivatives';\nGravity.External.ExcludeOtherForces = true;\n";
      else if (polyhedronCase)
         script += "Gravity.PolyhedralBodies = {Earth};\nGravity.CreateForceBody = Earth;\n"
            "Gravity.ShapeFileName = '" + polyhedronFile + "';\nGravity.BodyDensity = " +
            QString::number(plan["density_kg_m3"].toDouble(),'g',17) + ";\n"
            "Gravity.PrimaryBodies = {};\nGravity.PointMasses = {};\nGravity.Drag = None;\nGravity.SRP = Off;\n";
      else if (dragCase)
         script += "Gravity.PrimaryBodies = {Earth};\nGravity.PointMasses = {};\n"
            "Gravity.GravityField.Earth.Degree = 0;\nGravity.GravityField.Earth.Order = 0;\n"
            "Gravity.Drag.AtmosphereModel = Exponential;\nGravity.Drag.DragModel = '" + shape + "';\n"
            "Gravity.Drag.InputFile = '" + densityFile + "';\nGravity.SRP = Off;\n";
      else if (finiteCase)
         script += "Gravity.PrimaryBodies = {};\nGravity.PointMasses = {Earth};\n"
            "Gravity.Drag = None;\nGravity.SRP = Off;\n";
      else
         script += "Gravity.PrimaryBodies = {};\nGravity.PointMasses = {Earth};\n"
         "Gravity.Drag = None;\nGravity.SRP = On;\nGravity.SRP.SRPModel = " + shape + ";\n"
         "Gravity.SRP.Flux = " + QString::number(plan["flux_w_m2"].toDouble(),'g',17) + ";\n"
         "Gravity.SRP.Nominal_Sun = " + QString::number(plan["nominal_sun_km"].toDouble(),'g',17) + ";\n";
      if (plan["controlled_gravity"].toBool())
         script += "Gravity.GravityField.Earth.PotentialFile = '" + gravityFile + "';\n";
      script += "Gravity.RelativisticCorrection = Off;\nCreate Propagator P;\nP.FM = Gravity;\n"
         "P.Type = RungeKutta89;\nP.Accuracy = 1e-13;\nP.InitialStepSize = 1;\nP.MinStep = 1;\n"
         "P.MaxStep = 1;\nBeginMissionSequence;\n";
      if (finiteCase)
      {
         script.replace("BeginMissionSequence;\n",
            "Create Propagator Q;\nQ.FM = Gravity;\nQ.Type = RungeKutta89;\n"
            "Q.Accuracy = 1e-13;\nQ.InitialStepSize = 1;\nQ.MinStep = 1;\nQ.MaxStep = 1;\n"
            "BeginMissionSequence;\n");
         script += fileThrustCase ? "BeginFileThrust History(Sat);\n" : "BeginFiniteBurn Burn(Sat);\n";
      }
      script += "Propagate P(Sat, Ref, 'STM') {Sat.ElapsedSecs = 60, StopTolerance = 1e-6};\n";
      if (finiteCase)
         script.replace("P(Sat, Ref, 'STM')", "Synchronized P(Sat, 'STM') Q(Ref, 'STM')");
      const bool matrixOnly = plan["matrix_mode"].toString() == "AMatrix";
      if (matrixOnly)
         script.replace("'STM'", "'AMatrix'");
      if (plan["post_reports"].toBool())
      {
         QString resources = "Create Variable ReportPhase;\n";
         const QStringList matrixTypes{"OrbitSTM", "OrbitSTMA", "OrbitSTMB", "OrbitSTMC", "OrbitSTMD"};
         QStringList phases{"Powered", "Coast"};
         if (plan["restart_reports"].toBool())
            phases << "Restarted" << "FinalCoast";
         for (const auto &phase : phases)
         {
            if (phase == "Restarted")
               script += fileThrustCase ? "BeginFileThrust History(Sat);\n" : "BeginFiniteBurn Burn(Sat);\n";
            const bool powered = phase == "Powered" || phase == "Restarted";
            script += "ReportPhase = " + QString(powered ? "1" : "0") + ";\n";
            for (const auto &entry : plan["cases"].toArray())
            {
               const auto key = entry.toObject()["key"].toString();
               for (const auto &type : matrixTypes)
               {
                  const QString name = phase + key + type;
                  resources += "Create ReportFile " + name + ";\n" + name + ".Filename = '" +
                     output + "/" + name + ".txt';\n" + name + ".WriteHeaders = false;\n" +
                     name + ".Precision = 17;\n";
                  script += "Report " + name + " Sat." + key + "." + type + ";\n";
               }
            }
            if (powered)
               script += fileThrustCase ? "EndFileThrust History(Sat);\n" : "EndFiniteBurn Burn(Sat);\n";
         }
         script.replace("BeginMissionSequence;\n", resources + "BeginMissionSequence;\n");
      }
      const auto path = output + "/mission.script";
      save(path, script.toUtf8());
      const bool built = engine->InterpretScript(path.toStdString());
      save(output + "/build-messages.txt", QByteArray::fromStdString(messages.text));
      require(built, "Spherical SRP mission did not build");
      auto observe = [&]()
      {
         auto *sat = dynamic_cast<Spacecraft*>(engine->GetInternalObject("Sat"));
         auto *ref = dynamic_cast<Spacecraft*>(engine->GetInternalObject("Ref"));
         require(sat && ref, "Spherical SRP propagated spacecraft missing");
         sun = engine->GetSolarSystemInUse()->GetBody("Sun");
         earth = engine->GetSolarSystemInUse()->GetBody("Earth");
         require(sun && earth, "Executed spherical SRP bodies missing");
         PhysicalModel *field = nullptr;
         ODEModel *executedModel = nullptr;
         PropagationStateManager *stateManager = nullptr;
         double duration = 0;
         for (auto *command = engine->GetFirstCommand(); command; command = command->GetNext())
         {
            if (command->GetTypeName() != "Propagate")
               continue;
            auto *setup = dynamic_cast<PropSetup*>(command->GetClone(0));
            require(setup && setup->GetPropagator(), "Spherical SRP propagator missing");
            duration = setup->GetPropagator()->GetTime();
            auto *model = setup->GetODEModel();
            executedModel = model;
            stateManager = setup->GetPropStateManager();
            require(model && model->GetNumForces() == (polyhedronCase || externalCase ? 1 : 2), "Unexpected force-provider count");
            for (int index=0; index<model->GetNumForces(); ++index)
            {
               auto *candidate = model->GetForce(index);
               if (candidate->GetTypeName() == (externalCase ? "ExternalModel" : polyhedronCase ? "PolyhedronGravityModel" : dragCase ? "DragForce" :
                     fileThrustCase ? "FileThrust" : finiteCase ? "FiniteThrust" : "SolarRadiationPressure"))
                  field = candidate;
            }
            break;
         }
         require(field != nullptr, "Actual spherical SRP provider missing");
         QJsonObject plateProperties;
         if (shape == "NPlate")
         {
            auto *plate = engine->GetInternalObject("SatMirror");
            require(plate, "N-plate mirror missing");
            plateProperties = {{"type",QString::fromStdString(plate->GetStringParameter("Type"))},
               {"area",plate->GetRealParameter("Area")},
               {"area_coefficient",plate->GetRealParameter("AreaCoefficient")},
               {"specular",plate->GetRealParameter("SpecularFraction")},
               {"diffuse",plate->GetRealParameter("DiffuseFraction")},
               {"lit_fraction",plate->GetRealParameter("LitFraction")}};
            QJsonArray normal;
            const auto &direction=plate->GetRvectorParameter("PlateNormal");
            for (int component=0;component<3;++component) normal.append(direction[component]);
            plateProperties["normal"]=normal;
         }
         auto *drag = dynamic_cast<DragForce*>(field);
         if (dragCase)
         {
            require(drag && drag->GetAtmosphereModel(), "Drag atmosphere missing");
            QJsonObject provider{
               {"type",QString::fromStdString(drag->GetTypeName())}, {"body","Earth"},
               {"shape",QString::fromStdString(drag->GetStringParameter("DragModel"))},
               {"atmosphere",QString::fromStdString(drag->GetAtmosphereModel()->GetTypeName())},
               {"mass",sat->GetRealParameter("TotalMass")}, {"cd",sat->GetRealParameter("Cd")},
               {"area",sat->GetRealParameter("DragArea")}};
            if (shape == "SPADFile")
            {
               provider["spad_scale"] = sat->GetRealParameter("SPADDragScaleFactor");
               provider["interpolation"] = QString::fromStdString(sat->GetStringParameter("SPADDragInterpolationMethod"));
               provider["attitude"] = matrix(sat->GetAttitude(sat->GetEpoch()));
               provider["attitude_type"] = QString::fromStdString(sat->GetStringParameter("Attitude"));
            }
            if (plan.contains("reference_ballistics"))
               provider["reference_ballistics"] = QJsonObject{
                  {"mass_kg",ref->GetRealParameter("TotalMass")}, {"cd",ref->GetRealParameter("Cd")},
                  {"drag_area_m2",ref->GetRealParameter("DragArea")},
                  {"density_scale",ref->GetRealParameter("AtmosDensityScaleFactor")}};
            if (plan["controlled_gravity"].toBool())
            {
               auto *gravity = executedModel->GetForceOfType("GravityField");
               require(gravity, "Controlled gravity field missing");
               provider["gravity_mu"] = gravity->GetRealParameter("Mu");
               provider["gravity_degree"] = gravity->GetIntegerParameter("Degree");
               provider["gravity_order"] = gravity->GetIntegerParameter("Order");
            }
            if (plan["spherical_atmosphere"].toBool())
            {
               Real bodyState[6];
               require(drag->BuildModelState(sat->GetEpoch(),bodyState,sat->GetState().GetState()),
                       "Cannot obtain atmosphere density probe state");
               provider["density"] = drag->GetDensity(bodyState,sat->GetEpoch(),1);
               provider["earth_radius"] = earth->GetEquatorialRadius();
               provider["earth_flattening"] = earth->GetRealParameter(earth->GetParameterID("Flattening"));
            }
            save(output + "/force-provider.json", QJsonDocument(provider).toJson());
         }
         else if (externalCase)
            save(output + "/force-provider.json", QJsonDocument(QJsonObject{
               {"type",QString::fromStdString(field->GetTypeName())},
               {"module",QString::fromStdString(field->GetStringParameter("ScriptFileName"))},
               {"entry_point",QString::fromStdString(field->GetStringParameter("DerivativesFunction"))},
               {"dimension",field->GetDimension()}
            }).toJson());
         else if (polyhedronCase)
            save(output + "/force-provider.json", QJsonDocument(QJsonObject{
               {"type",QString::fromStdString(field->GetTypeName())},
               {"density",field->GetRealParameter(field->GetParameterID("BodyDensity"))},
               {"shape_file",QString::fromStdString(field->GetStringParameter("ShapeFileName"))}
            }).toJson());
         else if (fileThrustCase)
         {
            save(output + "/force-provider.json", QJsonDocument(QJsonObject{
               {"type",QString::fromStdString(field->GetTypeName())},
               {"mass",sat->GetRealParameter("TotalMass")},
               {"reference_mass",ref->GetRealParameter("TotalMass")},
               {"mode","ModelAccelOnly"},{"axes",plan["thrust_axes"].toString("EarthMJ2000Eq")},
               {"acceleration_m_s2",plan["file_acceleration_m_s2"]},
               {"attitude",matrix(sat->GetAttitude(sat->GetEpoch()))}
            }).toJson());
         }
         else if (finiteCase)
         {
            auto *thruster = sat->GetRefObject(Gmat::THRUSTER, "Engine");
            require(thruster, "Finite-thrust spacecraft engine missing");
            QJsonArray direction;
            for (int component=0;component<3;++component)
               direction.append(thruster->GetRealParameter(thruster->GetParameterID(
                  "ThrustDirection" + std::to_string(component+1))));
            QJsonObject provider{
               {"type",QString::fromStdString(field->GetTypeName())},
               {"mass",sat->GetRealParameter("TotalMass")},
               {"reference_mass",ref->GetRealParameter("TotalMass")},
               {"thrust_newtons",thruster->GetRealParameter(thruster->GetParameterID("C1"))},
               {"direction",direction},
               {"axes",QString::fromStdString(thruster->GetStringParameter(thruster->GetParameterID("Axes")))},
               {"decrement_mass",thruster->GetBooleanParameter(thruster->GetParameterID("DecrementMass"))},
               {"duty_cycle",thruster->GetRealParameter(thruster->GetParameterID("DutyCycle"))},
               {"scale_factor",thruster->GetRealParameter(thruster->GetParameterID("ThrustScaleFactor"))},
               {"attitude",matrix(sat->GetAttitude(sat->GetEpoch()))}
            };
            if (plan["decrement_mass"].toBool())
            {
               auto *fuel = sat->GetRefObject(Gmat::FUEL_TANK, "Fuel");
               require(fuel, "Finite-thrust fuel tank is missing");
               provider["fuel_mass"] = fuel->GetRealParameter(fuel->GetParameterID("FuelMass"));
               provider["dry_mass"] = sat->GetRealParameter("DryMass");
               provider["isp"] = thruster->GetRealParameter(thruster->GetParameterID("K1"));
               provider["gravity_accel"] = thruster->GetRealParameter(thruster->GetParameterID("GravitationalAccel"));
            }
            save(output + "/force-provider.json", QJsonDocument(provider).toJson());
         }
         else
            save(output + "/force-provider.json",QJsonDocument(QJsonObject{
            {"type",QString::fromStdString(field->GetTypeName())},{"body","Earth"},
            {"shape",QString::fromStdString(field->GetStringParameter("SRPModel"))},
            {"flux",field->GetRealParameter(field->GetParameterID("Flux"))},{"flux_pressure",field->GetRealParameter(field->GetParameterID("Flux_Pressure"))},
            {"nominal_sun",field->GetRealParameter(field->GetParameterID("Nominal_Sun"))},
            {"sun_radius",sun->GetEquatorialRadius()},{"earth_radius",earth->GetEquatorialRadius()},
            {"mass",sat->GetRealParameter("TotalMass")},{"area",sat->GetRealParameter("SRPArea")},
            {"cr",sat->GetRealParameter("Cr")},
            {"spad_scale",sat->GetRealParameter("SPADSRPScaleFactor")},
            {"spad_interpolation",QString::fromStdString(sat->GetStringParameter("SPADSRPInterpolationMethod"))},
            {"attitude",matrix(sat->GetAttitude(sat->GetEpoch()))}
            ,{"attitude_type",QString::fromStdString(sat->GetStringParameter("Attitude"))}
            ,{"plate",plateProperties}
         }).toJson());
         if (plan.contains("attitude_derivative_tolerance"))
         {
            GmatTime epoch = sat->GetEpochGT();
            QJsonArray derivatives;
            for (const auto &value : sat->GetAttitudeRotationMatrixDerivative(epoch))
               derivatives.append(matrix(value));
            save(output + "/attitude-derivatives.json", QJsonDocument(QJsonObject{
               {"state",state(sat)}, {"epoch",epoch.GetMjd()},
               {"matrices",derivatives}
            }).toJson());
         }
         QJsonArray poses;
         for (int index=0; index<=60; ++index)
         {
            const double seconds=duration*index/60;
            const QJsonArray position=forceSunPosition(sun, earth,
               GmatTime(21545.125+seconds/86400), false);
            QJsonObject sample{{"seconds",seconds},{"earth_to_sun",position}};
            if (polyhedronCase)
               sample["body_rotation"] = bodyRotation(earth, engine->GetSolarSystemInUse(), GmatTime(21545.125+seconds/86400));
            if (drag)
            {
               const Real *omega = drag->GetAtmosphereModel()->GetAngularVelocity(21545.125+seconds/86400);
               sample["angular_velocity"] = QJsonArray{omega[0],omega[1],omega[2]};
            }
            poses.append(sample);
         }
         save(output + "/force-poses.json",QJsonDocument(QJsonObject{{"samples",poses}}).toJson());
         if (plan.contains("plate_derivative_tolerance"))
         {
            auto *plate = dynamic_cast<Plate*>(engine->GetInternalObject("SatMirror"));
            require(plate, "Plate derivative probe is missing its plate");
            GmatTime epoch = sat->GetEpochGT();
            auto rotation = sat->GetAttitudeRotationMatrix(epoch);
            auto rotationDerivatives = sat->GetAttitudeRotationMatrixDerivative(epoch);
            const auto sunPosition = forceSunPosition(sun, earth, epoch, false);
            Rvector3 sunVector;
            for (int component = 0; component < 3; ++component)
               sunVector[component] = sat->GetState()[component] - sunPosition[component].toDouble();
            const auto bodyRoute = plate->GetReflectanceDerivative(sunVector, rotation, rotationDerivatives, epoch);
            const auto inertialRoute = plate->GetReflectanceDerivativeI(sunVector, rotation, rotationDerivatives, epoch);
            QJsonArray bodyValues, inertialValues;
            for (const auto &value : bodyRoute)
               bodyValues.append(QJsonArray{value[0],value[1],value[2]});
            for (const auto &value : inertialRoute)
               inertialValues.append(QJsonArray{value[0],value[1],value[2]});
            save(output + "/plate-derivatives.json", QJsonDocument(QJsonObject{
               {"state",state(sat)}, {"sun",sunPosition},
               {"body_route",bodyValues}, {"inertial_route",inertialValues}
            }).toJson());
         }
         OrbitStmForceGradient::Request gradientRequest(field);
         const auto derivative=field->GetDerivativesForSpacecraft(sat);
         QJsonArray acceleration;
         for (int i=3;i<6;++i) acceleration.append(derivative[i]);
         QJsonObject query{
            {"available",gradientRequest.available},{"gradient",matrix(gradientRequest.value)},
            {"acceleration",acceleration}};
         if (field->GetTypeName() == "SolarRadiationPressure")
            query["illumination"] = field->GetRealParameter(field->GetParameterID("PercentSun"));
         save(output + "/queried-gradient.json",QJsonDocument(query).toJson());
         if (externalCase && plan["callback_error_checks"].toBool())
         {
            const auto ownerBefore = snapshot(sat);
            const auto guideBefore = snapshot(ref);
            const int dimension = field->GetDimension();
            const std::vector<Real> derivatives(field->GetDerivativeArray(),field->GetDerivativeArray()+dimension);
            const auto entry = field->GetStringParameter("DerivativesFunction");
            struct RestoreEntry
            {
               PhysicalModel *field;
               std::string entry;
               ~RestoreEntry() { field->SetStringParameter("DerivativesFunction",entry); }
            } restoreEntry{field,entry};
            QJsonArray failures;
            for (const auto &name : {"BadLength","NonFinite","NonNumeric","Raises"})
            {
               field->SetStringParameter("DerivativesFunction",name);
               for (bool native : {false,true})
               {
                  bool rejected = false;
                  QString message;
                  try
                  {
                     if (native)
                        field->GetDerivatives(executedModel->GetState());
                     else
                        field->GetDerivativesForSpacecraft(sat);
                  }
                  catch (const BaseException &error)
                  {
                     rejected = true;
                     message = QString::fromStdString(error.GetFullMessage());
                  }
                  const bool atomic = std::equal(derivatives.begin(),derivatives.end(),field->GetDerivativeArray()) &&
                     snapshot(sat) == ownerBefore && snapshot(ref) == guideBefore;
                  require(rejected && atomic,"Invalid external callback did not reject atomically");
                  failures.append(QJsonObject{{"entry",name},{"native",native},{"rejected",rejected},
                     {"atomic",atomic},{"message",message}});
               }
               field->SetStringParameter("DerivativesFunction",entry);
               const auto recovered = field->GetDerivativesForSpacecraft(sat);
               for (int i=0;i<6;++i)
                  require(std::abs(recovered[i]-derivative[i]) < 1e-13,"External callback did not recover after rejection");
            }
            save(output + "/callback-rejections.json",QJsonDocument(QJsonObject{
               {"cases",failures},{"recovered",true},{"state_unchanged",snapshot(sat)==ownerBefore && snapshot(ref)==guideBefore}
            }).toJson());
         }
         if (finiteCase)
         {
            const auto current = OrbitStmContext::Capture(sat);
            const auto total = executedModel->GetDerivativesForSpacecraft(sat);
            save(output + "/current-acceleration.json", QJsonDocument(QJsonObject{
               {"captured_available",current.reference->self->accelerationAvailable},
               {"captured",QJsonArray{current.reference->self->acceleration[0],
                  current.reference->self->acceleration[1],current.reference->self->acceleration[2]}},
               {"direct_model",QJsonArray{total[3],total[4],total[5]}},
               {"model",QString::fromStdString(executedModel->GetName())}
            }).toJson());
         }
         if (dragCase && plan["zero_speed_gradient"].toBool())
         {
            const auto before = snapshot(sat);
            std::unique_ptr<Spacecraft> stationary(dynamic_cast<Spacecraft*>(sat->Clone()));
            require(stationary && stationary.get() != sat && stationary->InitializeClonedResource(),
                    "Cannot initialize zero-relative-speed drag probe");
            Real bodyState[6];
            require(drag->BuildModelState(stationary->GetEpoch(),bodyState,stationary->GetState().GetState()),
                    "Cannot construct zero-relative-speed body state");
            const Real *omega = drag->GetAtmosphereModel()->GetAngularVelocity(stationary->GetEpoch());
            for (int component = 0; component < 3; ++component)
            {
               const double rotation = omega[(component+1)%3]*bodyState[(component+2)%3] -
                  omega[(component+2)%3]*bodyState[(component+1)%3];
               stationary->GetState()[component+3] = rotation +
                  (stationary->GetState()[component+3] - bodyState[component+3]);
            }
            OrbitStmForceGradient::Request zeroRequest(drag);
            const auto zero = drag->GetDerivativesForSpacecraft(stationary.get());
            save(output + "/zero-speed-gradient.json",QJsonDocument(QJsonObject{
               {"available",zeroRequest.available}, {"gradient",matrix(zeroRequest.value)},
               {"acceleration",QJsonArray{zero[3],zero[4],zero[5]}},
               {"live_spacecraft_unchanged",before==snapshot(sat)}
            }).toJson());
         }
         if (matrixOnly)
         {
            require(executedModel && stateManager, "SPAD A-matrix propagation state missing");
            const int dimension = executedModel->GetDimension();
            std::vector<Real> inputState(executedModel->GetState(), executedModel->GetState()+dimension);
            const auto &mapping = *stateManager->GetStateMap();
            int matrixStart = -1;
            int guideMatrixStart = -1;
            for (int index=0;index<static_cast<int>(mapping.size());++index)
            {
               const auto *item = mapping[index];
               if (item->elementID == Gmat::CARTESIAN_STATE && item->subelement == 1)
               {
                  auto *owner = dynamic_cast<Spacecraft*>(item->object);
                  require(owner && index+6<=dimension, "SPAD Cartesian association missing");
                  if (finiteCase || polyhedronCase || externalCase)
                  {
                     auto *origin = field->GetForceOrigin();
                     require(origin, "Finite-thrust force origin is missing");
                     const auto offset = origin->GetMJ2000State(owner->GetEpoch());
                     for (int component=0;component<6;++component)
                        inputState[index+component] = owner->GetState()[component]-offset[component];
                  }
                  else
                     require(field->BuildModelState(owner->GetEpoch(), inputState.data()+index,
                        owner->GetState().GetState()), "SPAD A-matrix state conversion failed");
               }
               if (item->elementID == Gmat::ORBIT_A_MATRIX && item->object == sat && item->subelement == 1)
                  matrixStart = index;
               if (item->elementID == Gmat::ORBIT_A_MATRIX && item->object == ref && item->subelement == 1)
                  guideMatrixStart = index;
            }
            require(matrixStart>=0 && matrixStart+36<=dimension, "SPAD A-matrix association missing");
            const Real oldTime = field->GetTime();
            const Real forceEpoch = field->GetRealParameter(field->PhysicalModel::GetParameterID("Epoch"));
            const Real probeTime = (sat->GetEpoch()-forceEpoch)*86400;
            field->SetTime(probeTime);
            try
            {
               require(field->GetDerivatives(inputState.data(),0.0), "SPAD A-matrix evaluation failed");
            }
            catch (...)
            {
               field->SetTime(oldTime);
               throw;
            }
            field->SetTime(oldTime);
            const Real *derivatives = field->GetDerivativeArray();
            Rmatrix value(6,6);
            for (int row=0;row<6;++row)
               for (int column=0;column<6;++column)
                  value(row,column) = derivatives[matrixStart+row*6+column];
            QJsonObject proof{
               {"status","procedure-completed"},{"matrix_mode","AMatrix"},
               {"force_base_epoch",forceEpoch},{"probe_elapsed_seconds",probeTime},
               {"probe_epoch",forceEpoch+probeTime/86400},
               {"integration_seconds",duration},{"owner_state",state(sat)},
               {"guide_state",state(ref)},{"force_amatrix",matrix(value)},
               {"native_missions",1}};
            if (plan["check_reference_amatrix"].toBool())
            {
               require(guideMatrixStart>=0 && guideMatrixStart+36<=dimension,
                       "Reference A-matrix association missing");
               Rmatrix guideValue(6,6);
               for (int row=0;row<6;++row)
                  for (int column=0;column<6;++column)
                     guideValue(row,column)=derivatives[guideMatrixStart+row*6+column];
               proof["guide_amatrix"]=matrix(guideValue);
            }
            save(output + "/proof.json", QJsonDocument(proof).toJson());
            std::cout << "SPAD A-matrix-only mission completed\n";
            return;
         }
         const auto history = OrbitStmContext::Get(sat);
         require(history.reference && history.reference->self, "Spherical SRP reference history missing");
         QJsonArray initialAcceleration;
         for (int i=0;i<3;++i) initialAcceleration.append(history.reference->self->acceleration[i]);
         QJsonArray guideAcceleration;
         const auto guideHistory = OrbitStmContext::Historical(history, ref);
         for (int i=0;i<3;++i) guideAcceleration.append(guideHistory.acceleration[i]);
         save(output + "/reference-acceleration.json",QJsonDocument(QJsonObject{
            {"owner_acceleration",initialAcceleration},{"guide_acceleration",guideAcceleration},
            {"gradient",history.reference->self->accelerationGradient ?
               matrix(*history.reference->self->accelerationGradient) : QJsonArray()}}).toJson());
         if (dragCase)
            save(output + "/frame-force-inputs.json",QJsonDocument(QJsonObject{
               {"initial",poses.first().toObject()["angular_velocity"]},
               {"final",poses.last().toObject()["angular_velocity"]}
            }).toJson());
         else if (polyhedronCase)
            save(output + "/frame-sun-inputs.json",QJsonDocument(QJsonObject{
               {"initial",bodyRotation(earth,engine->GetSolarSystemInUse(),history.reference->epoch)},
               {"final",bodyRotation(earth,engine->GetSolarSystemInUse(),OrbitStmContext::Epoch(sat))},
               {"initial_epoch",history.reference->epoch.GetMjd()}, {"final_epoch",sat->GetEpoch()}
            }).toJson());
         else
            save(output + "/frame-sun-inputs.json",QJsonDocument(QJsonObject{
            {"initial",forceSunPosition(sun,earth,history.reference->epoch,field->HasPrecisionTime())},
            {"final",forceSunPosition(sun,earth,OrbitStmContext::Epoch(sat),field->HasPrecisionTime())},
            {"initial_epoch",history.reference->epoch.GetMjd()},
            {"final_epoch",sat->GetEpoch()}, {"precise",field->HasPrecisionTime()}
         }).toJson());

         const QStringList types{"OrbitSTM", "OrbitSTMA", "OrbitSTMB", "OrbitSTMC", "OrbitSTMD"};
         QJsonArray observations;
         QJsonObject selectedAxes;
         for (const auto &entry : plan["cases"].toArray())
         {
            const auto key = entry.toObject()["key"].toString();
            auto *frame = dynamic_cast<CoordinateSystem*>(engine->GetInternalObject(key.toStdString()));
            require(frame && frame->GetSecondaryObject() == sat, "Spherical SRP frame owner differs");
            if (entry.toObject().contains("axis_labels"))
            {
               QJsonArray labels;
               for (const auto &field : {"XAxis","YAxis","ZAxis"})
                  labels.append(QString::fromStdString(frame->GetAxisSystem()->GetStringParameter(field)));
               selectedAxes.insert(key,labels);
            }
            for (int index = 0; index < types.size(); ++index)
            {
               auto original = parameter(index, sat, frame, engine);
               std::unique_ptr<Parameter> clone(dynamic_cast<Parameter*>(original->Clone()));
               std::unique_ptr<Parameter> copy(dynamic_cast<Parameter*>(original->Clone()));
               require(clone && copy, "Spherical SRP parameter clone missing");
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
         if (!selectedAxes.isEmpty())
            save(output + "/axis-configuration.json",QJsonDocument(selectedAxes).toJson());
         save(output + "/proof.json", QJsonDocument(QJsonObject{
            {"status", "procedure-completed"}, {"integration_seconds", duration},
            {"owner_state", state(sat)}, {"guide_state", state(ref)},
            {"native_stm", matrix(sat->GetRmatrixParameter("OrbitSTM"))},
            {"guide_stm", matrix(ref->GetRmatrixParameter("OrbitSTM"))},
            {"observations", observations}, {"native_missions", 1}
         }).toJson());
      };
      FiniteThrustObserver observer;
      observer.sample = observe;
      QJsonArray reportSamples;
      if (plan["post_reports"].toBool())
         observer.beforeCommand = [&](GmatCommand *command)
         {
            if (command->GetTypeName() != "Report")
               return;
            const int phase = static_cast<int>(engine->GetInternalObject("ReportPhase")->GetRealParameter("Value"));
            if (!reportSamples.isEmpty() && reportSamples.last().toObject()["phase"].toInt() == phase)
               return;
            auto *sat = dynamic_cast<Spacecraft*>(engine->GetInternalObject("Sat"));
            auto *ref = dynamic_cast<Spacecraft*>(engine->GetInternalObject("Ref"));
            require(sat && ref, "Post-propagation report spacecraft missing");
            const auto before = snapshot(sat);
            const auto guideBefore = snapshot(ref);
            const auto current = OrbitStmContext::Capture(sat);
            QJsonArray reads;
            for (const auto &entry : plan["cases"].toArray())
            {
               const auto key = entry.toObject()["key"].toString();
               auto *frame = dynamic_cast<CoordinateSystem*>(engine->GetInternalObject(key.toStdString()));
               for (int index=0;index<5;++index)
               {
                  auto value = parameter(index, sat, frame, engine);
                  reads.append(QJsonObject{{"case",key},{"type",QStringList{
                     "OrbitSTM","OrbitSTMA","OrbitSTMB","OrbitSTMC","OrbitSTMD"}[index]},
                     {"actual",matrix(value->EvaluateRmatrix())},{"readonly",!value->IsSettable()}});
               }
            }
            reportSamples.append(QJsonObject{{"phase",phase},{"owner_state",state(sat)},
               {"guide_state",state(ref)},{"owner_unchanged",before==snapshot(sat)},
               {"guide_unchanged",guideBefore==snapshot(ref)},{"reads",reads},
               {"acceleration",QJsonArray{current.reference->self->acceleration[0],
                  current.reference->self->acceleration[1],current.reference->self->acceleration[2]}}});
            save(output + "/post-report-samples.json",QJsonDocument(reportSamples).toJson());
         };
      auto *previousObserver = finiteCase ? GmatCommand::SetExecutionObserver(&observer) : nullptr;
      struct RestoreObserver
      {
         bool enabled;
         CommandExecutionObserver *previous;
         ~RestoreObserver() { if (enabled) GmatCommand::SetExecutionObserver(previous); }
      } restoreObserver{finiteCase, previousObserver};
      messages.text.clear();
      const int status = engine->RunMission();
      save(output + "/run-messages.txt", QByteArray::fromStdString(messages.text));
      save(output + "/native-status.json", QJsonDocument(QJsonObject{
         {"build_status", built}, {"run_status", status}}).toJson());
      require(status == 1, "Spherical SRP mission did not complete");

      if (finiteCase)
      {
         require(observer.error.isEmpty(), observer.error.toUtf8().constData());
         require(observer.observed, "Finite-thrust live endpoint was not observed");
      }
      else
         observe();
      const auto finalReady = qEnvironmentVariable("GMAT_STM_FINAL_MAP_READY");
      const auto finalAck = qEnvironmentVariable("GMAT_STM_FINAL_MAP_ACK");
      if (!finalReady.isEmpty())
      {
         save(finalReady,QByteArray::number(QCoreApplication::applicationPid()));
         QElapsedTimer timer;
         timer.start();
         while (!QFile::exists(finalAck) && timer.elapsed() < 10000)
            QThread::msleep(5);
         require(QFile::exists(finalAck),"Final mapped images were not acknowledged");
      }
      std::cout << "Spherical SRP STM mission and 45 parameter reads completed\n";
      return 0;
   }
   catch (BaseException &error) { std::cerr << error.GetFullMessage(); }
   catch (std::exception &error) { std::cerr << error.what(); }
   return 1;
}
