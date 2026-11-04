#pragma once
#include "FormationAcceptance.hpp"
#include "CoordinateSystem.hpp"
#include "PlotStepRotatingOracle.hpp"

namespace plotlac {
using Vec=std::array<double,3>;
inline Vec position(int object,double t,bool nested) {
 const auto reference=formaccept::circular(formaccept::orbits[1],t);
 const auto input=formaccept::circular(formaccept::orbits[object],t);
 const Vec x=plotrotating::unit({reference[0],reference[1],reference[2]});
 Vec constraint{0,0,1};
 if(nested) {
  const double rate=std::sqrt(formaccept::mu/std::pow(7000.,3)),angle=rate*t;
  constraint={-std::sin(angle),std::cos(angle),0};
 }
 // Local X points toward Two. Local Z lies toward the prescribed constraint
 // after projecting off X. The remaining axis completes a right-handed basis.
 Vec y=plotrotating::unit(plotrotating::cross(x,constraint));
 for(auto &v:y)v=-v;
 const Vec z=plotrotating::cross(x,y);
 Vec result{};
 for(int i=0;i<3;++i){result[0]+=x[i]*input[i];result[1]+=y[i]*input[i];result[2]+=z[i]*input[i];}
 return result;
}
inline std::array<double,6> state(int object,double t,bool nested) {
 const double h=.1;
 const auto p=position(object,t,nested),a=position(object,t-2*h,nested),b=position(object,t-h,nested),c=position(object,t+h,nested),d=position(object,t+2*h,nested);
 std::array<double,6> result{};
 for(int i=0;i<3;++i){result[i]=p[i];result[i+3]=(a[i]-8*b[i]+8*c[i]-d[i])/(12*h);}
 return result;
}
inline QString configure(QString setup,bool nested) {
 QString frame;
 if(nested)frame="Create CoordinateSystem ConstraintFrame;\nConstraintFrame.Origin = Earth;\nConstraintFrame.Axes = ObjectReferenced;\nConstraintFrame.Primary = Earth;\nConstraintFrame.Secondary = One;\nConstraintFrame.XAxis = R;\nConstraintFrame.ZAxis = N;\n";
 frame+="Create CoordinateSystem LacFrame;\nLacFrame.Origin = Earth;\nLacFrame.Axes = LocalAlignedConstrained;\nLacFrame.ReferenceObject = Two;\nLacFrame.AlignmentVectorX = 1;\nLacFrame.AlignmentVectorY = 0;\nLacFrame.AlignmentVectorZ = 0;\nLacFrame.ConstraintVectorX = 0;\nLacFrame.ConstraintVectorY = 0;\nLacFrame.ConstraintVectorZ = 1;\nLacFrame.ConstraintCoordinateSystem = "+QString(nested?"ConstraintFrame":"EarthMJ2000Eq")+";\nLacFrame.ConstraintReferenceVectorX = 0;\nLacFrame.ConstraintReferenceVectorY = "+QString(nested?"1":"0")+";\nLacFrame.ConstraintReferenceVectorZ = "+QString(nested?"0":"1")+";\nOrb.CoordinateSystem = LacFrame;";
 setup.replace("Orb.CoordinateSystem = EarthMJ2000Eq;",frame);
 return setup;
}
inline QJsonObject capture(formaccept::Rig &rig,bool nested) {
 auto *moderator=Moderator::Instance();
 auto *one=dynamic_cast<Spacecraft*>(moderator->GetInternalObject("One"));
 auto *two=dynamic_cast<Spacecraft*>(moderator->GetInternalObject("Two"));
 auto *frame=dynamic_cast<CoordinateSystem*>(moderator->GetInternalObject("LacFrame"));
 formaccept::require(one&&two&&frame&&frame->GetReferenceObject()==two,"LAC runtime ReferenceObject identity differs");
 if(nested) {
  auto *constraint=dynamic_cast<CoordinateSystem*>(moderator->GetInternalObject("ConstraintFrame"));
  formaccept::require(constraint&&constraint->GetSecondaryObject()==one,"LAC nested runtime spacecraft identity differs");
 }
 QJsonArray outputs,owners;
 for(auto *sc:{one,two}) {
  QJsonArray state;for(int i=0;i<6;++i)state.append(sc->GetState()[i]);
  owners.append(QJsonObject{{"name",QString::fromStdString(sc->GetName())},{"epoch",sc->GetEpoch()},{"state",state}});
 }
 for(const auto &name:QStringList{"Orb","Map"}) {
  const auto model=rig.window.plotReceiver()->model(name);formaccept::require(bool(model),"LAC raw model missing");
  for(const auto &curve:model->curves) {
   QJsonArray points;
   for(const auto &p:curve.points)points.append(QJsonObject{{"elapsed",(p.epoch-21545)*86400},{"position",QJsonArray{p.x,p.y,p.z}},{"velocity",QJsonArray{p.viewVelocity[0],p.viewVelocity[1],p.viewVelocity[2]}},{"connect",p.connect}});
   outputs.append(QJsonObject{{"plot",name},{"object",curve.name},{"points",points}});
  }
 }
 return QJsonObject{{"reference_identity_checked",true},{"nested",nested},{"native_owners",owners},{"outputs",outputs}};
}
}
