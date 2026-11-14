#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "ObjectManagedFunction.hpp"
#include "UserDefinedFunction.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "ODEModel.hpp"
#include "CoordinateSystem.hpp"
#include "AxisSystem.hpp"
#include "Spacecraft.hpp"
#include "Propagate.hpp"
#include <array>
#include "SandboxException.hpp"
#include "ScriptInterpreter.hpp"
#include <QStatusBar>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTabWidget>
#include <QTest>
#include <QTimer>
using namespace formaccept;

static QString pointer(const void *p) { return QString::number(quintptr(p),16); }
static void reveal(QWidget *widget)
{
   require(widget,"Owned resource control absent");
   for (auto *page=widget;page;page=page->parentWidget())
   {
      for (auto *tabs:widget->window()->findChildren<QTabWidget*>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page);
      if (auto *scroll=qobject_cast<QScrollArea*>(page)) scroll->ensureWidgetVisible(widget);
   }
   events(); require(widget->isVisible() && widget->isEnabled(),"Owned resource control unreachable"); acceptanceView(*widget);
}


static const QStringList fields{"X","Y","Z","VX","VY","VZ"};
static std::array<double,6> circle(double r,double t)
{
   const double n=std::sqrt(400000/(r*r*r)),a=n*t,v=n*r;
   return {r*std::cos(a),r*std::sin(a),0,-v*std::sin(a),v*std::cos(a),0};
}
static QString craft(const QString &name,const QString &frame,const std::array<double,6> &state,int id,double mass)
{
   QString code="Create Spacecraft "+name+";\n"+name+".CoordinateSystem = "+frame+";\n"+name+".DateFormat = A1ModJulian;\n"+name+".Epoch = '21545';\n"+name+".DisplayStateType = Cartesian;\n"+name+".DryMass = "+number(mass)+";\n"+name+".NAIFId = "+QString::number(id)+";\n"+name+".NAIFIdReferenceFrame = "+QString::number(id-8000000)+";\n";
   for(int i=0;i<6;++i) code+=name+"."+fields[i]+" = "+number(state[i])+";\n";
   return code;
}
static QString resource(const QString &name,bool prop,bool local)
{
   if(!prop) return "Create CoordinateSystem "+name+";\n"+name+".Origin = Earth;\n"+name+".Axes = "+(local?"MJ2000Ec":"MJ2000Eq")+";\n";
   const QString fm=local?"FunctionFM":"MainFM";
   return "Create ForceModel "+fm+";\n"+fm+".CentralBody = Earth;\n"+fm+".PrimaryBodies = {};\n"+fm+".PointMasses = {Earth};\n"+fm+".Drag = None;\n"+fm+".SRP = Off;\nCreate Propagator "+name+";\n"+name+".FM = "+fm+";\n"+name+".Type = "+(local?"PrinceDormand78":"RungeKutta89")+";\n"+name+".Accuracy = "+(local?"1e-12":"1e-9")+";\n"+name+".InitialStepSize = "+(local?"1":"3")+";\n"+name+".MinStep = 0;\n"+name+".MaxStep = 5;\n";
}
static QJsonObject native(GmatBase *object,bool prop,bool local)
{
   require(object,"Owned native resource absent");
   QJsonObject state{{"type",QString::fromStdString(object->GetTypeName())},{"name",QString::fromStdString(object->GetName())},{"pointer",pointer(object)},{"global",object->IsGlobal()},{"automatic",object->IsAutomaticGlobal()},{"local",object->IsLocal()}};
   require(object->IsAutomaticGlobal()&&object->IsGlobal(),"Owned automatic-global constructor metadata differs");
   if(prop)
   {
      auto *p=dynamic_cast<PropSetup*>(object); require(p&&p->GetPropagator()&&p->GetODEModel(),"Owned PropSetup/components absent");
      auto *integrator=p->GetPropagator(); auto *fm=p->GetODEModel();
      require(integrator->GetTypeName()==(local?"PrinceDormand78":"RungeKutta89")&&integrator->GetRealParameter("Accuracy")== (local?1e-12:1e-9)&&integrator->GetRealParameter("InitialStepSize")== (local?1:3)&&integrator->GetRealParameter("MinStep")==0&&integrator->GetRealParameter("MaxStep")==5&&p->GetStringParameter("FM")== (local?"FunctionFM":"MainFM"),"Owned integrator configuration differs");
      const auto points=fm->GetStringArrayParameter("PointMasses");
      require(fm->GetStringParameter("CentralBody")=="Earth"&&fm->GetStringArrayParameter("PrimaryBodies").empty()&&points.size()==1&&points[0]=="Earth"&&fm->GetStringParameter("Drag")=="None"&&fm->GetOnOffParameter("SRP")=="Off","Owned force configuration differs");
      state["method"]=QString::fromStdString(integrator->GetTypeName()); state["accuracy"]=integrator->GetRealParameter("Accuracy"); state["initial_step"]=integrator->GetRealParameter("InitialStepSize"); state["min_step"]=0; state["max_step"]=5;state["fm"]=QString::fromStdString(p->GetStringParameter("FM"));state["central"]="Earth";state["point_masses"]=QJsonArray{"Earth"};state["primary_bodies"]=QJsonArray{};state["drag"]="None";state["srp"]="Off";state["owned_component_pointer"]=pointer(integrator);
   }
   else
   {
      auto *frame=dynamic_cast<CoordinateSystem*>(object);require(frame&&frame->GetAxisSystem()&&frame->GetStringParameter("Origin")=="Earth"&&frame->GetStringParameter("Axes")== (local?"MJ2000Ec":"MJ2000Eq")&&frame->GetAxisSystem()->GetTypeName()== (local?"MJ2000Ec":"MJ2000Eq"),"Owned frame configuration differs");state["origin"]="Earth";state["axes"]=QString::fromStdString(frame->GetStringParameter("Axes"));state["owned_component_pointer"]=pointer(frame->GetAxisSystem());
   }
   return state;
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("FunctionOwnedScope");
   try
   {
      require(argc==3&&acceptanceWidgetPlatform(),"Supply owned-scope startup and case");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath()); const QString key=argv[2]; const bool stoppedTime=key=="PropSetupStoppedTime",prop=key=="PropSetup"||stoppedTime;
      require(prop||key=="CoordinateSystem","Unknown owned-scope case");
      const QString type=prop?"Propagator":"CoordinateSystem",nativeType=prop?"PropSetup":"CoordinateSystem";
      const auto directory=rig.files.filePath("Owned function Ω"); require(QDir().mkpath(directory),"Owned-scope directory absent");
      const auto function=directory+"/ReadShared.gmf",outer=directory+"/NestedShared.gmf",saved=directory+"/Caller.script",report=directory+"/Values.txt";
      const std::array<double,6> anchor{7000,100,200,-.1,7.5,.8},frameInput{1000,-200,300,1.25,-.5,.75};
      QString original="function [Mark, State] = ReadShared()\n"+resource("FunctionOwned",prop,true)+craft("FunctionSC",prop?"EarthMJ2000Eq":"FunctionOwned",prop?circle(7000,0):frameInput,-114108,731)+"Create Variable Mark PrivateOnly;\nCreate Array State[6,1];\nPrivateOnly = 103;\nBeginMissionSequence;\nGlobal Calls;\nCalls = Calls + 1;\nWrite Shared FunctionOwned {Style = Script, MessageWindow = true, LogFile = false};\n";
      if(prop)original+="Propagate FunctionOwned(FunctionSC) {FunctionSC.ElapsedSecs = 30"+QString(stoppedTime?"":", StopTolerance = 1e-12")+"};\n";
      for(int i=0;i<6;++i)original+="State("+QString::number(i+1)+",1) = FunctionSC.EarthMJ2000Eq."+fields[i]+";\n";
      original+="Mark = PrivateOnly + Calls;\n% Function-declared owned resource Ω\n";
      const auto edited=QString(original).replace("PrivateOnly = 103;","PrivateOnly = 41;");
      const QString outerBody="function [Mark, State] = NestedShared()\nCreate Variable Mark;\nCreate Array State[6,1];\nCreate GmatFunction ReadShared;\nReadShared.FunctionPath = '"+function+"';\nBeginMissionSequence;\n[Mark, State] = ReadShared();\n";
      write(function,original.toUtf8());write(outer,outerBody.toUtf8());rig.save("initial.gmf",original.toUtf8());rig.save("nested.gmf",outerBody.toUtf8());
      QString caller="Earth.Mu = 400000;\n"+resource("Shared",prop,false)+craft("PrivateOnly",prop?"EarthMJ2000Eq":"Shared",anchor,-114104,987);
      if(prop)caller+=craft("MainSC","EarthMJ2000Eq",circle(9000,0),-114107,719);
      caller+="Create Variable First Second Third Keep Calls;\nCreate Array MainState[6,1] FirstState[6,1] SecondState[6,1] ThirdState[6,1];\nKeep = 103;\nCalls = 0;\nCreate GmatFunction ReadShared NestedShared;\nReadShared.FunctionPath = '"+function+"';\nNestedShared.FunctionPath = '"+outer+"';\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Precision = 17;\nBeginMissionSequence;\nGlobal Calls;\nWrite Shared {Style = Script, MessageWindow = true, LogFile = false};\n";
      if(prop)caller+="Propagate Shared(MainSC) {MainSC.ElapsedSecs = 15};\n";
      for(int i=0;i<6;++i)caller+="MainState("+QString::number(i+1)+",1) = "+(prop?"MainSC.EarthMJ2000Eq.":"PrivateOnly.Shared.")+fields[i]+";\n";
      QString columns;for(int i=1;i<=6;++i)columns+=" MainState("+QString::number(i)+",1)";
      caller+="Report Values"+columns+";\n";
      for(const auto &word:QStringList{"First","Second","Third"})
      {
         caller+="["+word+", "+word+"State] = "+(word=="Third"?"NestedShared":"ReadShared")+"();\nReport Values "+word;
         for(int i=1;i<=6;++i)caller+=" "+word+"State("+QString::number(i)+",1)";
         caller+=";\n";
      }
      caller+="Report Values Keep;\nKeep = Keep + 1;\nReport Values Keep;\n";
      require(!QRegularExpression("Global[^;]*(?:Shared|FunctionOwned)").match(caller+original+outerBody).hasMatch(),"Fixture explicitly globalizes an automatic resource");
      rig.source->setPlainText(caller);rig.save("caller.script",caller.toUtf8());const bool built=rig.window.buildScript();rig.save("build-messages.txt",rig.messages->toPlainText().toUtf8());
      if(!built)std::cerr<<rig.messages->toPlainText().toStdString();require(built,"Owned-scope caller Build failed");
      auto prototypes=[&]{QJsonObject values;for(const auto &name:QStringList{"Shared","PrivateOnly","Calls"}){auto *p=Moderator::Instance()->GetConfiguredObject(name.toStdString());require(p,"Owned-scope prototype absent");values[name]=QString::fromStdString(p->GetGeneratingString(Gmat::NO_COMMENTS));}require(!Moderator::Instance()->GetConfiguredObject("FunctionOwned")&&!Moderator::Instance()->GetConfiguredObject("FunctionSC"),"Function declaration leaked into caller configuration");return values;};
      const auto before=prototypes();rig.save("prototypes-before.json",QJsonDocument(before).toJson());native(Moderator::Instance()->GetConfiguredObject("Shared"),prop,false);
      for (auto *tabs:rig.window.findChildren<QTabWidget*>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==rig.resources || tabs->widget(i)->isAncestorOf(rig.resources)) tabs->setCurrentIndex(i);
      const auto entries=rig.resources->findItems("ReadShared",Qt::MatchExactly|Qt::MatchRecursive); require(entries.size()==1,"Owned function resource absent"); rig.resources->scrollToItem(entries[0]); rig.resources->itemDoubleClicked(entries[0],0); events();
      auto *owner=rig.area->activeSubWindow(); auto *panel=owner?dynamic_cast<ResourceEditor*>(owner->widget()):nullptr; require(panel,"Owned function editor absent"); owner->showNormal(); owner->move(0,0); owner->resize(rig.area->viewport()->size()); events(); auto *edit=panel->findChild<QPushButton*>("editFunctionFile"); reveal(edit);
      const int history=rig.source->document()->availableUndoSteps(); QJsonArray decisions;
      for (bool commit:{false,true})
      {
         bool visited=false; std::exception_ptr failure;
         QTimer::singleShot(0,panel,[&]{auto *dialog=panel->findChild<QDialog*>("functionFileDialog"); try { require(dialog,"Owned file dialog absent"); auto *text=dialog->findChild<QPlainTextEdit*>("functionFileText"); require(text && !text->isReadOnly() && text->toPlainText()==original,"Owned initial file differs"); reveal(text); QTest::mouseClick(text->viewport(),Qt::LeftButton); QTest::keyClick(text,Qt::Key_A,Qt::ControlModifier); QApplication::clipboard()->setText(edited); QTest::keyClick(text,Qt::Key_V,Qt::ControlModifier); events(); require(text->toPlainText()==edited && read(function)==original.toUtf8() && rig.source->toPlainText()==caller && rig.source->document()->availableUndoSteps()==history && prototypes()==before,"Owned draft changed file/caller/history/prototypes"); auto *button=dialog->findChild<QDialogButtonBox*>()->button(commit?QDialogButtonBox::Save:QDialogButtonBox::Cancel); reveal(button); QTest::mouseClick(button,Qt::LeftButton); visited=true; } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }});
         QTest::mouseClick(edit,Qt::LeftButton); if (failure) std::rethrow_exception(failure);
         require(visited && read(function)==(commit?edited:original).toUtf8() && rig.source->toPlainText()==caller,"Owned file Save/Cancel differs"); decisions.append(QJsonObject{{"saved",commit}});
      }
      require(owner->close(),"Owned editor failed to close"); events(); rig.save("edited.gmf",edited.toUtf8()); require(rig.window.saveScriptTo(saved),"Owned Unicode caller Save failed"); QJsonArray phases;

      for(int phase=0;phase<3;++phase)
      {
         if(phase==1)require(rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==caller&&prototypes()==before,"Owned Unicode reopen changed source/prototypes");
         const int undo=rig.source->document()->availableUndoSteps();rig.messages->clear();acceptanceView(rig.window);const auto outcome=rig.window.runMission();events();const auto tag=QString::number(phase),messages=rig.messages->toPlainText();rig.save(tag+"-messages.txt",messages.toUtf8());
         if(outcome!=MainWindow::RunResult::Completed)std::cerr<<messages.toStdString();require(outcome==MainWindow::RunResult::Completed,"Owned main/function mission failed");
         auto *shared=Moderator::Instance()->GetInternalObject("Shared"),*configured=Moderator::Instance()->GetConfiguredObject("Shared");
         require(shared&&shared!=configured&&!shared->IsLocal(),"Owned main automatic resource identity differs");const auto mainState=native(shared,prop,false);
         auto *anchorSC=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject("PrivateOnly"));require(anchorSC&&anchorSC->GetRealParameter("DryMass")==987&&anchorSC->GetEpoch()==21545,"Owned function-private variable changed same-name caller");
         for(int i=0;i<6;++i)require(anchorSC->GetState()[i]==anchor[i],"Owned resource use changed caller anchor state");
         require(Moderator::Instance()->GetInternalObject("Calls")->GetRealParameter("Value")==3,"Owned reader call counter differs");
         if(!prop)require(anchorSC->GetRefObject(Gmat::COORDINATE_SYSTEM,"Shared")==shared,"Actual caller frame identity differs");
         auto checkedWrites=[&](const QString &name,int count){QJsonArray result;auto matches=QRegularExpression("(?:^|\\n)Create "+type+" "+name+";").globalMatch(messages);while(matches.hasNext())result.append(matches.next().captured(0).trimmed());require(result.size()==count,"Owned actual Script constructor Write differs");return result;};
         const auto mainWrites=checkedWrites("Shared",4),functionWrites=checkedWrites("FunctionOwned",3);
         QJsonArray identities;
         auto readerIdentity=[&](UserDefinedFunction *reader,const QString &path){
            require(reader&&reader->GetName()=="ReadShared"&&reader->GetFunctionControlSequence()&&reader->GetGlobalObjectStore(),"Actual owned reader absent");
            const auto *store=reader->GetGlobalObjectStore();const auto found=store->find("Shared");require(found!=store->end()&&found->second==shared,"Owned function implicit main-global identity differs");
            auto *declared=reader->FindFunctionObject("FunctionOwned");require(declared&&declared!=shared&&declared!=configured,"Owned function declaration absent/shared with main");const auto declaration=native(declared,prop,true);
            QJsonArray commands;int propagations=0;QJsonObject executed;
            for(auto *command=reader->GetFunctionControlSequence();command;command=command->GetNext())
            {
               commands.append(QJsonObject{{"type",QString::fromStdString(command->GetTypeName())},{"source",QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS))}});
               if(command->GetTypeName()=="Propagate") {auto *coast=dynamic_cast<Propagate*>(command);require(coast,"Owned propagation command type differs");auto *actual=dynamic_cast<PropSetup*>(coast->GetRefObject(Gmat::PROP_SETUP,"FunctionOwned",0));executed=native(actual,true,true);executed["integrator_elapsed_seconds"]=actual->GetPropagator()->GetTime();executed["force_elapsed_seconds"]=actual->GetODEModel()->GetTime();executed["stop_tolerance"]=coast->GetRealParameter(coast->GetParameterID("StopTolerance"));++propagations;}
            }
            require(propagations==(prop?1:0),"Function-declared propagation route missing/duplicated");
            identities.append(QJsonObject{{"path",path},{"shared_global_pointer",pointer(found->second)},{"runtime_shared_pointer",pointer(shared)},{"configured_shared_pointer",pointer(configured)},{"declaration",declaration},{"executed_prop_setup",executed},{"propagations",propagations},{"fcs_commands",commands}});
         };
         int direct=0,nested=0;for(auto *command=Moderator::Instance()->GetFirstCommand();command;command=command->GetNext())
         {
            if(!command->IsOfType("CallFunction"))continue;auto *fn=dynamic_cast<UserDefinedFunction*>(command->GetRefObject(Gmat::FUNCTION,""));require(fn,"Owned caller function absent");
            if(fn->GetName()=="ReadShared")readerIdentity(fn,"direct-"+QString::number(++direct));
            else if(fn->GetName()=="NestedShared")for(auto *inner=fn->GetFunctionControlSequence();inner;inner=inner->GetNext())if(inner->IsOfType("CallFunction")){readerIdentity(dynamic_cast<UserDefinedFunction*>(inner->GetRefObject(Gmat::FUNCTION,"")),"nested");++nested;}
         }
         require(direct==2&&nested==1&&identities.size()==3,"Owned direct/direct/nested identity missing");
         const auto raw=read(report);rig.save(tag+"-values.txt",raw);const auto words=QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(words.size()==29,"Owned raw continuation width differs");QJsonArray values,expected,bounds;
         for(const auto &word:words){bool ok;const auto v=word.toDouble(&ok);require(ok&&std::isfinite(v),"Owned raw value invalid");values.append(v);}
         double functionInterval=30;
         if(stoppedTime)
         {
            functionInterval=identities[0].toObject()["executed_prop_setup"].toObject()["integrator_elapsed_seconds"].toDouble();
            require(std::abs(functionInterval-30)<=1e-6,"Default stopping time exceeds one microsecond");
            for(const auto &identity:identities){const auto executed=identity.toObject()["executed_prop_setup"].toObject();require(executed["integrator_elapsed_seconds"].toDouble()==functionInterval&&executed["force_elapsed_seconds"].toDouble()==functionInterval,"Executed integrator/force elapsed times differ");}
         }
         const auto mainExpected=prop?circle(9000,15):anchor;auto functionExpected=prop?circle(7000,functionInterval):frameInput;
         if(!prop){const double e=84381.448/3600*std::acos(-1.0)/180;for(int offset:{0,3}){const double y=functionExpected[offset+1],z=functionExpected[offset+2];functionExpected[offset+1]=std::cos(e)*y-std::sin(e)*z;functionExpected[offset+2]=std::sin(e)*y+std::cos(e)*z;}}
         auto appendState=[&](const std::array<double,6> &v){for(int i=0;i<6;++i){expected.append(v[i]);bounds.append(prop?(i<3?1e-7:1e-10):1e-9);}};
         appendState(mainExpected);for(int mark:{42,43,44}){expected.append(mark);bounds.append(0);appendState(functionExpected);}for(int keep:{103,104}){expected.append(keep);bounds.append(0);}
         rig.save(tag+"-oracle-diagnostics.json",QJsonDocument(QJsonObject{{"identities",identities},{"raw_values",values},{"expected_values",expected},{"absolute_bounds",bounds}}).toJson());
         require(expected.size()==29&&bounds.size()==29,"Owned oracle width differs");for(int i=0;i<29;++i)require(std::abs(values[i].toDouble()-expected[i].toDouble())<=bounds[i].toDouble(),"Owned independent main/function state or marker differs");
         const auto after=prototypes();rig.save(tag+"-prototypes.json",QJsonDocument(after).toJson());rig.save(tag+"-invariants.json",QJsonDocument(QJsonObject{{"prototypes",after==before},{"source",rig.source->toPlainText()==caller},{"history",rig.source->document()->availableUndoSteps()==undo},{"function_file",read(function)==edited.toUtf8()},{"nested_file",read(outer)==outerBody.toUtf8()},{"not_running",!rig.window.isRunning()}}).toJson());
         require(after==before&&rig.source->toPlainText()==caller&&rig.source->document()->availableUndoSteps()==undo&&read(function)==edited.toUtf8()&&read(outer)==outerBody.toUtf8()&&!rig.window.isRunning(),"Owned execution changed source/history/prototypes/files/lock");
         phases.append(QJsonObject{{"phase",phase},{"explicit_rebuild",phase<2},{"main_writes",mainWrites},{"function_writes",functionWrites},{"main_resource",mainState},{"caller_anchor_state",QJsonArray{7000,100,200,-.1,7.5,.8}},{"caller_anchor_mass",987},{"identities",identities},{"requested_function_interval_seconds",30},{"oracle_function_interval_seconds",functionInterval},{"stopping_time_absolute_bound_seconds",stoppedTime?1e-6:0},{"raw_values",values},{"expected_values",expected},{"absolute_bounds",bounds}});rig.save("phases.json",QJsonDocument(phases).toJson());
      }
      auto *tree=rig.window.findChild<QTreeWidget*>("Output"); const auto outputs=tree->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive); require(outputs.size()==1,"Owned report output absent"); tree->itemDoubleClicked(outputs[0],0); events(); auto *viewer=rig.area->activeSubWindow(); require(viewer && dynamic_cast<ReportViewer*>(viewer->widget()),"Owned actual report viewer absent"); auto *shown=viewer->findChild<QPlainTextEdit*>(); require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(read(report)).trimmed(),"Owned viewer differs from checked bytes"); acceptanceView(*viewer);

      const QJsonObject proof{{"checkpoint","function-declared-owned-resource-scope"},{"outcome","passed"},{"case",key},{"type",type},{"native_type",nativeType},{"stop_time_policy",stoppedTime?"measured-endpoint-and-default-one-microsecond-duration-bound":"fixed-input-endpoint"},{"missions",3},{"function_calls",12},{"reader_calls",9},{"file_decisions",decisions},{"phases",phases},{"scope","Actual main and function-declared PropSetup/CoordinateSystem creation and use;distinct method/frame configurations,implicit main-global identity,direct/direct/nested calls,private marker/caller isolation,independent synthetic point-mass-circle or fixed-obliquity frame outputs,file Cancel/Save,Unicode/no-Build repeat and actual report viewer. StoppedTime is a separate actual-endpoint/default-duration contract;the strict 30-second probe remains registered and unqualified. Declaration-template pointers are distinguished from executed Propagate-owned clones. No stale function-local-store access,all-method/axes/provider or whole requirement qualification."}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }
   catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}
   catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}
   return 1;
}
