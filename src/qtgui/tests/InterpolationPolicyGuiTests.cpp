#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "EphemerisFile.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
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
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool condition, const QString &message)
{
   if (!condition) throw std::runtime_error(message.toStdString());
}
void events()
{
   QApplication::processEvents();
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
   QApplication::processEvents();
}
void write(const QString &path, const QByteArray &data)
{
   QFile file(path);
   require(file.open(QIODevice::WriteOnly) && file.write(data)==data.size(),
         "Cannot retain interpolation GUI evidence");
}
void reveal(QWidget *widget)
{
   require(widget,"Interpolation GUI control missing");
   for (QWidget *parent=widget; parent; parent=parent->parentWidget())
      if (auto *scroll=qobject_cast<QScrollArea*>(parent)) scroll->ensureWidgetVisible(widget);
   events();
   require(widget->isVisible() && widget->isEnabled(),"Interpolation GUI control unreachable");
   acceptanceView(*widget);
}
void click(QDialogButtonBox *buttons,QDialogButtonBox::StandardButton which)
{
   auto *button=buttons ? buttons->button(which) : nullptr;
   reveal(button); QTest::mouseClick(button,Qt::LeftButton); events();
}
QString script(const QString &format,const QString &output)
{
   return "Create Spacecraft Sat;\nSat.NAIFId = -188801;\n"
      "Sat.DateFormat = A1ModJulian;\nSat.Epoch = '21545';\n"
      "Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\n"
      "Create Propagator P;\nP.FM = FM;\nP.InitialStepSize = 10;\nP.MinStep = 10;\nP.MaxStep = 10;\n"
      "Create EphemerisFile Export;\nExport.Spacecraft = Sat;\nExport.FileFormat = "+format+";\n"
      "Export.Filename = '"+output+"';\nExport.CoordinateSystem = EarthMJ2000Eq;\n"
      "Export.InterpolationOrder = 7;\nExport.StepSize = "+(format=="SPK" ? "IntegratorSteps" : "5")+";\n"
      +(format=="SPK" ? QString() : "Export.InsufficientDataAction = ReduceOrder;\n")+
      "BeginMissionSequence;\nPropagate 'Short interpolation arc' P(Sat) {Sat.ElapsedSecs = 10};\n";
}
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("InterpolationPolicyGui");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Supply startup and interpolation format");
      const QString format=QString::fromLocal8Bit(argv[2]);
      require(QStringList{"CCSDS-OEM","STK-TimePosVel","Code-500","SPK"}.contains(format),"Unknown interpolation format");
      const bool spk=format=="SPK";
      const int strictOrder=spk ? 9 : 7;
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      TestSettings settings; QTemporaryDir temporary;
      require(temporary.isValid(),"Interpolation GUI temporary directory unavailable");
      const auto retained=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      const auto directory=retained.isEmpty() ? temporary.path() : retained;
      const auto output=directory+"/Policy output Δ."+(spk ? "bsp" : "eph");
      const auto saved=directory+"/Policy round trip Δ.script";
      MainWindow window;
      require(window.initialize(startup),"Interpolation GUI runtime initialization failed");
      window.show(); acceptanceView(window); events();
      auto *source=window.findChild<QPlainTextEdit*>("scriptEditor");
      auto *messages=window.findChild<QPlainTextEdit*>("messageWindow");
      auto *area=window.findChild<QMdiArea*>("workspace");
      auto *tree=window.findChild<QTreeWidget*>("Resources");
      require(source && messages && area && tree,"Interpolation workspace missing");
      const auto initial=script(format,output);
      source->setPlainText(initial);
      require(window.buildScript(),"Interpolation GUI initial build failed: "+messages->toPlainText());
      source->document()->setModified(false);
      const int initialUndo=source->document()->availableUndoSteps();
      auto configured=[] { return Moderator::Instance()->GetConfiguredObject("Export"); };
      auto pristine=[&] {
         return source->toPlainText()==initial && source->document()->availableUndoSteps()==initialUndo &&
            !source->document()->isModified() && configured()->GetIntegerParameter("InterpolationOrder")==7 &&
            (spk || configured()->GetStringParameter("InsufficientDataAction")=="ReduceOrder");
      };
      QPointer<QMdiSubWindow> child;
      ResourceEditor *panel=nullptr;
      auto open=[&] {
         for (auto *tabs:window.findChildren<QTabWidget*>())
            for (int i=0;i<tabs->count();++i)
               if (tabs->widget(i)==tree || tabs->widget(i)->isAncestorOf(tree)) tabs->setCurrentIndex(i);
         const auto items=tree->findItems("Export",Qt::MatchExactly|Qt::MatchRecursive);
         require(items.size()==1,"Interpolation resource tree entry missing");
         tree->scrollToItem(items.first()); events(); reveal(tree);
         const auto point=tree->visualItemRect(items.first()).center();
         QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,point); events();
         QTest::mouseDClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,point); events();
         child=area->currentSubWindow(); panel=child ? dynamic_cast<ResourceEditor*>(child->widget()) : nullptr;
         require(panel,"Actual ephemeris Resource editor did not open");
         auto *dialog=panel->findChild<QDialog*>("ephemerisDialog");
         require(dialog && dialog->isVisible(),"Embedded ephemeris form missing");
         acceptanceView(*child);
         return dialog;
      };
      auto edit=[&](const QString &action,int order) {
         auto *dialog=open();
         auto *policy=dialog->findChild<QComboBox*>("ephemeris_InsufficientDataAction");
         require(policy && policy->count()==2 && policy->findText("Error")>=0 && policy->findText("ReduceOrder")>=0,
               "Interpolation policy choices differ");
         require(policy->isEnabled()==!spk,"SPK policy applicability differs");
         if (!spk) { reveal(policy); policy->setCurrentText(action); events(); acceptanceView(*policy); }
         auto *degree=dialog->findChild<QLineEdit*>("ephemeris_InterpolationOrder");
         reveal(degree); degree->setText(QString::number(order)); events(); acceptanceView(*degree);
         return dialog;
      };
      edit("Error",spk ? 3 : 7);
      require(panel->hasChanges() && pristine(),"Pending interpolation edit escaped draft isolation");
      auto closeChoice=[&](QMessageBox::StandardButton choice) {
         std::exception_ptr failure; QTimer timer; timer.setInterval(10);
         QObject::connect(&timer,&QTimer::timeout,&window,[&] {
            auto *box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (!box) return;
            timer.stop();
            try { acceptanceView(*box); auto *button=box->button(choice); require(button,"Draft close decision missing"); QTest::mouseClick(button,Qt::LeftButton); }
            catch (...) { failure=std::current_exception(); box->reject(); }
         });
         timer.start(); click(panel->findChild<QDialogButtonBox*>("resourceButtons"),QDialogButtonBox::Close);
         timer.stop(); if (failure) std::rethrow_exception(failure);
      };
      closeChoice(QMessageBox::Cancel);
      require(child && panel->hasChanges() && pristine(),"Close Cancel lost interpolation draft");
      closeChoice(QMessageBox::Discard);
      require(!child && pristine(),"Discard changed interpolation source/model/history");
      edit("Error",spk ? 6 : 0);
      click(panel->findChild<QDialogButtonBox*>("resourceButtons"),QDialogButtonBox::Apply);
      auto *status=panel->findChild<QLabel*>("resourceStatus");
      require(child && status && !status->text().isEmpty() && pristine() && panel->hasChanges(),
            "Invalid interpolation order changed source/model or lost its diagnostic");
      auto *degree=panel->findChild<QLineEdit*>("ephemeris_InterpolationOrder");
      reveal(degree); degree->setText(QString::number(strictOrder)); events();
      click(panel->findChild<QDialogButtonBox*>("resourceButtons"),QDialogButtonBox::Apply);
      const auto applied=source->toPlainText();
      require(applied!=initial && configured()->GetIntegerParameter("InterpolationOrder")==strictOrder,
            "Interpolation Apply failed");
      if (!spk) require(configured()->GetStringParameter("InsufficientDataAction")=="Error" &&
            applied.contains("Export.InsufficientDataAction = Error;"),"Strict policy was not committed/serialized");
      const int appliedUndo=source->document()->availableUndoSteps();
      panel=dynamic_cast<ResourceEditor*>(child->widget());
      auto *apply=panel->findChild<QDialogButtonBox*>("resourceButtons")->button(QDialogButtonBox::Apply);
      require(!panel->hasChanges() && apply && !apply->isEnabled(),"Clean interpolation Apply is active");
      QTest::mouseClick(apply,Qt::LeftButton); events();
      require(source->toPlainText()==applied && source->document()->availableUndoSteps()==appliedUndo,
            "Idempotent interpolation Apply changed history");
      child->close(); events();
      source->undo(); require(source->toPlainText()==initial && window.buildScript(),"Interpolation Undo failed");
      source->redo(); require(source->toPlainText()==applied && window.buildScript(),"Interpolation Redo failed");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() &&
            source->toPlainText()==applied,"Interpolation Unicode save/reload failed");
      {
      std::unique_ptr<GmatBase> cloned(configured()->Clone());
      EphemerisFile copied("CopiedPolicy"); copied.Copy(configured());
      require(cloned->GetIntegerParameter("InterpolationOrder")==strictOrder && copied.GetIntegerParameter(copied.GetParameterID("InterpolationOrder"))==strictOrder,
            "Interpolation order clone/copy failed");
      if (!spk) {
         require(cloned->GetStringParameter("InsufficientDataAction")=="Error" &&
               copied.GetStringParameter("InsufficientDataAction")=="Error","Interpolation policy clone/copy failed");
         bool rejected=false;
         try { cloned->SetStringParameter("InsufficientDataAction","UnknownPolicy"); }
         catch (BaseException&) { rejected=true; }
         require(rejected && cloned->GetStringParameter("InsufficientDataAction")=="Error",
               "Invalid policy mutated the cloned setting");
      }
      }
      messages->clear(); const auto failed=window.runMission(); events(); reveal(messages);
      const auto diagnostic=messages->toPlainText(); write(directory+"/strict-messages.txt",diagnostic.toUtf8());
      require(failed==MainWindow::RunResult::Failed && source->toPlainText()==applied,
            "Insufficient interpolation did not fail the mission without source edits");
      require(spk ? diagnostic.contains("SPK records must contain enough states for the interpolation degree") :
            diagnostic.contains("Insufficient data") && diagnostic.contains("order 7") &&
            diagnostic.contains("requires at least 8") && diagnostic.contains("received 2") && diagnostic.contains(output),
            "Insufficient interpolation diagnostic was not presented in the actual Message Window: "+diagnostic);
      write(directory+"/strict.script",applied.toUtf8());
      if (spk) require(!QFileInfo::exists(output),"Rejected SPK created an output kernel");
      edit("ReduceOrder",spk ? 3 : 7);
      click(panel->findChild<QDialogButtonBox*>("resourceButtons"),QDialogButtonBox::Apply);
      child->close(); events();
      const auto recovered=source->toPlainText();
      require(spk ? configured()->GetIntegerParameter("InterpolationOrder")==3 :
            configured()->GetStringParameter("InsufficientDataAction")=="ReduceOrder","Recovery edit was not committed");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() &&
            source->toPlainText()==recovered,"Recovery policy round trip failed");
      messages->clear(); const auto completed=window.runMission(); events(); reveal(messages);
      write(directory+"/recovery-messages.txt",messages->toPlainText().toUtf8());
      require(completed==MainWindow::RunResult::Completed && QFileInfo(output).size()>0 &&
            source->toPlainText()==recovered,"Interpolation recovery failed: "+messages->toPlainText());
      write(directory+"/recovery.script",recovered.toUtf8());
      write(directory+"/gui-verification.json",QJsonDocument(QJsonObject{
         {"format",format},{"status","passed"},{"actual_resource_tree_editor",true},
         {"strict_policy_configured",!spk},{"strict_order",strictOrder},{"SPK_policy_disabled",spk},{"draft_cancel_discard",true},
         {"invalid_order_atomic",true},{"apply_undo_redo_idempotent",true},{"unicode_persistence",true},
         {"native_clone_copy",true},{"invalid_policy_rejected",!spk},
         {"strict_run_failed",true},{"actual_message_window_error",true},
         {"recovery_completed",true},{"native_OS_input_qualified",false}}).toJson());
      source->document()->setModified(false);
      std::cout<<format.toStdString()<<" interpolation GUI policy passed\n";
      return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n'; }
     catch (std::exception &error) { std::cerr<<"FAIL "<<error.what()<<'\n'; }
   return 1;
}
