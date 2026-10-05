#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourcePreview.hpp"
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
   const auto path=qEnvironmentVariable("GMAT_QT_PLAYBACK_OPTIONS_EVIDENCE"); if (path.isEmpty()) return;
   require(QDir().mkpath(path),"Cannot create playback editor evidence directory"); QFile file(path+"/editor-"+name); require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Cannot preserve playback editor evidence");
}
static QComboBox *choice(QDialog *form,const char *name)
{
   auto *field=form->findChild<QComboBox *>(name); require(field,"Playback choice missing"); return field;
}
static QLineEdit *rate(QDialog *form)
{
   auto *field=form->findChild<QLineEdit *>("playbackTimeScale"); require(field,"Playback rate missing"); return field;
}
static void values(QDialog *form,const std::optional<QtPlaybackSetting> &expected)
{
   const auto setting=expected.value_or(QtPlaybackSetting()); const bool clock=setting.timeScale.has_value();
   require(choice(form,"playbackClockMode")->currentIndex()==(clock ? 1 : 0),"Playback clock selection did not reopen");
   require(choice(form,"playbackSyncParent")->currentData().toString()==setting.syncParent && choice(form,"playbackSyncParent")->isEnabled()==clock,"Playback parent selection/dependency mismatch");
   require(rate(form)->isEnabled()==clock && (setting.timeScale ? rate(form)->text().toDouble()==*setting.timeScale : rate(form)->text().isEmpty()),"Playback rate did not reopen");
   require(choice(form,"playbackToolbar")->currentIndex()==(setting.showToolbar ? (*setting.showToolbar ? 1 : 2) : 0),"Toolbar preference did not reopen");
}
static void closePanels(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("configurationPanel").toBool()) { if (auto *panel=dynamic_cast<ResourceEditor *>(child->widget())) panel->discardChanges(); child->close(); }
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static void playback(ResourceEditor *panel,bool accept,const std::function<void(QDialog *)> &edit)
{
   auto *tabs=panel->findChild<QTabWidget *>("resourceEditorTabs"); require(tabs && tabs->count()>1,"Advanced resource tab missing"); tabs->setCurrentIndex(tabs->count()-1);
   auto *button=panel->findChild<QPushButton *>("editPlaybackOptions"); require(button,"Playback options action missing"); QApplication::processEvents(); require(button->isVisible() && button->isEnabled(),"Playback action not reachable in visible resource editor"); std::exception_ptr failure;
   QTimer::singleShot(0,panel,[&] {
      auto *form=panel->findChild<QDialog *>("orbitPlaybackOptionsDialog");
      try {
         require(form && form->isVisible(),"Playback dialog missing or hidden"); edit(form); auto *buttons=form->findChild<QDialogButtonBox *>(); require(buttons,"Playback dialog buttons missing");
         buttons->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); require(form->result()==(accept ? QDialog::Accepted : QDialog::Rejected),"Playback dialog did not finish");
      } catch (...) { failure=std::current_exception(); if (form) form->reject(); }
   });
   button->click(); if (failure) std::rethrow_exception(failure);
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("PlaybackOptionsEditor");
   if (argc!=2 || QGuiApplication::platformName()!="offscreen") return 2;
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Playback editor fixture directory unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath()); MainWindow window; require(window.initialize(startup),"Playback editor initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *tree=window.findChild<QTreeWidget *>("Resources"); auto *area=window.findChild<QMdiArea *>("workspace"); require(editor && tree && area,"Mission workspace missing");
      QtCameraSetting camera; camera.fieldOfView=42; camera.primaryName="PreservedCamera"; camera.axesLabels=false; camera.frameLabel=true; camera.objectLabels["Sat"]=false;
      QtCameraSetting follower; QtPlaybackSetting followerPlayback; followerPlayback.timeScale=3; followerPlayback.syncParent="Display"; follower.playback=followerPlayback;
      const QString plain="% preserve playback source α\nCreate Spacecraft Sat;\nCreate OrbitView Display Parent Follower;\nDisplay.Add = {Sat, Earth};\nParent.Add = {Sat, Earth};\nFollower.Add = {Sat, Earth};\nCreate XYPlot OtherType;\n";
      const auto baseline=plain+qtCameraDirective("Display",camera)+"\n"+qtCameraDirective("Follower",follower)+"\nBeginMissionSequence;\n"; editor->setPlainText(baseline);
      const auto build=[&] { if (!window.buildScript()) throw std::runtime_error(("Playback editor build failed:\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()).toStdString()); }; build(); preserve("initial.script",baseline.toUtf8());
      const auto open=[&] { const auto items=tree->findItems("Display",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Display resource missing"); tree->itemDoubleClicked(items.front(),0); require(area->activeSubWindow(),"Resource editor window missing"); auto *panel=dynamic_cast<ResourceEditor *>(area->activeSubWindow()->widget()); require(panel,"Display form missing"); return panel; };
      auto *panel=open(); const auto undoSteps=editor->document()->availableUndoSteps();
      playback(panel,true,[&](QDialog *form) {
         values(form,std::nullopt); const auto *parents=choice(form,"playbackSyncParent");
         require(parents->findData("Parent")>=0 && parents->findData("Display")<0 && parents->findData("Follower")<0 && parents->findData("OtherType")<0,"Parent selector admitted self/descendant/non-OrbitView or lost eligible parent");
      }); require(!panel->hasChanges(),"Unchanged playback dialog created pending state"); panel->requestApply();
      require(editor->toPlainText()==baseline && editor->document()->availableUndoSteps()==undoSteps,"Unchanged playback Apply rewrote source/Undo");
      playback(panel,false,[&](QDialog *form) { choice(form,"playbackClockMode")->setCurrentIndex(1); rate(form)->setText("42"); }); require(!panel->hasChanges() && editor->toPlainText()==baseline,"Playback Cancel changed source or pending state");
      playback(panel,true,[&](QDialog *form) { choice(form,"playbackToolbar")->setCurrentIndex(2); }); require(panel->hasChanges() && editor->toPlainText()==baseline,"Toolbar OK did not remain pending"); closePanels(window); panel=open();
      QtPlaybackSetting changed; changed.timeScale=-120; changed.syncParent="Parent"; changed.showToolbar=false;
      playback(panel,true,[&](QDialog *form) {
         values(form,std::nullopt); choice(form,"playbackClockMode")->setCurrentIndex(1); require(rate(form)->isEnabled(),"Clock mode did not enable rate");
         choice(form,"playbackSyncParent")->setCurrentIndex(choice(form,"playbackSyncParent")->findData("Parent")); rate(form)->setText("-120"); choice(form,"playbackToolbar")->setCurrentIndex(2); values(form,changed);
         for (const auto &invalid:QStringList{"not a rate","nan","inf"}) { rate(form)->setText(invalid); form->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); require(form->isVisible() && !form->findChild<QLabel *>("playbackOptionsStatus")->text().isEmpty(),"Invalid rate did not stay pending"); }
         rate(form)->setText("-120");
         const auto evidence=qEnvironmentVariable("GMAT_QT_PLAYBACK_OPTIONS_EVIDENCE"); if (!evidence.isEmpty()) { QApplication::processEvents(); require(QDir().mkpath(evidence) && form->grab().save(evidence+"/editor-playback-dialog.png"),"Cannot preserve playback dialog screenshot"); }
      }); require(panel->hasChanges() && editor->toPlainText()==baseline,"Playback OK applied before parent Apply"); playback(panel,false,[&](QDialog *form) { values(form,changed); });
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); const auto applied=editor->toPlainText(); auto expectedCamera=camera; expectedCamera.playback=changed;
      require(qtCameraDirective("Display",qtCameraSettings(applied).value("Display"))==qtCameraDirective("Display",expectedCamera),"Apply lost playback settings or unrelated camera state");
      require(removeQtCameraSetting(applied,"Display")==removeQtCameraSetting(baseline,"Display"),"Playback Apply changed unrelated source/comments"); preserve("applied.script",applied.toUtf8());
      editor->undo(); require(editor->toPlainText()==baseline,"Playback Apply Undo not exact/atomic"); build(); editor->redo(); require(editor->toPlainText()==applied,"Playback Apply Redo not exact"); build(); closePanels(window);
      const auto saved=files.filePath("playback options α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved),"Unicode Playback options Save/reopen failed"); build(); require(editor->toPlainText()==applied,"Reopen changed playback metadata/source"); panel=open(); playback(panel,false,[&](QDialog *form) { values(form,changed); }); closePanels(window);
      for (const auto &parent:QStringList{"Display","Follower","MissingPlot","OtherType"}) {
         auto invalid=changed; invalid.syncParent=parent;
         const auto beforeUndo=editor->document()->availableUndoSteps(); const auto error=window.applyResourceChanges("Display",{{"@QtPlayback",qtPlaybackJson(invalid)}},applied);
         require(!error.isEmpty() && editor->toPlainText()==applied && editor->document()->availableUndoSteps()==beforeUndo,"Invalid parent changed source/Undo or escaped validation");
      }
      panel=open(); playback(panel,true,[&](QDialog *form) { values(form,changed); choice(form,"playbackSyncParent")->setCurrentIndex(0); rate(form)->setText("0"); choice(form,"playbackToolbar")->setCurrentIndex(1); }); panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); const auto held=editor->toPlainText();
      require(qtCameraSettings(held).value("Display").playback->timeScale==0 && qtCameraSettings(held).value("Display").playback->syncParent.isEmpty() && qtCameraSettings(held).value("Display").playback->showToolbar==true,"Zero/independent/show selection was lost"); closePanels(window);
      panel=open(); playback(panel,true,[&](QDialog *form) { choice(form,"playbackClockMode")->setCurrentIndex(0); choice(form,"playbackToolbar")->setCurrentIndex(2); }); panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); const auto toolbarOnly=editor->toPlainText(); const auto toolbarSetting=qtCameraSettings(toolbarOnly).value("Display").playback;
      require(toolbarSetting && !toolbarSetting->timeScale && toolbarSetting->syncParent.isEmpty() && toolbarSetting->showToolbar==false,"Toolbar-only preference introduced a clock"); closePanels(window);
      panel=open(); playback(panel,true,[&](QDialog *form) { values(form,toolbarSetting); form->findChild<QPushButton *>("playbackDefaults")->click(); values(form,std::nullopt); }); panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); const auto defaults=editor->toPlainText();
      require(!qtCameraSettings(defaults).value("Display").playback && defaults==baseline,"Qt defaults failed to clear only playback metadata"); closePanels(window);
      editor->undo(); require(editor->toPlainText()==toolbarOnly,"Qt defaults Undo lost playback settings"); build(); preserve("final.script",toolbarOnly.toUtf8());
      // New resources use the same visible action before they have a source directive.
      QtResourcePreview created(Moderator::Instance()->GetConfiguredObject("Display")->Clone()); created->SetName("NewDisplay"); QMap<QString,QString> creationChanges; QWidget holder; auto *layout=new QVBoxLayout(&holder);
      auto *creation=new ResourceEditor(*created,[&](const auto &changes) { creationChanges=changes; return QString(); },&holder,toolbarOnly,false,true); layout->addWidget(creation); holder.resize(800,900); holder.show();
      playback(creation,true,[&](QDialog *form) { values(form,std::nullopt); choice(form,"playbackClockMode")->setCurrentIndex(1); rate(form)->setText("7200"); }); creation->requestApply();
      require(qtPlayback(creationChanges.value("@QtPlayback"))->timeScale==7200,"New OrbitView lost pending playback settings");
      std::cout<<"PASS visible Playback options action and signed/zero/default clock, eligible parent selector, toolbar-only preference, Cancel/pending/discard, invalid rate/parent rollback, source-preserving Apply/Undo/Redo/Unicode reopen, clear and resource creation. Configuration only; no host/GPU qualification.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
