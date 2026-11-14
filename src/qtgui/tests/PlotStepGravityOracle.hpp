#pragma once
#include "FormationAcceptance.hpp"
#include "GravityField.hpp"
#include "ODEModel.hpp"
#include <QCryptographicHash>

namespace plotgravity {
inline QString configure(QString text) {
 text.replace("FM.PrimaryBodies = {};", "FM.PrimaryBodies = {Earth};");
 text.replace("FM.PointMasses = {Earth};", "FM.PointMasses = {};\nFM.GravityField.Earth.PotentialFile = 'JGM2.cof';\nFM.GravityField.Earth.Degree = 0;\nFM.GravityField.Earth.Order = 0;\nFM.GravityField.Earth.TideModel = 'None';");
 return text;
}
inline QJsonArray runtime() {
 using namespace formaccept;
 auto *command=Moderator::Instance()->GetFirstCommand();
 while(command&&!command->IsOfType("Propagate"))command=command->GmatCommand::GetNext();
 QJsonArray records;
 for(int i=0;i<2;++i) {
  auto *setup=command?dynamic_cast<PropSetup *>(command->GetClone(i)):nullptr;
  auto *model=setup?setup->GetODEModel():nullptr;
  require(model&&model->GetNumForces()==1,"Sampled gravity provider includes unintended contributors");
  auto *gravity=dynamic_cast<GravityField *>(model->GetForce(0));
  require(gravity&&gravity->GetTypeName()=="GravityField"&&gravity->GetBodyName()=="Earth"&&gravity->GetIntegerParameter("Degree")==0&&gravity->GetIntegerParameter("Order")==0,"Sampled gravity runtime force/body/degree/order differs");
  std::string body;double nativeMu=0;
  require(gravity->GetBodyAndMu(body,nativeMu)&&body=="Earth"&&std::abs(nativeMu-mu)<1e-9,"Sampled gravity Mu differs from independent circular oracle");
  const auto file=QFileInfo(QString::fromStdString(gravity->GetStringParameter("PotentialFileFullPath"))).canonicalFilePath();
  require(file==QFileInfo("../data/gravity/earth/JGM2.cof").canonicalFilePath()&&!file.isEmpty(),"Sampled gravity selected a different potential");
  records.append(QJsonObject{{"provider",QString::fromStdString(setup->GetName())},{"force_type",QString::fromStdString(gravity->GetTypeName())},{"degree",0},{"order",0},{"mu",nativeMu},{"potential",file},{"potential_sha256",QString::fromLatin1(QCryptographicHash::hash(read(file),QCryptographicHash::Sha256).toHex())}});
 }
 return records;
}
}
