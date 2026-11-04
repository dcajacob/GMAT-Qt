#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ScriptCompatibility.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "Rmatrix.hpp"
#include "SpacePoint.hpp"
#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTableWidget>
#include <QTimer>
#include <QTreeWidget>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static GmatBase *object(const QString &name) { return Moderator::Instance()->GetConfiguredObject(name.toStdString()); }
static QTreeWidgetItem *item(QTreeWidget *tree,const QString &name)
{
   for (auto *found:tree->findItems(name,Qt::MatchExactly|Qt::MatchRecursive))
      if (!found->data(0,Qt::UserRole).toString().isEmpty()) return found;
   throw std::runtime_error(("Missing resource "+name).toStdString());
}
static ResourceEditor *panel(QDialog *dialog)
{
   for (auto *widget:dialog->findChildren<QWidget *>()) if (auto *found=dynamic_cast<ResourceEditor *>(widget)) return found;
   throw std::runtime_error("Unified clone form missing");
}
static void closePanels(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList())
      if (child->property("resourceName").isValid()) child->close();
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("QtResourceClone");
   try {
      require(argc==2 && acceptanceWidgetPlatform(),"Resource Clone must run offscreen with one startup argument");
      QTemporaryDir files; require(files.isValid(),"Temporary clone directory unavailable"); TestSettings settings;
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Clone runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Resources");
      const auto report=files.filePath("clone states.txt");
      const QString commands="Keep = Keep + 2; % retain mission expression α\nReport Values Keep Cells(1,1);\n";
      const QString mission="BeginMissionSequence;\n"+commands;
      QtCameraSetting camera; camera.fieldOfView=37; camera.objectLabels["Craft"]=false;
      QString baseline=qtCameraDirective("Display",camera)+
         "% preserve implicit defaults and unrelated configuration α\n"
         "Create Spacecraft Craft;\nCraft.Cr = 1.9000; % retain coefficient\nCreate Propagator Integrator;\n"
         "Create GroundStation Site Site2 Other; % grouped declaration\n"
         "Site.Id = 'Site reference text';\nOther.MinimumElevationAngle = 3; % unrelated resource\n"
         "GMAT Site.MinimumElevationAngle = 1.2e1; % retain elevation α\n"
         "Create Variable Keep Counter;\nKeep = 7.00; % preserve literal spelling\nCounter = 0;\n"
         "Create String Caption;\nCaption = 'Site and Keep remain literal';\n"
         "Create Array Cells[2,2] Unrelated[1,1];\nCells(1,1) = 2.5000; % retain cell precision\nCells(2,2) = 4;\n"
         "Create CoordinateSystem Frame;\nFrame.Origin = Mars;\nFrame.Axes = MJ2000Ec;\n"
         "Create OrbitView Display;\nDisplay.Add = {Craft, Earth};\nDisplay.ShowPlot = false;\n"
         "Create ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\n"+mission;
      baseline=setArrayExpressions(baseline,"Cells",R"([{"row":1,"column":2,"expression":"Keep*2"}])",2,2);
      editor->setPlainText(baseline);
      if (!window.buildScript()) {
         auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");
         throw std::runtime_error(("Clone baseline failed\n"+(messages ? messages->toPlainText() : QString("Message Window unavailable"))).toStdString());
      }
      auto *action=window.findChild<QAction *>("cloneResource"); require(action && action->shortcut()==QKeySequence("Ctrl+Shift+C"),"Clone action/shortcut missing");
      // Inspect the real context menu, including wx's protected-resource exclusion.
      const auto menuCheck=[&](const QString &name,bool enabled) {
         auto *selected=item(tree,name); tree->scrollToItem(selected); QApplication::processEvents(); std::exception_ptr failure;
         QTimer::singleShot(0,&window,[&] {
            auto *menu=qobject_cast<QMenu *>(QApplication::activePopupWidget());
            try { require(menu,"Resource menu missing"); QAction *clone=nullptr; for (auto *candidate:menu->actions()) if (candidate->objectName()=="cloneResource") clone=candidate; require(clone && clone->isEnabled()==enabled,"Clone availability differs from resource protection"); }
            catch (...) { failure=std::current_exception(); }
            if (menu) menu->close();
         });
         tree->customContextMenuRequested(tree->visualItemRect(selected).center()); if (failure) std::rethrow_exception(failure);
      };
      menuCheck("Site",true); menuCheck("EarthMJ2000Eq",false); menuCheck("Earth",false); menuCheck("Integrator",false);
      const auto cloneDialog=[&](const std::function<void(QDialog *)> &check) {
         tree->setCurrentItem(item(tree,"Site")); std::exception_ptr failure;
         QTimer::singleShot(0,&window,[&] {
            auto *dialog=window.findChild<QDialog *>("cloneResourceDialog");
            try { require(dialog,"Unified clone dialog missing"); check(dialog); } catch (...) { failure=std::current_exception(); }
            if (dialog && dialog->result()!=QDialog::Accepted) dialog->reject();
         });
         action->trigger(); if (failure) std::rethrow_exception(failure);
      };
      QString suggested;
      cloneDialog([&](QDialog *dialog) {
         auto *name=dialog->findChild<QLineEdit *>("resourceName"); suggested=name->placeholderText();
         require(!suggested.isEmpty() && !object(suggested) && name->text().isEmpty(),"Clone name is not optional/unused");
         auto *form=panel(dialog); form->findChild<QLineEdit *>("station_Id")->setText("Cancelled");
         require(form->hasChanges() && editor->toPlainText()==baseline && !object(suggested),"Draft clone registered or changed source before commit");
         dialog->reject();
      });
      require(editor->toPlainText()==baseline && !object(suggested),"Clone Cancel changed source/model");
      cloneDialog([&](QDialog *dialog) {
         auto *name=dialog->findChild<QLineEdit *>("resourceName"); auto *form=panel(dialog);
         auto *id=form->findChild<QLineEdit *>("station_Id"); auto *elevation=form->findChild<QLineEdit *>("station_MinimumElevationAngle");
         require(id && elevation && id->text()=="Site reference text" && elevation->text().toDouble()==12,"Clone form did not copy configured settings");
         auto *clone=dialog->findChild<QDialogButtonBox *>("resourceCloneButtons")->button(QDialogButtonBox::Ok);
         name->setText("Site"); clone->click(); require(dialog->isVisible() && editor->toPlainText()==baseline && dialog->findChild<QLabel *>("resourceCloneStatus")->text().contains("already"),"Duplicate clone changed source or closed form");
         name->clear(); id->setText("Copy station"); elevation->setText("91"); clone->click();
         require(dialog->isVisible() && editor->toPlainText()==baseline && !object(suggested) && form->hasChanges(),"Invalid clone settings did not stay pending");
         elevation->setText("7"); clone->click(); require(dialog->result()==QDialog::Accepted && name->text()==suggested,"Blank name did not use suggestion or correction did not commit");
      });
      const auto configured=editor->toPlainText();
      require(configured.startsWith(baseline.left(baseline.indexOf("BeginMissionSequence"))) && configured.endsWith(commands),"Clone rewrote original configuration or mission");
      require(object(suggested) && object(suggested)->GetStringParameter("Id")=="Copy station" && object(suggested)->GetRealParameter("MinimumElevationAngle")==7,"Clone did not retain mixed edited fields");
      require(object("Site")->GetStringParameter("Id")=="Site reference text" && object("Site")->GetRealParameter("MinimumElevationAngle")==12,"Clone changed original resource");
      require(!configured.contains(suggested+".Location1") && !configured.contains("Craft.DryMass"),"Clone made implicit defaults explicit");
      closePanels(window); editor->undo(); require(editor->toPlainText()==baseline,"Clone Undo not exact/atomic");
      require(!window.cloneResource("Site","Stale",configured).isEmpty() && editor->toPlainText()==baseline,"Clone accepted stale source");
      require(window.buildScript() && !object(suggested),"Undo rebuild retained clone"); editor->redo(); require(editor->toPlainText()==configured && window.buildScript(),"Clone Redo/rebuild not exact");
      // Only palette defaults depend on creation order. Freeze the original's
      // effective values before cloning, independently of the copied source.
      auto *originalPoint=dynamic_cast<SpacePoint *>(object("Site")); require(originalPoint,"Clone source palette unavailable");
      const auto originalColors=qMakePair(originalPoint->GetCurrentOrbitColor(),originalPoint->GetCurrentTargetColor());
      const auto colorSource="PlainCopy.OrbitColor = "+QString::fromStdString(originalPoint->GetOrbitColorString())+";\nPlainCopy.TargetColor = "+QString::fromStdString(originalPoint->GetTargetColorString())+";\n";
      auto before=editor->toPlainText(); auto error=window.cloneResource("Site","PlainCopy",before); require(error.isEmpty(),qPrintable(error));
      const QString copied=QString("Create GroundStation PlainCopy;\nPlainCopy.Id = 'Site reference text';\nGMAT PlainCopy.MinimumElevationAngle = 1.2e1; % retain elevation α\n")+colorSource;
      auto expected=before; expected.insert(expected.indexOf("BeginMissionSequence"),copied);
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      if (!evidence.isEmpty()) {
         const QMap<QString,QString> values{{"expected",expected},{"actual",editor->toPlainText()},
            {"original-native",QString::fromStdString(object("Site")->GetGeneratingString(Gmat::SHOW_SCRIPT))},
            {"clone-native",QString::fromStdString(object("PlainCopy")->GetGeneratingString(Gmat::SHOW_SCRIPT))}};
         for (auto it=values.cbegin();it!=values.cend();++it) { QFile file(QDir(evidence).filePath("plain-copy-"+it.key()+".script")); if (file.open(QIODevice::WriteOnly)) file.write(it.value().toUtf8()); }
      }
      require(editor->toPlainText()==expected,"Unchanged clone lost literal/reference/comment source or copied an unrelated assignment");
      auto *clonedPoint=dynamic_cast<SpacePoint *>(object("PlainCopy"));
      require(clonedPoint&&clonedPoint->GetCurrentOrbitColor()==originalColors.first&&clonedPoint->GetCurrentTargetColor()==originalColors.second,"Unchanged clone did not preserve original native palette");
      auto *retainedPoint=dynamic_cast<SpacePoint *>(object("Site"));
      require(retainedPoint&&retainedPoint->GetCurrentOrbitColor()==originalColors.first&&retainedPoint->GetCurrentTargetColor()==originalColors.second&&!before.contains("Site.OrbitColor =")&&!before.contains("Site.TargetColor ="),"Cloning changed the original implicit palette or its source");
      for (const auto &pair:QList<QPair<QString,QString>>{{"Keep","KeepCopy"},{"Caption","CaptionCopy"},{"Cells","CellsCopy"},{"Frame","FrameCopy"},{"Display","DisplayCopy"}}) {
         before=editor->toPlainText(); error=window.cloneResource(pair.first,pair.second,before); require(error.isEmpty(),qPrintable(error));
         require(editor->toPlainText().startsWith(before.left(before.indexOf("BeginMissionSequence"))) && editor->toPlainText().endsWith(commands),"Cloning ordinary resource rewrote prior source");
      }
      const auto cloned=editor->toPlainText();
      require(arrayExpressions(cloned,"CellsCopy")==arrayExpressions(cloned,"Cells") && cloned.indexOf("% GMAT-Qt-Array-Expressions CellsCopy begin")>cloned.indexOf("% GMAT-Qt-Array-Expressions Cells end"),"Managed array clone lost formulas/markers/dependency order");
      require(cloned.contains("KeepCopy = 7.00; % preserve literal spelling") && cloned.contains("CaptionCopy = 'Site and Keep remain literal'") && cloned.contains("Create Array CellsCopy[2,2];") && cloned.contains("CellsCopy(1,1) = 2.5000; % retain cell precision"),"Scalar/string/array clone lost original source semantics");
      require(object("CellsCopy")->GetRmatrixParameter("RmatValue")(0,0)==2.5 && object("FrameCopy")->GetStringParameter("Origin")=="Mars" && object("FrameCopy")->GetStringParameter("Axes")=="MJ2000Ec","Array/frame clone did not reconstruct configured values");
      require(qtCameraSettings(cloned).contains("DisplayCopy") && qtCameraSettings(cloned).value("DisplayCopy").fieldOfView==37 && !qtCameraSettings(cloned).value("DisplayCopy").objectLabels.value("Craft"),"Display clone lost Qt camera/drawing metadata");
      for (const auto &name:QStringList{"Earth","EarthMJ2000Eq","SolarSystem","Integrator"}) require(!window.cloneResource(name,"Protected",cloned).isEmpty() && editor->toPlainText()==cloned,"Clone allowed protected resources");
      require(!window.cloneResource("Site","Bad Name",cloned).isEmpty() && !window.cloneResource("Site","Propagate",cloned).isEmpty() && editor->toPlainText()==cloned,"Clone accepted invalid/reserved name");
      tree->itemDoubleClicked(item(tree,"CellsCopy"),0); ResourceEditor *arrayPanel=nullptr;
      for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("resourceName").toString()=="CellsCopy") arrayPanel=dynamic_cast<ResourceEditor *>(child->widget());
      require(arrayPanel,"Cloned array editor missing"); std::exception_ptr gridFailure;
      QTimer::singleShot(0,&window,[&] {
         auto *dialog=arrayPanel->findChild<QDialog *>("arrayExpressionDialog");
         try { require(dialog,"Cloned array expression grid missing"); auto *grid=dialog->findChild<QTableWidget *>("arrayExpressionGrid"); require(grid && grid->item(0,1)->text()=="Keep*2","Cloned array grid did not reopen its retained formula"); }
         catch (...) { gridFailure=std::current_exception(); }
         if (dialog) dialog->reject();
      });
      arrayPanel->findChild<QPushButton *>("arrayExpressions")->click(); if (gridFailure) std::rethrow_exception(gridFailure);
      require(!arrayPanel->hasChanges() && editor->toPlainText()==cloned,"Cloned expression grid Cancel changed source"); closePanels(window);
      // A matching configuration literal must not be mistaken for an explicit
      // mission boundary merely because the first mission command repeats it.
      const QString collision="Create Spacecraft Vehicle;\nVehicle.X = 8000;\nVehicle.Y = 1234;\nBeginMissionSequence;\nVehicle.X = 8000;\n";
      editor->setPlainText(collision); require(window.buildScript(),"Boundary-collision baseline failed");
      error=window.cloneResource("Vehicle","VehicleCopy",collision); require(error.isEmpty(),qPrintable(error));
      require(editor->toPlainText().contains("VehicleCopy.X = 8000;") && editor->toPlainText().contains("VehicleCopy.Y = 1234;") && object("VehicleCopy")->GetRealParameter("Y")==1234,"Clone stopped at repeated configuration assignment before explicit mission boundary");
      editor->undo(); require(editor->toPlainText()==collision,"Boundary Clone Undo not exact");
      editor->setPlainText(cloned); require(window.buildScript(),"Managed clone fixture restore failed");
      const auto saved=files.filePath("cloned mission α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && editor->toPlainText()==cloned && window.buildScript(),"Clone Unicode save/reopen not exact");
      require(window.runMission()==MainWindow::RunResult::Completed,"Cloned mission did not complete");
      auto *initializedArray=Moderator::Instance()->GetInternalObject("CellsCopy");
      require(initializedArray && initializedArray->GetRmatrixParameter("RmatValue")(0,1)==14,"Managed array clone formula did not initialize independently before original mission commands");
      QFile data(report); require(data.open(QIODevice::ReadOnly),"Clone report missing");
      const auto values=QString::fromUtf8(data.readAll()).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(values.size()==2 && values[0].toDouble()==9 && values[1].toDouble()==2.5,"Clone changed independent original mission calculations");
      std::cout<<"PASS actual Clone action/context protection, unified optional-name draft/Cancel/duplicate/invalid correction, original settings/implicit source, mixed commit/Undo/Redo/Unicode reopen, grouped declarations and unrelated configuration, scalar/string/array/frame copies, Qt display metadata and independent original calculations; no native launch or broad matrix.\n";
      return 0;
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
