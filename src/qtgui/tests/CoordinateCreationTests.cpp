#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static void setField(QWidget *owner,const QString &name,const QString &value)
{
   if (auto *choice=owner->findChild<QComboBox *>("resource_"+name)) choice->setCurrentText(value);
   else if (auto *line=owner->findChild<QLineEdit *>("resource_"+name)) line->setText(value);
   else throw std::runtime_error(("Missing coordinate field "+name).toStdString());
}
static QString field(QWidget *owner,const QString &name)
{
   if (auto *choice=owner->findChild<QComboBox *>("resource_"+name)) return choice->currentText();
   if (auto *line=owner->findChild<QLineEdit *>("resource_"+name)) return line->text();
   throw std::runtime_error(("Missing coordinate field "+name).toStdString());
}
// This fixture adds one declaration and these coordinate-only assignments.
// Remove only their complete plain lines; compare every remaining original byte.
static QString unrelatedSource(QString source)
{
   const QString declaration="Create CoordinateSystem Frame;\n";
   require(source.startsWith(declaration),"Coordinate creation declaration moved or changed");
   source.remove(0,declaration.size());
   const QRegularExpression assignments("^(?:GMAT[ \t]+)?Frame\\.(?:Origin|Axes|Primary|Secondary|XAxis|YAxis|ZAxis)[ \t]*=[ \t]*[^;\\n]*;[ \t]*\\n",QRegularExpression::MultilineOption);
   source.remove(assignments); return source;
}
static ResourceEditor *creatorPanel(QDialog *dialog)
{
   for (auto *candidate:dialog->findChildren<QWidget *>())
      if (auto *panel=dynamic_cast<ResourceEditor *>(candidate);panel && panel->isVisible()) return panel;
   throw std::runtime_error("Coordinate creator form missing");
}
static void axes(ResourceEditor *panel,const std::function<void(QDialog *)> &check)
{
   std::exception_ptr failure;
   QTimer::singleShot(0,panel,[&] {
      auto *dialog=panel->findChild<QDialog *>("coordinateAxesDialog");
      try { require(dialog,"Nested axes dialog missing"); check(dialog); }
      catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
   });
   auto *button=panel->findChild<QPushButton *>("editCoordinateAxes"); require(button,"Coordinate axes action missing");
   button->click(); if (failure) std::rethrow_exception(failure);
}
static void acceptAxes(QDialog *dialog)
{
   auto *buttons=dialog->findChild<QDialogButtonBox *>("resourceButtons");
   require(buttons && buttons->button(QDialogButtonBox::Apply)->text()=="OK","Nested axes must retain settings with OK");
   buttons->button(QDialogButtonBox::Apply)->click();
   require(dialog->result()==QDialog::Accepted,"Nested axes OK failed");
}
static void radial(QDialog *dialog)
{
   dialog->findChild<QComboBox *>("coordinateAxisType")->setCurrentText("ObjectReferenced");
   setField(dialog,"Primary","Earth"); setField(dialog,"Secondary","Vehicle");
   setField(dialog,"XAxis","R"); setField(dialog,"YAxis",""); setField(dialog,"ZAxis","N");
}
static QMdiSubWindow *resourceWindow(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList())
      if (child->property("resourceName").toString()=="Frame") return child;
   throw std::runtime_error("Created coordinate editor missing");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("CoordinateCreation");
   if (argc!=2 || !acceptanceWidgetPlatform()) return 2;
   try {
      TestSettings isolated; QTemporaryDir files; require(files.isValid(),"Temporary files unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Coordinate runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString baseline="% Retain unrelated source α and defaults\nCreate Spacecraft Vehicle;\nCreate Variable Keep;\nKeep = 7; % keep initializer\nBeginMissionSequence;\nKeep = Keep + 2; % keep mission\n";
      editor->setPlainText(baseline); require(window.buildScript(),"Coordinate baseline failed");
      std::exception_ptr failure;
      const auto create=[&](const std::function<void(QDialog *,ResourceEditor *)> &check) {
         QTimer::singleShot(0,&window,[&] {
            auto *dialog=window.findChild<QDialog *>("newResourceDialog");
            try {
               require(dialog,"Resource creator missing"); dialog->findChild<QComboBox *>("resourceType")->setCurrentText("CoordinateSystem");
               dialog->findChild<QLineEdit *>("resourceName")->setText("Frame"); check(dialog,creatorPanel(dialog));
            } catch (...) { failure=std::current_exception(); }
            if (dialog && dialog->result()!=QDialog::Accepted) dialog->reject();
         });
         window.findChild<QAction *>("createResource")->trigger(); if (failure) std::rethrow_exception(failure);
      };
      create([&](QDialog *dialog,ResourceEditor *panel) {
         require(field(panel,"Axes")=="MJ2000Eq","Coordinate draft lacks default axes");
         axes(panel,[&](QDialog *nested) { radial(nested); nested->reject(); });
         require(field(panel,"Axes")=="MJ2000Eq" && !panel->hasChanges(),"Nested Cancel changed draft axes");
         setField(panel,"Origin","Mars");
         axes(panel,[&](QDialog *nested) { radial(nested); acceptAxes(nested); });
         require(panel->hasChanges() && field(panel,"Axes")=="ObjectReferenced" && field(panel,"Origin")=="Mars","Mixed coordinate edits not retained");
         axes(panel,[&](QDialog *nested) {
            require(nested->findChild<QComboBox *>("coordinateAxisType")->currentText()=="ObjectReferenced" && field(nested,"Secondary")=="Vehicle","Reopening lost pending axis fields");
            nested->findChild<QComboBox *>("coordinateAxisType")->setCurrentText("MJ2000Ec"); acceptAxes(nested);
         });
         require(field(panel,"Axes")=="MJ2000Ec","Pending axis type switch ignored");
         require(editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject("Frame"),"Nested OK committed before outer Create");
         dialog->reject();
      });
      require(editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject("Frame"),"Creator Cancel registered coordinate draft");
      create([&](QDialog *dialog,ResourceEditor *panel) {
         setField(panel,"Origin","Mars");
         axes(panel,[&](QDialog *nested) { radial(nested); setField(nested,"YAxis","R"); setField(nested,"ZAxis",""); acceptAxes(nested); });
         require(editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject("Frame"),"Nested invalid settings committed source");
         auto *commit=dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok);
         commit->click();
         require(dialog->result()!=QDialog::Accepted && editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject("Frame") && panel->hasChanges(),"Invalid outer Create did not retain draft/rollback");
         axes(panel,[&](QDialog *nested) { radial(nested); acceptAxes(nested); });
         require(editor->toPlainText()==baseline,"Correcting axes committed before outer Create");
         commit->click(); require(dialog->result()==QDialog::Accepted,"Corrected mixed coordinate Create failed");
      });
      auto *object=Moderator::Instance()->GetConfiguredObject("Frame");
      require(object && object->GetStringParameter("Origin")=="Mars" && object->GetStringParameter("Axes")=="ObjectReferenced" && object->GetOwnedObject(0)->GetStringParameter("Secondary")=="Vehicle","Outer Create lost origin/axis dependencies");
      const auto created=editor->toPlainText(); require(unrelatedSource(created)==baseline,"Creation rewrote unrelated source");
      auto *child=resourceWindow(window); require(child->close(),"Clean created editor failed to close"); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      editor->undo(); require(editor->toPlainText()==baseline && window.buildScript() && !Moderator::Instance()->GetConfiguredObject("Frame"),"Creation was not one exact Undo");
      editor->redo(); require(editor->toPlainText()==created && window.buildScript(),"Creation Redo failed");
      const auto saved=files.filePath("coordinates α.script");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==created,"Coordinate Unicode save/reopen changed source");
      // Exercise the retained MDI editor rather than a standalone owner that closes on Apply.
      auto *tree=window.findChild<QTreeWidget *>("Resources");
      const auto items=tree->findItems("Frame",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Coordinate tree item missing"); tree->itemDoubleClicked(items.front(),0);
      child=resourceWindow(window); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"Coordinate MDI form missing");
      setField(panel,"Origin","Earth");
      axes(panel,[&](QDialog *nested) { nested->findChild<QComboBox *>("coordinateAxisType")->setCurrentText("MJ2000Ec"); acceptAxes(nested); });
      require(editor->toPlainText()==created && panel->hasChanges() && Moderator::Instance()->GetConfiguredObject("Frame")->GetStringParameter("Origin")=="Mars","Nested edit bypassed parent Apply");
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      object=Moderator::Instance()->GetConfiguredObject("Frame");
      require(object->GetStringParameter("Origin")=="Earth" && object->GetStringParameter("Axes")=="MJ2000Ec" && child->isVisible() && !dynamic_cast<ResourceEditor *>(child->widget())->hasChanges(),"Parent Apply lost mixed edits or retained clean editor");
      const auto edited=editor->toPlainText(); require(unrelatedSource(edited)==baseline,"Coordinate Apply rewrote unrelated source");
      editor->undo(); require(editor->toPlainText()==created && window.buildScript(),"Coordinate Apply Undo changed source");
      editor->redo(); require(editor->toPlainText()==edited && window.buildScript(),"Coordinate Apply Redo changed source");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==edited,"Edited coordinate save/reopen changed source");
      std::cout<<"PASS coordinate default draft axes, nested Cancel/OK, pending reopen/type switches, mixed Origin/dependents, invalid outer Create rollback/correction, atomic creation and retained Apply, exact unrelated source/Undo/Redo/Unicode reopen; no numerical mission or native desktop test.\n";
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
