#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "Propagate.hpp"
#include "AcceptedStepHistory.hpp"
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
using namespace formaccept;

static Propagator *owned(GmatBase *object)
{
   auto *setup=dynamic_cast<PropSetup *>(object);require(setup&&setup->GetPropagator(),"Bulirsch owned propagator missing");return setup->GetPropagator();
}
static void reveal(QWidget *input)
{
   require(input,"Bulirsch input absent");
   for(auto *p=input;p;p=p->parentWidget()){
      for(auto *tabs:input->window()->findChildren<QTabWidget *>())if(tabs->indexOf(p)>=0)tabs->setCurrentWidget(p);
      if(auto *scroll=qobject_cast<QScrollArea *>(p))scroll->ensureWidgetVisible(input);
   }
   events();require(input->isVisible()&&input->isEnabled(),"Bulirsch input inaccessible");acceptanceView(*input);
}
static void type(ResourceEditor &panel,const QString &field,const QString &value)
{
   auto *line=panel.findChild<QLineEdit *>("resource_"+field);reveal(line);line->setFocus();
   QTest::keyClick(line,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(line,value);QTest::keyClick(line,Qt::Key_Tab);events();require(line->text()==value,"Typed Bulirsch value changed");
}
static ResourceEditor *open(Rig &rig)
{
   auto items=rig.resources->findItems("P",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Bulirsch tree item ambiguous");
   rig.resources->itemDoubleClicked(items.first(),0);events();auto *child=rig.area->activeSubWindow();
   auto *panel=child?dynamic_cast<ResourceEditor *>(child->widget()):nullptr;require(panel,"Bulirsch actual resource editor missing");return panel;
}
static void apply(ResourceEditor &panel)
{
   auto *buttons=panel.findChild<QDialogButtonBox *>("resourceButtons");require(buttons,"Bulirsch buttons absent");
   auto *button=buttons->button(QDialogButtonBox::Apply);reveal(button);QTest::mouseClick(button,Qt::LeftButton);events();
}
static void settings(GmatBase *object,double accuracy,double floor)
{
   auto *p=owned(object);require(p->GetTypeName()=="BulirschStoer"&&p->GetRealParameter("Accuracy")==accuracy
      &&p->GetRealParameter("MinimumTolerance")==floor&&p->GetRealParameter("MinimumReduction")==.6
      &&p->GetRealParameter("MaximumReduction")==.0001,"Bulirsch required setting/owner changed");
}
static QString fixture(Rig &rig,int direction,double accuracy,double floor,double minimum=.6,double maximum=.0001)
{
   auto text=rig.script({"One"},false);text.replace("P.Type = PrinceDormand78;","P.Type = BulirschStoer;");
   text.replace("P.Accuracy = 0.000000000001;","P.Accuracy = "+number(accuracy)+";\nP.MinimumTolerance = "+number(floor)+";\nP.MinimumReduction = "+number(minimum)+";\nP.MaximumReduction = "+number(maximum)+";");
   text.replace("P.InitialStepSize = 10;","P.InitialStepSize = 10;\nP.MinStep = 0;\nP.MaxStep = 2700;");
   text.replace("P(One) {One.ElapsedSecs = 60}",QString(direction<0?"BackProp P(One) {One.ElapsedSecs = -10}":"P(One) {One.ElapsedSecs = 10}"));return text;
}
static QJsonObject numerical(Rig &rig,int direction,const QString &name,double requested,double floor,bool cold)
{
   const auto input=fixture(rig,direction,cold?requested:1e-12,cold?floor:0.);
   rig.source->setPlainText(input);require(rig.window.buildScript(),"Bulirsch numerical fixture failed to build");
   require(rig.window.runMission()==MainWindow::RunResult::Completed,"Bulirsch numerical warmup failed");events();
   auto *command=Moderator::Instance()->GetFirstCommand();while(command&&!command->IsOfType("Propagate"))command=command->GetNext();
   require(command,"Bulirsch runtime command absent");auto *setup=command->GetClone(0);auto *p=owned(setup);
   if(!cold){p->SetRealParameter("Accuracy",requested);p->SetRealParameter("MinimumTolerance",floor);}
   settings(setup,requested,floor);require(std::abs(p->GetTime()-direction*10.)<1e-6,"Bulirsch warmup epoch differs");
   std::vector<AcceptedStepHistory::Step> captured;
   {AcceptedStepHistory::Transaction transaction(captured);require(p->Step(direction*1800.),"Bulirsch direct interval failed");transaction.Commit();}
   require(!captured.empty()&&std::abs(p->GetTime()-direction*1810.)<1e-6,"Bulirsch direct interval incomplete");
   QJsonArray steps;double maximumPosition=0,maximumVelocity=0;
   for(const auto &row:captured){
      require(row.integrator==p&&row.model&&row.state.size()==6&&direction*(row.endSeconds-row.startSeconds)>0,"Bulirsch primitive identity/direction invalid");
      const auto expected=circular(orbits[0],row.endSeconds);QJsonArray state,truth;
      for(int c=0;c<6;++c){const double error=std::abs(row.state[c]-expected[c]);require(std::isfinite(row.state[c])&&error<(c<3?1e-3:1e-6),"Bulirsch floor endpoint differs from independent circle");
         if(c<3)maximumPosition=std::max(maximumPosition,error);else maximumVelocity=std::max(maximumVelocity,error);state.append(row.state[c]);truth.append(expected[c]);}
      steps.append(QJsonObject{{"start",row.startSeconds},{"end",row.endSeconds},{"state",state},{"expected",truth}});
   }
   require(rig.source->toPlainText()==input,"Bulirsch numerical execution changed source");
   rig.save(name+".script",input.toUtf8());rig.save(name+"-warmup.csv",read(rig.report));
   const QJsonObject result{{"case",name},{"requested_accuracy",requested},{"floor",floor},{"configured_before_initialization",cold},
      {"steps",steps},{"max_position_error_km",maximumPosition},{"max_velocity_error_km_s",maximumVelocity}};
   rig.save(name+"-states.json",QJsonDocument(result).toJson());return result;
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("BulirschFloorAcceptance");
   try{
      require(argc==3,"Supply startup and direction");Rig rig(QFileInfo(argv[1]).absoluteFilePath());const QString selected=argv[2];require(selected=="Forward"||selected=="Backward","Unknown direction");const int direction=selected=="Forward"?1:-1;
      const auto original=fixture(rig,direction,1e-12,0,.7,.00001);rig.source->setPlainText(original);require(rig.window.buildScript(),"Bulirsch GUI fixture failed");
      auto *native=Moderator::Instance()->GetConfiguredObject("P");const auto generated=native->GetGeneratingString(Gmat::SCRIPTING);auto *panel=open(rig);
      const auto fields=resourceProperties(*owned(native));require(std::any_of(fields.begin(),fields.end(),[](const auto &f){return f.name=="MinimumTolerance"&&f.unit=="dimensionless";}),"Bulirsch floor units/metadata absent");
      type(*panel,"MinimumReduction","0.5");type(*panel,"MaximumReduction","0.0002");type(*panel,"MinimumTolerance","1e-8");
      require(panel->hasChanges()&&rig.source->toPlainText()==original&&native->GetGeneratingString(Gmat::SCRIPTING)==generated,"Bulirsch draft mutated applied source/native");
      auto *combo=panel->findChild<QComboBox *>("resource_Type");require(combo,"Bulirsch type choice absent");reveal(combo);combo->setCurrentText("RungeKutta89");combo->setCurrentText("BulirschStoer");events();
      require(panel->findChild<QLineEdit *>("resource_MinimumTolerance")->text()=="1e-8","Type roundtrip lost floor draft");
      auto *child=rig.area->activeSubWindow();std::exception_ptr closeFailure;QTimer closeTimer;closeTimer.setInterval(5);
      QObject::connect(&closeTimer,&QTimer::timeout,&rig.window,[&]{if(auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget())){closeTimer.stop();try{auto *button=box->button(QMessageBox::Discard);require(button,"Bulirsch discard choice missing");acceptanceView(*box);QTest::mouseClick(button,Qt::LeftButton);}catch(...){closeFailure=std::current_exception();box->reject();}}});
      closeTimer.start();require(child->close(),"Bulirsch dirty close rejected");closeTimer.stop();events();if(closeFailure)std::rethrow_exception(closeFailure);
      require(rig.source->toPlainText()==original&&Moderator::Instance()->GetConfiguredObject("P")==native&&native->GetGeneratingString(Gmat::SCRIPTING)==generated,"Discard changed applied Bulirsch graph");
      panel=open(rig);type(*panel,"MinimumReduction","0.6");type(*panel,"MaximumReduction","0.0001");type(*panel,"MinimumTolerance","1e-8");apply(*panel);
      require(!panel->hasChanges(),"Bulirsch Apply failed");const auto applied=rig.source->toPlainText();settings(Moderator::Instance()->GetConfiguredObject("P"),1e-12,1e-8);rig.closePanels();
      rig.source->undo();require(rig.source->toPlainText()==original&&rig.window.buildScript(),"Bulirsch exact Undo failed");rig.source->redo();require(rig.source->toPlainText()==applied&&rig.window.buildScript(),"Bulirsch exact Redo failed");
      const auto saved=rig.files.filePath("Bulirsch floor Ω.script");require(rig.window.saveScriptTo(saved)&&rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==applied,"Bulirsch Unicode persistence failed");
      panel=open(rig);native=Moderator::Instance()->GetConfiguredObject("P");const auto configured=native->GetGeneratingString(Gmat::SCRIPTING);const int history=rig.source->document()->availableUndoSteps();int invalid=0;
      for(const auto &bad:QStringList{"-0.1","1.1","nan"}){type(*panel,"MinimumTolerance",bad);apply(*panel);require(panel->hasChanges()&&rig.source->toPlainText()==applied&&Moderator::Instance()->GetConfiguredObject("P")==native&&native->GetGeneratingString(Gmat::SCRIPTING)==configured&&rig.source->document()->availableUndoSteps()==history,"Invalid floor changed source/native/history");++invalid;}
      type(*panel,"MinimumTolerance","1e-8");rig.closePanels();
      std::unique_ptr<GmatBase> copy(Moderator::Instance()->GetConfiguredObject("P")->Clone());settings(copy.get(),1e-12,1e-8);std::unique_ptr<GmatBase> nested(copy->Clone());settings(nested.get(),1e-12,1e-8);
      std::unique_ptr<GmatBase> direct(owned(nested.get())->Clone());require(direct->GetRealParameter("Accuracy")==1e-12&&direct->GetRealParameter("MinimumTolerance")==1e-8,"Direct Bulirsch clone lost requested/floor values");
      rig.save("GUI-applied.script",applied.toUtf8());QJsonArray results;
      for(auto item:QList<QPair<QString,QPair<double,double>>>{{"disabled",{1e-12,0}},{"below-request",{1e-12,1e-15}},{"raised-initialized",{1e-12,1e-8}},{"effective-initialized",{1e-8,0}},{"cold-floor",{1e-12,1e-8}},{"cold-effective",{1e-8,0}}})results.append(numerical(rig,direction,item.first,item.second.first,item.second.second,item.first.startsWith("cold")));
      require(results[0].toObject()["steps"]==results[1].toObject()["steps"],"Below-request floor changed strict operational integration");
      require(results[2].toObject()["steps"]==results[3].toObject()["steps"],"Initialized floor differs from equivalent requested tolerance");
      require(results[4].toObject()["steps"]==results[5].toObject()["steps"],"Cold floor differs from equivalent requested tolerance");
      require(results[0].toObject()["steps"]!=results[2].toObject()["steps"],"Floor had no observable operational effect");
      rig.save("numerical-matrix.json",QJsonDocument(results).toJson());
      rig.save("proof.json",QJsonDocument(QJsonObject{{"outcome","passed"},{"case",selected},{"required_fields",QJsonArray{"MinimumReduction","MaximumReduction","MinimumTolerance"}},
         {"GUI_type_draft_discard",true},{"GUI_apply_undo_redo_unicode",true},{"invalid_GUI_rejections",invalid},{"nested_direct_clones",true},
         {"initialized_and_cold_equivalence",true},{"strict_below_floor_equivalence",true},{"observable_operational_floor_effect",true},{"numerical_cases",results.size()},{"new_requirement_credit",0}}).toJson());return 0;
   }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
