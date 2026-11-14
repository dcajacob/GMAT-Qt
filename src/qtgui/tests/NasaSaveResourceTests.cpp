#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CommandEditor.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "GmatBase.hpp"
#include "DynamicDataDisplay.hpp"
#include "FileManager.hpp"
#include <QApplication>
#include <QCryptographicHash>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static void events(){QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QApplication::processEvents();}
static QByteArray read(const QString &path){QFile f(path);require(f.open(QIODevice::ReadOnly),"Save resource evidence missing");return f.readAll();}
static void write(const QString &path,const QByteArray &data){QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(data)==data.size(),"Save resource evidence write failed");}
static QJsonValue number(double value){require(std::isfinite(value),"Native resource property is nonfinite");return value;}
// Independent of resourceProperties() and every GetGeneratingString() path.
static QJsonValue value(GmatBase &object,int id,const QString &type){
 // The display's public native grid is the state API for its row/cell
 // settings. Its generic StringArray getter has no implementation.
 if(auto *display=dynamic_cast<DynamicDataDisplay *>(&object)){
  const auto name=QString::fromStdString(object.GetParameterText(id));
  if(QStringList{"AddParameters","TextColor","RowTextColors","BackgroundColor","WarnBounds","CritBounds"}.contains(name)){
   QJsonArray rows;for(const auto &nativeRow:display->GetDynamicDataStruct()){QJsonArray row;for(const auto &cell:nativeRow){
    if(name=="AddParameters")row.append(QString::fromStdString(cell.paramName));
    else if(name=="TextColor")row.append(static_cast<qint64>(cell.paramTextColor));
    // Automatic warning colors are computed output; the persistent setting
    // is automatic mode. Keep that mode distinct from a user-set color.
    else if(name=="RowTextColors")row.append(QJsonArray{static_cast<qint64>(cell.isTextColorUserSet?cell.paramTextColor:0),cell.isTextColorUserSet});
    else if(name=="BackgroundColor")row.append(static_cast<qint64>(cell.paramBackgroundColor));
    else if(name=="WarnBounds")row.append(QJsonArray{number(cell.warnLowerBound),number(cell.warnUpperBound)});
    else if(name=="CritBounds")row.append(QJsonArray{number(cell.critLowerBound),number(cell.critUpperBound)});
   }rows.append(row);}return rows;
  }
 }

 if(type=="Real"||type=="Time")return number(object.GetRealParameter(id));
 if(type=="Integer")return object.GetIntegerParameter(id);
 if(type=="UnsignedInt")return static_cast<qint64>(object.GetUnsignedIntParameter(id));
 if(type=="Boolean")return object.GetBooleanParameter(id);
 if(type=="GmatTime")return QString::fromStdString(object.GetGmatTimeParameter(id).ToString());
 if(type=="OnOff")return QString::fromStdString(object.GetOnOffParameter(id));
 if(type=="String"||type=="Object"||type=="Enumeration"||type=="Filename"||type=="Color"||type=="Equation")return QString::fromStdString(object.GetStringParameter(id));
 QJsonArray result;
 if(type=="StringArray"||type=="ObjectArray"){for(auto entry:object.GetStringArrayParameter(id))result.append(QString::fromStdString(entry));return result;}
 if(type=="IntegerArray"){for(auto entry:object.GetIntegerArrayParameter(id))result.append(entry);return result;}
 if(type=="UnsignedIntArray"){for(auto entry:object.GetUnsignedIntArrayParameter(id))result.append(static_cast<qint64>(entry));return result;}
 if(type=="BooleanArray"){for(auto entry:object.GetBooleanArrayParameter(id))result.append(bool(entry));return result;}
 if(type=="Rvector"){const auto &v=object.GetRvectorParameter(id);for(int i=0;i<v.GetSize();++i)result.append(number(v[i]));return result;}
 if(type=="Rmatrix"){const auto &m=object.GetRmatrixParameter(id);for(int row=0;row<m.GetNumRows();++row){QJsonArray r;for(int col=0;col<m.GetNumColumns();++col)r.append(number(m(row,col)));result.append(r);}return result;}
 if(type=="RealElement"){int rows=object.GetIntegerParameter("NumRows"),cols=object.GetIntegerParameter("NumCols");for(int row=0;row<rows;++row){QJsonArray r;for(int col=0;col<cols;++col)r.append(number(object.GetRealParameter(id,row,col)));result.append(r);}return result;}
 // Unsupported descriptors stay explicit getter gaps; GetField catches errors
 // as strings and must never masquerade as a successful native observation.
 throw std::runtime_error(("Unimplemented native property kind: "+type).toStdString());
}
static QJsonObject snapshot(GmatBase &object,std::set<GmatBase*> &active){
 require(active.insert(&object).second,"Cyclic owned resource graph");QJsonArray fields,owned;
 for(int id=0;id<object.GetParameterCount();++id){QJsonObject field{{"id",id},{"name",QString::fromStdString(object.GetParameterText(id))},{"type",QString::fromStdString(object.GetParameterTypeString(id))},{"read_only",object.IsParameterReadOnly(id)},{"cloaked",object.IsParameterCloaked(id)}};
  const auto name=field["name"].toString();
  // These computed outputs require a spacecraft attachment. A standalone
  // PowerSystem has no spacecraft; the persistence contract excludes them.
  if(object.IsOfType("PowerSystem")&&field["read_only"].toBool()&&QStringList{"TotalPowerAvailable","RequiredBusPower","ThrustPowerAvailable"}.contains(name)){field["not_observed"]="Computed output requires spacecraft attachment; read-only, not persistent configuration";fields.append(field);continue;}
  try{field["value"]=value(object,id,field["type"].toString());}catch(BaseException &e){field["getter_error"]=QString::fromStdString(e.GetFullMessage());}catch(std::exception &e){field["getter_error"]=e.what();}fields.append(field);
 }
 for(int i=0;i<object.GetOwnedObjectCount();++i){auto *child=object.GetOwnedObject(i);require(child,"Native owned resource missing");owned.append(snapshot(*child,active));}
 active.erase(&object);return {{"name",QString::fromStdString(object.GetName())},{"type",QString::fromStdString(object.GetTypeName())},{"fields",fields},{"owned",owned}};
}
// A body is held by the sandbox SolarSystem, not its ordinary object map.
// An initialized runtime spacecraft exposes that same native SolarSystem.
// Never substitute the configured pre-run object for the exported runtime state.
static GmatBase *runtimeObject(const QString &name,const QString &solarProbe){
 auto *moderator=Moderator::Instance();auto *configured=moderator->GetConfiguredObject(name.toStdString());
 if(!solarProbe.isEmpty()&&configured&&(configured->IsOfType(Gmat::CELESTIAL_BODY)||name=="SolarSystem")){
  auto *observer=dynamic_cast<Spacecraft *>(moderator->GetInternalObject(solarProbe.toStdString()));require(observer,"Runtime solar-system observer missing");auto *solar=observer->GetSolarSystem();require(solar&&solar!=moderator->GetConfiguredObject("SolarSystem"),"Body state oracle is not the sandbox solar-system clone");
  return name=="SolarSystem"?static_cast<GmatBase *>(solar):static_cast<GmatBase *>(solar->GetBody(name.toStdString()));
 }
 return moderator->GetInternalObject(name.toStdString());
}
// Optional orbit truth stays independent of active display descriptors and Save output.
static QJsonArray cartesianWitnesses(const QJsonObject &spec,bool runtime,const QString &solarProbe){
 QJsonArray records;
 for(const auto &entry:spec.value("cartesian_witnesses").toArray()){
  const auto witness=entry.toObject();const auto name=witness.value("object").toString();const auto expected=witness.value("expected").toArray();
  const auto positionBound=witness.value("position_bound_km").toDouble(),velocityBound=witness.value("velocity_bound_km_s").toDouble();
  auto *object=runtime?runtimeObject(name,solarProbe):Moderator::Instance()->GetConfiguredObject(name.toStdString());auto *spacecraft=dynamic_cast<Spacecraft *>(object);
  require(spacecraft&&expected.size()==6&&std::isfinite(positionBound)&&positionBound>0&&std::isfinite(velocityBound)&&velocityBound>0,"Save Cartesian witness object, shape or bounds invalid");
  const auto state=spacecraft->GetState("Cartesian");QJsonArray actual;
  for(int i=0;i<6;++i){require(expected[i].isDouble()&&std::isfinite(expected[i].toDouble())&&std::isfinite(state[i])&&std::abs(state[i]-expected[i].toDouble())<=(i<3?positionBound:velocityBound),"Save Cartesian state differs from independent orbit witness");actual.append(state[i]);}
  records.append(QJsonObject{{"object",name},{"coordinate_system",QString::fromStdString(spacecraft->GetStringParameter("CoordinateSystem"))},{"actual",actual},{"expected",expected},{"position_bound_km",positionBound},{"velocity_bound_km_s",velocityBound}});
 }
 return records;
}
// Resolve a declared owned-object type path through native ownership only.
// A missing or ambiguous step must fail rather than observing a sibling/root.
static GmatBase *ownedWitness(GmatBase *object,const QJsonObject &witness){
 for(const auto &entry:witness.value("owned_types").toArray()){
  GmatBase *selected=nullptr;int matches=0;
  if(object)for(int i=0;i<object->GetOwnedObjectCount();++i){auto *child=object->GetOwnedObject(i);if(child&&QString::fromStdString(child->GetTypeName())==entry.toString()){selected=child;++matches;}}
  require(matches==1&&!entry.toString().isEmpty(),"Owned literal witness path missing or ambiguous");object=selected;
 }
 return object;
}
static QJsonValue expandFixtureValue(const QJsonValue &value,const QString &root){
 if(value.isString())return QString(value.toString()).replace("@FILES@",root);
 if(value.isArray()){QJsonArray out;for(const auto &entry:value.toArray())out.append(expandFixtureValue(entry,root));return out;}
 if(value.isObject()){QJsonObject out;const auto obj=value.toObject();for(auto it=obj.begin();it!=obj.end();++it)out[it.key()]=expandFixtureValue(it.value(),root);return out;}
 return value;
}
static bool same(const QJsonValue &a,const QJsonValue &b){
 if(a.type()!=b.type())return false;if(a.isDouble())return std::abs(a.toDouble()-b.toDouble())<=1e-12*std::max(1.,std::abs(a.toDouble()));
 if(a.isArray()){const auto aa=a.toArray(),bb=b.toArray();if(aa.size()!=bb.size())return false;for(int i=0;i<aa.size();++i)if(!same(aa[i],bb[i]))return false;return true;}return a==b;
}
static void compare(const QJsonObject &before,const QJsonObject &after,const QString &path,QJsonArray &differences,QJsonArray &gaps,int &checked){
 if(before["type"]!=after["type"])differences.append(QJsonObject{{"path",path},{"reason","native type changed"}});
 const auto fields=before["fields"].toArray(),got=after["fields"].toArray();if(fields.size()!=got.size())differences.append(QJsonObject{{"path",path},{"reason","parameter count changed"}});
 for(int i=0;i<fields.size();++i){const auto a=fields[i].toObject(),b=i<got.size()?got[i].toObject():QJsonObject{};if(a["read_only"].toBool())continue;
  if(!a.contains("value")||!b.contains("value")){gaps.append(QJsonObject{{"path",path+"."+a["name"].toString()},{"before",a},{"after",b}});continue;}
  ++checked;if(a["name"]!=b["name"]||a["type"]!=b["type"]||!same(a["value"],b["value"]))differences.append(QJsonObject{{"path",path+"."+a["name"].toString()},{"before",a},{"after",b}});
 }
 const auto children=before["owned"].toArray();auto reloaded=after["owned"].toArray();
 // Force creators are written canonically, so their internal enumeration
 // order may change. Match each unique native force identity once; retain
 // ordered comparison for every other owned collection and property array.
 if(before["type"]=="ForceModel"||before["type"]=="ODEModel"){
  auto identity=[](const QJsonObject &node){return node["type"].toString()+"\n"+node["name"].toString();};std::set<QString> seenBefore,seenAfter;QJsonArray aligned;
  for(const auto &node:reloaded)if(!seenAfter.insert(identity(node.toObject())).second)differences.append(QJsonObject{{"path",path},{"reason","duplicate native force identity after reload"}});
  for(const auto &node:children){const auto key=identity(node.toObject());if(!seenBefore.insert(key).second)differences.append(QJsonObject{{"path",path},{"reason","duplicate native force identity before Save"}});QJsonObject match;int matches=0;for(const auto &other:reloaded)if(identity(other.toObject())==key){match=other.toObject();++matches;}if(matches==1)aligned.append(match);else differences.append(QJsonObject{{"path",path},{"reason","native force identity missing or ambiguous"}});}
  if(aligned.size()==reloaded.size())reloaded=aligned;
 }
if(children.size()!=reloaded.size())differences.append(QJsonObject{{"path",path},{"reason","owned resource count changed"}});
 for(int i=0;i<children.size()&&i<reloaded.size();++i)compare(children[i].toObject(),reloaded[i].toObject(),path+"/owned["+QString::number(i)+"]",differences,gaps,checked);
}
int main(int argc,char **argv){QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaSaveResource");try{
 require(argc==4&&acceptanceWidgetPlatform(),"Expected startup, Save-resource cases and key");const auto startup=QFileInfo(argv[1]).absoluteFilePath();const QString key=argv[3];QJsonObject spec;for(const auto &entry:QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object()["cases"].toArray())if(entry.toObject()["key"]==key)spec=entry.toObject();require(!spec.isEmpty(),"Unknown Save resource case");
 QDir::setCurrent(QFileInfo(startup).absolutePath());TestSettings settings;QTemporaryDir files;require(files.isValid(),"Save resource fixture directory missing");const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &name,const QByteArray &data){if(!evidence.isEmpty())write(QDir(evidence).filePath(name),data);};
 const auto assetRoot=files.filePath("fixture-inputs");QJsonArray assetRecords;const auto declaredFiles=spec.value("fixture_files").toObject();
 const bool useAssets=!declaredFiles.isEmpty()||QJsonDocument(spec).toJson().contains("@FILES@");
 if(useAssets){require(QDir().mkpath(assetRoot),"Save fixture asset directory unavailable");for(auto it=declaredFiles.begin();it!=declaredFiles.end();++it){
  const auto relative=it.key();require(!QDir::isAbsolutePath(relative)&&!relative.split('/').contains("..")&&!relative.isEmpty(),"Invalid Save fixture relative path");const auto destination=QDir(assetRoot).filePath(relative);require(QDir().mkpath(QFileInfo(destination).absolutePath()),"Save fixture asset parent unavailable");const auto data=it.value().toString().toUtf8();write(destination,data);const auto artifact="fixture-asset-"+QString::number(assetRecords.size())+".txt";save(artifact,data);assetRecords.append(QJsonObject{{"relative",relative},{"path",destination},{"artifact",artifact},{"sha256",QString::fromLatin1(QCryptographicHash::hash(data,QCryptographicHash::Sha256).toHex())},{"bytes",data.size()}});
 }save("fixture-assets.json",QJsonDocument(QJsonObject{{"root",assetRoot},{"files",assetRecords}}).toJson());}
 const auto solarProbe=spec.value("runtime_solar_system_probe").toString();
 const auto expandedSetup=spec.value("setup").toString().replace("@FILES@",assetRoot),expandedBeforeSave=spec.value("before_save").toString().replace("@FILES@",assetRoot);
 MainWindow window;require(window.initialize(startup),"Save resource runtime unavailable");window.show();events();acceptanceView(window);FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",files.path().toStdString());auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow");auto *tree=window.findChild<QTreeWidget *>("Mission");auto *area=window.findChild<QMdiArea *>("workspace");require(source&&messages&&tree&&area,"Save resource workspace missing");
 QStringList selected;for(auto v:spec["selected"].toArray())selected.append(v.toString());const QString before="Save 'ExportFamily' Keep; % retain Ω",after="Save 'ExportFamily' "+selected.join(' ')+"; % retain Ω";const QString baseline="% Native Save resource inventory Ω\nCreate Variable Keep;\nKeep = 41;\n"+expandedSetup+"BeginMissionSequence;\n"+expandedBeforeSave+before+"\nKeep = Keep + 1; % outside export\n";source->setPlainText(baseline);save("baseline.script",baseline.toUtf8());save("case.json",QJsonDocument(spec).toJson());const bool built=window.buildScript();save("build.messages.txt",messages->toPlainText().toUtf8());if(!built)std::cerr<<messages->toPlainText().toStdString()<<"\nSTATUS "<<window.statusBar()->currentMessage().toStdString()<<std::endl;require(built,"Save resource baseline build failed");
 auto *tabs=window.findChild<QTabWidget *>();for(int i=0;tabs&&i<tabs->count();++i)if(tabs->tabText(i)=="Mission")tabs->setCurrentIndex(i);QTreeWidgetItem *item=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->text(0).endsWith("ExportFamily"))item=*it;require(item,"Save resource Mission item missing");tree->expandAll();tree->setCurrentItem(item);tree->scrollToItem(item);events();acceptanceView(window);tree->itemDoubleClicked(item,0);events();auto *child=area->activeSubWindow();auto *panel=child?dynamic_cast<CommandEditor *>(child->widget()):nullptr;require(panel,"Actual Save resource command editor missing");child->showNormal();child->move(0,0);child->resize(area->viewport()->size());events();acceptanceView(*child);
 auto *field=panel->findChild<QLineEdit *>("commandField_Objects");require(field&&field->isVisible(),"Save resource Objects field missing");bool visited=false;std::exception_ptr failure;QTimer::singleShot(0,child,[&]{auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());try{require(dialog&&dialog->objectName()=="commandObjectDialog","Save resource picker missing");auto *list=dialog->findChild<QListWidget *>("commandObjectList");auto *buttons=dialog->findChild<QDialogButtonBox *>();require(list&&buttons,"Save resource picker controls missing");for(int i=0;i<list->count();++i)list->item(i)->setCheckState(Qt::Unchecked);require(!buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Empty resource selection accepted");for(int i=selected.size()-1;i>=0;--i){auto matches=list->findItems(selected[i],Qt::MatchExactly);require(matches.size()==1,"Save resource picker omitted selected resource");auto *row=list->takeItem(list->row(matches.front()));list->insertItem(0,row);row->setCheckState(Qt::Checked);}events();acceptanceView(*dialog);visited=true;buttons->button(QDialogButtonBox::Ok)->click();}catch(...){failure=std::current_exception();if(dialog)dialog->reject();}});panel->findChild<QPushButton *>("commandChoose_Objects")->click();if(failure)std::rethrow_exception(failure);require(visited&&field->text()==selected.join(' '),"Save resource picker order differs");panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();events();panel=dynamic_cast<CommandEditor *>(child->widget());require(panel,"Save resource live editor missing after Apply");const auto wanted=QString(baseline).replace(before,after);save("applied-source.script",source->toPlainText().toUtf8());QJsonArray applyLabels;for(auto *label:panel->findChildren<QLabel *>())applyLabels.append(label->text());save("apply-diagnostics.json",QJsonDocument(QJsonObject{{"labels",applyLabels},{"has_changes",panel->hasChanges()},{"status",window.statusBar()->currentMessage()}}).toJson());if(source->toPlainText()!=wanted||panel->hasChanges())std::cerr<<QJsonDocument(applyLabels).toJson().constData();require(source->toPlainText()==wanted&&!panel->hasChanges(),"Save resource Apply changed unrelated source");require(child->close(),"Save resource clean editor close failed");events();save("accepted.script",wanted.toUtf8());
 const auto status=window.runMission();events();save("run.messages.txt",messages->toPlainText().toUtf8());if(status!=MainWindow::RunResult::Completed)std::cerr<<messages->toPlainText().toStdString();require(status==MainWindow::RunResult::Completed,"Save resource mission failed");require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,"Save resource following mission statement not executed");
 QJsonObject runtime;for(auto entry:spec["objects"].toArray()){const QString name=entry.toString();auto *object=runtimeObject(name,solarProbe);if(!object)object=Moderator::Instance()->GetConfiguredObject(name.toStdString());require(object,"Saved native resource missing");std::set<GmatBase*> active;runtime[name]=snapshot(*object,active);}save("runtime-native.json",QJsonDocument(runtime).toJson());
 for(auto entry:spec["witnesses"].toArray()){const auto witness=expandFixtureValue(entry,assetRoot).toObject();auto *o=runtimeObject(witness["object"].toString(),solarProbe);if(!o)o=Moderator::Instance()->GetConfiguredObject(witness["object"].toString().toStdString());o=ownedWitness(o,witness);require(o&&QString::fromStdString(o->GetParameterTypeString(o->GetParameterID(witness["field"].toString().toStdString())))==witness["type"].toString(),"Save runtime literal witness descriptor type differs");require(o&&same(value(*o,o->GetParameterID(witness["field"].toString().toStdString()),witness["type"].toString()),witness["expected"]),"Save resource literal/reference witness differs");}
 const auto runtimeCartesian=cartesianWitnesses(spec,true,solarProbe);if(!runtimeCartesian.isEmpty())save("runtime-cartesian.json",QJsonDocument(runtimeCartesian).toJson());
 const auto output=files.filePath(spec["filename"].toString());const auto raw=read(output);save("export.data",raw);require(!raw.trimmed().isEmpty(),"Save produced an empty resource export");for(auto entry:spec["definitions"].toArray())require(QString::fromUtf8(raw).count(entry.toString())==1,"Save omitted or duplicated resource definition");
 const bool loaded=window.loadScript(output)&&window.buildScript();events();save("reload.messages.txt",messages->toPlainText().toUtf8());if(!loaded)std::cerr<<messages->toPlainText().toStdString();require(loaded,"Save resource export cannot independently rebuild");QJsonObject reloaded;QJsonArray differences,gaps;int checked=0;for(auto it=runtime.begin();it!=runtime.end();++it){auto *object=Moderator::Instance()->GetConfiguredObject(it.key().toStdString());require(object,"Reloaded resource missing");std::set<GmatBase*> active;const auto got=snapshot(*object,active);reloaded[it.key()]=got;compare(it.value().toObject(),got,it.key(),differences,gaps,checked);}save("reloaded-native.json",QJsonDocument(reloaded).toJson());save("native-differences.json",QJsonDocument(differences).toJson());save("native-getter-gaps.json",QJsonDocument(gaps).toJson());require(gaps.isEmpty(),"Writable native resource fields lack independent getter checks");require(differences.isEmpty(),"Save changed writable native resource properties or owned objects");require(checked>0,"Save native comparison checked no writable properties");
 for(auto entry:spec["witnesses"].toArray()){const auto witness=expandFixtureValue(entry,assetRoot).toObject();auto *object=Moderator::Instance()->GetConfiguredObject(witness["object"].toString().toStdString());object=ownedWitness(object,witness);require(object&&QString::fromStdString(object->GetParameterTypeString(object->GetParameterID(witness["field"].toString().toStdString())))==witness["type"].toString(),"Save reloaded literal witness descriptor type differs");require(object&&same(value(*object,object->GetParameterID(witness["field"].toString().toStdString()),witness["type"].toString()),witness["expected"]),"Save resource reloaded literal/reference witness differs");}

 const auto reloadedCartesian=cartesianWitnesses(spec,false,solarProbe);if(!reloadedCartesian.isEmpty())save("reloaded-cartesian.json",QJsonDocument(reloadedCartesian).toJson());

 const auto reloadProbe=spec.value("reload_probe").toString().replace("@FILES@",assetRoot);
 if(!reloadProbe.isEmpty()){
  const auto probeScript=source->toPlainText()+"\nBeginMissionSequence;\n"+reloadProbe;source->setPlainText(probeScript);save("reload-probe.script",probeScript.toUtf8());require(window.buildScript(),"Save reload probe build failed");require(window.runMission()==MainWindow::RunResult::Completed,"Save reload probe mission failed");events();save("reload-probe.messages.txt",messages->toPlainText().toUtf8());QJsonObject probeNative;
  for(auto entry:spec["objects"].toArray()){const auto name=entry.toString();auto *object=runtimeObject(name,solarProbe);require(object,"Save reload probe native resource missing");std::set<GmatBase*> active;probeNative[name]=snapshot(*object,active);}save("reload-probe-native.json",QJsonDocument(probeNative).toJson());
  for(auto entry:spec.value("probe_witnesses").toArray()){const auto witness=expandFixtureValue(entry,assetRoot).toObject();auto *object=runtimeObject(witness["object"].toString(),solarProbe);object=ownedWitness(object,witness);require(object&&same(value(*object,object->GetParameterID(witness["field"].toString().toStdString()),witness["type"].toString()),witness["expected"]),"Save reloaded execution probe differs from independent literal");}
 }
 const QJsonObject proof{{"checkpoint","save-resource-native"},{"outcome","passed"},{"case",key},{"selected",spec["selected"]},{"objects",spec["objects"]},{"witnesses",spec["witnesses"]},{"writable_native_properties_checked",checked},{"facets",QJsonArray{"actual-save-editor-and-picker","selection-order-and-empty-selection-guard","exact-source-Apply-and-following-statement","native-typed-getters-and-owned-object-metadata","standalone-export-rebuild-and-native-property-comparison"}},{"scope","Only listed fixtures: writable native descriptors and explicit literal/reference witnesses before and after reload. Other read-only fields are recorded without persistence claims. Complete any-Resource coverage requires every family, valid reference graph and unresolved getter gap to be conjoined. No new NASA requirement credit from a pilot."}};save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;}
