#include "FormationAcceptance.hpp"
#include "FactoryManager.hpp"
#include <memory>
using namespace formaccept;

int main(int argc, char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("FunctionResourceMetadata");
   try
   {
      require(argc==3 && acceptanceWidgetPlatform(),"Supply metadata startup and remaining resource catalog");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath()); require(rig.window.buildScript(),"Resource metadata baseline Build failed");
      const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();
      const auto source=rig.source->toPlainText(); const int history=rig.source->document()->availableUndoSteps();
      const auto configured=Moderator::Instance()->GetListOfObjects(Gmat::UNKNOWN_OBJECT);
      QJsonArray rows,failed;
      for (const auto &entry:catalog["cases"].toArray())
      {
         const auto spec=entry.toObject(); const auto type=spec["type"].toString(); QJsonObject row{{"factory_type",type},{"category_id",spec["category_id"]}};
         try
         {
            std::unique_ptr<GmatBase> object(FactoryManager::Instance()->CreateObject(spec["category_id"].toInt(),type.toStdString(),"MetadataOnly"));
            require(bool(object),"Advertised resource factory returned null");
            row["native_type"]=QString::fromStdString(object->GetTypeName()); row["native_name"]=QString::fromStdString(object->GetName());
            row["automatic_global"]=object->IsAutomaticGlobal(); row["default_global"]=object->IsGlobal(); row["parameter_count"]=object->GetParameterCount();
            QJsonArray parameters;
            for (int id=0;id<object->GetParameterCount();++id)
            {
               QJsonObject field{{"id",id}};
               try
               {
                  field["name"]=QString::fromStdString(object->GetParameterText(id)); const auto kind=object->GetParameterType(id);
                  field["type_id"]=int(kind); field["type"]=QString::fromStdString(object->GetParameterTypeString(id));
                  field["read_only"]=object->IsParameterReadOnly(id); field["command_settable"]=object->IsParameterCommandModeSettable(id);
                  QJsonArray choices; for (const auto &v:object->GetPropertyEnumStrings(id)) choices.append(QString::fromStdString(v)); field["choices"]=choices;
                  if (kind==Gmat::REAL_TYPE) field["value"]=object->GetRealParameter(id);
                  else if (kind==Gmat::INTEGER_TYPE) field["value"]=object->GetIntegerParameter(id);
                  else if (kind==Gmat::BOOLEAN_TYPE) field["value"]=object->GetBooleanParameter(id);
                  else if (kind==Gmat::STRING_TYPE || kind==Gmat::ENUMERATION_TYPE || kind==Gmat::ON_OFF_TYPE || kind==Gmat::FILENAME_TYPE || kind==Gmat::OBJECT_TYPE) field["value"]=QString::fromStdString(kind==Gmat::ON_OFF_TYPE?object->GetOnOffParameter(id):object->GetStringParameter(id));
                  else if (kind==Gmat::STRINGARRAY_TYPE || kind==Gmat::OBJECTARRAY_TYPE) { QJsonArray values; for (const auto &v:object->GetStringArrayParameter(id)) values.append(QString::fromStdString(v)); field["value"]=values; }
               }
               catch (BaseException &e) { field["metadata_error"]=QString::fromStdString(e.GetFullMessage()); }
               parameters.append(field);
            }
            row["fields"]=parameters; row["constructed"]=true;
         }
         catch (BaseException &e) { row["construction_error"]=QString::fromStdString(e.GetFullMessage()); row["constructed"]=false; failed.append(type); }
         catch (std::exception &e) { row["construction_error"]=e.what(); row["constructed"]=false; failed.append(type); }
         rows.append(row); rig.save("metadata-progress.json",QJsonDocument(rows).toJson());
      }
      require(rows.size()==catalog["cases"].toArray().size() && rows.size()==45,"Resource metadata candidate count changed");
      require(rig.source->toPlainText()==source && rig.source->document()->availableUndoSteps()==history && configured==Moderator::Instance()->GetListOfObjects(Gmat::UNKNOWN_OBJECT),"Unmanaged resource metadata changed configuration/source/history");
      acceptanceView(rig.window);
      const QJsonObject proof{{"checkpoint","remaining-resource-native-metadata"},{"outcome","passed"},{"candidate_count",45},{"rows",rows},{"unconstructed",failed},{"configured_source_history_unchanged",true},{"scope","Complete constructor/default-field observations for 45 remaining candidates,including explicit errors. This is not named construction,use,eligibility,main/function/global parity,science or exact requirement qualification. Construction errors remain unresolved and are not eligibility exclusions."}};
      rig.save("native-resource-metadata.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
   }
   catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; }
   catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; }
   return 1;
}
