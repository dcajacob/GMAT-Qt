#pragma once
#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "EditablePanel.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "Spacecraft.hpp"
#include "FormationInterface.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "PropagationStateManager.hpp"
#include "GmatCommand.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace formaccept {
inline void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
inline void events(){QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);QApplication::processEvents();}
inline QByteArray read(const QString &path){QFile f(path);require(f.open(QIODevice::ReadOnly),"Formation artifact missing");return f.readAll();}
inline void write(const QString &path,const QByteArray &bytes){QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Formation artifact write failed");}
inline QString number(double value){return QString::number(value,'g',17);}
constexpr double mu=398600.4415,epoch=21545,duration=60;
struct Orbit{QString name;double radius,phase;int plane;};
inline const std::array<Orbit,4> orbits{{{"One",7000,0,0},{"Two",9000,.4,1},{"Three",11000,-.3,2},{"Witness",13000,.8,0}}};
inline std::array<double,6> circular(const Orbit &orbit,double seconds){
 const double omega=std::sqrt(mu/(orbit.radius*orbit.radius*orbit.radius)),angle=orbit.phase+omega*seconds;
 const double x=orbit.radius*std::cos(angle),y=orbit.radius*std::sin(angle),vx=-orbit.radius*omega*std::sin(angle),vy=orbit.radius*omega*std::cos(angle);
 if(orbit.plane==0)return{x,y,0,vx,vy,0};if(orbit.plane==1)return{0,x,y,0,vx,vy};return{x,0,y,vx,0,vy};
}
inline QStringList members(GmatBase *object){require(object,"Formation native object missing");QStringList result;for(const auto &name:object->GetStringArrayParameter("Add"))result.append(QString::fromStdString(name));return result;}
class Rig{
public:
 TestSettings settings;QTemporaryDir files;MainWindow window;QPlainTextEdit *source,*messages;QMdiArea *area;QTreeWidget *resources,*mission;
 QString report,evidence;QJsonArray results;
 explicit Rig(const QString &startup){
  require(acceptanceWidgetPlatform()&&QApplication::platformName()!="offscreen","Formation native widget platform required");require(files.isValid(),"Formation fixture unavailable");QDir::setCurrent(QFileInfo(startup).absolutePath());
  report=files.filePath("Formation states Ω.csv");evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");window.resize(1200,850);window.show();require(window.initialize(startup),"Formation runtime unavailable");acceptanceView(window);
  require(std::abs(Moderator::Instance()->GetSolarSystemInUse()->GetBody("Earth")->GetGravitationalConstant()-mu)<1e-9,"Formation Earth gravitational constant differs from literal oracle input");
  source=window.findChild<QPlainTextEdit *>("scriptEditor");messages=window.findChild<QPlainTextEdit *>("messageWindow");area=window.findChild<QMdiArea *>("workspace");resources=window.findChild<QTreeWidget *>("Resources");mission=window.findChild<QTreeWidget *>("Mission");require(source&&messages&&area&&resources&&mission,"Formation workspace unavailable");
 }
 ~Rig(){source->document()->setModified(false);}
 void save(const QString &name,const QByteArray &bytes){if(!evidence.isEmpty())write(evidence+"/"+name,bytes);}
 QString script(const QStringList &fleet,bool group)const{
  QString text="% Formation analytic fixture Ω\nCreate Variable Keep;\nKeep = 41;\nCreate Spacecraft One Two Three Witness;\n";
  const QStringList fields{"X","Y","Z","VX","VY","VZ"};
  for(const auto &orbit:orbits){text+=orbit.name+".CoordinateSystem = EarthMJ2000Eq;\n"+orbit.name+".DisplayStateType = Cartesian;\n"+orbit.name+".DateFormat = A1ModJulian;\n"+orbit.name+".Epoch = '21545';\n";const auto state=circular(orbit,0);for(int i=0;i<6;++i)text+=orbit.name+"."+fields[i]+" = "+number(state[i])+";\n";}
  text+="Create Formation Fleet;\nFleet.Add = {"+fleet.join(", ")+"};\nCreate ForceModel FM;\nFM.CentralBody = Earth;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\nFM.Drag = None;\nFM.SRP = Off;\nCreate Propagator P;\nP.FM = FM;\nP.Type = PrinceDormand78;\nP.Accuracy = 0.000000000001;\nP.InitialStepSize = 10;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 25;\nBeginMissionSequence;\nPropagate 'Group flight' P("+QString(group?"Fleet":"One")+") {One.ElapsedSecs = 60}; % preserve propagation\nReport Values";
  for(const auto &orbit:orbits){text+=" "+orbit.name+".A1ModJulian "+orbit.name+".ElapsedSecs";for(const auto &field:fields)text+=" "+orbit.name+"."+field;}
  return text+" Keep;\n% Formation tail sentinel Ω\n";
 }
 void closePanels(){for(auto *child:area->subWindowList())if(auto *panel=dynamic_cast<EditablePanel *>(child->widget())){panel->discardChanges();child->close();}events();}
 QJsonObject run(const QString &phase,const QStringList &fleet,bool group){
  const auto before=source->toPlainText();const int undo=source->document()->availableUndoSteps();const auto status=window.runMission();events();save(phase+".messages.txt",messages->toPlainText().toUtf8());require(status==MainWindow::RunResult::Completed,"Formation witness mission failed");
  const auto bytes=read(report);save(phase+".csv",bytes);save(phase+".script",before.toUtf8());const auto lines=QString::fromUtf8(bytes).trimmed().split('\n');require(lines.size()==1,"Formation report row count differs");const auto columns=lines[0].split(',');require(columns.size()==33,"Formation report column count differs");QJsonArray values;
  for(const auto &column:columns){bool ok=false;const double value=column.toDouble(&ok);require(ok&&std::isfinite(value),"Formation report contains nonfinite value");values.append(value);}require(values.last().toDouble()==41,"Formation changed unrelated sentinel");
  const auto active=group?fleet:QStringList{"One"};QJsonArray expected,nativeMembers;
  for(int i=0;i<4;++i){const auto &orbit=orbits[i];const double seconds=active.contains(orbit.name)?duration:0;const auto truth=circular(orbit,seconds);expected.append(epoch+seconds/86400);expected.append(seconds);
   require(std::abs(values[i*8].toDouble()-(epoch+seconds/86400))<1e-10&&std::abs(values[i*8+1].toDouble()-seconds)<.00003,"Formation member/nonmember clock differs from shared stop or untouched epoch");
   auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject(orbit.name.toStdString()));require(sat,"Formation native spacecraft missing");auto &state=sat->GetState();require(state.GetSize()==6,"Formation native spacecraft state size differs");QJsonArray native;
   for(int c=0;c<6;++c){expected.append(truth[c]);const double observed=values[i*8+2+c].toDouble();require(std::abs(observed-truth[c])<(c<3?.00002:.00000003),"Formation member/nonmember state differs from independent three-plane circular oracle");require(std::abs(state[c]-observed)<1e-9,"Formation report differs from native spacecraft state");native.append(state[c]);}
   require(std::abs(sat->GetEpoch()-values[i*8].toDouble())<1e-10,"Formation report differs from native spacecraft epoch");nativeMembers.append(QJsonObject{{"name",orbit.name},{"state",native},{"epoch",sat->GetEpoch()}});
  }
  expected.append(41);auto *command=Moderator::Instance()->GetFirstCommand();int guard=0;while(command&&!command->IsOfType("Propagate")&&++guard<100)command=command->GetNext();auto *setup=command?dynamic_cast<PropSetup *>(command->GetClone(0)):nullptr;require(setup&&setup->GetPropagator(),"Formation actual Propagate integrator clone missing");auto *manager=setup->GetPropStateManager();auto *combined=manager?manager->GetState():nullptr;
  require(combined&&combined->GetSize()==6*active.size(),"Formation did not use one combined member propagation vector");ObjectArray objects;require(manager->GetStateObjects(objects),"Formation native propagation state objects unavailable");QJsonArray objectNames;for(auto *object:objects)objectNames.append(QString::fromStdString(object->GetName()));save(phase+".native-objects.json",QJsonDocument(QJsonObject{{"objects",objectNames},{"dimension",combined->GetSize()}}).toJson());require(objects.size()==1&&objects[0]->GetName()==(group?"Fleet":"One")&&objects[0]->IsOfType(group?Gmat::FORMATION:Gmat::SPACECRAFT),"Formation Propagate clone selected unexpected state objects");QJsonArray packed;
  for(int i=0;i<active.size();++i){int index=-1;for(int k=0;k<4;++k)if(orbits[k].name==active[i])index=k;require(index>=0,"Formation active member unknown");for(int c=0;c<6;++c){const double value=(*combined)[i*6+c];require(std::abs(value-values[index*8+2+c].toDouble())<1e-8,"Formation combined integrator vector lost member order/state");packed.append(value);}}
  if(group){auto *formation=dynamic_cast<FormationInterface *>(Moderator::Instance()->GetInternalObject("Fleet"));require(formation&&members(formation)==fleet&&formation->GetState().GetSize()==combined->GetSize(),"Formation runtime membership/dimension differs from configured group");for(int i=0;i<combined->GetSize();++i)require(std::abs(formation->GetState()[i]-(*combined)[i])<1e-8,"Formation container and combined integrator state differ");require(std::abs(formation->GetEpoch()-values[0].toDouble())<1e-10,"Formation container did not share member epoch");}
  require(members(Moderator::Instance()->GetConfiguredObject("Fleet"))==fleet,"Formation configured membership changed on Run");require(source->toPlainText()==before&&source->document()->availableUndoSteps()==undo,"Formation Run changed source/history");QJsonObject result{{"phase",phase},{"members",QJsonArray::fromStringList(fleet)},{"group",group},{"active",QJsonArray::fromStringList(active)},{"raw_values",values},{"analytic_values",expected},{"native_members",nativeMembers},{"combined_vector",packed},{"combined_dimension",combined->GetSize()},{"propagator",QString::fromStdString(setup->GetPropagator()->GetTypeName())}};results.append(result);save("results-values.json",QJsonDocument(results).toJson());return result;
 }
};
}
