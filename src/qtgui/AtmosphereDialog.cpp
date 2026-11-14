#include "AtmosphereDialog.hpp"
#include "ResourceProperties.hpp"
#include "ODEModel.hpp"
#include "DragForce.hpp"
#include "AtmosphereModel.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "FileManager.hpp"
#include <QFile>
#include <QFileInfo>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QScrollArea>
#include <QScreen>
#include <QVBoxLayout>
#include <algorithm>
#include <stdexcept>

namespace {
DragForce *dragForce(ODEModel &model)
{
   for (int i=0;i<model.GetNumForces();++i) if (auto *drag=dynamic_cast<DragForce *>(model.GetForce(i))) return drag;
   return nullptr;
}
QStringList atmosphereModels(const QString &body)
{
   QStringList result={"None"};
   for (const auto &type:Moderator::Instance()->GetListOfFactoryItems(Gmat::ATMOSPHERE,body.toStdString())) result.append(QString::fromStdString(type));
   // The legacy Earth qualifier still hides Exponential for R2013a. It is
   // registered and supported by the current engine and shipped Earth example.
   if (body=="Earth") for (const auto &type:Moderator::Instance()->GetListOfFactoryItems(Gmat::ATMOSPHERE))
      if (type=="Exponential" && !result.contains("Exponential")) result.append("Exponential");
   return result;
}
void validateWeather(DragForce &drag)
{
   const auto type=drag.GetStringParameter("AtmosphereModel");
   const bool marsGram=type=="MarsGRAM2005";
   if (type=="Exponential" || marsGram) {
      const auto path=QString::fromStdString(drag.GetStringParameter("InputFile"));
      if (!path.isEmpty() && (!QFileInfo(path).isFile() || !QFileInfo(path).isReadable()))
         throw std::runtime_error("Choose a readable atmosphere input file.");
      if (marsGram && path.isEmpty())
         throw std::runtime_error("Choose a readable Mars-GRAM input namelist file.");
      if (marsGram && !QStringList{"Mean","High","Low"}.contains(QString::fromStdString(drag.GetStringParameter("DensityModel"))))
         throw std::runtime_error("Select a Mars-GRAM density model: Mean, High or Low.");
      if (path.isEmpty() && drag.GetBodyName()!="Earth" && drag.GetBodyName()!="Mars")
         throw std::runtime_error("Choose an atmosphere input file for this body.");
      return;
   }
   if (drag.GetBodyName()!="Earth") return;
   auto validate=[&](const std::string &property,bool historic) {
      auto path=QString::fromStdString(drag.GetStringParameter(property));
      if (!QFileInfo::exists(path)) path=QString::fromStdString(FileManager::Instance()->GetAbsPathname("ATMOSPHERE_PATH"))+path;
      if (!QFileInfo(path).isFile() || !QFileInfo(path).isReadable())
         throw std::runtime_error("Choose a readable "+property+" input file.");
      const auto resolved=DragForce::CheckFluxFile(drag.GetStringParameter(property),historic);
      if (resolved.empty()) throw std::runtime_error("The selected file does not contain valid space-weather data.");
      // The core checker accepts either prediction format. The selected source
      // still needs its matching format, otherwise parsing fails at execution.
      QFile file(QString::fromStdString(resolved));
      if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("The weather file could not be opened.");
      while (!file.atEnd()) {
         const auto line=file.readLine().toUpper();
         if (!line.contains("DATATYPE CSSISPACEWEATHER") && !line.contains("BEGIN_DATA")) continue;
         const bool cssi=line.contains("DATATYPE CSSISPACEWEATHER");
         if (cssi!=(property=="CSSISpaceWeatherFile")) throw std::runtime_error("Choose a file matching the selected CSSI or Schatten weather source.");
         return;
      }
   };
   // DragForce::Initialize checks both filenames and AtmosphereModel's file
   // setters validate both formats, even when the selected sources are constant.
   validate("CSSISpaceWeatherFile",true);
   validate("SchattenFile",false);
   const auto predicted=drag.GetStringParameter("PredictedWeatherSource");
   if (predicted=="CSSISpaceWeatherFile") validate(predicted,false);
}
}

QSet<QString> applyAtmosphereProperties(GmatBase &object,const QMap<QString,QString> &values)
{
   QSet<QString> consumed;
   auto *forceModel=dynamic_cast<ODEModel *>(&object); if (!forceModel) return consumed;
   for (auto it=values.cbegin();it!=values.cend();++it) if (it.key()=="Drag" || it.key().startsWith("Drag.")) consumed.insert(it.key());
   if (consumed.isEmpty()) return consumed;
   auto *current=dragForce(*forceModel);
   const auto selected=values.value("Drag.AtmosphereModel",values.value("Drag",current ? QString::fromStdString(current->GetStringParameter("AtmosphereModel")) : "None"));
   if (selected=="None") { if (current) forceModel->DeleteForce(current); return consumed; }
   const auto body=values.value("Drag.BodyName",QString::fromStdString(current ? current->GetBodyName() : forceModel->GetStringParameter("CentralBody")));
   if (!atmosphereModels(body).contains(selected)) throw std::runtime_error("Select an available atmosphere model for this body.");
   const auto applyFields=[&](DragForce &target) {
      for (auto it=values.cbegin();it!=values.cend();++it) if (it.key().startsWith("Drag.") && it.key()!="Drag.AtmosphereModel" && it.key()!="Drag.BodyName")
         setResourceProperty(target,it.key().mid(5),it.value());
      validateWeather(target);
   };
   if (!current || QString::fromStdString(current->GetStringParameter("AtmosphereModel"))!=selected || QString::fromStdString(current->GetBodyName())!=body) {
      std::unique_ptr<DragForce> replacement(current ? static_cast<DragForce *>(current->Clone()) : new DragForce());
      replacement->SetStringParameter("BodyName",body.toStdString());
      replacement->SetStringParameter("AtmosphereModel",selected.toStdString());
      applyFields(*replacement);
      std::unique_ptr<AtmosphereModel> atmosphere(Moderator::Instance()->CreateAtmosphereModel(selected.toStdString(),""));
      if (!atmosphere) throw std::runtime_error("The atmosphere model could not be created.");
      atmosphere->SetSolarSystem(Moderator::Instance()->GetSolarSystemInUse());
      replacement->SetInternalAtmosphereModel(atmosphere.release());
      if (current) forceModel->DeleteForce(current);
      forceModel->AddForce(replacement.get()); current=replacement.release();
   } else applyFields(*current);
   return consumed;
}

AtmosphereDialog::AtmosphereDialog(GmatBase &forceModel,const QMap<QString,QString> &pending,QWidget *parent) : QDialog(parent),original(forceModel.Clone())
{
   setObjectName("atmosphereDialog"); setWindowTitle("Atmosphere and drag");
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("OK keeps changes pending. Apply the force model to update the mission.",this); help->setWordWrap(true); layout->addWidget(help);
   auto *scroll=new QScrollArea(this); scroll->setObjectName("atmosphereScroll"); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
   auto *content=new QWidget(scroll); auto *groups=new QVBoxLayout(content); groups->setContentsMargins(0,0,0,0); scroll->setWidget(content); layout->addWidget(scroll,1);
   auto *selection=new QGroupBox("Model selection",content); auto *selectionForm=new QFormLayout(selection); groups->addWidget(selection);
   auto *fm=dynamic_cast<ODEModel *>(original.get()); if (!fm) throw std::runtime_error("Atmosphere settings require a force model.");
   auto *current=dragForce(*fm); DragForce defaults; if (!current) current=&defaults;
   body=new QComboBox(selection); body->setObjectName("atmosphere_BodyName");
   for (int i=0;i<fm->GetNumForces();++i) if (fm->GetForce(i)->IsOfType("GravityField")) body->addItem(QString::fromStdString(fm->GetForce(i)->GetBodyName()));
   const auto initialBody=pending.value("Drag.BodyName",QString::fromStdString(current==&defaults ? fm->GetStringParameter("CentralBody") : current->GetBodyName()));
   if (body->findText(initialBody)<0) body->addItem(initialBody); body->setCurrentText(initialBody); selectionForm->addRow("Primary body",body);
   model=new QComboBox(selection); model->setObjectName("atmosphere_AtmosphereModel"); model->addItems(atmosphereModels(initialBody));
   const auto initialModel=pending.value("Drag.AtmosphereModel",pending.value("Drag",QString::fromStdString(fm->GetStringParameter("Drag"))));
   if (model->findText(initialModel)<0) model->addItem(initialModel); model->setCurrentText(initialModel); selectionForm->addRow("Atmosphere model",model);
   auto choice=[&](QFormLayout *form,const QString &name,const QString &label,const QStringList &options) {
      auto *combo=new QComboBox(this); combo->setObjectName("atmosphere_"+name); combo->addItems(options);
      const auto initial=pending.value("Drag."+name,QString::fromStdString(current->GetStringParameter(name.toStdString())));
      if (combo->findText(initial)<0) combo->addItem(initial); combo->setCurrentText(initial); controls.insert(name,combo); form->addRow(label,combo);
   };
   choice(selectionForm,"DragModel","Spacecraft shape",{"Spherical","SPADFile"});
   auto *sources=new QGroupBox("Space-weather sources",content); auto *sourceForm=new QFormLayout(sources); groups->addWidget(sources);
   choice(sourceForm,"HistoricWeatherSource","Historic / near term",{"ConstantFluxAndGeoMag","CSSISpaceWeatherFile"});
   choice(sourceForm,"PredictedWeatherSource","Long-term prediction",{"ConstantFluxAndGeoMag","CSSISpaceWeatherFile","SchattenFile"});
   auto *configuration=new QGroupBox("Model configuration",content); auto *configurationForm=new QFormLayout(configuration); groups->addWidget(configuration);
   auto number=[&](const QString &name,const QString &label,const QString &unit) {
      auto *row=new QWidget(configuration); auto *line=new QHBoxLayout(row); line->setContentsMargins(0,0,0,0);
      auto *edit=new QLineEdit(pending.value("Drag."+name,QString::number(current->GetRealParameter(name.toStdString()),'g',17)),row);
      edit->setObjectName("atmosphere_"+name); controls.insert(name,edit); line->addWidget(edit); line->addWidget(new QLabel(unit,row)); configurationForm->addRow(label,row);
   };
   number("F107","Solar flux (F10.7)","sfu"); number("F107A","Average solar flux (F10.7A)","sfu"); number("MagneticIndex","Geomagnetic index (Kp)","0–9");
   choice(configurationForm,"SchattenErrorModel","Schatten error",{"Nominal","PlusTwoSigma","MinusTwoSigma"});
   choice(configurationForm,"SchattenTimingModel","Schatten timing",{"NominalCycle","EarlyCycle","LateCycle"});
   choice(configurationForm,"DensityModel","Mars-GRAM density",{"Mean","High","Low"});
   auto *files=new QGroupBox("Input files",content); auto *fileForm=new QFormLayout(files); groups->addWidget(files);
   auto *fileHelp=new QLabel("For Mars-GRAM, choose its input namelist file. Earth weather models require both weather files, including with constant sources.",files); fileHelp->setWordWrap(true); fileForm->addRow(fileHelp);
   for (const auto &name:QStringList{"CSSISpaceWeatherFile","SchattenFile","InputFile"}) {
      auto *row=new QWidget(files); auto *line=new QHBoxLayout(row); line->setContentsMargins(0,0,0,0);
      auto *edit=new QLineEdit(pending.value("Drag."+name,QString::fromStdString(current->GetStringParameter(name.toStdString()))),row);
      edit->setObjectName("atmosphere_"+name); controls.insert(name,edit); line->addWidget(edit);
      auto *browse=new QPushButton("Browse…",row); browse->setObjectName("atmosphereBrowse_"+name); controls.insert("Browse_"+name,browse); line->addWidget(browse);
      fileForm->addRow(name=="CSSISpaceWeatherFile" ? "CSSI space weather" : name=="SchattenFile" ? "Schatten prediction" : "Atmosphere input",row);
      connect(browse,&QPushButton::clicked,this,[this,edit,name] {
         QFileDialog picker(this,"Choose "+name,edit->text()); picker.setObjectName("atmosphereFileDialog"); picker.setFileMode(QFileDialog::ExistingFile);
         if (picker.exec()==QDialog::Accepted && !picker.selectedFiles().isEmpty()) edit->setText(picker.selectedFiles().first());
      });
   }
   groups->addStretch();
   error=new QLabel(this); error->setObjectName("atmosphereError"); error->setWordWrap(true); layout->addWidget(error);
   for (auto *control:controls) {
      if (auto *combo=qobject_cast<QComboBox *>(control)) connect(combo,&QComboBox::currentTextChanged,error,&QLabel::clear);
      if (auto *edit=qobject_cast<QLineEdit *>(control)) connect(edit,&QLineEdit::textChanged,error,&QLabel::clear);
   }
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[this] {
      try {
         QMap<QString,QString> values={{"Drag.AtmosphereModel",model->currentText()},{"Drag.BodyName",body->currentText()}};
         const bool weather=model->currentText()!="None" && model->currentText()!="Exponential";
         const bool marsGram=model->currentText()=="MarsGRAM2005";
         for (auto it=controls.cbegin();it!=controls.cend();++it) {
            if (it.key().startsWith("Browse_")) continue;
            if (it.key()=="InputFile" && model->currentText()!="Exponential" && !marsGram) continue;
            if (it.key()=="DensityModel" && !marsGram) continue;
            if (it.key()!="InputFile" && it.key()!="DensityModel" && it.key()!="DragModel" && !weather) continue;
            if (body->currentText()!="Earth" && it.key()!="InputFile" && it.key()!="DensityModel" && it.key()!="DragModel" && it.key()!="F107" && it.key()!="F107A" && it.key()!="MagneticIndex") continue;
            if (auto *combo=qobject_cast<QComboBox *>(it.value())) values.insert("Drag."+it.key(),combo->currentText());
            else if (auto *edit=qobject_cast<QLineEdit *>(it.value())) values.insert("Drag."+it.key(),edit->text());
         }
         std::unique_ptr<GmatBase> candidate(original->Clone()); applyAtmosphereProperties(*candidate,values);
         acceptedValues=values; accept();
      } catch (BaseException &failure) { error->setText(QString::fromStdString(failure.GetFullMessage())); }
      catch (const std::exception &failure) { error->setText(QString::fromUtf8(failure.what())); }
   });
   connect(body,&QComboBox::currentTextChanged,this,[this] {
      const auto selected=model->currentText(); const QSignalBlocker block(model); model->clear(); model->addItems(atmosphereModels(body->currentText()));
      model->setCurrentText(model->findText(selected)>=0 ? selected : "None"); update();
   });
   connect(model,&QComboBox::currentTextChanged,this,[this] { update(); });
   for (const auto &name:QStringList{"HistoricWeatherSource","PredictedWeatherSource"}) connect(qobject_cast<QComboBox *>(controls.value(name)),&QComboBox::currentTextChanged,this,[this] { update(); });
   update();
   const auto available=screen()->availableGeometry();
   resize(std::min(760,available.width()-40),std::min(780,available.height()-80));
}

AtmosphereDialog::~AtmosphereDialog()=default;

void AtmosphereDialog::update()
{
   error->clear();
   const bool active=model->currentText()!="None",weather=active && model->currentText()!="Exponential",earth=body->currentText()=="Earth";
   for (auto it=controls.cbegin();it!=controls.cend();++it) it.value()->setEnabled(weather && earth);
   controls["DragModel"]->setEnabled(active);
   for (const auto &name:QStringList{"F107","F107A","MagneticIndex"}) controls[name]->setEnabled(weather);
   const auto predicted=qobject_cast<QComboBox *>(controls["PredictedWeatherSource"])->currentText();
   for (const auto &name:QStringList{"CSSISpaceWeatherFile","Browse_CSSISpaceWeatherFile","SchattenFile","Browse_SchattenFile"}) controls[name]->setEnabled(weather && earth);
   for (const auto &name:QStringList{"SchattenErrorModel","SchattenTimingModel"}) controls[name]->setEnabled(weather && earth && predicted=="SchattenFile");
   const bool marsGram=model->currentText()=="MarsGRAM2005";
   controls["DensityModel"]->setEnabled(marsGram);
   for (const auto &name:QStringList{"InputFile","Browse_InputFile"}) controls[name]->setEnabled(active && (model->currentText()=="Exponential" || marsGram));
}
