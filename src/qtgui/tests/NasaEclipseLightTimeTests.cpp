// Dedicated correction scenarios reuse the complete eclipse result workflow.
// Existing NasaEclipseResultTests.cpp and its assertions remain unchanged.
#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include "SpiceUsr.h"
#include "TestSettings.hpp"
#include <QApplication>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QStyle>
#include <QStyleOptionButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QByteArray read(const QString &path) { QFile f(path); if (!f.open(QIODevice::ReadOnly)) throw std::runtime_error(("Eclipse result artifact missing: "+path).toStdString()); return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Eclipse result artifact write failed"); }
static QString digest(const QString &path) { return QString::fromLatin1(QCryptographicHash::hash(read(path),QCryptographicHash::Sha256).toHex()); }
static void reveal(QWidget *w) { require(w,"Eclipse control missing"); for (QWidget *p=w;p;p=p->parentWidget()) if (auto *scroll=qobject_cast<QScrollArea *>(p)) scroll->ensureWidgetVisible(w); events(); require(w->isVisible()&&w->isEnabled(),"Eclipse control unreachable"); acceptanceView(*w); }
static void click(QDialogButtonBox *buttons,QDialogButtonBox::StandardButton which) { auto *button=buttons ? buttons->button(which) : nullptr; reveal(button); QTest::mouseClick(button,Qt::LeftButton); events(); }

int main(int argc,char **argv)
{
 QApplication app(argc,argv); QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QLocale::setDefault(QLocale::c());
 app.setOrganizationName("GMATTests"); app.setApplicationName("NasaEclipseLightTime");
 try {
  require(argc==4&&acceptanceWidgetPlatform(),"Expected startup, light-time cases and key");
  const QString key=argv[3],startup=QFileInfo(argv[1]).absoluteFilePath(); const auto fixture=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object(); QJsonObject spec;
  for (auto c:fixture["cases"].toArray()) if (c.toObject()["key"]==key) spec=c.toObject(); require(!spec.isEmpty(),"Eclipse result case missing");
  QDir::setCurrent(QFileInfo(startup).absolutePath()); const auto originalStartup=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_STARTUP_ORIGINAL",startup); const auto root=QDir::cleanPath(QDir(QFileInfo(originalStartup).absolutePath()).absoluteFilePath("../.."));
  const auto kernels=fixture["physical_inputs"].toObject()["kernels"].toObject();
  for (auto it=kernels.begin();it!=kernels.end();++it) require(digest(QDir(root).filePath(it.key()))==it.value().toString(),"Runtime eclipse physical input differs from independent oracle kernel");
  TestSettings settings; QTemporaryDir files; require(files.isValid(),"Eclipse fixture directory unavailable");
  const auto report=files.filePath("Eclipse intervals Δ.txt"),truth=files.filePath("Eclipse final Δ.csv"),saved=files.filePath("Eclipse Δ.script"); const QByteArray sentinel="KEEP prior report bytes: Δ\n"; write(report,sentinel);
  const auto target=spec["target"].toString(),initialTarget=target=="Sat" ? QString("Other") : QString("Sat");
  QString prefix="% Preserve eclipse source Δ\nSolarSystem.SPKFilename = '"+QDir(root).filePath("application/data/planetary_ephem/spk/DE421AllPlanets.bsp")+"';\nCreate Variable Keep;\nKeep = 9;\n";
  const QStringList components{"X","Y","Z","VX","VY","VZ"};
  for (const auto &name:QStringList{"Sat","Other"}) {
   const auto state=spec[name==target ? "initial_state" : "other_state"].toArray();
   prefix+="Create Spacecraft "+name+";\n"+name+".DateFormat = UTCGregorian;\n"+name+".Epoch = '01 Jan 2000 12:00:00.000';\n"+name+".CoordinateSystem = EarthMJ2000Eq;\n"+name+".DisplayStateType = Cartesian;\n"+name+".NAIFId = "+(name=="Sat" ? "-100070" : "-100071")+";\n";
   for (int i=0;i<6;++i) prefix+=name+"."+components[i]+" = "+QString::number(state[i].toDouble(),'g',17)+";\n";
  }
  prefix+="Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nCreate Propagator Prop;\nProp.FM = FM;\nProp.Type = RungeKutta89;\nProp.InitialStepSize = 10;\nProp.MinStep = 10;\nProp.MaxStep = 10;\nProp.Accuracy = 1e-12;\nCreate EclipseLocator Events;\nEvents.Spacecraft = "+initialTarget+";\nEvents.OccultingBodies = {Luna};\nEvents.EclipseTypes = {Umbra};\nEvents.RunMode = Disabled;\nEvents.UseLightTimeDelay = true;\nEvents.UseStellarAberration = true;\nEvents.WriteReport = "+(spec["write_report"].toBool() ? "false" : "true")+";\nEvents.Filename = '"+files.filePath("inactive report.txt")+"';\nEvents.StepSize = 60;\nCreate ReportFile Truth;\nTruth.Filename = '"+truth+"';\nTruth.WriteHeaders = false;\nTruth.FixedWidth = false;\nTruth.Delimiter = ',';\nTruth.Precision = 17;\n";
  QString tail="BeginMissionSequence;\nPropagate 'Prescribed circles' Prop(Sat,Other) {Sat.ElapsedSecs = 6500};\nReport 'Final witness' Truth";
  for (const auto &component:components) tail+=' '+target+".EarthMJ2000Eq."+component;
  tail+=" Keep; % preserve final Ω\n"; const auto initial=prefix+tail;
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"); auto save=[&](const QString &name,const QByteArray &bytes) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes); };
  MainWindow window; require(window.initialize(startup),"Eclipse runtime initialization failed"); window.show(); acceptanceView(window); events();
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow"); auto *area=window.findChild<QMdiArea *>("workspace"); require(source&&messages&&area,"Eclipse workspace missing");
  source->setPlainText(initial); const bool built=window.buildScript(); if (!built) std::cerr<<messages->toPlainText().toStdString(); require(built,"Eclipse initial build failed"); source->document()->setModified(false); const int undo=source->document()->availableUndoSteps(); save("initial.script",initial.toUtf8());
  auto *earth=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth"); require(std::abs(earth->GetGravitationalConstant()-spec["mu"].toDouble())<1e-9&&earth->GetStringParameter(earth->GetParameterID("SpiceFrameId"))=="ITRF93","Eclipse gravitational/frame input differs from oracle");
  require(digest(QString::fromStdString(FileManager::Instance()->GetFullPathname(FileManager::ICRF_FILE)))==kernels["application/data/icrf/ICRF_Table.txt"].toString(),"Native FK5/ICRF frame table differs from independent input");
  const auto pck=QString::fromStdString(FileManager::Instance()->GetFullPathname(FileManager::PLANETARY_PCK_FILE)); require(digest(pck)==kernels["application/data/planetary_coeff/SPICEPlanetaryConstantsKernel.tpc"].toString(),"Event manager PCK input differs from oracle");
  auto pristine=[&] { auto *object=Moderator::Instance()->GetConfiguredObject("Events"); return source->toPlainText()==initial&&source->document()->availableUndoSteps()==undo&&!source->document()->isModified()&&object&&object->GetStringParameter("RunMode")=="Disabled"&&object->GetStringParameter("Spacecraft")==initialTarget.toStdString()&&object->GetBooleanParameter("UseLightTimeDelay")&&object->GetBooleanParameter("UseStellarAberration"); };
  QPointer<QMdiSubWindow> child; ResourceEditor *panel=nullptr;
  auto open=[&] {
   auto *tree=window.findChild<QTreeWidget *>("Resources"); for (auto *tabs:window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==tree||tabs->widget(i)->isAncestorOf(tree)) tabs->setCurrentIndex(i);
   const auto found=tree->findItems("Events",Qt::MatchExactly|Qt::MatchRecursive); require(found.size()==1,"Eclipse resource tree entry missing"); tree->scrollToItem(found.first()); events(); reveal(tree); const auto point=tree->visualItemRect(found.first()).center(); QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,point); events(); QTest::mouseDClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,point); events(); child=area->currentSubWindow(); panel=child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr; require(panel,"Actual Eclipse resource editor did not open");
  };
  int cancels=0,invalids=0;
  auto configure=[&](bool cancel) {
   auto *form=panel->findChild<QDialog *>("eventLocatorDialog"); require(form&&form->isVisible(),"Visible Eclipse form missing");
   auto check=[&](const QString &name,bool wanted) { auto *box=form->findChild<QCheckBox *>("event_"+name); reveal(box); if (box->isChecked()!=wanted) { QStyleOptionButton option; option.initFrom(box); const auto rect=box->style()->subElementRect(QStyle::SE_CheckBoxIndicator,&option,box); QTest::mouseClick(box,Qt::LeftButton,Qt::NoModifier,rect.center()); events(); } require(box->isChecked()==wanted,"Eclipse checkbox did not retain selection"); acceptanceView(*box); };
   auto combo=[&](const QString &name,const QString &value) { auto *box=form->findChild<QComboBox *>("event_"+name); reveal(box); const int index=box->findText(value); require(index>=0,"Eclipse choice missing"); if (box->currentIndex()!=index) { QTest::mouseClick(box,Qt::LeftButton,Qt::NoModifier,QPoint(box->width()-10,box->height()/2)); events(); require(box->view()->isVisible(),"Eclipse choice popup did not open"); const auto modelIndex=box->model()->index(index,0); box->view()->scrollTo(modelIndex); events(); acceptanceView(*box->view()->window()); QTest::keyClick(box->view(),Qt::Key_Home); for (int row=0;row<index;++row) QTest::keyClick(box->view(),Qt::Key_Down); QTest::keyClick(box->view(),Qt::Key_Return); events(); } require(box->currentText()==value,"Eclipse choice did not commit"); };
   auto list=[&](const QString &name,const QStringList &wanted) { auto *items=form->findChild<QListWidget *>("event_"+name); reveal(items); QStringList selected; for (int i=0;i<items->count();++i) { auto *item=items->item(i); const auto state=wanted.contains(item->text()) ? Qt::Checked : Qt::Unchecked; if (item->checkState()!=state) { items->scrollToItem(item); items->setCurrentItem(item); events(); QTest::mouseClick(items->viewport(),Qt::LeftButton,Qt::NoModifier,items->visualItemRect(item).center()); if (item->checkState()!=state) QTest::keyClick(items,Qt::Key_Space); events(); } require(item->checkState()==state,"Eclipse list checkbox did not commit"); if (item->checkState()==Qt::Checked) selected.append(item->text()); } require(QSet<QString>(selected.begin(),selected.end())==QSet<QString>(wanted.begin(),wanted.end()),"Eclipse list lost an entry"); acceptanceView(*items); };
   auto line=[&](const QString &name,const QString &value) { auto *edit=form->findChild<QLineEdit *>("event_"+name); reveal(edit); edit->setText(value); events(); acceptanceView(*edit); };
   combo("Spacecraft",target); list("OccultingBodies",{"Earth"}); QStringList types; for (auto t:spec["types"].toArray()) types.append(t.toString()); list("EclipseTypes",types);
   check("UseLightTimeDelay",false); auto *stellar=form->findChild<QCheckBox *>("event_UseStellarAberration"); require(stellar&&!stellar->isChecked()&&!stellar->isEnabled(),"Disabling light time did not clear/disable stellar aberration"); check("UseLightTimeDelay",true); require(stellar->isEnabled(),"Reception correction did not enable stellar option"); check("UseStellarAberration",spec["use_stellar_aberration"].toBool());
   check("UseEntireInterval",!spec["bounded"].toBool()); if (spec["bounded"].toBool()) { combo("InputEpochFormat","UTCGregorian"); line("InitialEpoch","01 Jan 2000 12:41:40.000"); line("FinalEpoch","01 Jan 2000 12:58:20.000"); }
   combo("RunMode","Automatic"); check("WriteReport",spec["write_report"].toBool()); line("Filename",report); line("StepSize","5");
   auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");
   if (!cancel&&key=="AllTypes") { line("StepSize","0"); click(buttons,QDialogButtonBox::Apply); require(child&&child->isVisible()&&pristine()&&panel->hasChanges()&&!panel->findChild<QLabel *>("resourceStatus")->text().isEmpty(),"Invalid Eclipse search step changed source/model/history or lost diagnostic"); ++invalids; line("StepSize","5"); }
   require(pristine()&&panel->hasChanges(),"Eclipse edit escaped pending isolation"); acceptanceView(*child);
   if (cancel) {
    auto decision=[&](QMessageBox::StandardButton wanted) { std::exception_ptr failure; QTimer response; response.setInterval(10); QObject::connect(&response,&QTimer::timeout,&window,[&] { auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); if (!box) return; response.stop(); try { acceptanceView(*box); auto *button=box->button(wanted); require(button,"Eclipse close decision missing"); QTest::mouseClick(button,Qt::LeftButton); } catch (...) { failure=std::current_exception(); box->reject(); } }); response.start(); click(buttons,QDialogButtonBox::Close); response.stop(); if (failure) std::rethrow_exception(failure); };
    decision(QMessageBox::Cancel); require(child&&child->isVisible()&&pristine()&&panel->hasChanges(),"Eclipse Close Cancel lost draft"); ++cancels; decision(QMessageBox::Discard); require(!child&&pristine(),"Eclipse draft discard changed source/model/history");
   }
  };
  open(); configure(true); open(); configure(false); click(panel->findChild<QDialogButtonBox *>("resourceButtons"),QDialogButtonBox::Apply); const auto applied=source->toPlainText(); require(applied!=initial&&applied.endsWith(tail),"Eclipse Apply changed mission tail or omitted its edits");
  auto *configured=Moderator::Instance()->GetConfiguredObject("Events"); require(configured->GetStringParameter("Spacecraft")==target.toStdString()&&configured->GetBooleanParameter("UseLightTimeDelay")&&configured->GetBooleanParameter("UseStellarAberration")==spec["use_stellar_aberration"].toBool()&&configured->GetBooleanParameter("WriteReport")==spec["write_report"].toBool(),"Configured Eclipse properties disagree with visible controls");
  const int appliedUndo=source->document()->availableUndoSteps(); auto *cleanApply=child->widget()->findChild<QDialogButtonBox *>("resourceButtons")->button(QDialogButtonBox::Apply); require(cleanApply&&cleanApply->isVisible()&&!cleanApply->isEnabled(),"Clean Eclipse Apply should be visibly disabled"); acceptanceView(*child); QTest::mouseClick(cleanApply,Qt::LeftButton); events(); require(source->toPlainText()==applied&&source->document()->availableUndoSteps()==appliedUndo,"Repeated Eclipse Apply changed history"); child->close(); events(); source->undo(); require(source->toPlainText()==initial&&window.buildScript(),"Eclipse Undo did not restore exact source"); source->redo(); require(source->toPlainText()==applied&&window.buildScript(),"Eclipse Redo failed");
  require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==applied,"Eclipse Unicode reopen failed"); save("mission.script",applied.toUtf8()); messages->clear(); const auto result=window.runMission(); events(); save("messages.txt",messages->toPlainText().toUtf8()); if (result!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString(); require(result==MainWindow::RunResult::Completed,"Eclipse mission failed");
  const auto finalBytes=read(truth); save("final.csv",finalBytes); const auto final=QString::fromUtf8(finalBytes).trimmed().split(','); require(final.size()==7&&final.last().trimmed()=="9","Eclipse final witness/sentinel invalid"); const auto finalExpected=spec["final_state"].toArray(); for (int i=0;i<6;++i) { bool ok=false; const double value=final[i].toDouble(&ok); require(ok&&std::isfinite(value)&&std::abs(value-finalExpected[i].toDouble())<(i<3 ? 2e-5 : 2e-8),"Eclipse final state differs from independent circular orbit"); }
  const auto bytes=read(report); save("events.txt",bytes); auto text=QString::fromUtf8(bytes); QJsonArray actual;
  const auto baseline=QDateTime::fromString("01 Jan 2000 12:00:00.000","dd MMM yyyy HH:mm:ss.zzz");
  auto seconds=[&](const QString &stamp) { const auto time=QDateTime::fromString(stamp,"dd MMM yyyy HH:mm:ss.zzz"); require(time.isValid(),"Eclipse report UTC epoch invalid"); return baseline.msecsTo(time)/1000.; };
  if (spec["write_report"].toBool()) {
   require(text.startsWith("Spacecraft: "+target+"\n")&&!text.contains("KEEP prior report"),"Eclipse report target/header or freshness wrong");
   const QString stamp="(\\d{2} [A-Za-z]{3} \\d{4} \\d{2}:\\d{2}:\\d{2}\\.\\d{3})";
   const QRegularExpression pattern("^"+stamp+"\\s+"+stamp+"\\s+([\\deE.+-]+)\\s+(\\S+)\\s+(Umbra|Penumbra|Antumbra)\\s+(\\d+)\\s+([\\deE.+-]+)\\s*$",QRegularExpression::MultilineOption);
   auto matches=pattern.globalMatch(text); while (matches.hasNext()) { auto m=matches.next(); actual.append(QJsonObject{{"start",seconds(m.captured(1))},{"end",seconds(m.captured(2))},{"duration",m.captured(3).toDouble()},{"body",m.captured(4)},{"type",m.captured(5)},{"group",m.captured(6).toInt()},{"total_duration",m.captured(7).toDouble()}}); }
   const auto expected=spec["events"].toArray(); require(actual.size()==expected.size(),"Eclipse report event count differs from independent geometry");
   for (int i=0;i<actual.size();++i) { const auto a=actual[i].toObject(),e=expected[i].toObject(); require(a["body"]==e["body"]&&a["type"]==e["type"]&&a["group"]==e["group"],"Eclipse participant/type/total grouping differs from oracle"); for (const auto &name:QStringList{"start","end"}) require(std::abs(a[name].toDouble()-e[name].toDouble())<spec["time_tolerance"].toDouble(),"Eclipse UTC event boundary differs from independent reception-light-time oracle"); for (const auto &name:QStringList{"duration","total_duration"}) require(std::abs(a[name].toDouble()-e[name].toDouble())<spec["duration_tolerance"].toDouble(),"Eclipse duration differs from independent geometry"); require(std::abs(a["duration"].toDouble()-(a["end"].toDouble()-a["start"].toDouble()))<.002,"Eclipse duration disagrees with its reported UTC endpoints"); }
   if (expected.isEmpty()) require(text.contains("There are no eclipse events")&&!text.contains("Number of individual events"),"No-eclipse report fabricated or retained event summaries");
   else {
    auto number=[&](const QString &label) { auto match=QRegularExpression(QRegularExpression::escape(label)+"\\s*:\\s*([\\deE.+-]+)").match(text); require(match.hasMatch(),"Eclipse summary field missing"); return match.captured(1).toDouble(); };
    require(number("Number of individual events")==expected.size()&&number("Number of total events")==spec["total_events"].toInt()&&std::abs(number("Maximum duration (s)")-spec["maximum_duration"].toDouble())<spec["duration_tolerance"].toDouble(),"Eclipse report totals/maximum differ from independent event union"); auto maximum=QRegularExpression("Maximum duration at the (\\d+)(?:st|nd|rd|th) eclipse").match(text); require(maximum.hasMatch()&&maximum.captured(1).toInt()==spec["maximum_group"].toInt(),"Eclipse maximum identifies the wrong total event");
   }
   const int summary=text.indexOf("Eclipse type summary (individual intervals)"); require(summary>=0,"Per-type Eclipse summary missing");
   const QRegularExpression row("^(Umbra|Penumbra|Antumbra)\\s+(\\d+)\\s+([\\deE.+-]+)\\s+(\\S+)\\s*$",QRegularExpression::MultilineOption);
   QJsonObject byType; auto typeRows=row.globalMatch(text.mid(summary)); while (typeRows.hasNext()) { const auto m=typeRows.next(); require(!byType.contains(m.captured(1)),"Duplicate per-type Eclipse summary"); byType[m.captured(1)]=QJsonObject{{"count",m.captured(2).toInt()},{"maximum",m.captured(3).toDouble()},{"participants",m.captured(4)}}; }
   const auto expectedTypes=spec["per_type"].toObject(); require(byType.size()==expectedTypes.size(),"Per-type Eclipse summary omitted or invented a selected type");
   for (auto it=expectedTypes.begin();it!=expectedTypes.end();++it) { const auto e=it.value().toObject(),a=byType[it.key()].toObject(); QStringList participants; for (auto p:e["participants"].toArray()) participants.append(p.toString()); require(a["count"]==e["count"]&&std::abs(a["maximum"].toDouble()-e["maximum"].toDouble())<spec["duration_tolerance"].toDouble()&&a["participants"].toString()==(participants.isEmpty() ? "-" : participants.join(',')),"Per-type Eclipse counts/maximum/participants differ from independent intervals"); }
   save("type-summary.json",QJsonDocument(byType).toJson());
  } else require(bytes==sentinel,"WriteReport off replaced an existing report");
  const auto uncorrected=spec["uncorrected_events"].toArray(); require(actual.size()==uncorrected.size(),"Light-time control interval membership changed unexpectedly"); int sensitivityChecks=0;
  for (int i=0;i<actual.size();++i) for (const auto &name:QStringList{"start","end"}) { const double prior=uncorrected[i].toObject()[name].toDouble(),value=actual[i].toObject()[name].toDouble(); if (prior!=0.&&prior!=spec["duration"].toDouble()) { require(std::abs(value-prior)>5*spec["time_tolerance"].toDouble(),"Eclipse output is indistinguishable from ignoring light time"); ++sensitivityChecks; } else require(value==prior,"Eclipse light-time search altered confinement boundary"); }
  require(sensitivityChecks>0,"Light-time case has no discriminating event root");
  save("observations.json",QJsonDocument(actual).toJson());
  auto *output=window.findChild<QTreeWidget *>("Output"); for (auto *tabs:window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==output||tabs->widget(i)->isAncestorOf(output)) tabs->setCurrentIndex(i); const auto found=output->findItems("Events",Qt::MatchExactly|Qt::MatchRecursive); require(found.size()==1,"Eclipse Output entry missing"); output->scrollToItem(found.first()); events(); reveal(output); const auto point=output->visualItemRect(found.first()).center(); QTest::mouseClick(output->viewport(),Qt::LeftButton,Qt::NoModifier,point); events(); QTest::mouseDClick(output->viewport(),Qt::LeftButton,Qt::NoModifier,point); events(); auto *viewer=area->currentSubWindow() ? area->currentSubWindow()->widget()->findChild<QPlainTextEdit *>("report:Events") : nullptr; if (!viewer&&area->currentSubWindow()) viewer=qobject_cast<QPlainTextEdit *>(area->currentSubWindow()->widget()); require(viewer&&viewer->isReadOnly(),"Eclipse Output viewer missing or editable"); if (spec["write_report"].toBool()) require(viewer->toPlainText()==text.replace("\r\n","\n"),"Eclipse viewer does not contain complete fresh report"); else require(viewer->toPlainText().contains("No event report was written")&&!viewer->toPlainText().contains("KEEP prior report"),"Disabled Eclipse viewer exposed stale report"); acceptanceView(*area->currentSubWindow());
  const QJsonArray facets{"actual-embedded-eclipse-resource-controls","typed-target-body-and-shadow-selection","pending-close-cancel-discard-history-unicode","converged-reception-delay-and-stellar-toggle-dependency","independent-retarded-barycentric-limb-roots-and-durations","complete-report-or-disabled-file-and-stale-viewer-protection"}; const QJsonObject proof{{"checkpoint","eclipse-light-time-"+key},{"outcome","passed"},{"family",key},{"target",target},{"events",actual},{"write_report",spec["write_report"]},{"cancels",cancels},{"invalid_rejections",invalids},{"numeric_sentinel",9},{"light_time",true},{"stellar_aberration",spec["use_stellar_aberration"]},{"sensitivity_checks",sensitivityChecks},{"facets",facets},{"scope",fixture["scope"]}};
  save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; } catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
