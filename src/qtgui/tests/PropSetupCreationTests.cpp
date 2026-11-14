#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "ResourceDraft.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "ODEModel.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPushButton>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static PropSetup *setup(const QString &name)
{
   auto *value=dynamic_cast<PropSetup *>(Moderator::Instance()->GetConfiguredObject(name.toStdString()));
   require(value,"Created propagator missing"); return value;
}
static ODEModel *force(const QString &name)
{
   auto *value=dynamic_cast<ODEModel *>(Moderator::Instance()->GetConfiguredObject(name.toStdString()));
   require(value,"Created local force model missing"); return value;
}
static void earthGravity(ODEModel *model)
{
   bool found=false;
   for (int i=0;i<model->GetOwnedObjectCount();++i) {
      auto *component=model->GetOwnedObject(i);
      if (!component || !component->IsOfType("GravityField")) continue;
      require(component->GetStringParameter("BodyName")=="Earth","Default gravity body differs from wx Earth default");
      require(QFileInfo(QString::fromStdString(component->GetStringParameter("PotentialFile"))).fileName()=="JGM2.cof","Default gravity potential differs from wx JGM2 default");
      found=true;
   }
   require(found,"New default model lacks wx Earth gravity");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   if (!acceptanceWidgetPlatform() || argc!=2) return 2;
   QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("PropSetupCreation");
   const char *stage="runtime initialization";
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Temporary directory unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Script editor missing");
      const QString baseline="% Preserve imported implicit propagator and comments α\nCreate Spacecraft Vehicle;\nCreate PropSetup Imported;\nCreate ForceModel Existing;\nCreate Variable Keep Collision_ForceModel;\nKeep = 7; % keep this spelling\nCollision_ForceModel = 37;\nBeginMissionSequence;\nKeep = Keep + 2;\n";
      stage="baseline build";
      editor->setPlainText(baseline); require(window.buildScript(),"Baseline build failed");
      const auto importedModel=QString::fromStdString(setup("Imported")->GetStringParameter("FM"));
      const auto originalModels=Moderator::Instance()->GetListOfObjects(Gmat::ODE_MODEL);
      const auto originalProps=Moderator::Instance()->GetListOfObjects(Gmat::PROP_SETUP);
      stage="creator preview Cancel";
      std::exception_ptr failure; bool canceled=false;
      QTimer::singleShot(0,&window,[&] {
         auto *dialog=window.findChild<QDialog *>("newResourceDialog");
         try {
            require(dialog,"New resource dialog missing");
            auto *type=dialog->findChild<QComboBox *>("resourceType"); require(type,"Resource type selector missing"); type->setCurrentText("PropSetup");
            auto *model=dialog->findChild<QLineEdit *>("resource_FM");
            require(model && model->text().endsWith("_ForceModel") && !Moderator::Instance()->GetConfiguredObject(model->text().toStdString()),"Creation preview exposes or registers an implicit/shared force model");
            canceled=true;
         } catch (...) { failure=std::current_exception(); }
         if (dialog) dialog->reject();
      });
      window.findChild<QAction *>("createResource")->trigger(); if (failure) std::rethrow_exception(failure);
      require(canceled && editor->toPlainText()==baseline && Moderator::Instance()->GetListOfObjects(Gmat::ODE_MODEL)==originalModels && Moderator::Instance()->GetListOfObjects(Gmat::PROP_SETUP)==originalProps,"Opening/canceling propagator preview changed configured mission");
      const auto creator=[&](const std::function<void(QDialog *)> &check) {
         failure=nullptr;
         QTimer::singleShot(0,&window,[&] {
            auto *dialog=window.findChild<QDialog *>("newResourceDialog");
            try { require(dialog,"Resource creator missing"); dialog->findChild<QComboBox *>("resourceType")->setCurrentText("PropSetup"); check(dialog); }
            catch (...) { failure=std::current_exception(); }
            if (dialog && dialog->result()!=QDialog::Accepted) dialog->reject();
         });
         window.findChild<QAction *>("createResource")->trigger(); if (failure) std::rethrow_exception(failure);
      };
      const auto acceptCreator=[](QDialog *dialog) {
         dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok)->click();
         if (dialog->result()!=QDialog::Accepted) std::cerr<<dialog->findChild<QLabel *>("resourceCreationStatus")->text().toStdString()<<'\n';
         require(dialog->result()==QDialog::Accepted,"Actual propagator creator failed to accept");
      };
      const auto closeCreated=[&](const QString &name) {
         for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("resourceName").toString()==name)
            require(child->close(),"Clean created propagator panel failed to close");
         QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      };
      stage="actual creator acceptance and pending settings";
      QString previewModel;
      creator([&](QDialog *dialog) {
         auto *name=dialog->findChild<QLineEdit *>("resourceName"); auto *model=dialog->findChild<QLineEdit *>("resource_FM");
         require(name && model,"Actual creator name/model fields missing"); previewModel=model->text();
         require(!Moderator::Instance()->GetConfiguredObject(previewModel.toStdString()),"Creator preview model registered before Create");
         name->setText("StagingProp"); name->setText("GuiCreated");
         require(model->text()==previewModel && editor->toPlainText()==baseline,"Outer name change committed preview state");
         dialog->findChild<QComboBox *>("resource_Type")->setCurrentText("PrinceDormand78");
         dialog->findChild<QLineEdit *>("resource_InitialStepSize")->setText("600");
         dialog->findChild<QLineEdit *>("resource_Accuracy")->setText("1e-12");
         acceptCreator(dialog);
      });
      const auto guiSource=editor->toPlainText();
      require(setup("GuiCreated")->GetStringParameter("FM")=="GuiCreated_ForceModel" && setup("GuiCreated")->GetPropagator()->GetTypeName()=="PrinceDormand78" && setup("GuiCreated")->GetRealParameter("InitialStepSize")==600 && std::abs(setup("GuiCreated")->GetRealParameter("Accuracy")-1e-12)<1e-25,"Creator name change or pending Type/numeric fields lost");
      stage="actual creator default Earth/JGM2 gravity";
      earthGravity(force("GuiCreated_ForceModel"));
      auto guiUnrelated=guiSource;
      const QString guiDeclarations="Create ForceModel GuiCreated_ForceModel;\nCreate PropSetup GuiCreated;\n";
      require(guiUnrelated.startsWith(guiDeclarations),"Actual creator declarations moved or changed"); guiUnrelated.remove(0,guiDeclarations.size());
      guiUnrelated.remove(QRegularExpression("^(?:GMAT[ \\t]+)?GuiCreated\\.(?:Type|FM|InitialStepSize|Accuracy)[ \\t]*=[ \\t]*[^;\\n]*;[ \\t]*\\n",QRegularExpression::MultilineOption));
      require(guiSource.startsWith("Create ForceModel GuiCreated_ForceModel;\nCreate PropSetup GuiCreated;\n") && guiSource.contains("GuiCreated.FM = GuiCreated_ForceModel;") && guiSource.contains("GuiCreated.Type = PrinceDormand78;") && guiUnrelated==baseline && !Moderator::Instance()->GetConfiguredObject(previewModel.toStdString()) && !Moderator::Instance()->GetConfiguredObject("StagingProp") && !Moderator::Instance()->GetConfiguredObject("StagingProp_ForceModel"),"Actual creator registered stale preview/intermediate names or changed original source");
      stage="actual creator exact source Undo/Redo";
      closeCreated("GuiCreated"); editor->undo();
      require(editor->toPlainText()==baseline && window.buildScript() && !Moderator::Instance()->GetConfiguredObject("GuiCreated") && !Moderator::Instance()->GetConfiguredObject("GuiCreated_ForceModel"),"Actual creator propagator/model/settings were not one exact Undo");
      editor->redo(); require(editor->toPlainText()==guiSource && window.buildScript() && setup("GuiCreated")->GetStringParameter("FM")=="GuiCreated_ForceModel","Actual creator Redo lost scoped model");
      stage="actual creator explicit configured implicit-name model";
      creator([&](QDialog *dialog) {
         dialog->findChild<QLineEdit *>("resourceName")->setText("GuiImportedChoice");
         dialog->findChild<QLineEdit *>("resource_FM")->setText(importedModel);
         acceptCreator(dialog);
      });
      require(setup("GuiImportedChoice")->GetStringParameter("FM")==importedModel.toStdString() && !Moderator::Instance()->GetConfiguredObject("GuiImportedChoice_ForceModel") && !editor->toPlainText().contains("Create ForceModel GuiImportedChoice_ForceModel;"),"Actual explicit configured implicit-name model choice was replaced by an automatic model");
      closeCreated("GuiImportedChoice");
      const auto beforeFirst=editor->toPlainText();
      const auto create=[&](const QString &name,const QMap<QString,QString> &values=QMap<QString,QString>{}) {
         const auto error=window.createResource("PropSetup",name,editor->toPlainText(),1,1,std::nullopt,{},{},values);
         require(error.isEmpty(),qPrintable(error));
      };
      stage="direct default First creation and Earth/JGM2 gravity";
      create("First"); const auto firstSource=editor->toPlainText();
      require(firstSource.startsWith("Create ForceModel First_ForceModel;\nCreate PropSetup First;\nGMAT First.FM = First_ForceModel;\n") && firstSource.endsWith(beforeFirst),"First creation lacks one local declaration/link or changes imported source");
      require(setup("First")->GetStringParameter("FM")=="First_ForceModel" && setup("First")->GetPropagator()->GetTypeName()=="RungeKutta89","First default propagator/link incorrect"); earthGravity(force("First_ForceModel"));
      stage="direct First exact source Undo/Redo";
      editor->undo(); require(editor->toPlainText()==beforeFirst && window.buildScript() && !Moderator::Instance()->GetConfiguredObject("First") && !Moderator::Instance()->GetConfiguredObject("First_ForceModel"),"Propagator and local force model were not one exact Undo");
      editor->redo(); require(editor->toPlainText()==firstSource && window.buildScript() && setup("First")->GetStringParameter("FM")=="First_ForceModel","Propagator/local-model Redo failed");
      stage="direct Second integrator settings and Earth/JGM2 gravity";
      create("Second",{{"Type","PrinceDormand78"},{"InitialStepSize","600"},{"Accuracy","1e-12"},{"MinStep","0"},{"MaxStep","86400"}});
      require(setup("Second")->GetStringParameter("FM")=="Second_ForceModel" && setup("Second")->GetPropagator()->GetTypeName()=="PrinceDormand78" && setup("Second")->GetRealParameter("InitialStepSize")==600 && setup("Second")->GetRealParameter("MinStep")==0 && setup("Second")->GetRealParameter("MaxStep")==86400 && std::abs(setup("Second")->GetRealParameter("Accuracy")-1e-12)<1e-25,"Second scoped model or requested integrator settings lost");
      earthGravity(force("Second_ForceModel"));
      require(force("First_ForceModel")!=force("Second_ForceModel") && setup("First")->GetODEModel()!=setup("Second")->GetODEModel(),"Separate new propagators share one force model");
      stage="independent model SRP edit";
      auto error=window.applyResourceChanges("First_ForceModel",{{"SRP","On"}},editor->toPlainText()); require(error.isEmpty(),qPrintable(error));
      require(force("First_ForceModel")->GetOnOffParameter("SRP")=="On" && force("Second_ForceModel")->GetOnOffParameter("SRP")=="Off" && setup("Second")->GetODEModel()->GetOnOffParameter("SRP")=="Off","Editing one new model changed another propagator");
      stage="explicit existing model selection";
      create("Selected",{{"FM","Existing"},{"Type","PrinceDormand78"}});
      require(setup("Selected")->GetStringParameter("FM")=="Existing" && setup("Selected")->GetPropagator()->GetTypeName()=="PrinceDormand78" && !Moderator::Instance()->GetConfiguredObject("Selected_ForceModel") && !editor->toPlainText().contains("Create ForceModel Selected_ForceModel;"),"Explicit existing force-model selection generated or selected another model");
      stage="model name collision protection";
      create("Collision"); require(setup("Collision")->GetStringParameter("FM")=="Collision_ForceModel1" && force("Collision_ForceModel1") && Moderator::Instance()->GetConfiguredObject("Collision_ForceModel")->IsOfType("Variable") && Moderator::Instance()->GetConfiguredObject("Collision_ForceModel")->GetRealParameter("Value")==37,"Scoped model name collision overwrote an existing resource");
      stage="missing existing model rejection";
      const auto stable=editor->toPlainText();
      error=window.createResource("PropSetup","Invalid",stable,1,1,std::nullopt,{},{},{{"FM","Missing"}});
      require(!error.isEmpty() && editor->toPlainText()==stable && !Moderator::Instance()->GetConfiguredObject("Invalid") && !Moderator::Instance()->GetConfiguredObject("Invalid_ForceModel"),"Rejected existing-model choice left creation artifacts");
      stage="registered analytical type discovery";
      QString analytical;
      const auto registeredTypes=Moderator::Instance()->GetListOfFactoryItems(Gmat::PROPAGATOR);
      for (const auto &type:registeredTypes)
         if (!propagatorUsesForceModel(QString::fromStdString(type))) { analytical=QString::fromStdString(type); break; }
      if (!analytical.isEmpty()) {
         stage="analytical creation without numerical default model";
         create("Analytical",{{"Type",analytical}});
         require(setup("Analytical")->GetPropagator()->GetTypeName()==analytical.toStdString() && !setup("Analytical")->GetPropagator()->UsesODEModel() && !Moderator::Instance()->GetConfiguredObject("Analytical_ForceModel") && !editor->toPlainText().contains("Create ForceModel Analytical_ForceModel;"),"Analytical creation changed Type or materialized a numerical model");
      }
      stage="imported implicit preservation and Unicode save/reopen";
      const auto finalSource=editor->toPlainText();
      require(finalSource.contains("Create PropSetup Imported;\n") && !finalSource.contains("Create ForceModel Imported_ForceModel;") && QString::fromStdString(setup("Imported")->GetStringParameter("FM"))==importedModel,"Imported implicit propagator was materialized or relinked");
      const auto saved=files.filePath("scoped propagators α.script"); require(window.saveScriptTo(saved),"Save failed");
      QFile savedFile(saved); require(savedFile.open(QIODevice::ReadOnly) && savedFile.readAll()==finalSource.toUtf8(),"Saved source differs from accepted creation source");
      require(window.loadScript(saved) && window.buildScript() && editor->toPlainText()==finalSource,"Unicode save/reopen changed source");
      require(setup("GuiCreated")->GetStringParameter("FM")=="GuiCreated_ForceModel" && setup("GuiCreated")->GetPropagator()->GetTypeName()=="PrinceDormand78" && setup("GuiImportedChoice")->GetStringParameter("FM")==importedModel.toStdString() && setup("First")->GetStringParameter("FM")=="First_ForceModel" && setup("Second")->GetStringParameter("FM")=="Second_ForceModel" && setup("Selected")->GetStringParameter("FM")=="Existing" && setup("Second")->GetPropagator()->GetTypeName()=="PrinceDormand78" && force("First_ForceModel")->GetOnOffParameter("SRP")=="On" && force("Second_ForceModel")->GetOnOffParameter("SRP")=="Off","Save/reopen lost independent models, links, edits or integrator type");
      std::cout<<"PASS new propagator Cancel and actual creator acceptance/name changes/Type+numeric settings/configured implicit-name selection, scoped wx Earth/JGM2 defaults, two independent models and edit isolation, explicit existing selection, collision/rejection protection, exact source/Undo/Redo/Unicode save-reopen and imported implicit preservation";
      if (analytical.isEmpty()) std::cout<<"; analytical plugin unavailable (not qualified)"; else std::cout<<"; analytical Type "<<analytical.toStdString()<<" preserved without default FM";
      std::cout<<". No mission or scientific propagation run.\n"; return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL at "<<stage<<": "<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<"FAIL at "<<stage<<": "<<error.what()<<'\n'; return 1; }
}
