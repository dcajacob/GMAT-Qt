#include "ResourcePreview.hpp"
#include "ResourceForm.hpp"
#include "EphemerisDialog.hpp"
#include "ThrusterDialog.hpp"
#include "BurnDialog.hpp"
#include "GroundStationDialog.hpp"
#include "RegionDialog.hpp"
#include "CustomFovDialog.hpp"
#include "OrbitViewDialog.hpp"
#include "GroundTrackDialog.hpp"
#include "XYPlotDialog.hpp"
#include "EventLocatorDialog.hpp"
#include "DynamicDataDialog.hpp"
#include "TrackingConfigDialog.hpp"
#include "BallisticsMassDialog.hpp"
#include "AttitudeDialog.hpp"
#include "VisualModelDialog.hpp"
#include "CelestialBodyPanel.hpp"
#include "CelestialBody.hpp"
#include "ResourceProperties.hpp"
#include "ScriptCompatibility.hpp"
#include "GmatBase.hpp"
#include "Spacecraft.hpp"
#include <QDialogButtonBox>
#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QKeyEvent>
#include <memory>

ResourceForm::ResourceForm(GmatBase &object,QWidget *parent,const QString &script) : QWidget(parent)
{
   setObjectName("resourceForm");
   auto *layout=new QVBoxLayout(this); layout->setContentsMargins(0,0,0,0);
   tabs=new QTabWidget(this); tabs->setObjectName("resourceFormTabs"); layout->addWidget(tabs);
   const auto type=object.GetTypeName();
   if (auto *body=dynamic_cast<CelestialBody *>(&object)) {
      auto *form=new CelestialBodyPanel(*body,{},this); add("Body",form,[form] { return form->settings(); });
   } else if (object.IsOfType("EphemerisFile")) {
      auto *form=new EphemerisDialog(object,{},this); add("Output",form,[form] { return form->settings(); });
   } else if (object.IsOfType("Thruster")) {
      auto *form=new ThrusterDialog(object,{},this); add("Direction and performance",form,[form] { return form->settings(); });
   } else if (object.IsOfType("ImpulsiveBurn")) {
      auto *form=new BurnDialog(object,{},this); add("Burn",form,[form] { return form->settings(); });
   } else if (object.IsOfType("PlanetographicRegion")) {
      auto *form=new RegionDialog(object,this); add("Region",form,[form] { return form->settings(); });
   } else if (object.IsOfType("CustomFOV")) {
      auto *form=new CustomFovDialog(object,script,this); add("Field of view",form,[form] { return form->settings(); });
   } else if (object.IsOfType("GroundStation")) {
      auto *form=new GroundStationDialog(object,{},this); add("Station",form,[form] { return form->settings(); });
   } else if (type=="OrbitView") {
      QMap<QString,QString> pending;
      if (const auto count=qtCameraSettings(script).value(QString::fromStdString(object.GetName())).solverLastN) { pending.insert("SolverIterations","LastN"); pending.insert("@QtSolverLastN",QString::number(*count)); }
      auto *form=new OrbitViewDialog(object,pending,this); add("Orbit display",form,[form] { return form->settings(); });
   } else if (type=="GroundTrack" || type=="GroundTrackPlot") {
      auto *form=new GroundTrackDialog(object,{},this); add("Ground track",form,[form] { return form->settings(); });
   } else if (type=="XYPlot") {
      auto *form=new XYPlotDialog(object,{},this); add("XY plot",form,[form] { return form->settings(); });
   } else if (object.IsOfType("EventLocator")) {
      auto *form=new EventLocatorDialog(object,{},this); add("Events",form,[form] { return form->settings(); });
   } else if (object.IsOfType("DynamicDataDisplay")) {
      auto *form=new DynamicDataDialog(object,{},this); add("Data",form,[form] { return form->settings(); });
   } else if (object.IsOfType("TrackingFileSet")) {
      auto *form=new TrackingConfigDialog(object,{},this); add("Tracking",form,[form] { return QMap<QString,QString>{{"@TrackingConfigs",form->settings()}}; });
   } else if (object.IsOfType("Spacecraft")) {
      QMap<QString,QString> values; QMap<QString,QStringList> choices;
      for (const auto &field:resourceProperties(object)) { values[field.name]=field.value; choices[field.name]=field.choices; }
      auto *mass=new BallisticsMassDialog(values,choices,this); add("Ballistics and mass",mass,[mass] { return mass->values(); },true);
      auto *attitude=new AttitudeDialog(object,{},this); add("Attitude",attitude,[attitude] { return attitude->pendingValues(); });
      QMap<QString,QString> modelValues;
      for (auto it=values.cbegin();it!=values.cend();++it) if (it.key().startsWith("Model") || it.key()=="OrbitColor" || it.key()=="TargetColor") modelValues.insert(it.key(),it.value());
      auto *model=new VisualModelDialog(modelValues,[](const QMap<QString,QString> &pending) {
         // Model validation uses only model/colour fields. A configured-spacecraft
         // clone retains borrowed coordinate references that a rebuild can retire.
         QtResourcePreview preview(new Spacecraft("QtVisualValidation"));
         for (auto it=pending.cbegin();it!=pending.cend();++it) setResourceProperty(*preview,it.key(),it.value());
         auto normalized=pending;
         for (auto it=pending.cbegin();it!=pending.cend();++it)
            if (it.key().startsWith("ModelRotation") || it.key().startsWith("ModelOffset") || it.key()=="ModelScale")
               normalized[it.key()]=QString::number(preview->GetRealParameter(it.key().toStdString()),'g',17);
         normalized["@ResolvedModelFile"]=pending.value("ModelFile").isEmpty() ? QString() : QString::fromStdString(preview->GetStringParameter("ModelFileFullPath"));
         return normalized;
      },this);
      add("Visual model",model,[model] { return model->settings(); },true);
   }
   if (readers.size()==1) {
      auto *page=tabs->widget(0); tabs->removeTab(0); layout->removeWidget(tabs); tabs->hide();
      layout->addWidget(page); page->show();
   }
}
void ResourceForm::add(const QString &label,QWidget *dialog,std::function<QMap<QString,QString>()> read,bool scrollable)
{
   dialog->setWindowFlags(Qt::Widget);
   if (auto *modal=qobject_cast<QDialog *>(dialog)) modal->setModal(false);
   dialog->installEventFilter(this);
   for (auto *buttons:dialog->findChildren<QDialogButtonBox *>(QString(),Qt::FindDirectChildrenOnly)) {
      buttons->hide();
      for (auto *button:buttons->findChildren<QPushButton *>()) { button->setDefault(false); button->setAutoDefault(false); }
   }
   for (auto *help:dialog->findChildren<QLabel *>(QString(),Qt::FindDirectChildrenOnly))
      if (help->text().contains("OK")) help->setText("Edit settings here. Apply commits them; Close lets you discard pending changes.");
   QWidget *page=dialog;
   if (scrollable) {
      auto *scroll=new QScrollArea(this); scroll->setObjectName("resourceFormScroll");
      scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
      scroll->setWidget(dialog); page=scroll;
   }
   tabs->addTab(page,label); originals.append(read()); readers.append(std::move(read));
}
QMap<QString,QString> ResourceForm::values() const
{
   QMap<QString,QString> result;
   for (const auto &read:readers) { const auto values=read(); for (auto it=values.cbegin();it!=values.cend();++it) result[it.key()]=it.value(); }
   return result;
}
QSet<QString> ResourceForm::fields() const
{
   QSet<QString> result; const auto current=values();
   for (auto it=current.cbegin();it!=current.cend();++it) result.insert(it.key());
   // A format/model switch can activate a field omitted by the current reader.
   // Its control still owns that field; do not expose a second Advanced editor.
   for (auto *control:findChildren<QWidget *>()) {
      const auto name=control->objectName(); const int separator=name.indexOf('_');
      if (separator>0) result.insert(name.mid(separator+1));
   }
   return result;
}
QMap<QString,QString> ResourceForm::changes() const
{
   QMap<QString,QString> result;
   for (int i=0;i<readers.size();++i) {
      const auto values=readers[i]();
      if (values!=originals[i]) for (auto it=values.cbegin();it!=values.cend();++it) result[it.key()]=it.value();
   }
   return result;
}
bool ResourceForm::eventFilter(QObject *object,QEvent *event)
{
   if (event->type()==QEvent::KeyPress && static_cast<QKeyEvent *>(event)->key()==Qt::Key_Escape) return true;
   return QWidget::eventFilter(object,event);
}
