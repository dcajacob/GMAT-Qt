#include "ObjectReferencedAxes.hpp"
#include "SpacePoint.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
using Vector=std::array<double,3>;
using Matrix=std::array<Vector,3>;
static void require(bool value,const char*text){if(!value)throw std::runtime_error(text);}
static Vector add(Vector a,const Vector&b){for(int i=0;i<3;++i)a[i]+=b[i];return a;}
static Vector subtract(Vector a,const Vector&b){for(int i=0;i<3;++i)a[i]-=b[i];return a;}
static Vector scale(Vector a,double s){for(auto&v:a)v*=s;return a;}
static Vector cross(const Vector&a,const Vector&b){return{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
static Vector unit(Vector a){double n=0;for(double v:a)n+=v*v;n=std::sqrt(n);require(n>1e-8,"Analytic fixture became singular");return scale(a,1/n);}
struct Motion{Vector position,velocity,acceleration;};
static Vector position(const Motion&m,double t){return add(add(m.position,scale(m.velocity,t)),scale(m.acceleration,.5*t*t));}
static Vector velocity(const Motion&m,double t){return add(m.velocity,scale(m.acceleration,t));}
class AnalyticPoint:public SpacePoint{
public:
 Motion motion;
 bool failAcceleration = false;
 AnalyticPoint(const std::string&name,Motion m):SpacePoint(Gmat::SPACE_POINT,"AnalyticPoint",name),motion(m){}
 GmatBase*Clone()const override{return new AnalyticPoint(*this);}
 bool HasLocalClones()override{return false;}
 bool RenameRefObject(const UnsignedInt,const std::string&,const std::string&)override{return false;}
 const Rvector6 GetMJ2000State(const A1Mjd&epoch)override{const double t=(epoch.Get()-21545)*86400;const auto r=position(motion,t),v=velocity(motion,t);return Rvector6(r[0],r[1],r[2],v[0],v[1],v[2]);}
 const Rvector6 GetMJ2000State(const GmatTime&epoch)override{return GetMJ2000State(A1Mjd(epoch.GetMjd()));}
 const Rvector3 GetMJ2000Position(const A1Mjd&epoch)override{auto r=position(motion,(epoch.Get()-21545)*86400);return Rvector3(r[0],r[1],r[2]);}
 const Rvector3 GetMJ2000Position(const GmatTime&epoch)override{return GetMJ2000Position(A1Mjd(epoch.GetMjd()));}
 const Rvector3 GetMJ2000Velocity(const A1Mjd&epoch)override{auto v=velocity(motion,(epoch.Get()-21545)*86400);return Rvector3(v[0],v[1],v[2]);}
 const Rvector3 GetMJ2000Velocity(const GmatTime&epoch)override{return GetMJ2000Velocity(A1Mjd(epoch.GetMjd()));}
 const Rvector3 GetMJ2000Acceleration(const A1Mjd&)override{if(failAcceleration)throw std::runtime_error("intentional analytic acceleration exception");return Rvector3(motion.acceleration[0],motion.acceleration[1],motion.acceleration[2]);}
 const Rvector3 GetMJ2000Acceleration(const GmatTime&)override{return Rvector3(motion.acceleration[0],motion.acceleration[1],motion.acceleration[2]);}
};
class AxesProbe:public ObjectReferencedAxes{
public:
 AxesProbe():ObjectReferencedAxes("AccelerationProbe"){}
 void calculate(const A1Mjd&epoch){CalculateRotationMatrix(epoch,true);}
};
// Position-only geometric construction. The derivative oracle differences these
// bases on the prescribed quadratic trajectories; no native rotation or native
// analytic frame-derivative routine is part of the oracle.
static Matrix basis(const Motion&relative,double t,int first,int normal,int signFirst,int signNormal,bool alongVelocity){
 std::array<Vector,3> axes{};const auto r=position(relative,t),v=velocity(relative,t);axes[first]=scale(unit(alongVelocity?v:r),signFirst);axes[normal]=scale(unit(cross(r,v)),signNormal);const int missing=3-first-normal;
 if(missing==0)axes[0]=cross(axes[1],axes[2]);else if(missing==1)axes[1]=cross(axes[2],axes[0]);else axes[2]=cross(axes[0],axes[1]);
 Matrix m{};for(int i=0;i<3;++i)for(int j=0;j<3;++j)m[i][j]=axes[j][i];return m;
}
static Matrix derivative(const Motion&m,double t,int first,int normal,int sf,int sn,bool velocity){
 constexpr double h=.02;const auto mm=basis(m,t-2*h,first,normal,sf,sn,velocity),m1=basis(m,t-h,first,normal,sf,sn,velocity),p1=basis(m,t+h,first,normal,sf,sn,velocity),pp=basis(m,t+2*h,first,normal,sf,sn,velocity);Matrix d{};for(int i=0;i<3;++i)for(int j=0;j<3;++j)d[i][j]=(mm[i][j]-8*m1[i][j]+8*p1[i][j]-pp[i][j])/(12*h);return d;
}
static QJsonArray json(const Matrix&m){QJsonArray a;for(auto&r:m){QJsonArray row;for(double v:r)row.append(v);a.append(row);}return a;}
static QJsonArray bits(const Rmatrix33&m){QJsonArray a;for(int i=0;i<3;++i)for(int j=0;j<3;++j){const double value=m(i,j);std::uint64_t raw;std::memcpy(&raw,&value,sizeof(raw));a.append(QString::number(raw,16));}return a;}
int main(int argc,char**argv){try{
 require(argc==2,"Supply native derivative contract startup");FileManager::Instance()->ReadStartupFile(argv[1]);
 const std::array<Motion,3> motions{{{{7000,0,0},{0,7.5,0},{0,0,0}},{{7000,0,0},{0,7.5,0},{-.008,0,0}},{{7000,300,200},{-1,7,.5},{.004,-.006,.007}}}};
 const Motion translated{{100,-200,50},{.03,-.01,.02},{.0001,-.0002,.0003}};QJsonArray records;bool passed=true;double maximumRotation=0,maximumDerivative=0;int wrongDerivatives=0,zeroAccelerationProbes=0,nonzeroAccelerationProbes=0;
 for(int motion=0;motion<3;++motion)for(bool commonTranslation:{false,true})for(double requested:{-60.,0.,120.})for(bool alongVelocity:{false,true})for(int first=0;first<3;++first)for(int normal=0;normal<3;++normal)if(first!=normal)for(int sf:{-1,1})for(int sn:{-1,1}){
  Motion primary=commonTranslation?translated:Motion{{0,0,0},{0,0,0},{0,0,0}},secondary{add(primary.position,motions[motion].position),add(primary.velocity,motions[motion].velocity),add(primary.acceleration,motions[motion].acceleration)};AnalyticPoint anchor("Primary",primary),target("Secondary",secondary);AxesProbe axes;axes.SetPrimaryObject(&anchor);axes.SetSecondaryObject(&target);axes.ResetAxes();std::array<std::string,3> names{};names[first]=std::string(sf<0?"-":"")+(alongVelocity?"V":"R");names[normal]=std::string(sn<0?"-":"")+"N";axes.SetXAxis(names[0]);axes.SetYAxis(names[1]);axes.SetZAxis(names[2]);const A1Mjd epoch(21545+requested/86400);const double actual=(epoch.Get()-21545)*86400;axes.calculate(epoch);const auto R=axes.GetLastRotationMatrix(),D=axes.GetLastRotationDotMatrix();const auto expected=basis(motions[motion],actual,first,normal,sf,sn,alongVelocity),rate=derivative(motions[motion],actual,first,normal,sf,sn,alongVelocity);Matrix observed{},observedRate{};double rotationError=0,derivativeError=0;
  for(int i=0;i<3;++i)for(int j=0;j<3;++j){observed[i][j]=R(i,j);observedRate[i][j]=D(i,j);require(std::isfinite(R(i,j))&&std::isfinite(D(i,j)),"Native derivative returned a nonfinite value");rotationError=std::max(rotationError,std::abs(R(i,j)-expected[i][j]));derivativeError=std::max(derivativeError,std::abs(D(i,j)-rate[i][j]));}
  anchor.failAcceleration=true;target.failAcceleration=true;
  const auto orientation=axes.GetRotationMatrixOnly(epoch);
  const auto preciseOrientation=axes.GetRotationMatrixOnly(GmatTime(epoch.Get()));
  anchor.failAcceleration=false;target.failAcceleration=false;
  for(int i=0;i<3;++i)for(int j=0;j<3;++j)require(std::abs(orientation(i,j)-expected[i][j])<3e-13&&std::abs(preciseOrientation(i,j)-expected[i][j])<3e-13,"Geometry-only orientation differs from independent position/velocity basis");
  require(axes.GetLastRotationMatrix()==R&&axes.GetLastRotationDotMatrix()==D,"Geometry-only orientation changed cached frame or frame rate");
  maximumRotation=std::max(maximumRotation,rotationError);maximumDerivative=std::max(maximumDerivative,derivativeError);if(derivativeError>=5e-11)++wrongDerivatives;passed=passed&&rotationError<3e-13&&derivativeError<5e-11;if(motion==0)++zeroAccelerationProbes;else++nonzeroAccelerationProbes;
  records.append(QJsonObject{{"motion",motion},{"common_translation",commonTranslation},{"requested_elapsed",requested},{"actual_elapsed",actual},{"axes",QJsonArray{QString::fromStdString(names[0]),QString::fromStdString(names[1]),QString::fromStdString(names[2])}},{"rotation",json(observed)},{"expected_rotation",json(expected)},{"derivative",json(observedRate)},{"expected_derivative",json(rate)},{"rotation_error",rotationError},{"derivative_error",derivativeError},{"rotation_bits",bits(R)},{"derivative_bits",bits(D)}});
 }
 require(records.size()==864&&zeroAccelerationProbes==288&&nonzeroAccelerationProbes==576,"Native derivative probe matrix incomplete");
 int invalidAccelerations=0,providerExceptions=0;
 AnalyticPoint primary("InvalidPrimary",Motion{{0,0,0},{0,0,0},{0,0,0}}),secondary("InvalidSecondary",motions[2]);AxesProbe recovery;recovery.SetPrimaryObject(&primary);recovery.SetSecondaryObject(&secondary);recovery.ResetAxes();recovery.SetXAxis("V");recovery.SetYAxis("N");recovery.calculate(A1Mjd(21545));const auto savedR=recovery.GetLastRotationMatrix(),savedD=recovery.GetLastRotationDotMatrix();
 for(auto*point:{&primary,&secondary})for(int component=0;component<3;++component)for(double invalid:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()}){
  const auto before=point->motion.acceleration;point->motion.acceleration[component]=invalid;bool rejected=false;try{recovery.calculate(A1Mjd(21545));}catch(BaseException&e){rejected=e.GetFullMessage().find("acceleration")!=std::string::npos;}point->motion.acceleration=before;require(rejected&&recovery.GetLastRotationMatrix()==savedR&&recovery.GetLastRotationDotMatrix()==savedD,"Nonfinite acceleration was accepted or changed the last valid frame");recovery.calculate(A1Mjd(21545));require(recovery.GetLastRotationMatrix()==savedR&&recovery.GetLastRotationDotMatrix()==savedD,"Frame did not recover after invalid acceleration");++invalidAccelerations;
 }
 secondary.failAcceleration=true;bool threw=false;try{recovery.calculate(A1Mjd(21545));}catch(const std::runtime_error&e){threw=std::string(e.what())=="intentional analytic acceleration exception";}secondary.failAcceleration=false;require(threw&&recovery.GetLastRotationMatrix()==savedR&&recovery.GetLastRotationDotMatrix()==savedD,"Acceleration provider exception was swallowed or changed the last valid frame");recovery.calculate(A1Mjd(21545));require(recovery.GetLastRotationMatrix()==savedR&&recovery.GetLastRotationDotMatrix()==savedD,"Frame did not recover after acceleration provider exception");++providerExceptions;
 require(invalidAccelerations==18&&providerExceptions==1,"Acceleration rejection/exception matrix incomplete");
 const QJsonObject summary{{"checkpoint","object-referenced-acceleration-contract"},{"outcome",passed?"passed":"failed"},{"probes",records.size()},{"zero_acceleration_probes",zeroAccelerationProbes},{"nonzero_acceleration_probes",nonzeroAccelerationProbes},{"axis_definitions",48},{"invalid_accelerations_rejected",invalidAccelerations},{"provider_exceptions_propagated",providerExceptions},{"incorrect_derivatives",wrongDerivatives},{"maximum_rotation_error",maximumRotation},{"maximum_derivative_error",maximumDerivative},{"rotation_bound",3e-13},{"derivative_bound",5e-11},{"scope","Prescribed native SpacePoint acceleration and position-only five-point geometric derivative oracle;48signed R/V/N axis permutations,linear/central-instant/noncentral quadratic motion,three epochs,common accelerating translation. Not spacecraft EOM acceleration delivery,GUI,arbitrary ephemeris/singular or whole-domain qualification."}};
 const auto dir=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(!dir.isEmpty()){QFile file(dir+"/derivative-probes.json");require(file.open(QIODevice::WriteOnly),"Cannot preserve derivative probes");const auto bytes=QJsonDocument(QJsonObject{{"summary",summary},{"records",records}}).toJson();require(file.write(bytes)==bytes.size(),"Incomplete derivative probe evidence");}
 std::cout<<"NASA_CHECK "<<QJsonDocument(summary).toJson(QJsonDocument::Compact).constData()<<std::endl;require(passed,"Object-reference derivative discarded supplied acceleration");return 0;
 }catch(BaseException&e){std::cerr<<e.GetFullMessage()<<std::endl;}catch(std::exception&e){std::cerr<<e.what()<<std::endl;}return 1;}
