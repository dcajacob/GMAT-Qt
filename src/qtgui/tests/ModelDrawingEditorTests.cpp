#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourcePreview.hpp"
#include "ScriptCompatibility.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QApplication>
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
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void preserve(const QString &name,const QByteArray &bytes)
{
   const auto path=qEnvironmentVariable("GMAT_QT_MODEL_DRAWING_EVIDENCE"); if (path.isEmpty()) return;
   require(QDir().mkpath(path),"Cannot create model drawing evidence directory"); QFile file(path+"/editor-"+name); require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Cannot preserve model drawing editor evidence");
}
static QLineEdit *fileField(QDialog *form,const QString &name)
{
   auto *field=form->findChild<QLineEdit *>("orbitDrawing_modelFile_"+name); require(field,"Per-object model file field missing"); return field;
}
static void closePanels(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("configurationPanel").toBool()) { if (auto *panel=dynamic_cast<ResourceEditor *>(child->widget())) panel->discardChanges(); child->close(); }
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static void drawing(ResourceEditor *panel,bool accept,const std::function<void(QDialog *)> &edit)
{
   auto *tabs=panel->findChild<QTabWidget *>("resourceEditorTabs"); require(tabs && tabs->count()>1,"Advanced resource tab missing"); tabs->setCurrentIndex(tabs->count()-1);
   auto *button=panel->findChild<QPushButton *>("editOrbitDrawing"); require(button,"Object drawing action missing"); QApplication::processEvents(); require(button->isVisible() && button->isEnabled(),"Object drawing action is not reachable"); std::exception_ptr failure;
   QTimer::singleShot(0,panel,[&] {
      auto *form=panel->findChild<QDialog *>("orbitObjectDrawingDialog");
      try {
         require(form && form->isVisible(),"Object drawing dialog missing or hidden"); auto *pages=form->findChild<QTabWidget *>("orbitDrawingTabs"); require(pages,"Drawing pages missing");
         auto *models=form->findChild<QTableWidget *>("orbitObjectModels"); require(models,"Model override table missing"); auto *page=models->parentWidget(); require(pages->indexOf(page)>=0,"Model page is not in object drawing tabs"); pages->setCurrentWidget(page); QApplication::processEvents(); require(models->isVisible(),"Model overrides are not reachable in visible Models tab");
         edit(form); auto *buttons=form->findChild<QDialogButtonBox *>(); require(buttons,"Drawing dialog buttons missing"); buttons->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); require(form->result()==(accept ? QDialog::Accepted : QDialog::Rejected),"Drawing dialog did not finish");
      } catch (...) { failure=std::current_exception(); if (form) form->reject(); }
   }); button->click(); if (failure) std::rethrow_exception(failure);
}
static void browse(QDialog *form,const QString &name,const QString &path,bool accept)
{
   auto *button=form->findChild<QPushButton *>("orbitDrawing_modelBrowse_"+name); require(button && button->isVisible() && button->isEnabled(),"Model file Browse action is not reachable"); std::exception_ptr failure;
   QTimer::singleShot(0,form,[&] {
      auto *picker=form->findChild<QFileDialog *>("orbitDrawingModelFileDialog");
      try { require(picker && picker->isVisible() && picker->fileMode()==QFileDialog::ExistingFile,"Model Browse did not open an existing-file picker"); picker->selectFile(path); if (accept) QMetaObject::invokeMethod(picker,"accept",Qt::DirectConnection); else picker->reject(); }
      catch (...) { failure=std::current_exception(); if (picker) picker->reject(); }
   }); button->click(); if (failure) std::rethrow_exception(failure);
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("ModelDrawingEditor");
   if (argc!=2 || !acceptanceWidgetPlatform()) return 2;
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Model drawing fixture directory unavailable");
      const auto modelPath=files.filePath("plot model α.obj"); QFile model(modelPath); require(model.open(QIODevice::WriteOnly),"Model fixture unavailable"); model.write("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n"); model.close();
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath()); MainWindow window; require(window.initialize(startup),"Model drawing editor initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Resources"); auto *area=window.findChild<QMdiArea *>("workspace"); require(editor && tree && area,"Mission workspace missing");
      QtCameraSetting camera; camera.fieldOfView=42; camera.primaryName="PreservedCamera"; camera.axesLabels=false; camera.frameLabel=true; camera.objectLabels["Sat"]=false; camera.objectModelFiles["Other"]="retained missing model.obj";
      const QString plain="% preserve model drawing source α\nCreate Spacecraft Sat Other;\nCreate OrbitView Display;\nDisplay.Add = {Sat, Other, Earth};\nCreate XYPlot OtherType;\n";
      const auto baseline=plain+qtCameraDirective("Display",camera)+"\nBeginMissionSequence;\n"; editor->setPlainText(baseline);
      const auto build=[&] { if (!window.buildScript()) throw std::runtime_error(("Model editor build failed:\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()).toStdString()); }; build(); preserve("initial.script",baseline.toUtf8());
      const auto open=[&] { const auto items=tree->findItems("Display",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Display resource missing"); tree->itemDoubleClicked(items.front(),0); require(area->activeSubWindow(),"Resource editor window missing"); auto *panel=dynamic_cast<ResourceEditor *>(area->activeSubWindow()->widget()); require(panel,"Display form missing"); return panel; };
      auto *panel=open(); const auto undoSteps=editor->document()->availableUndoSteps();
      drawing(panel,true,[&](QDialog *form) {
         require(fileField(form,"Sat")->text().isEmpty() && fileField(form,"Earth")->text().isEmpty() && fileField(form,"Other")->text()==camera.objectModelFiles.value("Other"),"Imported model override/default did not reopen");
         require(form->findChild<QTableWidget *>("orbitObjectModels")->rowCount()==3,"Model overrides lost selected space points");
      }); require(!panel->hasChanges(),"Unchanged model dialog created pending state"); panel->requestApply(); require(editor->toPlainText()==baseline && editor->document()->availableUndoSteps()==undoSteps,"Unchanged model Apply rewrote source/Undo");
      drawing(panel,false,[&](QDialog *form) { fileField(form,"Sat")->setText(modelPath); }); require(!panel->hasChanges() && editor->toPlainText()==baseline,"Model dialog Cancel changed source/pending state");
      drawing(panel,true,[&](QDialog *form) { fileField(form,"Sat")->setText(modelPath); }); require(panel->hasChanges() && editor->toPlainText()==baseline,"Model OK applied before parent Apply"); closePanels(window); panel=open();
      drawing(panel,true,[&](QDialog *form) {
         require(fileField(form,"Sat")->text().isEmpty(),"Discarded pending model path survived panel close"); browse(form,"Sat",modelPath,false); require(fileField(form,"Sat")->text().isEmpty(),"Model Browse Cancel changed path"); browse(form,"Sat",modelPath,true); require(fileField(form,"Sat")->text()==modelPath,"Model Browse lost selected Unicode path"); fileField(form,"Earth")->setText("body override.obj");
         const auto evidence=qEnvironmentVariable("GMAT_QT_MODEL_DRAWING_EVIDENCE"); if (!evidence.isEmpty()) { QApplication::processEvents(); require(QDir().mkpath(evidence) && form->grab().save(evidence+"/editor-model-dialog.png"),"Cannot preserve model override dialog screenshot"); }
      }); require(panel->hasChanges() && editor->toPlainText()==baseline,"Model Browse acceptance did not remain pending"); drawing(panel,false,[&](QDialog *form) { require(fileField(form,"Sat")->text()==modelPath && fileField(form,"Earth")->text()=="body override.obj","Pending model overrides were not retained"); });
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); const auto applied=editor->toPlainText(); auto expectedCamera=camera; expectedCamera.objectModelFiles["Sat"]=modelPath; expectedCamera.objectModelFiles["Earth"]="body override.obj";
      require(qtCameraDirective("Display",qtCameraSettings(applied).value("Display"))==qtCameraDirective("Display",expectedCamera),"Model Apply lost overrides or unrelated camera metadata"); require(removeQtCameraSetting(applied,"Display")==removeQtCameraSetting(baseline,"Display"),"Model Apply changed unrelated source/comments/resource model"); preserve("applied.script",applied.toUtf8());
      editor->undo(); require(editor->toPlainText()==baseline,"Model Apply Undo not exact/atomic"); build(); editor->redo(); require(editor->toPlainText()==applied,"Model Apply Redo not exact"); build(); closePanels(window);
      const auto saved=files.filePath("model drawing α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved),"Unicode model drawing Save/reopen failed"); build(); require(editor->toPlainText()==applied,"Reopen changed model metadata/source"); panel=open(); drawing(panel,false,[&](QDialog *form) { require(fileField(form,"Sat")->text()==modelPath && fileField(form,"Earth")->text()=="body override.obj","Saved model overrides did not reopen"); }); closePanels(window);
      for (const auto &invalid:QStringList{"{\"Missing\":\"x.obj\"}","{\"OtherType\":\"x.obj\"}","{\"Sat\":true}"}) {
         const auto beforeUndo=editor->document()->availableUndoSteps(); const auto error=window.applyResourceChanges("Display",{{"@QtObjectModelFiles",invalid}},applied); require(!error.isEmpty() && editor->toPlainText()==applied && editor->document()->availableUndoSteps()==beforeUndo,"Invalid model membership/type changed source/Undo or escaped validation");
      }
      require(!window.applyResourceChanges("OtherType",{{"@QtObjectModelFiles","{\"Sat\":\"x.obj\"}"}},applied).isEmpty() && editor->toPlainText()==applied,"Model overrides escaped OrbitView type restriction");
      require(window.renameResource("Sat","ModelSat",applied).isEmpty(),"Model override object Rename failed"); const auto renamed=editor->toPlainText(); require(qtCameraSettings(renamed).value("Display").objectModelFiles.contains("ModelSat") && !qtCameraSettings(renamed).value("Display").objectModelFiles.contains("Sat"),"Rename lost named model override"); editor->undo(); require(editor->toPlainText()==applied,"Model Rename Undo not exact"); build();
      require(window.applyResourceChanges("Display",{{"Add","Other, Earth"}},applied).isEmpty(),"Model membership removal failed"); const auto pruned=editor->toPlainText(); require(!qtCameraSettings(pruned).value("Display").objectModelFiles.contains("Sat") && qtCameraSettings(pruned).value("Display").objectModelFiles.value("Earth")=="body override.obj","Removed object retained stale override or pruned other body"); editor->undo(); require(editor->toPlainText()==applied,"Model removal Undo not exact"); build();
      panel=open(); drawing(panel,true,[&](QDialog *form) { auto *reset=form->findChild<QPushButton *>("orbitDrawing_modelReset_Sat"); require(reset && reset->isVisible(),"Resource model reset action missing"); reset->click(); require(fileField(form,"Sat")->text().isEmpty(),"Resource model reset retained override"); }); panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); const auto resourceDefault=editor->toPlainText(); require(!qtCameraSettings(resourceDefault).value("Display").objectModelFiles.contains("Sat") && qtCameraSettings(resourceDefault).value("Display").objectModelFiles.value("Earth")=="body override.obj","Resource model reset erased other override"); closePanels(window); preserve("final.script",resourceDefault.toUtf8());
      QtResourcePreview created(Moderator::Instance()->GetConfiguredObject("Display")->Clone()); created->SetName("NewDisplay"); QMap<QString,QString> creationChanges; QWidget holder; auto *layout=new QVBoxLayout(&holder); auto *creation=new ResourceEditor(*created,[&](const auto &changes) { creationChanges=changes; return QString(); },&holder,resourceDefault,false,true); layout->addWidget(creation); holder.resize(800,900); holder.show();
      drawing(creation,true,[&](QDialog *form) { require(fileField(form,"Sat")->text().isEmpty(),"New OrbitView inherited another plot's override"); fileField(form,"Sat")->setText(modelPath); }); creation->requestApply(); require(qtObjectModelFiles(creationChanges.value("@QtObjectModelFiles")).value("Sat")==modelPath,"New OrbitView lost pending model override");
      std::cout<<"PASS visible Models tab and per-object model path/default controls; file Browse/Cancel/Unicode path; unchanged no-op, pending/discard, Apply/Undo/Redo/Save/reopen; invalid metadata and membership rollback; Rename/pruning; resource creation. Configuration only; no host/GPU qualification.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
