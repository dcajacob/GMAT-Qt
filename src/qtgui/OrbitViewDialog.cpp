#include "ResourcePreview.hpp"
#include "OrbitViewDialog.hpp"
#include "ResourceProperties.hpp"
#include "GmatBase.hpp"
#include "OrbitPlot.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QGuiApplication>
#include <QScreen>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>

namespace {
const QStringList cameraFields={"ViewPointReference","ViewPointVector","ViewDirection"};
QStringList vectorParts(const QString &value)
{
   auto input=value.trimmed();
   if (!input.startsWith('[') || !input.endsWith(']')) throw std::runtime_error("Enter a camera vector in brackets with three finite components.");
   auto parts=input.mid(1,input.size()-2).split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
   if (parts.size()!=3) throw std::runtime_error("A camera vector requires three components.");
   for (const auto &part:parts) { bool ok=false; auto number=part.toDouble(&ok); if (!ok || !std::isfinite(number)) throw std::runtime_error("Camera vector components must be finite numbers."); }
   return parts;
}
BooleanArray drawFlags(const QString &value,int count)
{
   const auto parts=value.split(QRegularExpression("[\\s,\\[\\]]+"),Qt::SkipEmptyParts);
   if (parts.size()!=count) throw std::runtime_error("DrawObject requires one true/false value for each selected object.");
   BooleanArray result;
   for (const auto &part:parts) { if (part!="true" && part!="false") throw std::runtime_error("DrawObject accepts only true or false."); result.push_back(part=="true"); }
   return result;
}
}
std::optional<int> orbitSolverLastN(const QString &value)
{
   const auto text=value.trimmed();
   if (text.isEmpty()) return std::nullopt;
   bool valid=false; const int count=text.toInt(&valid);
   if (!valid || count<0 || !QRegularExpression("^[+]?[0-9]+$").match(text).hasMatch())
      throw std::runtime_error("LastN count must be a nonnegative integer.");
   return count;
}
QSet<QString> applyOrbitViewProperties(GmatBase &plot,const QMap<QString,QString> &values)
{
   QSet<QString> applied;
   if (!plot.IsOfType("OrbitView")) return applied;
   if (values.contains("@QtSolverLastN")) { orbitSolverLastN(values.value("@QtSolverLastN")); applied.insert("@QtSolverLastN"); }
   if (values.contains("SolverIterations")) {
      const auto mode=values.value("SolverIterations");
      if (!QStringList{"All","Current","None","LastN"}.contains(mode)) throw std::runtime_error("Choose All, Current, None or LastN solver iterations.");
      setResourceProperty(plot,"SolverIterations",mode=="LastN" ? "All" : mode); applied.insert("SolverIterations");
   }
   if (values.contains("Add") || values.contains("DrawObject")) {
      const auto oldNames=plot.GetStringArrayParameter("Add"); BooleanArray oldFlags;
      for (const auto &name:oldNames) oldFlags.push_back(static_cast<OrbitPlot &>(plot).GetShowObject(name));
      QStringList names;
      if (values.contains("Add")) names=splitResourceReferences(values.value("Add")); else for (const auto &name:oldNames) names.append(QString::fromStdString(name));
      BooleanArray visible;
      if (values.contains("DrawObject")) visible=drawFlags(values.value("DrawObject"),names.size());
      else for (const auto &name:names) { auto found=std::find(oldNames.begin(),oldNames.end(),name.toStdString()); auto index=static_cast<size_t>(found-oldNames.begin()); visible.push_back(index>=oldFlags.size() || oldFlags[index]); }
      QSet<QString> unique;
      for (const auto &name:names) {
         auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString());
         if (!object || !object->IsOfType(Gmat::SPACE_POINT) || unique.contains(name)) throw std::runtime_error("Select each available space point at most once.");
         unique.insert(name);
      }
      if (!plot.TakeAction("Clear")) throw std::runtime_error("Cannot replace the selected plot objects.");
      for (int i=0;i<names.size();++i) if (!plot.SetStringParameter("Add",names[i].toStdString(),i)) throw std::runtime_error("Plot rejected an object selection.");
      plot.SetBooleanArrayParameter("DrawObject",visible); applied.insert("Add"); applied.insert("DrawObject");
   }
   for (const auto &field:cameraFields) if (values.contains(field)) {
      const auto value=values.value(field).trimmed();
      if (value.startsWith('[')) vectorParts(value);
      else { auto *object=Moderator::Instance()->GetConfiguredObject(value.toStdString()); if (!object || !object->IsOfType(Gmat::SPACE_POINT)) throw std::runtime_error("Select a space point or a three-component camera vector."); }
      setResourceProperty(plot,field,value); applied.insert(field);
   }
   return applied;
}
QString orbitViewScript(GmatBase &plot)
{
   QtResourcePreview copy(plot.Clone());
   BooleanArray flags;
   const auto names=copy->GetStringArrayParameter("Add");
   for (const auto &name:names) flags.push_back(static_cast<OrbitPlot &>(*copy).GetShowObject(name));
   copy->SetBooleanArrayParameter("DrawObject",flags);
   auto script=QString::fromStdString(copy->GetGeneratingString(Gmat::SCRIPTING));
   if (names.empty()) {
      // Empty lists are the default. Their explicit {} / [] assignments cannot
      // be interpreted as valid object/boolean entries by the base parser.
      const auto name=QRegularExpression::escape(QString::fromStdString(plot.GetName()));
      script.remove(QRegularExpression("^[ \t]*(?:GMAT[ \t]+)?"+name+"\\.(?:Add|DrawObject)[ \t]*=[^;]*;[ \t]*\\n?",QRegularExpression::MultilineOption));
   }
   return script;
}
void validateOrbitViewProperties(GmatBase &plot)
{
   if (!plot.IsOfType("OrbitView")) return;
   if (plot.GetBooleanParameter("ShowPlot") && plot.GetStringArrayParameter("Add").empty()) throw std::runtime_error("Select an object, or turn off Show plot.");
   const auto scale=plot.GetRealParameter("ViewScaleFactor");
   if (!std::isfinite(scale) || scale<=0) throw std::runtime_error("View scale must be greater than zero.");
   for (const auto &field:QStringList{"CoordinateSystem","ViewUpCoordinateSystem"}) {
      auto *object=Moderator::Instance()->GetConfiguredObject(plot.GetStringParameter(field.toStdString()));
      if (!object || !object->IsOfType(Gmat::COORDINATE_SYSTEM)) throw std::runtime_error("Select an available coordinate system.");
   }
   const QStringList types={"ViewPointRefType","ViewPointVectorType","ViewDirectionType"};
   for (int i=0;i<cameraFields.size();++i) {
      const auto value=QString::fromStdString(plot.GetStringParameter(cameraFields[i].toStdString()));
      if (plot.GetStringParameter(types[i].toStdString())=="Vector") {
         vectorParts(value);
      } else {
         auto *object=Moderator::Instance()->GetConfiguredObject(value.toStdString());
         if (!object || !object->IsOfType(Gmat::SPACE_POINT)) throw std::runtime_error("Select an available camera space point.");
      }
   }
}
OrbitViewDialog::OrbitViewDialog(GmatBase &plot,const QMap<QString,QString> &pending,QWidget *parent) : QDialog(parent)
{
   setObjectName("orbitViewDialog"); setWindowTitle("Orbit view — "+QString::fromStdString(plot.GetName()));
   resize(720,std::min(820,QGuiApplication::primaryScreen()->availableGeometry().height()-80));
   QMap<QString,QString> initial; QMap<QString,ResourceProperty> fields;
   for (const auto &field:resourceProperties(plot)) { initial.insert(field.name,field.value); fields.insert(field.name,field); }
   for (auto it=pending.cbegin();it!=pending.cend();++it) initial.insert(it.key(),it.value());
   initialSettings=initial;
   auto snapshot=qtResourcePreviewShared(plot.Clone());
   auto *layout=new QVBoxLayout(this); auto *help=new QLabel("Select plotted objects, camera and drawing settings. OK keeps edits pending; Apply updates the mission.",this); help->setWordWrap(true); layout->addWidget(help);
   auto *scroll=new QScrollArea(this); scroll->setObjectName("orbitViewScroll"); scroll->setWidgetResizable(true); layout->addWidget(scroll,1);
   auto *content=new QWidget(scroll); scroll->setWidget(content); auto *groups=new QVBoxLayout(content);
   auto *objectGroup=new QGroupBox("Objects",content); auto *objectLayout=new QVBoxLayout(objectGroup); groups->addWidget(objectGroup);
   objects=new QListWidget(objectGroup); objects->setObjectName("orbitObjects"); objects->setDragDropMode(QAbstractItemView::InternalMove); objects->setMaximumHeight(140);
   const auto names=splitResourceReferences(initial.value("Add"));
   // Generic Add editing can be pending before this dialog is opened. Preserve
   // visibility by name unless a complete pending positional array was supplied.
   BooleanArray originalFlags; const auto original=plot.GetStringArrayParameter("Add");
   for (const auto &name:original) originalFlags.push_back(static_cast<OrbitPlot &>(plot).GetShowObject(name));
   auto visible=originalFlags;
   try {
      if (initial.value("Add")!=fields.value("Add").value && initial.value("DrawObject")==fields.value("DrawObject").value) throw std::runtime_error("Retain flags by name");
      visible=drawFlags(initial.value("DrawObject"),names.size());
   } catch (const std::exception &) { visible.clear(); for (const auto &name:names) { auto found=std::find(original.begin(),original.end(),name.toStdString()); auto index=static_cast<size_t>(found-original.begin()); visible.push_back(index>=originalFlags.size() || originalFlags[index]); } }
   auto append=[this](const QString &name,bool show) { auto *item=new QListWidgetItem(name,objects); item->setFlags(item->flags()|Qt::ItemIsUserCheckable); item->setCheckState(show ? Qt::Checked : Qt::Unchecked); };
   for (int i=0;i<names.size();++i) append(names[i],visible[i]);
   objectLayout->addWidget(new QLabel("Checked objects are drawn. Uncheck to retain their trajectory without the body/model.",objectGroup)); objectLayout->addWidget(objects);
   auto *actions=new QHBoxLayout; objectLayout->addLayout(actions); auto *available=new QComboBox(objectGroup); available->setObjectName("orbitAvailableObjects");
   for (const auto &name:Moderator::Instance()->GetListOfObjects(Gmat::SPACE_POINT)) available->addItem(QString::fromStdString(name)); actions->addWidget(available,1);
   for (const auto &label:QStringList{"Add","Remove","Clear","Up","Down"}) {
      auto *button=new QPushButton(label,objectGroup); button->setObjectName("orbit"+label+"Object"); actions->addWidget(button);
      connect(button,&QPushButton::clicked,this,[this,label,available,append] {
         auto row=objects->currentRow();
         if (label=="Add") { if (!available->currentText().isEmpty() && objects->findItems(available->currentText(),Qt::MatchExactly).isEmpty()) append(available->currentText(),true); }
         else if (label=="Clear") objects->clear();
         else if (row>=0 && label=="Remove") delete objects->takeItem(row);
         else if (row>=0) { int target=row+(label=="Up" ? -1 : 1); if (target>=0 && target<objects->count()) { auto *item=objects->takeItem(row); objects->insertItem(target,item); objects->setCurrentRow(target); } }
      });
   }
   auto *camera=new QGroupBox("Camera",content); auto *cameraForm=new QFormLayout(camera); groups->addWidget(camera);
   auto choice=[&](const QString &name,const QString &label) { auto *combo=new QComboBox(camera); combo->setObjectName("orbit_"+name); combo->addItems(fields.value(name).choices.isEmpty() ? fields.value(name).references : fields.value(name).choices); if (combo->findText(initial.value(name))<0) combo->addItem(initial.value(name)); combo->setCurrentText(initial.value(name)); choices.insert(name,combo); cameraForm->addRow(label,combo); };
   choice("CoordinateSystem","View coordinate system");
   const QStringList defaults={"[0 0 0]","[0 0 30000]","[0 0 -1]"}; const QStringList labels={"Viewpoint reference","Viewpoint offset","View direction / target"};
   for (int i=0;i<cameraFields.size();++i) {
      const auto name=cameraFields[i]; auto *row=new QWidget(camera); auto *rowLayout=new QHBoxLayout(row); rowLayout->setContentsMargins(0,0,0,0);
      CameraInput input; input.choice=new QComboBox(row); input.choice->setObjectName("orbit_"+name); input.choice->addItem("Vector"); input.choice->addItems(fields.value(name).references); rowLayout->addWidget(input.choice,1);
      QStringList parts; const auto value=initial.value(name); try { parts=vectorParts(value); input.choice->setCurrentText("Vector"); } catch (const std::exception &) { parts=vectorParts(defaults[i]); if (input.choice->findText(value)<0) input.choice->addItem(value); input.choice->setCurrentText(value); }
      for (int j=0;j<3;++j) { input.vector[j]=new QLineEdit(parts[j],row); input.vector[j]->setObjectName("orbit_"+name+QString::number(j+1)); input.vector[j]->setMinimumWidth(65); input.vector[j]->setEnabled(input.choice->currentText()=="Vector"); rowLayout->addWidget(input.vector[j],1); }
      connect(input.choice,&QComboBox::currentTextChanged,this,[input](const QString &text) { for (auto *edit:input.vector) edit->setEnabled(text=="Vector"); }); cameras.insert(name,input); cameraForm->addRow(labels[i],row);
   }
   scale=new QLineEdit(initial.value("ViewScaleFactor"),camera); scale->setObjectName("orbit_ViewScaleFactor"); cameraForm->addRow("View scale factor",scale);
   choice("ViewUpCoordinateSystem","Up coordinate system"); choice("ViewUpAxis","Up axis");
   auto *drawing=new QGroupBox("Drawing and data",content); auto *form=new QFormLayout(drawing); groups->addWidget(drawing);
   const QMap<QString,QString> flagLabels={{"ShowPlot","Show plot"},{"ShowLabels","Show labels"},{"WireFrame","Wireframe"},{"EclipticPlane","Ecliptic plane"},{"XYPlane","XY plane"},{"Axes","Axes"},{"Grid","Grid"},{"SunLine","Sun line"},{"UseInitialView","Reset to script camera on each run"},{"EnableStars","Stars"},{"EnableConstellations","Constellations"}};
   for (auto it=flagLabels.cbegin();it!=flagLabels.cend();++it) { auto *check=new QCheckBox(it.value(),drawing); check->setObjectName("orbit_"+it.key()); check->setChecked(initial.value(it.key())=="true" || initial.value(it.key())=="On"); flags.insert(it.key(),check); form->addRow(check); }
   auto *solver=new QComboBox(drawing); solver->setObjectName("orbit_SolverIterations"); solver->addItems(fields.value("SolverIterations").choices); if (solver->findText("LastN")<0) solver->addItem("LastN"); solver->setCurrentText(initial.value("SolverIterations")); choices.insert("SolverIterations",solver); form->addRow("Solver iterations",solver);
   solverLastN=new QSpinBox(drawing); solverLastN->setObjectName("orbit_SolverIterLastN"); solverLastN->setRange(0,std::numeric_limits<int>::max()); solverLastN->setValue(orbitSolverLastN(initial.value("@QtSolverLastN")).value_or(1));
   solverLastN->setToolTip("LastN retains up to twice N finalized solver trajectories per plotted object. 0 retains all solver trajectories."); form->addRow("LastN count (0 = all)",solverLastN);
   connect(solver,&QComboBox::currentTextChanged,this,[this](const QString &mode) { solverLastN->setEnabled(mode=="LastN"); }); solverLastN->setEnabled(solver->currentText()=="LastN");
   const QMap<QString,QString> countLabels={{"DataCollectFrequency","Collect every N samples"},{"UpdatePlotFrequency","Update every N samples"},{"MaxPlotPoints","Maximum retained points"},{"NumPointsToRedraw","Recent segments to redraw (0 = all)"},{"StarCount","Star count"}};
   for (auto it=countLabels.cbegin();it!=countLabels.cend();++it) { auto *spin=new QSpinBox(drawing); spin->setObjectName("orbit_"+it.key()); spin->setRange(it.key()=="NumPointsToRedraw" ? 0 : 1,std::numeric_limits<int>::max()); spin->setValue(initial.value(it.key()).toInt()); counts.insert(it.key(),spin); form->addRow(it.value(),spin); }
   connect(flags.value("EnableStars"),&QCheckBox::toggled,counts.value("StarCount"),&QSpinBox::setEnabled); counts.value("StarCount")->setEnabled(flags.value("EnableStars")->isChecked());
   groups->addStretch(); error=new QLabel(this); error->setObjectName("orbitViewError"); error->setWordWrap(true); layout->addWidget(error);
   for (auto *edit:findChildren<QLineEdit *>()) connect(edit,&QLineEdit::textChanged,error,&QLabel::clear);
   for (auto *combo:findChildren<QComboBox *>()) connect(combo,&QComboBox::currentTextChanged,error,&QLabel::clear);
   connect(objects,&QListWidget::itemChanged,error,[this] { error->clear(); });
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons); connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[this,snapshot] {
      try { const auto values=settings(); QtResourcePreview candidate(snapshot->Clone()); auto applied=applyOrbitViewProperties(*candidate,values); for (auto it=values.cbegin();it!=values.cend();++it) if (!applied.contains(it.key())) setResourceProperty(*candidate,it.key(),it.value()); validateOrbitViewProperties(*candidate); accept(); }
      catch (BaseException &exception) { error->setText(QString::fromStdString(exception.GetFullMessage())); }
      catch (const std::exception &exception) { error->setText(QString::fromUtf8(exception.what())); }
   });
}
QMap<QString,QString> OrbitViewDialog::settings() const
{
   QMap<QString,QString> result; QStringList names,draw;
   for (int i=0;i<objects->count();++i) { names.append(objects->item(i)->text()); draw.append(objects->item(i)->checkState()==Qt::Checked ? "true" : "false"); } result.insert("Add",names.join(", ")); result.insert("DrawObject",draw.join(" "));
   for (auto it=cameras.cbegin();it!=cameras.cend();++it) {
      QStringList parts; for (auto *edit:it->vector) parts.append(edit->text().trimmed());
      QString value=it->choice->currentText()=="Vector" ? "["+parts.join(" ")+"]" : it->choice->currentText();
      // Formatting an unchanged vector must not count as an explicit camera
      // edit and discard imported primary-camera behavior in the parent Apply.
      try {
         const auto before=vectorParts(initialSettings.value(it.key())),after=vectorParts(value);
         bool equal=true; for (int i=0;i<3;++i) equal=equal && before[i].toDouble()==after[i].toDouble();
         if (equal) value=initialSettings.value(it.key());
      } catch (const std::exception &) {} // Object selections or input awaiting validation.
      result.insert(it.key(),value);
   }
   for (auto it=choices.cbegin();it!=choices.cend();++it) result.insert(it.key(),it.value()->currentText());
   result.insert("@QtSolverLastN",choices.value("SolverIterations")->currentText()=="LastN" ? QString::number(solverLastN->value()) : QString());
   for (auto it=flags.cbegin();it!=flags.cend();++it) { const bool boolean=it.key()=="ShowPlot" || it.key()=="ShowLabels"; result.insert(it.key(),it.value()->isChecked() ? (boolean ? "true" : "On") : (boolean ? "false" : "Off")); }
   for (auto it=counts.cbegin();it!=counts.cend();++it) result.insert(it.key(),QString::number(it.value()->value())); result.insert("ViewScaleFactor",scale->text().trimmed()); return result;
}
