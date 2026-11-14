#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "SatelliteImportData.hpp"
#include "PropagationGroupsDialog.hpp"
#include "QtPlotReceiver.hpp"
#include "PlotWidget.hpp"
#include "OrbitObjectAxes.hpp"
#include "ResourceEditor.hpp"
#include "SatelliteElementsPanel.hpp"
#include "SatelliteImportDialog.hpp"
#include "SpaceTrackAccount.hpp"
#include <QAction>
#include <QAccessible>
#include <QApplication>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDateEdit>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLineEdit>
#include <QLabel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
void events() { QApplication::processEvents(); QTest::qWait(40); }
void select(QTreeWidget *tree,const QString &name)
{
   const auto rows=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);
   require(rows.size()==1,"Resource row is missing or ambiguous");
   tree->scrollToItem(rows.first()); events();
   QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(rows.first()).center());
   QTest::qWait(250);
   std::cerr<<"Requested "<<name.toStdString()<<", current "<<(tree->currentItem() ? tree->currentItem()->text(0).toStdString() : "none")<<'\n';
   require(tree->currentItem()==rows.first() && rows.first()->isSelected(),"Resource selection was lost after clicking");
}

void checkAssignments(MainWindow &window,const QString &input,const QString &directory)
{
   auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
   QFile elements(input);
   require(elements.open(QIODevice::ReadOnly),"Cannot read orbital fixture");
   const auto data=parseSatelliteElements(elements.readAll()).first();
   const auto report=directory+"/propagated.txt";
   const QString first="Propagate 'First arc' Numerical(Original) {Original.ElapsedSecs = 60};";
   const QString second="Propagate 'Second arc' Numerical(Original) {Original.ElapsedSecs = 120};";
   const QString baseline=
      "Create Spacecraft Original;\n"
      "Original.DateFormat = UTCModJulian;\nOriginal.Epoch = '21723.28495062';\n"
      "Create Propagator Numerical;\n"
      "Create Variable Keep;\nKeep = 41;\n"
      "Create ReportFile Values;\nValues.Filename = '"+report+"';\n"
      "Values.WriteHeaders = false;\nValues.FixedWidth = false;\n"
      "Values.Delimiter = ',';\nValues.Precision = 17;\n"
      "BeginMissionSequence;\nKeep = Keep + 1;\n"+first+" % first stop\n"+second+" % second stop\n";
   const auto reset=[&](const QString &source) {
      editor->setPlainText(source);
      if (!window.buildScript()) {
         std::cerr<<window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()<<'\n';
         throw std::runtime_error("Cannot build propagation fixture");
      }
   };
   const auto command=[&](const QString &label) {
      for (int i=0;i<window.missionSnapshot().nodes.size();++i)
         if (window.missionSnapshot().nodes[i].statement.contains(label)) return i;
      throw std::runtime_error("Propagation target is missing");
   };
   reset(baseline);
   require(window.importSatellite(data,"Unassigned",baseline,directory).isEmpty(),"Resource-only import failed");
   require(editor->toPlainText().endsWith(baseline),"Default public import changed the mission commands");

   reset(baseline);
   const auto error=window.importSatellite(data,"Vanguard",baseline,directory,command("First arc"));
   require(error.isEmpty(),qPrintable(error));
   const auto imported=editor->toPlainText();
   auto expected=baseline;
   expected.replace(first,PropagationGroupsDialog::appendGroup(first,"VanguardProp","Vanguard"));
   require(imported.endsWith(expected),"Import changed unrelated commands, stops or comments");
   editor->undo();
   require(editor->toPlainText()==baseline,"Import and propagation assignment are not a single undo");
   editor->redo();
   require(editor->toPlainText()==imported && window.buildScript(),"Import redo did not restore a buildable mission");
   const auto saved=directory+"/assigned.script";
   require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Assigned mission did not save and reopen");
   QString reportCommand="Report Values";
   for (const auto &field:QStringList{"X","Y","Z","VX","VY","VZ"}) reportCommand+=" Vanguard.EarthMJ2000Eq."+field;
   editor->appendPlainText(reportCommand+" Keep;");
   require(window.runMission()==MainWindow::RunResult::Completed,"Joined propagation mission failed");
   QFile output(report);
   require(output.open(QIODevice::ReadOnly),"Joined propagation report is missing");
   const auto values=QString::fromUtf8(output.readAll()).trimmed().split(',');
   require(values.size()==7 && values.last().trimmed()=="42","Joined propagation changed the mission sentinel");
   // Reuse the source-versioned 60-second TLE result from the prior accepted
   // import check. Its original independent SGP4 witness and bounds are retained.
   const double reference[]={7122.2614061411923,-1014.0153337643309,271.74659955806817,
      1.4370491747779159,6.483761968802721,4.5261378355074493};
   for (int i=0;i<6;++i) {
      bool valid=false;
      const double value=values[i].toDouble(&valid);
      require(valid && std::isfinite(value) && std::abs(value-reference[i])<(i<3 ? 1e-5 : 1e-8),
         "Joined command changed the saved TLE propagation result");
   }

   reset(baseline);
   require(window.importSatellite(data,"SecondSat",baseline,directory,command("Second arc")).isEmpty(),"Second propagation target failed");
   expected=baseline;
   expected.replace(second,PropagationGroupsDialog::appendGroup(second,"SecondSatProp","SecondSat"));
   require(editor->toPlainText().endsWith(expected),"Import chose a different propagation target");

   const QString emptyMission="Create Variable Keep;\nKeep = 41;\nBeginMissionSequence;\nKeep = Keep + 1;\n";
   reset(emptyMission);
   require(window.importSatellite(data,"NewArc",emptyMission,directory,-1).isEmpty(),"New one-step propagation failed");
   require(editor->toPlainText().endsWith(emptyMission+"Propagate NewArcProp(NewArc);\n"),"New command did not use native one-step propagation");

   reset(baseline);
   auto raw=data;
   raw.elements.clear(); raw.extension.clear(); raw.propagatorType.clear();
   raw.state=std::array<double,6>{7000,0,0,0,7.5,0};
   require(window.importSatellite(raw,"StateSat",baseline,directory,command("First arc"),"Numerical").isEmpty(),"State vector assignment to numerical propagator failed");
   require(editor->toPlainText().contains("Numerical(StateSat)"),"State vector did not use the selected numerical setup");

   reset(baseline);
   require(!window.importSatellite(raw,"StateSat",baseline,directory,command("First arc"),"Missing").isEmpty(),"Missing numerical setup was accepted");
   require(editor->toPlainText()==baseline,"Invalid state setup mutated the script");
   require(!window.importSatellite(data,"Invalid",baseline,directory,999999).isEmpty(),"Stale propagation target was accepted");
   require(editor->toPlainText()==baseline,"Invalid target mutated the script");

   auto variational=baseline;
   variational.replace("Numerical(Original)","Numerical(Original, 'STM')");
   reset(variational);
   const int undoSteps=editor->document()->availableUndoSteps();
   const auto rejectedFiles=directory+"/rejected";
   require(!window.importSatellite(data,"Rejected",variational,rejectedFiles,command("First arc")).isEmpty(),"Ephemeris group accepted command-wide STM");
   require(editor->toPlainText()==variational && editor->document()->availableUndoSteps()==undoSteps,"Incompatible propagation mutated the script or undo history");
   require(QDir(rejectedFiles).entryList(QDir::Files).isEmpty(),"Rejected import left a new orbital-data file");
   require(window.buildScript(),"Failed import did not restore the original buildable mission");

   const QString continued="Propagate 'Preserved' BackProp Synchronized Numerical(Original) ... % retained comment\n"
      "  {Original.ElapsedSecs = -60, StopTolerance = 1e-8, EphemerisPolicy = 'Native'}; % trailing\n";
   const auto appended=PropagationGroupsDialog::appendGroup(continued,"CatalogProp","Catalog");
   auto preserved=continued;
   preserved.replace("Numerical(Original)","Numerical(Original) CatalogProp(Catalog)");
   require(appended==preserved,"Adding a group changed propagation modifiers or source formatting");
   std::cout<<"PASS selected and new commands, resource-only compatibility, state-vector setup, single Undo/Redo, save/reopen, saved numerical witness, invalid-target and incompatible-option rollback\n";
}

void checkElements(MainWindow &window,const QString &input,const QString &directory)
{
   QFile file(input);require(file.open(QIODevice::ReadOnly),"Cannot read elements fixture");
   const auto tle=parseSatelliteElements(file.readAll()).first();
   const QMap<QString,double> expected={{"INCLINATION",34.2682},{"RA_OF_ASC_NODE",348.7242},
      {"ECCENTRICITY",.1859667},{"ARG_OF_PERICENTER",331.7664},
      {"MEAN_ANOMALY",19.3264},{"MEAN_MOTION",10.82419157},{"BSTAR",.000028098}};
   auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
   auto *area=window.findChild<QMdiArea *>("workspace");
   const auto closePanels=[&] {
      for (auto *child:area->subWindowList()) if (dynamic_cast<ResourceEditor *>(child->widget())) child->close();
      events();
   };
   const auto open=[&](const QString &name) {
      closePanels();
      auto *tree=window.findChild<QTreeWidget *>("Resources");
      const auto rows=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive);
      require(rows.size()==1,"Imported spacecraft tree row is missing");
      tree->itemDoubleClicked(rows.first(),0);events();
      auto *panel=dynamic_cast<ResourceEditor *>(area->activeSubWindow()->widget());
      require(panel,"Spacecraft editor did not open");return panel;
   };
   const auto inspect=[&](ResourceEditor *panel,const SatelliteImportData &data) {
      auto *source=panel->findChild<QPlainTextEdit *>("satelliteElementsSource");
      require(source && source->isReadOnly() && source->toPlainText()==QString::fromUtf8(data.elements),"Spacecraft editor did not retain the raw source record");
      for (auto it=expected.cbegin();it!=expected.cend();++it) {
         auto *value=panel->findChild<QLineEdit *>("satelliteMean_"+it.key());
         require(value && value->isReadOnly() && std::abs(value->text().toDouble()-it.value())<1e-10,"Native mean-element value differs");
      }
      require(panel->findChild<QLineEdit *>("satelliteMean_EPOCH")->text()==data.epoch,"Source epoch differs");
      auto *tabs=panel->findChild<QTabWidget *>("resourceEditorTabs");
      require(tabs->tabText(tabs->currentIndex())=="Orbital elements","Imported spacecraft did not open its source elements");
      require(!panel->hasUnappliedInput(),"Viewing source elements marked the spacecraft dirty");
   };
   require(window.importSatellite(tle,"ElementTle",editor->toPlainText(),directory).isEmpty(),"TLE import failed");
   inspect(open("ElementTle"),tle);closePanels();
   const auto saved=directory+"/elements.script";
   require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Elements mission did not save and reopen");
   inspect(open("ElementTle"),tle);closePanels();
   QJsonObject fields{{"OBJECT_NAME","Vanguard"},{"NORAD_CAT_ID",5},{"EPOCH",tle.epoch},
      {"MEAN_MOTION",10.82419157},{"ECCENTRICITY",.1859667},{"INCLINATION",34.2682},
      {"RA_OF_ASC_NODE",348.7242},{"ARG_OF_PERICENTER",331.7664},{"MEAN_ANOMALY",19.3264},
      {"BSTAR",.000028098},{"MEAN_MOTION_DOT",.00000023},{"MEAN_MOTION_DDOT",0}};
   const auto json=QJsonDocument(QJsonArray{fields}).toJson();
   const auto omm=parseSatelliteElements(json).first();
   require(window.importSatellite(omm,"ElementOmm",editor->toPlainText(),directory).isEmpty(),"OMM import failed");
   inspect(open("ElementOmm"),omm);closePanels();
   SatelliteElementsPanel preview;
   const auto jsonPath=directory+"/original.json";
   QFile original(jsonPath);require(original.open(QIODevice::WriteOnly) && original.write(json)==json.size(),"Cannot write original OMM JSON");original.close();
   require(preview.setSource(jsonPath,"00005"),"OMM JSON was not recognized");
   require(preview.findChild<QPlainTextEdit *>("satelliteElementsSource")->toPlainText()==QString::fromUtf8(json),"Original OMM JSON changed in display");
   require(preview.setSource(jsonPath,"99999") && !preview.findChild<QLabel *>("satelliteElementsStatus")->text().isEmpty(),"Mismatched catalog ID showed another satellite");
   require(preview.findChild<QLineEdit *>("satelliteMean_MEAN_MOTION")->text().isEmpty(),"Missing catalog ID retained stale mean elements");
   require(preview.setSource(directory+"/missing.tle","5") && !preview.findChild<QLabel *>("satelliteElementsStatus")->text().isEmpty(),"Missing TLE source had no explanation");
   auto *plain=open("DefaultSC");
   auto *tabs=plain->findChild<QTabWidget *>("resourceEditorTabs");
   for (int i=0;i<tabs->count();++i) if (tabs->tabText(i)=="Orbital elements") require(!tabs->isTabVisible(i),"Numeric spacecraft showed an empty orbital-elements tab");
   std::cout<<"PASS imported TLE and OMM source plus native mean elements, clean viewing, save/reopen, original JSON, catalog matching, missing-file and numeric-spacecraft handling\n";
}

void checkHistoricalImport(MainWindow &window,const QString &directory)
{
   require(QDir().mkpath(directory),"Cannot create historical-import output directory");
   auto &account=SpaceTrackAccount::instance();
   bool loaded=false;
   QString accountError;
   account.load(&window,[&](const QString &error) { accountError=error;loaded=true; });
   require(QTest::qWaitFor([&] { return loaded; },35000),"Account lookup timed out");
   require(accountError.isEmpty(),"Saved account could not be loaded");
   require(!account.credentials().identity.isEmpty() && !account.credentials().password.isEmpty(),"Save the Space-Track account before running this live check");
   auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
   const QString command="Propagate 'Historical import check' P(Anchor) {Anchor.ElapsedSecs = 60};";
   const auto reportPath=directory+"/historical-state.txt";
   editor->setPlainText("Create Spacecraft Anchor;\n"
      "Anchor.DateFormat = UTCGregorian;\nAnchor.Epoch = '01 Jan 2024 00:00:00.000';\n"
      "Create Propagator P;\nCreate ReportFile State;\nState.Filename = '"+reportPath+"';\n"
      "State.WriteHeaders = false;\nState.FixedWidth = false;\nState.Delimiter = ',';\nState.Precision = 17;\n"
      "BeginMissionSequence;\n"+command+"\n");
   require(window.buildScript(),"Historical baseline did not build");
   const auto baseline=editor->toPlainText();
   SatelliteImportData chosen;
   int recordCount=0;
   QJsonArray requests;
   std::exception_ptr failure;
   QTimer::singleShot(0,&window,[&] {
      auto *dialog=dynamic_cast<SatelliteImportDialog *>(window.findChild<QDialog *>("satelliteImportDialog"));
      try {
         require(dialog,"Historical import dialog did not open");
         auto *network=dialog->findChild<QNetworkAccessManager *>();
         QObject::connect(network,&QNetworkAccessManager::finished,&window,[&](QNetworkReply *reply) {
            requests.append(QJsonObject{{"url",reply->url().toString()},
               {"http_status",reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()},
               {"network_error",static_cast<int>(reply->error())}});
         });
         dialog->findChild<QComboBox *>("satelliteCatalogSource")->setCurrentIndex(1);
         dialog->findChild<QLineEdit *>("satelliteCatalogQuery")->setText("25544");
         dialog->findChild<QDateEdit *>("satelliteHistoryDate")->setDate(QDate(2024,1,1));
         auto *search=dialog->findChild<QPushButton *>("searchSatelliteCatalog");
         auto *results=dialog->findChild<QListWidget *>("satelliteCatalogResults");
         search->click();
         require(QTest::qWaitFor([&] { return search->isEnabled(); },45000),"Historical request timed out");
         const auto status=dialog->findChild<QLabel *>("satelliteImportStatus")->text();
         if (results->count()==0) throw std::runtime_error(status.toStdString());
         chosen=dialog->selectedData();recordCount=results->count();
         require(chosen.catalogId=="25544" && chosen.source=="Space-Track","The returned historical satellite differs");
         require(chosen.epoch.left(10)>="2023-12-29" && chosen.epoch.left(10)<="2024-01-01","Historical result is outside the requested interval");
         dialog->findChild<QLineEdit *>("importSatelliteName")->setText("HistoricalISS");
         auto *target=dialog->findChild<QComboBox *>("satellitePropagationTarget");
         require(target->currentText()==command,"Existing Propagate command was not the default target");
         dialog->findChild<QPushButton *>("importSatellite")->click();
         require(!dialog->isVisible(),"Historical import was not accepted");
      } catch (...) {
         failure=std::current_exception();
         if (dialog) dialog->reject();
      }
   });
   window.findChild<QAction *>("importSatelliteCatalog")->trigger();
   QFile networkFile(directory+"/network.json");
   require(networkFile.open(QIODevice::WriteOnly),"Cannot save request status evidence");
   networkFile.write(QJsonDocument(requests).toJson());networkFile.close();
   if (failure) std::rethrow_exception(failure);
   const auto imported=editor->toPlainText();
   auto expected=baseline;
   expected.replace(command,PropagationGroupsDialog::appendGroup(command,"HistoricalISSProp","HistoricalISS"));
   require(imported.endsWith(expected),"Historical import changed unrelated mission commands");
   require(requests.size()==2,"Expected one login and one uncached history request");
   for (const auto &entry:requests) require(entry.toObject().value("http_status").toInt()==200,"A live Space-Track request failed");

   // Put the test companion at the element epoch before exercising the shared
   // command, preserving the runtime rule against disjoint simulation times.
   const auto epoch=QRegularExpression("HistoricalISS\\.Epoch = ('[^']+');").match(imported).captured(1);
   require(!epoch.isEmpty(),"Imported epoch was not saved in the script");
   auto aligned=imported;
   aligned.replace("Anchor.DateFormat = UTCGregorian;","Anchor.DateFormat = UTCModJulian;");
   aligned.replace("Anchor.Epoch = '01 Jan 2024 00:00:00.000';","Anchor.Epoch = "+epoch+";");
   aligned+="Report State HistoricalISS.ElapsedSecs HistoricalISS.EarthMJ2000Eq.X HistoricalISS.EarthMJ2000Eq.Y HistoricalISS.EarthMJ2000Eq.Z;\n";
   editor->setPlainText(aligned);
   require(window.buildScript(),"Epoch-aligned historical mission did not build");
   const auto saved=directory+"/HistoricalISS.script";
   require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Historical import did not save and reopen");
   require(editor->toPlainText()==aligned,"Save/reopen changed the historical mission");
   const auto sourcePath=QRegularExpression("HistoricalISS\\.EphemerisName = '([^']+)';").match(aligned).captured(1);
   QFile sourceFile(sourcePath);
   require(sourceFile.open(QIODevice::ReadOnly) && sourceFile.readAll()==chosen.elements,"Persisted historical orbital record differs");
   auto *tree=window.findChild<QTreeWidget *>("Resources");
   const auto rows=tree->findItems("HistoricalISS",Qt::MatchExactly|Qt::MatchRecursive);
   require(rows.size()==1,"Reopened historical spacecraft is missing from Resources");
   tree->itemDoubleClicked(rows.first(),0);events();
   auto *area=window.findChild<QMdiArea *>("workspace");
   auto *panel=dynamic_cast<ResourceEditor *>(area->activeSubWindow()->widget());
   require(panel,"Reopened spacecraft editor did not open");
   require(panel->findChild<QPlainTextEdit *>("satelliteElementsSource")->toPlainText()==QString::fromUtf8(chosen.elements),"Reopened spacecraft did not show its historical record");
   require(panel->findChild<QLineEdit *>("satelliteMean_EPOCH")->text()==chosen.epoch,"Reopened historical epoch differs");
   require(!panel->hasUnappliedInput(),"Inspecting the historical record changed the spacecraft");
   window.grab().save(directory+"/reopened-spacecraft.png");
   area->activeSubWindow()->close();events();
   require(window.runMission()==MainWindow::RunResult::Completed,"Historical spacecraft failed in the shared Propagate command");
   QFile report(reportPath);require(report.open(QIODevice::ReadOnly),"Historical propagation report is missing");
   const auto values=QString::fromUtf8(report.readAll()).trimmed().split(',');
   require(values.size()==4 && std::abs(values[0].toDouble()-60)<.001,"Historical spacecraft did not reach the shared stop time");
   for (const auto &value:values) { bool valid=false;const double number=value.toDouble(&valid);require(valid && std::isfinite(number),"Historical run produced a nonfinite result"); }
   QFile receipt(directory+"/result.json");require(receipt.open(QIODevice::WriteOnly),"Cannot save historical result evidence");
   receipt.write(QJsonDocument(QJsonObject{{"catalog_id",chosen.catalogId},{"selected_epoch",chosen.epoch},
      {"records_returned",recordCount},{"script",saved},{"source",sourcePath},
      {"source_sha256",QString::fromLatin1(QCryptographicHash::hash(chosen.elements,QCryptographicHash::Sha256).toHex())},
      {"network",requests},{"existing_command_assignment",true},{"save_reopen",true},{"shared_propagation_seconds",60},{"passed",true}}).toJson());
   std::cout<<"PASS live Space-Track historical lookup, existing-command assignment, orbital-source persistence, save/reopen, spacecraft editor and 60-second shared propagation\n";
}

void checkEpochGuard(MainWindow &window,const QString &directory,const QString &elementsPath)
{
   auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
   const auto report=directory+"/epoch-completed.txt";
   const QString resources=
      "Create Spacecraft Alpha Beta;\nAlpha.Epoch = 21545;\nBeta.Epoch = 21546;\n"
      "Create Propagator P Q;\n"
      "Create OrbitView GuardPlot;\nGuardPlot.Add = {Alpha, Beta, Earth};\n"
      "Create Variable Completed;\nCompleted = 0;\nCreate ReportFile Receipt;\n"
      "Receipt.Filename = '"+report+"';\nReceipt.WriteHeaders = false;\nBeginMissionSequence;\n";
   const QString completed="Completed = 1;\nReport Receipt Completed;\n";
   const auto run=[&](const QString &body,bool accepted,const QString &tag) {
      QFile::remove(report);
      editor->setPlainText(resources+body+completed);
      const auto result=window.runMission();
      events();
      const auto messages=window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText();
      require(result==(accepted ? MainWindow::RunResult::Completed : MainWindow::RunResult::Failed),qPrintable(tag+" gave the wrong run result\n"+messages));
      if (accepted) {
         QFile receipt(report);
         require(receipt.open(QIODevice::ReadOnly) && !receipt.readAll().trimmed().isEmpty(),"Aligned mission did not finish its command sequence");
      } else {
         require(messages.contains("same simulation time") && messages.contains("Alpha") && messages.contains("Beta") && messages.contains("UTC"),qPrintable("Epoch rejection did not identify spacecraft and dates\n"+messages));
         const auto plot=window.plotReceiver()->model("GuardPlot");
         require(bool(plot),"Rejected mission lost its plot configuration");
         for (const auto &curve:plot->curves) require(curve.points.empty(),"Disjoint command published trajectory samples before rejection");
         QFile receipt(report);
         require(!receipt.exists() || (receipt.open(QIODevice::ReadOnly) && receipt.readAll().trimmed().isEmpty()),"Disjoint mission executed a command after rejection");
      }
      std::cout<<"PASS "<<tag.toStdString()<<'\n';
   };
   run("Propagate P(Alpha) Q(Beta) {Alpha.ElapsedSecs = 60};\n",false,"independent disjoint epochs rejected");
   run("Propagate Synchronized P(Alpha) Q(Beta) {Alpha.ElapsedSecs = 60};\n",false,"synchronized disjoint epochs rejected");
   run("Beta.A1ModJulian = Alpha.A1ModJulian;\nPropagate Synchronized P(Alpha) Q(Beta) {Alpha.ElapsedSecs = 60};\n",true,"runtime epoch alignment accepted");
   run("Beta.A1ModJulian = Alpha.A1ModJulian + 0.0001 / 86400;\nPropagate Synchronized P(Alpha) Q(Beta) {Alpha.ElapsedSecs = 60};\n",true,"sub-millisecond epoch roundoff accepted");
   run("Beta.A1ModJulian = Alpha.A1ModJulian + 0.01;\nPropagate P(Alpha) Q(Beta) {Alpha.ElapsedSecs = 60};\n",false,"runtime epoch mismatch rejected");
   run("Propagate P(Alpha) {Alpha.ElapsedSecs = 60};\nPropagate Q(Beta) {Beta.ElapsedSecs = 60};\n",true,"separate command timelines accepted");
   QFile elements(elementsPath);
   require(elements.open(QIODevice::ReadOnly),"Cannot read mixed-propagator fixture");
   const auto data=parseSatelliteElements(elements.readAll()).first();
   const auto source=satelliteImportScript(data,"Vanguard",elementsPath)+
      "Create Spacecraft Alpha;\nCreate Propagator P;\nBeginMissionSequence;\n"
      "Alpha.A1ModJulian = Vanguard.A1ModJulian;\n"
      "Propagate Synchronized P(Alpha) VanguardProp(Vanguard) {Alpha.ElapsedSecs = 12000};\n"
      "Propagate Synchronized P(Alpha) VanguardProp(Vanguard) {Alpha.ElapsedSecs = 12060};\n";
   editor->setPlainText(source);
   require(window.runMission()==MainWindow::RunResult::Completed,"Long synchronized segment could not enter the next shared command");
   std::cout<<"PASS mixed propagators rejoin after a long synchronized segment\n";
}

double rotationDegrees(const PlotPoint &first,const PlotPoint &last)
{
   double trace=0;
   for (int i=0;i<9;++i) trace+=first.bodyToView[i]*last.bodyToView[i];
   return std::acos(std::clamp((trace-1)/2,-1.0,1.0))*180/std::acos(-1.0);
}

void checkTimeline(MainWindow &window,const QString &path,bool aligned)
{
   require(window.loadScript(path),"Cannot load timeline mission");
   require(window.runMission()==MainWindow::RunResult::Completed,"Timeline mission failed");
   auto *receiver=window.plotReceiver();
   const auto model=receiver->model("ReviewOrbit");
   require(bool(model),"Timeline orbit plot is missing");
   QJsonObject summary;
   for (const auto &curve:model->curves) {
      require(!curve.points.empty(),"Timeline object has no retained points");
      const auto &first=curve.points.front(),&last=curve.points.back();
      double jump=0,minEpoch=first.epoch,maxEpoch=first.epoch;
      for (size_t i=0;i<curve.points.size();++i) {
         const auto &point=curve.points[i];
         minEpoch=std::min(minEpoch,point.epoch); maxEpoch=std::max(maxEpoch,point.epoch);
         if (i) jump=std::max(jump,rotationDegrees(curve.points[i-1],point));
      }
      const double duration=(last.epoch-first.epoch)*86400;
      const double displacement=std::hypot(last.x-first.x,last.y-first.y,last.z-first.z);
      summary.insert(curve.name,QJsonObject{{"samples",int(curve.points.size())},{"first_epoch",first.epoch},
         {"last_epoch",last.epoch},{"duration_seconds",duration},{"span_days",maxEpoch-minEpoch},
         {"displacement_km",displacement},{"rotation_degrees",rotationDegrees(first,last)},
         {"max_adjacent_rotation_degrees",jump}});
      if (aligned) {
         require(std::abs(duration-120)<0.00005,"Synchronized objects cover different intervals");
         if (curve.name=="Earth") {
            require(jump<1,"Earth rotates discontinuously during the aligned mission");
            const auto rotation=rotationDegrees(first,last);
            require(rotation>0.49 && rotation<0.51,"Earth rotation differs from a two-minute sidereal interval");
         } else require(displacement>500,"A spacecraft did not traverse the two-minute arc");
      }
   }
   if (aligned) {
      const auto first=summary.value("Sat").toObject().value("first_epoch").toDouble();
      const auto other=summary.value("HAWK_15A").toObject().value("first_epoch").toDouble();
      require(std::abs(first-other)*86400<0.00005,"Spacecraft start at different epochs");
      PlotWidget *plot=nullptr;
      for (auto *child:receiver->workspaceArea()->subWindowList())
         if (child->property("plotName")=="ReviewOrbit") plot=dynamic_cast<PlotWidget *>(child->widget());
      require(plot,"Actual orbit playback widget is missing");
      QJsonArray replay;
      for (int position:{0,250,500,750,1000}) {
         receiver->setReplayPosition(position); events();
         QJsonObject sample{{"position",position}};
         for (const auto &curve:model->curves) {
            const auto *pose=orbitObjectPose(curve,plot->canvas()->frame());
            require(pose,"Playback dropped a synchronized object");
            require(pose->epoch>=first-1e-9 && pose->epoch<=first+120./86400+1e-9,"Playback left the common two-minute interval");
            sample.insert(curve.name,pose->epoch);
         }
         replay.append(sample);
      }
      summary.insert("replay",replay);
   }
   const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
   if (!directory.isEmpty()) {
      QDir().mkpath(directory);
      QFile output(directory+(aligned ? "/timeline-after.json" : "/timeline-before.json"));
      require(output.open(QIODevice::WriteOnly),"Cannot save timeline evidence");
      output.write(QJsonDocument(summary).toJson());
   }
   std::cout<<(aligned ? "PASS common-epoch synchronized mission and playback\n" : "Recorded original mixed-epoch mission\n");
}
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("SatelliteWorkflow");TestSettings settings;
   try {
      require(argc==3 || argc==4,"Supply the startup file and workflow check name");
      const auto mode=QString::fromLocal8Bit(argv[2]);
      const auto timelinePath=argc==4 ? QFileInfo(argv[3]).absoluteFilePath() : QString();
      if (mode=="historical") {
         require(!timelinePath.isEmpty(),"Supply a historical-import output directory");
         qputenv("XDG_CACHE_HOME",(timelinePath+"/cache").toUtf8());
      }
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      QTemporaryDir scratch;require(scratch.isValid(),"Cannot create import fixture");
      QFile input(scratch.filePath("vanguard.tle"));require(input.open(QIODevice::WriteOnly),"Cannot write TLE fixture");
      input.write("Vanguard\n1 00005U 58002B   00179.78495062  .00000023  00000-0  28098-4 0  4753\n2 00005  34.2682 348.7242 1859667 331.7664  19.3264 10.82419157413667\n");input.close();
      MainWindow window;window.resize(1500,950);window.show();require(window.initialize(startup),"Cannot initialize import workflow");events();
      if (mode=="historical") { checkHistoricalImport(window,timelinePath);return 0; }
      if (mode=="timeline" || mode=="timeline-before") {
         require(!timelinePath.isEmpty(),"Supply a timeline mission path");
         checkTimeline(window,timelinePath,mode=="timeline"); return 0;
      }
      if (mode=="assignments") { checkAssignments(window,input.fileName(),scratch.path()); return 0; }
      if (mode=="elements") { checkElements(window,input.fileName(),scratch.path()); return 0; }
      if (mode=="epoch-guard") { checkEpochGuard(window,scratch.path(),input.fileName()); return 0; }
      auto *tree=window.findChild<QTreeWidget *>("Resources");require(tree,"Resource tree is missing");
      int resets=0;QObject::connect(tree->model(),&QAbstractItemModel::modelReset,&window,[&] {++resets;});
      select(tree,"DefaultSC");
      std::exception_ptr failure;
      const auto importRecord=[&] {
         auto *dialog=window.findChild<QDialog *>("satelliteImportDialog");
         try {
            require(dialog,"Satellite import dialog is missing");
            if (mode=="raw") {
               dialog->findChild<QTabWidget *>("satelliteImportSource")->setCurrentIndex(2);
               dialog->findChild<QLineEdit *>("importSatelliteName")->setText("Vanguard");
               dialog->findChild<QLineEdit *>("satelliteStateEpoch")->setText("2000-06-27T18:50:19.733568Z");
               const QStringList fields{"X","Y","Z","VX","VY","VZ"};
               const QStringList values{"7000","0","0","0","7.5","0"};
               for (int i=0;i<fields.size();++i) dialog->findChild<QLineEdit *>("satelliteState"+fields[i])->setText(values[i]);
            } else {
               dialog->findChild<QTabWidget *>("satelliteImportSource")->setCurrentIndex(1);
               dialog->findChild<QLineEdit *>("satelliteImportFile")->setText(input.fileName());
               dialog->findChild<QPushButton *>("readSatelliteFile")->click();
            }
            if (mode=="new" || mode=="none") {
               auto *target=dialog->findChild<QComboBox *>("satellitePropagationTarget");
               target->setCurrentIndex(target->findData(mode=="new" ? -1 : -2));
            }
            dialog->findChild<QPushButton *>("importSatellite")->click();
            require(!dialog->isVisible(),"Satellite import failed");
         } catch (...) {failure=std::current_exception();if(dialog)dialog->reject();}
      };
      if (mode=="nested") {
         QTimer::singleShot(0,&window,[&] {
            auto *creation=qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!creation) return;
            QTimer::singleShot(0,&window,importRecord);
            creation->findChild<QPushButton *>("newSpacecraftImport")->click();
            if(failure)creation->reject();
         });
         window.findChild<QAction *>("createResource")->trigger();
      } else {
         QTimer::singleShot(0,&window,importRecord);
         window.findChild<QAction *>("importSatelliteCatalog")->trigger();
      }
      if(failure)std::rethrow_exception(failure);events();
      require(!QApplication::activeModalWidget(),"Import left an active modal dialog");
      require(tree->currentItem() && tree->currentItem()->text(0)=="Vanguard" && tree->currentItem()->isSelected(),"Import did not select the new spacecraft");
      int assigned=0;
      for (const auto &node:window.missionSnapshot().nodes)
         if (node.type=="Propagate" && node.statement.contains("(Vanguard)")) ++assigned;
      require(assigned==(mode=="none" ? 0 : 1),"GUI import did not use the selected mission-propagation option");
      if (mode=="stopped") {
         auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
         auto source=editor->toPlainText();
         source.replace(QRegularExpression("DefaultSC\\.ElapsedSecs\\s*=\\s*[0-9.]+"),"DefaultSC.ElapsedSecs = 31536000");
         source.replace("BeginMissionSequence;","BeginMissionSequence;\nDefaultSC.A1ModJulian = Vanguard.A1ModJulian;");
         editor->setPlainText(source);
         QTimer::singleShot(100,&window,[&] {window.stopMission();});
         require(window.runMission()==MainWindow::RunResult::Stopped,"Mission did not stop for selection reproduction");events();
      }
      if (mode=="accessible") {
         auto *accessible=QAccessible::queryAccessibleInterface(tree);
         require(accessible,"Resource accessibility interface is missing");
         for (const auto &name:QStringList{"Vanguard","DefaultSC","VanguardProp"}) {
            QAccessibleInterface *row=nullptr;
            for (int i=0;i<accessible->childCount();++i)
               if (accessible->child(i)->text(QAccessible::Name)==name) row=accessible->child(i);
            require(row && row->actionInterface(),"Accessible resource row is missing");
            tree->clearSelection();
            row->actionInterface()->doAction(QAccessibleActionInterface::toggleAction());events();
            require(tree->selectedItems().size()==1 && tree->selectedItems().first()->text(0)==name,"Accessible resource selection jumped to a different row");
            require(tree->currentItem() && tree->currentItem()->text(0)==name,"Accessible selection left the keyboard focus on another row");
            window.findChild<QPlainTextEdit *>("scriptEditor")->setFocus(); events();
            tree->setFocus(); events();
            require(tree->currentItem() && tree->currentItem()->text(0)==name && tree->currentItem()->isSelected(),"Returning focus reset the resource selection");
         }
         std::cout<<"PASS accessible resource selections target the requested row\n";
      } else if (mode=="selection" || mode=="nested" || mode=="stopped") {
         const int before=resets;
         for (const auto &name:QStringList{"Vanguard","DefaultSC","Propagators","VanguardProp","Resources"}) select(tree,name);
         require(resets==before,"Selecting a resource rebuilt the tree");
         std::cout<<"PASS resource selections persist after satellite import\n";
      } else {
         std::cout<<"PASS GUI import uses the selected propagation option\n";
      }
      return 0;
   } catch (const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
