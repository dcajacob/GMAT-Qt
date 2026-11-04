#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "CommandEditor.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDialog>
#include <QTextDocument>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMdiArea>
#include <QMessageBox>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QStyle>
#include <QStyleOptionButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <memory>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Ephemeris sampling artifact missing"); return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Ephemeris sampling artifact write failed"); }
static void reveal(QWidget *w) { require(w,"Ephemeris sampling control missing"); for (QWidget *p=w;p;p=p->parentWidget()) if (auto *scroll=qobject_cast<QScrollArea *>(p)) scroll->ensureWidgetVisible(w); events(); require(w->isVisible()&&w->isEnabled(),"Ephemeris sampling control unreachable"); acceptanceView(*w); }
static void click(QDialogButtonBox *box,QDialogButtonBox::StandardButton which) { auto *button=box ? box->button(which) : nullptr; reveal(button); QTest::mouseClick(button,Qt::LeftButton); events(); }

int main(int argc,char **argv)
{
 QApplication app(argc,argv); QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QLocale::setDefault(QLocale::c());
 app.setOrganizationName("GMATTests"); app.setApplicationName("NasaOemOutputTime");
 try {
  require(argc==4&&acceptanceWidgetPlatform(),"Expected startup, sampling cases and key");
  const QString key=argv[3],startup=QFileInfo(argv[1]).absoluteFilePath(); QJsonObject spec;
  const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object(); for (auto c:catalog["cases"].toArray()) if (c.toObject()["key"]==key) spec=c.toObject();
  require(!spec.isEmpty(),"Ephemeris sampling case missing"); QDir::setCurrent(QFileInfo(startup).absolutePath()); const QString scenario=spec["scenario"].toString();
  TestSettings settings; QTemporaryDir files; require(files.isValid(),"Ephemeris sampling fixture directory unavailable");
  const auto oem=files.filePath("Every state Δ.oem"),truth=files.filePath("Final state Δ.csv"),saved=files.filePath("Sampling Δ.script");
  const QString initialStep=spec["sampling"]=="IntegratorSteps" ? "20" : "IntegratorSteps";
  QString prefix="% Preserve ephemeris source Δ\nCreate Variable Keep;\nKeep = 9;\nCreate Spacecraft Sat;\nSat.Id = '"+catalog["object_id"].toString()+"';\nSat.DateFormat = UTCGregorian;\nSat.Epoch = '"+spec["epoch_utc"].toString()+"';\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = "+QString::number(spec["speed"].toDouble(),'g',17)+";\nSat.VZ = 0;\nCreate ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\n";
  for (const auto &name:QStringList{"PropA","PropB"}) prefix+="Create Propagator "+name+";\n"+name+".Type = "+(name=="PropA" ? "RungeKutta89" : "PrinceDormand78")+";\n"+name+".FM = FM;\n"+name+".InitialStepSize = 5;\n"+name+".MinStep = 5;\n"+name+".MaxStep = 5;\n"+name+".Accuracy = 1e-12;\n";
  prefix+="Create ImpulsiveBurn Reverse;\nReverse.CoordinateSystem = EarthMJ2000Eq;\nReverse.DecrementMass = false;\n";
  const auto dv=spec["reverse_delta_v"].toArray(); for (int i=0;i<3;++i) prefix+="Reverse.Element"+QString::number(i+1)+" = "+QString::number(dv[i].toDouble(),'g',17)+";\n";
  prefix+="Create EphemerisFile Export;\nExport.Spacecraft = Sat;\nExport.FileFormat = CCSDS-OEM;\nExport.FormatVersion = '1.0';\nExport.Filename = '"+oem+"';\nExport.CoordinateSystem = EarthMJ2000Eq;\nExport.WriteEphemeris = false;\nExport.InterpolationOrder = 7;\nExport.StepSize = "+initialStep+";\nCreate ReportFile Truth;\nTruth.Filename = '"+truth+"';\nTruth.WriteHeaders = false;\nTruth.FixedWidth = false;\nTruth.Delimiter = ',';\nTruth.Precision = 17;\n";
  const QString tail="Report 'Final witness' Truth Sat.ElapsedSecs Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ Keep; % preserve final Ω\n";
  if (spec["retain_raw_samples"].toBool()) prefix+="Create ReportFile RawSamples;\nRawSamples.Filename = '"+files.filePath("Native samples Δ.csv")+"';\nRawSamples.WriteHeaders = false;\nRawSamples.FixedWidth = false;\nRawSamples.Delimiter = ',';\nRawSamples.Precision = 17;\nRawSamples.Add = {Sat.A1ModJulian, Sat.EarthMJ2000Eq.X, Sat.EarthMJ2000Eq.Y, Sat.EarthMJ2000Eq.Z, Sat.EarthMJ2000Eq.VX, Sat.EarthMJ2000Eq.VY, Sat.EarthMJ2000Eq.VZ};\n";
  const QString initial=prefix+"BeginMissionSequence;\n"+spec["mission"].toString()+tail;
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"); auto save=[&](const QString &name,const QByteArray &bytes) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes); };
  MainWindow window; require(window.initialize(startup),"Ephemeris sampling initialization failed"); window.show(); acceptanceView(window); events();
  auto *source=window.findChild<QPlainTextEdit *>("scriptEditor"),*messages=window.findChild<QPlainTextEdit *>("messageWindow"); auto *area=window.findChild<QMdiArea *>("workspace"); require(source&&messages&&area,"Ephemeris sampling workspace missing");
  source->setPlainText(initial); const bool built=window.buildScript(); if (!built) std::cerr<<messages->toPlainText().toStdString(); require(built,"Ephemeris sampling initial build failed"); source->document()->setModified(false); const int undo=source->document()->availableUndoSteps(); save("initial.script",initial.toUtf8());
  require(std::abs(Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth")->GetGravitationalConstant()-spec["mu"].toDouble())<1e-9,"Ephemeris sampling gravitational input differs from literal oracle");
  auto *resource=Moderator::Instance()->GetConfiguredObject("Export");
  auto pristine=[&] { return QString::fromStdString(Moderator::Instance()->GetConfiguredObject("Export")->GetStringParameter("OutputTimeSystem"))=="UTC"&&source->toPlainText()==initial&&source->document()->availableUndoSteps()==undo&&!source->document()->isModified()&&!Moderator::Instance()->GetConfiguredObject("Export")->GetBooleanParameter("WriteEphemeris")&&QString::fromStdString(Moderator::Instance()->GetConfiguredObject("Export")->GetStringParameter("StepSize"))==initialStep; };
  auto *tree=window.findChild<QTreeWidget *>("Resources"); for (auto *tabs:window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==tree||tabs->widget(i)->isAncestorOf(tree)) tabs->setCurrentIndex(i);
  const auto found=tree->findItems("Export",Qt::MatchExactly|Qt::MatchRecursive); require(found.size()==1,"Ephemeris resource tree entry missing"); tree->scrollToItem(found.first()); events(); reveal(tree); QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(found.first()).center()); events(); QTest::mouseDClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(found.first()).center()); events();
  QPointer<QMdiSubWindow> child=area->currentSubWindow(); auto *panel=child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr; require(panel,"Actual resource editor did not open");
  int invalids=0,cancels=0;
  auto configure=[&](bool cancel) {
   auto *dialog=panel->findChild<QDialog *>("ephemerisDialog"); require(dialog&&dialog->isVisible(),"Visible embedded ephemeris form missing"); acceptanceView(*child);
   auto combo=[&](const QString &name,const QString &value) { auto *w=dialog->findChild<QComboBox *>("ephemeris_"+name); reveal(w); w->setCurrentText(value); events(); require(w->currentText()==value,"Ephemeris sampling choice was not retained"); acceptanceView(*w); };
   auto line=[&](const QString &name,const QString &value) { auto *w=dialog->findChild<QLineEdit *>("ephemeris_"+name); reveal(w); w->setText(value); events(); acceptanceView(*w); };
   auto *writeFlag=dialog->findChild<QCheckBox *>("ephemeris_WriteEphemeris"); reveal(writeFlag); QStyleOptionButton option; option.initFrom(writeFlag); const auto indicator=writeFlag->style()->subElementRect(QStyle::SE_CheckBoxIndicator,&option,writeFlag); QTest::mouseClick(writeFlag,Qt::LeftButton,Qt::NoModifier,indicator.center()); require(writeFlag->isChecked(),"Write ephemeris checkbox did not enable");
   combo("Spacecraft","Sat"); combo("CoordinateSystem","EarthMJ2000Eq"); combo("StepSize",spec["sampling"].toString()); line("InterpolationOrder","7");
   auto *method=dialog->findChild<QLineEdit *>("ephemeris_Interpolator"); require(method&&method->isReadOnly()&&method->text()=="Lagrange","OEM GUI does not display its required interpolation method");
   auto *outputClock=dialog->findChild<QComboBox *>("ephemeris_OutputTimeSystem"); require(outputClock,"OEM OutputTimeSystem selector missing");
   require(outputClock->count()==5&&outputClock->findText("A1")>=0&&outputClock->findText("TAI")>=0&&outputClock->findText("UTC")>=0&&outputClock->findText("TT")>=0&&outputClock->findText("TDB")>=0,"OEM output selector omits an original FRR2.2 system");
   combo("OutputTimeSystem",spec["time_system"].toString());
   if (!cancel&&key=="TAI_TTInputWindow") { combo("FileFormat","SPK"); require(!outputClock->isEnabled(),"Non-OEM format leaves output-time selector enabled"); combo("FileFormat","CCSDS-OEM"); require(outputClock->isEnabled()&&outputClock->currentText()==spec["time_system"].toString(),"Format switch lost pending OEM output time"); }
   std::unique_ptr<GmatBase> badTime(Moderator::Instance()->GetConfiguredObject("Export")->Clone()); const auto beforeTime=badTime->GetStringParameter("OutputTimeSystem"); bool rejectedTime=false; try { badTime->SetStringParameter("OutputTimeSystem","NasaBadClock"); } catch (BaseException &) { rejectedTime=true; } require(rejectedTime&&badTime->GetStringParameter("OutputTimeSystem")==beforeTime,"Invalid output-time assignment mutated native configuration");
   combo("EpochFormat",spec["input_epoch_format"].toString()); combo("InitialEpoch",spec["initial_epoch"].toString()); combo("FinalEpoch",spec["final_epoch"].toString());
   auto *buttons=panel->findChild<QDialogButtonBox *>("resourceButtons");
   if (!cancel&&(scenario=="FixedRemainder"||scenario=="ShortArc")) {
    const auto field=scenario=="ShortArc" ? "InterpolationOrder" : "StepSize";
    if (scenario=="ShortArc") line(field,"0"); else combo(field,"0"); click(buttons,QDialogButtonBox::Apply);
    require(child&&child->isVisible()&&pristine()&&panel->hasChanges()&&!panel->findChild<QLabel *>("resourceStatus")->text().isEmpty(),"Invalid ephemeris sampling settings mutated state or lost their diagnostic"); ++invalids;
    if (scenario=="ShortArc") line(field,"7"); else combo(field,spec["sampling"].toString());
   }
   if (cancel) {
    auto closeChoice=[&](QMessageBox::StandardButton decision) {
     std::exception_ptr failure; QTimer response; response.setInterval(10);
     QObject::connect(&response,&QTimer::timeout,&window,[&] { auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget()); if (!box) return; response.stop(); try { acceptanceView(*box); auto *button=box->button(decision); require(button,"Ephemeris close choice missing"); QTest::mouseClick(button,Qt::LeftButton); } catch (...) { failure=std::current_exception(); box->reject(); } });
     response.start(); click(buttons,QDialogButtonBox::Close); response.stop(); if (failure) std::rethrow_exception(failure);
    };
    closeChoice(QMessageBox::Cancel); require(child&&child->isVisible()&&panel->hasChanges()&&pristine(),"Close Cancel lost the pending ephemeris draft");
    closeChoice(QMessageBox::Discard); require(!child&&pristine(),"Discarding the ephemeris draft changed configured mission/history");
   }
  };
  configure(true); ++cancels; require(pristine(),"Ephemeris sampling Cancel changed source/model/history");
  reveal(tree); QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(found.first()).center()); events(); QTest::mouseDClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(found.first()).center()); events(); child=area->currentSubWindow(); panel=child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr; require(panel,"Ephemeris editor did not reopen after draft discard");
  configure(false); require(pristine()&&panel->hasChanges(),"Grouped ephemeris settings escaped pending isolation");
  click(panel->findChild<QDialogButtonBox *>("resourceButtons"),QDialogButtonBox::Apply); panel=child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr; require(panel,"Rebuilt ephemeris editor missing after Apply"); require(!source->toPlainText().isEmpty()&&source->toPlainText()!=initial,"Ephemeris sampling Apply did not commit");
  const auto applied=source->toPlainText(); require(applied.endsWith(spec["mission"].toString()+tail),"Ephemeris Apply altered mission statements/comments");
  require(Moderator::Instance()->GetConfiguredObject("Export")->GetBooleanParameter("WriteEphemeris"),"Applied exporter remained disabled");
  const int appliedUndo=source->document()->availableUndoSteps(); auto *cleanApply=child->widget()->findChild<QDialogButtonBox *>("resourceButtons")->button(QDialogButtonBox::Apply); require(cleanApply&&cleanApply->isVisible()&&!cleanApply->isEnabled()&&!panel->hasChanges(),"Clean ephemeris Apply must be visible and disabled"); acceptanceView(*child); QTest::mouseClick(cleanApply,Qt::LeftButton); events(); require(source->toPlainText()==applied&&source->document()->availableUndoSteps()==appliedUndo,"Repeated ephemeris Apply changed source/history"); child->close(); events();
  source->undo(); require(source->toPlainText()==initial&&window.buildScript(),"Ephemeris Undo did not restore exact source"); source->redo(); require(source->toPlainText()==applied&&window.buildScript(),"Ephemeris Redo failed");
  require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==applied,"Ephemeris Unicode save/reopen failed");
  if (scenario=="ToggleGap") {
   auto *missionTree=window.findChild<QTreeWidget *>("Mission"); for (auto *tabs:window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==missionTree||tabs->widget(i)->isAncestorOf(missionTree)) tabs->setCurrentIndex(i);
   for (const auto &entry:QList<QPair<QString,QString>>{{"Gap off","Off"},{"Gap on","On"}}) {
    QTreeWidgetItem *item=nullptr; for (QTreeWidgetItemIterator it(missionTree);*it;++it) if ((*it)->text(0).contains(entry.first)) { item=*it; break; }
    require(item,"Toggle gap command missing from Mission tree"); missionTree->scrollToItem(item); events(); reveal(missionTree); QTest::mouseClick(missionTree->viewport(),Qt::LeftButton,Qt::NoModifier,missionTree->visualItemRect(item).center()); events(); QTest::mouseDClick(missionTree->viewport(),Qt::LeftButton,Qt::NoModifier,missionTree->visualItemRect(item).center()); events();
    auto *owner=area->currentSubWindow(); auto *command=owner ? dynamic_cast<CommandEditor *>(owner->widget()) : nullptr; auto *state=command ? command->findChild<QComboBox *>("commandToggleState") : nullptr; reveal(state); state->setCurrentText(entry.second); events(); acceptanceView(*owner);
    require(command->hasChanges()&&command->findChild<QLineEdit *>("commandField_Subscribers")->text()=="Export","Actual Toggle state edit did not remain pending"); click(command->findChild<QDialogButtonBox *>(),QDialogButtonBox::Apply); owner->close(); events();
   }
   require(source->toPlainText()==QString(applied).replace("Toggle 'Gap off' Export On;","Toggle 'Gap off' Export Off;").replace("Toggle 'Gap on' Export Off;","Toggle 'Gap on' Export On;"),"Toggle gap GUI edits changed unrelated source");
  }
  require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript(),"Final sampling mission reopen failed"); save("mission.script",source->toPlainText().toUtf8()); messages->clear();
  const auto result=window.runMission(); events(); save("messages.txt",messages->toPlainText().toUtf8()); if (result!=MainWindow::RunResult::Completed) std::cerr<<messages->toPlainText().toStdString(); require(result==MainWindow::RunResult::Completed,"Ephemeris sampling execution failed");
  require(QString::fromStdString(Moderator::Instance()->GetConfiguredObject("Export")->GetStringParameter("OutputTimeSystem"))==spec["time_system"].toString(),"Native exporter lost its output time system");
  require(QString::fromStdString(Moderator::Instance()->GetConfiguredObject("Export")->GetStringParameter("EpochFormat"))==spec["input_epoch_format"].toString(),"Output time selection changed input-boundary epoch format");
  const auto payload=read(oem); save("mission.oem",payload); save("final.csv",read(truth)); if (spec["retain_raw_samples"].toBool()) save("raw-inputs.csv",read(files.filePath("Native samples Δ.csv")));
  auto seconds=[&](QString text) { if (text.endsWith('Z')) text.chop(1); const auto epochs=spec["expected_epochs"].toArray(); const auto wanted=spec["expected_segments"].toArray()[0].toArray(); for (int i=0;i<epochs.size();++i) if (text==epochs[i].toString()) return wanted[i].toArray()[0].toDouble(); throw std::runtime_error("OEM time tag differs from independent selected-system calendar"); };
  QJsonArray segments; QJsonObject metadata; QJsonArray rows; bool inMetadata=false;
  auto finish=[&] { if (!metadata.isEmpty()) { segments.append(QJsonObject{{"metadata",metadata},{"rows",rows}}); metadata={}; rows={}; } };
  for (const auto &line:QString::fromUtf8(payload).split('\n')) {
   const auto text=line.trimmed(); if (text=="META_START") { finish(); inMetadata=true; continue; } if (text=="META_STOP") { inMetadata=false; continue; }
   if (inMetadata) { const int equal=text.indexOf('='); if (equal>0) metadata[text.left(equal).trimmed()]=text.mid(equal+1).trimmed(); continue; }
   if (QRegularExpression("^\\d{4}-\\d\\d-\\d\\dT").match(text).hasMatch()) {
    const auto parts=text.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(parts.size()==7,"OEM Cartesian row has unexpected columns"); QJsonArray row{seconds(parts[0])};
    for (int i=1;i<7;++i) { bool ok=false; const double v=parts[i].toDouble(&ok); require(ok&&std::isfinite(v),"OEM Cartesian value is invalid"); row.append(v); } rows.append(row);
   }
  } finish();
  const auto expected=spec["expected_segments"].toArray(); require(segments.size()==expected.size(),"OEM does not separately bound the expected arcs");
  double maxPosition=0,maxVelocity=0; int points=0;
  for (int s=0;s<segments.size();++s) {
   const auto segment=segments[s].toObject(),meta=segment["metadata"].toObject(); const auto actual=segment["rows"].toArray(),wanted=expected[s].toArray();
   require(actual.size()==wanted.size(),"OEM sample cadence/count omits a boundary or fills a disabled interval"); require(meta["TIME_SYSTEM"]==spec["time_system"].toString()&&meta["CENTER_NAME"].toString().toUpper()=="EARTH"&&meta["REF_FRAME"]=="EME2000"&&meta["INTERPOLATION"].toString().toUpper()=="LAGRANGE","OEM frame/time/interpolation metadata differs from configured NASA fields");
   require(meta["INTERPOLATION_DEGREE"].toString().toInt()==spec["expected_degrees"].toArray()[s].toInt(),"OEM effective interpolation degree differs from guide/fixture");
   require(std::abs(seconds(meta["START_TIME"].toString())-wanted.first().toArray()[0].toDouble())<.002&&std::abs(seconds(meta["STOP_TIME"].toString())-wanted.last().toArray()[0].toDouble())<.002,"OEM metadata does not bound its expected arc");
   for (int i=0;i<actual.size();++i) { const auto a=actual[i].toArray(),b=wanted[i].toArray(); require(std::abs(a[0].toDouble()-b[0].toDouble())<.002,"OEM sample epoch differs from independent schedule");
    if (i) require(a[0].toDouble()>actual[i-1].toArray()[0].toDouble(),"OEM segment epochs are not strictly increasing");
    for (int c=1;c<7;++c) { const double error=std::abs(a[c].toDouble()-b[c].toDouble()); require(error<spec[c<4 ? "position_tolerance" : "velocity_tolerance"].toDouble(),"OEM state differs from independent complete circular oracle"); if (c<4) maxPosition=std::max(maxPosition,error); else maxVelocity=std::max(maxVelocity,error); } ++points;
   }
  }
  const auto final=QString::fromUtf8(read(truth)).trimmed().split(','); const auto expectedFinal=spec["expected_final"].toArray(); require(final.size()==8,"Final ephemeris witness has wrong column count");
  for (int i=0;i<8;++i) { bool ok=false; const double value=final[i].toDouble(&ok); require(ok&&std::isfinite(value)&&std::abs(value-expectedFinal[i].toDouble())<(i==0 ? 1e-4 : i<4 ? 2e-5 : 2e-8),"Final mission state differs from independent circular/sentinel witness"); }
  require(final.last().toDouble()==9,"Ephemeris configuration altered unrelated mission sentinel");
  if (!spec["user_interval"].toBool()) { const auto last=segments.last().toObject()["rows"].toArray().last().toArray(); for (int i=1;i<7;++i) require(std::abs(last[i].toDouble()-final[i].toDouble())<(i<4 ? 2e-10 : 2e-13),"Non-grid terminal OEM point is not the exact final propagated state"); }
  save("segments.json",QJsonDocument(segments).toJson());
  auto *outputTree=window.findChild<QTreeWidget *>("Output"); for (auto *tabs:window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==outputTree||tabs->widget(i)->isAncestorOf(outputTree)) tabs->setCurrentIndex(i);
  QTreeWidgetItem *outputItem=nullptr; for (auto *item:outputTree->findItems("Export",Qt::MatchExactly|Qt::MatchRecursive)) if (item->data(0,Qt::UserRole+1)=="ephemeris") outputItem=item;
  require(outputItem,"Generated OEM missing from Output tree"); outputTree->scrollToItem(outputItem); events(); reveal(outputTree); QTest::mouseClick(outputTree->viewport(),Qt::LeftButton,Qt::NoModifier,outputTree->visualItemRect(outputItem).center()); events(); QTest::mouseDClick(outputTree->viewport(),Qt::LeftButton,Qt::NoModifier,outputTree->visualItemRect(outputItem).center()); events();
  auto views=window.findChildren<QPlainTextEdit *>("report:Export"); require(!views.isEmpty()&&views.last()->toPlainText()==QString::fromUtf8(payload).replace("\r\n","\n"),"Output viewer does not contain the complete generated OEM"); acceptanceView(*area->currentSubWindow());
  if (key=="TAI_TTInputWindow") {
   const auto beforeSwitch=source->toPlainText();
   auto openAgain=[&] { for (auto *tabs:window.findChildren<QTabWidget *>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==tree||tabs->widget(i)->isAncestorOf(tree)) tabs->setCurrentIndex(i); events(); const auto currentItems=tree->findItems("Export",Qt::MatchExactly|Qt::MatchRecursive); require(currentItems.size()==1,"Applied exporter resource is missing or ambiguous"); tree->scrollToItem(currentItems.first()); events(); reveal(tree); const auto point=tree->visualItemRect(currentItems.first()).center(); QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,point); events(); QTest::mouseDClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,point); events(); child=area->currentSubWindow(); panel=child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr; require(panel,"Applied output-time editor did not reopen"); };
   openAgain(); auto *format=panel->findChild<QComboBox *>("ephemeris_FileFormat"); reveal(format); format->setCurrentText("SPK"); events(); require(!panel->findChild<QComboBox *>("ephemeris_OutputTimeSystem")->isEnabled(),"Applied non-UTC clock remains active for SPK");
   click(panel->findChild<QDialogButtonBox *>("resourceButtons"),QDialogButtonBox::Apply); save("format-switch.script",source->toPlainText().toUtf8());
   require(Moderator::Instance()->GetConfiguredObject("Export")->GetStringParameter("FileFormat")=="SPK"&&Moderator::Instance()->GetConfiguredObject("Export")->GetStringParameter("OutputTimeSystem")=="UTC","Committed format switch did not reset inactive clock");
   click(child->widget()->findChild<QDialogButtonBox *>("resourceButtons"),QDialogButtonBox::Close); require(!child,"Switched format editor did not close");
   const auto afterSwitch=source->toPlainText(); require(window.buildScript(),"Switched non-UTC script is not buildable");
   source->undo(); require(source->toPlainText()==beforeSwitch&&window.buildScript()&&Moderator::Instance()->GetConfiguredObject("Export")->GetStringParameter("OutputTimeSystem")=="TAI","Format-switch Undo lost selected OEM clock");
   source->redo(); require(source->toPlainText()==afterSwitch&&window.buildScript()&&Moderator::Instance()->GetConfiguredObject("Export")->GetStringParameter("OutputTimeSystem")=="UTC","Format-switch Redo restored an inactive clock");
   source->undo(); require(source->toPlainText()==beforeSwitch&&window.buildScript(),"Format-switch recovery did not restore OEM script");
  }
  const QJsonArray facets{"actual-resource-tree-ephemeris-editor","pending-cancel-invalid-history-unicode","literal-gravitational-input","independent-every-oem-state-and-epoch","metadata-cadence-segment-and-terminal-oracles","actual-complete-output-viewer","disabled-clean-apply-idempotence"};
  const QJsonObject proof{{"checkpoint","oem-output-time-"+key},{"outcome","passed"},{"family",key},{"segments",segments.size()},{"points",points},{"max_position_error",maxPosition},{"max_velocity_error",maxVelocity},{"invalid_rejections",invalids},{"cancels",cancels},{"numeric_sentinel",9},{"clean_apply_disabled",true},{"facets",facets},{"scope","Actual selected OEM time systems with independent calendars and every circular state,private A1 convention and original GUI lifecycle assertions. No NASA row admission or E404 conflict waiver."}};
  save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
 } catch (BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl; } catch (std::exception &e) { std::cerr<<"FAIL "<<e.what()<<std::endl; } return 1;
}
