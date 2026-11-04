#pragma once
// Additional specialist input inventory. No mission, Apply, or shown window.
#include "CelestialBodyPanel.hpp"
#include "CelestialBody.hpp"
#include "SolarSystem.hpp"
#include "PropSetup.hpp"
#include "ODEModel.hpp"
#include "AtmosphereDialog.hpp"
#include "GravityDialog.hpp"
#include "ExternalForceDialog.hpp"
#include "ReportParameterDialog.hpp"
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QFormLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#ifdef Q_OS_LINUX
#include <link.h>
#endif

namespace NestedInputCatalog {
inline QJsonArray controls(QWidget &owner,const QStringList &prefixes)
{
   QJsonArray result;
   for (auto *control:owner.findChildren<QWidget *>()) {
      const auto name=control->objectName();
      bool selected=false; for (const auto &prefix:prefixes) selected=selected || name.startsWith(prefix);
      if (!selected || (!qobject_cast<QLineEdit *>(control) && !qobject_cast<QComboBox *>(control) &&
          !qobject_cast<QListWidget *>(control) && !qobject_cast<QSpinBox *>(control) &&
          !qobject_cast<QCheckBox *>(control) && !(qobject_cast<QGroupBox *>(control) && qobject_cast<QGroupBox *>(control)->isCheckable()))) continue;
      QString label; QJsonArray parentLabels;
      for (auto *form:owner.findChildren<QFormLayout *>()) if (auto *text=qobject_cast<QLabel *>(form->labelForField(control))) label=text->text();
      if (control->parentWidget()!=&owner) for (auto *text:control->parentWidget()->findChildren<QLabel *>(QString(),Qt::FindDirectChildrenOnly)) if (!text->text().isEmpty()) parentLabels.append(text->text());
      QJsonObject row{{"object_name",name},{"class",control->metaObject()->className()},
         {"label",label},{"parent_labels",parentLabels},{"enabled",control->isEnabled()},{"explicitly_hidden",control->isHidden()}};
      if (auto *combo=qobject_cast<QComboBox *>(control)) {
         QJsonArray choices; for (int i=0;i<combo->count();++i) choices.append(combo->itemText(i));
         row["choices"]=choices; row["editable"]=combo->isEditable(); row["value"]=combo->currentText();
      } else if (auto *line=qobject_cast<QLineEdit *>(control)) { row["value"]=line->text(); row["read_only"]=line->isReadOnly(); }
      else if (auto *spin=qobject_cast<QSpinBox *>(control)) row["value"]=spin->value();
      else if (auto *box=qobject_cast<QCheckBox *>(control)) row["value"]=box->isChecked();
      else if (auto *box=qobject_cast<QGroupBox *>(control)) row["value"]=box->isChecked();
      result.append(row);
   }
   if (result.isEmpty()) throw std::runtime_error("Specialist editor supplied no named input controls");
   return result;
}
inline QJsonArray loadedImages()
{
   QStringList paths{QCoreApplication::applicationFilePath()};
#ifdef Q_OS_LINUX
   dl_iterate_phdr([](dl_phdr_info *info,size_t,void *data) {
      const auto path=QString::fromLocal8Bit(info->dlpi_name);
      if (QFileInfo(path).isAbsolute()) static_cast<QStringList *>(data)->append(path);
      return 0;
   },&paths);
#endif
   paths.removeDuplicates(); paths.sort(); QJsonArray result;
   for (const auto &path:paths) {
      const auto canonical=QFileInfo(path).canonicalFilePath(); QFile file(canonical);
      if (canonical.isEmpty() || !file.open(QIODevice::ReadOnly)) throw std::runtime_error("Cannot identify loaded file image");
      QCryptographicHash hash(QCryptographicHash::Sha256);
      if (!hash.addData(&file)) throw std::runtime_error("Cannot hash loaded file image");
      result.append(QJsonObject{{"canonical_path",canonical},{"sha256",QString::fromLatin1(hash.result().toHex())}});
   }
   return result;
}
inline void populate(QJsonObject &report,Moderator &moderator)
{
   const auto before=moderator.GetListOfObjects(Gmat::SPACECRAFT); QJsonArray scenes;
   auto capture=[&](const QString &name,auto build) {
      QJsonObject scene{{"name",name}};
      try { build(scene); scene["status"]="inventoried"; }
      catch (BaseException &error) { scene["status"]="rejected"; scene["error"]=QString::fromStdString(error.GetFullMessage()); }
      catch (const std::exception &error) { scene["status"]="rejected"; scene["error"]=error.what(); }
      scenes.append(scene);
   };
   for (const auto &name:QStringList{"Earth","Luna","Mars","Sun"}) capture("body:"+name,[&](QJsonObject &scene) {
      auto *body=moderator.GetSolarSystemInUse()->GetBody(name.toStdString());
      if (!body) throw std::runtime_error("Configured built-in body unavailable");
      scene["documentation_type"]="CelestialBody"; scene["native_type"]=QString::fromStdString(body->GetTypeName());
      QJsonArray descriptors;
      for (const auto &f:celestialBodyFields(*body)) descriptors.append(QJsonObject{{"field",f.name},{"label",f.label},
         {"parameter_type",QString::fromStdString(body->GetParameterTypeString(body->GetParameterID(f.name.toStdString())))},
         {"native_unit",QString::fromStdString(body->GetParameterUnit(body->GetParameterID(f.name.toStdString())))},
         {"descriptor_read_only",f.readOnly}});
      scene["fields"]=descriptors;
      CelestialBodyPanel panel(*body,[](const QMap<QString,QString>&){return QString();});
      scene["controls"]=controls(panel,{"body_"});
   });
   auto setup=resourceDraft("PropSetup","NestedUnitSetup"); auto *prop=dynamic_cast<PropSetup *>(setup.get());
   if (!prop || !prop->GetODEModel()) throw std::runtime_error("Default Earth force-model draft unavailable");
   auto *model=prop->GetODEModel();
   for (const auto &atmosphere:QStringList{"None","JacchiaRoberts"}) capture("atmosphere:"+atmosphere,[&](QJsonObject &scene) {
      AtmosphereDialog dialog(*model,{{"Drag.AtmosphereModel",atmosphere}});
      scene["documentation_type"]="ForceModel"; scene["controls"]=controls(dialog,{"atmosphere_"});
   });
   capture("gravity:Earth",[&](QJsonObject &scene) {
      GravityDialog dialog(*model,{}); scene["documentation_type"]="ForceModel"; scene["controls"]=controls(dialog,{"gravity_"});
   });
   capture("external:provider",[&](QJsonObject &scene) {
      ExternalForceDialog dialog(*model,{}); scene["documentation_type"]="ForceModel"; scene["controls"]=controls(dialog,{"externalForce"});
   });
   for (const auto &mode:QList<QPair<QString,ReportParameterDialog::Mode>>{
      {"Multiple",ReportParameterDialog::Mode::Multiple},{"NumericSingle",ReportParameterDialog::Mode::NumericSingle},
      {"FunctionInputs",ReportParameterDialog::Mode::FunctionInputs}}) capture("picker:"+mode.first,[&](QJsonObject &scene) {
      ReportParameterDialog dialog(mode.first=="NumericSingle" ? QStringList{"1"} : QStringList{},nullptr,mode.second);
      scene["controls"]=controls(dialog,{"reportParameterEntry","reportProperty","reportArray","reportSelected"});
   });
   if (moderator.GetListOfObjects(Gmat::SPACECRAFT)!=before) throw std::runtime_error("Nested inventory registered a spacecraft");
   int rejected=0; for (const auto &s:scenes) rejected+=s.toObject()["status"]!="inventoried";
   report["nested_input_scenes"]=scenes; report["rejected_scenes"]=rejected; report["inventoried_scenes"]=scenes.size()-rejected;
   report["loaded_file_images"]=loadedImages();
#ifdef Q_OS_LINUX
   report["loaded_file_image_set_measured"]=true;
#else
   report["loaded_file_image_set_measured"]=false;
#endif
   report["shown_windows"]=0;
}
}
