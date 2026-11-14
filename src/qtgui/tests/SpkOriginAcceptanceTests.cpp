#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "TimeSystemConverter.hpp"
#include "SpiceUsr.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
#include <memory>
using namespace formaccept;

static void reveal(QWidget *input)
{
   require(input,"SPK setting input absent");
   for(auto *p=input;p;p=p->parentWidget()){
      for(auto *tabs:input->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
      if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(input);
   }
   events();require(input->isVisible()&&input->isEnabled(),"SPK setting input inaccessible");acceptanceView(*input);
}
static Propagator *owned(GmatBase *object)
{
   auto *setup=dynamic_cast<PropSetup *>(object);require(setup&&setup->GetPropagator(),"Owned SPK propagator absent");return setup->GetPropagator();
}
static ResourceEditor *open(Rig &rig)
{
   auto items=rig.resources->findItems("Read",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"SPK tree item ambiguous");
   reveal(rig.resources);rig.resources->scrollToItem(items.first());events();const auto point=rig.resources->visualItemRect(items.first()).center();
   QTest::mouseClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();QTest::mouseDClick(rig.resources->viewport(),Qt::LeftButton,Qt::NoModifier,point);events();
   auto *child=rig.area->activeSubWindow();auto *panel=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr;require(panel,"Actual SPK resource editor absent");return panel;
}
static void type(ResourceEditor &panel,const QString &name,const QString &text)
{
   auto *line=panel.findChild<QLineEdit *>("resource_"+name);reveal(line);require(!line->isReadOnly(),"SPK origin/step remained read-only");
   line->setFocus();QTest::keyClick(line,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(line,text);QTest::keyClick(line,Qt::Key_Tab);events();require(line->text()==text,"Typed SPK setting changed");
}
static void choose(ResourceEditor &panel,const QString &name,const QString &text,bool editable=false)
{
   auto *box=panel.findChild<QComboBox *>("resource_"+name);reveal(box);
   if(editable){require(box->isEditable(),"SPK start epoch cannot be typed");box->setFocus();QTest::keyClick(box,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(box,text);QTest::keyClick(box,Qt::Key_Tab);events();}
   else if(box->currentText()!=text){
      QTest::qWait(QApplication::doubleClickInterval()+20);const int index=box->findText(text);require(index>=0,"SPK choice absent");
      QTest::mouseClick(box,Qt::LeftButton,Qt::NoModifier,QPoint(box->width()-10,box->height()/2));events();require(box->view()->isVisible(),"SPK popup did not open");
      acceptanceView(*box->view()->window());QTest::keyClick(box->view(),Qt::Key_Home);for(int i=0;i<index;++i)QTest::keyClick(box->view(),Qt::Key_Down);QTest::keyClick(box->view(),Qt::Key_Return);events();
   }
   require(box->currentText()==text,"SPK choice did not commit");
}
static void apply(ResourceEditor &panel)
{
   auto *buttons=panel.findChild<QDialogButtonBox *>("resourceButtons");auto *button=buttons?buttons->button(QDialogButtonBox::Apply):nullptr;
   reveal(button);QTest::mouseClick(button,Qt::LeftButton);events();
}
static void settings(GmatBase *object,const QString &origin,const QString &start)
{
   auto *p=owned(object);require(p->GetTypeName()=="SPK"&&p->GetStringParameter("CentralBody")==origin.toStdString()
      &&p->GetRealParameter("StepSize")==75&&p->GetStringParameter("StartEpoch")==start.toStdString()
      &&p->GetStringParameter("EpochFormat")=="TDBModJulian","SPK required settings/owned clone changed");
}
static void closeDiscard(Rig &rig)
{
   auto *child=rig.area->activeSubWindow();require(child,"SPK pending window absent");std::exception_ptr error;QTimer timer;timer.setInterval(5);
   QObject::connect(&timer,&QTimer::timeout,&rig.window,[&]{if(auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget())){timer.stop();try{auto *button=box->button(QMessageBox::Discard);require(button,"SPK Discard button absent");acceptanceView(*box);QTest::mouseClick(button,Qt::LeftButton);}catch(...){error=std::current_exception();box->reject();}}});
   timer.start();require(child->close(),"SPK dirty close rejected");timer.stop();events();if(error)std::rethrow_exception(error);
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("SpkOriginAcceptance");
   try{
      require(argc==4,"Supply startup, origin and epoch profile");Rig rig(QFileInfo(argv[1]).absoluteFilePath());const QString origin=argv[2],epochCase=argv[3];
      require(QStringList{"Earth","Mars","Probe"}.contains(origin)&&QStringList{"Spacecraft","Explicit","KernelStart"}.contains(epochCase),"Unknown SPK setting profile");
      const QString start=epochCase=="Explicit"?number(21545.+240./86400):epochCase=="Spacecraft"?"FromSpacecraft":"EphemStart";
      const double startSeconds=epochCase=="Explicit"?240:epochCase=="Spacecraft"?120:0,expectedSeconds=startSeconds+75;
      const auto kernel=rig.files.filePath("Affine origin Ω.bsp"),report=rig.files.filePath("SPK origin Δ.csv");SpiceInt handle;
      spkopn_c(kernel.toUtf8().constData(),"Independent affine origin witness",0,&handle);require(!failed_c(),"SPK affine kernel open failed");
      SpiceDouble epochs[]={0,150,900},states[3][6];for(int i=0;i<3;++i){const double t=epochs[i];const double row[]={7000+t,-100+2*t,200-.5*t,1,2,-.5};for(int j=0;j<6;++j)states[i][j]=row[j];}
      spkw09_c(handle,-199990,399,"J2000",0,900,"AFFINE EARTH STATE",1,3,states,epochs);spkcls_c(handle);require(!failed_c(),"SPK affine kernel write failed");rig.save("affine-origin.bsp",read(kernel));
      QString source="SolarSystem.EphemerisUpdateInterval = 0;\nCreate Moon Probe;\nProbe.CentralBody = Earth;\nProbe.Mu = 1;\nProbe.PosVelSource = TwoBodyPropagation;\nProbe.InitialEpoch = 21545;\nProbe.SMA = 20000;\nProbe.ECC = 0.2;\nProbe.INC = 31;\nProbe.RAAN = 50;\nProbe.AOP = 21;\nProbe.TA = 40;\nCreate CoordinateSystem EarthIcrf;\nEarthIcrf.Origin = Earth;\nEarthIcrf.Axes = ICRF;\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthIcrf;\nSat.NAIFId = -199990;\nSat.DateFormat = TDBModJulian;\nSat.Epoch = '"+number(21545.+120./86400)+"';\nSat.OrbitSpiceKernelName = {'"+kernel+"'};\nCreate Propagator Read;\nRead.Type = SPK;\nRead.StepSize = 60;\nRead.EpochFormat = TDBModJulian;\nRead.StartEpoch = EphemStart;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\n";
      const QString tail="BeginMissionSequence;\nPropagate 'One affine step' Read(Sat);\nReport Values Sat.A1ModJulian Sat.EarthIcrf.X Sat.EarthIcrf.Y Sat.EarthIcrf.Z Sat.EarthIcrf.VX Sat.EarthIcrf.VY Sat.EarthIcrf.VZ;\n% Preserve tail Ω\n";source+=tail;
      rig.source->setPlainText(source);require(rig.window.buildScript(),"SPK initial fixture build failed");rig.save("initial.script",source.toUtf8());
      auto *native=Moderator::Instance()->GetConfiguredObject("Read");require(owned(native)->GetStringParameter("CentralBody")=="Earth","Default SPK origin changed");
      const auto generated=native->GetGeneratingString(Gmat::SCRIPTING);const int history=rig.source->document()->availableUndoSteps();
      auto pristine=[&]{return rig.source->toPlainText()==source&&rig.source->document()->availableUndoSteps()==history&&Moderator::Instance()->GetConfiguredObject("Read")==native&&native->GetGeneratingString(Gmat::SCRIPTING)==generated;};
      auto fill=[&](ResourceEditor &panel){type(panel,"CentralBody",origin);type(panel,"StepSize","75");choose(panel,"EpochFormat","TDBModJulian");choose(panel,"StartEpoch",start,epochCase=="Explicit");};
      auto *panel=open(rig);type(*panel,"CentralBody",origin=="Earth"?"Mars":origin);type(*panel,"StepSize","75");choose(*panel,"StartEpoch",start,epochCase=="Explicit");
      require(pristine()&&panel->hasChanges(),"SPK draft changed source/native/history");choose(*panel,"Type","RungeKutta89");choose(*panel,"Type","SPK");
      require(panel->findChild<QLineEdit *>("resource_CentralBody")->text()==(origin=="Earth"?"Mars":origin)&&panel->findChild<QLineEdit *>("resource_StepSize")->text()=="75"&&panel->findChild<QComboBox *>("resource_StartEpoch")->currentText()==start,"Type roundtrip lost SPK setting draft");closeDiscard(rig);require(pristine(),"SPK Discard changed applied source/native/history");
      panel=open(rig);fill(*panel);int invalid=0;
      for(const auto &bad:QStringList{"MissingBody","Sat","Moon"}){type(*panel,"CentralBody",bad);apply(*panel);auto *label=panel->findChild<QLabel *>("resourceStatus");require(pristine()&&panel->hasChanges()&&label&&!label->text().isEmpty(),"Invalid SPK origin changed state/history or lost diagnostic");++invalid;}
      type(*panel,"CentralBody",origin);apply(*panel);require(!panel->hasChanges(),"SPK settings Apply failed");const auto applied=rig.source->toPlainText();require(applied!=source&&applied.endsWith(tail),"SPK Apply changed tail or lost settings");settings(Moderator::Instance()->GetConfiguredObject("Read"),origin,start);rig.closePanels();
      rig.source->undo();require(rig.source->toPlainText()==source&&rig.window.buildScript(),"SPK exact Undo failed");rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript(),"SPK exact Redo failed");
      const auto saved=rig.files.filePath("SPK origin Ω.script");require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==applied,"SPK Unicode persistence failed");
      std::unique_ptr<GmatBase> clone(Moderator::Instance()->GetConfiguredObject("Read")->Clone());settings(clone.get(),origin,start);std::unique_ptr<GmatBase> nested(clone->Clone());settings(nested.get(),origin,start);std::unique_ptr<GmatBase> direct(owned(nested.get())->Clone());require(direct->GetStringParameter("CentralBody")==origin.toStdString()&&direct->GetRealParameter("StepSize")==75&&direct->GetStringParameter("StartEpoch")==start.toStdString(),"Direct SPK clone lost settings");
      const auto result=rig.window.runMission();events();rig.save("messages.txt",rig.messages->toPlainText().toUtf8());require(result==MainWindow::RunResult::Completed,"SPK GUI-configured mission failed");rig.save("applied.script",applied.toUtf8());rig.save("state.csv",read(report));
      const auto columns=QString::fromUtf8(read(report)).trimmed().split(',');require(columns.size()==7,"SPK witness report width differs");const double epoch=columns[0].toDouble();
      const double seconds=(TimeSystemConverter::Instance()->Convert(epoch,TimeSystemConverter::A1MJD,TimeSystemConverter::TDBMJD,2430000.)-21545)*86400;
      require(std::abs(seconds-expectedSeconds)<2e-5,"SPK step/start epoch did not change execution");const double expected[]={7000+seconds,-100+2*seconds,200-.5*seconds,1,2,-.5};QJsonArray observed,truth;
      for(int i=0;i<6;++i){bool ok;const double value=columns[i+1].toDouble(&ok);require(ok&&std::isfinite(value)&&std::abs(value-expected[i])<(i<3?2e-5:2e-9),"SPK origin differs from independent affine state");observed.append(value);truth.append(expected[i]);}
      auto *command=Moderator::Instance()->GetFirstCommand();while(command&&!command->IsOfType("Propagate"))command=command->GetNext();require(command,"SPK runtime command absent");auto *setup=command->GetClone(0);settings(setup,origin,start);auto *p=owned(setup);require(p->GetPropOriginName()==origin.toStdString()&&p->GetPropOrigin()&&p->GetPropOrigin()->GetName()==origin.toStdString(),"Runtime SPK origin owner differs");
      auto *system=Moderator::Instance()->GetSolarSystemInUse();auto *runtimeSat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));require(runtimeSat,"Runtime SPK spacecraft absent");
      const auto preciseEpoch=runtimeSat->GetEpochGT();
      const auto delta=p->GetPropOrigin()->GetMJ2000State(preciseEpoch)-system->GetBody("Earth")->GetMJ2000State(preciseEpoch);
      const auto *raw=p->GetState();const auto *j2000=p->GetJ2KState();require(raw&&j2000&&p->GetDimension()==6,"SPK native vectors absent");QJsonArray nativeState,originOffset,nativeJ2000;
      QJsonArray diagnostic;for(int i=0;i<6;++i)diagnostic.append(QJsonObject{{"component",i},{"raw_FK5",raw[i]},{"Earth_J2000_FK5",j2000[i]},{"offset_FK5",delta[i]},{"difference",raw[i]-(j2000[i]-delta[i])}});
      rig.save("internal-state-diagnostic.json",QJsonDocument(diagnostic).toJson());
      for(int i=0;i<6;++i){require(std::abs(raw[i]-(j2000[i]-delta[i]))<(i<3?2e-5:2e-9),"Native SPK vector differs from selected-origin expectation");nativeState.append(raw[i]);originOffset.append(delta[i]);nativeJ2000.append(j2000[i]);}
      if(origin!="Earth")require(std::abs(raw[0]-j2000[0])+std::abs(raw[1]-j2000[1])+std::abs(raw[2]-j2000[2])>1,"Non-Earth origin had no operational effect");
      require(rig.source->toPlainText()==applied,"SPK Run changed source");
      rig.save("proof.json",QJsonDocument(QJsonObject{{"outcome","passed"},{"origin",origin},{"epoch_profile",epochCase},{"start_epoch",start},{"step_seconds",75},{"expected_seconds",expectedSeconds},{"observed_TDB_seconds",seconds},{"observed_A1Mjd",epoch},{"state",observed},{"expected_state",truth},{"native_relative_state",nativeState},{"native_J2000_FK5_state",nativeJ2000},{"shared_native_origin_offset",originOffset},{"native_precise_A1Mjd",QString::fromStdString(preciseEpoch.ToString())},{"required_fields",QJsonArray{"StepSize","CentralBody","EpochFormat","StartEpoch"}},{"default_Earth_retained",true},{"type_draft_Discard",true},{"Apply_Undo_Redo_Unicode",true},{"invalid_origin_rejections",invalid},{"nested_direct_runtime_clones",true},{"new_requirement_credit",0},{"scope","Actual SPK settings and independent affine Earth-relative state; internal origin-vector coherence uses shared native celestial-body input. No independent physical frame/ephemeris, ABI/platform/native OS input or universal scientific approval."}}).toJson());return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
