#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "Spacecraft.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Epoch lifecycle artifact unavailable"); return file.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile file(path); require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Epoch lifecycle artifact write failed"); }
static void reveal(QWidget *widget) {
   require(widget,"Epoch lifecycle control missing");
   for (auto *parent=widget;parent;parent=parent->parentWidget()) {
      for (auto *tabs:widget->window()->findChildren<QTabWidget *>()) if (tabs->indexOf(parent)>=0) tabs->setCurrentWidget(parent);
      if (auto *scroll=qobject_cast<QScrollArea *>(parent)) scroll->ensureWidgetVisible(widget);
   }
   events(); require(widget->isVisible(),"Epoch lifecycle control hidden");
}

int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication app(argc,argv); app.setQuitOnLastWindowClosed(false);
   app.setOrganizationName("GMATTests"); app.setApplicationName("NasaEpochLifecycle");
   try {
      require(argc==4&&acceptanceWidgetPlatform(),"Supply epoch lifecycle startup, existing cases and format");
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Epoch lifecycle directory unavailable");
      const QString key=argv[3],startup=QFileInfo(argv[1]).absoluteFilePath();
      require(key=="TAIModJulian"||key=="UTCGregorian","Unknown epoch lifecycle format");
      QJsonObject spec;
      for (const auto &entry:QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object()["cases"].toArray())
         if (entry.toObject()["key"].toString()==key) spec=entry.toObject();
      require(!spec.isEmpty(),"Existing independent epoch case unavailable");
      const auto literal=spec["literal"].toString(); const auto truth=spec["expected"].toObject();
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      auto save=[&](const QString &name,const QByteArray &value) { if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),value); };
      const auto report=files.filePath("epoch lifecycle.csv"),saved=files.filePath("Epoch lifecycle Ω.script");
      const QString initial="% NASA epoch lifecycle Ω\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DateFormat = TAIModJulian;\nSat.Epoch = 21545;\nSat.X = 7000;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.5;\nSat.VZ = 1;\nCreate Variable Keep;\nKeep = 7; % preserve initializer\nCreate ReportFile EpochCheck;\nEpochCheck.Filename = '"+report+"';\nEpochCheck.WriteHeaders = false;\nEpochCheck.Precision = 17;\nEpochCheck.FixedWidth = false;\nEpochCheck.Delimiter = ',';\nBeginMissionSequence;\nKeep = Keep + 2; % preserve mission\nReport EpochCheck Sat.A1ModJulian Sat.TAIModJulian Sat.UTCModJulian Sat.TTModJulian Sat.TDBModJulian Keep;\n";
      QString expected=initial; expected.remove("Sat.Epoch = 21545;\n");
      QString fields;
      if (key!="TAIModJulian") { expected.remove("Sat.DateFormat = TAIModJulian;\n"); fields="Sat.DateFormat = "+key+";\n"; }
      fields+="Sat.Epoch = '"+literal+"';\n";
      expected.replace("BeginMissionSequence;\n",fields+"BeginMissionSequence;\n");
      save("initial.script",initial.toUtf8()); save("expected.script",expected.toUtf8());
      QDir::setCurrent(QFileInfo(startup).absolutePath()); MainWindow window;
      require(window.initialize(startup),"Epoch lifecycle initialization failed"); window.show(); acceptanceView(window); events();
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");
      auto *tree=window.findChild<QTreeWidget *>("Resources"); auto *area=window.findChild<QMdiArea *>("workspace");
      require(source&&tree&&area,"Epoch lifecycle workspace missing"); source->setPlainText(initial);
      require(window.buildScript()&&window.saveScriptTo(files.filePath("baseline.script")),"Epoch lifecycle baseline failed");
      auto model=[&] {
         auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("Sat")); require(sat,"Epoch lifecycle spacecraft unavailable");
         const double state[6]={7000,0,0,0,7.5,1}; const auto actual=sat->GetState("Cartesian");
         for (int i=0;i<6;++i) require(std::abs(actual[i]-state[i])<1e-12,"Epoch lifecycle operation changed orbit");
         return sat->GetRealParameter("A1Epoch");
      };
      const double baseline=21545.+.0343817/86400.,wanted=truth["A1"].toDouble();
      auto epochIs=[&](double value) { require(std::abs(model()-value)*86400.<=.000002,"Epoch lifecycle native epoch differs from independent oracle"); };
      epochIs(baseline);
      auto open=[&](const QString &name)->QMdiSubWindow * {
         const auto matches=tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); require(matches.size()==1,"Epoch lifecycle resource missing/ambiguous");
         tree->scrollToItem(matches.front()); tree->setCurrentItem(matches.front()); acceptanceView(window);
         tree->itemDoubleClicked(matches.front(),0); events(); auto *child=area->activeSubWindow();
         require(child&&dynamic_cast<ResourceEditor *>(child->widget())&&child->windowTitle().contains(name),"Epoch lifecycle resource MDI missing");
         child->showNormal(); child->move(0,0); child->resize(area->viewport()->size()); acceptanceView(*child); return child;
      };
      auto panel=[](QMdiSubWindow *child) { auto *value=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr; require(value,"Epoch lifecycle owner unavailable"); return value; };
      auto button=[&](QMdiSubWindow *child,QDialogButtonBox::StandardButton action) {
         auto *box=panel(child)->findChild<QDialogButtonBox *>("resourceButtons"); require(box,"Epoch lifecycle buttons unavailable");
         auto *value=box->button(action); reveal(value); return value;
      };
      auto click=[&](QMdiSubWindow *child,QDialogButtonBox::StandardButton action) { acceptanceView(*child); QTest::mouseClick(button(child,action),Qt::LeftButton); events(); };
      auto draft=[&](QMdiSubWindow *child,const QString &format,const QString &value) {
         auto *combo=panel(child)->findChild<QComboBox *>("resource_DateFormat");
         auto *input=panel(child)->findChild<QLineEdit *>("resource_Epoch"); require(combo&&input,"Epoch lifecycle primary controls unavailable");
         reveal(combo); combo->setCurrentText(format); events(); require(combo->currentText()==format,"Epoch lifecycle format unavailable");
         reveal(input); input->setFocus(); input->selectAll(); QTest::keyClicks(input,value); events();
         require(input->text()==value&&panel(child)->hasChanges(),"Epoch lifecycle typed draft lost"); acceptanceView(*child);
      };
      auto prompt=[&](const QString &title,QMessageBox::StandardButton choice,const std::function<bool()> &action) {
         bool visited=false; std::exception_ptr failure; QTimer timer; timer.setInterval(20);
         QObject::connect(&timer,&QTimer::timeout,&window,[&] {
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); if (!dialog) return;
            timer.stop(); try {
               auto *box=qobject_cast<QMessageBox *>(dialog); require(box&&box->windowTitle()==title,"Unexpected epoch lifecycle prompt");
               auto *selected=box->button(choice); require(selected,"Expected epoch lifecycle prompt choice missing");
               acceptanceView(*box); require(selected->isVisible(),"Epoch lifecycle prompt choice hidden"); visited=true;
               QTest::mouseClick(selected,Qt::LeftButton);
            } catch (...) { failure=std::current_exception(); dialog->reject(); }
         });
         timer.start(); const bool result=action(); timer.stop(); events(); if (failure) std::rethrow_exception(failure);
         require(visited,"Epoch lifecycle prompt not exercised"); return result;
      };
      QPointer<QMdiSubWindow> sibling=open("Keep"),child=open("Sat"); const int originalUndo=source->document()->availableUndoSteps();
      const bool cleanApplyEnabled=button(child,QDialogButtonBox::Apply)->isEnabled();
      auto pristine=[&] { epochIs(baseline); require(source->toPlainText()==initial&&source->document()->availableUndoSteps()==originalUndo&&!source->document()->isModified()&&sibling&&sibling->isVisible(),"Epoch lifecycle draft changed source/history or sibling"); };
      draft(child,key,literal); pristine(); click(child,QDialogButtonBox::Cancel);
      require(!child,"Epoch Cancel did not close its owner"); pristine();
      child=open("Sat"); draft(child,key,literal);
      require(!prompt("Unapplied changes",QMessageBox::Cancel,[&] { return child->close(); }),"Epoch close refusal did not veto");
      pristine(); require(child&&child->isVisible()&&panel(child)->hasChanges(),"Epoch close refusal lost pending owner");
      require(prompt("Unapplied changes",QMessageBox::Discard,[&] { return child->close(); }),"Epoch discard close was rejected");
      require(!child,"Epoch discard did not close its owner"); pristine();
      child=open("Sat"); draft(child,key,literal); click(child,QDialogButtonBox::Apply);
      require(child&&!panel(child)->hasChanges()&&source->toPlainText()==expected,"Epoch Apply did not commit exact expected source and retain owner"); epochIs(wanted);
      const int appliedUndo=source->document()->availableUndoSteps(); click(child,QDialogButtonBox::Apply);
      require(child&&!panel(child)->hasChanges()&&source->toPlainText()==expected&&source->document()->availableUndoSteps()==appliedUndo,"Repeated epoch Apply changed source/history or owner"); epochIs(wanted);
      draft(child,key,"not-an-epoch"); click(child,QDialogButtonBox::Cancel);
      require(!child&&source->toPlainText()==expected&&source->document()->availableUndoSteps()==appliedUndo&&sibling&&sibling->isVisible(),"Epoch Cancel lost prior Apply or committed pending input"); epochIs(wanted);
      source->undo(); require(source->toPlainText()==initial&&window.buildScript(),"Epoch lifecycle Undo was not exact"); events(); epochIs(baseline);
      source->redo(); require(source->toPlainText()==expected&&window.buildScript(),"Epoch lifecycle Redo was not exact"); events(); epochIs(wanted);
      source->undo(); require(source->toPlainText()==initial&&window.buildScript(),"Epoch OK baseline recovery failed"); events(); epochIs(baseline);
      sibling=open("Keep"); child=open("Sat"); draft(child,key,literal); click(child,QDialogButtonBox::Ok);
      require(!child&&sibling&&sibling->isVisible()&&source->toPlainText()==expected,"Epoch valid OK did not commit exact source and close only owner"); epochIs(wanted);
      child=open("Sat"); const int cleanUndo=source->document()->availableUndoSteps(); click(child,QDialogButtonBox::Ok);
      require(!child&&source->toPlainText()==expected&&source->document()->availableUndoSteps()==cleanUndo&&sibling,"Epoch clean OK created history or closed sibling");
      require(window.saveScriptTo(saved)&&read(saved)==expected.toUtf8(),"Epoch lifecycle Unicode save differs"); child=open("Sat");
      auto *openAction=window.findChild<QAction *>("openMissionScript"); require(openAction,"Actual File/Open action unavailable");
      QMenu *fileMenu=nullptr; for (auto *action:window.menuBar()->actions()) if (action->menu()&&action->menu()->actions().contains(openAction)) fileMenu=action->menu();
      require(fileMenu,"Actual File menu unavailable"); bool selectedFile=false; std::exception_ptr fileFailure; QTimer chooser; chooser.setInterval(20);
      QObject::connect(&chooser,&QTimer::timeout,&window,[&] {
         auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget()); if (!dialog) return;
         chooser.stop(); try {
            auto *file=qobject_cast<QFileDialog *>(dialog); require(file&&file->windowTitle()=="Open GMAT script","Unexpected epoch File/Open dialog");
            file->selectFile(saved); acceptanceView(*file);
            require(file->selectedFiles()==QStringList{saved},"Epoch File/Open selected a different Unicode file");
            auto *box=file->findChild<QDialogButtonBox *>(); auto *accept=box?box->button(QDialogButtonBox::Open):nullptr;
            require(accept&&accept->isVisible()&&accept->isEnabled(),"Epoch File/Open acceptance unavailable"); selectedFile=true; QTest::mouseClick(accept,Qt::LeftButton);
         } catch (...) { fileFailure=std::current_exception(); dialog->reject(); }
      });
      acceptanceView(window); QTest::mouseClick(window.menuBar(),Qt::LeftButton,Qt::NoModifier,window.menuBar()->actionGeometry(fileMenu->menuAction()).center());
      require(QTest::qWaitFor([&] { return fileMenu->isVisible(); },1500),"Epoch File menu did not expose");
      fileMenu->setActiveAction(openAction); acceptanceView(*fileMenu); chooser.start(); QTest::keyClick(fileMenu,Qt::Key_Return); chooser.stop(); events();
      if (fileFailure) std::rethrow_exception(fileFailure);
      require(selectedFile&&!child&&!sibling&&source->toPlainText()==expected&&!source->document()->isModified(),"Epoch File/Open with clean panels failed to reload/retire owners"); epochIs(wanted);
      save("applied.script",source->toPlainText().toUtf8());
      require(window.runMission()==MainWindow::RunResult::Completed,"Epoch lifecycle mission failed"); events();
      const auto raw=read(report); save("values.csv",raw); const auto values=QString::fromUtf8(raw).trimmed().split(',');
      require(values.size()==6&&values[5].trimmed().toDouble()==9,"Epoch lifecycle report/sentinel differs");
      const QStringList systems{"A1","TAI","UTC","TT","TDB"}; QJsonArray checked;
      for (int i=0;i<5;++i) {
         bool ok=false; const double value=values[i].trimmed().toDouble(&ok),limit=systems[i]=="TDB"?.00005:.000002;
         const double error=std::abs(value-truth[systems[i]].toDouble())*86400.;
         require(ok&&std::isfinite(value)&&error<=limit,"Epoch lifecycle report differs from independent five-system oracle");
         checked.append(QJsonObject{{"system",systems[i]},{"actual_mjd",value},{"expected_mjd",truth[systems[i]]},{"error_seconds",error},{"tolerance_seconds",limit}});
      }
      child=open("Sat"); draft(child,key,"not-an-epoch"); const int exitUndo=source->document()->availableUndoSteps();
      require(!prompt("Unapplied panel changes",QMessageBox::Cancel,[&] { return window.close(); }),"Epoch program-exit refusal did not veto");
      require(window.isVisible()&&child&&panel(child)->hasChanges()&&source->toPlainText()==expected&&source->document()->availableUndoSteps()==exitUndo&&read(saved)==expected.toUtf8(),"Epoch refused exit lost draft/source/history/file"); epochIs(wanted);
      require(prompt("Unapplied panel changes",QMessageBox::Discard,[&] { return window.close(); }),"Epoch program discard-exit was rejected");
      require(!window.isVisible()&&!child&&source->toPlainText()==expected&&source->document()->availableUndoSteps()==exitUndo&&read(saved)==expected.toUtf8(),"Epoch accepted exit committed draft or changed saved source/history"); epochIs(wanted);
      const QJsonObject proof{{"checkpoint","epoch-lifecycle-"+key},{"outcome","passed"},{"family",key},{"numeric_sentinel",9},{"clean_apply_enabled",cleanApplyEnabled},{"report",checked},
         {"facets",QJsonArray{"epoch-repeat-apply-and-cancel-isolation","epoch-dirty-close-refuse-and-discard","epoch-valid-and-clean-ok-exact-source-history","actual-file-open-unicode-with-clean-panels","epoch-program-exit-refuse-and-discard","independent-five-time-system-result-and-orbit-isolation"}},
         {"scope","Selected TAI MJD/UTC Gregorian 2000 epochs reuse existing independent cases. Actual epoch controls, panel buttons and File/Open dialog; Qt close events and actual modal choices. No all-field clone, clean Apply disabled-state qualification, name-domain validation, unsaved-document Save-on-exit, all epochs/leap seconds, OS input, GPU/platform/stress or full-current-build qualification."}};
      save("proof.json",QJsonDocument(proof).toJson()); std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl; return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl; }
     catch (const std::exception &error) { std::cerr<<"FAIL "<<error.what()<<std::endl; }
   return 1;
}
