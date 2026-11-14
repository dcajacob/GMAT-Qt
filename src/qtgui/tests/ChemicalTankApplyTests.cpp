#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static void setField(QWidget *owner,const QString &name,const QString &value)
{
   if (auto *choice=owner->findChild<QComboBox *>("resource_"+name)) choice->setCurrentText(value);
   else if (auto *line=owner->findChild<QLineEdit *>("resource_"+name)) line->setText(value);
   else throw std::runtime_error(("Missing tank field "+name).toStdString());
}
static ResourceEditor *creatorPanel(QDialog *dialog)
{
   for (auto *candidate:dialog->findChildren<QWidget *>())
      if (auto *panel=dynamic_cast<ResourceEditor *>(candidate);panel && panel->isVisible()) return panel;
   throw std::runtime_error("Tank creator form missing");
}
static QMdiSubWindow *tankWindow(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList())
      if (child->property("resourceName").toString()=="MainTank") return child;
   throw std::runtime_error("Tank settings window missing");
}
static ResourceEditor *tankPanel(MainWindow &window)
{
   auto *panel=dynamic_cast<ResourceEditor *>(tankWindow(window)->widget());
   require(panel,"Tank settings form missing"); return panel;
}
static void closePanel(MainWindow &window)
{
   require(tankWindow(window)->close(),"Clean tank form failed to close");
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static void openPanel(MainWindow &window)
{
   auto *tree=window.findChild<QTreeWidget *>("Resources");
   const auto items=tree->findItems("MainTank",Qt::MatchExactly|Qt::MatchRecursive);
   require(items.size()==1,"Tank resource tree item missing"); tree->itemDoubleClicked(items.front(),0);
}
static void checkTank(double mass,double density,double volume,double pressure,const char *mode)
{
   auto *tank=Moderator::Instance()->GetConfiguredObject("MainTank"); require(tank,"Tank not registered");
   const auto equal=[](double a,double b) { return std::abs(a-b)<1e-11; };
   if (!equal(tank->GetRealParameter("FuelMass"),mass) || !equal(tank->GetRealParameter("FuelDensity"),density) ||
       !equal(tank->GetRealParameter("Volume"),volume) || !equal(tank->GetRealParameter("Pressure"),pressure) ||
       tank->GetStringParameter("PressureModel")!=mode) {
      std::cerr<<"Actual tank mass="<<tank->GetRealParameter("FuelMass")<<" density="<<tank->GetRealParameter("FuelDensity")
         <<" volume="<<tank->GetRealParameter("Volume")<<" pressure="<<tank->GetRealParameter("Pressure")
         <<" mode="<<tank->GetStringParameter("PressureModel")<<'\n';
      throw std::runtime_error("Coupled final tank configuration or pressure changed");
   }
   require(tank->Validate(),"Committed tank failed final validation");
}
static QString unrelatedSource(QString source)
{
   const QString declaration="Create ChemicalTank MainTank;\n";
   require(source.startsWith(declaration),"Tank declaration moved or changed"); source.remove(0,declaration.size());
   const QRegularExpression assignments("^(?:GMAT[ \\t]+)?MainTank\\.(?:FuelMass|FuelDensity|Volume|Pressure|PressureModel)[ \\t]*=[ \\t]*[^;\\n]*;[ \\t]*\\n",QRegularExpression::MultilineOption);
   source.remove(assignments); return source;
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("ChemicalTankApply");
   if (argc!=2 || !acceptanceWidgetPlatform()) return 2;
   try {
      TestSettings isolated; QTemporaryDir files; require(files.isValid(),"Temporary files unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Tank runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Script editor missing");
      const QString baseline="% Preserve unrelated source α\nCreate Spacecraft Vehicle;\nCreate Variable Keep;\nKeep = 7; % keep initializer\nBeginMissionSequence;\nKeep = Keep + 2; % keep mission\n";
      editor->setPlainText(baseline); require(window.buildScript(),"Tank baseline failed to build");
      std::exception_ptr failure;
      QTimer::singleShot(0,&window,[&] {
         auto *dialog=window.findChild<QDialog *>("newResourceDialog");
         try {
            require(dialog,"Resource creator missing");
            dialog->findChild<QComboBox *>("resourceType")->setCurrentText("ChemicalTank");
            dialog->findChild<QLineEdit *>("resourceName")->setText("MainTank");
            auto *panel=creatorPanel(dialog);
            setField(panel,"FuelMass","2500"); setField(panel,"FuelDensity","1000");
            setField(panel,"Pressure","5000"); setField(panel,"Volume","2");
            auto *create=dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok);
            create->click();
            require(dialog->result()!=QDialog::Accepted && editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject("MainTank") && panel->hasChanges(),"Invalid final tank creation did not retain draft and roll back");
            setField(panel,"FuelMass","1718"); create->click();
            if (dialog->result()!=QDialog::Accepted) std::cerr<<dialog->findChild<QLabel *>("resourceCreationStatus")->text().toStdString()<<'\n';
            require(dialog->result()==QDialog::Accepted,"Valid coupled tank creation failed");
         } catch (...) { failure=std::current_exception(); }
         if (dialog && dialog->result()!=QDialog::Accepted) dialog->reject();
      });
      window.findChild<QAction *>("createResource")->trigger(); if (failure) std::rethrow_exception(failure);
      checkTank(1718,1000,2,5000,"PressureRegulated");
      const auto created=editor->toPlainText(); require(unrelatedSource(created)==baseline,"Tank creation changed unrelated source");
      require(!tankPanel(window)->hasChanges(),"New tank form has unintended pending edits"); closePanel(window);
      editor->undo(); require(editor->toPlainText()==baseline && window.buildScript() && !Moderator::Instance()->GetConfiguredObject("MainTank"),"Tank creation was not one exact Undo");
      editor->redo(); require(editor->toPlainText()==created && window.buildScript(),"Tank creation Redo failed"); checkTank(1718,1000,2,5000,"PressureRegulated");
      openPanel(window); auto *panel=tankPanel(window);
      auto *original=Moderator::Instance()->GetConfiguredObject("MainTank"); const auto undoSteps=editor->document()->availableUndoSteps();
      setField(panel,"FuelMass","2500"); panel->requestApply();
      require(editor->toPlainText()==created && Moderator::Instance()->GetConfiguredObject("MainTank")==original && panel->hasChanges() && editor->document()->availableUndoSteps()==undoSteps,"Invalid final tank Apply changed source, registered resource or Undo history");
      checkTank(1718,1000,2,5000,"PressureRegulated");
      require(panel->findChild<QLabel *>("resourceStatus")->text().contains("Fuel volume exceeds tank capacity"),"Invalid final tank Apply did not explain capacity error");
      setField(panel,"FuelMass","1500"); setField(panel,"PressureModel","BlowDown"); setField(panel,"Pressure","6000");
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      checkTank(1500,1000,2,6000,"BlowDown"); require(!tankPanel(window)->hasChanges(),"Corrected tank Apply retained dirty form");
      const auto corrected=editor->toPlainText(); require(unrelatedSource(corrected)==baseline,"Corrected tank Apply changed unrelated source");
      // With existing BlowDown mode, editing configuration is not fuel depletion.
      // Leave Pressure untouched and require its intended value to survive the
      // simultaneous mass/density/capacity change and serializer/rebuild.
      panel=tankPanel(window); setField(panel,"FuelMass","1200"); setField(panel,"FuelDensity","900"); setField(panel,"Volume","2.5");
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      checkTank(1200,900,2.5,6000,"BlowDown");
      const auto edited=editor->toPlainText(); require(edited!=corrected && unrelatedSource(edited)==baseline,"BlowDown settings changed unrelated source or failed to commit"); closePanel(window);
      editor->undo(); require(editor->toPlainText()==corrected && window.buildScript(),"Coupled BlowDown edit was not one exact Undo"); checkTank(1500,1000,2,6000,"BlowDown");
      editor->redo(); require(editor->toPlainText()==edited && window.buildScript(),"Coupled BlowDown edit Redo failed"); checkTank(1200,900,2.5,6000,"BlowDown");
      const auto saved=files.filePath("tank α.script");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==edited,"Tank save/reopen changed source"); checkTank(1200,900,2.5,6000,"BlowDown");
      std::cout<<"PASS coupled ChemicalTank creation 1718/1000/2/5000, invalid final creation/Apply rollback and correction, explicit BlowDown pressure and untouched pressure preservation, unrelated source, exact Undo/Redo and Unicode save/reopen; no mission numerical or native desktop run.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
