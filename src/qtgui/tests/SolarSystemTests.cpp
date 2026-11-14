#include "MainWindow.hpp"
#include "SolarSystemPanel.hpp"
#include "UserParameter.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "Spacecraft.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include "AcceptanceView.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextDocument>
#include <QApplication>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QElapsedTimer>
#include <QThread>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <iostream>
#include <cmath>
#include <stdexcept>
static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static QString bytes(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Report unavailable"); return QString::fromUtf8(file.readAll()); }
static SolarSystem *system() { return Moderator::Instance()->GetSolarSystemInUse(); }
static SolarSystemPanel *open(MainWindow &window)
{
   auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto items=tree->findItems("SolarSystem",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"SolarSystem tree entry missing");
   tree->itemDoubleClicked(items.first(),0); QApplication::processEvents(); auto *workspace=window.findChild<QMdiArea *>("workspace");
   require(workspace->activeSubWindow(),"Solar-system MDI window missing"); auto *panel=dynamic_cast<SolarSystemPanel *>(workspace->activeSubWindow()->widget()); require(panel,"Solar-system panel missing"); return panel;
}
static void close(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (auto *panel=dynamic_cast<EditablePanel *>(child->widget())) { panel->discardChanges(); child->close(); }
   QApplication::processEvents();
}
template<class Open,class Inspect> static void modal(QWidget &owner,Open open,Inspect inspect)
{
   bool inspected=false; std::exception_ptr failure; QTimer timer; timer.setSingleShot(true);
   QObject::connect(&timer,&QTimer::timeout,&owner,[&] {
      auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
      try { require(dialog,"Expected modal missing"); inspected=true; inspect(dialog); } catch (...) { failure=std::current_exception(); }
      if (dialog && dialog->isVisible()) dialog->reject();
   }); timer.start(0); open(); timer.stop(); if (failure) std::rethrow_exception(failure); require(inspected,"Expected modal did not open");
}
static void capture(QWidget &widget,const QString &path)
{
   if (path.isEmpty()) return; QElapsedTimer elapsed; elapsed.start(); while (elapsed.elapsed()<250) { QApplication::processEvents(); QThread::msleep(5); }
   require(widget.grab().save(path),"Solar-system screenshot failed");
}
static void apply(SolarSystemPanel &panel)
{
   panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
   require(!panel.hasChanges(),qPrintable(panel.findChild<QLabel *>("solarSystemStatus")->text())); QApplication::processEvents();
}
static QString settingsScript(const QString &source,const QString &de,const QString &spk,const QString &pck,bool tt,double interval,const QString &fallback="DE405")
{
   // Independent scripted route: no Qt serializer/configuration helper.
   QString result="SolarSystem.EphemerisSource = "+(source=="SPICE" ? fallback : source)+";\nSolarSystem.DEFilename = '"+de+"';\n";
   if (source=="SPICE") result+="SolarSystem.EphemerisSource = SPICE;\n";
   return result+"SolarSystem.SPKFilename = '"+spk+"';\nSolarSystem.PCKFilename = '"+pck+"';\nSolarSystem.UseTTForEphemeris = "+(tt ? "true" : "false")+";\nSolarSystem.EphemerisUpdateInterval = "+QString::number(interval,'g',17)+";\n";
}
static QMap<QString,QList<double>> allBodyStates(const A1Mjd &epoch)
{
   QMap<QString,QList<double>> result;
   for (const auto &name:QStringList{"Sun","Mercury","Venus","Earth","Luna","Mars","Jupiter","Saturn","Uranus","Neptune","Pluto"}) {
      auto *body=system()->GetBody(name.toStdString()); require(body,"Default body missing for ephemeris comparison");
      const auto state=body->GetMJ2000State(epoch); QList<double> values;
      for (int i=0;i<6;++i) { require(std::isfinite(state[i]),"Body ephemeris contains a nonfinite value"); values.append(state[i]); }
      result.insert(name,values);
   }
   return result;
}
static void solarProof(const QString &operation)
{
   const QJsonObject record{{"checkpoint","solar."+operation},{"operation",operation},{"outcome","passed"},
      {"facets",QJsonArray{"gui-controls","model-readback","independent-script-route","mission-result","source-preservation"}}};
   std::cout<<"NASA_CHECK "<<QJsonDocument(record).toJson(QJsonDocument::Compact).constData()<<std::endl;
}
static void solarControlCases(MainWindow &window,QPlainTextEdit &editor,const QString &original,
                              const SolarSystemSettings &baseline,const QString &report,const QString &saved)
{
   close(window); editor.setPlainText(settingsScript("DE405",baseline.deFiles.value("DE405"),
      baseline.values.value("SPKFilename"),baseline.values.value("PCKFilename"),true,2.5)+original);
   require(window.buildScript(),"Solar control baseline failed");
   const auto valid=editor.toPlainText(); const auto before=solarSystemSettings(*system()).values;
   const auto steps=editor.document()->availableUndoSteps(); auto *panel=open(window); acceptanceView(*panel);
   auto *interval=panel->findChild<QLineEdit *>("ephemerisUpdateInterval");
   for (const auto &value:QStringList{"","not-a-number","nan","inf","-inf","1e999","-1e-12"}) {
      interval->setText(value); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
      require(panel->hasChanges() && interval->text()==value && !panel->findChild<QLabel *>("solarSystemStatus")->text().isEmpty(),"Invalid interval lost correction input or diagnostic");
      require(editor.toPlainText()==valid && editor.document()->availableUndoSteps()==steps && solarSystemSettings(*system()).values==before,"Invalid interval changed source, history or model");
   }
   close(window);
   for (bool tt:{false,true}) for (double value:{0.0,1e-12,2.5}) {
      editor.setPlainText(original); require(window.buildScript(),"Solar boundary fixture failed");
      panel=open(window); acceptanceView(*panel); panel->findChild<QComboBox *>("ephemerisSource")->setCurrentText("DE405");
      panel->findChild<QLineEdit *>("ephemerisUpdateInterval")->setText(QString::number(value,'g',17));
      auto *flag=panel->findChild<QCheckBox *>("useTTForEphemeris");
      if (flag->isChecked()!=tt) flag->click();
      apply(*panel); const auto configured=editor.toPlainText();
      require(system()->GetEphemUpdateInterval()==value && system()->GetBooleanParameter("UseTTForEphemeris")==tt,"Solar control did not retain independently specified interval and TT flag");
      require(configured.endsWith(original.mid(original.indexOf("BeginMissionSequence"))),"Solar controls changed mission source");
      QFile::remove(report); require(!QFileInfo::exists(report) && window.runMission()==MainWindow::RunResult::Completed,"Solar boundary mission failed");
      const auto guiReport=bytes(report); const auto fields=guiReport.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(fields.size()==13,"Solar boundary report columns wrong");
      const A1Mjd epoch(fields[12].toDouble()); const auto guiStates=allBodyStates(epoch);
      require(window.saveScriptTo(saved) && window.loadScript(saved) && editor.toPlainText()==configured && system()->GetEphemUpdateInterval()==value && system()->GetBooleanParameter("UseTTForEphemeris")==tt,"Solar controls did not persist through save/reopen");
      editor.setPlainText(settingsScript("DE405",baseline.deFiles.value("DE405"),baseline.values.value("SPKFilename"),baseline.values.value("PCKFilename"),tt,value)+original);
      QFile::remove(report); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==guiReport,"GUI interval/TT mission differs from independent script route");
      require(allBodyStates(epoch)==guiStates,"Default-body states differ between GUI and independent script route");
      close(window);
   }
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtSolarSystem");
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid() && argc>1,"Solar-system fixtures unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(),screenshots=argc>2 ? QString::fromLocal8Bit(argv[2]) : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Solar-system runtime initialization failed"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto report=files.filePath("solar.txt"),saved=files.filePath("solar ü.script");
      const QString resources="% solar-system round trip Δ\nCreate Spacecraft Sat;\nCreate ForceModel FM;\nFM.PointMasses = {Sun, Luna};\nCreate Propagator Prop;\nProp.FM = FM;\nCreate CoordinateSystem SunFrame;\nSunFrame.Origin = Sun;\nSunFrame.Axes = MJ2000Eq;\nCreate CoordinateSystem MoonFrame;\nMoonFrame.Origin = Luna;\nMoonFrame.Axes = MJ2000Eq;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\n";
      const QString mission="BeginMissionSequence; % retain mission text\nPropagate 'Solar run' Prop(Sat) {Sat.ElapsedSecs = 120}; % retain command comment\nReport Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.SunFrame.X Sat.SunFrame.Y Sat.SunFrame.Z Sat.MoonFrame.X Sat.MoonFrame.Y Sat.MoonFrame.Z Sat.EarthFixed.X Sat.EarthFixed.Y Sat.EarthFixed.Z Sat.A1ModJulian;\n";
      const auto original=resources+mission; editor->setPlainText(original); require(window.saveScriptTo(saved) && window.buildScript(),"Solar-system mission failed to build");
      auto baseline=solarSystemSettings(*system()); require(baseline.sources.contains("DE405") && baseline.sources.contains("DE421") && baseline.sources.contains("DE424") && baseline.sources.contains("SPICE"),"Linux source choices unavailable");
      std::cout<<"Runtime sources: "<<baseline.sources.join(", ").toStdString()<<'\n';
      auto *panel=open(window); require(!panel->hasChanges(),"New panel already dirty"); auto *combo=panel->findChild<QComboBox *>("ephemerisSource"); auto *de=panel->findChild<QLineEdit *>("deFilename");
      const auto oldDE=de->text(); combo->setCurrentText("DE421"); require(de->text()==baseline.deFiles.value("DE421"),"DE source did not select corresponding file");
      const auto customDE=files.filePath("DE421 copied ü.bin"); require(QFile::copy(de->text(),customDE),"Cannot copy DE421 fixture"); de->setText(customDE); combo->setCurrentText("DE424"); combo->setCurrentText("DE421"); require(de->text()==customDE,"Switching sources lost pending filename");
      require(panel->hasChanges() && system()->GetStringParameter("EphemerisSource")==baseline.values.value("EphemerisSource").toStdString() && editor->toPlainText()==original,"Pending source changed model/script");
      modal(*panel,[&] { panel->findChild<QPushButton *>("deFilenameBrowse")->click(); },[](QDialog *dialog) { require(qobject_cast<QFileDialog *>(dialog),"DE Browse missing"); }); require(de->text()==customDE,"Browse Cancel discarded pending file");
      modal(*panel,[&] { panel->findChild<QPushButton *>("deFilenameBrowse")->click(); },[&](QDialog *dialog) { auto *chooser=qobject_cast<QFileDialog *>(dialog); chooser->selectFile(customDE); static_cast<QDialog *>(chooser)->accept(); }); require(de->text()==customDE,"Browse acceptance changed filename");
      modal(*panel,[&] { panel->findChild<QPushButton *>("showScript")->click(); },[&](QDialog *dialog) { const auto text=dialog->findChild<QPlainTextEdit *>("inspectionText")->toPlainText(); require(!text.contains(customDE),"Applied preview includes pending edits"); });
      capture(*panel->parentWidget(),screenshots.isEmpty() ? QString() : screenshots+".de.png");
      modal(*panel,[&] { panel->parentWidget()->close(); },[](QDialog *dialog) { auto *question=qobject_cast<QMessageBox *>(dialog); require(question,"Pending-close confirmation missing"); question->button(QMessageBox::Cancel)->click(); }); require(panel->isVisible() && panel->hasChanges(),"Close Cancel lost edits");
      modal(*panel,[&] { panel->parentWidget()->close(); },[](QDialog *dialog) { auto *question=qobject_cast<QMessageBox *>(dialog); question->button(QMessageBox::Discard)->click(); }); QApplication::processEvents(); require(editor->toPlainText()==original,"Discard changed source");
      // Exercise every installed DE source using the GUI and compare the full
      // report to a separately written script through the same GMAT engine.
      QStringList reports;
      for (const auto &source:QStringList{"DE405","DE421","DE424"}) {
         close(window); editor->setPlainText(original); require(window.buildScript(),"DE baseline restore failed"); panel=open(window); panel->findChild<QComboBox *>("ephemerisSource")->setCurrentText(source);
         const auto filename=source=="DE421" ? customDE : baseline.deFiles.value(source); panel->findChild<QLineEdit *>("deFilename")->setText(filename); panel->findChild<QLineEdit *>("ephemerisUpdateInterval")->setText("1.25"); panel->findChild<QCheckBox *>("useTTForEphemeris")->setChecked(true); apply(*panel);
         const auto configured=editor->toPlainText(); require(configured.endsWith(mission) && configured.contains("% solar-system round trip Δ"),"GUI solar edit changed mission/comments"); editor->undo(); require(editor->toPlainText()==original,"Solar-system Undo not atomic/exact"); editor->redo(); require(editor->toPlainText()==configured,"Solar-system Redo not exact");
         require(window.runMission()==MainWindow::RunResult::Completed,"GUI DE mission failed"); const auto guiReport=bytes(report); reports.append(guiReport);
         const auto fields=guiReport.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(fields.size()==13,"Solar report columns wrong");
         const A1Mjd epoch(fields[12].toDouble()); const auto earthSun=system()->GetBody("Earth")->GetMJ2000State(epoch)-system()->GetBody("Sun")->GetMJ2000State(epoch),earthMoon=system()->GetBody("Earth")->GetMJ2000State(epoch)-system()->GetBody("Luna")->GetMJ2000State(epoch);
         const auto guiStates=allBodyStates(epoch);
         for (int i=0;i<3;++i) { require(std::abs(fields[i+3].toDouble()-fields[i].toDouble()-earthSun[i])<2e-5,"Sun-frame output disagrees with body ephemeris"); require(std::abs(fields[i+6].toDouble()-fields[i].toDouble()-earthMoon[i])<2e-5,"Moon-frame output disagrees with body ephemeris"); }
         require(window.saveScriptTo(saved) && window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed && bytes(report)==guiReport,"Solar-system Unicode save/reopen changed report");
         editor->setPlainText(settingsScript(source,filename,baseline.values.value("SPKFilename"),baseline.values.value("PCKFilename"),true,1.25)+original); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==guiReport,"GUI/raw-script DE calculations differ");
         require(allBodyStates(epoch)==guiStates,"DE default-body states differ between GUI and independent script route");
      }
      require(reports[0]!=reports[1] && reports[1]!=reports[2],"DE source selection did not affect ephemeris outputs");
      // Paired SPICE source/SPK/PCK selection and retained DE fallback.
      close(window); editor->setPlainText(original); require(window.buildScript(),"SPICE baseline restore failed"); panel=open(window); const auto spice=files.filePath("planetary ü.bsp"),pck=files.filePath("constants ü.tpc");
      require(QFile::copy(baseline.values.value("SPKFilename"),spice) && QFile::copy(baseline.values.value("PCKFilename"),pck),"Cannot copy SPICE kernel fixtures"); panel->findChild<QComboBox *>("ephemerisSource")->setCurrentText("DE421"); panel->findChild<QLineEdit *>("deFilename")->setText(customDE); panel->findChild<QComboBox *>("ephemerisSource")->setCurrentText("SPICE"); require(!panel->findChild<QLineEdit *>("deFilename")->isEnabled() && panel->findChild<QLineEdit *>("spkFilename")->isEnabled(),"SPICE dependent fields wrong"); panel->findChild<QLineEdit *>("spkFilename")->setText(spice); panel->findChild<QLineEdit *>("pckFilename")->setText(pck); panel->findChild<QLineEdit *>("ephemerisUpdateInterval")->setText("0");
      for (const auto &pair:QList<QPair<QString,QString>>{{"spkFilename",spice},{"pckFilename",pck}}) {
         auto *input=panel->findChild<QLineEdit *>(pair.first); const auto pending=input->text();
         modal(*panel,[&] { panel->findChild<QPushButton *>(pair.first+"Browse")->click(); },[](QDialog *dialog) { require(qobject_cast<QFileDialog *>(dialog),"SPICE kernel file picker missing"); });
         require(input->text()==pending,"Kernel picker Cancel changed pending filename");
         modal(*panel,[&] { panel->findChild<QPushButton *>(pair.first+"Browse")->click(); },[&](QDialog *dialog) { auto *picker=qobject_cast<QFileDialog *>(dialog); require(picker,"SPICE kernel file picker missing"); picker->selectFile(pair.second); static_cast<QDialog *>(picker)->accept(); });
         require(input->text()==pair.second,"Kernel file picker did not retain the selected Unicode filename");
      }
      capture(*panel->parentWidget(),screenshots.isEmpty() ? QString() : screenshots+".spice.png"); apply(*panel); const auto configured=editor->toPlainText(); require(window.runMission()==MainWindow::RunResult::Completed,"SPICE mission failed"); const auto expected=bytes(report);
      const auto spiceColumns=expected.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(spiceColumns.size()==13,"SPICE report columns wrong");
      const A1Mjd spiceEpoch(spiceColumns[12].toDouble()); const auto spiceStates=allBodyStates(spiceEpoch);
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.runMission()==MainWindow::RunResult::Completed && bytes(report)==expected,"SPICE save/reopen changed report"); panel=open(window); require(panel->findChild<QLineEdit *>("deFilename")->text()==customDE,"SPICE lost explicit DE fallback"); close(window);
      editor->setPlainText(settingsScript("SPICE",customDE,spice,pck,false,0,"DE421")+original); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==expected,"GUI/raw-script SPICE calculations differ");
      require(allBodyStates(spiceEpoch)==spiceStates,"SPICE default-body states differ between GUI and independent script route");
      // Unrelated edits use engine serialization. Its SPICE/DE field order
      // cannot rebuild a custom fallback unless the Qt route preserves it.
      auto beforeOtherEdit=editor->toPlainText(); require(window.applyResourceChanges("Sat",{{"Cd","2.7"}},beforeOtherEdit).isEmpty(),"Unrelated spacecraft edit broke custom SPICE/DE fallback"); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==expected,"Unrelated drag-coefficient edit changed this no-drag mission");
      require(window.createResource("Variable","Unused",editor->toPlainText()).isEmpty() && window.deleteResource("Unused",editor->toPlainText()).isEmpty(),"Unrelated create/delete broke custom SPICE/DE fallback"); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==expected,"Create/delete changed SPICE report");
      auto snapshot=window.missionSnapshot(); int command=-1; for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type=="Report") command=i;
      require(command>=0 && window.applyMissionChange(snapshot,command,MissionEdit::Replace,snapshot.nodes[command].statement).isEmpty(),"Mission edit broke custom SPICE/DE fallback"); require(window.runMission()==MainWindow::RunResult::Completed && bytes(report)==expected,"Mission round trip changed SPICE report");
      // Reject invalid scalar settings without changing source/undo or model.
      const auto valid=editor->toPlainText(); const auto settingsBefore=solarSystemSettings(*system()).values;
      const auto truncated=files.filePath("truncated.de"); { QFile file(truncated); require(file.open(QIODevice::WriteOnly),"Cannot create truncated DE fixture"); file.write("incomplete header"); }
      for (const auto &bad:QList<QMap<QString,QString>>{{{"EphemerisUpdateInterval","-1"}},{{"EphemerisUpdateInterval","nan"}},{{"EphemerisUpdateInterval","inf"}},{{"EphemerisSource","Unknown"}},{{"UseTTForEphemeris","maybe"}},{{"LSKFilename","anything"}},{{"DEFilename",files.filePath("missing.de")}},{{"DEFilename",truncated}},{{"SPKFilename",files.filePath("missing.bsp")}},{{"PCKFilename",spice}},{{"SPKFilename",pck}}}) {
         const auto error=window.applyResourceChanges("SolarSystem",bad,valid); require(!error.isEmpty() && editor->toPlainText()==valid && solarSystemSettings(*system()).values==settingsBefore,"Invalid solar settings changed model/source");
      }
      panel=open(window); panel->findChild<QLineEdit *>("ephemerisUpdateInterval")->setText("-2"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(panel->hasChanges() && !panel->findChild<QLabel *>("solarSystemStatus")->text().isEmpty() && editor->toPlainText()==valid,"Invalid GUI interval discarded pending edit"); panel->findChild<QLineEdit *>("ephemerisUpdateInterval")->setText("2.5"); apply(*panel); require(system()->GetEphemUpdateInterval()==2.5,"Invalid-edit correction failed");
      require(window.runMission()==MainWindow::RunResult::Completed,"Solar-system correction did not recover mission");
      // Existing effective/repeated settings, repeated statements and comments
      // remain readable while ordered replacement leaves commands untouched.
      close(window); const auto commented=resources+"SolarSystem.EphemerisSource = DE421; % old source\nSolarSystem.DEFilename = '"+customDE+"'; % keep old file comment\nSolarSystem.EphemerisUpdateInterval = 0; % earlier interval\nSolarSystem.EphemerisUpdateInterval = 2; % effective interval\n% SolarSystem.UseTTForEphemeris = true; fake comment\n"+mission; editor->setPlainText(commented); require(window.buildScript(),"Commented settings fixture failed");
      require(window.applyResourceChanges("SolarSystem",{{"EphemerisUpdateInterval","3.125"}},commented).isEmpty(),"Repeated/inline solar settings edit failed"); const auto changed=editor->toPlainText(); require(changed.endsWith(mission) && changed.contains("% old source") && changed.contains("% keep old file comment") && changed.contains("% effective interval") && changed.contains("% SolarSystem.UseTTForEphemeris = true; fake comment"),"Solar-system edit lost comments/mission"); editor->undo(); require(editor->toPlainText()==commented,"Commented solar Undo not exact"); editor->redo(); require(editor->toPlainText()==changed,"Commented solar Redo not exact");
      require(!window.deleteResource("SolarSystem",changed).isEmpty(),"Intrinsic SolarSystem was deletable"); close(window);
      require(window.applyResourceChanges("SolarSystem",{{"EphemerisUpdateInterval","4"}},changed).isEmpty(),"Repeated solar Apply failed"); const auto repeated=editor->toPlainText(); require(window.applyResourceChanges("SolarSystem",{{"EphemerisUpdateInterval","3.125"}},repeated).isEmpty() && editor->toPlainText()==changed,"Repeated Apply accumulated empty configuration lines");
      // The same preservation helper supports a GMAT implicit mission boundary.
      const auto implicit=resources+"SolarSystem.EphemerisUpdateInterval = 0;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 120}; % implicit boundary\nReport Values Sat.EarthMJ2000Eq.X;\n"; editor->setPlainText(implicit); require(window.buildScript(),"Implicit solar mission failed"); require(window.applyResourceChanges("SolarSystem",{{"EphemerisUpdateInterval","1"}},implicit).isEmpty() && editor->toPlainText().endsWith(implicit.mid(implicit.indexOf("Propagate Prop"))),"Implicit mission boundary was rewritten");
      require(window.runMission()==MainWindow::RunResult::Completed,"Implicit solar mission failed after Apply");
      close(window); editor->setPlainText(original); require(window.buildScript(),"Stale panel fixture failed"); panel=open(window); panel->findChild<QLineEdit *>("ephemerisUpdateInterval")->setText("10"); editor->appendPlainText("% another script edit"); const auto stale=editor->toPlainText(); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(panel->hasChanges() && editor->toPlainText()==stale && panel->findChild<QLabel *>("solarSystemStatus")->text().contains("mission has changed",Qt::CaseInsensitive),"Stale solar panel was applied"); close(window);
      const QString loop="Create Variable n;\nBeginMissionSequence;\nWhile n < 1e12;\nn = n + 1;\nEndWhile;\n"; editor->setPlainText(loop); require(window.buildScript(),"Stop fixture failed"); panel=open(window); bool guarded=false;
      QTimer::singleShot(20,&window,[&] { guarded=!panel->isEnabled() && !window.applyResourceChanges("SolarSystem",{{"EphemerisUpdateInterval","3"}},loop).isEmpty(); window.stopMission(); }); require(window.runMission()==MainWindow::RunResult::Stopped && guarded && panel->isEnabled(),"Solar panel not guarded during run/re-enabled after Stop"); close(window);
      editor->setPlainText(original); require(window.runMission()==MainWindow::RunResult::Completed,"Solar mission did not recover after Stop");
      solarControlCases(window,*editor,original,baseline,report,saved);
      for (const auto &mode:QStringList{"DE405","DE421","DE424","SPICE"}) solarProof(mode);
      solarProof("EphemerisUpdateInterval"); solarProof("UseTTForEphemeris");
      solarProof("DEFilename"); solarProof("SPKFilename");
      std::cout<<"Qt solar system: DE405/421/424 and SPICE, paired Unicode files, source-dependent/pending controls, picker Cancel/accept, close Discard/Cancel, Apply/correction/rollback, exact Undo/Redo, save/reopen, independent reports/body-frame checks and comment/implicit mission preservation passed\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
