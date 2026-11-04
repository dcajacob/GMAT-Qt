#include "FormationAcceptance.hpp"
#include "FactoryManager.hpp"
#include "Parameter.hpp"
#include "Spinner.hpp"
#include <memory>
using namespace formaccept;
struct DerivedSpinner : Spinner { using Spinner::Spinner; };

int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("AttitudeResourceCopy");
   try
   {
      require(argc==3 && acceptanceWidgetPlatform(),"Supply attitude Copy startup and catalog");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath()); require(rig.window.buildScript(),"Attitude Copy baseline Build failed");
      const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();
      const auto text=rig.source->toPlainText(); const int history=rig.source->document()->availableUndoSteps(); const auto configured=Moderator::Instance()->GetListOfObjects(Gmat::UNKNOWN_OBJECT);
      std::unique_ptr<Parameter> incompatible(FactoryManager::Instance()->CreateParameter("Variable","Incompatible")); require(bool(incompatible),"Attitude Copy incompatible control absent");
      QJsonArray rows,failures;
      for (const auto &entry:catalog["cases"].toArray())
      {
         const auto spec=entry.toObject(); const auto type=spec["type"].toString(),field=spec["field"].toString();
         auto make=[&](const char *name) { return std::unique_ptr<GmatBase>(FactoryManager::Instance()->CreateObject(Gmat::ATTITUDE,type.toStdString(),name)); };
         auto source=make("FactorySource"),destination=make("FactoryDestination"); require(source && destination,"Attitude Copy constructor absent");
         const auto factorySourceName=QString::fromStdString(source->GetName()),factoryDestinationName=QString::fromStdString(destination->GetName());
         // Explicit native identities isolate Copy from the separately retained factory-name defect.
         source->SetName("CopySource"); destination->SetName("CopyDestination");
         source->SetStringParameter(field.toStdString(),spec["first"].toString().toStdString()); destination->SetStringParameter(field.toStdString(),spec["second"].toString().toStdString());
         auto snapshot=[&](GmatBase *object) { return QJsonObject{{"name",QString::fromStdString(object->GetName())},{"type",QString::fromStdString(object->GetTypeName())},{"field",QString::fromStdString(object->GetStringParameter(field.toStdString()))},{"global",object->IsGlobal()},{"local",object->IsLocal()}}; };
         const auto original=snapshot(source.get()),before=snapshot(destination.get());
         require(original["field"]==spec["first"] && before["field"]==spec["second"],"Attitude Copy independent literal seeds differ");
         QJsonObject row{{"type",type},{"native_type",original["type"]},{"field",field},{"source_seed",spec["first"]},{"destination_seed",spec["second"]},{"factory_source_name",factorySourceName},{"factory_destination_name",factoryDestinationName}};
         bool copied=false;
         try { destination->Copy(source.get()); copied=true; }
         catch (BaseException &e) { row["copy_error"]=QString::fromStdString(e.GetFullMessage()); require(snapshot(source.get())==original && snapshot(destination.get())==before,"Failed attitude Copy mutated its operands"); failures.append(type); }
         row["copy_passed"]=copied;
         if (copied)
         {
            require(destination->GetName()=="CopyDestination" && snapshot(destination.get())["field"]==spec["first"] && snapshot(source.get())==original,"Attitude Copy literal/identity/source differs");
            std::unique_ptr<GmatBase> clone(destination->Clone()); require(clone && snapshot(clone.get())["field"]==spec["first"],"Attitude Clone field differs");
            clone->SetStringParameter(field.toStdString(),spec["second"].toString().toStdString()); require(snapshot(destination.get())["field"]==spec["first"] && snapshot(source.get())==original,"Attitude clone aliases original state");
            destination->SetStringParameter(field.toStdString(),spec["second"].toString().toStdString()); require(snapshot(source.get())==original,"Attitude Copy destination aliases source"); const auto independent=snapshot(destination.get());
            int rejected=0; for (const GmatBase *bad:{static_cast<const GmatBase*>(nullptr),static_cast<const GmatBase*>(incompatible.get())})
            { bool threw=false; try { destination->Copy(bad); } catch (BaseException &) { threw=true; } require(threw && snapshot(destination.get())==independent,"Attitude invalid Copy was accepted or changed state"); ++rejected; }
            destination->Copy(destination.get()); require(snapshot(destination.get())==independent,"Attitude self-Copy changed state");
            row["copy_value"]=spec["first"]; row["source_unchanged"]=true; row["destination_identity_preserved"]=true; row["clone_independent"]=true; row["self_unchanged"]=true; row["invalid_rejections"]=rejected;
         }
         rows.append(row); rig.save("copy-progress.json",QJsonDocument(rows).toJson());
      }
      require(rows.size()==8 && rows.size()==catalog["copy_contract_count"].toInt(),"Attitude Copy class omitted"); int derivedRejected=0;
      QJsonArray formats;
      if (failures.isEmpty())
      {
         Spinner exact("Exact"); DerivedSpinner derived("Derived");
         for (auto pair:{std::make_pair(static_cast<GmatBase*>(&exact),static_cast<const GmatBase*>(&derived)),std::make_pair(static_cast<GmatBase*>(&derived),static_cast<const GmatBase*>(&exact))})
         { bool threw=false; try { pair.first->Copy(pair.second); } catch (BaseException &) { threw=true; } require(threw,"Attitude Copy silently sliced an unsupported derived type"); ++derivedRejected; }
         std::unique_ptr<GmatBase> nadir(FactoryManager::Instance()->CreateObject(Gmat::ATTITUDE,"NadirPointing","ModeWitness")); require(bool(nadir),"Attitude format control absent");
         for (auto matlab:{Gmat::MATLAB_STRUCT,Gmat::EPHEM_HEADER}) for (auto script:{Gmat::SCRIPTING,Gmat::OWNED_OBJECT,Gmat::SHOW_SCRIPT,Gmat::NO_COMMENTS,Gmat::OBJECT_EXPORT,Gmat::DEBUG_INSPECT})
         {
            const auto quoted=QString::fromStdString(nadir->GetGeneratingString(matlab));
            const auto unquoted=QString::fromStdString(nadir->GetGeneratingString(script));
            require(quoted.contains("AttitudeReferenceBody = 'Earth';") && unquoted.contains("AttitudeReferenceBody = Earth;") && !unquoted.contains("AttitudeReferenceBody = 'Earth';"),"Attitude serialization retained the previous format mode");
            formats.append(QJsonObject{{"quoted_mode",int(matlab)},{"script_mode",int(script)},{"quoted",quoted},{"unquoted",unquoted}});
         }
         rig.save("format-controls.json",QJsonDocument(formats).toJson());
      }
      require(rig.source->toPlainText()==text && rig.source->document()->availableUndoSteps()==history && configured==Moderator::Instance()->GetListOfObjects(Gmat::UNKNOWN_OBJECT),"Unmanaged attitude Copy controls changed source/history/configuration"); acceptanceView(rig.window);
      const QJsonObject proof{{"checkpoint","attitude-resource-copy-contract"},{"outcome",failures.isEmpty()?"passed":"failed"},{"class_count",rows.size()},{"rows",rows},{"failures",failures},{"invalid_rejections",2*(rows.size()-failures.size())},{"unsupported_derived_rejections",derivedRejected},{"format_controls",formats},{"scope","Constructed/uninitialized selected string-field Copy/Clone contract with explicit native source/destination names,independent literals,source/clone independence,self-copy and invalid-source guards. Factory-provided names are recorded separately. Twelve format transitions additionally check NadirPointing reference-name serialization. No frame/provider evaluation,all-state copying,attitude formula or scientific approval."}};
      rig.save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return failures.isEmpty()?0:1;
   }
   catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; }
   catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; }
   return 1;
}
