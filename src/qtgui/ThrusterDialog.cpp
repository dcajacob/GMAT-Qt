#include "ThrusterDialog.hpp"
#include "VectorLineEdit.hpp"
#include "BurnDialog.hpp"
#include "ResourceProperties.hpp"
#include "GmatBase.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QGuiApplication>
#include <QScreen>
#include <cmath>
#include <memory>
#include <stdexcept>

void validateThrusterProperties(GmatBase &thruster)
{
   if (!thruster.IsOfType("Thruster")) return;
   validateBurnProperties(thruster);
   if (std::hypot(thruster.GetRealParameter("ThrustDirection1"),thruster.GetRealParameter("ThrustDirection2"),thruster.GetRealParameter("ThrustDirection3"))==0)
      throw std::runtime_error("Thrust direction must have a nonzero component.");
   if (thruster.IsOfType("ElectricThruster")) {
      if (thruster.GetRealParameter("MinimumUsablePower")>thruster.GetRealParameter("MaximumUsablePower")) throw std::runtime_error("Minimum usable power cannot exceed maximum usable power.");
      const auto model=thruster.GetStringParameter("ThrustModel");
      if (model!="ThrustMassPolynomial" && thruster.GetRealParameter("Isp")<=0) throw std::runtime_error("The selected thrust model requires Isp greater than zero.");
      if (model=="ConstantThrustAndIsp" && thruster.GetRealParameter("ConstantThrust")<0) throw std::runtime_error("Constant thrust must be nonnegative.");
      if (model=="FixedEfficiency" && (thruster.GetRealParameter("FixedEfficiency")<0 || thruster.GetRealParameter("FixedEfficiency")>1)) throw std::runtime_error("Fixed efficiency must be between zero and one.");
   }
}
ThrusterDialog::ThrusterDialog(GmatBase &thruster,const QMap<QString,QString> &pending,QWidget *parent) : QDialog(parent)
{
   setObjectName("thrusterDialog"); setWindowTitle("Thruster setup — "+QString::fromStdString(thruster.GetName()));
   resize(620,std::min(720,QGuiApplication::primaryScreen()->availableGeometry().height()-80));
   QMap<QString,QString> initial; QMap<QString,ResourceProperty> fields;
   for (const auto &field:resourceProperties(thruster)) { initial.insert(field.name,field.value); fields.insert(field.name,field); }
   for (auto it=pending.cbegin();it!=pending.cend();++it) initial.insert(it.key(),it.value());
   auto snapshot=std::shared_ptr<GmatBase>(thruster.Clone());
   auto *layout=new QVBoxLayout(this); auto *help=new QLabel("Configure direction and performance. Tanks/mixtures and polynomial coefficients have separate editors in the resource panel. OK keeps edits pending until Apply.",this); help->setWordWrap(true); layout->addWidget(help);
   auto *scroll=new QScrollArea(this); scroll->setObjectName("thrusterScroll"); scroll->setWidgetResizable(true); layout->addWidget(scroll,1);
   auto *content=new QWidget(scroll); scroll->setWidget(content); auto *groups=new QVBoxLayout(content);
   auto *direction=new QGroupBox("Direction",content); auto *directionForm=new QFormLayout(direction); groups->addWidget(direction);
   auto select=[&](QFormLayout *form,const QString &name,const QString &label) {
      auto *combo=new QComboBox(content); combo->setObjectName("thruster_"+name); combo->addItems(fields.value(name).choices.isEmpty() ? fields.value(name).references : fields.value(name).choices);
      if (combo->findText(initial.value(name))<0) combo->addItem(initial.value(name)); combo->setCurrentText(initial.value(name)); choices.insert(name,combo); form->addRow(label,combo);
   };
   auto number=[&](QFormLayout *form,const QString &name,const QString &label) {
      auto *edit=new VectorLineEdit(initial.value(name),content); edit->setObjectName("thruster_"+name); numbers.insert(name,edit);
      const auto caption=fields.value(name).unit=="dimensionless" ? label+" (dimensionless)" : label;
      form->addRow(caption,edit);
   };
   select(directionForm,"CoordinateSystem","Coordinate system"); select(directionForm,"Axes","Local axes"); select(directionForm,"Origin","Local origin");
   for (int i=1;i<=3;++i) number(directionForm,"ThrustDirection"+QString::number(i),"Direction "+QString::number(i));
   VectorLineEdit::link({numbers.value("ThrustDirection1"),numbers.value("ThrustDirection2"),numbers.value("ThrustDirection3")});
   auto updateFrame=[this] {
      const bool local=choices.value("CoordinateSystem")->currentText()=="Local";
      const auto axes=choices.value("Axes")->currentText(); choices.value("Axes")->setEnabled(local);
      choices.value("Origin")->setEnabled(local && axes!="MJ2000Eq" && axes!="SpacecraftBody");
   };
   connect(choices.value("CoordinateSystem"),&QComboBox::currentTextChanged,this,updateFrame); connect(choices.value("Axes"),&QComboBox::currentTextChanged,this,updateFrame); updateFrame();
   auto *performance=new QGroupBox("Performance",content); auto *form=new QFormLayout(performance); groups->addWidget(performance);
   number(form,"DutyCycle","Duty cycle (0–1)"); number(form,"ThrustScaleFactor","Thrust scale factor"); number(form,"GravitationalAccel","Gravitational acceleration (m/s²)");
   if (thruster.IsOfType("ElectricThruster")) {
      select(form,"ThrustModel","Thrust model"); number(form,"MinimumUsablePower","Minimum usable power (kW)"); number(form,"MaximumUsablePower","Maximum usable power (kW)");
      number(form,"FixedEfficiency","Fixed efficiency (0–1)"); number(form,"Isp","Isp (s)"); number(form,"ConstantThrust","Constant thrust (N)");
      auto updateModel=[this] {
         const auto model=choices.value("ThrustModel")->currentText();
         numbers.value("FixedEfficiency")->setEnabled(model=="FixedEfficiency"); numbers.value("ConstantThrust")->setEnabled(model=="ConstantThrustAndIsp"); numbers.value("Isp")->setEnabled(model!="ThrustMassPolynomial");
      };
      connect(choices.value("ThrustModel"),&QComboBox::currentTextChanged,this,updateModel); updateModel();
   }
   groups->addStretch(); error=new QLabel(this); error->setObjectName("thrusterError"); error->setWordWrap(true); layout->addWidget(error);
   for (auto *edit:numbers) connect(edit,&QLineEdit::textChanged,error,&QLabel::clear);
   for (auto *combo:choices) connect(combo,&QComboBox::currentTextChanged,error,&QLabel::clear);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons); connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[this,snapshot] {
      try { const auto values=settings(); std::unique_ptr<GmatBase> candidate(snapshot->Clone()); const auto applied=applyBurnProperties(*candidate,values); for (auto it=values.cbegin();it!=values.cend();++it) if (!applied.contains(it.key())) setResourceProperty(*candidate,it.key(),it.value()); validateThrusterProperties(*candidate); accept(); }
      catch (BaseException &exception) { error->setText(QString::fromStdString(exception.GetFullMessage())); }
      catch (const std::exception &exception) { error->setText(QString::fromUtf8(exception.what())); }
   });
}
QMap<QString,QString> ThrusterDialog::settings() const
{
   QMap<QString,QString> result;
   for (auto it=choices.cbegin();it!=choices.cend();++it) if ((it.key()!="Origin" && it.key()!="Axes") || choices.value("CoordinateSystem")->currentText()=="Local") result.insert(it.key(),it.value()->currentText());
   for (auto it=numbers.cbegin();it!=numbers.cend();++it) result.insert(it.key(),it.value()->text().trimmed()); return result;
}
