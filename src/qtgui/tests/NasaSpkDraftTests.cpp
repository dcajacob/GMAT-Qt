#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <iostream>
#include <functional>
#include <stdexcept>
static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static PropSetup *setup(const QString &name) {auto *p=dynamic_cast<PropSetup *>(Moderator::Instance()->GetConfiguredObject(name.toStdString()));require(p,"Draft test configured propagator missing");return p;}
template<class T> static T *field(QWidget *owner,const QString &name) {events();auto *p=owner->findChild<T *>("resource_"+name);require(p && p->isVisible(),qPrintable("Active draft field missing or hidden: "+name));return p;}
static void spk(QWidget *owner)
{
   field<QComboBox>(owner,"Type")->setCurrentText("SPK");events();
   auto *step=field<QLineEdit>(owner,"StepSize");auto *format=field<QComboBox>(owner,"EpochFormat");auto *epoch=field<QComboBox>(owner,"StartEpoch");
   require(step->text()=="300" && format->findText("UTCGregorian")>=0 && epoch->isEditable() && epoch->findText("FromSpacecraft")>=0,"Pending numeric-to-SPK draft lacks typed SPK defaults/source choices");
   auto *origin=field<QLineEdit>(owner,"CentralBody");require(origin->isReadOnly() && !origin->toolTip().isEmpty(),"Inactive SPK Origin is exposed as an effective editable setting");
   auto *fm=owner->findChild<QLineEdit *>("resource_FM");require(fm && !fm->isEnabled(),"SPK force model control remains active");
   step->setText("29");format->setCurrentText("UTCGregorian");epoch->setCurrentText("01 Jan 2000 12:03:00.000");
   format->setCurrentText("UTCModJulian");require(epoch->currentText()!="01 Jan 2000 12:03:00.000","Pending SPK format change did not convert the explicit epoch");format->setCurrentText("UTCGregorian");require(epoch->currentText()=="01 Jan 2000 12:03:00.000","Pending SPK format round trip moved the known explicit calendar epoch");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaSpkDraft");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup and SPK draft case");TestSettings settings;QTemporaryDir files;const QString mode=QString::fromLocal8Bit(argv[2]);require(mode=="Create" || mode=="Edit" || mode=="Families","Unknown SPK draft case");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;require(window.initialize(startup),"SPK draft runtime initialization failed");window.show();
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");require(source,"SPK draft source editor missing");const QString baseline="% Retain unrelated mission and configured model\nCreate ForceModel Shared;\nShared.PrimaryBodies = {};\nShared.PointMasses = {Earth};\nCreate Spacecraft Vehicle;\nCreate Propagator Existing;\nExisting.FM = Shared;\nExisting.InitialStepSize = 17;\nExisting.MaxStep = 730;\nCreate Variable Keep;\nKeep = 7;\nBeginMissionSequence;\nKeep = Keep + 1;\n";
      source->setPlainText(baseline);require(window.buildScript(),"SPK draft baseline did not build");const auto objects=Moderator::Instance()->GetListOfObjects(Gmat::PROP_SETUP),models=Moderator::Instance()->GetListOfObjects(Gmat::ODE_MODEL);int rejected=0;
      if (mode=="Create" || mode=="Families") {
         auto create=[&](const std::function<void(QDialog *)> &check) {std::exception_ptr error;QTimer::singleShot(0,&window,[&] {auto *dialog=window.findChild<QDialog *>("newResourceDialog");try {require(dialog,"Actual SPK creator missing");dialog->findChild<QComboBox *>("resourceType")->setCurrentText("PropSetup");check(dialog);}catch (...) {error=std::current_exception();}if (dialog && dialog->result()!=QDialog::Accepted) dialog->reject();});window.findChild<QAction *>("createResource")->trigger();if (error) std::rethrow_exception(error);};
         if (mode=="Families") {
            create([&](QDialog *dialog) {
               dialog->findChild<QLineEdit *>("resourceName")->setText("CreatedBS");field<QLineEdit>(dialog,"InitialStepSize")->setText("53");field<QLineEdit>(dialog,"Accuracy")->setText("2e-10");
               auto select=[&](const QString &name) {field<QComboBox>(dialog,"Type")->setCurrentText(name);events();require(field<QLineEdit>(dialog,"InitialStepSize")->text()=="53" && field<QLineEdit>(dialog,"Accuracy")->text()=="2e-10","New numerical family lost compatible pending controls");};
               select("AdamsBashforthMoulton");field<QLineEdit>(dialog,"LowerError")->setText("1e-15");field<QLineEdit>(dialog,"TargetError")->setText("1e-13");acceptanceView(*dialog);
               select("BulirschStoer");require(!dialog->findChild<QLineEdit *>("resource_LowerError"),"Inactive predictor-corrector controls retained in the active family");field<QLineEdit>(dialog,"MinimumReduction")->setText("0.1");field<QLineEdit>(dialog,"MaximumReduction")->setText("0.7");acceptanceView(*dialog);
               select("AdamsBashforthMoulton");require(field<QLineEdit>(dialog,"LowerError")->text()=="1e-15" && field<QLineEdit>(dialog,"TargetError")->text()=="1e-13","Returning family lost pending predictor-corrector settings");select("BulirschStoer");require(field<QLineEdit>(dialog,"MinimumReduction")->text()=="0.1" && field<QLineEdit>(dialog,"MaximumReduction")->text()=="0.7","Returning family lost pending extrapolator settings");
               dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok)->click();require(dialog->result()==QDialog::Accepted,"Pending extrapolator-family Create failed");
            });auto *p=setup("CreatedBS")->GetPropagator();require(p->GetTypeName()=="BulirschStoer" && p->GetRealParameter("InitialStepSize")==53 && p->GetRealParameter("Accuracy")==2e-10 && p->GetRealParameter("MinimumReduction")==.1 && p->GetRealParameter("MaximumReduction")==.7 && !source->toPlainText().contains("CreatedBS.LowerError"),"Created extrapolator omitted active settings or serialized inactive family controls");
         } else {
         create([&](QDialog *dialog) {field<QLineEdit>(dialog,"InitialStepSize")->setText("53");spk(dialog);acceptanceView(*dialog);field<QComboBox>(dialog,"Type")->setCurrentText("RungeKutta89");require(field<QLineEdit>(dialog,"InitialStepSize")->text()=="53","Switching back lost pending numeric settings");field<QComboBox>(dialog,"Type")->setCurrentText("SPK");require(field<QLineEdit>(dialog,"StepSize")->text()=="29" && field<QComboBox>(dialog,"StartEpoch")->currentText()=="01 Jan 2000 12:03:00.000","Returning to SPK lost its pending settings");dialog->reject();});
         require(source->toPlainText()==baseline && Moderator::Instance()->GetListOfObjects(Gmat::PROP_SETUP)==objects && Moderator::Instance()->GetListOfObjects(Gmat::ODE_MODEL)==models,"Draft type-switch Cancel changed source or registered resources");
         create([&](QDialog *dialog) {dialog->findChild<QLineEdit *>("resourceName")->setText("CreatedSpk");spk(dialog);auto *ok=dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok);const auto undo=source->document()->availableUndoSteps();field<QLineEdit>(dialog,"StepSize")->setText("0");ok->click();events();require(dialog->isVisible() && !dialog->findChild<QLabel *>("resourceCreationStatus")->text().isEmpty() && source->toPlainText()==baseline && source->document()->availableUndoSteps()==undo && Moderator::Instance()->GetListOfObjects(Gmat::PROP_SETUP)==objects,"Invalid pending SPK Create was not atomic");++rejected;field<QLineEdit>(dialog,"StepSize")->setText("29");acceptanceView(*dialog);ok->click();require(dialog->result()==QDialog::Accepted,"Corrected pending SPK Create failed");});
         auto *p=setup("CreatedSpk")->GetPropagator();require(p->GetTypeName()=="SPK" && p->GetRealParameter("StepSize")==29 && p->GetStringParameter("EpochFormat")=="UTCGregorian" && p->GetStringParameter("StartEpoch")=="01 Jan 2000 12:03:00.000" && !setup("CreatedSpk")->GetODEModel(),"Create did not install the selected SPK and exact pending tuple");require(Moderator::Instance()->GetListOfObjects(Gmat::ODE_MODEL)==models && !source->toPlainText().contains("CreatedSpk.InitialStepSize") && !source->toPlainText().contains("CreatedSpk.FM"),"SPK Create registered or serialized inactive numeric settings/model");
         const auto saved=files.filePath("SPK draft Δ.script");const auto applied=source->toPlainText();require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && source->toPlainText()==applied && setup("CreatedSpk")->GetPropagator()->GetRealParameter("StepSize")==29,"Created SPK draft did not survive exact Unicode save/reopen");
         }
      } else {
         auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(tree && area,"SPK draft workspace missing");ResourceEditor *panel=nullptr;QMdiSubWindow *child=nullptr;
         auto open=[&](bool trigger) {if (trigger) {const auto items=tree->findItems("Existing",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"SPK draft resource tree entry missing");tree->itemDoubleClicked(items.front(),0);events();}child=nullptr;for (auto *c:area->subWindowList()) if (c->property("resourceName").toString()=="Existing" && dynamic_cast<ResourceEditor *>(c->widget())) child=c;require(child,"Actual SPK draft resource editor missing");panel=dynamic_cast<ResourceEditor *>(child->widget());child->showNormal();area->setActiveSubWindow(child);events();acceptanceView(*child);require(!panel->hasChanges(),"Fresh SPK draft panel is dirty");};
         open(true);spk(panel);require(panel->hasChanges() && source->toPlainText()==baseline && setup("Existing")->GetPropagator()->GetTypeName()=="RungeKutta89","Pending SPK switch changed configured mission");panel->requestApply();events();open(false);require(setup("Existing")->GetPropagator()->GetTypeName()=="SPK" && setup("Existing")->GetPropagator()->GetRealParameter("StepSize")==29 && !setup("Existing")->GetODEModel(),"Existing numeric-to-SPK Apply did not commit selected owned propagator");const auto spkSource=source->toPlainText();
         const auto history=source->document()->availableUndoSteps();require(!window.applyResourceChanges("Existing",{{"CentralBody","Luna"}},spkSource).isEmpty() && source->toPlainText()==spkSource && source->document()->availableUndoSteps()==history && setup("Existing")->GetPropagator()->GetStringParameter("CentralBody")=="Earth","Inactive SPK Origin edit reported success or changed the mission");++rejected;
         field<QComboBox>(panel,"Type")->setCurrentText("PrinceDormand78");require(field<QLineEdit>(panel,"InitialStepSize")->isEnabled() && field<QLineEdit>(panel,"FM")->isEnabled(),"SPK-to-numeric pending controls missing/inactive");field<QLineEdit>(panel,"FM")->setText("Vehicle");
         auto choose=[&](bool accept) {std::exception_ptr error;QTimer::singleShot(0,panel,[&] {auto *dialog=panel->findChild<QDialog *>("propagatorForceModelDialog");try {require(dialog,"Force model selection dialog missing");auto *list=dialog->findChild<QListWidget *>("propagatorForceModelList");require(list && !list->findItems("Vehicle",Qt::MatchExactly).size(),"Force model picker includes a spacecraft");const auto items=list->findItems("Shared",Qt::MatchExactly);require(items.size()==1,"Configured force model absent from picker");list->setCurrentItem(items.front());acceptanceView(*dialog);dialog->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();}catch (...) {error=std::current_exception();if (dialog) dialog->reject();}});panel->findChild<QPushButton *>("propagatorForceModelChoose")->click();if (error) std::rethrow_exception(error);};
         choose(false);require(field<QLineEdit>(panel,"FM")->text()=="Vehicle" && source->toPlainText()==spkSource,"Force model picker Cancel changed pending/applied settings");choose(true);require(field<QLineEdit>(panel,"FM")->text()=="Shared" && source->toPlainText()==spkSource,"Force model picker did not retain pending selection before Apply");
         field<QLineEdit>(panel,"InitialStepSize")->setText("41");field<QLineEdit>(panel,"MaxStep")->setText("600");acceptanceView(*child);panel->requestApply();events();open(false);require(setup("Existing")->GetPropagator()->GetTypeName()=="PrinceDormand78" && setup("Existing")->GetPropagator()->GetRealParameter("InitialStepSize")==41 && setup("Existing")->GetStringParameter("FM")=="Shared","SPK-to-numeric Apply did not restore the selected model/settings");const auto numericSource=source->toPlainText();require(child->close(),"Clean switched propagator did not close");events();source->undo();require(source->toPlainText()==spkSource && window.buildScript() && setup("Existing")->GetPropagator()->GetTypeName()=="SPK","SPK reverse-switch Undo did not restore exact source/type");source->redo();require(source->toPlainText()==numericSource && window.buildScript() && setup("Existing")->GetPropagator()->GetTypeName()=="PrinceDormand78","SPK reverse-switch Redo did not restore exact source/type");require(Moderator::Instance()->GetListOfObjects(Gmat::ODE_MODEL)==models,"Existing type switches changed configured model membership");
      }
      const auto output=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if (!output.isEmpty()) {QFile saved(QDir(output).filePath("source.script"));require(saved.open(QIODevice::WriteOnly),"Cannot retain pending propagator source");saved.write(source->toPlainText().toUtf8());}
      const QJsonObject proof{{"checkpoint","spk-draft."+mode},{"outcome","passed"},{"invalid_rejections",rejected},{"facets",mode=="Create" ? QJsonArray{"pending-type-dependent-controls","inactive-origin-and-force-model","per-type-pending-retention","cancel-no-registration","invalid-create-atomicity","corrected-create","no-inactive-source-fields","unicode-save-reopen"} : mode=="Families" ? QJsonArray{"pending-type-dependent-controls","pending-numerical-family-controls","per-type-pending-retention","compatible-field-retention","no-inactive-source-fields","created-owned-family-values"} : QJsonArray{"pending-type-dependent-controls","numeric-to-spk-apply","spk-to-numeric-controls-apply","model-selection","exact-undo-redo","model-membership","inactive-origin-api-rejection"}},{"scope","Type-dependent PropSetup creation/editing only; kernel chooser and new scientific propagation qualification are separate"}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }catch (BaseException &e) {std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch (const std::exception &e) {std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
