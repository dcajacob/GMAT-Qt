#include "GroundTrackDialog.hpp"
#include "ResourceProperties.hpp"
#include "GmatBase.hpp"
#include "Moderator.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QImageReader>
#include <QScrollArea>
#include <QGuiApplication>
#include <QScreen>
#include <memory>
#include <limits>
#include <stdexcept>

void validateGroundTrackTexture(GmatBase &plot)
{
   const auto id=plot.GetParameterID("TextureMap");
   const auto value=plot.GetTypeName()=="GroundTrack" && plot.IsParameterReadOnly(id) ? std::string() : plot.GetStringParameter(id);
   if (!plot.IsParameterValid("TextureMap",value)) throw std::runtime_error("Choose an available texture map, or leave it blank for the body's default.");
   std::string name,path;
   if (!FileManager::Instance()->GetTextureMapFile(value,plot.GetStringParameter("CentralBody"),plot.GetName(),name,path,false) || QImageReader(QString::fromStdString(path)).read().isNull())
      throw std::runtime_error("The ground-track texture map cannot be read as an image.");
}
GroundTrackDialog::GroundTrackDialog(GmatBase &plot,const QMap<QString,QString> &pending,QWidget *parent) : QDialog(parent)
{
   setObjectName("groundTrackDialog"); setWindowTitle("Ground track — "+QString::fromStdString(plot.GetName()));
   resize(620,std::min(680,QGuiApplication::primaryScreen()->availableGeometry().height()-80));
   QMap<QString,QString> initial; QStringList modes;
   for (const auto &field:resourceProperties(plot)) { initial.insert(field.name,field.value); if (field.name=="SolverIterations") modes=field.choices; }
   for (auto it=pending.cbegin();it!=pending.cend();++it) initial.insert(it.key(),it.value());
   auto snapshot=std::shared_ptr<GmatBase>(plot.Clone());
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Choose the central body, spacecraft/stations, data settings and texture. OK keeps edits pending; Apply updates the mission.",this); help->setWordWrap(true); layout->addWidget(help);
   auto *scroll=new QScrollArea(this); scroll->setObjectName("groundTrackScroll"); scroll->setWidgetResizable(true); layout->addWidget(scroll,1);
   auto *content=new QWidget(scroll); scroll->setWidget(content); auto *groups=new QVBoxLayout(content);
   auto *drawing=new QGroupBox("Drawing",content); auto *drawForm=new QFormLayout(drawing); groups->addWidget(drawing);
   body=new QComboBox(drawing); body->setObjectName("groundCentralBody");
   for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::CELESTIAL_BODY)) body->addItem(QString::fromStdString(name));
   body->setCurrentText(initial.value("CentralBody")); drawForm->addRow("Central body",body);
   objects=new QListWidget(drawing); objects->setObjectName("groundObjects"); objects->setDragDropMode(QAbstractItemView::InternalMove);
   auto names=splitResourceReferences(initial.value("Add")); const auto selected=names;
   for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::SPACE_POINT)) {
      auto *object=Moderator::Instance()->GetConfiguredObject(name); const auto text=QString::fromStdString(name);
      if (object && (object->IsOfType("Spacecraft") || object->IsOfType("GroundStation")) && !names.contains(text)) names.append(text);
   }
   for (const auto &name:names) { auto *item=new QListWidgetItem(name,objects); item->setFlags(item->flags()|Qt::ItemIsUserCheckable); item->setCheckState(selected.contains(name) ? Qt::Checked : Qt::Unchecked); }
   drawForm->addRow("Selected objects",objects);
   auto *actions=new QWidget(drawing); auto *row=new QHBoxLayout(actions); row->setContentsMargins(0,0,0,0);
   for (const auto &label:QStringList{"Select all","Clear selection"}) {
      auto *button=new QPushButton(label,actions); button->setObjectName(label=="Select all" ? "groundSelectAll" : "groundClearSelection"); row->addWidget(button);
      connect(button,&QPushButton::clicked,this,[this,label] { for (int i=0;i<objects->count();++i) objects->item(i)->setCheckState(label=="Select all" ? Qt::Checked : Qt::Unchecked); });
   }
   row->addStretch(); drawForm->addRow(actions);
   auto *sampling=new QGroupBox("Data",content); auto *dataForm=new QFormLayout(sampling); groups->addWidget(sampling);
   const QMap<QString,QString> labels={{"IntegrationStepsToSkip","Integration steps to skip"},{"DataCollectFrequency","Collect every N samples"},{"UpdatePlotFrequency","Update every N samples"},{"MaxPlotPoints","Maximum retained points"},{"NumPointsToRedraw","Recent segments to redraw"}};
   for (const auto &name:QStringList{"IntegrationStepsToSkip","DataCollectFrequency","UpdatePlotFrequency","MaxPlotPoints","NumPointsToRedraw"}) {
      auto *spin=new QSpinBox(sampling); spin->setObjectName("ground_"+name); spin->setRange(name=="IntegrationStepsToSkip" ? -1 : name=="NumPointsToRedraw" ? 0 : 1,std::numeric_limits<int>::max());
      if (name=="NumPointsToRedraw") { spin->setSpecialValueText("All retained points"); spin->setToolTip("Limits recent trajectory segments during runs and replay. The complete retained trajectory appears at run completion."); }
      if (name=="IntegrationStepsToSkip") { spin->setSpecialValueText("Use publication samples"); spin->setToolTip("0 collects every accepted integration step. Positive values skip that many steps between points. Overrides publication collection frequency."); }
      spin->setValue(initial.value(name, name=="IntegrationStepsToSkip" ? "-1" : "0").toInt()); data.insert(name,spin); dataForm->addRow(labels.value(name),spin);
   }
   auto *options=new QGroupBox("Other options",content); auto *optionForm=new QFormLayout(options); groups->addWidget(options);
   show=new QCheckBox("Show plot",options); show->setObjectName("groundShowPlot"); show->setChecked(initial.value("ShowPlot")=="true"); optionForm->addRow(show);
   solver=new QComboBox(options); solver->setObjectName("groundSolverIterations"); solver->addItems(modes); solver->setCurrentText(initial.value("SolverIterations")); optionForm->addRow("Solver iterations",solver);
   auto *textureRow=new QWidget(options); auto *textureLayout=new QHBoxLayout(textureRow); textureLayout->setContentsMargins(0,0,0,0);
   texture=new QLineEdit(initial.value("TextureMap"),textureRow); texture->setObjectName("groundTextureMap"); texture->setPlaceholderText("Default body texture"); textureLayout->addWidget(texture);
   auto *browse=new QPushButton("Browse…",textureRow); browse->setObjectName("groundBrowseTexture"); textureLayout->addWidget(browse);
   auto *clear=new QPushButton("Default",textureRow); clear->setObjectName("groundDefaultTexture"); textureLayout->addWidget(clear); connect(clear,&QPushButton::clicked,texture,&QLineEdit::clear);
   optionForm->addRow("Texture map",textureRow);
   connect(browse,&QPushButton::clicked,this,[this] { QFileDialog picker(this,"Choose texture map",texture->text()); picker.setObjectName("groundTextureDialog"); picker.setFileMode(QFileDialog::ExistingFile); if (picker.exec()==QDialog::Accepted && !picker.selectedFiles().isEmpty()) texture->setText(picker.selectedFiles().first()); });
   // Retain per-body custom choices while previewing other bodies.
   connect(body,&QComboBox::currentTextChanged,this,[this,previous=body->currentText(),maps=QMap<QString,QString>()](const QString &name) mutable {
      maps.insert(previous,texture->text()); previous=name;
      if (!maps.contains(name)) if (auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString())) maps.insert(name,QString::fromStdString(object->GetStringParameter("TextureMapFileName")));
      texture->setText(maps.value(name));
   });
   groups->addStretch(); error=new QLabel(this); error->setObjectName("groundTrackError"); error->setWordWrap(true); layout->addWidget(error);
   connect(texture,&QLineEdit::textChanged,error,&QLabel::clear); connect(body,&QComboBox::currentTextChanged,error,&QLabel::clear);
   connect(objects,&QListWidget::itemChanged,error,[this] { error->clear(); });
   connect(show,&QCheckBox::toggled,error,&QLabel::clear);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons); connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[this,snapshot] {
      try {
         const auto values=settings(); std::unique_ptr<GmatBase> candidate(snapshot->Clone()); setResourceProperty(*candidate,"CentralBody",values.value("CentralBody"));
         for (auto it=values.cbegin();it!=values.cend();++it) if (it.key()!="CentralBody" && it.key()!="Add") setResourceProperty(*candidate,it.key(),it.value());
         if (show->isChecked() && values.value("Add").isEmpty()) throw std::runtime_error("Select an object, or turn off Show plot.");
         for (const auto &name:splitResourceReferences(values.value("Add"))) { auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString()); if (!object || !object->IsOfType(Gmat::SPACE_POINT)) throw std::runtime_error("Select an available space point."); }
         validateGroundTrackTexture(*candidate); accept();
      } catch (BaseException &exception) { error->setText(QString::fromStdString(exception.GetFullMessage())); }
      catch (const std::exception &exception) { error->setText(QString::fromUtf8(exception.what())); }
   });
}
QMap<QString,QString> GroundTrackDialog::settings() const
{
   QStringList selected; for (int i=0;i<objects->count();++i) if (objects->item(i)->checkState()==Qt::Checked) selected.append(objects->item(i)->text());
   QMap<QString,QString> values={{"CentralBody",body->currentText()},{"Add",selected.join(", ")},{"ShowPlot",show->isChecked() ? "true" : "false"},{"SolverIterations",solver->currentText()},{"TextureMap",texture->text().trimmed()}};
   for (auto it=data.cbegin();it!=data.cend();++it) values.insert(it.key(),QString::number(it.value()->value())); return values;
}
