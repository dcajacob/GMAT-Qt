#include "GravityDialog.hpp"
#include "ResourceProperties.hpp"
#include "ODEModel.hpp"
#include "PhysicalModel.hpp"
#include "CelestialBody.hpp"
#include "HarmonicGravity.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <stdexcept>

namespace {
const QStringList fields{"Degree","Order","StmLimit","PotentialFile","TideFile","TideModel"};
QString text(GmatBase &object,const QString &name)
{
   const auto id=object.GetParameterID(name.toStdString());
   return object.GetParameterType(id)==Gmat::INTEGER_TYPE ? QString::number(object.GetIntegerParameter(id)) : QString::fromStdString(object.GetStringParameter(id));
}
QMap<QString,QString> modelAliases(const QString &body)
{
   if (body=="Earth") return {{"JGM-2","JGM2"},{"JGM-3","JGM3"},{"EGM-96","EGM96"}};
   if (body=="Luna") return {{"LP165P","LP165P"}};
   if (body=="Venus") return {{"MGNP180U","MGNP180U"}};
   if (body=="Mars") return {{"Mars50c","MARS50C"}};
   return {};
}
}

GmatBase *GravityDialog::force(const QString &name) const
{
   auto *model=dynamic_cast<ODEModel *>(original.get());
   for (int i=0;model && i<model->GetNumForces();++i) {
      auto *candidate=model->GetForce(i);
      if (candidate->IsOfType("GravityField") && candidate->GetBodyName()==name.toStdString()) return candidate;
   }
   throw std::runtime_error("The primary body's gravity field is no longer available.");
}

GravityDialog::GravityDialog(GmatBase &forceModel,const QMap<QString,QString> &pending,QWidget *parent)
   : QDialog(parent),original(forceModel.Clone())
{
   setObjectName("gravityDialog");setWindowTitle("Gravity and tides");setSizeGripEnabled(true);
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("OK keeps these settings pending. Apply the force model to update the mission.",this);help->setWordWrap(true);layout->addWidget(help);
   auto *selection=new QFormLayout;layout->addLayout(selection);
   body=new QComboBox(this);body->setObjectName("gravity_Body");selection->addRow("Primary body",body);
   auto *model=dynamic_cast<ODEModel *>(original.get());if (!model) throw std::runtime_error("Gravity settings require a force model.");
   for (int i=0;i<model->GetNumForces();++i) if (auto *item=model->GetForce(i);item->IsOfType("GravityField")) {
      const auto name=QString::fromStdString(item->GetBodyName());QMap<QString,QString> values;
      for (const auto &field:fields) values[field]=text(*item,field);
      baseline[name]=values;
      for (const auto &field:fields) if (pending.contains("GravityField."+name+"."+field)) values[field]=pending.value("GravityField."+name+"."+field);
      states[name]=values;sources[name]=!values["TideFile"].isEmpty() ? "Tide File" : values["TideModel"]!="None" ? "Inherited" : "None";body->addItem(name);
   }
   auto *gravity=new QGroupBox("Gravity field",this);auto *gravityForm=new QFormLayout(gravity);layout->addWidget(gravity);
   potentialModel=new QComboBox(gravity);potentialModel->setObjectName("gravity_PotentialModel");gravityForm->addRow("Model",potentialModel);
   auto fileRow=[&](QFormLayout *form,const QString &name,const QString &label) {
      auto *row=new QWidget(this);auto *line=new QHBoxLayout(row);line->setContentsMargins(0,0,0,0);
      auto *entry=new QLineEdit(row);entry->setObjectName("gravity_"+name);edits[name]=entry;line->addWidget(entry);
      auto *browse=new QPushButton("Browse…",row);browse->setObjectName("gravityBrowse_"+name);line->addWidget(browse);form->addRow(label,row);
      connect(browse,&QPushButton::clicked,this,[this,entry,name] {
         QFileDialog picker(this,name=="TideFile" ? "Choose tide file" : "Choose gravity potential",entry->text());picker.setObjectName("gravityFileDialog");picker.setFileMode(QFileDialog::ExistingFile);
         picker.setNameFilter(name=="TideFile" ? "Tide files (*.tide *.TIDE);;All files (*)" : "Gravity files (*.cof *.grv *.COF *.GRV);;All files (*)");
         if (picker.exec()==QDialog::Accepted && !picker.selectedFiles().isEmpty()) entry->setText(picker.selectedFiles().front());
      });
      connect(entry,&QLineEdit::textChanged,this,[this] { if (!updating) {saveBody();refresh(true);} });return browse;
   };
   fileRow(gravityForm,"PotentialFile","Potential file");
   limits=new QLabel(gravity);limits->setObjectName("gravity_FileLimits");gravityForm->addRow("File limits",limits);
   for (const auto &name:QStringList{"Degree","Order","StmLimit"}) {
      auto *entry=new QLineEdit(gravity);entry->setObjectName("gravity_"+name);edits[name]=entry;gravityForm->addRow(name=="StmLimit" ? "STM degree/order limit" : name,entry);
   }
   auto *tides=new QGroupBox("Tides",this);auto *tideForm=new QFormLayout(tides);layout->addWidget(tides);
   source=new QComboBox(tides);source->setObjectName("gravity_TideSource");tideForm->addRow("Data source",source);
   tideModel=new QComboBox(tides);tideModel->setObjectName("gravity_TideModel");tideForm->addRow("Tide model",tideModel);
   tideBrowse=fileRow(tideForm,"TideFile","Tide file");
   error=new QLabel(this);error->setObjectName("gravityError");error->setWordWrap(true);layout->addWidget(error);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this);buttons->setObjectName("gravityButtons");layout->addWidget(buttons);
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);connect(buttons,&QDialogButtonBox::accepted,this,[this]{acceptChanges();});
   connect(body,&QComboBox::currentTextChanged,this,[this](const QString &name){if (!updating) {saveBody();loadBody(name);} });
   connect(source,&QComboBox::currentTextChanged,this,[this](const QString &value) {
      if (updating || activeBody.isEmpty()) return;saveBody();sources[activeBody]=value;
      updating=true;if (value!="Tide File") {states[activeBody]["TideFile"].clear();edits["TideFile"]->clear();}
      if (value=="None") states[activeBody]["TideModel"]="None";updating=false;refresh(true);
   });
   connect(tideModel,&QComboBox::currentTextChanged,this,[this](const QString &value){if (!updating && !activeBody.isEmpty()) states[activeBody]["TideModel"]=value;});
   connect(potentialModel,&QComboBox::currentTextChanged,this,[this](const QString &value){if (!updating && presets.contains(value)) edits["PotentialFile"]->setText(presets[value]);});
   if (body->count()) loadBody(body->currentText());
   else {gravity->setEnabled(false);tides->setEnabled(false);limits->setText("No primary body");error->setText("Select a primary body in the force model to configure gravity and tides.");}
   resize(760,620);
}

void GravityDialog::saveBody()
{
   if (activeBody.isEmpty()) return;
   for (auto it=edits.cbegin();it!=edits.cend();++it) states[activeBody][it.key()]=it.value()->text();
   if (!tideModel->currentText().isEmpty()) states[activeBody]["TideModel"]=tideModel->currentText();
}

void GravityDialog::loadBody(const QString &name)
{
   activeBody=name;updating=true;presets.clear();potentialModel->clear();
   const auto aliases=modelAliases(name);
   for (auto it=aliases.cbegin();it!=aliases.cend();++it) {
      const auto filename=QString::fromStdString(Moderator::Instance()->GetPotentialFileName(it.value().toStdString()));
      if (!filename.isEmpty()) {presets[it.key()]=filename;potentialModel->addItem(it.key());}
   }
   potentialModel->addItem("Other");
   for (auto it=edits.cbegin();it!=edits.cend();++it) it.value()->setText(states[name].value(it.key()));
   source->clear();source->addItems({"None","Inherited","Tide File"});source->setCurrentText(sources.value(name));
   tideModel->clear();tideModel->addItem(states[name].value("TideModel","None"));updating=false;refresh();
}

void GravityDialog::refresh(bool adjustModel)
{
   if (activeBody.isEmpty()) return;saveBody();updating=true;error->clear();limits->setText("Unknown");
   const auto dataSource=sources.value(activeBody);const bool external=dataSource=="Tide File";
   edits["TideFile"]->setEnabled(external);tideBrowse->setEnabled(external);tideModel->setEnabled(false);
   try {
      auto *celestial=dynamic_cast<CelestialBody *>(Moderator::Instance()->GetConfiguredObject(activeBody.toStdString()));if (!celestial) throw std::runtime_error("The primary body is unavailable.");
      QtResourcePreview copy(force(activeBody)->Clone());
      copy->SetStringParameter("PotentialFile",states[activeBody]["PotentialFile"].toStdString());
      const auto filename=copy->GetStringParameter("PotentialFileFullPath");
      const auto tideFile=external ? states[activeBody]["TideFile"] : QString();
      copy->SetStringParameter("TideFile",tideFile.toStdString());
      HarmonicGravity metadata(filename,copy->GetStringParameter("TideFileFullPath"),celestial->GetEquatorialRadius(),celestial->GetGravitationalConstant(),celestial->GetName(),false);
      HarmonicGravity inherited(filename,"",celestial->GetEquatorialRadius(),celestial->GetGravitationalConstant(),celestial->GetName(),false);
      limits->setText(QString("Degree %1 / Order %2").arg(metadata.GetNN()).arg(metadata.GetMM()));
      QString preset="Other";for (auto it=presets.cbegin();it!=presets.cend();++it) if (QFileInfo(it.value()).canonicalFilePath()==QFileInfo(QString::fromStdString(filename)).canonicalFilePath()) preset=it.key();potentialModel->setCurrentText(preset);
      const bool canInherit=activeBody=="Earth" || QFileInfo(QString::fromStdString(filename)).suffix().compare("grv",Qt::CaseInsensitive)==0 || inherited.HaveTideModel(HarmonicGravity::Solid);
      source->clear();source->addItem("None");if (canInherit) source->addItem("Inherited");source->addItem("Tide File");
      if (!canInherit && sources[activeBody]=="Inherited" && adjustModel) {sources[activeBody]="None";states[activeBody]["TideModel"]="None";}
      if (source->findText(sources[activeBody])<0) source->addItem(sources[activeBody]);source->setCurrentText(sources[activeBody]);
      QStringList models;for (int i=0;i<HarmonicGravity::ETideCount;++i) if (metadata.HaveTideModel(i)) models.append(QString::fromStdString(HarmonicGravity::ETideString[i]));
      auto selected=states[activeBody].value("TideModel","None");if (sources[activeBody]=="None") selected="None";
      if (!models.contains(selected) && adjustModel) selected="None";states[activeBody]["TideModel"]=selected;
      tideModel->clear();tideModel->addItems(models);if (tideModel->findText(selected)<0) tideModel->addItem(selected);tideModel->setCurrentText(selected);
      tideModel->setEnabled(sources[activeBody]!="None" && (!external || !tideFile.isEmpty()));
      if (external && tideFile.isEmpty()) error->setText("Choose a tide file to enable its supported models.");
   } catch (BaseException &failure) {error->setText(QString::fromStdString(failure.GetFullMessage()));}
     catch (const std::exception &failure) {error->setText(QString::fromUtf8(failure.what()));}
   updating=false;
}

void GravityDialog::acceptChanges()
{
   saveBody();
   try {
      QtResourcePreview candidate(original->Clone());QMap<QString,QString> result;
      auto *model=dynamic_cast<ODEModel *>(candidate.get());
      for (auto it=states.cbegin();it!=states.cend();++it) {
         const auto &name=it.key();auto values=it.value();
         if (sources.value(name)=="None") {values["TideFile"].clear();values["TideModel"]="None";}
         if (sources.value(name)=="Inherited") values["TideFile"].clear();
         if (sources.value(name)=="Tide File" && values["TideFile"].trimmed().isEmpty()) throw std::runtime_error("Choose a tide file or change the data source.");
         GmatBase *owned=nullptr;for (int i=0;model && i<model->GetNumForces();++i) if (model->GetForce(i)->IsOfType("GravityField") && model->GetForce(i)->GetBodyName()==name.toStdString()) owned=model->GetForce(i);
         if (!owned) throw std::runtime_error("The selected gravity field is unavailable.");
         for (const auto &field:QStringList{"PotentialFile","TideFile","Degree","Order","StmLimit","TideModel"}) {
            setResourceProperty(*owned,field,values.value(field));
            if (values.value(field)!=baseline[name].value(field)) result["GravityField."+name+"."+field]=values.value(field);
         }
      }
      validateResourceProperties(*candidate);acceptedValues=result;accept();
   } catch (BaseException &failure) {error->setText(QString::fromStdString(failure.GetFullMessage()));}
     catch (const std::exception &failure) {error->setText(QString::fromUtf8(failure.what()));}
}
