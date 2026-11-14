#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "CelestialBodyPanel.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "SolarSystem.hpp"
#include "CelestialBody.hpp"
#include "BaseException.hpp"
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
#include <QListWidget>
#include <QLocale>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using State=std::array<double,6>;
const QStringList fields{"InitialEpoch","SMA","ECC","INC","RAAN","AOP","TA"};
constexpr double gravity=398600.4415, bodyMu=63;
void require(bool ok,const QString &message) { if (!ok) throw std::runtime_error(message.toStdString()); }
void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); QApplication::processEvents(); }
void write(const QString &path,const QByteArray &bytes) { QFile f(path); require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Cannot retain body-orbit evidence"); }
QByteArray read(const QString &path) { QFile f(path); require(f.open(QIODevice::ReadOnly),"Body-orbit artifact missing"); return f.readAll(); }
QString number(double value) { return QString::number(value,'g',17); }
void reveal(QWidget *widget)
{
   require(widget,"Body-orbit control missing");
   for (auto *parent=widget;parent;parent=parent->parentWidget()) {
      for (auto *tabs:widget->window()->findChildren<QTabWidget*>())
         if (tabs->indexOf(parent)>=0) tabs->setCurrentWidget(parent);
      if (auto *scroll=qobject_cast<QScrollArea*>(parent)) scroll->ensureWidgetVisible(widget);
   }
   events(); require(widget->isVisible()&&widget->isEnabled(),"Body-orbit control unreachable"); acceptanceView(*widget);
}
void click(QDialogButtonBox *buttons,QDialogButtonBox::StandardButton which)
{
   auto *button=buttons?buttons->button(which):nullptr; reveal(button); QTest::mouseClick(button,Qt::LeftButton); events();
}
// Independent perifocal equations and a fixed-step Cartesian RK4 oracle.
// No GMAT state-conversion or Kepler solver supplies expected states.
State initial(const State &elements,double mu,double addedAnomaly=0)
{
   const double rad=std::acos(-1.)/180.,a=elements[0],e=elements[1],i=elements[2]*rad;
   const double node=elements[3]*rad,peri=elements[4]*rad,ta=(elements[5]+addedAnomaly)*rad;
   const double p=a*(1-e*e),radius=p/(1+e*std::cos(ta)),speed=std::sqrt(mu/p);
   const State pqw{radius*std::cos(ta),radius*std::sin(ta),0,-speed*std::sin(ta),speed*(e+std::cos(ta)),0};
   const double c=std::cos(node),s=std::sin(node),w=std::cos(peri),v=std::sin(peri),ci=std::cos(i),si=std::sin(i);
   const std::array<double,9> rotation{c*w-s*v*ci,-c*v-s*w*ci,s*si,s*w+c*v*ci,-s*v+c*w*ci,-c*si,v*si,w*si,ci};
   State out{}; for (int block=0;block<2;++block) for (int row=0;row<3;++row)
      for (int col=0;col<3;++col) out[3*block+row]+=rotation[3*row+col]*pqw[3*block+col];
   return out;
}
State rk4(State state,double duration,double mu)
{
   const int steps=std::max(1,int(std::ceil(std::abs(duration)/.25))); const double h=duration/steps;
   auto derivative=[&](const State &v) {const double radius=std::hypot(v[0],v[1],v[2]),f=-mu/(radius*radius*radius);return State{v[3],v[4],v[5],f*v[0],f*v[1],f*v[2]};};
   auto add=[](State v,const State &d,double step) {for (int k=0;k<6;++k)v[k]+=step*d[k];return v;};
   for (int step=0;step<steps;++step) {const auto a=derivative(state),b=derivative(add(state,a,h/2)),c=derivative(add(state,b,h/2)),d=derivative(add(state,c,h));for (int k=0;k<6;++k)state[k]+=h*(a[k]+2*b[k]+2*c[k]+d[k])/6;}
   return state;
}
QJsonArray json(const State &state) { QJsonArray out; for (auto value:state) out.append(value); return out; }
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("UserBodyOrbit"); QLocale::setDefault(QLocale::c());
   try {
      require(argc==3&&acceptanceWidgetPlatform(),"Supply startup and body type");
      const QString selected=QString::fromLocal8Bit(argv[2]); const bool nested=selected=="NestedMoon";
      const QString kind=nested?"Moon":selected;
      require(QStringList{"Moon","Asteroid","Comet","Planet"}.contains(kind),"Unknown original body type");
      const QString parent=nested?"Parent":kind=="Moon"?"Earth":"Sun";
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      TestSettings settings; QTemporaryDir temporary; require(temporary.isValid(),"Body-orbit temporary directory missing");
      const auto retained=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR"),directory=retained.isEmpty()?temporary.path():retained;
      const auto saved=directory+"/Body orbit Ω.script",report=directory+"/Witness Δ.txt";
      const auto kernel=QDir::cleanPath(QFileInfo(startup).absolutePath()+"/../data/planetary_ephem/spk/ceres_1900_2100.bsp");
      require(QFileInfo::exists(kernel),"Shipped body kernel missing");
      QString setup="% Body orbit and unrelated witness Ω\nSolarSystem.EphemerisUpdateInterval = 0;\nEarth.Mu = "+number(gravity)+";\nSun.Mu = "+number(gravity)+";\n";
      if (nested) setup+="Create Moon Parent;\nParent.CentralBody = Earth;\nParent.Mu = 63;\nParent.PosVelSource = TwoBodyPropagation;\nParent.InitialEpoch = 21545;\nParent.SMA = 50000;\nParent.ECC = 0;\nParent.INC = 0;\nParent.RAAN = 0;\nParent.AOP = 0;\nParent.TA = 0;\n";
      setup+="Create "+kind+" Probe;\nProbe.CentralBody = "+parent+";\nProbe.Mu = 63;\nProbe.NAIFId = 2000001;\nProbe.OrbitSpiceKernelName = {'"+kernel+"'};\nCreate Spacecraft Witness;\nCreate Variable Keep;\nKeep = 41;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nBeginMissionSequence;\nKeep = Keep + 1;\nReport Values Keep;\n% Preserve mission tail Δ\n";
      MainWindow window; require(window.initialize(startup),"Body-orbit runtime initialization failed"); window.resize(1200,850); window.show(); acceptanceView(window); events();
      auto *source=window.findChild<QPlainTextEdit*>("scriptEditor"),*messages=window.findChild<QPlainTextEdit*>("messageWindow");
      auto *area=window.findChild<QMdiArea*>("workspace"); auto *tree=window.findChild<QTreeWidget*>("Resources"); require(source&&messages&&area&&tree,"Body-orbit workspace missing");
      source->setPlainText(setup); require(window.buildScript(),"Body-orbit baseline build failed: "+messages->toPlainText()); source->document()->setModified(false);
      auto body=[&](const QString &name=QString("Probe")) {auto *p=Moderator::Instance()->GetSolarSystemInUse()->GetBody(name.toStdString());require(p,"Configured body missing: "+name);return p;};
      require(body()->IsUserDefined()&&body()->GetTypeName()==kind.toStdString()&&body()->GetCentralBody()==parent.toStdString(),"Body type/parent differs from original context");
      const int baselineUndo=source->document()->availableUndoSteps();
      require(window.runMission()==MainWindow::RunResult::Completed&&QString::fromUtf8(read(report)).trimmed()=="42","Initial SPICE witness failed");events();
      const auto baselineSpice=body()->GetMJ2000State(GmatTime(21545.));
      const auto initialNative=QString::fromStdString(body()->GetGeneratingString(Gmat::SHOW_SCRIPT));
      auto pristine=[&] {return source->toPlainText()==setup&&source->document()->availableUndoSteps()==baselineUndo&&!source->document()->isModified()&&QString::fromStdString(body()->GetGeneratingString(Gmat::SHOW_SCRIPT))==initialNative;};
      QPointer<QMdiSubWindow> child; CelestialBodyPanel *panel=nullptr;
      auto open=[&] {
         for (auto *tabs:window.findChildren<QTabWidget*>()) for (int i=0;i<tabs->count();++i)
            if (tabs->widget(i)==tree||tabs->widget(i)->isAncestorOf(tree)) tabs->setCurrentIndex(i);
         const auto items=tree->findItems("Probe",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Body-orbit tree identity missing");
         tree->scrollToItem(items.first()); events(); reveal(tree); const auto point=tree->visualItemRect(items.first()).center();
         QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,point); events(); QTest::mouseDClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,point); events();
         child=area->activeSubWindow(); panel=child?dynamic_cast<CelestialBodyPanel*>(child->widget()):nullptr; require(panel,"Actual celestial-body editor missing");
         auto *center=panel->findChild<QLineEdit*>("body_CentralBody"); reveal(center); require(center->isReadOnly()&&center->text()==parent,"Body central-parent identity changed");
         return panel;
      };
      auto mode=[&](const QString &value) {
         auto *combo=panel->findChild<QComboBox*>("body_PosVelSource"); reveal(combo);
         require(combo->findText("SPICE")>=0&&combo->findText("TwoBodyPropagation")>=0,"A required ephemeris choice is missing");
         combo->setCurrentText(value); events(); acceptanceView(*combo);
         for (const auto &field:fields) {auto *input=panel->findChild<QLineEdit*>("body_"+field);reveal(input);require(input->isReadOnly()==(value!="TwoBodyPropagation"),"Orbit field applicability differs: "+field);}
         auto *kernels=panel->findChild<QListWidget*>("body_OrbitSpiceKernelName"); require(kernels&&kernels->parentWidget()->isEnabled()==(value=="SPICE"),"Kernel controls do not follow propagation mode");
      };
      auto set=[&](double epoch,const State &elements) {
         for (int i=0;i<fields.size();++i) {auto *input=panel->findChild<QLineEdit*>("body_"+fields[i]);reveal(input);require(!input->isReadOnly(),"Body orbital input read-only");input->setText(number(i?elements[i-1]:epoch));events();acceptanceView(*input);}
      };
      const State ellipse{24000,.4,67,123,87,55};
      open(); mode("SPICE"); mode("TwoBodyPropagation"); set(21546,ellipse); mode("SPICE"); mode("TwoBodyPropagation");
      require(panel->hasChanges()&&pristine(),"Pending orbit or mode edits changed source/native state");
      auto closeChoice=[&](QMessageBox::StandardButton which) {
         std::exception_ptr failure; QTimer timer; timer.setInterval(10);
         QObject::connect(&timer,&QTimer::timeout,&window,[&] {auto *box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget());if(!box)return;timer.stop();try{acceptanceView(*box);auto *button=box->button(which);require(button,"Body draft close choice missing");QTest::mouseClick(button,Qt::LeftButton);}catch(...){failure=std::current_exception();box->reject();}});
         timer.start();click(panel->findChild<QDialogButtonBox*>(),QDialogButtonBox::Close);timer.stop();if(failure)std::rethrow_exception(failure);
      };
      closeChoice(QMessageBox::Cancel); require(child&&panel->hasChanges()&&pristine(),"Body Close Cancel discarded a draft");
      closeChoice(QMessageBox::Discard); require(!child&&pristine(),"Body draft discard changed source/model/history");
      open(); mode("TwoBodyPropagation"); set(21546,ellipse);
      const QList<QPair<QString,QString>> invalid{{"InitialEpoch","6115"},{"InitialEpoch","58128"},{"SMA","0"},{"SMA","-24000"},{"ECC","1"},{"ECC","-0.1"},{"INC","181"}};
      for (const auto &entry:invalid) {
         auto *input=panel->findChild<QLineEdit*>("body_"+entry.first);const auto valid=input->text();input->setText(entry.second);events();click(panel->findChild<QDialogButtonBox*>(),QDialogButtonBox::Apply);
         auto *status=panel->findChild<QLabel*>("bodyStatus");require(child&&status&&!status->text().isEmpty()&&panel->hasChanges()&&pristine(),"Invalid orbit edit escaped atomic rollback: "+entry.first);acceptanceView(*status);input->setText(valid);events();
      }
      click(panel->findChild<QDialogButtonBox*>(),QDialogButtonBox::Apply); events();
      const auto applied=source->toPlainText();require(applied!=setup,"Body orbit Apply did not commit");
      auto native=[&](double epoch,const State &elements) {
         auto *p=body();require(p->GetCentralBody()==parent.toStdString()&&p->GetStringParameter(p->GetParameterID("PosVelSource"))=="TwoBodyPropagation","Native body mode/parent differs");
         for (int i=0;i<fields.size();++i)require(std::abs(p->GetRealParameter(p->GetParameterID(fields[i].toStdString()))-(i?elements[i-1]:epoch))<=1e-12,"Native orbital property differs: "+fields[i]);
      };
      native(21546,ellipse); panel=dynamic_cast<CelestialBodyPanel*>(child->widget());
      require(panel&&!panel->hasChanges()&&!panel->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Apply)->isEnabled(),"Clean body Apply remains active");
      const int appliedUndo=source->document()->availableUndoSteps();QTest::mouseClick(panel->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Apply),Qt::LeftButton);events();require(source->toPlainText()==applied&&source->document()->availableUndoSteps()==appliedUndo,"Idempotent body Apply changed source/history");
      child->close();events();source->undo();require(source->toPlainText()==setup&&window.buildScript(),"Body orbit Undo failed");source->redo();require(source->toPlainText()==applied&&window.buildScript(),"Body orbit Redo failed");native(21546,ellipse);
      QJsonArray phases;
      auto science=[&](const QString &phase,double epoch,const State &elements) {
         native(epoch,elements);const auto text=source->toPlainText();
         require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==text,"Body orbit Unicode round trip failed");native(epoch,elements);
         require(window.runMission()==MainWindow::RunResult::Completed,"Body-orbit witness mission failed: "+messages->toPlainText());events();require(QString::fromUtf8(read(report)).trimmed()=="42"&&source->toPlainText()==text,"Body orbit changed continuation witness or source");
         auto *p=body(),*center=body(parent);const double mu=center->GetGravitationalConstant()+bodyMu;require(std::abs(center->GetGravitationalConstant()-(nested?bodyMu:gravity))<1e-9,"Oracle gravitational input differs");
         const bool circular=elements[1]==0;const double period=circular?2*std::acos(-1.)*std::sqrt(elements[0]*elements[0]*elements[0]/mu):0;
         const std::vector<double> times=circular?std::vector<double>{0.,period/4,-period/4,30*period+37.25,-30*period-37.25}:std::vector<double>{0.,600.,-400.,1200.};
         QJsonArray states;double maxPosition=0,maxVelocity=0,maxAcceleration=0;
         for (double dt:times) {
            GmatTime time(epoch);time.AddSeconds(dt);const auto expected=circular?initial(elements,mu,360*dt/period):rk4(initial(elements,mu),dt,mu);
            const auto parentState=center->GetMJ2000State(time),actual=p->GetMJ2000State(time)-parentState;
            const auto realBody=p->GetMJ2000State(A1Mjd(time.GetMjd()));const auto realParent=center->GetMJ2000State(A1Mjd(time.GetMjd()));const auto realState=realBody-realParent;
            const auto acceleration=p->GetMJ2000Acceleration(time)-center->GetMJ2000Acceleration(time);const double radius=std::hypot(expected[0],expected[1],expected[2]);
            QJsonArray observed,observedReal;
            State parentGt,parentReal,absoluteReal;for(int i=0;i<6;++i){parentGt[i]=parentState[i];parentReal[i]=realParent[i];absoluteReal[i]=realBody[i];}
            for(int i=0;i<6;++i)if(!std::isfinite(realState[i])||std::abs(realState[i]-expected[i])>(i<3?2e-6:2e-9))write(directory+"/"+phase+"-oracle-failure.json",QJsonDocument(QJsonObject{{"seconds",dt},{"epoch_real",time.GetMjd()},{"expected",json(expected)},{"parent_GmatTime",json(parentGt)},{"parent_A1",json(parentReal)},{"body_A1_absolute",json(absoluteReal)}}).toJson());
            for (int i=0;i<6;++i) {const double error=std::abs(actual[i]-expected[i]),realError=std::abs(realState[i]-expected[i]);require(std::isfinite(actual[i])&&std::isfinite(realState[i])&&error<=(i<3?2e-6:2e-9)&&realError<=(i<3?2e-6:2e-9),"Body orbit differs from independent Cartesian oracle: "+phase+" component "+QString::number(i)+" error "+number(error)+" A1 error "+number(realError));if(i<3)maxPosition=std::max({maxPosition,error,realError});else maxVelocity=std::max({maxVelocity,error,realError});observed.append(actual[i]);observedReal.append(realState[i]);}
            for (int i=0;i<3;++i) {const double error=std::abs(acceleration[i]+mu*expected[i]/(radius*radius*radius));require(error<=1e-13,"Body acceleration differs from inverse-square oracle");maxAcceleration=std::max(maxAcceleration,error);}
            if (nested) {
               State parentElements{50000,0,0,0,0,0};const double elapsed=(time.GetMjd()-21545)*86400,parentPeriod=2*std::acos(-1.)*std::sqrt(50000.*50000.*50000./(gravity+bodyMu));
               const auto expectedParent=initial(parentElements,gravity+bodyMu,360*elapsed/parentPeriod);const auto combined=p->GetMJ2000State(time)-body("Earth")->GetMJ2000State(time);
               for (int i=0;i<6;++i)require(std::abs(combined[i]-expectedParent[i]-expected[i])<=(i<3?2e-5:2e-8),"Nested body parent translation differs from independent circles/conic");
            }
            states.append(QJsonObject{{"seconds",dt},{"expected",json(expected)},{"actual",observed},{"actual_A1",observedReal}});
         }
         phases.append(QJsonObject{{"phase",phase},{"type",kind},{"parent",parent},{"epoch",epoch},{"elements",json(elements)},{"mu",mu},{"states",states},{"max_position_error_km",maxPosition},{"max_velocity_error_km_s",maxVelocity},{"max_acceleration_error_km_s2",maxAcceleration}});
         write(directory+"/states.json",QJsonDocument(phases).toJson());write(directory+"/"+phase+".script",text.toUtf8());
      };
      science("ellipse",21546,ellipse);
      const State hyperbola{-30000,1.4,47,112,38,-20};open();mode("TwoBodyPropagation");set(21545,hyperbola);click(panel->findChild<QDialogButtonBox*>(),QDialogButtonBox::Apply);child->close();events();science("hyperbola",21545,hyperbola);
      const State circle{20000,0,kind=="Moon"?0.:kind=="Asteroid"?180.:kind=="Comet"?90.:31.,233,17,40};open();mode("TwoBodyPropagation");set(21545,circle);click(panel->findChild<QDialogButtonBox*>(),QDialogButtonBox::Apply);child->close();events();science("circular-multiple-revolution",21545,circle);
      open();mode("SPICE");click(panel->findChild<QDialogButtonBox*>(),QDialogButtonBox::Apply);child->close();events();
      const auto spiceSource=source->toPlainText();require(body()->GetStringParameter(body()->GetParameterID("PosVelSource"))=="SPICE","Committed SPICE return failed");
      require(window.saveScriptTo(saved)&&window.loadScript(saved)&&window.buildScript()&&source->toPlainText()==spiceSource,"SPICE mode Unicode round trip failed");
      require(window.runMission()==MainWindow::RunResult::Completed&&QString::fromUtf8(read(report)).trimmed()=="42","Restored SPICE witness failed");events();
      const auto restoredSpice=body()->GetMJ2000State(GmatTime(21545.));QJsonArray beforeSpice,afterSpice;
      for(int i=0;i<6;++i){require(std::isfinite(restoredSpice[i])&&std::abs(restoredSpice[i]-baselineSpice[i])<=1e-9,"TwoBody/SPICE switching changed the original kernel state");beforeSpice.append(baselineSpice[i]);afterSpice.append(restoredSpice[i]);}
      write(directory+"/mode-restoration.json",QJsonDocument(QJsonObject{{"initial_SPICE",beforeSpice},{"restored_SPICE",afterSpice},{"native_kernel",kernel},{"source",spiceSource}}).toJson());
      write(directory+"/proof.json",QJsonDocument(QJsonObject{{"case",selected},{"type",kind},{"parent",parent},{"status","passed"},{"actual_tree_editor",true},{"required_modes_available",true},{"committed_SPICE_return",true},{"native_SPICE_state_restored",true},{"orbit_fields",7},{"draft_mode_round_trip",true},{"cancel_discard_atomic",true},{"invalid_GUI_rejections",invalid.size()},{"apply_undo_redo_idempotent",true},{"unicode_persistence",true},{"mission_witness",42},{"precision_and_Real_queries",true},{"nested_parent",nested},{"numerical_phases",phases.size()},{"native_OS_input_qualified",false},{"new_requirement_credit",0}}).toJson());
      source->document()->setModified(false);std::cout<<selected.toStdString()<<" body orbit passed\n";return 0;
   } catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n';}
     catch (std::exception &error) {std::cerr<<"FAIL "<<error.what()<<'\n';}
   return 1;
}
