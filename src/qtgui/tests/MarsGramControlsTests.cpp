#include "AcceptanceView.hpp"
#include "AtmosphereDialog.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "ODEModel.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value,const char *message)
{ if (!value) throw std::runtime_error(message); }
void write(const QString &path,const QByteArray &bytes)
{
   QFile file(path);
   require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),
           "Could not retain atmosphere editor evidence");
}
void expose(AtmosphereDialog &dialog)
{ dialog.show(); QApplication::processEvents(); acceptanceView(dialog); }
}

int main(int argc,char **argv)
{
   QApplication application(argc,argv);
   application.setOrganizationName("GMATTests");
   application.setApplicationName("MarsGramControls");
   const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
   QJsonObject proof{{"outcome","failed"}};
   try
   {
      require(argc==2,"Supply startup file"); TestSettings settings;
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      auto *moderator=Moderator::Instance();
      require(moderator->Initialize(startup.toStdString(),false),"Native atmosphere runtime failed");
      auto *model=moderator->CreateODEModel("ForceModel","EditorControls");
      require(model,"Native force model absent");
      const auto before=model->GetGeneratingString(Gmat::NO_COMMENTS);
      const int beforeForces=model->GetNumForces();
      QTemporaryDir files; require(files.isValid(),"Editor path fixture unavailable");
      const auto path=files.filePath("pending input Ω.txt");
      // Readability/pending-string fixture only: no atmosphere reads this file.
      write(path,"Editor metadata fixture; no density or SDK execution.\n");
      const auto types=moderator->GetListOfFactoryItems(Gmat::ATMOSPHERE,"Mars");
      const bool provider=std::find(types.begin(),types.end(),"MarsGRAM2005")!=types.end();
      bool passed=true; QJsonArray checks;
      auto check=[&](const QString &name,bool value) {
         checks.append(QJsonObject{{"check",name},{"passed",value}}); passed=passed&&value;
      };
      {
         AtmosphereDialog dialog(*model,{{"Drag.BodyName","Mars"},
            {"Drag.AtmosphereModel","MarsGRAM2005"},{"Drag.InputFile",path},
            {"Drag.DensityModel","High"}});
         expose(dialog);
         auto *input=dialog.findChild<QLineEdit*>("atmosphere_InputFile");
         auto *density=dialog.findChild<QComboBox*>("atmosphere_DensityModel");
         check("Mars input draft remains editable",input&&input->isEnabled()&&input->text()==path);
         check("Mars density draft restored",density&&density->isEnabled()&&density->currentText()=="High");
         check("Mars density options",density&&density->count()==3&&density->findText("Mean")>=0&&density->findText("High")>=0&&density->findText("Low")>=0);
         if (input) input->setText(path+" changed");
         if (density) density->setCurrentText("Low");
         check("Pending editor does not apply",dialog.values().isEmpty()&&model->GetGeneratingString(Gmat::NO_COMMENTS)==before);
         dialog.findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel)->click();
         check("Cancel preserves native graph",dialog.result()==QDialog::Rejected&&dialog.values().isEmpty()&&model->GetGeneratingString(Gmat::NO_COMMENTS)==before&&model->GetNumForces()==beforeForces);
      }
      if (!provider)
      {
         AtmosphereDialog dialog(*model,{{"Drag.BodyName","Mars"},
            {"Drag.AtmosphereModel","MarsGRAM2005"},{"Drag.InputFile",path},
            {"Drag.DensityModel","Mean"}});
         expose(dialog);
         dialog.findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
         auto *error=dialog.findChild<QLabel*>("atmosphereError");
         check("Absent real provider still rejects",dialog.isVisible()&&dialog.values().isEmpty()&&error&&error->text().contains("available atmosphere model")&&model->GetGeneratingString(Gmat::NO_COMMENTS)==before);
         dialog.reject();
      }
      {
         AtmosphereDialog dialog(*model,{{"Drag.BodyName","Earth"},
            {"Drag.AtmosphereModel","Exponential"},{"Drag.InputFile",path}});
         expose(dialog);
         auto *input=dialog.findChild<QLineEdit*>("atmosphere_InputFile");
         auto *density=dialog.findChild<QComboBox*>("atmosphere_DensityModel");
         check("Exponential input remains editable",input&&input->isEnabled());
         check("Exponential does not edit Mars density",density&&!density->isEnabled());
         dialog.findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
         check("Exponential accepts only applicable metadata",dialog.result()==QDialog::Accepted&&dialog.values().value("Drag.InputFile")==path&&!dialog.values().contains("Drag.DensityModel")&&model->GetGeneratingString(Gmat::NO_COMMENTS)==before);
      }
      {
         AtmosphereDialog dialog(*model,{{"Drag.BodyName","Earth"},{"Drag.AtmosphereModel","None"}});
         expose(dialog);
         auto *input=dialog.findChild<QLineEdit*>("atmosphere_InputFile");
         auto *density=dialog.findChild<QComboBox*>("atmosphere_DensityModel");
         check("None disables model-specific edits",input&&!input->isEnabled()&&density&&!density->isEnabled());
         dialog.reject();
      }
      check("Final native force graph unchanged",model->GetGeneratingString(Gmat::NO_COMMENTS)==before&&model->GetNumForces()==beforeForces);
      proof=QJsonObject{{"checkpoint","marsgram-editor-controls"},{"outcome",passed?"passed":"failed"},
         {"real_provider_available",provider},{"checks",checks},{"new_requirement_credit",0},
         {"scope","Dedicated editor pending fields, Cancel, actual unavailable-provider rejection and real Exponential metadata acceptance. No fake atmosphere factory or SDK, density calculation, Mars-GRAM positive application or scientific qualification."}};
   }
   catch (BaseException &error) { proof["error"]=QString::fromStdString(error.GetFullMessage()); }
   catch (std::exception &error) { proof["error"]=QString::fromUtf8(error.what()); }
   if (!evidence.isEmpty()) write(evidence+"/proof.json",QJsonDocument(proof).toJson());
   std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
   return proof["outcome"]=="passed"?0:1;
}
