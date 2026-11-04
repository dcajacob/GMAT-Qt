#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ScriptCompatibility.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTableWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static GmatBase *object() { return Moderator::Instance()->GetConfiguredObject("Display"); }
static void preserve(const QString &name,const QByteArray &bytes)
{
   const auto path=qEnvironmentVariable("GMAT_QT_STAR_OPTIONS_EVIDENCE"); if (path.isEmpty()) return;
   require(QDir().mkpath(path),"Cannot create star editor evidence directory"); QFile file(path+"/editor-"+name); require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Cannot preserve star editor evidence");
}
static QLineEdit *field(QDialog *form,const char *name)
{
   auto *edit=form->findChild<QLineEdit *>(name); require(edit,"Star options field missing"); return edit;
}
static QComboBox *preset(QDialog *form)
{
   auto *choice=form->findChild<QComboBox *>("starPreset"); require(choice,"Star preset missing"); return choice;
}
static void values(QDialog *form,const QtStarSetting &expected)
{
   require(preset(form)->currentText()==expected.preset && field(form,"starCatalog")->text()==expected.catalog && field(form,"starCustomCount")->text().toInt()==expected.customCount &&
      field(form,"starMinMagnitude")->text().toDouble()==expected.minMagnitude && field(form,"starMaxMagnitude")->text().toDouble()==expected.maxMagnitude &&
      field(form,"starMinPixels")->text().toDouble()==expected.minPixels && field(form,"starMaxPixels")->text().toDouble()==expected.maxPixels && field(form,"starMinDimRatio")->text().toDouble()==expected.minDimRatio,
      "Star options did not retain all custom values");
   const bool custom=expected.preset=="Custom";
   require(field(form,"starCustomCount")->isEnabled()==custom && field(form,"starCatalog")->isEnabled()==custom && form->findChild<QPushButton *>("starCatalogBrowse")->isEnabled()==custom,"Star preset dependency state wrong");
}
static void closePanels(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("configurationPanel").toBool()) { if (auto *panel=dynamic_cast<ResourceEditor *>(child->widget())) panel->discardChanges(); child->close(); }
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static void dialog(ResourceEditor *panel,const char *action,const char *name,bool accept,const std::function<void(QDialog *)> &edit)
{
   auto *tabs=panel->findChild<QTabWidget *>("resourceEditorTabs"); require(tabs && tabs->count()>1,"Advanced resource tab missing"); tabs->setCurrentIndex(tabs->count()-1);
   auto *button=panel->findChild<QPushButton *>(action); require(button,"Orbit dialog action missing"); std::exception_ptr failure;
   QTimer::singleShot(0,panel,[&] {
      auto *form=panel->findChild<QDialog *>(name);
      try {
         require(form,"Orbit dialog missing"); edit(form); auto *buttons=form->findChild<QDialogButtonBox *>(); require(buttons,"Orbit dialog buttons missing");
         buttons->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); require(form->result()==(accept ? QDialog::Accepted : QDialog::Rejected),"Orbit dialog did not finish");
      } catch (...) { failure=std::current_exception(); if (form) form->reject(); }
   });
   button->click(); if (failure) std::rethrow_exception(failure);
}
static void stars(ResourceEditor *panel,bool accept,const std::function<void(QDialog *)> &edit) { dialog(panel,"editStarOptions","orbitStarOptionsDialog",accept,edit); }
static void catalogBrowse(QDialog *form,const QString &path)
{
   std::exception_ptr failure;
   QTimer::singleShot(0,form,[&] {
      auto *picker=qobject_cast<QFileDialog *>(QApplication::activeModalWidget());
      try { require(picker,"Star catalog Browse did not open a file picker"); picker->selectFile(path); static_cast<QDialog *>(picker)->accept(); }
      catch (...) { failure=std::current_exception(); if (picker) picker->reject(); }
   });
   form->findChild<QPushButton *>("starCatalogBrowse")->click(); if (failure) std::rethrow_exception(failure);
   require(field(form,"starCatalog")->text()==path,"Catalog Browse did not retain the selected path");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("StarOptionsEditor");
   if (argc!=2 || !acceptanceWidgetPlatform()) return 2;
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Star editor fixture directory unavailable");
      const auto catalog=files.filePath("owned catalog α.txt"),second=files.filePath("selected catalog β.txt");
      for (const auto &path:QStringList{catalog,second}) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write("ra_360 dec mag colorindex\n0 0 0 0\n90 0 4 0.5\n")>0,"Cannot write independent star catalog"); }
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath()); MainWindow window; require(window.initialize(startup),"Star editor initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Resources"); auto *area=window.findChild<QMdiArea *>("workspace"); require(editor && tree && area,"Mission workspace missing");
      QtStarSetting initial; initial.catalog=catalog; initial.customCount=11; initial.maxMagnitude=5.5; initial.maxPixels=6; initial.minDimRatio=.4;
      QtCameraSetting camera; camera.fieldOfView=42; camera.primaryName="PreservedCamera"; camera.axesLabels=false; camera.frameLabel=true; camera.objectLabels["Sat"]=false; camera.stars=initial; camera.solverLastN=3;
      const QString plain="% preserve independent star editor source α\nCreate Spacecraft Sat;\nCreate OrbitView Display;\nDisplay.Add = {Sat, Earth};\nDisplay.CoordinateSystem = EarthMJ2000Eq;\nDisplay.EnableStars = Off;\nDisplay.StarCount = 11;\n";
      const auto baseline=plain+qtCameraDirective("Display",camera)+"\nBeginMissionSequence;\n"; editor->setPlainText(baseline);
      const auto build=[&] { if (!window.buildScript()) throw std::runtime_error(("Star editor build failed:\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()).toStdString()); }; build(); preserve("initial.script",baseline.toUtf8());
      const auto open=[&] { const auto items=tree->findItems("Display",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Display resource missing"); tree->itemDoubleClicked(items.front(),0); require(area->activeSubWindow(),"Resource editor window missing"); auto *panel=dynamic_cast<ResourceEditor *>(area->activeSubWindow()->widget()); require(panel,"Display form missing"); return panel; };
      auto importedDefault=initial; importedDefault.customCount=40000; auto importedCamera=camera; importedCamera.stars=importedDefault;
      const auto importedSource=QString(plain).replace("Display.StarCount = 11;\n","")+qtCameraDirective("Display",importedCamera)+"\nBeginMissionSequence;\n";
      editor->setPlainText(importedSource); build(); require(object()->GetIntegerParameter("StarCount")==7000,"Imported mismatch fixture did not use core default7000"); auto *untouched=open(); const auto noOpUndoSteps=editor->document()->availableUndoSteps();
      stars(untouched,true,[&](QDialog *form) { values(form,importedDefault); }); require(!untouched->hasChanges() && editor->toPlainText()==importedSource,"Unchanged imported Star options created a count edit"); untouched->requestApply();
      require(editor->toPlainText()==importedSource && object()->GetIntegerParameter("StarCount")==7000 && editor->document()->availableUndoSteps()==noOpUndoSteps,"Unchanged imported Star options Apply rewrote source/model/Undo"); preserve("imported-noop.script",importedSource.toUtf8()); closePanels(window); editor->setPlainText(baseline); build();
      auto *panel=open();
      dialog(panel,"editOrbitView","orbitViewDialog",false,[&](QDialog *form) {
         auto *mode=form->findChild<QComboBox *>("orbit_SolverIterations"); auto *count=form->findChild<QSpinBox *>("orbit_SolverIterLastN");
         require(mode && count && mode->currentText()=="LastN" && count->value()==3,"Setup reopen lost unchanged LastN metadata without a backing row");
      });
      stars(panel,false,[&](QDialog *form) { values(form,initial); preset(form)->setCurrentText("Custom"); field(form,"starCustomCount")->setText("27"); });
      require(!panel->hasChanges() && editor->toPlainText()==baseline,"Star dialog Cancel changed source/pending state");
      stars(panel,true,[&](QDialog *form) { values(form,initial); preset(form)->setCurrentText("Printer"); });
      require(panel->hasChanges() && editor->toPlainText()==baseline,"Preset OK applied before parent Apply"); closePanels(window); panel=open();
      QtStarSetting changed=initial; changed.preset="Custom"; changed.catalog=second; changed.customCount=31; changed.minMagnitude=-1; changed.maxMagnitude=7.5; changed.minPixels=2; changed.maxPixels=9; changed.minDimRatio=.25;
      stars(panel,true,[&](QDialog *form) {
         values(form,initial); preset(form)->setCurrentText("Custom"); catalogBrowse(form,second);
         field(form,"starCustomCount")->setText("31"); field(form,"starMinMagnitude")->setText("-1"); field(form,"starMaxMagnitude")->setText("7.5"); field(form,"starMinPixels")->setText("2"); field(form,"starMaxPixels")->setText("9"); field(form,"starMinDimRatio")->setText("0.25"); values(form,changed);
         for (const auto &style:QStringList{"Monitor","Virtual Reality","Printer"}) { preset(form)->setCurrentText(style); auto expected=changed; expected.preset=style; values(form,expected); require(form->findChild<QLabel *>("starPresetDescription")->text().contains(style=="Printer" ? "Suppress" : "40,000"),"Preset explanation missing"); }
         preset(form)->setCurrentText("Custom"); values(form,changed);
         field(form,"starMinMagnitude")->setText("8"); form->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); require(form->isVisible() && !form->findChild<QLabel *>("starOptionsStatus")->text().isEmpty(),"Invalid magnitude bounds did not stay pending"); field(form,"starMinMagnitude")->setText("-1");
         field(form,"starCustomCount")->setText("0"); form->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); require(form->isVisible(),"Invalid zero count accepted"); field(form,"starCustomCount")->setText("31");
         const auto evidence=qEnvironmentVariable("GMAT_QT_STAR_OPTIONS_EVIDENCE"); if (!evidence.isEmpty()) { QApplication::processEvents(); require(QDir().mkpath(evidence) && form->grab().save(evidence+"/editor-custom-dialog.png"),"Cannot preserve star options screenshot"); }
      });
      auto *primaryCount=panel->findChild<QSpinBox *>("orbit_StarCount"); require(primaryCount && primaryCount->value()==31,"Star options did not update the visible primary count");
      require(panel->hasChanges() && editor->toPlainText()==baseline,"Star options did not remain pending"); stars(panel,false,[&](QDialog *form) { values(form,changed); preset(form)->setCurrentText("Qt default"); require(!field(form,"starCustomCount")->isEnabled(),"Qt default retained active custom controls"); });
      auto *pendingProperties=panel->findChild<QTableWidget *>("resourceProperties"); require(pendingProperties,"Pending Advanced properties missing"); int pendingCountRow=-1;
      for (int row=0;row<pendingProperties->rowCount();++row) if (pendingProperties->item(row,0)->text()=="StarCount") pendingCountRow=row; require(pendingCountRow>=0,"Pending Advanced count missing");
      pendingProperties->item(pendingCountRow,1)->setText("11"); stars(panel,false,[&](QDialog *form) { auto reverted=changed; reverted.customCount=11; values(form,reverted); }); pendingProperties->item(pendingCountRow,1)->setText("31");
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); const auto applied=editor->toPlainText(); auto expectedCamera=camera; expectedCamera.stars=changed;
      require(qtCameraDirective("Display",qtCameraSettings(applied).value("Display"))==qtCameraDirective("Display",expectedCamera) && object()->GetIntegerParameter("StarCount")==31 && object()->GetOnOffParameter("EnableStars")=="Off","Apply lost star values/count synchronization or changed EnableStars");
      preserve("applied.script",applied.toUtf8());
      const QRegularExpression countAssignment("^[ \t]*(?:GMAT[ \t]+)?Display\\.StarCount[ \t]*=[^;\n]*;[ \t]*(?:\n|$)",QRegularExpression::MultilineOption);
      const auto appliedSource=removeQtCameraSetting(applied,"Display"),baselineSource=removeQtCameraSetting(baseline,"Display");
      auto assignments=countAssignment.globalMatch(appliedSource); int assignmentCount=0; while (assignments.hasNext()) { const auto assignment=assignments.next().captured(); require(assignment.contains("= 31;"),"Applied count assignment has the wrong value"); ++assignmentCount; }
      require(assignmentCount==1 && QString(appliedSource).remove(countAssignment)==QString(baselineSource).remove(countAssignment),"Star Apply changed unrelated source/comments");
      editor->undo(); require(editor->toPlainText()==baseline,"Star Apply Undo not exact/atomic"); build(); editor->redo(); require(editor->toPlainText()==applied,"Star Apply Redo not exact"); build(); closePanels(window);
      const auto saved=files.filePath("star options α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved),"Unicode Star options Save/reopen failed"); build(); require(editor->toPlainText()==applied,"Reopen changed star metadata/source"); panel=open(); stars(panel,false,[&](QDialog *form) { values(form,changed); const auto evidence=qEnvironmentVariable("GMAT_QT_STAR_OPTIONS_EVIDENCE"); if (!evidence.isEmpty()) { QApplication::processEvents(); require(form->grab().save(evidence+"/editor-reopened-dialog.png"),"Cannot preserve clean reopened Star options screenshot"); } }); closePanels(window);
      panel=open(); const auto undoSteps=editor->document()->availableUndoSteps(); stars(panel,true,[&](QDialog *form) { field(form,"starCatalog")->setText(files.filePath("missing-catalog.txt")); }); panel->requestApply();
      require(panel->hasChanges() && editor->toPlainText()==applied && editor->document()->availableUndoSteps()==undoSteps && object()->GetIntegerParameter("StarCount")==31 && !panel->findChild<QLabel *>("resourceStatus")->text().isEmpty(),"Missing-catalog Apply changed source/model/Undo or discarded pending correction"); closePanels(window); build();
      require(!window.applyResourceChanges("Display",{{"@QtStars","{\"customCount\":0}"}},applied).isEmpty() && editor->toPlainText()==applied,"Invalid star metadata did not roll back");
      panel=open(); stars(panel,true,[&](QDialog *form) { preset(form)->setCurrentText("Monitor"); }); panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); const auto monitor=editor->toPlainText(); require(qtCameraSettings(monitor).value("Display").stars->customCount==31 && qtCameraSettings(monitor).value("Display").stars->preset=="Monitor","Preset Apply discarded custom values"); closePanels(window);
      panel=open(); auto *primaryEnabled=panel->findChild<QCheckBox *>("orbit_EnableStars"); primaryCount=panel->findChild<QSpinBox *>("orbit_StarCount");
      require(primaryEnabled && primaryCount && !primaryEnabled->isChecked() && primaryCount->value()==31,"Visible primary star count readback disagrees with metadata");
      primaryEnabled->setChecked(true); require(primaryCount->isEnabled(),"Existing primary Star Enable did not enable count"); primaryCount->setValue(17);
      stars(panel,false,[&](QDialog *form) { auto pending=changed; pending.preset="Monitor"; pending.customCount=17; values(form,pending); preset(form)->setCurrentText("Custom"); pending.preset="Custom"; values(form,pending); });
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); const auto counted=editor->toPlainText(); const auto countedSetting=qtCameraSettings(counted).value("Display"); require(countedSetting.stars && countedSetting.stars->preset=="Monitor" && countedSetting.stars->customCount==17 && object()->GetIntegerParameter("StarCount")==17,"Ordinary count Apply did not update retained custom count"); closePanels(window);
      panel=open(); dialog(panel,"editOrbitView","orbitViewDialog",true,[&](QDialog *form) { auto *count=form->findChild<QSpinBox *>("orbit_StarCount"); require(count && count->value()==17,"Ordinary count did not reopen"); count->setValue(18); });
      stars(panel,true,[&](QDialog *form) { auto pending=changed; pending.preset="Monitor"; pending.customCount=18; values(form,pending); preset(form)->setCurrentText("Custom"); field(form,"starCustomCount")->setText("19"); }); dialog(panel,"editOrbitView","orbitViewDialog",true,[&](QDialog *form) { auto *count=form->findChild<QSpinBox *>("orbit_StarCount"); require(count && count->value()==19,"Star options did not update pending Orbit setup count"); count->setValue(23); });
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); auto paired=editor->toPlainText(); require(qtCameraSettings(paired).value("Display").stars->customCount==23 && object()->GetIntegerParameter("StarCount")==23,"Later ordinary count lost to earlier pending Star options"); closePanels(window);
      panel=open(); stars(panel,true,[&](QDialog *form) { preset(form)->setCurrentText("Monitor"); }); auto *properties=panel->findChild<QTableWidget *>("resourceProperties"); require(properties,"Advanced properties missing"); bool foundCount=false;
      for (int row=0;row<properties->rowCount();++row) if (properties->item(row,0)->text()=="StarCount") { properties->item(row,1)->setText("29"); foundCount=true; } require(foundCount,"Advanced StarCount control missing");
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); paired=editor->toPlainText(); require(qtCameraSettings(paired).value("Display").stars->customCount==29 && object()->GetIntegerParameter("StarCount")==29,"Later Advanced count lost to pending preset"); closePanels(window);
      panel=open(); stars(panel,true,[&](QDialog *form) { preset(form)->setCurrentText("Qt default"); }); require(panel->hasChanges() && editor->toPlainText()==paired,"Qt default did not remain pending"); panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); const auto defaults=editor->toPlainText(); auto noOverride=qtCameraSettings(paired).value("Display"); noOverride.stars.reset();
      require(!qtCameraSettings(defaults).value("Display").stars && qtCameraDirective("Display",qtCameraSettings(defaults).value("Display"))==qtCameraDirective("Display",noOverride) && removeQtCameraSetting(defaults,"Display")==removeQtCameraSetting(paired,"Display"),"Qt default failed to clear only the star override"); closePanels(window); panel=open(); stars(panel,false,[&](QDialog *form) { require(preset(form)->currentText()=="Qt default" && field(form,"starCustomCount")->text().toInt()==29,"Qt default did not seed Custom from the current engine count"); preset(form)->setCurrentText("Custom"); require(field(form,"starCustomCount")->text().toInt()==29,"Qt default to Custom lost ordinary count"); }); closePanels(window);
      editor->undo(); require(editor->toPlainText()==paired,"Default Undo lost star settings"); build(); preserve("final.script",paired.toUtf8());
      std::cout<<"PASS actual Star options preset/custom fields and Browse, disabled retained values, Cancel/pending parent discard, invalid bounds/count/catalog rollback, Apply/count synchronization/Undo/Redo/Unicode reopen, preset-independent Enable and Qt default clear. Configuration only; no mission/host/GPU qualification.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
