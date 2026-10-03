#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "CustomFovDialog.hpp"
#include "CustomFovProperties.hpp"
#include "ResourcePreview.hpp"
#include "CustomFOV.hpp"
#include "FileManager.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "GmatConstants.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static CustomFOV *mask()
{
   auto *object=dynamic_cast<CustomFOV *>(Moderator::Instance()->GetConfiguredObject("Mask")); require(object,"CustomFOV is not configured"); return object;
}
static ResourceEditor *maskPanel(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("resourceName").toString()=="Mask") {
      auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"CustomFOV editor missing"); return panel;
   }
   throw std::runtime_error("CustomFOV editor was not retained");
}
static void openMask(MainWindow &window)
{
   auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto items=tree->findItems("Mask",Qt::MatchExactly|Qt::MatchRecursive);
   require(items.size()==1,"CustomFOV tree item missing"); tree->itemDoubleClicked(items.front(),0);
}
static void closeMask(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("resourceName").toString()=="Mask") child->close();
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static void pairs(QWidget *owner,const QStringList &a,const QStringList &b)
{
   require(a.size()==b.size(),"Test pairs are mismatched"); auto *table=owner->findChild<QTableWidget *>("customFovVertices"); require(table,"Paired FOV editor missing");
   table->setRowCount(a.size()); for (int i=0;i<a.size();++i) { table->setItem(i,0,new QTableWidgetItem(a[i])); table->setItem(i,1,new QTableWidgetItem(b[i])); }
}
static QString unrelated(QString source)
{
   source.remove(QRegularExpression("^Create CustomFOV Mask;\\n",QRegularExpression::MultilineOption));
   source.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?Mask\\.[A-Za-z][A-Za-z0-9_.]*[ \\t]*=[^\\n]*;[^\\n]*\\n",QRegularExpression::MultilineOption));
   source.remove(QRegularExpression("^[ \\t]*% preserve FOV angle α[ \\t]*\\n",QRegularExpression::MultilineOption));
   return source;
}
static QString withoutProperty(QString source,const QString &property)
{
   source.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?Mask\\."+QRegularExpression::escape(property)+"[ \\t]*=[^\\n]*;[^\\n]*\\n",QRegularExpression::MultilineOption));
   return source;
}
static void checkPolygon(const QStringList &a,const QStringList &b,const QString &first="ConeAngles",const QString &second="ClockAngles")
{
   const Rvector actualA=mask()->GetRvectorParameter(first.toStdString()),actualB=mask()->GetRvectorParameter(second.toStdString());
   require(actualA.GetSize()==a.size() && actualB.GetSize()==b.size(),"FOV did not retain the authored paired vertex count");
   for (int i=0;i<a.size();++i) require(std::abs(actualA[i]-a[i].toDouble())<1e-12 && std::abs(actualB[i]-b[i].toDouble())<1e-12,"FOV authored angle changed");
}
static void visibility(GmatBase &object,bool interpolated=false)
{
   QtResourcePreview copy(object.Clone()); require(copy->Initialize(),"Edited FOV failed initialization"); auto *fov=dynamic_cast<CustomFOV *>(copy.get());
   // The untouched engine also rejects exact boresight for this polygon.
   // Use a verified interior point away from rays through polygon vertices.
   require(fov && fov->CheckTargetVisibility(5*GmatMathConstants::RAD_PER_DEG,45*GmatMathConstants::RAD_PER_DEG) && !fov->CheckTargetVisibility(40*GmatMathConstants::RAD_PER_DEG,0),"Known interior/exterior FOV geometry disagrees");
   if (interpolated) require(fov->GetRvectorParameter("ConeAngles").GetSize()>4,"Azimuth/elevation interpolation mode was lost");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("CustomFovControls");
   if (argc!=2 || QGuiApplication::platformName()!="offscreen") return 2;
   try {
      TestSettings isolated; QTemporaryDir files; require(files.isValid(),"Temporary files unavailable"); const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"CustomFOV runtime initialization failed"); window.show(); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Script editor missing");
      const QString baseline="% Preserve unrelated mission β\nCreate Variable Keep;\nKeep = 7; % initializer remains exact\nBeginMissionSequence;\nKeep = Keep + 2; % expression remains exact\n";
      editor->setPlainText(baseline); require(window.buildScript(),"CustomFOV baseline failed");
      const auto blankError=window.createResource("CustomFOV","BlankFov",baseline);
      require(!blankError.isEmpty() && editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject("BlankFov"),"Untouched one-point CustomFOV was registered");
      const auto blankUndo=editor->document()->availableUndoSteps();
      const auto blankColor=window.createResource("CustomFOV","BlankColor",baseline,1,1,std::nullopt,{},{},{{"Color","Red"}});
      require(!blankColor.isEmpty() && editor->toPlainText()==baseline && editor->document()->availableUndoSteps()==blankUndo && !Moderator::Instance()->GetConfiguredObject("BlankColor"),"Color-only creator registered a one-point FOV");
      const QStringList cone{"20","20","20","20"},clock{"0","90","180","270"}; std::exception_ptr failure;
      const auto creator=[&](bool cancel) {
         QTimer::singleShot(0,&window,[&] {
            auto *dialog=window.findChild<QDialog *>("newResourceDialog");
            try {
               require(dialog,"CustomFOV creator missing"); auto *type=dialog->findChild<QComboBox *>("resourceType"); require(type && type->findText("CustomFOV")>=0,"CustomFOV is absent from creation"); type->setCurrentText("CustomFOV"); dialog->findChild<QLineEdit *>("resourceName")->setText("Mask");
               ResourceEditor *panel=nullptr; for (auto *widget:dialog->findChildren<QWidget *>()) if (auto *candidate=dynamic_cast<ResourceEditor *>(widget);candidate && candidate->findChild<QTableWidget *>("customFovVertices")) panel=candidate;
               require(panel,"Creator lacks paired FOV controls"); auto *create=dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok);
               if (!cancel) { create->click(); require(dialog->result()!=QDialog::Accepted && editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject("Mask"),"Blank creator bypassed paired validation"); }
               pairs(panel,cone,clock); require(editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject("Mask"),"Pending FOV creation modified configured source/model");
               if (cancel) { dialog->reject(); return; }
               panel->findChild<QTableWidget *>("customFovVertices")->item(0,0)->setText("nan"); const auto undo=editor->document()->availableUndoSteps(); create->click();
               require(dialog->result()!=QDialog::Accepted && editor->toPlainText()==baseline && editor->document()->availableUndoSteps()==undo && panel->hasChanges() && !Moderator::Instance()->GetConfiguredObject("Mask"),"Invalid FOV creation was not atomic/pending");
               panel->findChild<QTableWidget *>("customFovVertices")->item(0,0)->setText("20"); create->click();
               if (dialog->result()!=QDialog::Accepted) std::cerr<<dialog->findChild<QLabel *>("resourceCreationStatus")->text().toStdString()<<'\n';
               require(dialog->result()==QDialog::Accepted,"Valid paired FOV creation failed");
            } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
         });
         window.findChild<QAction *>("createResource")->trigger(); if (failure) std::rethrow_exception(failure);
      };
      creator(true); require(editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject("Mask"),"FOV creator Cancel changed mission");
      creator(false); checkPolygon(cone,clock); visibility(*mask()); const auto created=editor->toPlainText(); require(unrelated(created)==baseline && !created.contains("Mask.Azimuth") && !created.contains("Mask.Elevation") && !created.contains("Mask.FOVFileName"),"FOV creation wrote alternate inputs or changed unrelated bytes"); closeMask(window);
      editor->undo(); require(editor->toPlainText()==baseline && window.buildScript() && !Moderator::Instance()->GetConfiguredObject("Mask"),"FOV creation was not one exact Undo"); editor->redo(); require(editor->toPlainText()==created && window.buildScript(),"FOV creation Redo failed"); checkPolygon(cone,clock);
      auto commented=created; commented.replace(QRegularExpression("(Mask\\.ConeAngles[^\\n]*;)"),"\\1 % preserve FOV angle α"); editor->setPlainText(commented); require(window.buildScript(),"Commented FOV source failed");
      openMask(window); auto *panel=maskPanel(window); auto *table=panel->findChild<QTableWidget *>("customFovVertices");
      const auto pendingBefore=panel->hasChanges(); panel->findChild<QPushButton *>("customFovAddVertex")->click(); require(table->rowCount()==5,"FOV Add vertex did not grow paired rows"); table->setCurrentCell(4,0); panel->findChild<QPushButton *>("customFovRemoveVertex")->click(); require(table->rowCount()==4 && panel->hasChanges()==pendingBefore,"FOV Remove vertex did not restore pending pairs");
      // Standalone focused dialog Cancel never changes the retained owner.
      { CustomFovDialog canceled(*mask(),commented,&window); pairs(&canceled,{"10","10","10"},{"0","120","240"}); canceled.reject(); }
      require(editor->toPlainText()==commented,"FOV dialog Cancel changed mission source");
      table->item(0,0)->setText("-1"); auto *configured=mask(); const auto undo=editor->document()->availableUndoSteps(); panel->requestApply();
      require(mask()==configured && editor->toPlainText()==commented && editor->document()->availableUndoSteps()==undo && panel->hasChanges(),"Invalid angle Apply lost source/model/pending state"); table->item(0,0)->setText("20");
      panel->findChild<QComboBox *>("customFovCoordinates")->setCurrentIndex(1); panel->findChild<QLineEdit *>("customFov_InterpolationStepSize")->setText("30"); panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      const auto azel=editor->toPlainText(); require(azel!=commented && unrelated(azel)==baseline && azel.contains("Mask.Azimuth") && azel.contains("Mask.Elevation") && !azel.contains("Mask.ConeAngles") && !azel.contains("Mask.ClockAngles") && azel.count("preserve FOV angle α")==1,"Coordinate switch lost comments/unrelated source or wrote both representations");
      checkPolygon(clock,{"70","70","70","70"},"Azimuth","Elevation"); visibility(*mask(),true); require(!maskPanel(window)->hasChanges(),"Accepted FOV form reopened dirty"); closeMask(window);
      editor->undo(); require(editor->toPlainText()==commented && window.buildScript(),"FOV representation Apply was not one exact Undo"); editor->redo(); require(editor->toPlainText()==azel && window.buildScript(),"FOV representation Redo failed"); openMask(window);
      panel=maskPanel(window); require(panel->findChild<QComboBox *>("customFovCoordinates")->currentData().toString()=="AzimuthElevation" && panel->findChild<QTableWidget *>("customFovVertices")->rowCount()==4,"Imported Az/El form lost representation or materialized derived interpolation");
      // Unrelated Advanced Color Apply must preserve every authored Az/El
      // statement and interpolation setting rather than normalizing aliases.
      auto *advanced=panel->findChild<QTableWidget *>("resourceProperties"); int colorRow=-1;
      for (int row=0;row<advanced->rowCount();++row) if (advanced->item(row,0)->text()=="Color") colorRow=row;
      require(colorRow>=0,"FOV Color control missing"); advanced->item(colorRow,1)->setText("Red"); panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      const auto colored=editor->toPlainText(); require(colored!=azel && withoutProperty(colored,"Color")==withoutProperty(azel,"Color"),"Unrelated Color Apply changed FOV geometry/source"); closeMask(window);
      editor->undo(); require(editor->toPlainText()==azel && window.buildScript(),"Color Apply Undo changed original FOV source"); openMask(window); panel=maskPanel(window);
      const auto path=files.filePath("own mask α.txt"); QFile input(path); require(input.open(QIODevice::WriteOnly),"FOV input file unavailable"); input.write("ConeClockAngles\n20 0\n20 90\n20 180\n20 270\n"); input.close();
      auto *source=panel->findChild<QComboBox *>("customFovSourceMode"); source->setCurrentIndex(1); auto *file=panel->findChild<QLineEdit *>("customFov_FOVFileName"); file->setText(files.filePath("missing mask.txt"));
      require(!panel->findChild<QTableWidget *>("customFovVertices")->isEnabled() && !panel->findChild<QPushButton *>("customFovAddVertex")->isEnabled(),"FOV file mode did not disable vertices"); configured=mask(); panel->requestApply(); require(mask()==configured && editor->toPlainText()==azel && panel->hasChanges(),"Missing FOV file changed configured source/model");
      file->setText(path); panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); const auto fileSource=editor->toPlainText();
      require(mask()->GetStringParameter("FOVFileName")==path.toStdString() && fileSource.contains("Mask.FOVFileName") && !fileSource.contains(QRegularExpression("Mask\\.(?:ConeAngles|ClockAngles|Azimuth|Elevation)\\s*=")) && unrelated(fileSource)==baseline && fileSource.count("preserve FOV angle α")==1,"File-mode Apply retained explicit/derived angles or changed source/comments"); visibility(*mask());
      require(window.applyResourceChanges("Mask",{{"Alpha","128"}},fileSource).isEmpty(),"Unrelated file-backed Alpha Apply failed");
      const auto alpha=editor->toPlainText(); require(alpha!=fileSource && withoutProperty(alpha,"Alpha")==withoutProperty(fileSource,"Alpha"),"Unrelated Alpha Apply normalized file-backed geometry/source"); closeMask(window);
      editor->undo(); require(editor->toPlainText()==fileSource && window.buildScript(),"File-backed Alpha Undo changed original source");
      openMask(window); panel=maskPanel(window); require(panel->findChild<QComboBox *>("customFovSourceMode")->currentData().toString()=="File","Reopened FOV lost file mode"); panel->findChild<QComboBox *>("customFovSourceMode")->setCurrentIndex(0); panel->findChild<QComboBox *>("customFovCoordinates")->setCurrentIndex(0); pairs(panel,cone,clock); panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      const auto restored=editor->toPlainText(); require(mask()->GetStringParameter("FOVFileName").empty() && !restored.contains("Mask.FOVFileName") && unrelated(restored)==baseline,"File-to-vertices kept stale input or changed unrelated source"); checkPolygon(cone,clock); visibility(*mask()); closeMask(window);
      const auto saved=files.filePath("FOV β.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==restored,"Unicode FOV Save/reopen changed source"); checkPolygon(cone,clock); visibility(*mask());
      // An invalid coupled request cannot mutate even its detached owner.
      QtResourcePreview preview(mask()->Clone()); const auto before=QString::fromStdString(preview->GetGeneratingString(Gmat::SCRIPTING)); bool rejected=false;
      try { applyCustomFovProperties(*preview,{{"@CustomFovSource","Vertices"},{"@CustomFovCoordinates","ConeClock"},{"ConeAngles","20 20 20"},{"ClockAngles","0 90 180 270"}}); } catch (...) { rejected=true; }
      require(rejected && QString::fromStdString(preview->GetGeneratingString(Gmat::SCRIPTING))==before,"Mismatched pairs mutated the pending owner");
      // Detached validation must not reset the configured FOV search path,
      // including a rejected edit and a relative file resolved through it.
      {
         auto *manager=FileManager::Instance(); const auto state=manager->CaptureState();
         manager->SetAbsPathname("FOV_DATA_PATH",files.path().toStdString());
         const auto searchPath=manager->GetPathname("FOV_DATA_PATH");
         QtResourcePreview pending(mask()->Clone()); bool invalid=false;
         try { applyCustomFovProperties(*pending,{{"ConeAngles","20 20 20"},{"ClockAngles","0 90 180 270"}}); } catch (...) { invalid=true; }
         require(invalid && manager->GetPathname("FOV_DATA_PATH")==searchPath,"Rejected FOV edit reset the configured file search path");
         applyCustomFovProperties(*pending,{{"@CustomFovSource","File"},{"FOVFileName",QFileInfo(path).fileName()}});
         require(manager->GetPathname("FOV_DATA_PATH")==searchPath && pending->GetStringParameter("FOVFileName")==QFileInfo(path).fileName().toStdString(),"Relative FOV file edit reset or ignored the configured search path");
         applyCustomFovProperties(*pending,{{"@CustomFovSource","Vertices"},{"FOVFileName",""},{"ConeAngles","20 20 20 20"},{"ClockAngles","0 90 180 270"}});
         require(manager->GetPathname("FOV_DATA_PATH")==searchPath,"Valid vertex edit reset the configured file search path");
         manager->RestoreState(*state);
      }
      // Clone copies the chosen authored representation without materializing
      // aliases. Changed clone geometry uses the same grouped transaction.
      require(window.cloneResource("Mask","CopyMask",restored).isEmpty(),"Unchanged FOV Clone failed");
      const auto cloned=editor->toPlainText(); require(cloned.contains("Create CustomFOV CopyMask") && !cloned.contains("CopyMask.Azimuth") && !cloned.contains("CopyMask.Elevation") && !cloned.contains("CopyMask.FOVFileName"),"FOV Clone wrote unused inputs");
      editor->undo(); require(editor->toPlainText()==restored && window.buildScript() && !Moderator::Instance()->GetConfiguredObject("CopyMask"),"FOV Clone Undo failed");
      require(window.cloneResource("Mask","CopyMask",restored,{{"@CustomFovSource","Vertices"},{"@CustomFovCoordinates","ConeClock"},{"ConeAngles","10 10 10"},{"ClockAngles","0 120 240"}}).isEmpty(),"Changed FOV Clone failed");
      auto *copy=Moderator::Instance()->GetConfiguredObject("CopyMask"); require(copy && copy->GetRvectorParameter("ConeAngles").GetSize()==3 && mask()->GetRvectorParameter("ConeAngles").GetSize()==4,"Clone geometry changed the original FOV or lost pairs");
      editor->undo(); require(editor->toPlainText()==restored && window.buildScript(),"Changed FOV Clone Undo failed");
      std::cout<<"PASS CustomFOV paired creation/Cancel, atomic invalid-input recovery, Add/Remove, cone-clock and azimuth-elevation modes, interpolation retention without derived source, file transitions/recovery, exact unrelated source/comments/Undo/Redo/Unicode reopen, and known interior/exterior geometry; no mission propagation or native qualification.\n";
   } catch (BaseException &failure) { std::cerr<<failure.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &failure) { std::cerr<<failure.what()<<'\n'; return 1; }
}
