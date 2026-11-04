#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "BodyFixedPoint.hpp"
#include "CelestialBody.hpp"
#include "A1Mjd.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>

using Vector=std::array<double,3>;
static void require(bool ok,const char *message) {if (!ok) throw std::runtime_error(message);}
static void events() {QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);}
static void reveal(QWidget &control)
{
   for (auto *page=&control;page;page=page->parentWidget()) {
      for (auto *tabs:control.window()->findChildren<QTabWidget *>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page);
      if (auto *scroll=qobject_cast<QScrollArea *>(page)) scroll->ensureWidgetVisible(&control);
   }
   events();require(control.isVisible() && control.isEnabled(),qPrintable("Station input not visible/enabled: "+control.objectName()));
}
static QLineEdit *entry(QWidget &owner,const QString &name)
{
   auto *value=owner.findChild<QLineEdit *>("station_"+name);require(value,qPrintable("Station input missing: "+name));reveal(*value);return value;
}
static QComboBox *choice(QWidget &owner,const QString &name)
{
   auto *value=owner.findChild<QComboBox *>("station_"+name);require(value,qPrintable("Station selector missing: "+name));return value;
}
static void select(QWidget &owner,const QString &name,const QString &value)
{
   auto *control=choice(owner,name);reveal(*control);require(control->findText(value)>=0,qPrintable("Station selector value missing: "+name+"="+value));control->setCurrentText(value);events();
}
static BodyFixedPoint *station(bool runtime=false)
{
   auto *object=runtime ? Moderator::Instance()->GetInternalObject("Site") : Moderator::Instance()->GetConfiguredObject("Site");
   auto *result=dynamic_cast<BodyFixedPoint *>(object);require(result && object->IsOfType("GroundStation"),"Configured/running ground station missing");return result;
}
static QMap<QString,QString> properties()
{
   QMap<QString,QString> values;for (const auto &field:resourceProperties(*station())) values[field.name]=field.value;return values;
}
// Independent geocentric/geodetic-to-body-fixed formula. Do not use GMAT's
// BodyFixedStateConverter as the oracle for the code it also serves in the UI.
static Vector cartesian(const QString &shape,double latitude,double longitude,double height)
{
   constexpr double a=6400.0,f=0.01;const double radians=std::acos(-1.0)/180,lat=latitude*radians,lon=longitude*radians;
   const double e2=shape=="Ellipsoid" ? f*(2-f) : 0.0,N=a/std::sqrt(1-e2*std::sin(lat)*std::sin(lat));
   return {(N+height)*std::cos(lat)*std::cos(lon),(N+height)*std::cos(lat)*std::sin(lon),(N*(1-e2)+height)*std::sin(lat)};
}
static QJsonArray array(const Vector &v) {return {v[0],v[1],v[2]};}
static void modal(QObject &owner,const QString &name,const std::function<void()> &trigger,const std::function<void(QDialog &)> &inspect)
{
   std::exception_ptr failure;bool visited=false;
   QTimer::singleShot(0,&owner,[&] {
      auto *dialog=owner.findChild<QDialog *>(name);
      try {require(dialog,"Station modal did not open");acceptanceView(*dialog);visited=true;inspect(*dialog);}
      catch (...) {failure=std::current_exception();if (dialog) dialog->reject();}
   });trigger();if (failure) std::rethrow_exception(failure);require(visited,"Station modal was not inspected");
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaStationCoordinates");
   try {
      require(argc==3 && acceptanceWidgetPlatform(),"Expected startup, station case and supported GUI platform");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Station acceptance output directory missing");
      const QString name=QString::fromLocal8Bit(argv[2]);const QStringList cases{"SphericalSphere","SphericalEllipsoid","CartesianSphere","CartesianEllipsoid","NorthSphere","NorthEllipsoid","SouthSphere","SouthEllipsoid","EquatorSphere","EquatorEllipsoid"};require(cases.contains(name),"Unknown station coordinate case");
      const auto type=name.startsWith("Cartesian") ? QString("Cartesian") : QString("Spherical");const auto shape=name.endsWith("Ellipsoid") ? QString("Ellipsoid") : QString("Sphere");
      const double latitude=name.startsWith("North") ? 90 : name.startsWith("South") ? -90 : name.startsWith("Equator") ? 0 : 31.25;
      const double longitude=name.startsWith("South") ? 360 : (name.startsWith("North") || name.startsWith("Equator")) ? 0 : -81.5;
      const double height=(name.startsWith("North") || name.startsWith("South") || name.startsWith("Equator")) ? 0 : 1.2;
      const auto expected=cartesian(shape,latitude,longitude,height),editedExpected=cartesian(shape,-22.75,450.25,-0.3);
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window;require(window.initialize(startup),"Station acceptance runtime initialization failed");window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(editor && tree && area,"Station workspace missing");
      const QString baseline="% Preserve station acceptance source Δ\nEarth.EquatorialRadius = 6400;\nEarth.Flattening = 0.01;\nCreate Variable Keep;\nKeep = 13; % preserve unrelated initializer\nBeginMissionSequence;\nKeep = Keep + 1; % preserve mission Δ\n";
      editor->setPlainText(baseline);require(window.buildScript(),"Station creation baseline did not build");const auto initialUndo=editor->document()->availableUndoSteps();
      auto unchangedCreation=[&] {require(editor->toPlainText()==baseline && editor->document()->availableUndoSteps()==initialUndo && !Moderator::Instance()->GetConfiguredObject("Site"),"Station draft changed model/source/history before Create");};
      int invalidChecks=0;
      auto fill=[&](QWidget &owner,double lat,double lon,double alt,bool inspectConversion) {
         select(owner,"CentralBody","Mars");select(owner,"CentralBody","Earth");select(owner,"StateType","Spherical");select(owner,"HorizonReference",shape);
         entry(owner,"Location1")->setText(QString::number(lat,'g',17));entry(owner,"Location2")->setText(QString::number(lon,'g',17));entry(owner,"Location3")->setText(QString::number(alt,'g',17));
         const auto truth=cartesian(shape,lat,lon,alt);
         select(owner,"StateType","Cartesian");
         require(!choice(owner,"HorizonReference")->isEnabled(),"Cartesian station leaves horizon reference editable");
         for (int i=0;i<3;++i) {
            const auto field="Location"+QString::number(i+1);auto *value=entry(owner,field);auto *label=owner.findChild<QLabel *>("stationLabel_"+field);auto *unit=owner.findChild<QLabel *>("stationUnit_"+field);
            require(label && label->text()==QString("XYZ")[i] && unit && unit->text()=="km","Cartesian station labels/units differ");
            if (inspectConversion) require(std::abs(value->text().toDouble()-truth[i])<=1e-8,"Station preview conversion differs from independent shape formula");
            value->setText(QString::number(truth[i],'g',17));
         }
         if (type=="Spherical") {
            select(owner,"StateType","Spherical");require(choice(owner,"HorizonReference")->isEnabled(),"Spherical station disables horizon reference");
            const QStringList labels{"Latitude","Longitude","Altitude"},units{"deg","deg","km"};
            for (int i=0;i<3;++i) {const auto field="Location"+QString::number(i+1);require(owner.findChild<QLabel *>("stationLabel_"+field)->text()==labels[i] && owner.findChild<QLabel *>("stationUnit_"+field)->text()==units[i],"Spherical station labels/units differ");}
            entry(owner,"Location1")->setText(QString::number(lat,'g',17));entry(owner,"Location2")->setText(QString::number(lon,'g',17));entry(owner,"Location3")->setText(QString::number(alt,'g',17));
         }
         entry(owner,"Id")->setText("NASA-"+name);events();
      };
      auto creator=[&](bool accept) {
         auto *action=window.findChild<QAction *>("createResource");require(action && action->isEnabled(),"New resource action missing");
         modal(window,"newResourceDialog",[&]{action->trigger();},[&](QDialog &dialog) {
            auto *types=dialog.findChild<QComboBox *>("resourceType");auto *objectName=dialog.findChild<QLineEdit *>("resourceName");require(types && types->findText("GroundStation")>=0 && objectName,"Ground-station creation controls missing");objectName->setText("Site");types->setCurrentText("GroundStation");events();
            auto *form=dialog.findChild<QWidget *>("groundStationDialog");require(form && form->isVisible() && !form->isWindow(),"Creation does not expose the actual embedded station form");fill(*form,latitude,longitude,height,true);acceptanceView(dialog);unchangedCreation();
            auto *buttons=dialog.findChild<QDialogButtonBox *>("resourceCreationButtons");require(buttons,"Station creation buttons missing");
            if (accept && (name=="SphericalEllipsoid" || name=="CartesianSphere")) {
               const QStringList fields=type=="Spherical" ? QStringList{"Location1"} : QStringList{"Location1","Location2","Location3"};
               const QStringList invalid=type=="Spherical" ? QStringList{"95","-95","nan","inf","","not-a-number"} : QStringList{"nan","inf","","not-a-number"};
               for (const auto &field:fields) {auto *value=entry(*form,field);const auto valid=value->text();for (const auto &bad:invalid) {
                  value->setText(bad);require(buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Station Create action was not available for validation");buttons->button(QDialogButtonBox::Ok)->click();events();++invalidChecks;
                  require(dialog.isVisible() && value->text()==bad && !dialog.findChild<QLabel *>("resourceCreationStatus")->text().isEmpty(),"Invalid station Create accepted, lost input or lacks a diagnostic");unchangedCreation();acceptanceView(dialog);
               }value->setText(valid);}
            }
            buttons->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
            if (accept && dialog.result()!=QDialog::Accepted) std::cerr<<dialog.findChild<QLabel *>("resourceCreationStatus")->text().toStdString()<<std::endl;
            require(dialog.result()==(accept ? QDialog::Accepted : QDialog::Rejected),"Valid station Create/Cancel did not finish");
         });
      };
      creator(false);unchangedCreation();creator(true);events();
      const auto created=editor->toPlainText();const auto createdValues=properties();
      require(created.contains("Create GroundStation Site;") && created.endsWith("BeginMissionSequence;\nKeep = Keep + 1; % preserve mission Δ\n") && created.contains("Keep = 13; % preserve unrelated initializer") && created.contains("Earth.EquatorialRadius = 6400;") && created.contains("Earth.Flattening = 0.01;"),"Station Create rewrote unrelated source/mission or lost definition");
      ResourceEditor *panel=nullptr;QMdiSubWindow *child=nullptr;
      auto open=[&](bool trigger) {
         if (trigger) {const auto matches=tree->findItems("Site",Qt::MatchExactly|Qt::MatchRecursive);require(matches.size()==1,"Created station tree entry missing");tree->itemDoubleClicked(matches.front(),0);events();}
         QList<QMdiSubWindow *> matches;for (auto *candidate:area->subWindowList()) if (candidate->property("resourceName").toString()=="Site" && dynamic_cast<ResourceEditor *>(candidate->widget())) matches.append(candidate);
         require(matches.size()==1,"Actual station resource editor missing or duplicated");child=matches.front();panel=dynamic_cast<ResourceEditor *>(child->widget());child->showNormal();area->setActiveSubWindow(child);child->raise();events();acceptanceView(*child);
         require(panel->findChild<QWidget *>("groundStationDialog") && !panel->hasChanges(),"Created/reopened station form is missing or dirty");
      };
      open(false);require(child->close(),"Clean created station editor did not close");events();editor->undo();require(editor->toPlainText()==baseline && window.buildScript() && !Moderator::Instance()->GetConfiguredObject("Site"),"Station creation Undo did not restore exact source/model");editor->redo();require(editor->toPlainText()==created && window.buildScript() && properties()==createdValues,"Station creation Redo did not restore exact source/model");
      auto persist=[&](const QString &label,const QString &source,const QMap<QString,QString> &values) {
         const auto path=files.filePath(label+" "+name+" Δ.script");require(window.saveScriptTo(path) && window.loadScript(path) && window.buildScript(),"Station Unicode save/reopen failed");QFile file(path);require(file.open(QIODevice::ReadOnly) && QString::fromUtf8(file.readAll())==source && editor->toPlainText()==source && properties()==values,"Station persistence changed exact source/properties");
      };
      double maxError=0;auto geometry=[&](const Vector &truth) {
         require(window.runMission()==MainWindow::RunResult::Completed,"Station geometry mission failed");auto *runtime=station(true);require(runtime!=station(),"Station check did not use a separate running sandbox object");
         require(runtime->GetStringParameter("StateType")==type.toStdString() && runtime->GetStringParameter("HorizonReference")==shape.toStdString() && runtime->GetStringParameter("CentralBody")=="Earth","Runtime station coordinate selectors differ from GUI");
         auto *body=dynamic_cast<CelestialBody *>(runtime->GetCentralBody());require(body && body->GetEquatorialRadius()==6400 && body->GetRealParameter(body->GetParameterID("Flattening"))==0.01,"Running station used different body shape constants");
         const auto position=runtime->GetBodyFixedLocation(A1Mjd(21545)),later=runtime->GetBodyFixedLocation(A1Mjd(21546));Vector result{};
         for (int i=0;i<3;++i) {result[i]=position[i];maxError=std::max(maxError,std::abs(result[i]-truth[i]));if (std::abs(result[i]-truth[i])>1e-8) std::cerr<<"Station coordinate "<<i<<" actual="<<result[i]<<" expected="<<truth[i]<<std::endl;require(std::isfinite(result[i]) && std::abs(result[i]-truth[i])<=1e-8 && result[i]==later[i],"Runtime station body-fixed position differs from independent geometry or moves in the body frame");}
         auto *keep=Moderator::Instance()->GetInternalObject("Keep");require(keep && keep->GetRealParameter("Value")==14,"Station fixture did not execute its known mission result");return result;
      };
      persist("created",created,createdValues);const auto createdPosition=geometry(expected);
      open(true);auto *form=panel->findChild<QWidget *>("groundStationDialog");entry(*form,"Id")->setText("Discarded");const auto editUndo=editor->document()->availableUndoSteps();
      auto closePending=[&](QMessageBox::StandardButton response) {
         std::exception_ptr failure;bool seen=false;QTimer::singleShot(0,&window,[&] {auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget());try {require(box && box->windowTitle()=="Unapplied changes" && box->button(response),"Station close omitted the pending-change prompt");acceptanceView(*box);seen=true;box->button(response)->click();} catch (...) {failure=std::current_exception();if (box) box->reject();}});
         panel->findChild<QDialogButtonBox *>("resourceButtons")->button(QDialogButtonBox::Close)->click();if (failure) std::rethrow_exception(failure);require(seen,"Station pending-close prompt not inspected");events();require(editor->toPlainText()==created && properties()==createdValues && editor->document()->availableUndoSteps()==editUndo,"Station Close/Cancel/Discard changed applied source/model/history");
      };
      closePending(QMessageBox::Cancel);require(panel->hasChanges() && entry(*form,"Id")->text()=="Discarded","Canceling station Close lost the pending edit");QPointer<QMdiSubWindow> retained=child;closePending(QMessageBox::Discard);require(!retained,"Discarding station changes did not close editor");
      open(true);form=panel->findChild<QWidget *>("groundStationDialog");fill(*form,-22.75,450.25,-0.3,true);const auto invalidOriginal=entry(*form,"Location3")->text();entry(*form,"Location3")->setText("nan");panel->requestApply();events();++invalidChecks;
      require(panel->hasChanges() && entry(*form,"Location3")->text()=="nan" && !panel->findChild<QLabel *>("resourceStatus")->text().isEmpty() && editor->toPlainText()==created && properties()==createdValues && editor->document()->availableUndoSteps()==editUndo,"Invalid station Apply was not atomic or lost pending input/diagnostic");entry(*form,"Location3")->setText(invalidOriginal);panel->requestApply();events();panel=dynamic_cast<ResourceEditor *>(child->widget());require(panel && !panel->hasChanges(),"Corrected station coordinate Apply failed");
      const auto applied=editor->toPlainText();const auto appliedValues=properties();require(applied.endsWith("BeginMissionSequence;\nKeep = Keep + 1; % preserve mission Δ\n") && applied.contains("Keep = 13; % preserve unrelated initializer"),"Station coordinate Apply rewrote unrelated source/mission");require(child->close(),"Applied station editor did not close");events();editor->undo();require(editor->toPlainText()==created && window.buildScript() && properties()==createdValues,"Station coordinate Undo was not exact");editor->redo();require(editor->toPlainText()==applied && window.buildScript() && properties()==appliedValues,"Station coordinate Redo was not exact");persist("applied",applied,appliedValues);const auto appliedPosition=geometry(editedExpected);
      if (type=="Spherical") require(std::abs(station()->GetRealParameter("Location2")-90.25)<=1e-12,"Applied station longitude did not normalize modulo 360");
      const QJsonObject proof{{"checkpoint","station-coordinates."+name},{"case",name},{"state_type",type},{"horizon_reference",shape},{"central_body","Earth"},{"body_radius_km",6400},{"body_flattening",0.01},{"latitude_degrees",latitude},{"longitude_input_degrees",longitude},{"height_km",height},{"created_cartesian_km",array(createdPosition)},{"created_expected_cartesian_km",array(expected)},{"applied_cartesian_km",array(appliedPosition)},{"applied_expected_cartesian_km",array(editedExpected)},{"maximum_coordinate_error_km",maxError},{"coordinate_tolerance_km",1e-8},{"invalid_rejections_checked",invalidChecks},{"outcome","passed"},
         {"facets",QJsonArray{"visible-embedded-creation-form","create-cancel-isolation","typed-body-shape-state-selectors","independent-preview-conversion","labels-units-dependencies","fresh-sandbox-body-fixed-geometry","body-shape-constants","static-body-fixed-position","pending-close-cancel-discard","invalid-apply-correct-and-commit","exact-creation-and-edit-undo-redo","unicode-save-reopen","known-mission-result"}},
         {"scope","synthetic known Earth shape and specified coordinate cases; not empirical geodesy, Earth orientation, contact/measurement accuracy or a complete input/platform domain"}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   } catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}
     catch (const std::exception &error) {std::cerr<<"FAIL "<<error.what()<<std::endl;}
   return 1;
}
