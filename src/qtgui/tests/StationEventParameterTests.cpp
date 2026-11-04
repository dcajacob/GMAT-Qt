// Native configuration/error paths for the new field, independent of GUI replay.
#include "FormationAcceptance.hpp"
#include "EphemManager.hpp"
#include <functional>
#include <limits>
#include <memory>
using namespace formaccept;
int main(int argc,char **argv){QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("StationEventParameters");try{
 require(argc==2,"Supply native startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
 rig.source->setPlainText("Create Spacecraft Probe;\nCreate ContactLocator Events;\nEvents.Target = Probe;\nEvents.RunMode = Disabled;\nEvents.EventTypes = {};\nBeginMissionSequence;\n");
 require(rig.window.buildScript(),"Explicit empty event-list input did not parse");
 auto *object=Moderator::Instance()->GetConfiguredObject("Events");require(object,"Native locator missing");
 require(object->GetStringArrayParameter("EventTypes").empty(),"Interpreter ignored explicit empty EventTypes");
 int rejected=0;
 auto reject=[&](const std::function<void()> &action,const std::string &fragment){bool caught=false;try{action();}catch(BaseException &e){caught=true;require(e.GetFullMessage().find(fragment)!=std::string::npos,"Wrong native validation diagnostic");}require(caught,"Invalid native selection/search accepted");++rejected;};
 reject([&]{object->Initialize();},"at least one");
 const auto id=object->GetParameterID("EventTypes");
 require(object->GetPropertyEnumStrings(id)==StringArray({"ContactIntervals","StationRise","StationSet"}),"Native event-type inventory differs");
 require(object->SetStringParameter(id,"StationRise"),"Native rise selection failed");
 require(object->SetStringParameter(id,"StationSet",1),"Native indexed set selection failed");
 const StringArray both{"StationRise","StationSet"};require(object->GetStringArrayParameter(id)==both,"Native selection retained implicit intervals");
 reject([&]{object->SetStringParameter(id,"NotAnEvent");},"EventTypes entries");
 reject([&]{object->SetStringParameter(id,"StationRise");},"only once");
 reject([&]{object->SetStringParameter(id,"StationRise",1);},"only once");
 reject([&]{object->SetStringParameter(id,"StationRise",-1);},"index out of range");
 reject([&]{object->GetStringParameter(id,-1);},"index out of range");
 require(object->GetStringArrayParameter(id)==both,"Rejected native setter mutated selection");
 std::unique_ptr<GmatBase> clone(object->Clone());require(clone->GetStringArrayParameter(id)==both,"Native clone lost event selection");
 require(clone->TakeAction("Clear","EventTypes")&&clone->GetStringArrayParameter(id).empty()&&object->GetStringArrayParameter(id)==both,"Native clone clear affected original");
 auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("Probe"));require(sat,"Validation spacecraft missing");
 EphemManager manager(false);manager.SetObject(sat);RealArray rises{123},sets{456};
 auto invalid=[&](double elevation,double start,double stop,double step,const std::string &observer="399"){
  manager.GetStationElevationCrossings(observer,elevation,"ECLIPJ2000",{},"NONE",start,stop,false,false,false,step,rises,sets);
 };
 reject([&]{invalid(91,21545,21546,1);},"valid elevation");
 reject([&]{invalid(0,21545,21546,0);},"positive finite");
 reject([&]{invalid(0,21546,21545,1);},"valid elevation");
 reject([&]{invalid(std::numeric_limits<double>::quiet_NaN(),21545,21546,1);},"valid elevation");
 reject([&]{invalid(0,21545,21546,std::numeric_limits<double>::infinity());},"positive finite");
 reject([&]{invalid(0,21545,21546,1,"not-a-number");},"numeric NAIF ID");
 require(rises.empty()&&sets.empty(),"Rejected crossing search retained stale outputs");
 require(rejected==12,"Native rejection gate count changed");
 const QJsonObject proof{{"checkpoint","station-event-parameter-contract"},{"outcome","passed"},{"explicit_empty_parsed",true},{"empty_initialization_rejected",true},{"native_choices",QJsonArray{"ContactIntervals","StationRise","StationSet"}},{"clone_isolated",true},{"rejected_inputs",rejected},{"stale_outputs_cleared",true},{"facets",QJsonArray{"native-event-selection-serialization-validation-clone","crossing-search-argument-validation"}}};
 rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;}
