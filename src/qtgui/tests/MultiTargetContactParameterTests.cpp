// Native list, reference and result-ownership contracts for multiple targets.
#include "FormationAcceptance.hpp"
#include "ContactLocator.hpp"
#include "ContactEvent.hpp"
#include <functional>
using namespace formaccept;
namespace {
int deletedResults=0;
struct CountedResult : ContactResult { ~CountedResult() override { ++deletedResults; } };
struct OwnedLocator : ContactLocator {
 OwnedLocator():ContactLocator("Owned"){}
 void seed() { contactResults.push_back(new CountedResult); }
 void seedWorker() { targetWorkers.emplace_back(new ContactLocator("Worker")); }
 size_t workers() const { return targetWorkers.size(); }
 size_t observerCount() const { return directObservers.size(); }
};
}
int main(int argc,char **argv){QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("MultiTargetContactParameters");try{
 require(argc==2,"Supply native startup");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
 rig.source->setPlainText("Create Spacecraft Sat Other Third;\nCreate GroundStation Station;\nCreate ContactLocator Events;\nEvents.Target = Sat;\nEvents.AdditionalTargets = {Other, Third};\nEvents.AdditionalTargets = {};\nEvents.RunMode = Disabled;\nBeginMissionSequence;\n");
 require(rig.window.buildScript(),"Additional target input and explicit clear did not parse");
 auto *object=dynamic_cast<ContactLocator*>(Moderator::Instance()->GetConfiguredObject("Events"));require(object,"Native locator missing");
 require(object->GetStringArrayParameter("AdditionalTargets").empty(),"Interpreter ignored explicit empty target list");
 auto *sat=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetConfiguredObject("Sat"));
 auto *other=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetConfiguredObject("Other"));
 auto *third=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetConfiguredObject("Third"));
 auto *station=Moderator::Instance()->GetConfiguredObject("Station");require(sat&&other&&third&&station,"Native fixtures missing");
 require(object->SetRefObject(sat,Gmat::SPACECRAFT,"Sat"),"Primary reference failed");
 int rejected=0;
 auto reject=[&](const std::function<void()> &action,const std::string &fragment){bool caught=false;try{action();}catch(BaseException &e){caught=true;require(e.GetFullMessage().find(fragment)!=std::string::npos,"Wrong native validation diagnostic");}require(caught,"Invalid target selection accepted");++rejected;};
 const auto id=object->GetParameterID("AdditionalTargets");
 require(object->GetParameterType(id)==Gmat::OBJECTARRAY_TYPE&&object->GetPropertyObjectType(id)==Gmat::SPACECRAFT,"Native list metadata wrong");
 require(object->SetStringParameter(id,"Other")&&object->SetStringParameter(id,"Third",1),"Native list setters failed");
 const StringArray expected{"Other","Third"};require(object->GetStringArrayParameter(id)==expected,"Native list order lost");
 reject([&]{object->SetStringParameter(id,"Sat");},"primary Target");
 reject([&]{object->SetStringParameter(id,"Other");},"only once");
 reject([&]{object->SetStringParameter(id,"Other",1);},"only once");
 reject([&]{object->SetStringParameter(id,"Other",-1);},"index out of range");
 reject([&]{object->GetStringParameter(id,-1);},"index out of range");
 reject([&]{object->SetStringParameter(id,"",1);},"name spacecraft");
 reject([&]{object->SetRefObject(station,Gmat::GROUND_STATION,"Other");},"reference spacecraft");
 reject([&]{object->Initialize();},"Unresolved AdditionalTargets");
 require(object->SetRefObject(other,Gmat::SPACECRAFT,"Other")&&object->SetRefObject(third,Gmat::SPACECRAFT,"Third"),"Native target references failed");
 require(object->GetRefObject(Gmat::SPACECRAFT,"Other",0)==other,"Additional target reference inaccessible");
 const auto refs=object->GetRefObjectNameArray(Gmat::SPACECRAFT);for(const auto &name:expected)require(std::find(refs.begin(),refs.end(),name)!=refs.end(),"Additional target omitted from sandbox references");
 const auto oldId=other->GetIntegerParameter("NAIFId");other->SetIntegerParameter("NAIFId",sat->GetIntegerParameter("NAIFId"));
 reject([&]{object->Initialize();},"distinct NAIF IDs");other->SetIntegerParameter("NAIFId",oldId);
 object->SetStringParameter("Observers","Other");reject([&]{object->Initialize();},"also be an observer");object->TakeAction("Clear","Observers");
 ContactLocator copy(*object),assigned("Assigned");assigned=*object;
 require(copy.GetStringArrayParameter(id)==expected&&assigned.GetStringArrayParameter(id)==expected,"Copy/assignment lost target list");
 require(copy.RenameRefObject(Gmat::SPACECRAFT,"Other","Renamed")&&copy.GetStringParameter(id,0)=="Renamed"&&copy.GetRefObject(Gmat::SPACECRAFT,"Renamed",0)==other,"Rename lost spacecraft binding");
 require(copy.TakeAction("Clear","AdditionalTargets")&&copy.GetStringArrayParameter(id).empty()&&object->GetStringArrayParameter(id)==expected,"Clone clear mutated original");
 reject([&]{assigned.SetStringParameter(id,"Sat",0);},"primary Target");
 { OwnedLocator owner; owner.seed(); } require(deletedResults==1,"Locator destructor leaked owned contact results");
 ContactResult results;results.SetObserverName("Station");results.SetNoEvents("No preserved contacts");results.AddEvent(new ContactEvent(21545,21545.01));
 ContactResult resultCopy(results),resultAssigned;resultAssigned=results;
 require(resultCopy.NumberOfEvents()==1&&resultCopy.GetEvent(0)!=results.GetEvent(0)&&resultCopy.GetReportString()==results.GetReportString()&&resultAssigned.GetReportString()==results.GetReportString(),"Contact result copy/assignment lost data or ownership");
 resultCopy.TakeAction("Clear");require(resultCopy.NumberOfEvents()==0&&resultCopy.GetReportString().find("No preserved contacts")!=std::string::npos,"Cleared result lost no-event label");
 reject([&]{resultCopy.GetEvent(0);},"Index out-of-range");
 ContactEvent detailed(21545,21545.1,{"Observer","StartRange"},true);
 detailed.eventNumber=7;detailed.eventSignalPath="Station -> Sat";detailed.eventStation="Station";detailed.eventPass=3;
 detailed.eventTimes={21545,21545.1};detailed.eventRanges={100,200};detailed.eventRangeRates={1,2};detailed.eventAzimuths={3,4};detailed.eventElevations={5,6};detailed.eventMaxElevation=8;detailed.eventMaxElevationEpoch=21545.05;detailed.customReport=true;detailed.columnSpacing=" | ";
 for(auto &column:detailed.reportColumns){column.columnTitle="Title";column.columnWidth=12;column.aerColumn=true;column.cellData={"one","two"};}
 ContactEvent detailCopy(detailed),detailAssigned(0,0);detailAssigned=detailed;
 auto sameDetail=[&](const ContactEvent &value){
  require(value.eventNumber==7&&value.eventSignalPath==detailed.eventSignalPath&&value.eventStation==detailed.eventStation&&value.eventPass==3&&value.eventTimes==detailed.eventTimes&&value.eventRanges==detailed.eventRanges&&value.eventRangeRates==detailed.eventRangeRates&&value.eventAzimuths==detailed.eventAzimuths&&value.eventElevations==detailed.eventElevations&&value.eventMaxElevation==8&&value.eventMaxElevationEpoch==21545.05&&value.customReport&&value.leftJustified&&value.columnSpacing==" | "&&value.reportColumns.size()==2,"Detailed event copy lost arrays/metadata");
  for(size_t i=0;i<2;++i){const auto &column=value.reportColumns[i];const auto &original=detailed.reportColumns[i];require(column.columnName==original.columnName&&column.columnNumber==original.columnNumber&&column.columnTitle==original.columnTitle&&column.columnWidth==original.columnWidth&&column.aerColumn&&column.cellData==original.cellData,"Detailed event copy lost column data");}
 };
 sameDetail(detailCopy);sameDetail(detailAssigned);require(detailCopy.GetReportString()==detailed.GetReportString()&&detailAssigned.GetAERString()==detailed.GetAERString(),"Detailed event copy changed rendered cells");
 detailCopy.reportColumns[0].cellData[0]="changed";require(detailed.reportColumns[0].cellData[0]=="one","Detailed event clone shares cell ownership");
 OwnedLocator bound;bound.SetStringParameter("Target","Sat");bound.SetStringParameter("AdditionalTargets","Other");bound.SetRefObject(other,Gmat::SPACECRAFT,"Other");bound.seedWorker();
 bound.SetRefObject(other,Gmat::SPACECRAFT,"Other");require(bound.workers()==1,"Identical reference unnecessarily invalidated results");
 Spacecraft replacement(*other);bound.SetRefObject(&replacement,Gmat::SPACECRAFT,"Other");require(bound.workers()==0&&bound.GetRefObject(Gmat::SPACECRAFT,"Other",0)==&replacement,"Replaced reference retained stale target worker");
 reject([&]{bound.LocateEvents();},"Initialize all selected");
 OwnedLocator reinit;reinit.Copy(object);reinit.SetSolarSystem(Moderator::Instance()->GetSolarSystemInUse());reinit.SetRefObject(sat,Gmat::SPACECRAFT,"Sat");reinit.SetStringParameter("Observers","Station");reinit.SetRefObject(station,Gmat::GROUND_STATION,"Station");
 require(reinit.Initialize()&&reinit.Initialize()&&reinit.observerCount()==1&&reinit.workers()==2,"Repeated initialization duplicates observers or loses workers");
 require(rejected==13,"Native rejection gate count changed");
 const QJsonObject proof{{"checkpoint","multi-target-contact-parameter-contract"},{"outcome","passed"},{"explicit_empty_parsed",true},{"rejected_inputs",rejected},{"reference_copy_assignment_rename",true},{"detailed_event_value_copy",true},{"rebind_invalidates_worker",true},{"reinitialization_idempotent",true},{"owned_result_deleted",deletedResults},{"facets",QJsonArray{"native-target-list-contract","sandbox-reference-membership","copy-assignment-rename-isolation","owned-contact-result-destruction"}}};
 rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;}
