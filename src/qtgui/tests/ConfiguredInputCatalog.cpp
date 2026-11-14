// Specialist-editor input inventory; no mission, shown window or acceptance replay.
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "ResourceDraft.hpp"
#include "ResourceProperties.hpp"
#include "ResourceEditor.hpp"
#include "AttitudeDialog.hpp"
#include "SpacecraftOrbit.hpp"
#include "TestSettings.hpp"
#include "GmatBase.hpp"
#include "Spacecraft.hpp"
#include "Attitude.hpp"
#include "TimeSystemConverter.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QTableWidget>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <iostream>
#include <stdexcept>
#ifdef Q_OS_LINUX
#include <link.h>
#endif

namespace {
void require(bool condition,const QString &message)
{ if (!condition) throw std::runtime_error(message.toStdString()); }
QJsonArray names(const StringArray &values)
{ QJsonArray a; for (const auto &v:values) a.append(QString::fromStdString(v)); return a; }
QJsonArray loadedImages()
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
      require(!canonical.isEmpty() && file.open(QIODevice::ReadOnly),"Cannot identify a loaded file image: "+path);
      QCryptographicHash hash(QCryptographicHash::Sha256); require(hash.addData(&file),"Cannot hash loaded file image: "+path);
      result.append(QJsonObject{{"loaded_path",path},{"canonical_path",canonical},{"sha256",QString::fromLatin1(hash.result().toHex())}});
   }
   return result;
}
QJsonArray metadata(GmatBase &object)
{
   QJsonArray result;
   for (int id=0;id<object.GetParameterCount();++id) {
      QJsonObject item{{"id",id}};
      try {
         item["field"]=QString::fromStdString(object.GetParameterText(id));
         item["parameter_type"]=QString::fromStdString(object.GetParameterTypeString(id));
         item["native_unit"]=QString::fromStdString(object.GetParameterUnit(id));
         item["read_only"]=object.IsParameterReadOnly(id); item["visible"]=object.IsParameterVisible(id);
      } catch (BaseException &e) { item["error"]=QString::fromStdString(e.GetFullMessage()); }
      result.append(item);
   }
   return result;
}
QJsonArray attitudeControls(AttitudeDialog &dialog)
{
   QJsonArray result;
   for (auto *control:dialog.findChildren<QWidget *>()) {
      const auto name=control->objectName();
      if (name!="attitudeModel" && !name.startsWith("attitude_")) continue;
      require(qobject_cast<QLineEdit *>(control)||qobject_cast<QComboBox *>(control),"Unexpected attitude input class");
      const auto field=name=="attitudeModel" ? QString("Attitude") : name.mid(9);
      auto *unit=dialog.findChild<QLabel *>("attitudeUnit_"+field);
      QJsonObject item{{"field",field},{"class",control->metaObject()->className()},
         {"object_name",name},{"unit",unit ? unit->text() : QString()},
         {"enabled",control->isEnabled()},{"explicitly_hidden",control->isHidden()}};
      if (auto *combo=qobject_cast<QComboBox *>(control)) {
         QStringList options; for (int i=0;i<combo->count();++i) options.append(combo->itemText(i));
         item["choices"]=QJsonArray::fromStringList(options); item["value"]=combo->currentText();
      } else item["value"]=qobject_cast<QLineEdit *>(control)->text();
      result.append(item);
   }
   require(!result.isEmpty(),"Attitude editor supplied no input controls"); return result;
}
QJsonArray orbitControls(GmatBase &object)
{
   ResourceEditor editor(object,[](const QMap<QString,QString>&){return QString();});
   auto labels=spacecraftOrbitElementNames(object); labels.append({"Epoch","DateFormat","AnomalyType","CoordinateSystem","DisplayStateType"});
   QJsonArray result;
   for (auto *table:editor.findChildren<QTableWidget *>()) {
      if (table->columnCount()!=4 || !table->horizontalHeaderItem(0) || table->horizontalHeaderItem(0)->text()!="Property") continue;
      for (int row=0;row<table->rowCount();++row) {
         const auto field=table->item(row,0)->text(); if (!labels.contains(field)) continue;
         auto *control=table->cellWidget(row,1); const auto *unit=table->item(row,2);
         QJsonObject item{{"field",field},{"unit",unit ? unit->text() : QString()},
            {"row_hidden",table->isRowHidden(row)},{"editor_class",control ? control->metaObject()->className() : "QTableWidgetItem"}};
         if (auto *combo=qobject_cast<QComboBox *>(control)) {
            QStringList options; for (int i=0;i<combo->count();++i) options.append(combo->itemText(i));
            item["choices"]=QJsonArray::fromStringList(options); item["value"]=combo->currentText();
         } else if (auto *line=qobject_cast<QLineEdit *>(control)) item["value"]=line->text();
         else if (table->item(row,1)) item["value"]=table->item(row,1)->text();
         result.append(item);
      }
   }
   require(result.size()==11,"Orbit editor did not expose all six elements and five selectors/epoch fields"); return result;
}
template<class F> QJsonObject capture(QJsonObject state,F action)
{
   try { action(state); state["status"]="inventoried"; }
   catch (BaseException &e) { state["status"]="rejected"; state["error"]=QString::fromStdString(e.GetFullMessage()); }
   catch (const std::exception &e) { state["status"]="rejected"; state["error"]=e.what(); }
   return state;
}
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("ConfiguredInputCatalog");
   try {
      require(argc==4,"Supply startup, configured input mode and output path"); TestSettings settings;
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(),output=QFileInfo(argv[3]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath()); MainWindow window;
      require(window.initialize(startup),"Configured input initialization failed"); auto *moderator=Moderator::Instance();
      const auto spacecraftBefore=moderator->GetListOfObjects(Gmat::SPACECRAFT); const QString mode=argv[2];
      QJsonObject report{{"schema_version",1},{"mode",mode},{"platform",app.platformName()},
         {"mission_executions",0},{"shown_windows",0},{"GUI_behavior_qualified",false},{"all_input_documentation_qualified",false}};
      QJsonArray states;
      if (mode=="--configured-catalog") {
         report["attitude_models"]=names(moderator->GetListOfFactoryItems(Gmat::ATTITUDE));
         report["orientation_representations"]=QJsonArray{"EulerAngles","Quaternion","DirectionCosineMatrix","MRPs"};
         report["rate_representations"]=QJsonArray{"EulerAngleRates","AngularVelocity"};
         report["euler_sequences"]=names(Attitude::GetEulerSequenceStrings());
         report["constraint_types"]=names(Attitude::GetAttitudeConstraintTypes());
         report["epoch_formats"]=names(TimeSystemConverter::Instance()->GetValidTimeRepresentations());
      } else if (mode.startsWith("--attitude=")) {
         const auto model=mode.mid(11); auto sample=resourceDraft("Spacecraft","ConfiguredAttitudeSample");
         applyAttitudeProperties(*sample,{{"Attitude",model}}); auto *attitude=dynamic_cast<Attitude *>(sample->GetOwnedObject(0));
         require(attitude,"Selected attitude model is unavailable"); const bool initial=attitude->SetInitialAttitudeAllowed();
         report["model"]=model; report["initial_attitude_allowed"]=initial;
         const auto orientations=initial ? QStringList{"EulerAngles","Quaternion","DirectionCosineMatrix","MRPs"} : QStringList{QString()};
         const auto rates=initial ? QStringList{"EulerAngleRates","AngularVelocity"} : QStringList{QString()};
         const auto sequences=initial ? names(Attitude::GetEulerSequenceStrings()) : QJsonArray{QString()};
         for (const auto &orientation:orientations) for (const auto &rate:rates) for (const auto sequenceValue:sequences) {
            const auto sequence=sequenceValue.toString(); QJsonObject config{{"model",model},{"orientation",orientation},{"rate",rate},{"euler_sequence",sequence}};
            states.append(capture(config,[&](QJsonObject &state) {
               auto draft=resourceDraft("Spacecraft","ConfiguredAttitudeProbe"); applyAttitudeProperties(*draft,{{"Attitude",model}});
               if (initial) {
                  // A nonsingular orientation permits all twelve Euler sequences.
                  applyAttitudeProperties(*draft,{{"AttitudeDisplayStateType","EulerAngles"},{"AttitudeRateDisplayStateType","AngularVelocity"},
                     {"EulerAngleSequence",sequence},{"EulerAngle1","10"},{"EulerAngle2","20"},{"EulerAngle3","30"}});
                  applyAttitudeProperties(*draft,{{"AttitudeDisplayStateType",orientation},{"AttitudeRateDisplayStateType",rate}});
               }
               auto *owned=draft->GetOwnedObject(0); state["native_type"]=QString::fromStdString(owned->GetTypeName()); state["native_parameters"]=metadata(*owned);
               AttitudeDialog dialog(*draft,{}); state["controls"]=attitudeControls(dialog);
            }));
         }
      } else if (mode=="--orbit") {
         for (bool hyperbolic:{false,true}) for (const auto &representation:QStringList{"Keplerian","ModifiedKeplerian"}) {
            const auto anomalies=hyperbolic ? QStringList{"TA","MA","HA"} : QStringList{"TA","MA","EA"};
            for (const auto &anomaly:anomalies) states.append(capture(QJsonObject{{"orbit_regime",hyperbolic ? "hyperbolic" : "elliptic"},{"representation",representation},{"anomaly",anomaly}},[&](QJsonObject &state) {
               auto draft=resourceDraft("Spacecraft","ConfiguredOrbitProbe"); auto *spacecraft=dynamic_cast<Spacecraft *>(draft.get()); require(spacecraft,"Spacecraft draft unavailable");
               spacecraft->SetState(Rvector6(8000,0,0,0,hyperbolic ? 12.0 : 8.0,1.0));
               applySpacecraftOrbitProperties(*draft,{{"DisplayStateType",representation},{"AnomalyType",anomaly}});
               state["controls"]=orbitControls(*draft); state["offered_anomalies"]=QJsonArray::fromStringList(spacecraftOrbitAnomalies(*draft));
               state["element_names"]=QJsonArray::fromStringList(spacecraftOrbitElementNames(*draft));
            }));
         }
      } else if (mode=="--epochs") {
         for (const auto &format:TimeSystemConverter::Instance()->GetValidTimeRepresentations()) states.append(capture(QJsonObject{{"date_format",QString::fromStdString(format)}},[&](QJsonObject &state) {
            auto draft=resourceDraft("Spacecraft","ConfiguredEpochProbe"); applySpacecraftOrbitProperties(*draft,{{"DateFormat",QString::fromStdString(format)}});
            state["controls"]=orbitControls(*draft);
         }));
      } else throw std::runtime_error("Unknown configured input inventory mode");
      report["states"]=states; int rejected=0; for (const auto &state:states) rejected+=state.toObject()["status"]!="inventoried";
      report["rejected_states"]=rejected; report["inventoried_states"]=states.size()-rejected;
      require(moderator->GetListOfObjects(Gmat::SPACECRAFT)==spacecraftBefore,"Inventory registered a configured spacecraft");
      report["loaded_file_images"]=loadedImages();
#ifdef Q_OS_LINUX
      report["loaded_file_image_set_measured"]=true;
#else
      report["loaded_file_image_set_measured"]=false;
#endif
      const auto bytes=QJsonDocument(report).toJson(); QFile file(output);
      require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Configured input inventory write failed");
      std::cout<<"CONFIGURED_INPUT_CATALOG "<<mode.toStdString()<<" inventoried="<<states.size()-rejected<<" rejected="<<rejected<<std::endl; return 0;
   } catch (BaseException &e) { std::cerr<<e.GetFullMessage()<<std::endl; }
   catch (const std::exception &e) { std::cerr<<e.what()<<std::endl; } return 1;
}
