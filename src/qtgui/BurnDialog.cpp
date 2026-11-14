#include "BurnDialog.hpp"
#include "VectorLineEdit.hpp"
#include "ResourceProperties.hpp"
#include "GmatBase.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QGuiApplication>
#include <QScreen>
#include <QRegularExpression>
#include <memory>
#include <stdexcept>

QSet<QString> applyBurnProperties(GmatBase &object,const QMap<QString,QString> &values)
{
   QSet<QString> applied;
   const bool impulsive=object.IsOfType("ImpulsiveBurn");
   if (!impulsive && !object.IsOfType("Thruster")) return applied;
   if (values.contains("CoordinateSystem")) {
      setResourceProperty(object,"CoordinateSystem",values.value("CoordinateSystem")); applied.insert("CoordinateSystem");
   }
   for (const auto &name:QStringList{"Origin","Axes"}) if (values.contains(name)) {
      if (object.GetStringParameter("CoordinateSystem")=="Local") setResourceProperty(object,name,values.value(name));
      else if (!values.contains("CoordinateSystem")) throw std::runtime_error("Local origin and axes require CoordinateSystem = Local.");
      applied.insert(name);
   }
   if (impulsive && values.contains("Tank")) {
      const auto names=splitResourceReferences(values.value("Tank")); QSet<QString> seen;
      for (const auto &name:names) {
         auto *tank=Moderator::Instance()->GetConfiguredObject(name.toStdString());
         if (!tank || !tank->IsOfType(Gmat::FUEL_TANK) || tank->GetName()!=name.toStdString()) throw std::runtime_error("Select an available fuel tank.");
         if (seen.contains(name)) throw std::runtime_error("Each tank must be unique."); seen.insert(name);
      }
      object.TakeAction("ClearTanks"); for (const auto &name:names) object.SetStringParameter("Tank",name.toStdString()); applied.insert("Tank");
   }
   return applied;
}
void validateBurnProperties(GmatBase &object)
{
   const bool impulsive=object.IsOfType("ImpulsiveBurn");
   if (!impulsive && !object.IsOfType("Thruster")) return;
   const auto frame=object.GetStringParameter("CoordinateSystem");
   if (frame=="Local") {
      auto *body=Moderator::Instance()->GetConfiguredObject(object.GetStringParameter("Origin"));
      if (!body || !body->IsOfType(Gmat::CELESTIAL_BODY)) throw std::runtime_error("Select an available local origin body.");
   } else {
      auto *coordinate=Moderator::Instance()->GetConfiguredObject(frame);
      if (!coordinate || !coordinate->IsOfType(Gmat::COORDINATE_SYSTEM)) throw std::runtime_error("Select Local or an available coordinate system.");
   }
   if (impulsive && object.GetBooleanParameter("DecrementMass") && object.GetStringArrayParameter("Tank").size()>1)
      throw std::runtime_error("Impulsive mass depletion supports one fuel tank. Select a single tank.");
}
QString burnResourceScript(GmatBase &object)
{
   auto block=QString::fromStdString(object.GetGeneratingString(Gmat::SCRIPTING));
   const QString name=object.IsOfType("ImpulsiveBurn") ? "Tank" : "Thrusters";
   if (object.GetStringArrayParameter(name.toStdString()).empty()) {
      const auto key=QString::fromStdString(object.GetName())+"."+name;
      block.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?"+QRegularExpression::escape(key)+"[ \\t]*=[ \\t]*\\{[^;]*?\\}[ \\t]*;",QRegularExpression::MultilineOption));
   }
   return block;
}
BurnDialog::BurnDialog(GmatBase &burn,const QMap<QString,QString> &pending,QWidget *parent) : QDialog(parent)
{
   setObjectName("burnDialog"); setWindowTitle("Impulsive burn setup — "+QString::fromStdString(burn.GetName()));
   resize(600,std::min(650,QGuiApplication::primaryScreen()->availableGeometry().height()-80));
   QMap<QString,QString> initial; QMap<QString,ResourceProperty> fields;
   for (const auto &field:resourceProperties(burn)) { initial.insert(field.name,field.value); fields.insert(field.name,field); }
   for (auto it=pending.cbegin();it!=pending.cend();++it) initial.insert(it.key(),it.value());
   auto snapshot=std::shared_ptr<GmatBase>(burn.Clone());
   auto *layout=new QVBoxLayout(this); auto *help=new QLabel("Configure the delta-V vector and optional fuel depletion. OK keeps changes pending until resource Apply.",this); help->setWordWrap(true); layout->addWidget(help);
   auto *scroll=new QScrollArea(this); scroll->setObjectName("burnScroll"); scroll->setWidgetResizable(true); layout->addWidget(scroll,1);
   auto *content=new QWidget(scroll); scroll->setWidget(content); auto *groups=new QVBoxLayout(content);
   auto *direction=new QGroupBox("Delta-V vector",content); auto *directionForm=new QFormLayout(direction); groups->addWidget(direction);
   auto select=[&](QFormLayout *form,const QString &name,const QString &label) {
      auto *combo=new QComboBox(content); combo->setObjectName("burn_"+name); combo->addItems(fields.value(name).choices.isEmpty() ? fields.value(name).references : fields.value(name).choices);
      if (combo->findText(initial.value(name))<0) combo->addItem(initial.value(name)); combo->setCurrentText(initial.value(name)); choices.insert(name,combo); form->addRow(label,combo);
   };
   auto number=[&](QFormLayout *form,const QString &name,const QString &label) { auto *edit=new VectorLineEdit(initial.value(name),content); edit->setObjectName("burn_"+name); numbers.insert(name,edit); form->addRow(label,edit); };
   select(directionForm,"CoordinateSystem","Coordinate system"); select(directionForm,"Axes","Local axes"); select(directionForm,"Origin","Local origin");
   for (int i=1;i<=3;++i) number(directionForm,"Element"+QString::number(i),"Element "+QString::number(i)+" (km/s)");
   VectorLineEdit::link({numbers.value("Element1"),numbers.value("Element2"),numbers.value("Element3")});
   auto updateFrame=[this] {
      const bool local=choices.value("CoordinateSystem")->currentText()=="Local"; const auto axes=choices.value("Axes")->currentText();
      choices.value("Axes")->setEnabled(local); choices.value("Origin")->setEnabled(local && axes!="MJ2000Eq" && axes!="SpacecraftBody");
   };
   connect(choices.value("CoordinateSystem"),&QComboBox::currentTextChanged,this,updateFrame); connect(choices.value("Axes"),&QComboBox::currentTextChanged,this,updateFrame); updateFrame();
   auto *fuel=new QGroupBox("Fuel depletion",content); auto *fuelForm=new QFormLayout(fuel); groups->addWidget(fuel);
   decrement=new QCheckBox("Decrement mass",fuel); decrement->setObjectName("burn_DecrementMass"); decrement->setChecked(initial.value("DecrementMass")=="true"); fuelForm->addRow(decrement);
   auto *tank=new QComboBox(fuel); tank->setObjectName("burn_Tank"); tank->addItem("No fuel tank",QString());
   for (const auto &name:fields.value("Tank").references) tank->addItem(name,name);
   auto index=tank->findData(initial.value("Tank"));
   if (index<0) { tank->addItem("Existing tanks: "+initial.value("Tank"),initial.value("Tank")); index=tank->count()-1; }
   tank->setCurrentIndex(index); choices.insert("Tank",tank); fuelForm->addRow("Fuel tank",tank);
   number(fuelForm,"Isp","Isp (s)"); number(fuelForm,"GravitationalAccel","Gravitational acceleration (m/s²)");
   auto updateMass=[this] { const bool active=decrement->isChecked(); numbers.value("Isp")->setEnabled(active); numbers.value("GravitationalAccel")->setEnabled(active); };
   connect(decrement,&QCheckBox::toggled,this,updateMass); updateMass();
   groups->addStretch(); error=new QLabel(this); error->setObjectName("burnError"); error->setWordWrap(true); layout->addWidget(error);
   for (auto *edit:numbers) connect(edit,&QLineEdit::textChanged,error,&QLabel::clear);
   for (auto *combo:choices) connect(combo,&QComboBox::currentTextChanged,error,&QLabel::clear);
   connect(decrement,&QCheckBox::toggled,error,&QLabel::clear);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons); connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[this,snapshot] {
      try {
         const auto values=settings(); std::unique_ptr<GmatBase> candidate(snapshot->Clone()); const auto applied=applyBurnProperties(*candidate,values);
         for (auto it=values.cbegin();it!=values.cend();++it) if (!applied.contains(it.key())) setResourceProperty(*candidate,it.key(),it.value());
         validateBurnProperties(*candidate); if (!candidate->Validate()) throw std::runtime_error("The burn rejected these settings."); accept();
      } catch (BaseException &exception) { error->setText(QString::fromStdString(exception.GetFullMessage())); }
      catch (const std::exception &exception) { error->setText(QString::fromUtf8(exception.what())); }
   });
}
QMap<QString,QString> BurnDialog::settings() const
{
   QMap<QString,QString> result;
   for (auto it=choices.cbegin();it!=choices.cend();++it) if ((it.key()!="Origin" && it.key()!="Axes") || choices.value("CoordinateSystem")->currentText()=="Local")
      result.insert(it.key(),it.key()=="Tank" ? it.value()->currentData().toString() : it.value()->currentText());
   for (auto it=numbers.cbegin();it!=numbers.cend();++it) result.insert(it.key(),it.value()->text().trimmed());
   result.insert("DecrementMass",decrement->isChecked() ? "true" : "false"); return result;
}
