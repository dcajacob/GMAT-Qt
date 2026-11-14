#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "ObjectManagedFunction.hpp"
#include "Parameter.hpp"
#include "ParameterInfo.hpp"
#include "CoordinateSystem.hpp"
#include "AxisSystem.hpp"
#include "ODEModel.hpp"
#include "FactoryManager.hpp"
#include "Publisher.hpp"
#include "Subscriber.hpp"
#include "UserDefinedFunction.hpp"
#include "Sandbox.hpp"
#include "Debugger.hpp"
#include <memory>
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
   require(widget,"Binding resource control absent");
   for (auto *page=widget;page;page=page->parentWidget())
   {
      for (auto *tabs:widget->window()->findChildren<QTabWidget*>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page);
      if (auto *scroll=qobject_cast<QScrollArea*>(page)) scroll->ensureWidgetVisible(widget);
   }
   events(); require(widget->isVisible() && widget->isEnabled(),"Binding resource control unreachable"); acceptanceView(*widget);
   auto *top=widget->window();top->raise();top->activateWindow();QApplication::setActiveWindow(top);widget->setFocus(Qt::OtherFocusReason);events();if(qobject_cast<QPlainTextEdit*>(widget))require(widget->hasFocus(),"Binding text editor did not take focus");
}


static QString reference(const QJsonObject &spec,bool shared,bool function)
{
   return spec[shared?"shared_reference":function?"local_private_reference":"root_private_reference"].toString();
}
static QJsonValue expectedValue(const QJsonObject &spec,bool shared,bool inside,int calls){return spec["observations"].toObject()[shared?"shared":inside?"private":"root"].toArray()[calls].toObject()["expected"];}
static double bound(const QString &,const QString &) { return 1e-6; }
static void compareValue(const QJsonValue &actual,const QJsonValue &expected,double tolerance)
{
   if(expected.isString()){require(actual==expected,"Binding string value differs from authored stamp");return;}
   if(expected.isArray())
   {
      require(actual.isArray()&&actual.toArray().size()==expected.toArray().size(),"Binding array shape differs");const auto a=actual.toArray(),e=expected.toArray();
      for(int i=0;i<a.size();++i)compareValue(a[i],e[i],tolerance);return;
   }
   require(actual.isDouble()&&std::isfinite(actual.toDouble())&&std::abs(actual.toDouble()-expected.toDouble())<=tolerance,"Binding numeric value differs from selected literal/formula");
}
static QJsonArray stateJson(Spacecraft *sc){QJsonArray a;for(int i=0;i<6;++i)a.append(sc->GetState()[i]);return a;}
static QJsonObject observation(const QJsonObject &spec,bool shared,bool inside,int calls){return spec["observations"].toObject()[shared?"shared":inside?"private":"root"].toArray()[calls].toObject();}
static void compareState(const QJsonValue &actual,const QJsonValue &expected){require(actual.toArray().size()==6&&expected.toArray().size()==6,"Conic state width differs");for(int i=0;i<6;++i)compareValue(actual.toArray()[i],expected.toArray()[i],i<3?1e-6:1e-9);}
static QJsonArray vectorJson(const Rvector &v){require(v.GetSize()==6,"Special vector width differs");QJsonArray a;for(int i=0;i<6;++i)a.append(v[i]);return a;}
static QJsonArray matrixJson(const Rmatrix33 &m){QJsonArray a;for(int i=0;i<3;++i)for(int j=0;j<3;++j)a.append(m(i,j));return a;}
static QJsonArray transported(const QJsonArray &state,const Rmatrix33 &rotation,const Rmatrix33 &rate)
{
   require(state.size()==6,"Special literal coordinate state width differs");QJsonArray result;
   for(int i=0;i<6;++i){double value=0;for(int j=0;j<3;++j){value+=rotation(i%3,j)*state[j+(i<3?0:3)].toDouble();if(i>=3)value+=rate(i%3,j)*state[j].toDouble();}result.append(value);}return result;
}

class BindingObserver final:public Subscriber
{
public:
   BindingObserver(Rig &rig,const QJsonObject &spec,const QString &prefix):Subscriber("ConicParameterBindingObserver","ConicParameterBindingObserver"),rig(&rig),spec(spec),prefix(prefix){}
   ~BindingObserver() override{Publisher::Instance()->Unsubscribe(this);}
   GmatBase *Clone() const override{return new BindingObserver(*this);}
   bool ReceiveData(const Real *data,Integer count=0) override
   {
      bool initial=false;if(!Publisher::GetPublishedPropagationObjects(data,count,initial)||!initial)return true;
      try
      {
         auto *command=dynamic_cast<GmatCommand*>(currentProvider);require(command,"Conic publication provider absent");auto *function=dynamic_cast<ObjectManagedFunction*>(command->GetCurrentFunction());const bool inside=function!=nullptr;
         const auto *locals=inside?function->GetFunctionObjectStore():command->GetObjectMap();const auto rootGlobals=inside?ObjectMap{}:Moderator::Instance()->GetSandbox()->GetGlobalObjectMap();const auto *globals=inside?function->GetGlobalObjectStore():&rootGlobals;require(locals&&globals,"Conic live scope maps absent");
         auto lookup=[&](const std::string &name){const auto a=locals->find(name);if(a!=locals->end())return a->second;const auto b=globals->find(name);return b==globals->end()?nullptr:b->second;};auto *counter=lookup("Calls");require(counter,"Conic call counter absent");const int calls=int(counter->GetRealParameter("Value"));
         QJsonArray localKeys,globalKeys;for(const auto &v:*locals)localKeys.append(QString::fromStdString(v.first));for(const auto &v:*globals)globalKeys.append(QString::fromStdString(v.first));events.append(QJsonObject{{"inside_function",inside},{"function",inside?QString::fromStdString(function->GetName()):QString()},{"calls",calls},{"completed_samples",records.size()},{"local_keys",localKeys},{"global_keys",globalKeys},{"provider",QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS))}});rig->save(prefix+"-callback-events.json",QJsonDocument(events).toJson());require(calls==(inside?events.size()-1:0)&&(!inside||function->GetName()=="ReadShared"),"Conic callback order differs");
         const auto type=spec["factory_type"].toString();const auto meta=spec["metadata"].toObject();QJsonArray rows;
         for(bool shared:{false,true})
         {
            const QString ownerName=shared?"SharedSC":"PrivateOnly",full=reference(spec,shared,inside);auto *owner=dynamic_cast<Spacecraft*>(lookup(ownerName.toStdString()));auto *root=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject(ownerName.toStdString()));auto *configured=Moderator::Instance()->GetConfiguredObject(ownerName.toStdString());require(owner&&root&&configured&&owner!=configured&&owner->IsOfType(meta["owner_type_id"].toInt()),"Conic native Spacecraft owner absent");require(owner->IsGlobal()==shared&&owner->IsLocal()==(inside&&!shared)&&(shared?owner==root:inside?owner!=root:owner==root),"Conic owner scope identity/flags differ");
            auto *parameter=dynamic_cast<Parameter*>(lookup(full.toStdString()));require(parameter&&parameter->IsSystemParameter()&&parameter->GetName()==full.toStdString()&&parameter->GetTypeName()==meta["native_type"].toString().toStdString()&&int(parameter->GetReturnType())==meta["return_type"].toInt()&&int(parameter->GetOwnerType())==meta["owner_type_id"].toInt()&&parameter->GetRefObject(parameter->GetOwnerType(),ownerName.toStdString())==owner,"Conic actual Parameter owner/reference/type differs");
            const auto frameName=spec["frame_name"].toString();const bool parameterFrame=spec["frame_role"]=="parameter";
            auto *frame=dynamic_cast<CoordinateSystem*>(parameterFrame?parameter->GetRefObject(Gmat::COORDINATE_SYSTEM,frameName.toStdString()):owner->GetRefObject(Gmat::COORDINATE_SYSTEM,""));auto *wanted=dynamic_cast<CoordinateSystem*>(Moderator::Instance()->GetInternalObject(frameName.toStdString()));require(frame&&frame==wanted&&frame->GetAxisSystem()->GetTypeName()==spec["frame_axes"].toString().toStdString(),"Special actual owner/Parameter frame differs");auto *solar=frame->GetSolarSystem();auto *earth=solar?solar->GetBody("Earth"):nullptr;require(earth&&frame->GetOrigin()==earth&&(!inside||function->GetSolarSystem()==solar)&&earth->GetRealParameter(earth->GetParameterID("Mu"))==spec["mu"].toDouble(),"Special actual SolarSystem/origin/Mu differs");GmatBase *origin=nullptr;if(meta["origin_dependent"].toBool()){origin=parameter->GetRefObject(Gmat::SPACE_POINT,"Earth");require(origin==earth,"Special origin-dependent Parameter bound wrong Earth");}
            const auto expected=observation(spec,shared,inside,calls);QJsonValue actual;QJsonArray shape;if(parameter->GetReturnType()==Gmat::REAL_TYPE)actual=parameter->EvaluateReal();else{require(int(parameter->GetReturnType())==11,"Conic selected return kind unsupported");const auto &value=parameter->EvaluateRvector6();QJsonArray v;for(int i=0;i<6;++i)v.append(value[i]);actual=v;shape=QJsonArray{6,1};}
            const auto beforeState=stateJson(owner);Rvector6 raw;for(int i=0;i<6;++i)raw[i]=owner->GetState()[i];const auto direct=vectorJson(frame->FromBaseSystem(A1Mjd(owner->GetEpoch()),raw,true,true));const auto rotation=frame->GetLastRotationMatrix(),rate=frame->GetLastRotationDotMatrix();const auto expectedInertial=transported(expected["state"].toArray(),rotation,rate);rig->save(prefix+"-special-diagnostic.json",QJsonDocument(QJsonObject{{"reference",full},{"actual",actual},{"expected",expected["expected"]},{"inertial",beforeState},{"reference_state",direct},{"expected_reference_state",expected["state"]},{"rotation",matrixJson(rotation)},{"rotation_dot",matrixJson(rate)}}).toJson());compareValue(actual,expected["expected"],bound("SpecialOrbit",type));compareState(direct,expected["state"]);compareState(beforeState,expectedInertial);require(std::abs(owner->GetEpoch()-21545)<=2e-10&&std::abs(owner->GetEpochGT().GetMjd()-21545)<=2e-10,"Special epoch stores differ");if(spec["frame_axes"]=="BodyFixed"){require(earth->GetFlattening()==expected["flattening"].toDouble()&&earth->GetEquatorialRadius()==expected["equatorial_radius"].toDouble(),"Special actual spheroid differs");}
            require(parameter->IsReportable()==meta["reportable"].toBool()&&parameter->IsPlottable()==meta["plottable"].toBool(),"Conic actual reportability/plottability flags differ");const bool settable=meta["settable"].toBool();require(parameter->IsSettable()==settable&&ParameterInfo::Instance()->IsSettable(type.toStdString())==settable&&parameter->IsGlobal()==shared,"Conic writable/Global metadata differs");std::unique_ptr<Parameter> native(FactoryManager::Instance()->CreateParameter(type.toStdString()));require(bool(native),"Conic native factory metadata control absent");const auto unit=QString::fromStdString(parameter->GetStringParameter("Unit"));require(unit==QString::fromStdString(native->GetStringParameter("Unit")),"Conic live/prototype unit label differs");QJsonObject mutation;
            if(spec["writable_probe"].toBool())
            {
               require(settable&&actual.isDouble(),"Conic strict idempotent probe type differs");parameter->SetReal(actual.toDouble());const double after=parameter->EvaluateReal();compareValue(after,expected["expected"],bound("Conic",type));compareState(stateJson(owner),beforeState);require(parameter->GetRefObject(parameter->GetOwnerType(),ownerName.toStdString())==owner&&(parameterFrame?parameter->GetRefObject(Gmat::COORDINATE_SYSTEM,frameName.toStdString()):owner->GetRefObject(Gmat::COORDINATE_SYSTEM,""))==frame,"Conic idempotent setter changed owner/frame");mutation=QJsonObject{{"target",actual},{"after",after},{"after_state",stateJson(owner)},{"idempotent_only",true},{"owner_and_frame_preserved",true}};

            }
            rows.append(QJsonObject{{"shared",shared},{"reference",full},{"factory_type",type},{"native_type",QString::fromStdString(parameter->GetTypeName())},{"return_type",int(parameter->GetReturnType())},{"owner_type",int(parameter->GetOwnerType())},{"parameter_pointer",pointer(parameter)},{"reference_owner_pointer",pointer(owner)},{"live_owner_pointer",pointer(owner)},{"root_owner_pointer",pointer(root)},{"configured_owner_pointer",pointer(configured)},{"owner_name",ownerName},{"owner_type_name",QString::fromStdString(owner->GetTypeName())},{"owner_global",owner->IsGlobal()},{"owner_local",owner->IsLocal()},{"parameter_global",parameter->IsGlobal()},{"parameter_local",parameter->IsLocal()},{"shape",shape},{"actual",actual},{"expected",expected["expected"]},{"absolute_bound",bound("Conic",type)},{"inertial_state",beforeState},{"owner_epoch",owner->GetEpoch()},{"owner_gmat_time_mjd",owner->GetEpochGT().GetMjd()},{"reference_state",direct},{"expected_inertial_from_measured_frame",expectedInertial},{"frame_to_base_rotation",matrixJson(rotation)},{"frame_to_base_rotation_dot",matrixJson(rate)},{"frame_name",frameName},{"frame_axes",spec["frame_axes"]},{"frame_role",spec["frame_role"]},{"parameter_frame_reference_observed",parameterFrame},{"origin_equatorial_radius",earth->GetEquatorialRadius()},{"origin_flattening",earth->GetFlattening()},{"frame_pointer",pointer(frame)},{"expected_frame_pointer",pointer(wanted)},{"origin_pointer",pointer(frame->GetOrigin())},{"expected_origin_pointer",pointer(earth)},{"parameter_origin_pointer",origin?pointer(origin):QString("0")},{"solar_system_pointer",pointer(solar)},{"function_solar_system_pointer",inside?pointer(function->GetSolarSystem()):QString("0")},{"origin_mu",earth->GetRealParameter(earth->GetParameterID("Mu"))},{"native_idempotent",mutation},{"runtime_reportable",parameter->IsReportable()},{"runtime_plottable",parameter->IsPlottable()},{"runtime_settable",parameter->IsSettable()},{"info_settable",meta["settable"]},{"legacy_unit",unit},{"factory_prototype_unit",QString::fromStdString(native->GetStringParameter("Unit"))}});
         }
         records.append(QJsonObject{{"inside_function",inside},{"function",inside?QString::fromStdString(function->GetName()):QString()},{"calls",calls},{"global_map_is_value_snapshot",!inside},{"provider",QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS))},{"parameters",rows}});rig->save(prefix+"-live-parameter-samples.json",QJsonDocument(records).toJson());
      }
      catch(BaseException &e){error=QString::fromStdString(e.GetFullMessage());failure=std::current_exception();}
      catch(std::exception &e){error=e.what();failure=std::current_exception();}
      return true;
   }
   Rig *rig;QJsonObject spec;QString prefix,error;QJsonArray records,events;std::exception_ptr failure;
};

static QString spacecraft(const QString &name,int x,int mass,int id,const QString &stamp=QString())
{
   QString code="Create Spacecraft "+name+";\n"+name+".CoordinateSystem = EarthMJ2000Eq;\n"+name+".DateFormat = "+(stamp.isEmpty()?"A1ModJulian":"UTCGregorian")+";\n"+name+".Epoch = '"+(stamp.isEmpty()?"21545":stamp)+"';\n"+name+".DisplayStateType = Cartesian;\n"+name+".X = "+QString::number(x)+";\n"+name+".Y = 100;\n"+name+".Z = 200;\n"+name+".VX = -0.1;\n"+name+".VY = 7.5;\n"+name+".VZ = 0.8;\n"+name+".DryMass = "+QString::number(mass)+";\n"+name+".NAIFId = "+QString::number(id)+";\n"+name+".NAIFIdReferenceFrame = "+QString::number(id-8000000)+";\n";return code;
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("FunctionSpecialOrbitBinding");
   try
   {
      require(argc==4&&acceptanceWidgetPlatform(),"Supply parameter-binding startup,catalog and type");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();const QString argument=argv[3];QJsonObject spec;QStringList inventory;
      for(const auto &v:catalog["cases"].toArray())inventory.append(v.toObject()["factory_type"].toString());for(const auto &v:catalog["special_binding_cases"].toArray())if(v.toObject()["argument"]==argument)spec=v.toObject();const QString key=spec["factory_type"].toString();
      require(catalog["factory_rows"]==566&&inventory.size()==321&&!spec.isEmpty()&&catalog["initial_types"].toArray().contains(key)&&spec["family"]=="SpecialOrbit","Full parameter inventory or selected owner fixture differs");
      QStringList actual;for(const auto &name:Moderator::Instance()->GetListOfFactoryItems(Gmat::PARAMETER))actual.append(QString::fromStdString(name));inventory+=QStringList{"Array","String","Variable"};inventory.sort();actual.sort();require(actual==inventory,"Advertised complete ParameterFactory universe changed");
      const auto family=spec["family"].toString(),type=spec["factory_type"].toString();std::unique_ptr<Parameter> prototype(FactoryManager::Instance()->CreateParameter(type.toStdString()));
      require(prototype&&prototype->IsSystemParameter()&&prototype->GetTypeName()==spec["metadata"].toObject()["native_type"].toString().toStdString(),"Selected unmanaged native factory control differs");
      const auto directory=rig.files.filePath("Live parameter owner Ω");require(QDir().mkpath(directory),"Binding Unicode directory absent");
      const auto function=directory+"/ReadShared.gmf",outer=directory+"/NestedShared.gmf",saved=directory+"/Caller.script",report=directory+"/Values.txt";
      auto literal=[](double v){return QString::number(v,'g',17);};
      auto stateCode=[&](const QString &name,const QJsonArray &values){QString code;const QStringList fields{"X","Y","Z","VX","VY","VZ"};for(int i=0;i<6;++i)code+=name+"."+fields[i]+" = "+literal(values[i].toDouble())+";\n";return code;};
      auto target=[&](const QString &name,int x,int mass,int id){QString code=spacecraft(name,x,mass,id);code.replace(name+".CoordinateSystem = EarthMJ2000Eq;",name+".CoordinateSystem = "+spec["frame_name"].toString()+";");return code;};
      auto base=[&](const QString &scope){return spec["observations"].toObject()[scope].toArray()[0].toObject()["state"].toArray();};
      QString declaration=target("PrivateOnly",7500,275,-128101)+stateCode("PrivateOnly",base("private"))+spacecraft("ScopePulse",9000,701,-128102);
      QString mutate;for(int calls=1;calls<=3;++calls){mutate+="If Calls == "+QString::number(calls)+";\n";for(bool shared:{false,true}){mutate+=stateCode(shared?"SharedSC":"PrivateOnly",observation(spec,shared,true,calls)["state"].toArray());if(spec["writable_probe"].toBool())mutate+=reference(spec,shared,true)+" = "+reference(spec,shared,true)+";\n";}mutate+="EndIf;\n";}
      const bool scalarBridge=prototype->GetReturnType()==Gmat::REAL_TYPE&&!spec["metadata"].toObject()["reportable"].toBool();
      const bool vectorBridge=prototype->GetReturnType()!=Gmat::REAL_TYPE;const QString vectorDeclarations=vectorBridge?"Create Array PrivateVector[6,1] SharedVector[6,1];\n":scalarBridge?"Create Variable PrivateScalar SharedScalar;\n":QString();
      auto actualReport=[&](bool inside){if(scalarBridge)return "PrivateScalar = "+reference(spec,false,inside)+";\nSharedScalar = "+reference(spec,true,inside)+";\nReport BindingValues PrivateScalar SharedScalar;\n";if(!vectorBridge)return "Report BindingValues "+reference(spec,false,inside)+" "+reference(spec,true,inside)+";\n";QString code="PrivateVector = "+reference(spec,false,inside)+";\nSharedVector = "+reference(spec,true,inside)+";\nReport BindingValues";for(const auto &name:QStringList{"PrivateVector","SharedVector"})for(int i=1;i<=6;++i)code+=" "+name+"("+QString::number(i)+",1)";return code+";\n";};
      const QString global="Global SharedSC Calls BindingValues"+QString(spec["frame_axes"]=="BodyFixed"?" SpecialFixed":"")+";\n";
      const QString original="function [Mark] = ReadShared()\n"+declaration+vectorDeclarations+"Create Variable Mark Keep ScopeEnd;\nKeep = 103;\nScopeEnd = 12345;\nBeginMissionSequence;\n"+global+"Calls = Calls + 1;\n"+mutate+"Write "+reference(spec,false,true)+" "+reference(spec,true,true)+" ScopeEnd {Style = Verbose, MessageWindow = true, LogFile = false};\n"+actualReport(true)+"Propagate PulseProp(ScopePulse) {ScopePulse.ElapsedSecs = 1};\nMark = Keep + Calls;\n% Live special-state owner binding Ω\n";
      const auto edited=QString(original).replace("Keep = 103;","Keep = 41;");
      const QString outerBody="function [Mark] = NestedShared()\nCreate Variable Mark;\nCreate GmatFunction ReadShared;\nReadShared.FunctionPath = '"+function+"';\nBeginMissionSequence;\n[Mark] = ReadShared();\n";
      write(function,original.toUtf8());write(outer,outerBody.toUtf8());rig.save("initial.gmf",original.toUtf8());rig.save("nested.gmf",outerBody.toUtf8());
      QString frameSetup;if(spec["frame_axes"]=="BodyFixed")frameSetup="Earth.Flattening = 0.1;\nEarth.EquatorialRadius = 6378.1363;\nCreate CoordinateSystem SpecialFixed;\nSpecialFixed.Origin = Earth;\nSpecialFixed.Axes = BodyFixed;\n";
      QString caller=frameSetup+target("PrivateOnly",7000,987,-126001)+stateCode("PrivateOnly",base("root"))+target("SharedSC",8000,150,-126002)+stateCode("SharedSC",base("shared"))+spacecraft("MainPulse",9000,702,-126003);
      caller+=vectorDeclarations+"Create ForceModel PulseFM;\nPulseFM.PrimaryBodies = {};\nPulseFM.PointMasses = {Earth};\nPulseFM.Drag = None;\nPulseFM.SRP = Off;\nCreate Propagator PulseProp;\nPulseProp.FM = PulseFM;\nPulseProp.Type = RungeKutta89;\nPulseProp.InitialStepSize = 1;\nPulseProp.MinStep = 0;\nPulseProp.MaxStep = 1;\nCreate Variable First Second Third Keep Calls ScopeEnd;\nKeep = 103;\nCalls = 0;\nScopeEnd = 24680;\nCreate GmatFunction ReadShared NestedShared;\nReadShared.FunctionPath = '"+function+"';\nNestedShared.FunctionPath = '"+outer+"';\nCreate ReportFile BindingValues;\nBindingValues.Filename = '"+directory+"/Bindings.txt';\nBindingValues.WriteHeaders = false;\nBindingValues.FixedWidth = false;\nBindingValues.Precision = 17;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Precision = 17;\nBeginMissionSequence;\n"+global+"Write 'Arm bindings' Keep {Style = Verbose, MessageWindow = true, LogFile = false};\nWrite "+reference(spec,false,false)+" "+reference(spec,true,false)+" ScopeEnd {Style = Verbose, MessageWindow = true, LogFile = false};\n"+actualReport(false)+"Propagate PulseProp(MainPulse) {MainPulse.ElapsedSecs = 1};\n[First] = ReadShared();\n[Second] = ReadShared();\n[Third] = NestedShared();\nReport Values First Second Third Keep;\nKeep = Keep + 1;\nReport Values Keep;\n";
      rig.source->setPlainText(caller);rig.save("caller.script",caller.toUtf8());const bool built=rig.window.buildScript();rig.save("build-messages.txt",rig.messages->toPlainText().toUtf8());if(!built){std::cerr<<rig.messages->toPlainText().toStdString();QJsonArray errors;for(const auto &e:Moderator::GetScriptInterpreter()->GetErrorList()){errors.append(QString::fromStdString(e));std::cerr<<e<<std::endl;}rig.save("build-errors.json",QJsonDocument(errors).toJson());}require(built,"Binding main parameter caller Build failed");
      auto prototypes=[&]{QJsonObject values;for(const auto &name:QStringList{"PrivateOnly","SharedSC","Calls","Keep","ScopeEnd"}){auto *p=Moderator::Instance()->GetConfiguredObject(name.toStdString());require(p,"Binding prototype absent");values[name]=QString::fromStdString(p->GetGeneratingString(Gmat::NO_COMMENTS));}return values;};const auto before=prototypes();rig.save("prototypes-before.json",QJsonDocument(before).toJson());
      for (auto *tabs:rig.window.findChildren<QTabWidget*>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==rig.resources || tabs->widget(i)->isAncestorOf(rig.resources)) tabs->setCurrentIndex(i);
      const auto entries=rig.resources->findItems("ReadShared",Qt::MatchExactly|Qt::MatchRecursive); require(entries.size()==1,"Binding function resource absent"); rig.resources->scrollToItem(entries[0]); rig.resources->itemDoubleClicked(entries[0],0); events();
      auto *owner=rig.area->activeSubWindow(); auto *panel=owner?dynamic_cast<ResourceEditor*>(owner->widget()):nullptr; require(panel,"Binding function editor absent"); owner->showNormal(); owner->move(0,0); owner->resize(rig.area->viewport()->size()); events(); auto *edit=panel->findChild<QPushButton*>("editFunctionFile"); reveal(edit);
      const int history=rig.source->document()->availableUndoSteps(); QJsonArray decisions;
      for (bool commit:{false,true})
      {
         bool visited=false; std::exception_ptr failure;
         QTimer modal;modal.setInterval(20);QObject::connect(&modal,&QTimer::timeout,panel,[&]{auto *dialog=panel->findChild<QDialog*>("functionFileDialog");if(!dialog)return;modal.stop();try { require(dialog,"Binding file dialog absent"); auto *text=dialog->findChild<QPlainTextEdit*>("functionFileText"); require(text && !text->isReadOnly() && text->toPlainText()==original,"Binding initial file differs"); reveal(text); QTest::mouseClick(text->viewport(),Qt::LeftButton); QTest::keyClick(text,Qt::Key_A,Qt::ControlModifier); QApplication::clipboard()->setText(edited); QTest::keyClick(text,Qt::Key_V,Qt::ControlModifier); events(); require(text->toPlainText()==edited && read(function)==original.toUtf8() && rig.source->toPlainText()==caller && rig.source->document()->availableUndoSteps()==history && prototypes()==before,"Binding draft changed file/caller/history/prototypes"); auto *button=dialog->findChild<QDialogButtonBox*>()->button(commit?QDialogButtonBox::Save:QDialogButtonBox::Cancel); reveal(button); QTest::mouseClick(button,Qt::LeftButton); visited=true; } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }});
         reveal(edit);modal.start();QTest::mouseClick(edit,Qt::LeftButton);modal.stop(); if (failure) {auto *focus=QApplication::focusWidget();auto *active=QApplication::activeWindow();rig.save("file-editor-failure-diagnostic.json",QJsonDocument(QJsonObject{{"caller_unchanged",rig.source->toPlainText()==caller},{"history_unchanged",rig.source->document()->availableUndoSteps()==history},{"file_unchanged",read(function)==original.toUtf8()},{"focus_widget",focus?QString(focus->metaObject()->className())+":"+focus->objectName():QString()},{"active_window",active?QString(active->metaObject()->className())+":"+active->objectName():QString()}}).toJson());std::rethrow_exception(failure);}
         require(visited && read(function)==(commit?edited:original).toUtf8() && rig.source->toPlainText()==caller,"Binding file Save/Cancel differs"); decisions.append(QJsonObject{{"saved",commit}});
      }
      require(owner->close(),"Binding editor failed to close"); events(); rig.save("edited.gmf",edited.toUtf8()); require(rig.window.saveScriptTo(saved),"Binding Unicode caller Save failed"); QJsonArray phases;

      for(int phase=0;phase<3;++phase)
      {
         if(phase==1)require(rig.window.loadScript(saved)&&rig.window.buildScript()&&rig.source->toPlainText()==caller&&prototypes()==before,"Binding Unicode reopen changed source/prototypes");
         const auto tag=QString::number(phase);BindingObserver observer(rig,spec,tag);Debugger *debug=nullptr;for(auto *child:rig.window.children())if(auto *candidate=dynamic_cast<Debugger*>(child))debug=candidate;
         int arm=-1;const auto snapshot=rig.window.missionSnapshot();for(int i=0;i<snapshot.nodes.size();++i)if(snapshot.nodes[i].statement.contains("Arm bindings"))arm=i;
         require(debug&&arm>=0&&rig.window.setBreakpoint(arm,true),"Binding actual observer arm boundary absent");bool armed=false;std::exception_ptr armError;auto previous=debug->paused;
         debug->paused=[&](const QString &kind){if(previous)previous(kind);try{require(!armed&&kind=="Write","Binding arm command/order differs");observer.SetInternalCoordSystem(Publisher::Instance()->GetInternalCoordSystem());if(auto *frame=Publisher::Instance()->GetDataCoordSystem())observer.SetDataCoordSystem(frame);require(Publisher::Instance()->Subscribe(&observer),"Binding observer subscription failed");armed=true;if(auto *dialog=rig.window.findChild<QDialog*>("debuggerDialog"))acceptanceView(*dialog);}catch(...){armError=std::current_exception();rig.window.stopMission();}QTimer::singleShot(0,&rig.window,[&]{debug->resume();});};
         const int undo=rig.source->document()->availableUndoSteps();rig.messages->clear();acceptanceView(rig.window);const auto outcome=rig.window.runMission();debug->paused=previous;Publisher::Instance()->Unsubscribe(&observer);events();
         const auto messages=rig.messages->toPlainText();rig.save(tag+"-messages.txt",messages.toUtf8());rig.save(tag+"-observer-error.txt",observer.error.toUtf8());if(armError)std::rethrow_exception(armError);if(observer.failure)std::rethrow_exception(observer.failure);
         if(outcome!=MainWindow::RunResult::Completed)std::cerr<<messages.toStdString();require(outcome==MainWindow::RunResult::Completed&&armed&&observer.records.size()==4,"Binding scope mission/callback census failed");
         QJsonArray privateWrites,sharedWrites;
         auto parse=[&](const QString &ref,const QJsonValue &expected){QJsonArray values;const QString delimiters="(?:"+QRegularExpression::escape(reference(spec,false,false))+"|"+QRegularExpression::escape(reference(spec,false,true))+"|"+QRegularExpression::escape(reference(spec,true,true))+"|ScopeEnd)";auto matches=QRegularExpression("(?:^|\\n)"+QRegularExpression::escape(ref)+" =\\s*\\n([\\s\\S]*?)(?=\\n"+delimiters+" =)").globalMatch(messages);while(matches.hasNext()){auto raw=matches.next().captured(1).trimmed();if(expected.isString()){if(raw.startsWith('\'')&&raw.endsWith('\''))raw=raw.mid(1,raw.size()-2);values.append(raw);}else{for(const auto &c:QStringList{"[","]",";",","})raw.replace(c," ");const auto words=raw.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);if(expected.isArray()){QJsonArray array;for(const auto &word:words){bool ok;const double v=word.toDouble(&ok);require(ok&&std::isfinite(v),"Actual binding array Write invalid");array.append(v);}values.append(array);}else{require(words.size()==1,"Actual scalar Write width differs");bool ok;const double v=words[0].toDouble(&ok);require(ok&&std::isfinite(v),"Actual binding scalar Write invalid");values.append(v);}}}return values;};
         const auto first=observer.records[0].toObject()["parameters"].toArray()[0].toObject()["actual"];
         if(reference(spec,false,false)==reference(spec,false,true))privateWrites=parse(reference(spec,false,false),first);
         else{privateWrites=parse(reference(spec,false,false),first);const auto local=parse(reference(spec,false,true),first);for(const auto &v:local)privateWrites.append(v);}
         sharedWrites=parse(reference(spec,true,true),first);require(privateWrites.size()==4&&sharedWrites.size()==4,"Actual main/private/global parameter Write count differs");
         for(int i=0;i<4;++i){const auto parameters=observer.records[i].toObject()["parameters"].toArray();compareValue(privateWrites[i],parameters[0].toObject()["actual"],bound(family,type));compareValue(sharedWrites[i],parameters[1].toObject()["actual"],bound(family,type));}
         const auto bindingBytes=read(directory+"/Bindings.txt");require(!bindingBytes.isEmpty(),"Actual binding parameter Report produced no bytes");rig.save(tag+"-binding-values.txt",bindingBytes);
         const auto raw=read(report);rig.save(tag+"-values.txt",raw);QJsonArray markers;for(const auto &word:QString::fromUtf8(raw).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)){bool ok;const double v=word.toDouble(&ok);require(ok&&std::isfinite(v),"Binding marker report invalid");markers.append(v);}require(markers==QJsonArray{42,43,44,103,104},"Binding file/private/global/caller marker differs");
         auto *root=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject("PrivateOnly"));require(root&&root->GetRealParameter("DryMass")==987&&Moderator::Instance()->GetInternalObject("ScopeEnd")->GetRealParameter("Value")==24680,"Binding private use changed caller shadow/sentinel");
         const auto rootValue=stateJson(root);Rvector6 rootRaw;for(int i=0;i<6;++i)rootRaw[i]=root->GetState()[i];auto *rootCS=dynamic_cast<CoordinateSystem*>(root->GetRefObject(Gmat::COORDINATE_SYSTEM,""));require(rootCS,"Special caller coordinate system absent");compareState(vectorJson(rootCS->FromBaseSystem(A1Mjd(21545),rootRaw,true,true)),observation(spec,false,false,0)["state"]);require(root->GetEpoch()==21545,"Cartesian caller root epoch changed");
         const auto after=prototypes();rig.save(tag+"-prototypes.json",QJsonDocument(after).toJson());const QJsonObject invariants{{"root_selected_property",true},{"prototypes",after==before},{"source",rig.source->toPlainText()==caller},{"history",rig.source->document()->availableUndoSteps()==undo},{"function_file",read(function)==edited.toUtf8()},{"nested_file",read(outer)==outerBody.toUtf8()},{"not_running",!rig.window.isRunning()}};rig.save(tag+"-invariants.json",QJsonDocument(invariants).toJson());for(const auto &v:invariants)require(v.toBool(),"Binding run changed source/history/prototypes/files/lock");
         phases.append(QJsonObject{{"phase",phase},{"explicit_rebuild",phase<2},{"live_samples",observer.records},{"private_writes",privateWrites},{"shared_writes",sharedWrites},{"raw_markers",markers},{"callback_private_maps_accessed_only_while_executing",true}});rig.save("phases.json",QJsonDocument(phases).toJson());
      }
      auto *tree=rig.window.findChild<QTreeWidget*>("Output"); const auto outputs=tree->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive); require(outputs.size()==1,"Binding report output absent"); tree->itemDoubleClicked(outputs[0],0); events(); QMdiSubWindow *viewer=nullptr;QJsonArray viewerCandidates;QElapsedTimer viewerDeadline;viewerDeadline.start();while(!viewer&&viewerDeadline.elapsed()<1000){events();for(auto *candidate:rig.area->subWindowList()){if(dynamic_cast<ReportViewer*>(candidate->widget())&&candidate->widget()->toolTip()==report){require(!viewer,"Duplicate actual ReportViewer for selected file");viewer=candidate;}}if(!viewer)QTest::qWait(10);}for(auto *candidate:rig.area->subWindowList())viewerCandidates.append(QJsonObject{{"title",candidate->windowTitle()},{"object",candidate->widget()->objectName()},{"tooltip",candidate->widget()->toolTip()},{"report_viewer",dynamic_cast<ReportViewer*>(candidate->widget())!=nullptr},{"active",candidate==rig.area->activeSubWindow()}});rig.save("report-viewer-diagnostic.json",QJsonDocument(QJsonObject{{"expected_path",report},{"status",rig.window.statusBar()->currentMessage()},{"candidates",viewerCandidates}}).toJson());require(viewer&&dynamic_cast<ReportViewer*>(viewer->widget()),"Binding actual report viewer absent");rig.area->setActiveSubWindow(viewer); auto *shown=viewer->findChild<QPlainTextEdit*>(); require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(read(report)).trimmed(),"Binding viewer differs from checked bytes"); acceptanceView(*viewer);

      const QJsonObject proof{{"checkpoint","live-function-special-orbit-parameter-owner-binding"},{"outcome","passed"},{"case",key},{"argument",argument},{"mode",spec["mode"]},{"writable_probe",spec["writable_probe"]},{"report_via_scalar_variables",scalarBridge},{"route","special-state-scoped-owner"},{"family",family},{"full_system_parameter_inventory",321},{"initial_batch_types",catalog["initial_types"]},{"missions",3},{"function_calls",12},{"live_callback_samples",12},{"actual_parameter_owner_checks",24},{"file_decisions",decisions},{"phases",phases},{"scope","All321 retained.38 special-state types bind actual main/private/Global Spacecraft owners,actual coordinate/origin/body/SolarSystem references and original metadata. Independent special geometry or explicitly bounded Brouwer prototype parity checks selected values and full scripted Cartesian controls. BodyFixed transport uses measured native rotation/rate inputs,not independent Earth-orientation truth. Strict advertised writes are idempotent only;Brouwer is not independent averaging truth or licensed MATLAB execution. Owners stay at21545 while pulse spacecraft provide live callbacks. No broad frame/provider/propagation/whole-row qualification;all59 holds remain."}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }
   catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}
   catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}
   return 1;
}
