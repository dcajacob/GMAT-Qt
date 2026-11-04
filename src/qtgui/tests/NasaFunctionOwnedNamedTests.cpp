#include "FormationAcceptance.hpp"
#include "FactoryManager.hpp"
#include "ScriptInterpreter.hpp"
#include <QStatusBar>
#include <memory>
using namespace formaccept;

int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("FunctionOwnedNamed");
   try
   {
      require(argc==4 && acceptanceWidgetPlatform(),"Supply named-object probe startup,catalog and key");
      const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();
      QJsonObject spec; for (const auto &v:catalog["cases"].toArray()) if (v.toObject()["key"].toString()==argv[3]) spec=v.toObject();
      require(!spec.isEmpty() && catalog["cases"].toArray().size()==88,"Named probe candidate omitted");
      const auto type=spec["type"].toString(); Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      require(rig.window.buildScript(),"Named probe baseline Build failed");
      QJsonObject proof{{"checkpoint","owned-expression-main-named-probe"},{"outcome","passed"},{"case",spec["key"]},{"factory_type",type},{"category_id",spec["category_id"]},{"qualified",false}};
      auto persist=[&](){rig.save("progress.json",QJsonDocument(proof).toJson());};
      auto *moderator=Moderator::Instance();
      auto describe=[&](GmatBase *object) {
         QJsonObject row{{"present",object!=nullptr}};
         if (!object) return row;
         row["name"]=QString::fromStdString(object->GetName()); row["native_type"]=QString::fromStdString(object->GetTypeName());
         row["global"]=object->IsGlobal(); row["automatic_global"]=object->IsAutomaticGlobal(); row["local"]=object->IsLocal();
         row["parameter_count"]=object->GetParameterCount();
         QJsonArray fields;
         for (int id=0;id<object->GetParameterCount();++id)
         {
            QJsonObject field{{"id",id}};
            try { field["name"]=QString::fromStdString(object->GetParameterText(id)); field["type_id"]=int(object->GetParameterType(id)); field["type"]=QString::fromStdString(object->GetParameterTypeString(id)); field["read_only"]=object->IsParameterReadOnly(id); }
            catch (BaseException &e) { field["metadata_error"]=QString::fromStdString(e.GetFullMessage()); }
            fields.append(field);
         }
         row["fields"]=fields;
         try { row["generated"]=QString::fromStdString(object->GetGeneratingString(Gmat::NO_COMMENTS)); }
         catch (BaseException &e) { row["serialization_error"]=QString::fromStdString(e.GetFullMessage()); }
         return row;
      };
      try { std::unique_ptr<GmatBase> object(FactoryManager::Instance()->CreateObject(spec["category_id"].toInt(),type.toStdString(),"FactoryOnly")); proof["factory_constructor"]=describe(object.get()); }
      catch (BaseException &e) { proof["factory_constructor"]=QJsonObject{{"present",false},{"error",QString::fromStdString(e.GetFullMessage())}}; }
      persist();
      auto build=[&](const QString &phase,const QString &text) {
         rig.source->setPlainText(text); const int history=rig.source->document()->availableUndoSteps();
         rig.save(phase+".script",text.toUtf8()); rig.messages->clear();
         const bool built=rig.window.buildScript(); events();
         const auto messages=rig.messages->toPlainText(); rig.save(phase+".messages.txt",messages.toUtf8());
         QStringList errors; for (const auto &v:ScriptInterpreter::Instance()->GetErrorList()) errors.append(QString::fromStdString(v));
         require(rig.source->toPlainText()==text && rig.source->document()->availableUndoSteps()==history,"Named probe Build changed source/history");
         QJsonObject result{{"built",built},{"configured",describe(moderator->GetConfiguredObject("NamedWitness"))},{"errors",QJsonArray::fromStringList(errors)},{"status",rig.window.statusBar()->currentMessage()},{"source_history_unchanged",true}};
         proof[phase]=result; persist(); return built;
      };
      const QString declaration="% Named creation probe Ω\nCreate "+type+" NamedWitness;\nCreate Variable Keep;\nKeep = 41;\n";
      build("create",declaration+"BeginMissionSequence;\nKeep = Keep + 1;\n");
      const QString output="Create ReportFile Values;\nValues.Filename = '"+rig.report+"';\nValues.WriteHeaders = false;\nValues.Precision = 25;\n";
      const QString mission="BeginMissionSequence;\nKeep = Keep + 1;\n";
      auto run=[&](const QString &phase) {
         QFile::remove(rig.report); const auto text=rig.source->toPlainText(); const int history=rig.source->document()->availableUndoSteps(); rig.messages->clear();
         const auto status=rig.window.runMission(); events(); rig.save(phase+".messages.txt",rig.messages->toPlainText().toUtf8());
         require(rig.source->toPlainText()==text && rig.source->document()->availableUndoSteps()==history,"Named probe Run changed source/history");
         const auto bytes=QFile::exists(rig.report)?read(rig.report):QByteArray{}; rig.save(phase+".values.txt",bytes);
         const bool completed=status==MainWindow::RunResult::Completed;
         if (completed) { bool valid=false; const auto marker=QString::fromUtf8(bytes).trimmed().toDouble(&valid); require(valid && marker==42,"Completed named probe lost independent continuation marker"); }
         QJsonObject runtime;
         try { runtime=describe(moderator->GetInternalObject("NamedWitness")); }
         catch (BaseException &e) { runtime=QJsonObject{{"present",false},{"lookup_error",QString::fromStdString(e.GetFullMessage())}}; }
         QJsonObject result{{"completed",completed},{"run_result",int(status)},{"runtime",runtime},{"marker_bytes",QString::fromUtf8(bytes)},{"source_history_unchanged",true}};
         proof[phase]=result; persist(); return completed;
      };
      const bool writable=build("write_build",declaration+output+mission+"Write NamedWitness;\nReport Values Keep;\n");
      if (writable) run("write_run"); else { proof["write_run"]=QJsonObject{{"attempted",false},{"reason","Write mission did not Build"}}; persist(); }
      require(build("recovery_build","% Named probe recovery Ω\nCreate Variable Keep;\nKeep = 41;\n"+output+mission+"Report Values Keep;\n"),"Named probe did not recover after observed outcome");
      require(run("recovery_run") && !moderator->GetConfiguredObject("NamedWitness"),"Named probe recovery failed or leaked the prior configured object");
      acceptanceView(rig.window);
      proof["scope"]="Complete named main Create/storage/Write observations for this recorded factory entry,including unsuccessful Build/Run outcomes and independent recovery. Metadata and generated text are observations,not numerical oracles. Default-setup failure does not exclude a type. No function/global,owned-operation,scientific or whole NASA qualification.";
      rig.save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
   }
   catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; }
   catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; }
   return 1;
}
