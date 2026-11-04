#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <iostream>
#include <memory>
#include <stdexcept>
static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
static QByteArray read(const QString &path){QFile f(path);require(f.open(QIODevice::ReadOnly),"Copy contract catalog absent");return f.readAll();}
int main(int argc,char **argv){
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("ResourcePolymorphicCopy");
   try{
      require(argc==3,"Supply startup and resource catalog");TestSettings settings;const auto startup=QFileInfo(argv[1]).absoluteFilePath();const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();QDir::setCurrent(QFileInfo(startup).absolutePath());
      const QStringList selected{"ConicalFOV","RectangularFOV","ErrorModel","ProcessNoiseModel","EstimatedParameter"};QJsonArray specs;
      QString script="Create Variable Unrelated;\nUnrelated = 23;\n";
      for(auto v:catalog["cases"].toArray()){
         const auto s=v.toObject();const auto type=s["type"].toString();if(!selected.contains(type))continue;specs.append(s);
         script+="Create "+type+" Src"+type+" Dst"+type+";\nSrc"+type+"."+s["field"].toString()+" = "+s["first"].toString()+";\nDst"+type+"."+s["field"].toString()+" = "+s["second"].toString()+";\n";
      }
      script+="BeginMissionSequence;\nUnrelated = Unrelated + 1;\n";require(specs.size()==5,"Copy contract requires all five repaired types");MainWindow window;require(window.initialize(startup),"Copy contract initialization failed");auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");require(editor,"Copy contract source editor missing");editor->setPlainText(script);require(window.buildScript(),"Copy contract typed fixture failed");
      QJsonArray checks;
      for(auto v:specs){
         const auto s=v.toObject();const auto type=s["type"].toString();const auto field=s["field"].toString().toStdString();const double first=s["first"].toString().toDouble(),second=s["second"].toString().toDouble();
         auto *sourcePrototype=Moderator::Instance()->GetConfiguredObject(("Src"+type).toStdString()),*targetPrototype=Moderator::Instance()->GetConfiguredObject(("Dst"+type).toStdString());require(sourcePrototype&&targetPrototype,"Copy contract configured instances missing");
         std::unique_ptr<GmatBase> source(sourcePrototype->Clone()),target(targetPrototype->Clone()),peer(targetPrototype->Clone());target->SetName("Destination");peer->SetName("IndependentDestination");
         require(source->GetRealParameter(field)==first&&target->GetRealParameter(field)==second,"Copy contract seed differs");target->Copy(source.get());peer->Copy(source.get());
         require(target->GetRealParameter(field)==first&&peer->GetRealParameter(field)==first&&source->GetRealParameter(field)==first,"Polymorphic Copy lost property/source values");require(target->GetName()=="Destination"&&peer->GetName()=="IndependentDestination","Polymorphic Copy changed destination identity");
         const bool owned=type=="ProcessNoiseModel"||type=="EstimatedParameter";GmatBase *ownedBefore=nullptr;
         if(owned){require(source->GetOwnedObjectCount()==1&&target->GetOwnedObjectCount()==1&&peer->GetOwnedObjectCount()==1,"Copy contract owned model count differs");auto *a=source->GetOwnedObject(0),*b=target->GetOwnedObject(0),*c=peer->GetOwnedObject(0);require(a&&b&&c&&a!=b&&a!=c&&b!=c,"Polymorphic Copy aliases owned models");ownedBefore=b;}
         target->SetRealParameter(field,second);require(source->GetRealParameter(field)==first&&peer->GetRealParameter(field)==first&&target->GetRealParameter(field)==second,"Copy mutation escaped independent destination");target->Copy(target.get());require(target->GetRealParameter(field)==second&&target->GetName()=="Destination"&&(!owned||target->GetOwnedObject(0)==ownedBefore),"Self Copy changed value,identity or owned model");
         int rejected=0;for(auto *invalid:{static_cast<GmatBase *>(nullptr),Moderator::Instance()->GetConfiguredObject("Unrelated")}){bool threw=false;try{target->Copy(invalid);}catch(BaseException &){threw=true;}require(threw&&target->GetRealParameter(field)==second&&target->GetName()=="Destination"&&(!owned||target->GetOwnedObject(0)==ownedBefore),"Null/mismatched Copy did not reject atomically");++rejected;}
         require(sourcePrototype->GetRealParameter(field)==first&&targetPrototype->GetRealParameter(field)==second,"Native Copy changed configured prototypes");checks.append(QJsonObject{{"type",type},{"field",s["field"]},{"source_value",first},{"independent_destination_value",second},{"peer_value",first},{"owned_model_independence",owned},{"invalid_sources_rejected",rejected},{"self_copy_preserved",true},{"configured_prototypes_preserved",true}});
      }
      require(editor->toPlainText()==script,"Native Copy changed GUI caller source");const QJsonObject proof{{"checkpoint","resource-polymorphic-copy-contract"},{"outcome","passed"},{"types",5},{"copies",10},{"self_copies",5},{"rejected_sources",10},{"independent_owned_models",2},{"checks",checks},{"facets",QJsonArray{"polymorphic-whole-object-value-copy-and-destination-names","independent-owned-models-and-mutating-copies","atomic-null-and-mismatched-type-rejection","self-copy-and-configured-prototype-preservation"}},{"scope","Supporting native Copy API contract using parsed typed fixtures. Not a visible GUI workflow,all fields,all model settings,covariance correctness,FOV geometry,statistical/filter science or heap-leak proof."}};
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(!evidence.isEmpty()){for(const auto &entry:QList<QPair<QString,QByteArray>>{{"copy-contract.script",script.toUtf8()},{"copy-contract.json",QJsonDocument(proof).toJson()}}){QFile f(QDir(evidence).filePath(entry.first));require(f.open(QIODevice::WriteOnly)&&f.write(entry.second)==entry.second.size(),"Copy contract evidence write failed");}}
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
