#include "AttitudeDialog.hpp"
#include "ResourceProperties.hpp"
#include "GmatBase.hpp"
#include "Attitude.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QFileDialog>
#include <QSignalBlocker>
#include <QGuiApplication>
#include <QScreen>
#include <QRegularExpression>
#include <stdexcept>

namespace {
QString controlValue(QWidget *widget)
{
   if (auto *combo=qobject_cast<QComboBox *>(widget)) return combo->currentText();
   return qobject_cast<QLineEdit *>(widget)->text().trimmed();
}
QString label(const QString &name)
{
   const QMap<QString,QString> labels={{"AttitudeDisplayStateType","Orientation representation"},
      {"AttitudeRateDisplayStateType","Rate representation"},{"AttitudeCoordinateSystem","Reference coordinate system"},
      {"EulerAngleSequence","Euler sequence"},{"AttitudeReferenceBody","Reference body"},
      {"AttitudeConstraintType","Constraint"},{"AttitudeFileName","Attitude ephemeris file"},
      {"OffsetQuaternion","Frame-to-body offset (x y z w)"}};
   if (labels.contains(name)) return labels.value(name);
   QString result=name;
   result.replace(QRegularExpression("([a-z])([A-Z])"),"\\1 \\2");
   return result;
}
QVector<ResourceProperty> attitudeFields(GmatBase &spacecraft)
{
   auto *attitude=dynamic_cast<Attitude *>(spacecraft.GetOwnedObject(0));
   if (!attitude) throw std::runtime_error("This spacecraft has no attitude model.");
   auto fields=resourceProperties(*attitude);
   if (attitude->SetInitialAttitudeAllowed()) {
      const QMap<QString,QString> state={{"Quaternion","Q1"},{"EulerAngles","EulerAngle1"},{"MRPs","MRP1"},{"DirectionCosineMatrix","DCM11"}};
      const auto required=QStringList{state.value(QString::fromStdString(attitude->GetStringParameter("AttitudeDisplayStateType"))),
         attitude->GetStringParameter("AttitudeRateDisplayStateType")=="AngularVelocity" ? "AngularVelocityX" : "EulerAngleRate1"};
      for (const auto &name:required) {
         bool found=false; for (const auto &field:fields) found=found || field.name==name;
         if (!found) throw std::runtime_error("Attitude values could not be read. Check the model and its reference settings.");
      }
   }
   return fields;
}
}

AttitudeDialog::AttitudeDialog(GmatBase &spacecraft,const QMap<QString,QString> &pending,QWidget *parent)
   : QDialog(parent),candidate(spacecraft.Clone())
{
   setObjectName("attitudeDialog"); setWindowTitle("Spacecraft attitude");
   layout=new QVBoxLayout(this);
   auto *help=new QLabel("Choose an attitude model and its reference, orientation and rate settings. OK keeps changes pending until spacecraft Apply.",this);
   help->setWordWrap(true); layout->addWidget(help);
   auto *configuration=new QFormLayout; layout->addLayout(configuration);
   model=new QComboBox(this); model->setObjectName("attitudeModel");
   for (const auto &name:Moderator::Instance()->GetListOfFactoryItems(Gmat::ATTITUDE)) model->addItem(QString::fromStdString(name));
   if (!candidate) throw std::runtime_error("This spacecraft cannot be edited.");
   applyAttitudeProperties(*candidate,pending);
   const auto current=QString::fromStdString(candidate->GetStringParameter("Attitude"));
   if (model->findText(current)<0) model->addItem(current);
   model->setCurrentText(current); configuration->addRow("Attitude model",model);
   error=new QLabel(this); error->setObjectName("attitudeError"); error->setWordWrap(true);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this);
   layout->addWidget(error); layout->addWidget(buttons);
   rebuild();
   connect(model,&QComboBox::currentTextChanged,this,[this](const QString &name) { submit("Attitude",name); });
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[this] {
      if (!submit()) return;
      acceptedValues.clear(); acceptedValues.insert("Attitude",QString::fromStdString(candidate->GetStringParameter("Attitude")));
      for (const auto &field:attitudeFields(*candidate)) acceptedValues.insert(field.name,field.value);
      accept();
   });
   auto size=QSize(640,720);
   if (auto *screen=QGuiApplication::primaryScreen()) size=size.boundedTo(screen->availableGeometry().size()-QSize(40,80));
   resize(size);
}
AttitudeDialog::~AttitudeDialog()=default;

void AttitudeDialog::rebuild()
{
   if (fields) { layout->removeWidget(fields); fields->hide(); fields->setParent(nullptr); fields->deleteLater(); }
   controls.clear(); loaded.clear();
   auto *scroll=new QScrollArea(this); scroll->setWidgetResizable(true); fields=scroll;
   auto *content=new QWidget(scroll); auto *groups=new QVBoxLayout(content); scroll->setWidget(content);
   QMap<QString,QFormLayout *> forms;
   for (const auto &name:QStringList{"Configuration","Initial attitude","Initial rate","Model settings"}) {
      auto *group=new QGroupBox(name,content); auto *form=new QFormLayout(group); forms.insert(name,form); groups->addWidget(group);
   }
   auto *attitude=dynamic_cast<Attitude *>(candidate->GetOwnedObject(0));
   for (auto field:attitudeFields(*candidate)) {
      if (!attitude->SetInitialAttitudeAllowed() && (field.name=="AttitudeDisplayStateType" || field.name=="AttitudeRateDisplayStateType")) continue;
      if (field.name=="AttitudeDisplayStateType") field.choices={"EulerAngles","Quaternion","DirectionCosineMatrix","MRPs"};
      if (field.name=="AttitudeRateDisplayStateType") field.choices={"EulerAngleRates","AngularVelocity"};
      if (field.name=="EulerAngleSequence") for (const auto &choice:Attitude::GetEulerSequenceStrings()) field.choices.append(QString::fromStdString(choice));
      if (field.name=="AttitudeConstraintType") for (const auto &choice:Attitude::GetAttitudeConstraintTypes()) field.choices.append(QString::fromStdString(choice));
      if (field.name.startsWith("AngularVelocity") || field.name.startsWith("EulerAngleRate") || field.name=="SpinRate" || field.name=="PrecessionRate") field.unit="deg/s";
      else if (QRegularExpression("^EulerAngle[123]$").match(field.name).hasMatch() || field.name=="InitialSpinAngle" || field.name=="InitialPrecessionAngle" || field.name=="NutationAngle") field.unit="deg";
      if (field.name=="AttitudeCoordinateSystem" || field.name=="AttitudeReferenceBody") {
         field.references.clear();
         for (const auto &name:Moderator::Instance()->GetListOfObjects(field.name=="AttitudeCoordinateSystem" ? Gmat::COORDINATE_SYSTEM : Gmat::CELESTIAL_BODY))
            field.references.append(QString::fromStdString(name));
      }
      // These are scalar fields in the engine; orientation/rate edits are
      // submitted together as vectors/matrices, never one quaternion cell at a time.
      QWidget *control=nullptr;
      const bool selector=field.name=="AttitudeDisplayStateType" || field.name=="AttitudeRateDisplayStateType" || field.name=="EulerAngleSequence" || field.name=="AttitudeCoordinateSystem";
      if (!field.choices.isEmpty() || !field.references.isEmpty()) {
         auto *combo=new QComboBox(content); combo->addItems(field.choices.isEmpty() ? field.references : field.choices);
         if (combo->findText(field.value)<0) combo->addItem(field.value);
         combo->setCurrentText(field.value); control=combo;
         if (selector) connect(combo,&QComboBox::currentTextChanged,this,[this,name=field.name](const QString &value) { submit(name,value); });
      } else control=new QLineEdit(field.value,content);
      control->setObjectName("attitude_"+field.name); controls.insert(field.name,control); loaded.insert(field.name,field.value);
      QString section="Model settings";
      if (selector) section="Configuration";
      else if (field.name.startsWith("Q") || field.name.startsWith("DCM") || field.name.startsWith("MRP") || (field.name.startsWith("EulerAngle") && !field.name.startsWith("EulerAngleRate"))) section="Initial attitude";
      else if (field.name.startsWith("AngularVelocity") || field.name.startsWith("EulerAngleRate")) section="Initial rate";
      auto *row=new QWidget(content); auto *rowLayout=new QHBoxLayout(row); rowLayout->setContentsMargins(0,0,0,0); rowLayout->addWidget(control,1);
      if (!field.unit.isEmpty()) {
         auto *unit=new QLabel(field.unit,row); unit->setObjectName("attitudeUnit_"+field.name); rowLayout->addWidget(unit);
      }
      if (field.filename || field.name=="AttitudeFileName") {
         auto *browse=new QPushButton("Browse…",row); browse->setObjectName("attitudeBrowse_"+field.name); rowLayout->addWidget(browse);
         connect(browse,&QPushButton::clicked,this,[this,control] {
            QFileDialog dialog(this,"Select attitude ephemeris",controlValue(control)); dialog.setObjectName("attitudeFileDialog"); dialog.setFileMode(QFileDialog::ExistingFile);
            if (dialog.exec()==QDialog::Accepted && !dialog.selectedFiles().isEmpty()) qobject_cast<QLineEdit *>(control)->setText(dialog.selectedFiles().first());
         });
      }
      forms.value(section)->addRow(label(field.name),row);
   }
   for (auto *form:forms) if (!form->rowCount()) form->parentWidget()->hide();
   if (attitude->IsOfType("SpiceAttitude")) {
      auto *info=new QLabel("Configure attitude, clock and frame kernels in the spacecraft SPICE section.",content); info->setWordWrap(true); groups->addWidget(info);
   }
   groups->addStretch(); layout->insertWidget(2,fields,1);
}

QMap<QString,QString> AttitudeDialog::pendingValues() const
{
   // A retained form may outlive an interpreter rebuild. Reading pending input
   // must not clone/read engine objects or normalize the controls.
   QMap<QString,QString> result{{"Attitude",model->currentText()}};
   for (auto it=controls.cbegin();it!=controls.cend();++it) result[it.key()]=controlValue(it.value());
   return result;
}

QMap<QString,QString> AttitudeDialog::settings()
{
   if (!submit()) throw std::runtime_error(error->text().toStdString());
   QMap<QString,QString> result{{"Attitude",QString::fromStdString(candidate->GetStringParameter("Attitude"))}};
   for (const auto &field:attitudeFields(*candidate)) result[field.name]=field.value;
   return result;
}
bool AttitudeDialog::submit(const QString &selector,const QString &selected)
{
   try {
      std::unique_ptr<GmatBase> next(candidate->Clone());
      QMap<QString,QString> edits;
      for (auto it=controls.cbegin();it!=controls.cend();++it) {
         if (it.key()==selector) continue;
         const auto value=controlValue(it.value()); if (value!=loaded.value(it.key())) edits.insert(it.key(),value);
      }
      applyAttitudeProperties(*next,edits);
      if (selector=="Attitude") {
         const auto old=QString::fromStdString(next->GetStringParameter("Attitude"));
         models.insert(old,std::shared_ptr<GmatBase>(next->Clone()));
         if (models.contains(selected)) next.reset(models.value(selected)->Clone());
         else applyAttitudeProperties(*next,{{"Attitude",selected}});
      } else if (!selector.isEmpty()) applyAttitudeProperties(*next,{{selector,selected}});
      // Enumerate before committing, so failed conversion leaves pending inputs intact.
      attitudeFields(*next);
      candidate=std::move(next); error->clear();
      if (!selector.isEmpty()) rebuild();
      return true;
   } catch (BaseException &ex) { error->setText(QString::fromStdString(ex.GetFullMessage())); }
   catch (const std::exception &ex) { error->setText(QString::fromUtf8(ex.what())); }
   auto *combo=selector=="Attitude" ? model : qobject_cast<QComboBox *>(controls.value(selector));
   if (combo) { const QSignalBlocker block(combo); combo->setCurrentText(selector=="Attitude" ? QString::fromStdString(candidate->GetStringParameter("Attitude")) : loaded.value(selector)); }
   return false;
}
