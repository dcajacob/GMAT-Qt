#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ScriptCompatibility.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QPushButton>
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
static GmatBase *object(const QString &name) { return Moderator::Instance()->GetConfiguredObject(name.toStdString()); }
static void preserve(const QString &name,const QByteArray &data)
{
   const auto path=qEnvironmentVariable("GMAT_QT_SENSOR_MASK_EVIDENCE"); if (path.isEmpty()) return;
   require(QDir().mkpath(path),"Cannot create sensor editor evidence directory"); QFile file(path+"/editor-"+name);
   require(file.open(QIODevice::WriteOnly) && file.write(data)==data.size(),"Cannot preserve sensor editor evidence");
}
static QComboBox *choice(QTableWidget *table,int row,int column)
{
   auto *value=qobject_cast<QComboBox *>(table->cellWidget(row,column)); require(value,"Sensor mask choice missing"); return value;
}
static QLineEdit *length(QTableWidget *table,int row)
{
   auto *value=qobject_cast<QLineEdit *>(table->cellWidget(row,5)); require(value,"Sensor mask length missing"); return value;
}
static void row(QTableWidget *table,int index,const QtSensorMaskSetting &mask)
{
   require(index>=0 && index<table->rowCount() && table->item(index,0) && table->item(index,3),"Sensor mask row missing");
   require(table->item(index,0)->text()==mask.name && table->item(index,3)->text()==mask.label &&
      choice(table,index,1)->currentText()==mask.source && choice(table,index,2)->currentText()==mask.hardware &&
      choice(table,index,4)->currentText()==(mask.automaticLength ? "Auto" : "Manual") &&
      length(table,index)->text().toDouble()==mask.length && length(table,index)->isEnabled()!=mask.automaticLength,
      "Sensor mask row lost values, ordering or length-mode state");
}
static void fill(QTableWidget *table,int index,const QtSensorMaskSetting &mask)
{
   table->item(index,0)->setText(mask.name); table->item(index,3)->setText(mask.label);
   choice(table,index,1)->setCurrentText(mask.source); choice(table,index,2)->setCurrentText(mask.hardware);
   choice(table,index,4)->setCurrentText(mask.automaticLength ? "Auto" : "Manual"); length(table,index)->setText(QString::number(mask.length,'g',17)); row(table,index,mask);
}
static void action(QDialog *dialog,const char *name)
{
   auto *button=dialog->findChild<QPushButton *>(name); require(button,"Sensor mask action missing"); button->click();
}
static void closePanels(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("configurationPanel").toBool()) {
      if (auto *panel=dynamic_cast<ResourceEditor *>(child->widget())) panel->discardChanges(); child->close();
   }
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static void dialog(ResourceEditor *panel,bool accept,const std::function<void(QDialog *,QTableWidget *)> &edit)
{
   auto *tabs=panel->findChild<QTabWidget *>("resourceEditorTabs"); require(tabs && tabs->count()>1,"Resource Advanced tab missing"); tabs->setCurrentIndex(tabs->count()-1);
   auto *open=panel->findChild<QPushButton *>("editSensorMasks"); require(open,"Sensor mask editor action missing");
   std::exception_ptr failure;
   QTimer::singleShot(0,panel,[&] {
      auto *form=panel->findChild<QDialog *>("orbitSensorMaskDialog");
      try {
         require(form,"Sensor mask dialog missing"); auto *table=form->findChild<QTableWidget *>("orbitSensorMasks"); require(table && table->columnCount()==6,"Sensor mask table missing"); edit(form,table);
         auto *buttons=form->findChild<QDialogButtonBox *>(); require(buttons,"Sensor mask OK/Cancel missing");
         buttons->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
         require(form->result()==(accept ? QDialog::Accepted : QDialog::Rejected),qPrintable("Sensor mask dialog did not finish: "+form->findChild<QLabel *>("sensorMaskStatus")->text()));
      } catch (...) { failure=std::current_exception(); if (form) form->reject(); }
   });
   open->click(); if (failure) std::rethrow_exception(failure);
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("SensorMaskEditor");
   if (argc!=2 || QGuiApplication::platformName()!="offscreen") return 2;
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Sensor editor fixture directory unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Sensor editor initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Resources"); auto *area=window.findChild<QMdiArea *>("workspace"); require(editor && tree && area,"Mission workspace missing");
      const QString resources=
         "% independently authored sensor editing fixture: Sat CamOne remain literal comments α\n"
         "Create Spacecraft Sat Friend;\n"
         "Create ConicalFOV FovOne FovTwo;\nFovOne.FieldOfViewAngle = 12;\nFovTwo.FieldOfViewAngle = 6;\n"
         "Create Imager CamOne CamTwo;\n"
         "CamOne.FieldOfView = FovOne;\nCamOne.DirectionX = 0;\nCamOne.DirectionY = 0;\nCamOne.DirectionZ = 1;\nCamOne.SecondDirectionX = 0;\nCamOne.SecondDirectionY = 1;\nCamOne.SecondDirectionZ = 0;\n"
         "CamTwo.FieldOfView = FovTwo;\nCamTwo.DirectionX = 0;\nCamTwo.DirectionY = 0;\nCamTwo.DirectionZ = 1;\nCamTwo.SecondDirectionX = 0;\nCamTwo.SecondDirectionY = 1;\nCamTwo.SecondDirectionZ = 0;\n"
         "Sat.AddHardware = {CamOne};\nFriend.AddHardware = {CamTwo};\n"
         "Create OrbitView Display;\nDisplay.Add = {Sat, Friend, Earth};\nDisplay.CoordinateSystem = EarthMJ2000Eq;\nDisplay.ViewScaleFactor = 1.25;\n";
      QtSensorMaskSetting imported; imported.name="Sat"; imported.source="Sat"; imported.hardware="CamOne"; imported.label="Sat CamOne literal α"; imported.automaticLength=true; imported.length=1;
      QtCameraSetting camera; camera.perspective=true; camera.fieldOfView=42; camera.primaryName="PreservedCamera"; camera.axesLabels=false; camera.frameLabel=true; camera.objectLabels["Sat"]=false; camera.objectLineWidths["Sat"]=2.25; camera.sensorMasks={imported};
      const QString baseline=resources+qtCameraDirective("Display",camera)+"\nBeginMissionSequence;\n"; editor->setPlainText(baseline);
      const auto build=[&] { if (!window.buildScript()) { const auto diagnostic=window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText(); preserve("build-failure.script",editor->toPlainText().toUtf8()); preserve("build-failure.txt",diagnostic.toUtf8()); throw std::runtime_error(("Sensor editor build failed:\n"+diagnostic).toStdString()); } };
      build(); preserve("initial.script",baseline.toUtf8());
      const auto open=[&] {
         const auto items=tree->findItems("Display",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Display resource missing"); tree->itemDoubleClicked(items.front(),0);
         require(area->activeSubWindow(),"Resource editor window missing"); auto *panel=dynamic_cast<ResourceEditor *>(area->activeSubWindow()->widget()); require(panel,"Display resource form missing"); return panel;
      };
      auto *panel=open();
      dialog(panel,false,[&](QDialog *form,QTableWidget *table) {
         require(table->rowCount()==1,"Imported masks did not reopen"); row(table,0,imported); table->item(0,3)->setText("Discarded label"); action(form,"sensorMaskAdd"); require(table->rowCount()==2,"Add did not add a row");
      });
      require(!panel->hasChanges() && editor->toPlainText()==baseline,"Dialog Cancel changed pending/source settings");
      dialog(panel,true,[&](QDialog *,QTableWidget *table) { row(table,0,imported); table->item(0,3)->setText("Discard parent pending mask"); });
      require(panel->hasChanges() && editor->toPlainText()==baseline,"Dialog OK applied before parent Apply");
      closePanels(window); panel=open();
      dialog(panel,false,[&](QDialog *,QTableWidget *table) { require(table->rowCount()==1,"Parent discard retained added masks"); row(table,0,imported); });
      require(!panel->hasChanges() && editor->toPlainText()==baseline,"Parent discard changed authored source");
      auto changed=imported; changed.automaticLength=false; changed.length=7.5;
      QtSensorMaskSetting secondary; secondary.name="Secondary"; secondary.source="Friend"; secondary.hardware="CamTwo"; secondary.label="Friend CamTwo literal"; secondary.length=2;
      QtSensorMaskSetting extra; extra.name="Extra"; extra.source="Sat"; extra.hardware="CamOne"; extra.label="Third mask"; extra.automaticLength=false; extra.length=3;
      const QVector<QtSensorMaskSetting> expected={changed,extra,secondary};
      dialog(panel,true,[&](QDialog *form,QTableWidget *table) {
         row(table,0,imported); fill(table,0,changed);
         action(form,"sensorMaskAdd"); require(table->rowCount()==2,"Second mask was not added"); fill(table,1,secondary);
         action(form,"sensorMaskAdd"); require(table->rowCount()==3,"Temporary mask was not added"); table->item(2,0)->setText("Drop"); table->setCurrentCell(2,0); action(form,"sensorMaskRemove"); require(table->rowCount()==2,"Selected mask was not removed");
         action(form,"sensorMaskAdd"); require(table->rowCount()==3,"Third mask was not added"); fill(table,2,extra);
         table->setCurrentCell(0,0); action(form,"sensorMaskDown"); row(table,0,secondary); row(table,1,changed); row(table,2,extra);
         action(form,"sensorMaskUp"); row(table,0,changed); row(table,1,secondary); row(table,2,extra);
         table->setCurrentCell(2,0); action(form,"sensorMaskUp"); for (int i=0;i<expected.size();++i) row(table,i,expected[i]);
         const auto evidence=qEnvironmentVariable("GMAT_QT_SENSOR_MASK_EVIDENCE");
         if (!evidence.isEmpty()) { QApplication::processEvents(); require(QDir().mkpath(evidence) && form->grab().save(evidence+"/editor-pending-dialog.png"),"Cannot preserve offscreen sensor-mask editor screenshot"); }
      });
      require(panel->hasChanges() && editor->toPlainText()==baseline,"Mask edit/reorder did not remain pending");
      dialog(panel,false,[&](QDialog *,QTableWidget *table) {
         require(table->rowCount()==expected.size(),"Pending masks did not reopen"); for (int i=0;i<expected.size();++i) row(table,i,expected[i]); table->item(0,3)->setText("Discard pending replacement");
      });
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); const auto applied=editor->toPlainText();
      auto expectedCamera=camera; expectedCamera.sensorMasks=expected;
      require(applied!=baseline && qtCameraDirective("Display",qtCameraSettings(applied).value("Display"))==qtCameraDirective("Display",expectedCamera),"Apply lost masks or unrelated camera settings");
      require(removeQtCameraSetting(applied,"Display")==removeQtCameraSetting(baseline,"Display"),"Mask Apply rewrote unrelated engine source/comments"); preserve("applied.script",applied.toUtf8());
      editor->undo(); require(editor->toPlainText()==baseline,"Mask Apply Undo not exact/atomic"); build(); editor->redo(); require(editor->toPlainText()==applied,"Mask Apply Redo not exact"); build(); closePanels(window);
      const auto saved=files.filePath("sensor masks α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved),"Unicode Save/reopen failed"); build(); require(editor->toPlainText()==applied,"Save/reopen changed sensor metadata/source");
      panel=open(); dialog(panel,false,[&](QDialog *,QTableWidget *table) { require(table->rowCount()==expected.size(),"Saved masks did not reopen"); for (int i=0;i<expected.size();++i) row(table,i,expected[i]); });
      require(!panel->hasChanges(),"Reopen/Cancel created a pending edit"); closePanels(window);
      for (const auto &invalid:QStringList{"MissingSource","MissingHardware"}) {
         panel=open(); const auto before=editor->toPlainText(); const auto add=object("Display")->GetStringArrayParameter("Add"); const auto scale=object("Display")->GetRealParameter("ViewScaleFactor");
         const auto undoSteps=editor->document()->availableUndoSteps();
         dialog(panel,true,[&](QDialog *,QTableWidget *table) { choice(table,0,invalid=="MissingSource" ? 1 : 2)->setCurrentText(invalid); });
         require(panel->hasChanges() && editor->toPlainText()==before,"Invalid reference committed before Apply"); panel->requestApply();
         require(panel->hasChanges() && !panel->findChild<QLabel *>("resourceStatus")->text().isEmpty() && editor->toPlainText()==before && editor->document()->availableUndoSteps()==undoSteps,
            "Invalid sensor reference Apply changed source/Undo or discarded pending correction");
         require(object("Display") && object("Display")->GetStringArrayParameter("Add")==add && object("Display")->GetRealParameter("ViewScaleFactor")==scale && object("CamOne")->GetRefObjectName(Gmat::FIELD_OF_VIEW)=="FovOne",
            "Invalid sensor reference Apply changed configured resources"); closePanels(window); build();
      }
      panel=open(); dialog(panel,true,[&](QDialog *form,QTableWidget *table) { while (table->rowCount()) { table->setCurrentCell(0,0); action(form,"sensorMaskRemove"); } });
      require(panel->hasChanges() && editor->toPlainText()==applied,"Remove-all did not remain pending"); panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      const auto removed=editor->toPlainText(); auto withoutMasks=expectedCamera; withoutMasks.sensorMasks.clear();
      require(qtCameraDirective("Display",qtCameraSettings(removed).value("Display"))==qtCameraDirective("Display",withoutMasks) && removeQtCameraSetting(removed,"Display")==removeQtCameraSetting(applied,"Display"),"Remove-all retained masks or lost unrelated settings");
      editor->undo(); require(editor->toPlainText()==applied,"Remove-all Undo not exact"); build(); closePanels(window);
      require(window.renameResource("Sat","Vehicle",applied).isEmpty(),"Mask source Rename failed"); const auto sourceRenamed=editor->toPlainText(); const auto sourceMasks=qtCameraSettings(sourceRenamed).value("Display").sensorMasks;
      require(sourceMasks.size()==3 && sourceMasks[0].source=="Vehicle" && sourceMasks[1].source=="Vehicle" && sourceMasks[2].source=="Friend" && sourceMasks[0].name=="Sat" && sourceMasks[0].label==changed.label && sourceMasks[0].hardware=="CamOne" && sourceRenamed.contains("% independently authored sensor editing fixture: Sat CamOne remain literal comments α"),"Source Rename lost mask references or changed literal names/labels/comments");
      editor->undo(); require(editor->toPlainText()==applied,"Source Rename Undo not atomic"); build(); editor->redo(); require(editor->toPlainText()==sourceRenamed,"Source Rename Redo not exact"); build();
      require(window.renameResource("CamOne","Camera",sourceRenamed).isEmpty(),"Mask hardware Rename failed"); const auto hardwareRenamed=editor->toPlainText(); const auto hardwareMasks=qtCameraSettings(hardwareRenamed).value("Display").sensorMasks;
      require(hardwareMasks.size()==3 && hardwareMasks[0].hardware=="Camera" && hardwareMasks[1].hardware=="Camera" && hardwareMasks[2].hardware=="CamTwo" && hardwareMasks[0].name=="Sat" && hardwareMasks[0].label==changed.label && hardwareMasks[0].source=="Vehicle" && object("Camera") && !object("CamOne") && object("Vehicle"),"Hardware Rename lost sensor references or changed mask literals");
      editor->undo(); require(editor->toPlainText()==sourceRenamed,"Hardware Rename Undo not atomic"); build(); editor->redo(); require(editor->toPlainText()==hardwareRenamed,"Hardware Rename Redo not exact"); build(); preserve("renamed.script",hardwareRenamed.toUtf8());
      require(window.saveScriptTo(saved) && window.loadScript(saved),"Renamed sensor Save/reopen failed"); build(); require(editor->toPlainText()==hardwareRenamed,"Renamed sensor reopening lost source");
      panel=open(); dialog(panel,false,[&](QDialog *,QTableWidget *table) { require(table->rowCount()==hardwareMasks.size(),"Renamed masks did not reopen"); for (int i=0;i<hardwareMasks.size();++i) row(table,i,hardwareMasks[i]); }); closePanels(window);
      std::cout<<"PASS actual sensor-mask controls: imported/pending Cancel and parent discard, Add/Edit/Up/Down/Remove with Auto/Manual lengths, parent Apply/atomic Undo/Redo, unchanged unrelated source/settings, Unicode Save/reopen, empty-list removal, source/hardware Rename with literal preservation, and invalid-reference Apply rollback. No mission or host/GPU qualification.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
     catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
