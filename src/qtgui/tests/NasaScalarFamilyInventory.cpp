// Read-only factory/type inventory. This is discovery, never acceptance credit.
#include "Moderator.hpp"
#include "FactoryManager.hpp"
#include "ParameterInfo.hpp"
#include "Parameter.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>
#include <memory>
#include <stdexcept>
int main(int argc,char **argv) {
 QCoreApplication app(argc,argv);
 try {
  if(argc!=3)throw std::runtime_error("Supply startup and inventory output");
  auto *moderator=Moderator::Instance();
  if(!moderator->Initialize(argv[1]))throw std::runtime_error("Inventory initialization failed");
  const auto types=moderator->GetListOfFactoryItems(Gmat::PARAMETER);
  auto *info=ParameterInfo::Instance();QJsonArray rows;
  for(const auto &type:types) {
   QJsonObject row{{"type",QString::fromStdString(type)}};
   try {
    std::unique_ptr<Parameter> parameter(FactoryManager::Instance()->CreateParameter(type,"Inventory."+type));
    if(!parameter)throw std::runtime_error("Factory returned null");
    row.insert("native_type",QString::fromStdString(parameter->GetTypeName()));
    row.insert("return_type",int(parameter->GetReturnType()));
    row.insert("scalar",parameter->GetReturnType()==Gmat::REAL_TYPE);
    row.insert("owner",QString::fromStdString(GmatBase::GetObjectTypeString(parameter->GetOwnerType())));
    row.insert("owned_object",QString::fromStdString(GmatBase::GetObjectTypeString(info->GetOwnedObjectType(type))));
    row.insert("dependency",int(info->GetDepObjectType(type)));
    row.insert("dependency_name",QString::fromStdString(Parameter::GetDependentTypeString(info->GetDepObjectType(type))));
    row.insert("plottable",parameter->IsPlottable());row.insert("reportable",parameter->IsReportable());row.insert("settable",parameter->IsSettable());
    row.insert("body_fixed_required",info->RequiresBodyFixedCS(type));row.insert("body_origin_required",info->RequiresCelestialBodyCSOrigin(type));
    row.insert("attached",info->IsForAttachedObject(type));row.insert("owned",info->IsForOwnedObject(type));
   } catch(BaseException &e) {row.insert("error",QString::fromStdString(e.GetFullMessage()));}
     catch(std::exception &e) {row.insert("error",e.what());}
   rows.append(row);
  }
  QFile out(argv[2]);if(!out.open(QIODevice::WriteOnly))throw std::runtime_error("Inventory output unavailable");
  out.write(QJsonDocument(QJsonObject{{"kind","native-factory-discovery-only"},{"rows",rows},{"NASA_credit",0}}).toJson());
  std::cout<<"SCALAR_FAMILY_INVENTORY "<<rows.size()<<" types; no acceptance credit\n";
 } catch(BaseException &e) {std::cerr<<e.GetFullMessage()<<'\n';return 1;}
   catch(std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
 return 0;
}
