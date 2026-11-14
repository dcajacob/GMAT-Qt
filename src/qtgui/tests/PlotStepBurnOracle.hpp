#ifndef GMAT_PLOT_STEP_BURN_ORACLE_HPP
#define GMAT_PLOT_STEP_BURN_ORACLE_HPP
#include "FormationAcceptance.hpp"
#include <algorithm>

namespace plotburn {
struct Settings {
   bool electric=false,decrement=false,restart=false,nonlinear=false;
   double stop() const {return nonlinear?37.123:37.;}
   double thrust() const {return electric?.5:100.;} // N
   double isp() const {return electric?2500.:300.;} // s, prescribed g0=10 m/s^2
};
inline Settings settings(const QString &name) {
   return {name.contains("Electric"),!name.endsWith("Fixed"),name.endsWith("Restart"),name.endsWith("Nonlinear")};
}
// State relative to the prescribed central body: x,y,z [km], vx,vy,vz
// [km/s], fuel and total mass [kg]. Constant inertial force, no hidden GMAT
// propagation or parameter evaluation. Residual gravity is <3e-25 km over 50 s.
inline std::array<double,8> state(const Settings &s,double time,double measuredBurnStop=-1) {
   long double x=7000,vx=0,mass=1000;const double stop=measuredBurnStop>=0?measuredBurnStop:s.stop();
   const std::vector<double> boundaries=s.restart?std::vector<double>{0,10,27,50}:std::vector<double>{0,stop,stop+13};
   for(size_t i=0;i+1<boundaries.size();++i) {
      const long double dt=std::max(0.,std::min(time,boundaries[i+1])-boundaries[i]);
      if(dt==0)continue;
      const bool firing=s.restart?i!=1:i==0;
      x+=vx*dt;
      if(firing) {
         const long double force=s.thrust(),exhaust=s.isp()*10.L/1000.L;
         if(s.decrement) {
            const long double q=force/(s.isp()*10.L),fraction=q*dt/mass;
            const long double logRatio=std::log1p(-fraction);
            x+=exhaust*(dt+(mass/q-dt)*logRatio);
            vx-=exhaust*logRatio;mass-=q*dt;
         } else {x+=force/mass/1000.L*dt*dt/2;vx+=force/mass/1000.L*dt;}
      }
   }
   return {double(x),300+time,500+.2*time,double(vx),1,.2,double(mass-900),double(mass)};
}
inline QString mission(const Settings &s) {
   const QString report="Report Values One.ElapsedSecs One.ProbeEq.X One.ProbeEq.Y One.ProbeEq.Z One.ProbeEq.VX One.ProbeEq.VY One.ProbeEq.VZ One.Fuel.FuelMass One.TotalMass;\n";
   QString text=report+"BeginFiniteBurn Burn(One);\nPropagate P(One) {One.ElapsedSecs = 10};\n"+report;
   if(s.restart)text+="EndFiniteBurn Burn(One);\nPropagate P(One) {One.ElapsedSecs = 17};\n"+report+"BeginFiniteBurn Burn(One);\nPropagate P(One) {One.ElapsedSecs = 23};\nEndFiniteBurn Burn(One);\n"+report;
   else {
      const QString advance=s.nonlinear?"Propagate P(One) {One.ProbeEq.X = "+formaccept::number(state(s,s.stop())[0])+", StopTolerance = 1e-10};\n":"Propagate P(One) {One.ElapsedSecs = 27};\n";
      text+=advance+report+"EndFiniteBurn Burn(One);\nPropagate P(One) {One.ElapsedSecs = 13};\n"+report;
   }
   return text;
}
inline QString configure(QString setup,const Settings &s) {
   const auto kernel=QFileInfo("../data/planetary_ephem/spk/ceres_1900_2100.bsp").canonicalFilePath();
   formaccept::require(!kernel.isEmpty(),"Burn oracle shared ephemeris kernel missing");
   const QString body="Create Asteroid Probe;\nProbe.NAIFId = 2000001;\nProbe.OrbitSpiceKernelName = {'"+kernel+"'};\nProbe.Mu = 1e-20;\nProbe.EquatorialRadius = 1000;\nProbe.Flattening = 0;\nProbe.OrientationEpoch = 21545;\nProbe.SpinAxisRAConstant = -90;\nProbe.SpinAxisRARate = 0;\nProbe.SpinAxisDECConstant = 90;\nProbe.SpinAxisDECRate = 0;\nProbe.RotationConstant = 0;\nProbe.RotationRate = 0;\nCreate CoordinateSystem ProbeEq;\nProbeEq.Origin = Probe;\nProbeEq.Axes = MJ2000Eq;\n";
   setup.replace(".CoordinateSystem = EarthMJ2000Eq;",".CoordinateSystem = ProbeEq;");
   setup.replace("FM.PrimaryBodies = {};","FM.CentralBody = Probe;\nFM.PrimaryBodies = {};");
   setup.replace("FM.PointMasses = {Earth};","FM.PointMasses = {Probe};");
   setup.replace("Create GroundTrackPlot Map;","Create GroundTrackPlot Map;\nMap.CentralBody = Probe;");
   const QStringList fields{"X","Y","Z","VX","VY","VZ"};const auto initial=state(s,0);
   for(int i=0;i<6;++i)setup.replace("One."+fields[i]+" = "+formaccept::number(formaccept::circular(formaccept::orbits[0],0)[i])+";","One."+fields[i]+" = "+formaccept::number(initial[i])+";");
   QString hardware="Create "+QString(s.electric?"ElectricTank":"ChemicalTank")+" Fuel;\nFuel.FuelMass = 100;\nCreate "+QString(s.electric?"ElectricThruster":"ChemicalThruster")+" Engine;\nEngine.Tank = {Fuel};\nEngine.CoordinateSystem = ProbeEq;\nEngine.ThrustDirection1 = 1;\nEngine.ThrustDirection2 = 0;\nEngine.ThrustDirection3 = 0;\nEngine.DutyCycle = 1;\nEngine.ThrustScaleFactor = 1;\nEngine.GravitationalAccel = 10;\nEngine.DecrementMass = "+QString(s.decrement?"true":"false")+";\n";
   if(s.electric)hardware+="Engine.ThrustModel = ConstantThrustAndIsp;\nEngine.ConstantThrust = 0.5;\nEngine.Isp = 2500;\nEngine.MinimumUsablePower = 1;\nEngine.MaximumUsablePower = 10;\nCreate NuclearPowerSystem Power;\nPower.InitialMaxPower = 10;\nPower.AnnualDecayRate = 0;\nPower.Margin = 0;\nPower.BusCoeff1 = 0;\nPower.BusCoeff2 = 0;\nPower.BusCoeff3 = 0;\nOne.PowerSystem = Power;\n";
   else {
      hardware+="Fuel.PressureModel = PressureRegulated;\n";
      for(int i=1;i<=16;++i)hardware+="Engine.C"+QString::number(i)+" = "+QString::number(i==1?100:0)+";\nEngine.K"+QString::number(i)+" = "+QString::number(i==1?300:0)+";\n";
   }
   hardware+="One.DryMass = 900;\nOne.Tanks = {Fuel};\nOne.Thrusters = {Engine};\nCreate FiniteBurn Burn;\nBurn.Thrusters = {Engine};\n";
   setup.replace("BeginMissionSequence;",hardware+"BeginMissionSequence;");
   return body+setup;
}
inline QJsonArray checkReport(formaccept::Rig &rig,const QString &name,const Settings &s) {
   const auto bytes=formaccept::read(rig.report);rig.save(name+"-report.csv",bytes);
   const auto lines=QString::fromUtf8(bytes).trimmed().split('\n');
   formaccept::require(lines.size()==4,"Burn endpoint report row count differs");
   const std::vector<double> times=s.restart?std::vector<double>{0,10,27,50}:std::vector<double>{0,10,s.stop(),s.stop()+13};
   const double stop=s.nonlinear?lines[2].section(',',0,0).toDouble():s.stop();
   formaccept::require(std::isfinite(stop)&&std::abs(stop-s.stop())<1e-4,"Burn measured stop epoch exceeds literal grid tolerance");
   QJsonArray rows;
   for(int i=0;i<4;++i) {
      const auto fields=lines[i].split(',');formaccept::require(fields.size()==9,"Burn report schema differs");
      const double actualTime=fields[0].toDouble();
      const auto truth=state(s,s.nonlinear?actualTime:times[i],stop);QJsonArray row;
      for(int j=0;j<9;++j) {
         bool ok=false;const double observed=fields[j].toDouble(&ok),wanted=j==0?times[i]:truth[j-1];
         const double tolerance=j==0?1e-4:j<=3?2e-6:j<=6?2e-9:1e-7;
         formaccept::require(ok&&std::isfinite(observed)&&std::abs(observed-wanted)<tolerance,"Burn report differs from independent thrust/mass-flow solution");row.append(observed);
      }
      rows.append(row);
   }
   return rows;
}
}
#endif
