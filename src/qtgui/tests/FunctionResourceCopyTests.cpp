#include "FormationAcceptance.hpp"
#include "FactoryManager.hpp"
#include "Parameter.hpp"
#include "PointMassForce.hpp"
#include "Subscriber.hpp"
#include <QRegularExpression>
#include <memory>
using namespace formaccept;

struct DerivedPointMass : PointMassForce { using PointMassForce::PointMassForce; };
static bool same(const QJsonValue &a,const QJsonValue &b)
{
   if (a.isDouble() && b.isDouble()) return std::abs(a.toDouble()-b.toDouble())<1e-12;
   if (a.isArray() && b.isArray()) { const auto x=a.toArray(),y=b.toArray(); if (x.size()!=y.size()) return false; for (int i=0;i<x.size();++i) if (!same(x[i],y[i])) return false; return true; }
   return a==b;
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("FunctionResourceCopy");
   try
   {
      require(argc==3 && acceptanceWidgetPlatform(),"Supply resource Copy startup and catalog");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath()); require(rig.window.buildScript(),"Copy baseline Build failed");
      const auto text=rig.source->toPlainText(); const int history=rig.source->document()->availableUndoSteps(); const auto configured=Moderator::Instance()->GetListOfObjects(Gmat::UNKNOWN_OBJECT);
      const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();
      std::unique_ptr<Parameter> incompatible(FactoryManager::Instance()->CreateParameter("Variable","Incompatible")); require(bool(incompatible),"Copy incompatible control absent");
      QJsonArray rows;
      for (const auto &entry:catalog["cases"].toArray())
      {
         const auto spec=entry.toObject(); if (!spec["copy_contract"].toBool()) continue;
         const auto type=spec["type"].toString(),field=spec["field"].toString(),kind=spec["kind"].toString();
         auto make=[&](const char *name){return std::unique_ptr<GmatBase>(FactoryManager::Instance()->CreateObject(spec["category_id"].toInt(),type.toStdString(),name));};
         auto source=make("CopySource"),destination=make("CopyDestination"); require(source && destination,"Copy constructor control absent");
         QString first=spec["first"].toString(),second=spec["second"].toString();
         if (spec["read_only"].toBool()) { if (kind=="string") { first="Mars"; second="Luna"; } else { first="3"; second="7"; } }
         first.replace("@ROOT@",rig.files.path()); second.replace("@ROOT@",rig.files.path());
         auto parsed=[&](const QString &seed)->QJsonValue {
            if (kind=="real" || kind=="integer") return seed.toDouble();
            if (kind=="boolean") return seed=="true";
            if (kind=="vector") { QJsonArray values; for (const auto &word:seed.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)) values.append(word.toDouble()); return values; }
            return seed;
         };
         const auto expectedFirst=parsed(first),expectedSecond=parsed(second);
         auto set=[&](GmatBase *object,const QString &seed) {
            const int id=object->GetParameterID(field.toStdString());
            if (kind=="real") object->SetRealParameter(id,seed.toDouble());
            else if (kind=="integer") object->SetIntegerParameter(id,seed.toInt());
            else if (kind=="boolean") object->SetBooleanParameter(id,seed=="true");
            else if (kind=="onoff") object->SetOnOffParameter(id,seed.toStdString());
            else if (kind=="vector") { const auto values=parsed(seed).toArray(); Rvector vector(values.size()); for (int i=0;i<values.size();++i) vector[i]=values[i].toDouble(); object->SetRvectorParameter(id,vector); }
            else object->SetStringParameter(id,seed.toStdString());
         };
         auto value=[&](GmatBase *object)->QJsonValue {
            const int id=object->GetParameterID(field.toStdString());
            if (kind=="real") return object->GetRealParameter(id);
            if (kind=="integer") return object->GetIntegerParameter(id);
            if (kind=="boolean") return object->GetBooleanParameter(id);
            if (kind=="vector") { const auto &vector=object->GetRvectorParameter(id); QJsonArray values; for (int i=0;i<vector.GetSize();++i) values.append(vector[i]); return values; }
            return QString::fromStdString(kind=="onoff"?object->GetOnOffParameter(id):object->GetStringParameter(id));
         };
         auto snapshot=[&](GmatBase *object) { return QJsonObject{{"name",QString::fromStdString(object->GetName())},{"type",QString::fromStdString(object->GetTypeName())},{"field",value(object)},{"global",object->IsGlobal()},{"local",object->IsLocal()},{"script",QString::fromStdString(object->GetGeneratingString(Gmat::NO_COMMENTS))}}; };
         set(source.get(),first); set(destination.get(),second); require(same(value(source.get()),expectedFirst) && same(value(destination.get()),expectedSecond),"Copy independent native seeds differ");
         const auto sourceBefore=snapshot(source.get()); destination->Copy(source.get());
         require(same(value(destination.get()),expectedFirst) && destination->GetName()=="CopyDestination" && snapshot(source.get())==sourceBefore,"Polymorphic Copy value/identity/source differs");
         if (type=="MessageWindow") { rig.messages->clear(); const Real packet[]{1.234567890123456}; auto *subscriber=dynamic_cast<Subscriber*>(destination.get()); require(subscriber && subscriber->ReceiveData(packet,1),"Copied message subscriber rejected native data"); events(); rig.save("message-copy-output.txt",rig.messages->toPlainText().toUtf8()); require(rig.messages->toPlainText().contains(QString::number(packet[0],'f',first.toInt())),"Copied MessageWindow did not use the copied output precision"); }
         std::unique_ptr<GmatBase> cloned(destination->Clone()); require(cloned && same(value(cloned.get()),expectedFirst),"Copy clone control differs");
         set(cloned.get(),second); require(same(value(destination.get()),expectedFirst) && snapshot(source.get())==sourceBefore,"Copy clone aliases original state");
         set(destination.get(),second); require(snapshot(source.get())==sourceBefore,"Copy destination aliases source");
         const auto before=snapshot(destination.get()); int rejected=0;
         for (const GmatBase *bad:{static_cast<const GmatBase*>(nullptr),static_cast<const GmatBase*>(incompatible.get())})
         {
            bool threw=false; try { destination->Copy(bad); } catch (BaseException &) { threw=true; }
            require(threw && snapshot(destination.get())==before,"Invalid Copy source mutated destination or was accepted"); ++rejected;
         }
         destination->Copy(destination.get()); require(snapshot(destination.get())==before,"Copy self-assignment changed state");
         rows.append(QJsonObject{{"type",type},{"field",field},{"native_type",QString::fromStdString(source->GetTypeName())},{"source_seed",expectedFirst},{"destination_seed",expectedSecond},{"copy_value",expectedFirst},{"clone_independent",true},{"source_unchanged",true},{"destination_identity_preserved",true},{"invalid_rejections",rejected},{"self_unchanged",true}});
         rig.save("copy-progress.json",QJsonDocument(rows).toJson());
      }
      require(rows.size()==catalog["copy_contract_count"].toInt() && rows.size()==22,"Copy contract class omitted");
      PointMassForce exact("Exact"); DerivedPointMass derived("Derived"); int derivedRejected=0;
      for (auto pair:{std::make_pair(static_cast<GmatBase*>(&exact),static_cast<const GmatBase*>(&derived)),std::make_pair(static_cast<GmatBase*>(&derived),static_cast<const GmatBase*>(&exact))})
      {
         bool threw=false; try { pair.first->Copy(pair.second); } catch (BaseException &) { threw=true; }
         require(threw,"Inherited Copy silently sliced an unsupported derived class"); ++derivedRejected;
      }
      require(rig.source->toPlainText()==text && rig.source->document()->availableUndoSteps()==history && configured==Moderator::Instance()->GetListOfObjects(Gmat::UNKNOWN_OBJECT),"Unmanaged Copy checks changed GUI configuration/history/source");
      acceptanceView(rig.window);
      const QJsonObject proof{{"checkpoint","resource-polymorphic-copy-contract"},{"outcome","passed"},{"class_count",rows.size()},{"rows",rows},{"invalid_rejections",2*rows.size()},{"unsupported_derived_rejections",derivedRejected},{"scope","Unmanaged native Copy/Clone with independent literal selected fields,destination identity,source/clone independence,null/incompatible atomic rejection,self-copy and unsupported-derived rejection. Read-only script fields are changed only through the internal native API for Copy controls;script policy is unchanged. Constructed/uninitialized resource state only;no numerical model,initialized provider/frame,stream/file execution,all-state copying or whole NASA qualification."}};
      rig.save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
   }
   catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; }
   catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; }
   return 1;
}
