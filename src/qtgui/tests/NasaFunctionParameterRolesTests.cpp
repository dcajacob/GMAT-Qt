#include "FormationAcceptance.hpp"
#include "FactoryManager.hpp"
#include "Parameter.hpp"
#include "GmatType.hpp"
#include <memory>
using namespace formaccept;

int main(int argc,char **argv) {
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("FunctionParameterRoles");
 try {
  require(argc==2&&acceptanceWidgetPlatform(),"Supply native parameter-role startup");
  Rig rig(QFileInfo(argv[1]).absoluteFilePath());require(rig.window.buildScript(),"Parameter-role baseline Build failed");
  auto *moderator=Moderator::Instance();const auto source=rig.source->toPlainText();const int history=rig.source->document()->availableUndoSteps();const auto configured=moderator->GetListOfObjects(Gmat::UNKNOWN_OBJECT);
  const auto types=moderator->GetListOfFactoryItems(Gmat::PARAMETER);QJsonArray rows;QStringList user,system,failed;
  for(const auto &type:types) {
   std::unique_ptr<Parameter> parameter(FactoryManager::Instance()->CreateParameter(type));
   if(!parameter){failed.append(QString::fromStdString(type));continue;}
   const auto name=QString::fromStdString(type);(parameter->IsSystemParameter()?system:user).append(name);
   rows.append(QJsonObject{{"factory_type",name},{"native_type",QString::fromStdString(parameter->GetTypeName())},{"parameter_class",QString::fromStdString(parameter->GetParameterClassType())},{"is_system_parameter",parameter->IsSystemParameter()},{"key",int(parameter->GetKey())},{"owner_type_id",int(parameter->GetOwnerType())},{"owner_type_name",QString::fromStdString(GmatType::GetTypeName(parameter->GetOwnerType()))},{"return_type",int(parameter->GetReturnType())},{"cycle_type",int(parameter->GetCycleType())},{"coordinate_system_dependent",parameter->IsCoordSysDependent()},{"origin_dependent",parameter->IsOriginDependent()},{"owned_object_dependent",parameter->IsOwnedObjectDependent()},{"needs_coordinate_system",parameter->NeedCoordSystem()},{"requires_body_fixed_frame",parameter->RequiresBodyFixedCS()},{"requires_celestial_body_origin",parameter->RequiresCelestialBodyCSOrigin()},{"needs_forces",parameter->NeedsForces()},{"needs_external_clone",parameter->NeedExternalClone()},{"settable",parameter->IsSettable()},{"reportable",parameter->IsReportable()},{"plottable",parameter->IsPlottable()},{"default_global",parameter->IsGlobal()}});
  }
  if(!failed.isEmpty())rig.save("failed-parameter-role.json",QJsonDocument(QJsonObject{{"factory_types",QJsonArray::fromStringList(failed)},{"completed_rows",rows}}).toJson());
  require(failed.isEmpty(),qPrintable("Factory parameter metadata construction failed: "+failed.join(",")));
  user.sort();system.sort();require(rows.size()==int(types.size())&&user==QStringList({"Array","String","Variable"}),"User/system parameter role partition changed");
  require(rig.source->toPlainText()==source&&rig.source->document()->availableUndoSteps()==history&&configured==moderator->GetListOfObjects(Gmat::UNKNOWN_OBJECT),"Unmanaged parameter metadata altered GUI configuration/history");acceptanceView(rig.window);
  const QJsonObject proof{{"checkpoint","function-resource-parameter-role-census"},{"outcome","passed"},{"factory_parameter_count",rows.size()},{"system_parameter_count",system.size()},{"user_parameter_count",user.size()},{"user_parameters",QJsonArray::fromStringList(user)},{"system_parameters",QJsonArray::fromStringList(system)},{"rows",rows},{"configured_source_history_unchanged",true},{"scope","Unmanaged factory constructors and native metadata only. No evaluation,owner/reference binding,named main/function creation,use,global eligibility,numerical science or exact requirement qualification."}};
  rig.save("native-parameter-roles.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
