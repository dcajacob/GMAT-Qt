#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ReportViewer.hpp"
#include "ObjectManagedFunction.hpp"
#include "Parameter.hpp"
#include "ParameterInfo.hpp"
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
}


static QString reference(const QJsonObject &spec,bool shared,bool function)
{
   return spec[shared?"shared_reference":function?"local_private_reference":"root_private_reference"].toString();
}
static QJsonValue expectedValue(const QJsonObject &spec,bool shared,bool function,int calls)
{
   return spec[shared?"shared_value":function?"private_value":"root_value"].toDouble()+(function?calls*spec["step"].toDouble():0);
}
static double bound(const QString &,const QString &) { return 1e-12; }
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
class BindingObserver final:public Subscriber
{
public:
   BindingObserver(Rig &rig,const QJsonObject &spec,const QString &prefix):Subscriber("RotationParameterBindingObserver","RotationParameterBindingObserver"),rig(&rig),spec(spec),prefix(prefix){}
   ~BindingObserver() override{Publisher::Instance()->Unsubscribe(this);}
   GmatBase *Clone() const override{return new BindingObserver(*this);}
   bool ReceiveData(const Real *data,Integer count=0) override
   {
      bool initial=false;if(!Publisher::GetPublishedPropagationObjects(data,count,initial)||!initial)return true;
      try
      {
         auto *command=dynamic_cast<GmatCommand*>(currentProvider);require(command,"Binding publication provider is not a command");
         auto *function=dynamic_cast<ObjectManagedFunction*>(command->GetCurrentFunction());const bool inside=function!=nullptr;
         // These maps are inspected synchronously while their owning function is
         // executing. No private pointer is dereferenced after callback return.
         const auto *locals=inside?function->GetFunctionObjectStore():command->GetObjectMap();
         const auto rootGlobals=inside?ObjectMap{}:Moderator::Instance()->GetSandbox()->GetGlobalObjectMap();
         const auto *globals=inside?function->GetGlobalObjectStore():&rootGlobals;
         require(locals&&globals,"Live binding scope map absent");
         auto lookup=[&](const std::string &name){const auto a=locals->find(name);if(a!=locals->end())return a->second;const auto b=globals->find(name);return b==globals->end()?nullptr:b->second;};
         auto *counter=lookup("Calls");require(counter,"Binding counter absent");const int calls=int(counter->GetRealParameter("Value"));
         QJsonArray localKeys,globalKeys;for(const auto &v:*locals)localKeys.append(QString::fromStdString(v.first));for(const auto &v:*globals)globalKeys.append(QString::fromStdString(v.first));
         events.append(QJsonObject{{"inside_function",inside},{"function",inside?QString::fromStdString(function->GetName()):QString()},{"calls",calls},{"completed_samples",records.size()},{"local_keys",localKeys},{"global_keys",globalKeys},{"provider",QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS))}});rig->save(prefix+"-callback-events.json",QJsonDocument(events).toJson());
         require(calls==(inside?events.size()-1:0)&&(!inside||function->GetName()=="ReadShared"),"Binding callback order/function identity differs");
         const auto family=spec["family"].toString(),type=spec["factory_type"].toString();const auto meta=spec["metadata"].toObject();QJsonArray parameters;
         for(bool shared:{false,true})
         {
            const QString ownerName=shared?"SharedSC":"PrivateOnly",full=reference(spec,shared,inside);auto *owner=dynamic_cast<Spacecraft*>(lookup(ownerName.toStdString()));
            auto *root=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject(ownerName.toStdString()));auto *configured=Moderator::Instance()->GetConfiguredObject(ownerName.toStdString());
            require(owner&&root&&configured&&owner!=configured&&owner->IsOfType(meta["owner_type_id"].toInt()),"Live native owner/class/prototype differs");
            require(owner->IsGlobal()==shared&&owner->IsLocal()==(inside&&!shared)&&(shared?owner==root:inside?owner!=root:owner==root),"Private/global owner identity or flags differ");
            auto *parameter=dynamic_cast<Parameter*>(lookup(full.toStdString()));require(parameter,"Live actual Parameter instance missing from scope map");
            require(parameter->IsSystemParameter()&&parameter->GetName()==full.toStdString()&&parameter->GetTypeName()==meta["native_type"].toString().toStdString()&&int(parameter->GetReturnType())==meta["return_type"].toInt()&&int(parameter->GetOwnerType())==meta["owner_type_id"].toInt(),"Actual Parameter factory/type/owner metadata differs");
            auto *referent=parameter->GetRefObject(parameter->GetOwnerType(),ownerName.toStdString());require(referent==owner,"Actual Parameter reference does not bind live scoped owner");
            QJsonValue actual;QJsonArray shape;
            switch(parameter->GetReturnType())
            {
               case Gmat::REAL_TYPE:actual=parameter->EvaluateReal();break;
               case Gmat::STRING_TYPE:actual=QString::fromStdString(parameter->EvaluateString());break;
               case Gmat::RVECTOR_TYPE:{QJsonArray values;const auto &v=parameter->EvaluateRvector();for(int i=0;i<v.GetSize();++i)values.append(v[i]);actual=values;shape=QJsonArray{v.GetSize(),1};break;}
               case Gmat::RMATRIX_TYPE:{QJsonArray values;const auto &v=parameter->EvaluateRmatrix();for(int i=0;i<v.GetNumRows();++i)for(int j=0;j<v.GetNumColumns();++j)values.append(v(i,j));actual=values;shape=QJsonArray{v.GetNumRows(),v.GetNumColumns()};break;}
               default:require(false,"Unsupported selected parameter return kind");
            }
            const auto expected=expectedValue(spec,shared,inside,calls);const auto kind=spec["hardware_kind"].toString(),hardwareName=(shared?"Shared":inside?"Local":"Root")+kind;
            const auto hardwareType=kind=="Tank"?Gmat::FUEL_TANK:Gmat::THRUSTER;
            auto *hardware=owner->GetRefObject(hardwareType,hardwareName.toStdString());auto *rootHardware=root->GetRefObject(hardwareType,(shared?"Shared": "Root")+kind.toStdString());auto *configuredHardware=Moderator::Instance()->GetConfiguredObject((shared?"Shared": "Root")+kind.toStdString());
            require(hardware&&rootHardware&&configuredHardware&&hardware->GetName()==hardwareName.toStdString()&&hardware->GetTypeName()==spec["hardware_type"].toString().toStdString(),"Actual hardware name/type/reference missing");
            require(hardware!=configuredHardware&&(shared?hardware==rootHardware:inside?hardware!=rootHardware:hardware==rootHardware),"Private/global actual hardware clone identity differs");
            const double direct=hardware->GetRealParameter(type.toStdString());compareValue(actual,expected,1e-12);compareValue(direct,expected,1e-12);
            QJsonArray rotation,location,primary,secondary,nativeMatrix;
            for(int i=0;i<3;++i){const auto axis=std::string(1,"XYZ"[i]);location.append(hardware->GetRealParameter("HWOriginInBCS"+axis));primary.append(hardware->GetRealParameter("Direction"+axis));secondary.append(hardware->GetRealParameter("SecondDirection"+axis));for(int j=0;j<3;++j)rotation.append(hardware->GetRealParameter("R_SB"+std::to_string(i+1)+std::to_string(j+1)));}
            const auto &matrix=hardware->GetRmatrixParameter("RotationMatrix");require(matrix.GetNumRows()==3&&matrix.GetNumColumns()==3,"Hardware rotation matrix shape differs");for(int i=0;i<3;++i)for(int j=0;j<3;++j)nativeMatrix.append(matrix(i,j));
            const QString scope=shared?"shared_":inside?"private_":"root_";compareValue(rotation,spec[scope+"matrix"],1e-12);compareValue(nativeMatrix,spec[scope+"matrix"],1e-12);compareValue(primary,spec[scope+"primary"],1e-12);compareValue(secondary,spec[scope+"secondary"],1e-12);
            require(location==QJsonArray{0,0,0}&&hardware->GetStringParameter("RotationOption")=="Construct","Construct/zero-offset tank fixture differs");
            for(int i=0;i<3;++i)for(int j=0;j<3;++j){double dot=0;for(int k=0;k<3;++k)dot+=rotation[i*3+k].toDouble()*rotation[j*3+k].toDouble();require(std::abs(dot-(i==j?1:0))<=1e-12,"Tank rotation is not orthonormal");}
            const double determinant=rotation[0].toDouble()*(rotation[4].toDouble()*rotation[8].toDouble()-rotation[5].toDouble()*rotation[7].toDouble())-rotation[1].toDouble()*(rotation[3].toDouble()*rotation[8].toDouble()-rotation[5].toDouble()*rotation[6].toDouble())+rotation[2].toDouble()*(rotation[3].toDouble()*rotation[7].toDouble()-rotation[4].toDouble()*rotation[6].toDouble());require(std::abs(determinant-1)<=1e-12,"Tank rotation handedness differs");
            const bool infoSettable=ParameterInfo::Instance()->IsSettable(type.toStdString());require(parameter->IsSettable()==meta["settable"].toBool()&&infoSettable==meta["settable"].toBool(),"Original rotation writable metadata changed");
            QString setterResult="accepted",setterError;
            try{parameter->SetReal(expected.toDouble());}catch(BaseException &e){setterResult="rejected";setterError=QString::fromStdString(e.GetFullMessage());}
            const double postValue=parameter->EvaluateReal();QJsonArray postMatrix;const auto &after=hardware->GetRmatrixParameter("RotationMatrix");for(int i=0;i<3;++i)for(int j=0;j<3;++j)postMatrix.append(after(i,j));compareValue(postValue,expected,1e-12);compareValue(postMatrix,spec[scope+"matrix"],1e-12);
            if(setterResult=="rejected")require(postMatrix==rotation&&postValue==actual.toDouble(),"Rejected rotation setter changed owner or cache value");
            const QJsonObject attached{{"name",hardwareName},{"type",QString::fromStdString(hardware->GetTypeName())},{"pointer",pointer(hardware)},{"root_pointer",pointer(rootHardware)},{"configured_pointer",pointer(configuredHardware)},{"property",type},{"direct_value",direct},{"R_SB",rotation},{"RotationMatrix",nativeMatrix},{"primary",primary},{"secondary",secondary},{"location",location},{"rotation_option","Construct"},{"determinant",determinant},{"runtime_settable",parameter->IsSettable()},{"info_settable",infoSettable},{"native_field_readonly",hardware->IsParameterReadOnly(type.toStdString())},{"native_setter_result",setterResult},{"native_setter_error",setterError},{"post_value",postValue},{"post_matrix",postMatrix}};
            rig->save(prefix+"-hardware-geometry.json",QJsonDocument(QJsonObject{{"reference",full},{"attached",attached}}).toJson());
            parameters.append(QJsonObject{{"reference",full},{"factory_type",type},{"native_type",QString::fromStdString(parameter->GetTypeName())},{"return_type",int(parameter->GetReturnType())},{"owner_type",int(parameter->GetOwnerType())},{"parameter_pointer",pointer(parameter)},{"reference_owner_pointer",pointer(referent)},{"live_owner_pointer",pointer(owner)},{"root_owner_pointer",pointer(root)},{"configured_owner_pointer",pointer(configured)},{"direct_owner_property_value",direct},{"base_bookkeeping_owner_pointer",pointer(parameter->GetOwner())},{"owner_name",ownerName},{"owner_type_name",QString::fromStdString(owner->GetTypeName())},{"owner_global",owner->IsGlobal()},{"owner_local",owner->IsLocal()},{"parameter_global",parameter->IsGlobal()},{"parameter_local",parameter->IsLocal()},{"shared",shared},{"shape",shape},{"actual",actual},{"expected",expected},{"absolute_bound",bound(family,type)},{"attached",attached}});
         }
         records.append(QJsonObject{{"inside_function",inside},{"function",inside?QString::fromStdString(function->GetName()):QString()},{"calls",calls},{"provider",QString::fromStdString(command->GetGeneratingString(Gmat::NO_COMMENTS))},{"locals_pointer",pointer(locals)},{"globals_pointer",pointer(globals)},{"global_map_is_value_snapshot",!inside},{"parameters",parameters}});
         rig->save(prefix+"-live-parameter-samples.json",QJsonDocument(records).toJson());
      }
      catch(BaseException &e){if(!failure){error=QString::fromStdString(e.GetFullMessage());failure=std::current_exception();}}
      catch(std::exception &e){if(!failure){error=e.what();failure=std::current_exception();}}
      return true;
   }
   QJsonArray records,events;std::exception_ptr failure;QString error;
protected:
   bool Distribute(Integer) override{return true;}
   bool Distribute(const Real *,Integer) override{return true;}
private:
   Rig *rig;QJsonObject spec;QString prefix;
};
static QString spacecraft(const QString &name,int x,int mass,int id,const QString &stamp=QString())
{
   QString code="Create Spacecraft "+name+";\n"+name+".CoordinateSystem = EarthMJ2000Eq;\n"+name+".DateFormat = "+(stamp.isEmpty()?"A1ModJulian":"UTCGregorian")+";\n"+name+".Epoch = '"+(stamp.isEmpty()?"21545":stamp)+"';\n"+name+".DisplayStateType = Cartesian;\n"+name+".X = "+QString::number(x)+";\n"+name+".Y = 100;\n"+name+".Z = 200;\n"+name+".VX = -0.1;\n"+name+".VY = 7.5;\n"+name+".VZ = 0.8;\n"+name+".DryMass = "+QString::number(mass)+";\n"+name+".NAIFId = "+QString::number(id)+";\n"+name+".NAIFIdReferenceFrame = "+QString::number(id-8000000)+";\n";return code;
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("FunctionRotationParameterBinding");
   try
   {
      require(argc==4&&acceptanceWidgetPlatform(),"Supply parameter-binding startup,catalog and type");Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();const QString argument=argv[3],key=argument.endsWith("WritableProbe")?argument.chopped(QString("WritableProbe").size()):argument;const bool writableProbe=argument.endsWith("WritableProbe");QJsonObject spec;QStringList inventory;
      for(const auto &v:catalog["cases"].toArray())inventory.append(v.toObject()["factory_type"].toString());for(const auto &v:catalog["rotation_binding_cases"].toArray())if(v.toObject()["factory_type"]==key)spec=v.toObject();
      require(catalog["factory_rows"]==566&&inventory.size()==321&&!spec.isEmpty()&&catalog["initial_types"].toArray().contains(key)&&spec["family"]=="AttachedHardwareRotation","Full parameter inventory or selected owner fixture differs");
      QStringList actual;for(const auto &name:Moderator::Instance()->GetListOfFactoryItems(Gmat::PARAMETER))actual.append(QString::fromStdString(name));inventory+=QStringList{"Array","String","Variable"};inventory.sort();actual.sort();require(actual==inventory,"Advertised complete ParameterFactory universe changed");
      const auto family=spec["family"].toString(),type=spec["factory_type"].toString();std::unique_ptr<Parameter> prototype(FactoryManager::Instance()->CreateParameter(type.toStdString()));
      require(prototype&&prototype->IsSystemParameter()&&prototype->GetTypeName()==spec["metadata"].toObject()["native_type"].toString().toStdString(),"Selected unmanaged native factory control differs");
      require(spec["root_value"]!=spec["private_value"]&&spec["root_value"]!=spec["shared_value"]&&spec["private_value"]!=spec["shared_value"],"Rotation selected scalar stamps do not distinguish all three scopes");
      const auto directory=rig.files.filePath("Live parameter owner Ω");require(QDir().mkpath(directory),"Binding Unicode directory absent");
      const auto function=directory+"/ReadShared.gmf",outer=directory+"/NestedShared.gmf",saved=directory+"/Caller.script",report=directory+"/Values.txt";
      auto literal=[](double v){return QString::number(v,'g',17);};
      const auto kind=spec["hardware_kind"].toString();
      auto hardwareSetup=[&](const QString &name,const QString &scope){QString code="Create ChemicalTank "+name+";\n"+name+".FuelMass = 100;\n"+name+".PressureModel = PressureRegulated;\n";for(int i=0;i<3;++i){const QString axis=QString("XYZ")[i];code+=name+".Direction"+axis+" = "+literal(spec[scope+"primary"].toArray()[i].toDouble())+";\n"+name+".SecondDirection"+axis+" = "+literal(spec[scope+"secondary"].toArray()[i].toDouble())+";\n";}return code;};
      auto attach=[&](const QString &owner,const QString &hardware){return owner+".Tanks = {"+hardware+"};\n";};
      QString declaration=hardwareSetup("LocalTank","private_")+spacecraft("PrivateOnly",7500,275,-121101)+spacecraft("ScopePulse",9000,701,-121102)+attach("PrivateOnly","LocalTank");
      const QString mutate=writableProbe?"PrivateOnly.LocalTank."+type+" = "+literal(spec["private_value"].toDouble())+";\nSharedSC.SharedTank."+type+" = "+literal(spec["shared_value"].toDouble())+";\n":QString();
      const bool vectorBridge=type=="Cartesian";const QString vectorDeclarations=vectorBridge?"Create Array PrivateVector[6,1] SharedVector[6,1];\n":QString();
      auto actualReport=[&](bool inside){if(!vectorBridge)return "Report BindingValues "+reference(spec,false,inside)+" "+reference(spec,true,inside)+";\n";QString code="PrivateVector = "+reference(spec,false,inside)+";\nSharedVector = "+reference(spec,true,inside)+";\nReport BindingValues";for(const auto &name:QStringList{"PrivateVector","SharedVector"})for(int i=1;i<=6;++i)code+=" "+name+"("+QString::number(i)+",1)";return code+";\n";};
      const QString original="function [Mark] = ReadShared()\n"+declaration+vectorDeclarations+"Create Variable Mark Keep ScopeEnd;\nKeep = 103;\nScopeEnd = 12345;\nBeginMissionSequence;\nGlobal SharedSC Calls BindingValues;\nCalls = Calls + 1;\n"+mutate+"Write "+reference(spec,false,true)+" "+reference(spec,true,true)+" ScopeEnd {Style = Verbose, MessageWindow = true, LogFile = false};\n"+actualReport(true)+"Propagate PulseProp(ScopePulse) {ScopePulse.ElapsedSecs = 1};\nMark = Keep + Calls;\n% Live binding ownership Ω\n";
      const auto edited=QString(original).replace("Keep = 103;","Keep = 41;");
      const QString outerBody="function [Mark] = NestedShared()\nCreate Variable Mark;\nCreate GmatFunction ReadShared;\nReadShared.FunctionPath = '"+function+"';\nBeginMissionSequence;\n[Mark] = ReadShared();\n";
      write(function,original.toUtf8());write(outer,outerBody.toUtf8());rig.save("initial.gmf",original.toUtf8());rig.save("nested.gmf",outerBody.toUtf8());
      QString caller=hardwareSetup("RootTank","root_")+hardwareSetup("SharedTank","shared_")+spacecraft("PrivateOnly",7000,987,-121001)+spacecraft("SharedSC",8000,150,-121002)+spacecraft("MainPulse",9000,702,-121003)+attach("PrivateOnly","RootTank")+attach("SharedSC","SharedTank");
      caller+=vectorDeclarations+"Create ForceModel PulseFM;\nPulseFM.PrimaryBodies = {};\nPulseFM.PointMasses = {Earth};\nPulseFM.Drag = None;\nPulseFM.SRP = Off;\nCreate Propagator PulseProp;\nPulseProp.FM = PulseFM;\nPulseProp.Type = RungeKutta89;\nPulseProp.InitialStepSize = 1;\nPulseProp.MinStep = 0;\nPulseProp.MaxStep = 1;\nCreate Variable First Second Third Keep Calls ScopeEnd;\nKeep = 103;\nCalls = 0;\nScopeEnd = 24680;\nCreate GmatFunction ReadShared NestedShared;\nReadShared.FunctionPath = '"+function+"';\nNestedShared.FunctionPath = '"+outer+"';\nCreate ReportFile BindingValues;\nBindingValues.Filename = '"+directory+"/Bindings.txt';\nBindingValues.WriteHeaders = false;\nBindingValues.FixedWidth = false;\nBindingValues.Precision = 17;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Precision = 17;\nBeginMissionSequence;\nGlobal SharedSC Calls BindingValues;\nWrite 'Arm bindings' Keep {Style = Verbose, MessageWindow = true, LogFile = false};\nWrite "+reference(spec,false,false)+" "+reference(spec,true,false)+" ScopeEnd {Style = Verbose, MessageWindow = true, LogFile = false};\n"+actualReport(false)+"Propagate PulseProp(MainPulse) {MainPulse.ElapsedSecs = 1};\n[First] = ReadShared();\n[Second] = ReadShared();\n[Third] = NestedShared();\nReport Values First Second Third Keep;\nKeep = Keep + 1;\nReport Values Keep;\n";
      rig.source->setPlainText(caller);rig.save("caller.script",caller.toUtf8());const bool built=rig.window.buildScript();rig.save("build-messages.txt",rig.messages->toPlainText().toUtf8());if(!built){std::cerr<<rig.messages->toPlainText().toStdString();QJsonArray errors;for(const auto &e:Moderator::GetScriptInterpreter()->GetErrorList()){errors.append(QString::fromStdString(e));std::cerr<<e<<std::endl;}rig.save("build-errors.json",QJsonDocument(errors).toJson());}require(built,"Binding main parameter caller Build failed");
      auto prototypes=[&]{QJsonObject values;for(const auto &name:QStringList{"PrivateOnly","SharedSC","Calls","Keep","ScopeEnd","Root"+kind,"Shared"+kind}){auto *p=Moderator::Instance()->GetConfiguredObject(name.toStdString());require(p,"Binding prototype absent");values[name]=QString::fromStdString(p->GetGeneratingString(Gmat::NO_COMMENTS));}return values;};const auto before=prototypes();rig.save("prototypes-before.json",QJsonDocument(before).toJson());
      for (auto *tabs:rig.window.findChildren<QTabWidget*>()) for (int i=0;i<tabs->count();++i) if (tabs->widget(i)==rig.resources || tabs->widget(i)->isAncestorOf(rig.resources)) tabs->setCurrentIndex(i);
      const auto entries=rig.resources->findItems("ReadShared",Qt::MatchExactly|Qt::MatchRecursive); require(entries.size()==1,"Binding function resource absent"); rig.resources->scrollToItem(entries[0]); rig.resources->itemDoubleClicked(entries[0],0); events();
      auto *owner=rig.area->activeSubWindow(); auto *panel=owner?dynamic_cast<ResourceEditor*>(owner->widget()):nullptr; require(panel,"Binding function editor absent"); owner->showNormal(); owner->move(0,0); owner->resize(rig.area->viewport()->size()); events(); auto *edit=panel->findChild<QPushButton*>("editFunctionFile"); reveal(edit);
      const int history=rig.source->document()->availableUndoSteps(); QJsonArray decisions;
      for (bool commit:{false,true})
      {
         bool visited=false; std::exception_ptr failure;
         QTimer modal;modal.setInterval(20);QObject::connect(&modal,&QTimer::timeout,panel,[&]{auto *dialog=panel->findChild<QDialog*>("functionFileDialog");if(!dialog)return;modal.stop();try { require(dialog,"Binding file dialog absent"); auto *text=dialog->findChild<QPlainTextEdit*>("functionFileText"); require(text && !text->isReadOnly() && text->toPlainText()==original,"Binding initial file differs"); reveal(text); QTest::mouseClick(text->viewport(),Qt::LeftButton); QTest::keyClick(text,Qt::Key_A,Qt::ControlModifier); QApplication::clipboard()->setText(edited); QTest::keyClick(text,Qt::Key_V,Qt::ControlModifier); events(); require(text->toPlainText()==edited && read(function)==original.toUtf8() && rig.source->toPlainText()==caller && rig.source->document()->availableUndoSteps()==history && prototypes()==before,"Binding draft changed file/caller/history/prototypes"); auto *button=dialog->findChild<QDialogButtonBox*>()->button(commit?QDialogButtonBox::Save:QDialogButtonBox::Cancel); reveal(button); QTest::mouseClick(button,Qt::LeftButton); visited=true; } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }});
         reveal(edit);modal.start();QTest::mouseClick(edit,Qt::LeftButton);modal.stop(); if (failure) std::rethrow_exception(failure);
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
         auto *root=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject("PrivateOnly"));require(root&&root->GetRealParameter("DryMass")==987&&root->GetState()[0]==7000&&Moderator::Instance()->GetInternalObject("ScopeEnd")->GetRealParameter("Value")==24680,"Binding private use changed caller shadow/sentinel");
         auto *hardware=root->GetRefObject(kind=="Tank"?Gmat::FUEL_TANK:Gmat::THRUSTER,("Root"+kind).toStdString());require(hardware&&std::abs(hardware->GetRealParameter(type.toStdString())-spec["root_value"].toDouble())<=1e-12,"Private hardware mutation escaped to caller shadow");
         const auto after=prototypes();rig.save(tag+"-prototypes.json",QJsonDocument(after).toJson());const QJsonObject invariants{{"root_selected_property",std::abs(hardware->GetRealParameter(type.toStdString())-spec["root_value"].toDouble())<=bound(family,type)},{"prototypes",after==before},{"source",rig.source->toPlainText()==caller},{"history",rig.source->document()->availableUndoSteps()==undo},{"function_file",read(function)==edited.toUtf8()},{"nested_file",read(outer)==outerBody.toUtf8()},{"not_running",!rig.window.isRunning()}};rig.save(tag+"-invariants.json",QJsonDocument(invariants).toJson());for(const auto &v:invariants)require(v.toBool(),"Binding run changed source/history/prototypes/files/lock");
         phases.append(QJsonObject{{"phase",phase},{"explicit_rebuild",phase<2},{"live_samples",observer.records},{"private_writes",privateWrites},{"shared_writes",sharedWrites},{"raw_markers",markers},{"callback_private_maps_accessed_only_while_executing",true}});rig.save("phases.json",QJsonDocument(phases).toJson());
      }
      auto *tree=rig.window.findChild<QTreeWidget*>("Output"); const auto outputs=tree->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive); require(outputs.size()==1,"Binding report output absent"); tree->itemDoubleClicked(outputs[0],0); events(); auto *viewer=rig.area->activeSubWindow(); require(viewer && dynamic_cast<ReportViewer*>(viewer->widget()),"Binding actual report viewer absent"); auto *shown=viewer->findChild<QPlainTextEdit*>(); require(shown && shown->isReadOnly() && shown->toPlainText().trimmed()==QString::fromUtf8(read(report)).trimmed(),"Binding viewer differs from checked bytes"); acceptanceView(*viewer);

      const QJsonObject proof{{"checkpoint","live-function-rotation-parameter-owner-binding"},{"outcome","passed"},{"case",key},{"argument",argument},{"route","attached-hardware-rotation-field"},{"writable_probe",writableProbe},{"factory_settable",prototype->IsSettable()},{"family",family},{"full_system_parameter_inventory",321},{"initial_batch_types",catalog["initial_types"]},{"missions",3},{"function_calls",12},{"live_callback_samples",12},{"actual_parameter_owner_checks",24},{"file_decisions",decisions},{"phases",phases},{"scope","All321 original types/metadata retained. Nine R_SB Parameter owner routes use distinct configured nonidentity ChemicalTank orientations and independent rational matrices in main/private/Global scopes. Native setters are probed idempotently and outcomes recorded without requiring rejection;strict WritableProbe missions separately require supported idempotent assignments and remain unqualified if rejected. No writable reclassification,whole requirement credit,general arbitrary/degenerate orientations,refresh-after-direction-change,hardware translation,body-frame dynamics or science approval. Exact caller/prototype/history/file/Unicode/no-Build and Write/Report checks;no expired private-map access."}};
      rig.save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }
   catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}
   catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}
   return 1;
}
