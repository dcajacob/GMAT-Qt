#include "Moderator.hpp"
#include "PropSetup.hpp"
#include "PropagationStateManager.hpp"
#include "ODEModel.hpp"
#include "Spacecraft.hpp"
#include "ObjectReferencedAxes.hpp"
#include "CoordinateSystem.hpp"
#include "MJ2000EcAxes.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

static void require(bool ok,const char *message) {if(!ok)throw std::runtime_error(message);}
using V=std::array<double,3>;
using M=std::array<V,3>;
static V cross(const V&a,const V&b) {return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
static V unit(V a) {double n=0;for(auto x:a)n+=x*x;n=std::sqrt(n);for(auto &x:a)x/=n;return a;}
// Position/velocity-only basis on a prescribed local quadratic trajectory.
// This finite-difference oracle does not use native rotation derivatives.
static M basis(const V&r,const V&v,const V&a,double t)
{
   V p{},w{};for(int i=0;i<3;++i){p[i]=r[i]+t*v[i]+.5*t*t*a[i];w[i]=v[i]+t*a[i];}
   const auto x=unit(w),y=unit(cross(p,w)),z=cross(x,y);M result{};
   for(int i=0;i<3;++i){result[i][0]=x[i];result[i][1]=y[i];result[i][2]=z[i];}return result;
}
class AxesProbe:public ObjectReferencedAxes
{
public:
   AxesProbe():ObjectReferencedAxes("ContextAxes"){}
   void calculate(const A1Mjd&t){CalculateRotationMatrix(t,true);}
};
class GenericForce:public PhysicalModel
{
public:
   int mode=0,calls=0;Spacecraft *last=nullptr;SpacePoint *origin=nullptr;
   GenericForce():PhysicalModel(Gmat::PHYSICAL_MODEL,"AcceptanceGenericForce","AcceptanceGenericForce"){}
   GmatBase *Clone()const override {return new GenericForce(*this);}
   bool HasLocalClones()override {return false;}
   bool RenameRefObject(const UnsignedInt,const std::string &,const std::string &)override {return false;}
   bool Initialize()override {isInitialized=true;return true;}
   Rvector6 GetDerivativesForSpacecraft(Spacecraft *sc)override
   {
      ++calls;last=sc;
      if(mode==1)throw ODEModelException("intentional force-query failure");
      if(mode==2){Rvector3 unused;ODEModel::GetBoundRelativeAcceleration(sc,origin,A1Mjd(sc->GetEpoch()),unused);}
      if(mode==3)return PhysicalModel::GetDerivativesForSpacecraft(sc);
      const auto *state=sc->GetState().GetState();const double mass=sc->GetRealParameter(sc->GetParameterID("TotalMass"));
      Rvector6 result;
      result[3]=2e-6*state[3]+.002/mass;
      result[4]=-3e-6*state[4];
      result[5]=4e-6*state[5]+.0007;
      if(mode==4)result[5]=std::numeric_limits<double>::quiet_NaN();
      return result;
   }
};
int main(int argc,char **argv)
{
   QCoreApplication app(argc,argv);
   try {
      require(argc==3,"Supply startup and Force/Lifetime context case");const QString key=argv[2];require(key=="Force"||key=="Lifetime","Unknown acceleration context case");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      auto *m=Moderator::Instance();require(m->Initialize(startup.toStdString(),false),"Acceleration context runtime unavailable");
      std::istringstream script("Create Spacecraft Sat;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 130;\nSat.Z = -200;\nSat.VX = -0.3;\nSat.VY = 7.8;\nSat.VZ = 0.8;\nCreate ForceModel Forces;\nForces.PrimaryBodies = {};\nForces.PointMasses = {Earth};\nForces.Drag = None;\nForces.SRP = Off;\nCreate Propagator P;\nP.FM = Forces;\nP.MinStep = 0;\nP.MaxStep = 1;\nBeginMissionSequence;\nPropagate P(Sat) {Sat.ElapsedSecs = 1};\n");
      require(m->InterpretScript(&script,true)&&m->RunMission()==1,"Acceleration context seed mission failed");
      auto *sc=dynamic_cast<Spacecraft *>(m->GetInternalObject("Sat"));auto *command=m->GetFirstCommand();while(command&&!command->IsOfType("Propagate"))command=command->GetNext();
      auto *setup=command?dynamic_cast<PropSetup *>(command->GetClone(0)):nullptr;auto *model=setup?setup->GetODEModel():nullptr;
      require(sc&&model,"Actual accepted spacecraft or native force clone missing");auto *earth=model->GetForceOrigin();require(earth&&earth->GetName()=="Earth","Unexpected seed force origin");
      auto query=[&] {Rvector3 value;require(ODEModel::GetBoundRelativeAcceleration(sc,earth,A1Mjd(sc->GetEpoch()),value),"Live acceleration context absent");return value;};
      if(key=="Force") {
         auto *force=new GenericForce;force->origin=earth;model->AddForce(force);require(model->Initialize(),"Generic force model initialization failed");model->ActivateAccelerationContext();
         AxesProbe axes;axes.SetPrimaryObject(earth);axes.SetSecondaryObject(sc);axes.ResetAxes();axes.SetXAxis("V");axes.SetYAxis("N");
         const double mu=earth->GetRealParameter(earth->GetParameterID("Mu"));int checked=0,rejected=0;
         for(const auto &s:std::array<std::array<double,6>,3>{{{{7000,130,-200,-.3,7.8,.8}},{{7200,-300,900,-1,7.1,-.4}},{{6500,700,500,.2,8.4,1.1}}}}) {
            const std::array<const char *,6> names{{"X","Y","Z","VX","VY","VZ"}};
            for(int i=0;i<6;++i)sc->SetRealParameter(names[i],s[i]);
            for(double mass:{700.,1300.}) {
               sc->SetRealParameter("DryMass",mass);const double actualMass=sc->GetRealParameter("TotalMass");
               const auto *native=sc->GetState().GetState();const std::array<double,6> before{{native[0],native[1],native[2],native[3],native[4],native[5]}};const auto epoch=sc->GetEpochGT();
               V r{{before[0],before[1],before[2]}},v{{before[3],before[4],before[5]}},a{};double norm=0;for(auto x:r)norm+=x*x;norm=std::sqrt(norm);
               for(int i=0;i<3;++i)a[i]=-mu*r[i]/(norm*norm*norm);
               a[0]+=2e-6*v[0]+.002/actualMass;a[1]+=-3e-6*v[1];a[2]+=4e-6*v[2]+.0007;
               const auto observed=query();for(int i=0;i<3;++i)require(std::abs(observed[i]-a[i])<1e-13,"Accepted-state acceleration differs from independent gravity plus generic force");
               require(force->last==sc,"Force query retained a detached spacecraft identity");
               axes.calculate(A1Mjd(sc->GetEpoch()));const auto D=axes.GetLastRotationDotMatrix();constexpr double h=.02;const auto mm=basis(r,v,a,-2*h),m1=basis(r,v,a,-h),p1=basis(r,v,a,h),pp=basis(r,v,a,2*h);
               for(int i=0;i<3;++i)for(int j=0;j<3;++j)require(std::abs(D(i,j)-(mm[i][j]-8*m1[i][j]+8*p1[i][j]-pp[i][j])/(12*h))<5e-11,"Live noncircular/generic-force frame derivative differs from geometric oracle");
               require(std::memcmp(before.data(),sc->GetState().GetState(),6*sizeof(double))==0&&sc->GetEpochGT()==epoch,"Acceleration query changed accepted state or precise epoch");++checked;
            }
         }
         const auto baseline=query();const auto savedR=axes.GetLastRotationMatrix(),savedD=axes.GetLastRotationDotMatrix();
         for(int mode:{1,2,3,4}) {
            force->mode=mode;Rvector3 output(17,18,19);bool threw=false;
            try{ODEModel::GetBoundRelativeAcceleration(sc,earth,A1Mjd(sc->GetEpoch()),output);}catch(BaseException &){threw=true;}
            require(threw&&output==Rvector3(17,18,19),"Unavailable/erroring/recursive/nonfinite force was accepted or mutated output");
            bool frameThrew=false;try{axes.calculate(A1Mjd(sc->GetEpoch()));}catch(BaseException &){frameThrew=true;}
            require(frameThrew&&axes.GetLastRotationMatrix()==savedR&&axes.GetLastRotationDotMatrix()==savedD,"Failed acceleration query changed the last valid frame");
            force->mode=0;require(query()==baseline,"Acceleration query failed to recover");++rejected;
         }
         Rvector3 unchanged(3,4,5);bool epochRejected=false;try{ODEModel::GetBoundRelativeAcceleration(sc,earth,A1Mjd(sc->GetEpoch()+1./86400),unchanged);}catch(BaseException &){epochRejected=true;}
         require(epochRejected&&unchanged==Rvector3(3,4,5),"Wrong-epoch acceleration query succeeded or changed output");
         std::cout<<"PASS live acceleration context: "<<checked<<" noncircular state/mass oracles, 54 geometric derivative components, "<<rejected<<" force error/recursion/availability paths, precise-epoch rejection and unchanged accepted state.\n";
      } else {
         const auto baseline=query();
         auto rejectsWithoutOutput=[&] {
            Rvector3 output(3,4,5);bool rejected=false;
            try{ODEModel::GetBoundRelativeAcceleration(sc,earth,A1Mjd(sc->GetEpoch()),output);}catch(BaseException &){rejected=true;}
            require(rejected&&output==Rvector3(3,4,5),"Mismatched state origin or axes was accepted or mutated output");
         };
         auto *body=sc->GetJ2000Body();sc->SetJ2000Body(nullptr);rejectsWithoutOutput();sc->SetJ2000Body(body);
         require(query()==baseline,"State-origin guard failed to recover");
         auto *internal=sc->GetInternalCoordSystem();
         {CoordinateSystem alternate(*internal);MJ2000EcAxes ecliptic("AcceptanceEcliptic");
          require(alternate.SetRefObject(&ecliptic,Gmat::AXIS_SYSTEM,ecliptic.GetName()),"Alternate axes setup failed");
          sc->SetInternalCoordSystem(&alternate);rejectsWithoutOutput();sc->SetInternalCoordSystem(internal);}
         require(query()==baseline,"State-axes guard failed to recover");
         {PropagationStateManager transientStorage;model->SetPropStateManager(&transientStorage);require(query()==baseline,"State-manager rebinding changed the live force provider");model->SetPropStateManager(setup->GetPropStateManager());require(query()==baseline,"State-manager restoration changed physical acceleration");}
         {Spacecraft copy(*sc);Rvector3 value(3,4,5);require(!ODEModel::GetBoundRelativeAcceleration(&copy,earth,A1Mjd(copy.GetEpoch()),value)&&value==Rvector3(3,4,5),"Spacecraft copy inherited live dynamics context");
          model->BindAccelerationContext(&copy,true);bool rejected=false;try{ODEModel::GetBoundRelativeAcceleration(&copy,earth,A1Mjd(copy.GetEpoch()),value);}catch(BaseException &){rejected=true;}require(rejected,"Detached spacecraft identity was accepted by a live model");}
         auto temporary=std::make_unique<ODEModel>(*model);*temporary=*model;temporary->BindAccelerationContext(sc,true);
         {Rvector3 untouched(3,4,5);bool inactive=false;try{ODEModel::GetBoundRelativeAcceleration(sc,earth,A1Mjd(sc->GetEpoch()),untouched);}catch(BaseException &){inactive=true;}require(inactive&&untouched==Rvector3(3,4,5),"Copied/assigned model inherited an initialized live context");}
         temporary.reset();
         Rvector3 value(3,4,5);bool retired=false;try{ODEModel::GetBoundRelativeAcceleration(sc,earth,A1Mjd(sc->GetEpoch()),value);}catch(BaseException &){retired=true;}
         require(retired&&value==Rvector3(3,4,5),"Destroyed force provider was reused or silently became zero");
         model->ActivateAccelerationContext();require(query()==baseline,"Explicit live-provider recovery changed physical acceleration");
         ODEModel::SuspendAccelerationContext(sc);bool suspended=false;try{query();}catch(BaseException &){suspended=true;}require(suspended,"Non-numerical handoff retained a stale force provider");model->ActivateAccelerationContext();require(query()==baseline,"Numerical-provider recovery after suspension failed");
         std::cout<<"PASS acceleration context identity/lifetime: no inherited clone binding, detached identity rejected, dead provider rejected without output mutation, explicit provider recovery.\n";
      }
      m->Finalize();return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<'\n';}catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<'\n';}return 1;
}
