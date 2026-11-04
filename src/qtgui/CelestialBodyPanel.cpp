#include "CelestialBodyPanel.hpp"
#include "CelestialBody.hpp"
#include "DateUtil.hpp"
#include "SolarSystem.hpp"
#ifdef __USE_SPICE__
#include "SpiceInterface.hpp"
#endif
#include "Moderator.hpp"
#include "FileManager.hpp"
#include "ResourceProperties.hpp"
#include "UserParameter.hpp"
#include "InspectionDialog.hpp"
#include "OrbitRenderer.hpp"
#include "RgbColor.hpp"
#include <QTabWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QDialog>
#include <QFileDialog>
#include <QColorDialog>
#include <QImageReader>
#include <QPixmap>
#include <QFileInfo>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace {
QString defaultRotationSource(const CelestialBody &body)
{
   return QString::fromStdString(Gmat::ROTATION_DATA_SOURCE_STRINGS[
      body.GetName()=="Luna" ? Gmat::DE_FILE : Gmat::IAU_SIMPLIFIED]);
}
}
QVector<BodyField> celestialBodyFields(CelestialBody &body)
{
   QVector<BodyField> fields;
   auto add=[&](const QString &section,const QString &name,const QString &label,bool readOnly=false) {
      BodyField field; field.name=name; field.label=label; field.section=section;
      const auto id=body.GetParameterID(name.toStdString()); field.type=body.GetParameterType(id);
      field.readOnly=readOnly || body.IsParameterReadOnly(id);
      if (field.type==Gmat::REAL_TYPE) field.value=QString::number(body.GetRealParameter(id),'g',17);
      else if (field.type==Gmat::INTEGER_TYPE) field.value=QString::number(body.GetIntegerParameter(id));
      else if (field.type==Gmat::STRINGARRAY_TYPE) { for (const auto &path:body.GetStringArrayParameter(id)) { if (!field.value.isEmpty()) field.value+='\n'; field.value+=QString::fromStdString(path); } field.files=true; }
      else field.value=QString::fromStdString(body.GetStringParameter(id));
      field.filename=field.type==Gmat::FILENAME_TYPE;
      if (name=="PosVelSource") for (const auto &choice:body.GetEphemSourceList()) field.choices.append(QString::fromStdString(choice));
      fields.append(field);
   };
   add("Properties","Mu","Gravitational parameter (km³/s²)"); add("Properties","EquatorialRadius","Equatorial radius (km)"); add("Properties","Flattening","Flattening");
   const auto sources=body.GetEphemSourceList();
   const bool kernels=std::find(sources.begin(),sources.end(),"SPICE")!=sources.end();
   if (kernels) add("Properties","PlanetarySpiceKernelName","Planetary PCK files");
   add("Orbit","PosVelSource","Ephemeris source",!body.IsUserDefined()); add("Orbit","CentralBody","Central body",true);
   if (kernels) { add("Orbit","NAIFId","NAIF ID"); add("Orbit","OrbitSpiceKernelName","Orbit SPK files"); }
   add("Orientation","RotationDataSource","Rotation source",!(kernels && (body.IsUserDefined() || body.GetName()=="Luna")));
   add("Orientation","OrientationEpoch","Reference epoch (A1ModJulian)",!body.IsUserDefined());
   if (body.GetName()=="Earth") add("Orientation","NutationUpdateInterval","Nutation update interval (s)");
   for (const auto &entry:QList<QPair<QString,QString>>{{"SpinAxisRAConstant","Pole right ascension (deg)"},{"SpinAxisRARate","RA rate (deg/century)"},{"SpinAxisDECConstant","Pole declination (deg)"},{"SpinAxisDECRate","DEC rate (deg/century)"},{"RotationConstant","Prime meridian (deg)"},{"RotationRate","Rotation rate (deg/day)"}}) add("Orientation",entry.first,entry.second,!body.IsUserDefined());
   if (kernels) { add("Orientation","SpiceFrameId","SPICE frame ID"); add("Orientation","FrameSpiceKernelName","Frame FK files"); }
   add("Visualization","TextureMapFileName","Texture map"); add("Visualization","3DModelFile","3D model");
   for (const auto &axis:QStringList{"X","Y","Z"}) add("Visualization","3DModelOffset"+axis,"Model offset "+axis);
   for (const auto &axis:QStringList{"X","Y","Z"}) add("Visualization","3DModelRotation"+axis,"Model rotation "+axis+" (deg)");
   add("Visualization","3DModelScale","Model scale"); add("Visualization","OrbitColor","Orbit color"); add("Visualization","TargetColor","Target color");
   return fields;
}
namespace {
QString quotedFile(const QString &value)
{
   for (const auto ch:value) if (ch.unicode()<32 || ch==QChar(0x2028) || ch==QChar(0x2029) || ch==QChar(39) || ch=='%') throw std::runtime_error("Use a filename or frame ID without apostrophes, percent signs or control characters.");
   return "'"+value+"'";
}
QString assetPath(const QString &name,const QString &field,const QString &value)
{
   std::string file,path; auto *fm=FileManager::Instance();
   const bool ok=field=="TextureMapFileName" ? fm->GetTextureMapFile(value.toStdString(),name.toStdString(),name.toStdString(),file,path,false) : fm->GetBody3dModelFile(value.toStdString(),name.toStdString(),name.toStdString(),file,path,false);
   if (!ok || (path.empty() && !value.isEmpty())) throw std::runtime_error("The selected appearance file cannot be found.");
   return QString::fromStdString(path);
}
}
QString celestialBodyScript(CelestialBody &body,const QString &source,const QMap<QString,QString> &changes,const QString &firstMissionStatement)
{
   QMap<QString,BodyField> fields; for (const auto &field:celestialBodyFields(body)) fields.insert(field.name,field);
   const auto rotation=changes.value("RotationDataSource",QString::fromStdString(body.GetStringParameter(body.GetParameterID("RotationDataSource"))));
   if (rotation=="SPICE" && changes.value("SpiceFrameId",QString::fromStdString(body.GetStringParameter(body.GetParameterID("SpiceFrameId")))).trimmed().isEmpty())
      throw std::runtime_error("Choose a SPICE frame ID before applying SPICE rotation.");
   QString block; QStringList replaced;
   // Iterate in page order: source/NAIF before files, independent real values,
   // appearance files before their pose. No CelestialBody clone is created:
   // its destructor can unload kernels also used by the running engine.
   for (const auto &field:celestialBodyFields(body)) {
      if (!changes.contains(field.name)) continue;
      if (field.readOnly) throw std::runtime_error(("This body setting is read-only: "+field.name).toStdString());
      const auto value=changes.value(field.name); QString literal;
      if (field.name=="RotationDataSource") {
         const auto fallback=defaultRotationSource(body);
         if (value!=fallback && value!="SPICE") throw std::runtime_error("Choose a rotation source available for this body.");
         // Rebuilding without an override restores the factory source. Avoid
         // asking the engine's legacy setter to select a default it ignores.
         if (value==fallback) { replaced.append(field.name); continue; }
         literal=value;
      } else if (field.type==Gmat::REAL_TYPE) {
         bool ok=false; const auto number=value.toDouble(&ok); if (!ok || !std::isfinite(number)) throw std::runtime_error(("Enter a finite number for "+field.label+".").toStdString());
         if (field.name=="OrientationEpoch" && (number<DateUtil::EARLIEST_VALID_MJD_VALUE || number>DateUtil::LATEST_VALID_MJD_VALUE))
            throw std::runtime_error("Orientation epoch must be between 6116.0 and 58127.5 A1ModJulian, inclusive.");
         if ((field.name=="Mu" || field.name=="EquatorialRadius") && number<=0) throw std::runtime_error((field.label+" must be greater than zero.").toStdString());
         if ((field.name=="Flattening" || field.name=="NutationUpdateInterval") && number<0) throw std::runtime_error((field.label+" must be zero or greater.").toStdString());
         if (field.name.startsWith("3DModelOffset") && (number< -3.5 || number>3.5)) throw std::runtime_error("Model offsets must be between -3.5 and 3.5.");
         if (field.name.startsWith("3DModelRotation") && (number< -180 || number>180)) throw std::runtime_error("Model rotations must be between -180 and 180 degrees.");
         if (field.name=="3DModelScale" && (number<0.001 || number>1000)) throw std::runtime_error("Model scale must be between 0.001 and 1000.");
         literal=QString::number(number,'g',17);
      } else if (field.type==Gmat::INTEGER_TYPE) {
         bool ok=false; const auto number=value.toInt(&ok); if (!ok) throw std::runtime_error("Enter a signed integer NAIF ID."); literal=QString::number(number);
      } else if (field.files) {
         QStringList entries,seen;
         for (const auto &path:value.split('\n',Qt::SkipEmptyParts)) {
            const QFileInfo file(path); if (!file.isFile() || !file.isReadable()) throw std::runtime_error(("Choose an existing readable kernel: "+path).toStdString());
#ifdef __USE_SPICE__
            const auto kind=field.name=="OrbitSpiceKernelName" ? "SPK" : field.name=="FrameSpiceKernelName" ? "FK" : "PCK";
            if (!SpiceInterface::IsValidKernel(file.absoluteFilePath().toStdString(),kind))
               throw std::runtime_error(("Choose a valid "+QString(kind)+" kernel: "+path).toStdString());
#endif
            const auto identity=file.canonicalFilePath(); if (seen.contains(identity)) throw std::runtime_error("Choose each kernel only once."); seen.append(identity); entries.append(quotedFile(path));
         }
         literal="{"+entries.join(", ")+"}";
         // Kernel setters append. Clear startup/default membership before
         // writing the complete pending list, including removal of defaults.
         if (!entries.isEmpty()) block+="GMAT "+QString::fromStdString(body.GetName())+"."+field.name+" = {};\n";
      } else if (field.filename) {
         literal=quotedFile(value);
         if (!value.isEmpty()) {
            const auto path=assetPath(QString::fromStdString(body.GetName()),field.name,value);
            if (field.name=="TextureMapFileName" && QImageReader(path).read().isNull()) throw std::runtime_error("Choose a readable texture image.");
            if (field.name=="3DModelFile" && !orbitModelFileReadable(path)) throw std::runtime_error("Choose a readable 3DS or OBJ model supported by the viewer.");
         }
      } else if (field.type==Gmat::COLOR_TYPE) {
         auto color=RgbColor(RgbColor::ToIntColor(value.toStdString())); literal=QString("[%1 %2 %3]").arg(color.Red()).arg(color.Green()).arg(color.Blue());
      } else if (!field.choices.isEmpty()) {
         if (!field.choices.contains(value)) throw std::runtime_error("Choose a source available for this body."); literal=value;
      } else literal=quotedFile(value);
      block+="GMAT "+QString::fromStdString(body.GetName())+"."+field.name+" = "+literal+";\n"; replaced.append(field.name);
   }
   for (auto it=changes.cbegin();it!=changes.cend();++it) if (!fields.contains(it.key())) throw std::runtime_error(("Use script settings for this body property: "+it.key()).toStdString());
   return setConfigurationBlock(source,QString::fromStdString(body.GetName()),replaced,block,firstMissionStatement);
}
QString celestialBodySettingsError(CelestialBody &body,const QMap<QString,QString> &changes)
{
   for (const auto &field:celestialBodyFields(body)) {
      if (!changes.contains(field.name)) continue; const auto expected=changes.value(field.name);
      bool equal=field.value==expected;
      if (field.type==Gmat::REAL_TYPE) equal=field.value.toDouble()==expected.toDouble();
      else if (field.type==Gmat::INTEGER_TYPE) equal=field.value.toInt()==expected.toInt();
      else if (field.type==Gmat::COLOR_TYPE) equal=RgbColor::ToIntColor(field.value.toStdString())==RgbColor::ToIntColor(expected.toStdString());
      else if (field.filename) {
         // Empty texture restores the engine's per-body/default texture; empty
         // model clears it. Resolve the applied engine filename and full path.
         if (expected.isEmpty()) equal=field.name=="TextureMapFileName" || field.value.isEmpty();
         else equal=QFileInfo(assetPath(QString::fromStdString(body.GetName()),field.name,expected)).canonicalFilePath()==QFileInfo(QString::fromStdString(body.GetStringParameter(body.GetParameterID(field.name=="TextureMapFileName" ? "TextureMapFullPath" : "3DModelFileFullPath")))).canonicalFilePath();
      }
      if (!equal) return "GMAT did not retain "+field.label+". The previous configuration was restored.";
   }
   return {};
}

CelestialBodyPanel::~CelestialBodyPanel()
{
   // QListWidget emits modelReset while QWidget tears down children, after
   // this class's settings getters have been destroyed. Retire callbacks now.
   for (auto *child:findChildren<QObject *>())
      QObject::disconnect(child,nullptr,this,nullptr);
}

CelestialBodyPanel::CelestialBodyPanel(CelestialBody &body,Apply apply,QWidget *parent) : EditablePanel(parent)
{
   setObjectName("celestialBodyPanel"); const auto name=QString::fromStdString(body.GetName()); auto *layout=new QVBoxLayout(this);
   layout->addWidget(new QLabel(name+" — "+QString::fromStdString(body.GetTypeName()),this));
   auto *tabs=new QTabWidget(this); tabs->setObjectName("bodyPages"); layout->addWidget(tabs,1); QMap<QString,QFormLayout *> forms;
   for (const auto &section:QStringList{"Properties","Orbit","Orientation","Visualization"}) {
      auto *scroll=new QScrollArea(tabs); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame); auto *page=new QWidget(scroll); scroll->setWidget(page); forms[section]=new QFormLayout(page); forms[section]->setRowWrapPolicy(QFormLayout::WrapLongRows); forms[section]->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow); tabs->addTab(scroll,section);
   }
   if (!body.IsUserDefined()) { auto *note=new QLabel("Ephemeris source and files for built-in bodies are selected in SolarSystem. Built-in pole and rotation constants are read-only.",this); note->setWordWrap(true); layout->addWidget(note); }
   auto *sourceFile=new QLineEdit(QString::fromStdString(body.GetSourceFileName()),this); sourceFile->setReadOnly(true); sourceFile->setObjectName("bodyEphemerisFile");
   auto *texture=new QLabel(this); texture->setObjectName("bodyTexturePreview"); texture->setAlignment(Qt::AlignCenter); texture->setMinimumHeight(100); texture->setMaximumHeight(140);
   for (const auto &field:celestialBodyFields(body)) {
      original[field.name]=field.value; auto *form=forms[field.section];
      if (field.files) {
         auto *row=new QWidget(this); auto *box=new QVBoxLayout(row); box->setContentsMargins(0,0,0,0); auto *list=new QListWidget(row); list->setObjectName("body_"+field.name); list->addItems(field.value.split('\n',Qt::SkipEmptyParts)); list->setMinimumHeight(80); list->setMaximumHeight(140); list->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded); list->setSelectionMode(QAbstractItemView::ExtendedSelection); box->addWidget(list);
         values[field.name]=[list] { QStringList paths; for (int i=0;i<list->count();++i) paths.append(list->item(i)->text()); return paths.join('\n'); };
         auto *actions=new QHBoxLayout; box->addLayout(actions);
         for (const auto &action:QStringList{"Add","Replace","Remove","Up","Down"}) {
            auto *button=new QPushButton(action,row); button->setObjectName("body"+action+"_"+field.name); actions->addWidget(button);
            connect(button,&QPushButton::clicked,this,[this,list,action] {
               if (action=="Add") { const auto paths=QFileDialog::getOpenFileNames(this,"Add kernel files"); for (const auto &path:paths) if (list->findItems(path,Qt::MatchExactly).isEmpty()) list->addItem(path); }
               else if (action=="Replace") { if (list->currentRow()<0) return; const auto path=QFileDialog::getOpenFileName(this,"Replace kernel",list->currentItem()->text()); if (!path.isEmpty()) list->currentItem()->setText(path); }
               else if (action=="Remove") { for (auto *item:list->selectedItems()) delete list->takeItem(list->row(item)); }
               else { const int index=list->currentRow(),target=index+(action=="Up" ? -1 : 1); if (index<0 || target<0 || target>=list->count()) return; auto *item=list->takeItem(index); list->insertItem(target,item); list->setCurrentItem(item); }
            });
         }
         form->addRow(field.label,row);
      } else if (!field.choices.isEmpty()) {
         auto *combo=new QComboBox(this); combo->setObjectName("body_"+field.name); combo->addItems(field.choices); combo->setCurrentText(field.value); combo->setEnabled(!field.readOnly); form->addRow(field.label,combo); values[field.name]=[combo] { return combo->currentText(); };
         if (field.name=="PosVelSource") form->addRow("Ephemeris file",sourceFile);
      } else {
         auto *edit=new QLineEdit(field.value,this); edit->setObjectName("body_"+field.name); edit->setReadOnly(field.readOnly || field.name=="RotationDataSource"); values[field.name]=[edit] { return edit->text(); };
         if (field.name=="RotationDataSource" && !field.readOnly) {
            auto *row=new QWidget(this); auto *box=new QHBoxLayout(row); box->setContentsMargins(0,0,0,0); box->addWidget(edit,1);
            auto *choose=new QPushButton("Choose…",row); choose->setObjectName("bodyChoose_RotationDataSource"); box->addWidget(choose); form->addRow(field.label,row);
            const auto fallback=defaultRotationSource(body);
            connect(choose,&QPushButton::clicked,this,[this,edit,fallback] {
               QDialog dialog(this); dialog.setObjectName("bodyRotationSourceDialog"); dialog.setWindowTitle("Body rotation source");
               auto *layout=new QVBoxLayout(&dialog); auto *choice=new QComboBox(&dialog); choice->setObjectName("bodyRotationSourceChoice"); choice->addItems({fallback,"SPICE"}); choice->setCurrentText(edit->text()); layout->addWidget(choice);
               auto *note=new QLabel("SPICE uses the selected frame and kernels. The body default uses its original rotation model. Apply commits the selection.",&dialog); note->setWordWrap(true); layout->addWidget(note);
               auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog); layout->addWidget(buttons); connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
               if (dialog.exec()==QDialog::Accepted) edit->setText(choice->currentText());
            });
         } else if (field.filename || field.type==Gmat::COLOR_TYPE) {
            auto *row=new QWidget(this); auto *box=new QHBoxLayout(row); box->setContentsMargins(0,0,0,0); box->addWidget(edit,1); auto *button=new QPushButton(field.filename ? "Browse…" : "Choose…",row); button->setObjectName("bodyChoose_"+field.name); box->addWidget(button); form->addRow(field.label,row);
            connect(button,&QPushButton::clicked,this,[this,edit,field] {
               if (field.filename) { const auto path=QFileDialog::getOpenFileName(this,"Choose "+field.label,edit->text()); if (!path.isEmpty()) edit->setText(path); }
               else { QColor initial; try { initial=QColor::fromRgb(RgbColor::ToIntColor(edit->text().toStdString())&0xffffff); } catch (...) {} const auto color=QColorDialog::getColor(initial,this); if (color.isValid()) edit->setText(QString("[%1 %2 %3]").arg(color.red()).arg(color.green()).arg(color.blue())); }
            });
         } else form->addRow(field.label,edit);
      }
      if (field.name=="TextureMapFileName") form->addRow("Texture preview",texture);
   }
   auto *textureField=findChild<QLineEdit *>("body_TextureMapFileName");
   const auto preview=[texture,textureField,name] { try { const auto path=assetPath(name,"TextureMapFileName",textureField->text()); const QPixmap pixmap(path); if (pixmap.isNull()) { texture->setText("No readable texture preview"); texture->setPixmap({}); } else { texture->setText({}); texture->setPixmap(pixmap.scaled(340,130,Qt::KeepAspectRatio,Qt::SmoothTransformation)); } } catch (...) { texture->setPixmap({}); texture->setText("Texture file cannot be found"); } };
   connect(textureField,&QLineEdit::textChanged,this,[preview] { preview(); }); preview();
   auto *source=findChild<QComboBox *>("body_PosVelSource");
   const auto dependencies=[this,source] { const bool spice=source->currentText()=="SPICE"; if (auto *id=findChild<QLineEdit *>("body_NAIFId")) id->setReadOnly(!spice); if (auto *files=findChild<QListWidget *>("body_OrbitSpiceKernelName")) files->parentWidget()->setEnabled(spice); };
   connect(source,&QComboBox::currentTextChanged,this,[dependencies] { dependencies(); }); dependencies();
   if (auto *rotation=findChild<QLineEdit *>("body_RotationDataSource")) {
      const bool custom=body.IsUserDefined(); const auto active=[this,rotation,custom] {
         for (const auto &key:QStringList{"OrientationEpoch","SpinAxisRAConstant","SpinAxisRARate","SpinAxisDECConstant","SpinAxisDECRate","RotationConstant","RotationRate"})
            if (auto *field=findChild<QLineEdit *>("body_"+key)) field->setReadOnly(!custom || rotation->text()=="SPICE");
      };
      connect(rotation,&QLineEdit::textChanged,this,[active] { active(); }); active();
   }
   status=new QLabel(this); status->setObjectName("bodyStatus"); status->setWordWrap(true); layout->addWidget(status);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Apply|QDialogButtonBox::Close,this); layout->addWidget(buttons);
   const auto script=QString::fromStdString(body.GetGeneratingString(Gmat::SHOW_SCRIPT)); auto *show=new QPushButton("Show script…",this); show->setObjectName("showScript"); buttons->addButton(show,QDialogButtonBox::ActionRole); connect(show,&QPushButton::clicked,this,[this,script] { InspectionDialog dialog("Celestial-body script",script,"Applied settings. Pending edits are not included.",this); dialog.exec(); });
   connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,this,[this,apply] { QMap<QString,QString> changed; const auto pending=settings(); for (auto it=pending.cbegin();it!=pending.cend();++it) if (it.value()!=original.value(it.key())) changed.insert(it.key(),it.value()); if (changed.isEmpty()) { status->setText("No changes to apply."); return; } const auto error=apply(changed); if (error.isEmpty()) { applied=true; appliedSuccessfully(); } else status->setText(error); });
   connect(buttons->button(QDialogButtonBox::Close),&QPushButton::clicked,this,[this] { parentWidget()->close(); });
   // Match GmatPanel::EnableUpdate: Apply follows the complete pending model,
   // including chooser-driven edits and kernel-list insert/remove/reorder.
   auto *applyButton=buttons->button(QDialogButtonBox::Apply);
   const auto updateApply=[this,applyButton] { applyButton->setEnabled(hasChanges()); };
   for (auto *edit:findChildren<QLineEdit *>())
      connect(edit,&QLineEdit::textChanged,this,updateApply);
   for (auto *choice:findChildren<QComboBox *>())
      connect(choice,&QComboBox::currentTextChanged,this,updateApply);
   for (auto *list:findChildren<QListWidget *>()) {
      auto *model=list->model();
      connect(model,&QAbstractItemModel::dataChanged,this,updateApply);
      connect(model,&QAbstractItemModel::rowsInserted,this,updateApply);
      connect(model,&QAbstractItemModel::rowsRemoved,this,updateApply);
      connect(model,&QAbstractItemModel::rowsMoved,this,updateApply);
      connect(model,&QAbstractItemModel::modelReset,this,updateApply);
   }
   updateApply();
}
QMap<QString,QString> CelestialBodyPanel::settings() const { QMap<QString,QString> result; for (auto it=values.cbegin();it!=values.cend();++it) result.insert(it.key(),it.value()()); return result; }
bool CelestialBodyPanel::hasChanges() const { return !applied && settings()!=original; }
