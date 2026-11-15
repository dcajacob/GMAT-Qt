#include "FormationAcceptance.hpp"
#include "OrbitObjectDrawingDialog.hpp"
#include "OrbitSensorMaskDialog.hpp"
#include "OrbitStarOptionsDialog.hpp"
#include "OrbitPlaybackOptionsDialog.hpp"
#include "SolarSystemPanel.hpp"
#include "PlotWidget.hpp"
#include "ParameterInfo.hpp"
#include <QAction>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QTableWidget>
#include <QTest>
#include <QToolBar>
using namespace formaccept;
static void reveal(QWidget *widget)
{
   require(widget,"Display-options unit control missing");
   for (auto *parent=widget;parent;parent=parent->parentWidget()) {
      for (auto *tabs:widget->window()->findChildren<QTabWidget *>())
         if (tabs->indexOf(parent)>=0) tabs->setCurrentWidget(parent);
      if (auto *scroll=qobject_cast<QScrollArea *>(parent)) scroll->ensureWidgetVisible(widget);
   }
   events();require(widget->isVisible(),"Display-options unit control hidden");acceptanceView(*widget);
}
static QString caption(QWidget &owner,QWidget *field)
{
   for (auto *form:owner.findChildren<QFormLayout *>())
      if (auto *label=qobject_cast<QLabel *>(form->labelForField(field))) {reveal(label);return label->text();}
   return {};
}
static QJsonObject graph()
{
   QJsonObject result;auto *moderator=Moderator::Instance();
   for (const auto &name:moderator->GetListOfObjects(Gmat::UNKNOWN_OBJECT))
      if (auto *object=moderator->GetConfiguredObject(name))
         result[QString::fromStdString(name)]=QString::fromStdString(object->GetGeneratingString(Gmat::SHOW_SCRIPT));
   return result;
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("DisplayOptionsUnitTests");
   try {
      require(argc==2,"Supply startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const QString source="% Display option units Δ\nCreate Spacecraft Sat;\nSat.NAIFId = -101294;\nSat.NAIFIdReferenceFrame = -911294;\nCreate ConicalFOV Cone;\nCreate Imager Camera;\nCamera.FieldOfView = Cone;\nSat.AddHardware = {Camera};\nBeginMissionSequence;\n";
      const auto saved=rig.files.filePath("Display options Ω.script");write(saved,source.toUtf8());
      require(rig.window.loadScript(saved)&&rig.window.buildScript(),"Display options fixture failed");
      const auto native=graph();const int undo=rig.source->document()->availableUndoSteps();
      auto *info=ParameterInfo::Instance();const auto names=info->GetNamesOfParameters(),types=info->GetTypesOfParameters();
      auto *solar=Moderator::Instance()->GetSolarSystemInUse();const auto solarBefore=solarSystemSettings(*solar);
      QJsonArray observations;int failures=0;
      auto stable=[&] {
         const auto solarAfter=solarSystemSettings(*solar);
         require(graph()==native&&rig.source->toPlainText()==source&&read(saved)==source.toUtf8()
            &&rig.source->document()->availableUndoSteps()==undo&&info->GetNamesOfParameters()==names
            &&info->GetTypesOfParameters()==types&&solarAfter.values==solarBefore.values
            &&solarAfter.sources==solarBefore.sources&&solarAfter.deFiles==solarBefore.deFiles,
            "Display options changed native/source/history/inventory/Solar System");
      };
      auto observe=[&](const QString &key,const QString &control,const QString &actual,const QString &expected) {
         const bool passed=actual==expected;failures+=!passed;
         observations.append(QJsonObject{{"key",key},{"control",control},{"actual",actual},{"expected",expected},{"passed",passed}});
         rig.save("unit-observations.json",QJsonDocument(observations).toJson());stable();
      };
      {
         OrbitObjectDrawingDialog dialog({"Sat","Earth"},{},{},&rig.window);const auto before=dialog.settings();dialog.show();events();
         for (const auto &spec:QList<QStringList>{{"orbitObjectDrawing","3","Width (px)"},{"orbitObjectMarkers","3","Size (px)"},{"orbitObjectFonts","1","Font (px)"}}) {
            auto *table=dialog.findChild<QTableWidget *>(spec[0]);require(table&&table->rowCount()==2,"Drawing repeat rows absent");reveal(table);
            const int column=spec[1].toInt();for (int row=0;row<2;++row) {auto *field=table->cellWidget(row,column);require(field&&field->isEnabled(),"Drawing numeric input unavailable");reveal(field);}
            observe("drawing",spec[0],table->horizontalHeaderItem(column)->text(),spec[2]);
         }
         require(dialog.settings()==before,"Drawing unit inspection changed pending values");dialog.reject();stable();
      }
      {
         QtStarSetting setting;setting.preset="Custom";OrbitStarOptionsDialog dialog(setting,&rig.window);const auto before=qtStarsJson(dialog.settings());dialog.show();events();
         const QMap<QString,QString> expected={{"starCustomCount","Custom star count"},{"starMinMagnitude","Minimum magnitude"},
            {"starMaxMagnitude","Maximum magnitude"},{"starMinPixels","Minimum size (pixels)"},
            {"starMaxPixels","Maximum size (pixels)"},{"starMinDimRatio","Minimum dimming ratio (0–1)"}};
         for (auto it=expected.cbegin();it!=expected.cend();++it) {auto *field=dialog.findChild<QLineEdit *>(it.key());reveal(field);require(field->isEnabled(),"Custom star input disabled");observe("stars",it.key(),caption(dialog,field),it.value());}
         require(qtStarsJson(dialog.settings())==before,"Star unit inspection changed pending values");dialog.reject();stable();
      }
      {
         QtSensorMaskSetting mask;mask.name="Mask";mask.source="Sat";mask.hardware="Camera";mask.automaticLength=false;mask.length=5;
         OrbitSensorMaskDialog dialog({"Sat"},{mask},&rig.window);const auto before=qtSensorMasksJson(dialog.settings());dialog.show();events();
         auto *table=dialog.findChild<QTableWidget *>("orbitSensorMasks");require(table&&table->rowCount()==1,"Mask row missing");reveal(table);
         auto *field=table->cellWidget(0,5);reveal(field);require(field->isEnabled(),"Manual mask length disabled");
         observe("mask","manual-length",table->horizontalHeaderItem(5)->text(),"Length (km)");
         require(qtSensorMasksJson(dialog.settings())==before,"Mask unit inspection changed pending values");dialog.reject();stable();
      }
      {
         QtPlaybackSetting setting;setting.timeScale=3600;OrbitPlaybackOptionsDialog dialog(setting,{},&rig.window);const auto before=qtPlaybackJson(dialog.settings());dialog.show();events();
         auto *field=dialog.findChild<QLineEdit *>("playbackTimeScale");reveal(field);require(field->isEnabled(),"Playback time scale disabled");
         observe("playback","playbackTimeScale",caption(dialog,field),"Simulation seconds per real second");
         require(qtPlaybackJson(dialog.settings())==before,"Playback unit inspection changed pending values");dialog.reject();stable();
      }
      {
         int applied=0;SolarSystemPanel panel(*solar,[&](const QMap<QString,QString> &){++applied;return QString();});panel.resize(900,650);panel.show();events();
         auto *field=panel.findChild<QLineEdit *>("ephemerisUpdateInterval");reveal(field);
         observe("solar-system","ephemerisUpdateInterval",caption(panel,field),"Update interval (s)");
         require(applied==0,"Solar System unit inspection applied values");panel.discardChanges();panel.close();stable();
      }
      {
         auto model=std::make_shared<PlotModel>(PlotModel::Kind::Orbit);model->title="Camera units";model->perspective=true;
         PlotWidget plot(model);plot.resize(1050,750);plot.show();events();const auto before=model->fieldOfView;
         auto *action=plot.findChild<QAction *>("orbitCameraAction");require(action,"Orbit camera action missing");QWidget *button=nullptr;
         for (auto *bar:plot.findChildren<QToolBar *>()) if (bar->actions().contains(action)) button=bar->widgetForAction(action);
         reveal(button);QTest::mouseClick(button,Qt::LeftButton);events();
         auto *dialog=plot.findChild<QDialog *>("orbitCameraDialog");require(dialog&&dialog->isVisible(),"Camera dialog not visible");
         auto *field=dialog->findChild<QDoubleSpinBox *>("orbitFieldOfView");reveal(field);require(field->isEnabled(),"Perspective FOV disabled");
         observe("camera","orbitFieldOfView",field->suffix(),"°");dialog->reject();
         require(model->perspective&&model->fieldOfView==before,"Camera unit inspection changed projection");plot.close();stable();
      }
      require(observations.size()==13,"Display options observation inventory differs");
      QJsonObject proof{{"status",failures?"failed":"passed"},{"observations",observations},{"mismatches",failures},
         {"source_native_history_file_and_parameter_inventory_unchanged",true},{"pending_settings_unchanged",true},
         {"mission_executions",0},{"new_requirement_credit",0}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return failures?1:0;
   } catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n';}
     catch (std::exception &error) {std::cerr<<"FAIL "<<error.what()<<'\n';}
   return 1;
}
