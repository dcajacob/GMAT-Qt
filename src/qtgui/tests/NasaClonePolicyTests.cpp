#include "FormationAcceptance.hpp"
#include "CelestialBodyPanel.hpp"
#include "ResourceEditor.hpp"
#include "SpacePoint.hpp"
#include <QAction>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <functional>
using namespace formaccept;
static void reveal(QWidget *w) { require(w,"Clone control absent"); for (auto *p=w;p;p=p->parentWidget()) if (auto *s=qobject_cast<QScrollArea *>(p)) s->ensureWidgetVisible(w); events(); require(w->isVisible()&&w->isEnabled(),"Clone control hidden/disabled"); }

int main(int argc,char **argv)
{
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
 app.setOrganizationName("GMATTests"); app.setApplicationName("NasaClonePolicy");
 try {
  require(argc==4,"Supply clone startup, cases and key"); const QString key=argv[3]; QJsonObject spec;
  for (auto v:QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object()["cases"].toArray()) if (v.toObject()["key"]==key) spec=v.toObject();
  require(!spec.isEmpty(),"Unknown clone policy case"); Rig rig(QFileInfo(argv[1]).absoluteFilePath());
  const QString mode=spec["mode"].toString(),name=spec["resource"].toString(),type=spec["type"].toString(),field=spec["field"].toString(),modified=spec["modified"].toString();
  const bool spacecraft=mode=="modified",body=!spacecraft&&key!="Propagator";
  const auto path=rig.files.filePath("Clone policy Δ.script"),report=rig.files.filePath("clone policy.csv");
  QString config;
  if (spacecraft) {
   config="Create Spacecraft Sat;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = 21545;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.5;\nSat.VZ = 1;\nSat.DryMass = 913.25;\n";
   if (key=="SpacecraftAttitude") config+="Sat.Attitude = 'Spinner';\nSat.AttitudeDisplayStateType = 'EulerAngles';\nSat.AttitudeRateDisplayStateType = 'AngularVelocity';\n";
   config+="Sat."+field+" = "+spec["base"].toString()+"; % retain initial spelling\n";
  } else if (!body) config="Create ForceModel SupportForces;\nCreate Propagator Integrator;\nIntegrator.FM = SupportForces;\nIntegrator.Type = RungeKutta89;\nIntegrator.InitialStepSize = 120;\nIntegrator.MinStep = 0;\nIntegrator.MaxStep = 300;\n";
  const QString initial="% Clone policy source Ω\n"+config+"Create Variable Keep OtherValue;\nKeep = 41;\nOtherValue = 17;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\nKeep = Keep + 1;\nOtherValue = OtherValue + 2;\nReport Values Keep OtherValue;\n% Clone policy tail Δ\n";
  write(path,initial.toUtf8()); require(rig.window.loadScript(path)&&rig.window.buildScript(),"Clone policy fixture build failed"); rig.save("initial.script",initial.toUtf8()); rig.save("case.json",QJsonDocument(spec).toJson());
  auto object=[&](const QString &which,bool runtime=false)->GmatBase * {
   auto *engine=Moderator::Instance(); GmatBase *o=body?engine->GetSolarSystemInUse()->GetBody(which.toStdString()):(runtime?engine->GetInternalObject(which.toStdString()):engine->GetConfiguredObject(which.toStdString()));
   require(o&&o->GetTypeName()==type.toStdString(),"Clone native identity/type differs"); if (body) require(!dynamic_cast<CelestialBody *>(o)->IsUserDefined(),"Protected body fixture is user-defined"); return o;
  };
  auto property=[&](const QString &which,bool runtime=false) { const double v=object(which,runtime)->GetRealParameter(field.toStdString()); require(std::isfinite(v),"Clone native field nonfinite"); return v; };
  auto serialization=[&] { return QString::fromStdString(object(name)->GetGeneratingString(Gmat::SHOW_SCRIPT)); };
  auto item=[&](const QString &which) {
   for (auto *t:rig.window.findChildren<QTabWidget *>()) for (int i=0;i<t->count();++i) if (t->tabText(i)=="Resources") t->setCurrentIndex(i);
   const auto entries=rig.resources->findItems(which,Qt::MatchExactly|Qt::MatchRecursive); require(entries.size()==1,"Clone tree identity absent/ambiguous"); rig.resources->expandAll(); rig.resources->setCurrentItem(entries[0]); rig.resources->scrollToItem(entries[0]); events(); return entries[0];
  };
  auto open=[&](const QString &which) {
   rig.resources->itemDoubleClicked(item(which),0); events(); QPointer<QMdiSubWindow> owner=rig.area->activeSubWindow(); require(owner&&owner->property("resourceName")==which,"Clone opened wrong owner"); owner->showNormal(); owner->move(0,0); owner->resize(rig.area->viewport()->size()); rig.area->setActiveSubWindow(owner); acceptanceView(*owner); return owner;
  };
  auto editedSource=[&](QString text,const QString &which,const QString &value) {
   const QString prefix="^(?:GMAT )?"+which+"\\."+field+" = [^;\\n]*;";
   text.remove(QRegularExpression(prefix+"(?=[ \t]*%)",QRegularExpression::MultilineOption));
   text.remove(QRegularExpression(prefix+"[ \t]*\\n",QRegularExpression::MultilineOption));
   const QString line=QString(body?"GMAT ":"")+which+"."+field+" = "+value+";\n";
   require(text.count("BeginMissionSequence;\n")==1,"Clone field source boundary ambiguous"); return text.replace("BeginMissionSequence;\n",line+"BeginMissionSequence;\n");
  };
  QJsonArray states,menus,runs,operations;
  QMap<QString,QPair<UnsignedInt,UnsignedInt>> expectedColors;
  auto record=[&](const QString &phase,const QString &which) {
   const double value=property(which); states.append(QJsonObject{{"phase",phase},{"name",which},{"field",field},{"value",value},{"serialization",QString::fromStdString(object(which)->GetGeneratingString(Gmat::SHOW_SCRIPT))}}); rig.save("states.json",QJsonDocument(states).toJson()); return value;
  };
  auto edit=[&](const QString &which,const QString &value) {
   const auto before=rig.source->toPlainText(),wanted=editedSource(before,which,value); const int undo=rig.source->document()->availableUndoSteps(); const auto prior=property(which);
   auto owner=open(which); auto *panel=dynamic_cast<EditablePanel *>(owner->widget()); require(panel,"Clone editable panel absent");
   if (spacecraft) for (auto tab:spec["tabs"].toArray()) { bool found=false; for (auto *tabs:owner->findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->tabText(i)==tab.toString()) { tabs->setCurrentIndex(i); found=true; } require(found,"Clone spacecraft page absent"); }
   auto *input=owner->findChild<QLineEdit *>(spec["control"].toString()); require(input&&!input->isReadOnly(),"Clone editable field absent/read-only"); reveal(input); input->setFocus(); input->selectAll(); QTest::keyClicks(input,value); events();
   require(input->text()==value&&panel->hasChanges()&&rig.source->toPlainText()==before&&property(which)==prior&&rig.source->document()->availableUndoSteps()==undo,"Clone draft changed source/native/history");
   auto *buttons=owner->findChild<QDialogButtonBox *>(); require(buttons&&buttons->button(QDialogButtonBox::Apply),"Clone Apply absent"); auto *apply=buttons->button(QDialogButtonBox::Apply); reveal(apply); acceptanceView(*owner); QTest::mouseClick(apply,Qt::LeftButton); events();
   if (rig.source->toPlainText()!=wanted) { rig.save("unexpected-apply.script",rig.source->toPlainText().toUtf8()); rig.save("wanted-apply.script",wanted.toUtf8()); }
   require(owner&&rig.source->toPlainText()==wanted&&std::abs(property(which)-value.toDouble())<1e-12&&rig.source->document()->availableUndoSteps()>undo,"Clone field Apply source/native/history differs");
   auto *current=dynamic_cast<EditablePanel *>(owner->widget()); require(current&&!current->hasChanges()&&owner->close(),"Clone field Apply did not leave clean owner"); events(); require(!owner,"Clone field editor did not retire"); operations.append("apply-"+which+"-"+value); record("applied",which); rig.save("applied-"+which+"-"+value+".script",wanted.toUtf8());
  };
  auto menu=[&](const QString &which,bool enabled,const std::function<void(QDialog *)> &choose={}) {
   auto *entry=item(which); const auto before=rig.source->toPlainText(),native=serialization(); const int undo=rig.source->document()->availableUndoSteps(); const bool changed=rig.source->document()->isModified();
   bool visited=false,chosen=false; std::exception_ptr failure; QTimer modal; modal.setInterval(10);
   QObject::connect(&modal,&QTimer::timeout,&rig.window,[&] {
    auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); if (!dialog) return; modal.stop();
    try { require(dialog->objectName()=="cloneResourceDialog"&&choose,"Clone modal absent/unexpected"); acceptanceView(*dialog); chosen=true; choose(dialog); }
    catch (...) { failure=std::current_exception(); dialog->reject(); }
   });
   acceptanceView(*rig.resources);
   QTimer::singleShot(0,&rig.window,[&] {
    auto *popup=qobject_cast<QMenu *>(QApplication::activePopupWidget());
    try {
     require(popup,"Clone actual context menu absent"); QAction *clone=nullptr; for (auto *a:popup->actions()) if (a->objectName()=="cloneResource") clone=a;
     require(clone&&clone->isEnabled()==enabled,"Clone availability contradicts documented default-resource policy"); acceptanceView(*popup); visited=true;
     if (!enabled) { QTest::mouseClick(popup,Qt::LeftButton,Qt::NoModifier,popup->actionGeometry(clone).center()); require(!QApplication::activeModalWidget(),"Disabled Clone opened a dialog"); QTest::keyClick(popup,Qt::Key_Escape); }
     else { popup->setActiveAction(clone); modal.start(); QTest::keyClick(popup,Qt::Key_Return); }
    } catch (...) { failure=std::current_exception(); if (popup) popup->close(); }
   });
   const auto point=rig.resources->visualItemRect(entry).center(); QContextMenuEvent event(QContextMenuEvent::Mouse,point,rig.resources->viewport()->mapToGlobal(point)); QApplication::sendEvent(rig.resources->viewport(),&event); events(); modal.stop(); if (failure) std::rethrow_exception(failure); require(visited&&(!enabled||chosen),"Clone actual menu/modal was not exercised");
   if (!enabled) require(rig.source->toPlainText()==before&&serialization()==native&&rig.source->document()->availableUndoSteps()==undo&&rig.source->document()->isModified()==changed&&!QApplication::activeModalWidget(),"Protected Clone changed source/native/history or opened modal");
   menus.append(QJsonObject{{"name",which},{"enabled",enabled},{"source_preserved",!enabled},{"undo_before",undo},{"undo_after",rig.source->document()->availableUndoSteps()}}); rig.save("menus.json",QJsonDocument(menus).toJson());
  };
  auto run=[&](const QString &phase,const QMap<QString,QString> &expected) {
   rig.closePanels(); const auto source=rig.source->toPlainText(); const int undo=rig.source->document()->availableUndoSteps(); write(report,"unexecuted sentinel\n"); acceptanceView(rig.window); require(rig.window.runMission()==MainWindow::RunResult::Completed,"Clone witness mission failed"); events();
   const auto raw=read(report); require(QString::fromUtf8(raw).trimmed()=="42,19","Clone report differs from independent continuation");
   require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42&&Moderator::Instance()->GetInternalObject("OtherValue")->GetRealParameter("Value")==19,"Clone native continuation differs"); QJsonArray native;
   for (auto it=expected.begin();it!=expected.end();++it) {
    require(std::abs(property(it.key(),true)-it.value().toDouble())<1e-12,"Clone runtime field differs from accepted definition"); QJsonObject witness{{"name",it.key()},{"field",field},{"value",property(it.key(),true)}};
    if (spacecraft) { auto *sat=dynamic_cast<Spacecraft *>(object(it.key(),true)); require(sat&&sat->GetRealParameter("DryMass")==913.25&&sat->GetStringParameter("CoordinateSystem")=="EarthMJ2000Eq","Clone changed spacecraft mass/frame"); QJsonArray state; const double truth[]={7000,0,0,0,7.5,1}; for (int i=0;i<6;++i) { const double v=sat->GetState()[i]; require(std::abs(v-truth[i])<1e-12,"Clone changed unrelated Cartesian state"); state.append(v); } witness["state"]=state; witness["dry_mass"]=913.25; witness["frame"]="EarthMJ2000Eq";
     require(expectedColors.contains(it.key())&&sat->GetCurrentOrbitColor()==expectedColors[it.key()].first&&sat->GetCurrentTargetColor()==expectedColors[it.key()].second,"Clone runtime palette differs from original pre-clone values");
    }
    native.append(witness);
   }
   require(rig.source->toPlainText()==source&&rig.source->document()->availableUndoSteps()==undo,"Clone Run changed source/history"); rig.save(phase+".csv",raw); rig.save(phase+".script",source.toUtf8()); runs.append(QJsonObject{{"phase",phase},{"report",QJsonArray{42,19}},{"native",native}}); rig.save("runs.json",QJsonDocument(runs).toJson());
  };
  if (!spacecraft) {
   const double base=record("default",name); menu(name,false); operations.append("default-clone-unavailable");
   const auto unchanged=rig.source->toPlainText(); const auto error=rig.window.cloneResource(name,name+"Copy",unchanged); require(error.contains("cannot be cloned")&&rig.source->toPlainText()==unchanged,"Protected Clone backend guard missing");
   edit(name,modified); require(property(name)!=base,"Protected modified fixture did not change its field"); const auto applied=rig.source->toPlainText(); rig.save("modified.script",applied.toUtf8()); menu(name,false); operations.append("modified-clone-unavailable");
   require(!Moderator::Instance()->GetConfiguredObject((name+"Copy").toStdString()),"Protected Clone registered a copy");
   rig.source->undo(); require(rig.source->toPlainText()==initial&&rig.window.buildScript()&&property(name)==base,"Protected clone field Undo failed"); rig.source->redo(); require(rig.source->toPlainText()==applied&&rig.window.buildScript()&&property(name)==modified.toDouble(),"Protected clone field Redo failed"); operations.append("exact-field-undo-redo");
   require(rig.window.saveScriptTo(path)&&read(path)==applied.toUtf8()&&rig.window.loadScript(path)&&rig.window.buildScript(),"Protected clone Unicode save/reopen failed"); run("protected-reopened",{{name,modified}});
  } else {
   record("baseline",name); edit(name,modified); const auto applied=rig.source->toPlainText();
   // NASA specifies an exact object copy. Unwritten palette defaults depend
   // on creation order, so capture the original input values before cloning.
   auto *originalPoint=dynamic_cast<SpacePoint *>(object(name)); require(originalPoint,"Clone source has no native palette");
   const auto originalColors=qMakePair(originalPoint->GetCurrentOrbitColor(),originalPoint->GetCurrentTargetColor());
   const auto colorSource="Twin.OrbitColor = "+QString::fromStdString(originalPoint->GetOrbitColorString())+";\nTwin.TargetColor = "+QString::fromStdString(originalPoint->GetTargetColorString())+";\n";
   require(!applied.contains("Sat.OrbitColor =")&&!applied.contains("Sat.TargetColor ="),"Clone palette fixture no longer exercises implicit original defaults");
   expectedColors.insert("Sat",originalColors); expectedColors.insert("Twin",originalColors);
   auto clone=[&](bool accept) {
    const auto before=rig.source->toPlainText(); const int undo=rig.source->document()->availableUndoSteps();
    menu(name,true,[&](QDialog *dialog) {
     auto *input=dialog->findChild<QLineEdit *>("resourceName"); auto *buttons=dialog->findChild<QDialogButtonBox *>("resourceCloneButtons"); require(input&&buttons,"Modified clone name/actions absent");
     auto *fieldInput=dialog->findChild<QLineEdit *>(spec["control"].toString()); require(fieldInput&&std::abs(fieldInput->text().toDouble()-modified.toDouble())<1e-12,"Modified clone form did not inherit selected property");
     input->setText(accept?"Sat":"Twin"); acceptanceView(*dialog);
     if (!accept) { QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton); require(dialog->result()==QDialog::Rejected,"Clone Cancel did not close"); return; }
     QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton); events(); auto *status=dialog->findChild<QLabel *>("resourceCloneStatus"); require(dialog->isVisible()&&status&&status->text().contains("already")&&rig.source->toPlainText()==before&&rig.source->document()->availableUndoSteps()==undo,"Duplicate clone changed state or omitted correction diagnostic");
     input->setText("Twin"); QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton); events(); require(dialog->result()==QDialog::Accepted,"Modified clone correction failed");
    });
    if (!accept) require(rig.source->toPlainText()==before&&rig.source->document()->availableUndoSteps()==undo&&!Moderator::Instance()->GetConfiguredObject("Twin"),"Modified clone Cancel changed source/model/history");
    operations.append(accept?"modified-clone-accepted":"modified-clone-cancel"); rig.closePanels();
   };
   clone(false); clone(true); QString block="Create Spacecraft Twin;\n";
   for (const auto &line:applied.left(applied.indexOf("BeginMissionSequence;\n")).split('\n')) if (line.startsWith("Sat.")) block+="Twin"+line.mid(3)+"\n";
   block+=colorSource;
   QString cloned=applied; cloned.replace("BeginMissionSequence;\n",block+"BeginMissionSequence;\n");
   rig.save("expected-before-clone-check.script",cloned.toUtf8()); rig.save("actual-before-clone-check.script",rig.source->toPlainText().toUtf8());
   record("before-clone-check","Sat"); record("before-clone-check","Twin");
   auto *clonedPoint=dynamic_cast<SpacePoint *>(object("Twin"));
   auto *retainedPoint=dynamic_cast<SpacePoint *>(object(name));
   require(clonedPoint&&retainedPoint&&clonedPoint->GetCurrentOrbitColor()==originalColors.first&&clonedPoint->GetCurrentTargetColor()==originalColors.second&&retainedPoint->GetCurrentOrbitColor()==originalColors.first&&retainedPoint->GetCurrentTargetColor()==originalColors.second,"Clone did not preserve implicit original/copy native colors");
   require(rig.source->toPlainText()==cloned&&property("Sat")==modified.toDouble()&&property("Twin")==modified.toDouble(),"Modified clone differs from independent exact copied source or configured properties");
   rig.save("expected-cloned.script",cloned.toUtf8()); rig.save("cloned.script",rig.source->toPlainText().toUtf8()); record("cloned","Sat"); record("cloned","Twin"); run("cloned",{{"Sat",modified},{"Twin",modified}});
   edit("Twin",spec["third"].toString()); const auto twinEdited=rig.source->toPlainText(); require(property("Sat")==modified.toDouble(),"Editing clone changed original");
   edit("Sat",spec["fourth"].toString()); require(property("Twin")==spec["third"].toString().toDouble(),"Editing original changed clone"); const auto independent=rig.source->toPlainText(); operations.append("independent-original-and-copy-edits"); run("independent",{{"Sat",spec["fourth"].toString()},{"Twin",spec["third"].toString()}});
   for (const auto &wanted:QStringList{twinEdited,cloned,applied}) { rig.source->undo(); require(rig.source->toPlainText()==wanted&&rig.window.buildScript(),"Modified clone three-step exact Undo failed"); }
   require(!Moderator::Instance()->GetConfiguredObject("Twin"),"Clone Undo retained copied native object");
   for (const auto &wanted:QStringList{cloned,twinEdited,independent}) { rig.source->redo(); require(rig.source->toPlainText()==wanted&&rig.window.buildScript(),"Modified clone three-step exact Redo failed"); }
   operations.append("clone-and-independent-edits-three-step-history"); require(rig.window.saveScriptTo(path)&&read(path)==independent.toUtf8()&&rig.window.loadScript(path)&&rig.window.buildScript(),"Modified clone Unicode save/reopen failed"); run("reopened",{{"Sat",spec["fourth"].toString()},{"Twin",spec["third"].toString()}});
  }
  rig.save("final.script",rig.source->toPlainText().toUtf8()); rig.save("operations.json",QJsonDocument(operations).toJson());
  const QJsonObject proof{{"checkpoint","clone-policy-"+key},{"outcome","passed"},{"case",key},{"mode",mode},{"field",field},{"menus",menus},{"states",states},{"runs",runs},{"operations",operations},{"facets",QJsonArray{"actual-resource-menu-default-and-modified-clone-policy","actual-field-apply-native-source-and-draft-isolation","clone-cancel-or-refusal-exact-source-model-history","exact-undo-redo-unicode-save-reopen","independent-native-fields-and-continuation-results"}},{"scope","Default planetary-body and propagator Clone unavailable before and after selected field Apply; two spacecraft modified clones with independent original/copy edits and unchanged state/mass/frame. User-body clone conflict, historical aliases/ChangeAll/Name InputText recipes remain unresolved. No physical body accuracy, attitude dynamics, visual pixels, native OS input/GPU/platform/stress/current private corpus or full newest-build qualification."}};
  rig.save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; } catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
