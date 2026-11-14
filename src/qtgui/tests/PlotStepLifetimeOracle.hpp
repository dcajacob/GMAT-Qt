#pragma once
#include "FormationAcceptance.hpp"
#include "QtPlotReceiver.hpp"

namespace plotlifetime {
inline QString reportCommand() {
 return "Report Values One.ElapsedSecs One.EarthMJ2000Eq.X One.EarthMJ2000Eq.Y One.EarthMJ2000Eq.Z One.EarthMJ2000Eq.VX One.EarthMJ2000Eq.VY One.EarthMJ2000Eq.VZ;\n";
}

// Check both runs of the same saved script against the literal expected arcs.
inline QJsonObject inspect(formaccept::Rig &rig, const QString &name, const QVector<double> &epochs) {
 const bool gap=name=="LifetimeToggleGap";
 const double stop=gap?57:name=="LifetimeFinishOff"?30:37;
 const QStringList fields=QString::fromUtf8(formaccept::read(rig.report)).trimmed().split(',');
 formaccept::require(fields.size()==7,"Lifetime final report schema differs");
 const auto state=formaccept::circular(formaccept::orbits[0],stop);
 QJsonArray report,plots;
 for(int i=0;i<fields.size();++i) {
  bool valid=false;
  const double value=fields[i].toDouble(&valid);
  const double expected=i==0?stop:state[i-1];
  formaccept::require(valid&&std::abs(value-expected)<(i==0?1e-4:i<=3?4e-5:5e-8),"Lifetime propagation report differs from analytic orbit");
  report.append(value);
 }
 for(const QString &nameOfPlot:QStringList{"Orb","Map"}) {
  const auto model=rig.window.plotReceiver()->model(nameOfPlot);
  formaccept::require(model&&model->endOfRun,"Lifetime completed viewer missing");
  QJsonArray points;
  for(const auto &curve:model->curves) {
   if(curve.name!="One")continue;
   for(const auto &point:curve.points)points.append(QJsonObject{{"epoch",(point.epoch-21545)*86400},{"connect",point.connect},{"x",point.x},{"y",point.y},{"z",point.z}});
  }
  plots.append(QJsonObject{{"name",nameOfPlot},{"points",points},{"end_of_run",model->endOfRun},{"active",model->active}});
  rig.save(name+"-latest-plots.json",QJsonDocument(plots).toJson());
  formaccept::require(points.size()==epochs.size(),"Lifetime retained count differs");
  for(int i=0;i<epochs.size();++i) {
   const auto p=points[i].toObject();
   formaccept::require(std::abs(p["epoch"].toDouble()-epochs[i])<1e-4,"Lifetime retained epoch differs");
   formaccept::require(p["connect"].toBool()==(i>0&&!(gap&&epochs[i]==30)),"Lifetime segment crosses disabled interval");
  }
  if(nameOfPlot=="Map"&&name=="LifetimeFinishOff")formaccept::require(!model->active,"Completion reactivated GroundTrack");
 }
 return QJsonObject{{"plots",plots},{"report",report}};
}
}
