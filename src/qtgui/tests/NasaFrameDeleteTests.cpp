#include "FormationAcceptance.hpp"
#include "CoordinateSystem.hpp"
#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
using namespace formaccept;

int main(int argc,char **argv)
{
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaFrameDelete");
 try {
  require(argc==4,"Expected frame deletion startup, case file and key");QJsonObject spec;const QString key=argv[3];
  for(auto c:QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object()["cases"].toArray())if(c.toObject()["key"]==key)spec=c.toObject();
  require(!spec.isEmpty(),"Frame deletion case absent");const QString mode=spec["mode"].toString(),name=spec["name"].toString();const bool protectedFrame=mode=="protected";Rig rig(QFileInfo(argv[1]).absoluteFilePath());
  const auto saved=rig.files.filePath("Frame protection Ω.script");
  QString initial="% Frame deletion source Ω\nCreate Variable Keep;\nKeep = 41;\nCreate Spacecraft Sat;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\nSat.CoordinateSystem = "+QString(mode=="resource-ref"?"ScratchFrame":"EarthMJ2000Eq")+";\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.5;\nSat.VZ = 1;\n";
  if(!protectedFrame)initial+="Create CoordinateSystem ScratchFrame;\nScratchFrame.Origin = Earth;\nScratchFrame.Axes = MJ2000Eq;\n";
  initial+="Create CoordinateSystem WitnessFrame;\nWitnessFrame.Origin = Mars;\nWitnessFrame.Axes = MJ2000Eq;\nCreate ReportFile Values;\nValues.Filename = '"+rig.report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\nKeep = Keep + 1;\nReport Values Keep Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ"+QString(mode=="command-ref"?" Sat.ScratchFrame.X":"")+";\n% Frame deletion tail Δ\n";
  rig.source->setPlainText(initial);require(rig.window.buildScript()&&rig.window.saveScriptTo(saved),"Frame deletion fixture/save failed");rig.source->document()->setModified(false);const int undo=rig.source->document()->availableUndoSteps();rig.save("initial.script",initial.toUtf8());rig.save("case.json",QJsonDocument(spec).toJson());write(rig.report,"unexecuted sentinel\n");
  auto frame=[&](const QString &which){auto *object=dynamic_cast<CoordinateSystem *>(Moderator::Instance()->GetConfiguredObject(which.toStdString()));require(object,"Frame deletion native coordinate missing/wrong type");return object;};
  auto snapshot=[&]{QJsonArray entries;QStringList names{"EarthMJ2000Eq","EarthMJ2000Ec","EarthFixed","EarthICRF","WitnessFrame"};if(!protectedFrame)names.append(name);for(const auto &which:names){auto *f=frame(which);entries.append(QJsonObject{{"name",which},{"built_in",f->IsBuiltIn()},{"origin",QString::fromStdString(f->GetStringParameter(f->GetParameterID("Origin")))},{"axes",QString::fromStdString(f->GetStringParameter(f->GetParameterID("Axes")))},{"generated",QString::fromStdString(f->GetGeneratingString(Gmat::SHOW_SCRIPT))}});}return entries;};
  auto identity=[&]{auto *f=frame(name);require(f->IsBuiltIn()==protectedFrame&&QString::fromStdString(f->GetStringParameter(f->GetParameterID("Origin")))=="Earth"&&QString::fromStdString(f->GetStringParameter(f->GetParameterID("Axes")))==spec["axes"].toString(),"Frame deletion built-in/definition precondition differs");};identity();const auto before=snapshot();rig.save("native-before.json",QJsonDocument(before).toJson());
  auto stable=[&]{identity();require(rig.source->toPlainText()==initial&&!rig.source->document()->isModified()&&rig.source->document()->availableUndoSteps()==undo&&read(saved)==initial.toUtf8()&&snapshot()==before&&Moderator::Instance()->GetConfiguredObject("Keep")->GetRealParameter("Value")==41,"Frame deletion refusal changed source/history/file/native definitions");};
  QJsonArray menus,decisions;QString directError;
  auto context=[&](const QString &phase){
   for(auto *tabs:rig.window.findChildren<QTabWidget *>())for(int i=0;i<tabs->count();++i)if(tabs->tabText(i)=="Resources")tabs->setCurrentIndex(i);
   auto found=rig.resources->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);require(found.size()==1,"Frame deletion actual tree identity absent/ambiguous");rig.resources->expandAll();rig.resources->scrollToItem(found[0]);events();acceptanceView(*rig.resources);
   bool visited=false;std::exception_ptr failure;QTimer dialogs;dialogs.setInterval(10);int stage=0;
   QObject::connect(&dialogs,&QTimer::timeout,&rig.window,[&]{auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());if(!box)return;try{
    acceptanceView(*box);
    if(stage==0&&box->windowTitle()=="Delete resource"){require(box->text().contains(name)&&box->button(QMessageBox::Yes),"Frame deletion confirmation identifies wrong resource");decisions.append(QJsonObject{{"phase",phase},{"title",box->windowTitle()},{"text",box->text()},{"choice","Yes"}});stage=1;QTest::mouseClick(box->button(QMessageBox::Yes),Qt::LeftButton);}
    else if(stage==1&&box->windowTitle()=="Cannot delete resource"){const auto diagnostic=box->text();require(diagnostic.contains("used by another resource or mission command"),"Referenced frame deletion failed for an unexpected reason");decisions.append(QJsonObject{{"phase",phase},{"title",box->windowTitle()},{"text",diagnostic},{"choice","Ok"}});stage=2;dialogs.stop();QTest::mouseClick(box->button(QMessageBox::Ok),Qt::LeftButton);}
    else throw std::runtime_error("Unexpected frame deletion dialog sequence");
   }catch(...){failure=std::current_exception();dialogs.stop();box->reject();}});
   QTimer::singleShot(0,&rig.window,[&]{auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());try{
    require(menu&&menu->objectName()=="resourceContextMenu","Frame deletion actual context menu absent");QAction *remove=nullptr,*open=nullptr;for(auto *a:menu->actions()){if(a->text().startsWith("Delete resource"))remove=a;if(a->objectName()=="openResource")open=a;}
    QJsonObject entry{{"phase",phase},{"name",name},{"open_enabled",open&&open->isEnabled()},{"delete_present",remove!=nullptr},{"delete_enabled",remove&&remove->isEnabled()}};menus.append(entry);rig.save("menus.json",QJsonDocument(menus).toJson());require(open&&open->isEnabled(),"Frame context menu did not target a ready named resource");acceptanceView(*menu);visited=true;
    if(protectedFrame){require(!remove,"Protected coordinate Delete is exposed instead of absent");QTest::keyClick(menu,Qt::Key_Escape);}
    else{require(remove&&remove->isEnabled(),"Referenced custom-frame Delete is unexpectedly hidden/disabled");dialogs.start();QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(remove).center());}
   }catch(...){failure=std::current_exception();if(menu)menu->close();}});
   const auto point=rig.resources->visualItemRect(found[0]).center();QContextMenuEvent event(QContextMenuEvent::Mouse,point,rig.resources->viewport()->mapToGlobal(point));QApplication::sendEvent(rig.resources->viewport(),&event);dialogs.stop();events();if(failure)std::rethrow_exception(failure);require(visited&&!QApplication::activePopupWidget()&&!QApplication::activeModalWidget()&&(protectedFrame?stage==0:stage==2),"Frame deletion context/refusal did not finish");stable();
  };
  context("before-run");require(read(rig.report)=="unexecuted sentinel\n","Frame deletion refusal overwrote unexecuted output");
  if(protectedFrame){directError=rig.window.deleteResource(name,initial);events();require(!directError.isEmpty(),"Frontend directly deleted a protected coordinate system");stable();}
  auto run=[&](const QString &phase){acceptanceView(rig.window);require(rig.window.runMission()==MainWindow::RunResult::Completed,"Frame deletion recovery mission failed");events();const auto raw=read(rig.report);const auto fields=QString::fromUtf8(raw).trimmed().split(',');const auto expected=spec["report"].toArray();require(fields.size()==expected.size(),"Frame deletion report cardinality differs");QJsonArray values;for(int i=0;i<fields.size();++i){bool ok=false;const double value=fields[i].toDouble(&ok);require(ok&&std::isfinite(value)&&std::abs(value-expected[i].toDouble())<1e-10,"Frame deletion report differs from independent zero-time state");values.append(value);}auto *sat=Moderator::Instance()->GetInternalObject("Sat"),*keep=Moderator::Instance()->GetInternalObject("Keep");require(sat&&keep&&keep->GetRealParameter("Value")==42,"Frame deletion native continuation differs");QJsonArray native{42};for(const auto *field:{"X","Y","Z","VX","VY","VZ"})native.append(sat->GetRealParameter(field));for(int i=0;i<7;++i)require(std::abs(native[i].toDouble()-expected[i].toDouble())<1e-10,"Frame deletion native Cartesian state differs");stable();rig.save(phase+".csv",raw);return QJsonObject{{"phase",phase},{"report",values},{"native",native}};};
  QJsonArray runs;runs.append(run("first"));const auto last=read(rig.report);context("after-run");require(read(rig.report)==last,"Post-run frame refusal altered the prior report");runs.append(run("second"));
  rig.save("final.script",rig.source->toPlainText().toUtf8());rig.save("native-after.json",QJsonDocument(snapshot()).toJson());rig.save("decisions.json",QJsonDocument(decisions).toJson());rig.save("messages.txt",rig.messages->toPlainText().toUtf8());
  const QJsonObject proof{{"checkpoint","frame-delete-"+key},{"outcome","passed"},{"case",key},{"name",name},{"mode",mode},{"built_in",protectedFrame},{"menus",menus},{"decisions",decisions},{"direct_error",directError},{"runs",runs},{"facets",QJsonArray{"actual-coordinate-context-menu-before-and-after-mission","four-built-in-frame-delete-absence-and-direct-refusal","custom-frame-resource-command-reference-error-and-no-mutation","independent-raw-native-zero-time-results-and-exact-definition-history-file-isolation"}},{"scope","Four documented built-in coordinate systems and two selected custom-frame reference paths. Protected Delete must be absent; referenced custom Delete is visible and must report refusal. Existing allowed custom-frame lifecycle remains the affected positive control. Toolkit event delivery, not native OS injection; no arbitrary frame transformation accuracy/platform/stress/full-build claim."}};
  rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException&e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception&e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
