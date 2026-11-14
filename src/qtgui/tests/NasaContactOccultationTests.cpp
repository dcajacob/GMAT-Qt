// Selectable third-body occultation cases reuse the contact-result workflow.
#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include "BodyFixedPoint.hpp"
#include "A1Mjd.hpp"
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
static QByteArray read(const QString &path) { QFile f(path); if (!f.open(QIODevice::ReadOnly)) throw std::runtime_error(("Contact result artifact missing: "+path).toStdString()); return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Contact result artifact write failed"); }
static QString digest(const QString &path) { return QString::fromLatin1(QCryptographicHash::hash(read(path),QCryptographicHash::Sha256).toHex()); }
static void reveal(QWidget *w) { require(w,"Contact control missing"); for (QWidget *p=w;p;p=p->parentWidget()) if (auto *scroll=qobject_cast<QScrollArea *>(p)) scroll->ensureWidgetVisible(w); events(); require(w->isVisible()&&w->isEnabled(),"Contact control unreachable"); acceptanceView(*w); }
static void click(QDialogButtonBox *buttons,QDialogButtonBox::StandardButton which) { auto *button=buttons ? buttons->button(which) : nullptr; reveal(button); QTest::mouseClick(button,Qt::LeftButton); events(); }

int main(int argc,char **argv)
{
 QApplication app(argc,argv); QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QLocale::setDefault(QLocale::c());
 app.setOrganizationName("GMATTests"); app.setApplicationName("NasaContactOccultation");
 try {
  require(argc==4&&acceptanceWidgetPlatform(),"Expected startup, contact cases and key");
  const QString key=argv[3],startup=QFileInfo(argv[1]).absoluteFilePath(); const auto fixture=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object(); QJsonObject spec;
  for (auto c:fixture["cases"].toArray()) if (c.toObject()["key"]==key) spec=c.toObject(); require(!spec.isEmpty(),"Contact result case missing");
  QDir::setCurrent(QFileInfo(startup).absolutePath()); const auto originalStartup=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_STARTUP_ORIGINAL",startup); const auto root=QDir::cleanPath(QDir(QFileInfo(originalStartup).absolutePath()).absoluteFilePath("../.."));
  const auto kernels=fixture["physical_inputs"].toObject()["kernels"].toObject();
  for (auto it=kernels.begin();it!=kernels.end();++it) require(digest(QDir(root).filePath(it.key()))==it.value().toString(),"Runtime contact physical input differs from independent oracle kernel");
  TestSettings settings; QTemporaryDir files; require(files.isValid(),"Contact fixture directory unavailable");
  const auto report=files.filePath("Contact intervals Δ.txt"),truth=files.filePath("Contact final Δ.csv"),saved=files.filePath("Contact Δ.script"); const QByteArray sentinel="KEEP prior report bytes: Δ\n"; write(report,sentinel);
  const auto target=spec["target"].toString(),initialTarget=target=="Sat" ? QString("Other") : QString("Sat");
  QString prefix="% Preserve contact source Δ\nSolarSystem.EphemerisSource = 'SPICE';\nSolarSystem.EphemerisUpdateInterval = 0;\nSolarSystem.SPKFilename = '"+QDir(root).filePath("application/data/planetary_ephem/spk/DE421AllPlanets.bsp")+"';\nCreate Variable Keep;\nKeep = 9;\n";
  prefix+="Create CoordinateSystem MoonIcrf;\nMoonIcrf.Origin = Luna;\nMoonIcrf.Axes = ICRF;\nLuna.Mu = "+QString::number(spec["orbit_mu"].toDouble(),'g',17)+";\nLuna.EquatorialRadius = "+QString::number(fixture["physical_inputs"].toObject()["moon_radii"].toArray()[0].toDouble(),'g',17)+";\nLuna.Flattening = 0;\n";
  const QStringList components{"X","Y","Z","VX","VY","VZ"};
  prefix+="Earth.EquatorialRadius = "+QString::number(fixture["equatorial_radius"].toDouble(),'g',17)+";\nEarth.Flattening = "+QString::number(fixture["flattening"].toDouble(),'g',17)+";\n";
  QStringList selectedObservers; for (auto value:spec["observers"].toArray()) selectedObservers.append(value.toString());
  QStringList selectedOcculters; for (auto value:spec["occulting_bodies"].toArray()) selectedOcculters.append(value.toString());
  for (auto value:fixture["stations"].toArray()) { const auto station=value.toObject(); const auto name=station["name"].toString(); prefix+="Create GroundStation "+name+";\n"+name+".StateType = Spherical;\n"+name+".HorizonReference = Ellipsoid;\n"; int i=1; for (const auto &field:QStringList{"latitude","longitude","height"}) prefix+=name+".Location"+QString::number(i++)+" = "+QString::number(station[field].toDouble(),'g',17)+";\n"; prefix+=name+".MinimumElevationAngle = "+QString::number(station["minimum_elevation"].toDouble(),'g',17)+";\n"; }

  for (const auto &name:QStringList{"Sat","Other"}) {
   const auto state=spec[name==target ? "initial_state" : "other_state"].toArray();
   prefix+="Create Spacecraft "+name+";\n"+name+".DateFormat = UTCGregorian;\n"+name+".Epoch = '01 Jan 2000 12:00:00.000';\n"+name+".CoordinateSystem = MoonIcrf;\n"+name+".DisplayStateType = Cartesian;\n"+name+".NAIFId = "+(name=="Sat" ? "-100070" : "-100071")+";\n";
   for (int i=0;i<6;++i) prefix+=name+"."+components[i]+" = "+QString::number(state[i].toDouble(),'g',17)+";\n";
  }
  prefix+="Create ForceModel FM;\nFM.CentralBody = Luna;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Luna};\nCreate Propagator Prop;\nProp.FM = FM;\nProp.Type = RungeKutta89;\nProp.InitialStepSize = 10;\nProp.MinStep = 10;\nProp.MaxStep = 10;\nProp.Accuracy = 1e-12;\nCreate ContactLocator Events;\nEvents.Target = "+initialTarget+";\nEvents.OccultingBodies = {"+(selectedOcculters.contains("Luna") ? QString("Mars") : QString("Luna"))+"};\nEvents.Observers = {Dark};\nEvents.ReportFormat = Legacy;\nEvents.RunMode = Disabled;\nEvents.UseLightTimeDelay = true;\nEvents.UseStellarAberration = true;\nEvents.WriteReport = "+(spec["write_report"].toBool() ? "false" : "true")+";\nEvents.Filename = '"+files.filePath("inactive report.txt")+"';\nEvents.StepSize = 60;\nCreate ReportFile Truth;\nTruth.Filename = '"+truth+"';\nTruth.WriteHeaders = false;\nTruth.FixedWidth = false;\nTruth.Delimiter = ',';\nTruth.Precision = 17;\n";
  QString tail="BeginMissionSequence;\nPropagate 'Prescribed circles' Prop(Sat,Other) {Sat.ElapsedSecs = "+QString::number(spec["duration"].toDouble(),'g',17)+"};\nReport 'Final witness' Truth";
  for (const auto &component:components) tail+=' '+target+".MoonIcrf."+component;
  tail+=" Keep; % preserve final Ω\n"; const auto initial=prefix+tail;
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"); auto save=[&](const QString &name,const QByteArray &bytes) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes); };
  MainWindow window; require(window.initialize(startup),"Contact runtime initialization failed"); window.show(); acceptanceView(window); events();
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow"); auto *area=window.findChild<QMdiArea *>("workspace"); require(source&&messages&&area,"Contact workspace missing");
  save("initial.script",initial.toUtf8()); source->setPlainText(initial); const bool built=window.buildScript(); events(); save("initial-messages.txt",messages->toPlainText().toUtf8()); if (!built) std::cerr<<messages->toPlainText().toStdString(); require(built,"Contact initial build failed"); source->document()->setModified(false); const int undo=source->document()->availableUndoSteps(); save("initial.script",initial.toUtf8());
  auto *earth=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth"); require(std::abs(earth->GetGravitationalConstant()-spec["mu"].toDouble())<1e-9&&earth->GetStringParameter(earth->GetParameterID("SpiceFrameId"))=="ITRF93","Contact gravitational/frame input differs from oracle");
  auto *moon=Moderator::Instance()->GetSolarSystemInUse()->GetBody("Luna"); require(moon&&std::abs(moon->GetGravitationalConstant()-spec["orbit_mu"].toDouble())<1e-10&&std::abs(moon->GetEquatorialRadius()-fixture["physical_inputs"].toObject()["moon_radii"].toArray()[0].toDouble())<1e-10,"Native lunar orbit/shape inputs differ from independent oracle");
  require(digest(QString::fromStdString(FileManager::Instance()->GetFullPathname(FileManager::ICRF_FILE)))==kernels["application/data/icrf/ICRF_Table.txt"].toString(),"Native FK5/ICRF frame table differs from independent input");
  const auto pck=QString::fromStdString(FileManager::Instance()->GetFullPathname(FileManager::PLANETARY_PCK_FILE)); require(digest(pck)==kernels["application/data/planetary_coeff/SPICEPlanetaryConstantsKernel.tpc"].toString(),"Event manager PCK input differs from oracle");
  auto pristine=[&] { auto *object=Moderator::Instance()->GetConfiguredObject("Events"); return source->toPlainText()==initial&&source->document()->availableUndoSteps()==undo&&!source->document()->isModified()&&object&&object->GetStringParameter("RunMode")=="Disabled"&&object->GetStringParameter("Target")==initialTarget.toStdString()&&object->GetBooleanParameter("UseLightTimeDelay")&&object->GetBooleanParameter("UseStellarAberration"); };
  QPointer<QMdiSubWindow> child; ResourceEditor *panel=nullptr;
  auto open=[&] {
   auto *tree=window.findChild<QTreeWidget *>("Resources"); for (auto *tabs:window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==tree||tabs->widget(i)->isAncestorOf(tree)) tabs->setCurrentIndex(i);
   const auto found=tree->findItems("Events",Qt::MatchExactly|Qt::MatchRecursive); require(found.size()==1,"Contact resource tree entry missing"); tree->scrollToItem(found.first()); events(); reveal(tree); const auto point=tree->visualItemRect(found.first()).center(); QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,point); events(); QTest::mouseDClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,point); events(); child=area->currentSubWindow(); panel=child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr; require(panel,"Actual Contact resource editor did not open");
  };
  int cancels=0,invalids=0;
  auto configure=[&](bool cancel) {
   auto *form=panel->findChild<QDialog *>("eventLocatorDialog"); require(form&&form->isVisible(),"Visible Contact form missing");
   auto check=[&](const QString &name,bool wanted) { auto *box=form->findChild<QCheckBox *>("event_"+name); reveal(box); if (box->isChecked()!=wanted) { QStyleOptionButton option; option.initFrom(box); const auto rect=box->style()->subElementRect(QStyle::SE_CheckBoxIndicator,&option,box); QTest::mouseClick(box,Qt::LeftButton,Qt::NoModifier,rect.center()); events(); } require(box->isChecked()==wanted,"Contact checkbox did not retain selection"); acceptanceView(*box); };
   auto combo=[&](const QString &name,const QString &value) { auto *box=form->findChild<QComboBox *>("event_"+name); reveal(box); const int index=box->findText(value); require(index>=0,"Contact choice missing"); if (box->currentIndex()!=index) { QTest::mouseClick(box,Qt::LeftButton,Qt::NoModifier,QPoint(box->width()-10,box->height()/2)); events(); require(box->view()->isVisible(),"Contact choice popup did not open"); const auto modelIndex=box->model()->index(index,0); box->view()->scrollTo(modelIndex); events(); acceptanceView(*box->view()->window()); QTest::keyClick(box->view(),Qt::Key_Home); for (int row=0;row<index;++row) QTest::keyClick(box->view(),Qt::Key_Down); QTest::keyClick(box->view(),Qt::Key_Return); events(); } require(box->currentText()==value,"Contact choice did not commit"); };
   auto list=[&](const QString &name,const QStringList &wanted) { auto *items=form->findChild<QListWidget *>("event_"+name); reveal(items); QStringList selected; for (int i=0;i<items->count();++i) { auto *item=items->item(i); const auto state=wanted.contains(item->text()) ? Qt::Checked : Qt::Unchecked; if (item->checkState()!=state) { items->scrollToItem(item); items->setCurrentItem(item); events(); QTest::mouseClick(items->viewport(),Qt::LeftButton,Qt::NoModifier,items->visualItemRect(item).center()); if (item->checkState()!=state) QTest::keyClick(items,Qt::Key_Space); events(); } require(item->checkState()==state,"Contact list checkbox did not commit"); if (item->checkState()==Qt::Checked) selected.append(item->text()); } require(QSet<QString>(selected.begin(),selected.end())==QSet<QString>(wanted.begin(),wanted.end()),"Contact list lost an entry"); acceptanceView(*items); };
   auto line=[&](const QString &name,const QString &value) { auto *edit=form->findChild<QLineEdit *>("event_"+name); reveal(edit); edit->setText(value); events(); acceptanceView(*edit); };
   combo("Target",target); list("OccultingBodies",selectedOcculters); list("Observers",selectedObservers); combo("ReportFormat","Legacy");
   check("UseLightTimeDelay",false); auto *stellar=form->findChild<QCheckBox *>("event_UseStellarAberration"); require(stellar&&!stellar->isChecked()&&!stellar->isEnabled(),"Disabling light time did not clear/disable stellar aberration");
   check("UseEntireInterval",!spec["bounded"].toBool()); if (spec["bounded"].toBool()) { combo("InputEpochFormat","UTCGregorian"); line("InitialEpoch","01 Jan 2000 12:13:20.000"); line("FinalEpoch","01 Jan 2000 12:53:20.000"); }
   combo("RunMode","Automatic"); check("WriteReport",spec["write_report"].toBool()); line("Filename",report); line("StepSize","5");
   auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");
   if (!cancel&&key=="Luna") { line("StepSize","0"); click(buttons,QDialogButtonBox::Apply); require(child&&child->isVisible()&&pristine()&&panel->hasChanges()&&!panel->findChild<QLabel *>("resourceStatus")->text().isEmpty(),"Invalid Contact search step changed source/model/history or lost diagnostic"); ++invalids; line("StepSize","5"); }
   require(pristine()&&panel->hasChanges(),"Contact edit escaped pending isolation"); acceptanceView(*child);
   if (cancel) {
    auto decision=[&](QMessageBox::StandardButton wanted) { std::exception_ptr failure; QTimer response; response.setInterval(10); QObject::connect(&response,&QTimer::timeout,&window,[&] { auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); if (!box) return; response.stop(); try { acceptanceView(*box); auto *button=box->button(wanted); require(button,"Contact close decision missing"); QTest::mouseClick(button,Qt::LeftButton); } catch (...) { failure=std::current_exception(); box->reject(); } }); response.start(); click(buttons,QDialogButtonBox::Close); response.stop(); if (failure) std::rethrow_exception(failure); };
    decision(QMessageBox::Cancel); require(child&&child->isVisible()&&pristine()&&panel->hasChanges(),"Contact Close Cancel lost draft"); ++cancels; decision(QMessageBox::Discard); require(!child&&pristine(),"Contact draft discard changed source/model/history");
   }
  };
  open(); configure(true); open(); configure(false); click(panel->findChild<QDialogButtonBox *>("resourceButtons"),QDialogButtonBox::Apply); const auto applied=source->toPlainText(); require(applied!=initial&&applied.endsWith(tail),"Contact Apply changed mission tail or omitted its edits");
  auto *configured=Moderator::Instance()->GetConfiguredObject("Events"); require(configured->GetStringParameter("Target")==target.toStdString()&&!configured->GetBooleanParameter("UseLightTimeDelay")&&!configured->GetBooleanParameter("UseStellarAberration")&&configured->GetBooleanParameter("WriteReport")==spec["write_report"].toBool(),"Configured Contact properties disagree with visible controls");
  const auto actualOcculters=configured->GetStringArrayParameter("OccultingBodies"); QStringList nativeOcculters; for (const auto &body:actualOcculters) nativeOcculters.append(QString::fromStdString(body)); require(QSet<QString>(nativeOcculters.begin(),nativeOcculters.end())==QSet<QString>(selectedOcculters.begin(),selectedOcculters.end()),"Native occulting-body selection differs from visible controls");
  const int appliedUndo=source->document()->availableUndoSteps(); auto *cleanApply=child->widget()->findChild<QDialogButtonBox *>("resourceButtons")->button(QDialogButtonBox::Apply); require(cleanApply&&cleanApply->isVisible()&&!cleanApply->isEnabled(),"Clean Contact Apply should be visibly disabled"); acceptanceView(*child); QTest::mouseClick(cleanApply,Qt::LeftButton); events(); require(source->toPlainText()==applied&&source->document()->availableUndoSteps()==appliedUndo,"Repeated Contact Apply changed history"); child->close(); events(); source->undo(); require(source->toPlainText()==initial&&window.buildScript(),"Contact Undo did not restore exact source"); source->redo(); require(source->toPlainText()==applied&&window.buildScript(),"Contact Redo failed");
  require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==applied,"Contact Unicode reopen failed"); save("mission.script",applied.toUtf8()); messages->clear(); const auto result=window.runMission(); events(); save("messages.txt",messages->toPlainText().toUtf8()); if (result!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString(); require(result==MainWindow::RunResult::Completed,"Contact mission failed");
  const auto finalBytes=read(truth); save("final.csv",finalBytes); const auto final=QString::fromUtf8(finalBytes).trimmed().split(','); require(final.size()==7&&final.last().trimmed()=="9","Contact final witness/sentinel invalid"); const auto finalExpected=spec["final_state"].toArray(); for (int i=0;i<6;++i) { bool ok=false; const double value=final[i].toDouble(&ok); require(ok&&std::isfinite(value)&&std::abs(value-finalExpected[i].toDouble())<(i<3 ? 2e-5 : 2e-8),"Contact final state differs from independent circular orbit"); }
  QJsonObject nativeStations;
  for (auto value:fixture["stations"].toArray()) { const auto expected=value.toObject(); const auto name=expected["name"].toString(); auto *station=dynamic_cast<BodyFixedPoint *>(Moderator::Instance()->GetInternalObject(name.toStdString())); require(station,"Running contact station missing"); const auto position=station->GetBodyFixedLocation(A1Mjd(21545)); const auto wanted=expected["fixed_position"].toArray(); QJsonArray actual; for (int i=0;i<3;++i) { require(std::isfinite(position[i])&&std::abs(position[i]-wanted[i].toDouble())<1e-8,"Contact station differs from independent geodetic position"); actual.append(position[i]); } require(station->GetRealParameter("MinimumElevationAngle")==expected["minimum_elevation"].toDouble(),"Contact station elevation threshold changed"); nativeStations[name]=actual; }
  save("native-stations.json",QJsonDocument(nativeStations).toJson());
  const auto bytes=read(report); save("events.txt",bytes); auto text=QString::fromUtf8(bytes); QJsonObject actual;
  require(text.startsWith("Target: "+target+"\n")&&!text.contains("KEEP prior report"),"Contact report target/header or freshness wrong");
  const auto baseline=QDateTime::fromString("01 Jan 2000 12:00:00.000","dd MMM yyyy HH:mm:ss.zzz");
  auto seconds=[&](const QString &stamp) { const auto time=QDateTime::fromString(stamp,"dd MMM yyyy HH:mm:ss.zzz"); require(time.isValid(),"Contact UTC report epoch invalid"); return baseline.msecsTo(time)/1000.; };
  const QString stamp="(\\d{2} [A-Za-z]{3} \\d{4} \\d{2}:\\d{2}:\\d{2}\\.\\d{3})";
  const QRegularExpression rows("^"+stamp+"\\s+"+stamp+"\\s+([\\deE.+-]+)\\s*$",QRegularExpression::MultilineOption);
  const auto sections=text.split("Observer: "); require(sections.size()==selectedObservers.size()+1,"Contact report observer membership/count differs from selection");
  int totalIntervals=0;
  for (int i=1;i<sections.size();++i) { const auto section=sections[i]; const auto observer=section.section('\n',0,0).trimmed(); require(selectedObservers.contains(observer)&&!actual.contains(observer),"Contact report contains an unselected/duplicate observer"); QJsonArray found; auto matches=rows.globalMatch(section); while(matches.hasNext()) { const auto m=matches.next(); found.append(QJsonObject{{"start",seconds(m.captured(1))},{"end",seconds(m.captured(2))},{"duration",m.captured(3).toDouble()}}); }
   const auto expected=spec["events"].toObject()[observer].toArray(); require(found.size()==expected.size(),"Contact report interval count differs from independent elevation roots");
   for (int j=0;j<found.size();++j) { const auto a=found[j].toObject(),e=expected[j].toObject(); for (const auto &name:QStringList{"start","end"}) require(std::abs(a[name].toDouble()-e[name].toDouble())<spec["time_tolerance"].toDouble(),"Contact UTC boundary differs from independent elevation-root oracle"); require(std::abs(a["duration"].toDouble()-e["duration"].toDouble())<spec["duration_tolerance"].toDouble(),"Contact duration differs from independent elevation roots"); require(std::abs(a["duration"].toDouble()-(a["end"].toDouble()-a["start"].toDouble()))<.002,"Contact duration disagrees with its printed UTC boundaries"); }
   const auto count=QRegularExpression("Number of events\\s*:\\s*(\\d+)").match(section); require(count.hasMatch()&&count.captured(1).toInt()==expected.size(),"Contact section summary count wrong"); if (expected.isEmpty()) require(section.contains("There are no contact events"),"Selected no-contact observer lost explicit absence report"); totalIntervals+=found.size(); actual[observer]=found;
  }
  save("observations.json",QJsonDocument(actual).toJson());
  auto *output=window.findChild<QTreeWidget *>("Output"); for (auto *tabs:window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==output||tabs->widget(i)->isAncestorOf(output)) tabs->setCurrentIndex(i); const auto found=output->findItems("Events",Qt::MatchExactly|Qt::MatchRecursive); require(found.size()==1,"Contact Output entry missing"); output->scrollToItem(found.first()); events(); reveal(output); const auto point=output->visualItemRect(found.first()).center(); QTest::mouseClick(output->viewport(),Qt::LeftButton,Qt::NoModifier,point); events(); QTest::mouseDClick(output->viewport(),Qt::LeftButton,Qt::NoModifier,point); events(); auto *viewer=area->currentSubWindow() ? area->currentSubWindow()->widget()->findChild<QPlainTextEdit *>("report:Events") : nullptr; if (!viewer&&area->currentSubWindow()) viewer=qobject_cast<QPlainTextEdit *>(area->currentSubWindow()->widget()); require(viewer&&viewer->isReadOnly(),"Contact Output viewer missing or editable"); if (spec["write_report"].toBool()) require(viewer->toPlainText()==text.replace("\r\n","\n"),"Contact viewer does not contain complete fresh report"); else require(viewer->toPlainText().contains("No event report was written")&&!viewer->toPlainText().contains("KEEP prior report"),"Disabled Contact viewer exposed stale report"); acceptanceView(*area->currentSubWindow());
  const QJsonArray facets{"actual-embedded-contact-resource-controls","selected-multiple-observers-and-unselected-isolation","pending-close-cancel-discard-history-unicode","independent-geodetic-elevation-rise-set-roots","selected-no-contact-membership","complete-raw-report-summaries-and-output-viewer","selected-third-body-finite-segment-suppression","unselected-body-and-multiple-selection-isolation"};
  const QJsonObject proof{{"checkpoint","contact-occultation-"+key},{"outcome","passed"},{"family",key},{"target",target},{"occulting_bodies",QJsonArray::fromStringList(nativeOcculters)},{"orbit_frame","MoonIcrf"},{"duration",spec["duration"]},{"orbit_mu",spec["orbit_mu"]},{"events",actual},{"total_intervals",totalIntervals},{"observer_count",selectedObservers.size()},{"cancels",cancels},{"invalid_rejections",invalids},{"numeric_sentinel",9},{"native_stations",nativeStations},{"facets",facets},{"scope",fixture["scope"]}};
  save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; } catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
