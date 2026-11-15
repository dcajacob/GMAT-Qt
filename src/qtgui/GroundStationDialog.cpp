#include "GroundStationDialog.hpp"
#include "PlacePicker.hpp"
#include "ResourceProperties.hpp"
#include "BodyFixedPoint.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "BaseException.hpp"
#include "RgbColor.hpp"
#include <QColorDialog>
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
#include <QStylePainter>
#include <QStyleOptionFrame>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
class CoordinateLineEdit final : public QLineEdit
{
public:
   CoordinateLineEdit(const QString &value,QWidget *parent) : QLineEdit(value,parent)
   {
      const auto describe=[this] {
         setToolTip("Exact value " + text() + "\nFocus this field to edit all digits.");
         setAccessibleDescription("Exact value " + text());
      };
      connect(this,&QLineEdit::textChanged,this,describe);
      describe();
   }
protected:
   void paintEvent(QPaintEvent *event) override
   {
      bool valid=false; const double value=text().toDouble(&valid);
      if (hasFocus() || !valid || !std::isfinite(value)) { QLineEdit::paintEvent(event); return; }
      QStyleOptionFrame option; initStyleOption(&option);
      QStylePainter painter(this); painter.drawPrimitive(QStyle::PE_PanelLineEdit,option);
      const QRect content=style()->subElementRect(QStyle::SE_LineEditContents,&option,this)
         .marginsRemoved(textMargins()).adjusted(2,0,-2,0);
      painter.setClipRect(content); painter.setFont(font());
      painter.setPen(palette().color(isEnabled() ? QPalette::Active : QPalette::Disabled,QPalette::Text));
      painter.drawText(content,alignment()|Qt::AlignVCenter,QString::number(value,'g',12));
   }
};
}

QSet<QString> applyGroundStationLocation(GmatBase &object,const QMap<QString,QString> &values)
{
   QSet<QString> consumed;
   if (!object.IsOfType("GroundStation")) return consumed;
   for (const auto &name:QStringList{"CentralBody","StateType","HorizonReference","Location1","Location2","Location3"})
      if (values.contains(name)) consumed.insert(name);
   if (consumed.isEmpty()) return consumed;
   auto *station=dynamic_cast<BodyFixedPoint *>(&object);
   if (!station) throw std::runtime_error("This station does not provide a body-fixed location.");
   auto *solarSystem=Moderator::Instance()->GetSolarSystemInUse();
   const auto body=values.value("CentralBody",QString::fromStdString(station->GetStringParameter("CentralBody")));
   if (!solarSystem || !solarSystem->GetBody(body.toStdString())) throw std::runtime_error("Select an available celestial body for the station.");
   station->SetStringParameter("CentralBody",body.toStdString());
   station->SetSolarSystem(solarSystem);
   // Initialize only the body/frame data. GroundStation's override also
   // initializes hardware; that belongs to interpretation/execution, not preview.
   station->BodyFixedPoint::Initialize();
   for (const auto &name:QStringList{"StateType","HorizonReference","Location1","Location2","Location3"})
      if (values.contains(name)) setResourceProperty(object,name,values.value(name));
   return consumed;
}

GroundStationDialog::GroundStationDialog(GmatBase &station,const QMap<QString,QString> &pending,QWidget *parent)
   : QDialog(parent),original(station.Clone())
{
   setObjectName("groundStationDialog"); setWindowTitle("Ground station");
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Use settings keeps changes pending. Apply the station to update the mission.",this); help->setWordWrap(true); layout->addWidget(help);
   auto *scroll=new QScrollArea(this); scroll->setObjectName("stationScroll"); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
   auto *content=new QWidget(scroll); auto *groups=new QVBoxLayout(content); groups->setContentsMargins(0,0,0,0); scroll->setWidget(content); layout->addWidget(scroll,1);
   std::unique_ptr<GmatBase> preview(original->Clone()); QString initialError;
   try { applyGroundStationLocation(*preview,pending); }
   catch (BaseException &failure) { initialError=QString::fromStdString(failure.GetFullMessage()); }
   catch (const std::exception &failure) { initialError=QString::fromUtf8(failure.what()); }
   QMap<QString,QString> initial;
   for (const auto &property:resourceProperties(*preview)) initial.insert(property.name,property.value);
   for (auto it=pending.cbegin();it!=pending.cend();++it) initial.insert(it.key(),it.value());
   auto *identity=new QGroupBox("Station",content); auto *identityForm=new QFormLayout(identity); groups->addWidget(identity);
   auto text=[&](QFormLayout *form,const QString &name,const QString &label,const QString &unit=QString()) {
      auto *row=new QWidget(this); auto *line=new QHBoxLayout(row); line->setContentsMargins(0,0,0,0);
      auto *edit=new QLineEdit(initial.value(name),row); edit->setObjectName("station_"+name); fields.insert(name,edit); line->addWidget(edit);
      if (!unit.isEmpty()) line->addWidget(new QLabel(unit,row)); form->addRow(label,row); return edit;
   };
   text(identityForm,"Id","ID"); text(identityForm,"MinimumElevationAngle","Minimum elevation","deg");
   auto *location=new QGroupBox("Location",content); auto *locationForm=new QFormLayout(location); groups->addWidget(location);
   body=new QComboBox(location); body->setObjectName("station_CentralBody");
   for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::CELESTIAL_BODY)) body->addItem(QString::fromStdString(name));
   body->setCurrentText(initial.value("CentralBody")); locationForm->addRow("Central body",body);
   auto selector=[&](const QString &name,const QString &label) {
      auto *combo=new QComboBox(location); combo->setObjectName("station_"+name);
      for (const auto &choice:station.GetPropertyEnumStrings(station.GetParameterID(name.toStdString()))) combo->addItem(QString::fromStdString(choice));
      combo->setCurrentText(initial.value(name)); locationForm->addRow(label,combo); return combo;
   };
   type=selector("StateType","State type"); horizon=selector("HorizonReference","Horizon reference");
   for (int i=0;i<3;++i) {
      const auto name="Location"+QString::number(i+1); auto *row=new QWidget(location); auto *line=new QHBoxLayout(row); line->setContentsMargins(0,0,0,0);
      auto *edit=new CoordinateLineEdit(initial.value(name),row); edit->setObjectName("station_"+name); fields.insert(name,edit); line->addWidget(edit);
      labels[i]=new QLabel(location); labels[i]->setObjectName("stationLabel_"+name);
      units[i]=new QLabel(row); units[i]->setObjectName("stationUnit_"+name); line->addWidget(units[i]); locationForm->addRow(labels[i],row);
   }
   auto *findPlace=new QPushButton("Find a place…",location); findPlace->setObjectName("stationFindPlace");
   findPlace->setToolTip("Choose an offline place or search online to fill latitude and longitude");
   findPlace->setEnabled(body->currentText()=="Earth"); locationForm->addRow(findPlace);
   connect(findPlace,&QPushButton::clicked,this,[this] { choosePlace(); });
   connect(body,&QComboBox::currentTextChanged,findPlace,[findPlace](const QString &name) { findPlace->setEnabled(name=="Earth"); });
   auto *display=new QGroupBox("Display",content); auto *displayForm=new QFormLayout(display); groups->addWidget(display);
   for (const auto &name:QStringList{"OrbitColor","TargetColor"}) {
      auto *edit=text(displayForm,name,name=="OrbitColor" ? "Station color" : "Target color");
      auto *button=new QPushButton("Choose…",display); button->setObjectName("stationChoose_"+name);
      qobject_cast<QHBoxLayout *>(edit->parentWidget()->layout())->addWidget(button);
      connect(button,&QPushButton::clicked,this,[this,edit] {
         QColor initial;
         try { initial=QColor::fromRgb(RgbColor::ToIntColor(edit->text().trimmed().toStdString())&0xffffff); } catch (...) {}
         QColorDialog picker(initial,this); picker.setObjectName("stationColorDialog");
         if (picker.exec()==QDialog::Accepted) { const auto color=picker.selectedColor(); edit->setText(QString("[%1 %2 %3]").arg(color.red()).arg(color.green()).arg(color.blue())); }
      });
   }
   auto *mask=new QGroupBox("Horizon mask",content); auto *maskForm=new QFormLayout(mask); groups->addWidget(mask);
   auto *maskEdit=text(maskForm,"HorizonMaskFileName","Optional input file");
   auto *browse=new QPushButton("Browse…",mask); browse->setObjectName("stationBrowse_Mask"); qobject_cast<QHBoxLayout *>(maskEdit->parentWidget()->layout())->addWidget(browse);
   auto *clear=new QPushButton("Clear",mask); clear->setObjectName("stationClear_Mask"); qobject_cast<QHBoxLayout *>(maskEdit->parentWidget()->layout())->addWidget(clear);
   connect(clear,&QPushButton::clicked,maskEdit,&QLineEdit::clear);
   connect(browse,&QPushButton::clicked,this,[this,maskEdit] {
      QFileDialog picker(this,"Choose horizon mask",maskEdit->text()); picker.setObjectName("stationMaskDialog"); picker.setFileMode(QFileDialog::ExistingFile);
      if (picker.exec()==QDialog::Accepted && !picker.selectedFiles().isEmpty()) maskEdit->setText(picker.selectedFiles().first());
   });
   groups->addStretch();
   error=new QLabel(this); error->setObjectName("stationError"); error->setWordWrap(true); layout->addWidget(error);
   for (auto *edit:fields) connect(edit,&QLineEdit::textChanged,error,&QLabel::clear);
   previousBody=body->currentText(); previousType=type->currentText(); previousHorizon=horizon->currentText();
   refresh(*preview,false); error->setText(initialError);
   connect(fields.value("Location2"),&QLineEdit::textChanged,this,[this] { refreshLongitudeHint(); });
   for (auto *combo:{body,type,horizon}) connect(combo,&QComboBox::currentTextChanged,this,[this] { convert(); });
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   buttons->button(QDialogButtonBox::Ok)->setText("Use settings");
   buttons->button(QDialogButtonBox::Ok)->setToolTip("Keep these settings pending in the resource editor, then Apply them to the mission");
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[this] {
      try {
         const auto proposed=settings(); std::unique_ptr<GmatBase> preview(original->Clone());
         const auto consumed=applyGroundStationLocation(*preview,proposed);
         for (auto it=proposed.cbegin();it!=proposed.cend();++it) if (!consumed.contains(it.key())) setResourceProperty(*preview,it.key(),it.value());
         acceptedValues=proposed;
         // Keep the engine's normalized longitude in the pending panel too.
         for (int i=1;i<=3;++i) acceptedValues["Location"+QString::number(i)]=QString::number(preview->GetRealParameter("Location"+std::to_string(i)),'g',17);
         accept();
      } catch (BaseException &failure) { error->setText(QString::fromStdString(failure.GetFullMessage())); }
      catch (const std::exception &failure) { error->setText(QString::fromUtf8(failure.what())); }
   });
   const auto available=screen()->availableGeometry();
   resize(std::min(760,available.width()-40),std::min(640,available.height()-80));
}

GroundStationDialog::~GroundStationDialog()=default;

void GroundStationDialog::choosePlace()
{
   PlacePicker picker(this);
   if (picker.exec()!=QDialog::Accepted) return;
   const auto place=picker.selectedPlace();
   if (!place) return;
   try {
      std::unique_ptr<GmatBase> preview(original->Clone());
      applyGroundStationLocation(*preview,settings());
      preview->SetStringParameter("StateType","Spherical");
      preview->SetStringParameter("HorizonReference","Ellipsoid");
      preview->SetRealParameter("Location1",place->coordinates.y());
      preview->SetRealParameter("Location2",place->coordinates.x());
      const QSignalBlocker typeBlock(type),horizonBlock(horizon);
      type->setCurrentText("Spherical"); horizon->setCurrentText("Ellipsoid");
      refresh(*preview); previousType=type->currentText(); previousHorizon=horizon->currentText(); error->clear();
   } catch (BaseException &failure) { error->setText(QString::fromStdString(failure.GetFullMessage())); }
   catch (const std::exception &failure) { error->setText(QString::fromUtf8(failure.what())); }
}

QMap<QString,QString> GroundStationDialog::settings() const
{
   QMap<QString,QString> result={{"CentralBody",body->currentText()},{"StateType",type->currentText()},{"HorizonReference",horizon->currentText()}};
   for (auto it=fields.cbegin();it!=fields.cend();++it) result.insert(it.key(),it.value()->text());
   return result;
}

void GroundStationDialog::refresh(GmatBase &station,bool updateValues)
{
   for (int i=0;i<3;++i) {
      const auto index=std::to_string(i+1);
      labels[i]->setText(QString::fromStdString(station.GetStringParameter("LOCATION_LABEL_"+index)));
      units[i]->setText(QString::fromStdString(station.GetStringParameter("LOCATION_UNITS_"+index)));
      if (updateValues) fields["Location"+QString::number(i+1)]->setText(QString::number(station.GetRealParameter("Location"+index),'g',17));
   }
   horizon->setEnabled(type->currentText()!="Cartesian");
   refreshLongitudeHint();
}

void GroundStationDialog::refreshLongitudeHint()
{
   if (type->currentText()!="Spherical") return;
   bool valid=false; const double longitude=fields.value("Location2")->text().toDouble(&valid);
   if (!valid || !std::isfinite(longitude)) { units[1]->setText("deg E"); return; }
   const double signedLongitude=std::remainder(longitude,360.);
   units[1]->setText(QString("deg E · %1° %2").arg(QString::number(std::abs(signedLongitude),'g',10),signedLongitude<0 ? "W" : "E"));
   units[1]->setWordWrap(true);
   units[1]->setToolTip("The field uses east-positive longitude. The equivalent signed direction is shown here.");
}

void GroundStationDialog::convert()
{
   try {
      auto source=settings(); source["StateType"]=previousType; source["HorizonReference"]=previousHorizon;
      std::unique_ptr<GmatBase> preview(original->Clone()); applyGroundStationLocation(*preview,source);
      preview->SetStringParameter("StateType",type->currentText().toStdString());
      preview->SetStringParameter("HorizonReference",horizon->currentText().toStdString());
      refresh(*preview); error->clear(); previousBody=body->currentText(); previousType=type->currentText(); previousHorizon=horizon->currentText();
   } catch (BaseException &failure) {
      const QSignalBlocker a(body),b(type),c(horizon); body->setCurrentText(previousBody); type->setCurrentText(previousType); horizon->setCurrentText(previousHorizon);
      findChild<QPushButton *>("stationFindPlace")->setEnabled(previousBody=="Earth");
      error->setText(QString::fromStdString(failure.GetFullMessage()));
   } catch (const std::exception &failure) {
      const QSignalBlocker a(body),b(type),c(horizon); body->setCurrentText(previousBody); type->setCurrentText(previousType); horizon->setCurrentText(previousHorizon);
      findChild<QPushButton *>("stationFindPlace")->setEnabled(previousBody=="Earth");
      error->setText(QString::fromUtf8(failure.what()));
   }
}
