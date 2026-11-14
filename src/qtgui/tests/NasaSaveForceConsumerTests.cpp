// Reuse the unchanged independent native descriptor/owned-object readers.
// The original generic Save main and every assertion remain available intact.
#define main retainedSaveResourceMain
#include "NasaSaveResourceTests.cpp"
#undef main
#include "ODEModel.hpp"
#include "PhysicalModel.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "PropagationStateManager.hpp"
#include "GmatCommand.hpp"
#include <algorithm>
#include <vector>

static QJsonArray finiteVector(const Real *data, int count)
{
   require(data && count > 0, "Save consumer native vector absent");
   QJsonArray result;
   for (int i=0; i<count; ++i) result.append(number(data[i]));
   return result;
}

static QJsonObject configuredSnapshot()
{
   QJsonObject result;
   for (const auto &name: {"FM", "Prop", "Sat"}) {
      auto *object=Moderator::Instance()->GetConfiguredObject(name);
      require(object, "Save consumer configured object absent");
      std::set<GmatBase *> active;
      result[name]=snapshot(*object,active);
   }
   return result;
}

static QJsonObject savedRuntimeSnapshot()
{
   QJsonObject result;
   for (const auto &name: {"FM", "Prop", "Sat"}) {
      auto *object=Moderator::Instance()->GetInternalObject(name);
      require(object, "Save consumer exported runtime object absent");
      std::set<GmatBase *> active;
      result[name]=snapshot(*object,active);
   }
   return result;
}

static QJsonObject consumer(double seconds,double elapsedBound)
{
   auto *command=Moderator::Instance()->GetFirstCommand();
   while (command && !command->IsOfType("Propagate")) command=command->GetNext();
   auto *setup=command?dynamic_cast<PropSetup *>(command->GetClone(0)):nullptr;
   auto *model=setup?setup->GetODEModel():nullptr;
   auto *sat=dynamic_cast<Spacecraft *>(Moderator::Instance()->GetInternalObject("Sat"));
   require(setup && setup->GetPropagator() && model && sat,
           "Save consumer actual propagation clone/spacecraft absent");
   const QJsonObject timing{{"expected_seconds",seconds},
      {"integrator",QString::fromStdString(setup->GetPropagator()->GetTypeName())},
      {"integrator_seconds",setup->GetPropagator()->GetTime()},
      {"model_seconds",model->GetTime()},{"spacecraft_epoch",sat->GetEpoch()}};
   const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
   if (!evidence.isEmpty()) write(QDir(evidence).filePath(QString::number(seconds,'g',17)+"-latest-timing.json"),QJsonDocument(timing).toJson());
   std::cout<<"NASA_DIAGNOSTIC "<<QJsonDocument(timing).toJson(QJsonDocument::Compact).constData()<<std::endl;
   require(setup->GetPropagator()->GetTypeName()=="RungeKutta89"
           && std::abs(setup->GetPropagator()->GetTime()-seconds)<elapsedBound,
           "Save consumer actual integrator/time differs");
   auto *manager=setup->GetPropStateManager();
   auto *packed=manager?manager->GetState():nullptr;
   require(packed && packed->GetSize()==42 && model->GetDimension()==42,
           "Save consumer omitted six-state/full36-element STM propagation");
   const auto nativePacked=finiteVector(packed->GetState(),42);
   require(same(nativePacked,finiteVector(setup->GetPropagator()->GetState(),42)),
           "Save consumer integrator/manager packed state differs");
   require(model->GetDerivatives(packed->GetState(),0,1),
           "Save consumer actual ODE derivative evaluation failed");
   const auto derivative=finiteVector(model->GetDerivativeArray(),42);
   const auto state=finiteVector(sat->GetState().GetState(),6);
   const auto &stm=sat->GetRmatrixParameter(sat->GetParameterID("OrbitSTM"));
   require(stm.GetNumRows()==6 && stm.GetNumColumns()==6,
           "Save consumer native OrbitSTM dimensions differ");
   QJsonArray matrix;
   for (int i=0; i<6; ++i) {
      QJsonArray row;
      for (int j=0; j<6; ++j) {
         row.append(number(stm(i,j)));
         require(same(row[j],nativePacked[6+i*6+j]), "Save consumer native/packed STM differs");
      }
      matrix.append(row);
      require(same(state[i],nativePacked[i]), "Save consumer native/packed Cartesian state differs");
   }
   QJsonArray order;
   QJsonObject forces;
   bool activeDrag=false,activeRelativity=false;
   for (int i=0; i<model->GetNumForces(); ++i) {
      auto *force=model->GetForce(i);
      require(force, "Save consumer native force absent");
      const auto type=QString::fromStdString(force->GetTypeName());
      const auto name=QString::fromStdString(force->GetName());
      const auto identity=type+":"+name;
      require(!forces.contains(identity), "Save consumer duplicate force identity");
      const auto forceValue=force->GetDerivativesForSpacecraft(sat);
      QJsonArray values;
      double acceleration=0;
      for (int j=0; j<6; ++j) {
         values.append(number(forceValue[j]));
         if (j>=3) acceleration+=forceValue[j]*forceValue[j];
      }
      order.append(QJsonArray{type,name});
      forces[identity]=values;
      if (type=="DragForce") activeDrag=acceleration>0;
      if (type=="RelativisticCorrection") activeRelativity=acceleration>0;
   }
   require(activeDrag && activeRelativity && forces.size()==5,
           "Save consumer did not exercise all five forces with nonzero drag/relativity");
   require(sat->GetRealParameter("DragArea")>0 && sat->GetRealParameter("SRPArea")>0,
           "Save consumer silently substituted a null-area trajectory");
   return QJsonObject{{"seconds",seconds},{"epoch",number(sat->GetEpoch())},
      {"state",state},{"STM",matrix},{"packed_state",nativePacked},{"derivative",derivative},
      {"force_order",order},{"force_derivatives",forces},{"dimension",42},
      {"integrator_seconds",number(setup->GetPropagator()->GetTime())},
      {"mass",number(sat->GetRealParameter("TotalMass"))},
      {"DragArea",number(sat->GetRealParameter("DragArea"))},
      {"SRPArea",number(sat->GetRealParameter("SRPArea"))}};
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");
   app.setApplicationName("NasaSaveForceConsumer");
   try {
      require(argc==4 && acceptanceWidgetPlatform(), "Supply startup, Save consumer catalog and key");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      const auto catalog=QJsonDocument::fromJson(read(QFileInfo(argv[2]).absoluteFilePath())).object();
      QJsonObject spec;
      for (const auto &entry:catalog["cases"].toArray())
         if (entry.toObject()["key"].toString()==argv[3]) spec=entry.toObject();
      require(!spec.isEmpty() && catalog["relative_bound"].toDouble()==1e-12
              && catalog["elapsed_bound_seconds"].toDouble()==1e-5,
              "Save consumer case or original strict comparison bound changed");
      TestSettings settings;
      QTemporaryDir files;
      require(files.isValid(), "Save consumer temporary output unavailable");
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window;
      require(window.initialize(startup), "Save consumer GUI runtime unavailable");
      window.resize(1200,850);window.show();events();acceptanceView(window);
      auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");
      auto *messages=window.findChild<QPlainTextEdit *>("messageWindow");
      require(source && messages, "Save consumer GUI workspace absent");
      FileManager::Instance()->SetAbsPathname("OUTPUT_PATH",files.path().toStdString());
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      auto save=[&](const QString &name,const QByteArray &bytes) {
         if (!evidence.isEmpty()) write(QDir(evidence).filePath(name),bytes);
      };
      auto json=[&](const QString &name,const QJsonObject &value) {save(name,QJsonDocument(value).toJson());};
      json("case.json",spec);
      const QString keep="Create Variable Keep;\nKeep = 41;\n";
      const QString saveCommand="Save 'ExportFamily' FM Prop Sat;\n";
      const auto exportScript=spec["setup"].toString()+spec["spacecraft_setup"].toString()+keep+
         "BeginMissionSequence;\n"+saveCommand+"Keep = Keep + 1; % export sentinel Ω\n";
      source->setPlainText(exportScript);save("export-only.script",exportScript.toUtf8());
      const bool exportBuilt=window.buildScript();
      save("export-build.messages.txt",messages->toPlainText().toUtf8());
      require(exportBuilt, "Save consumer export-only build failed");
      json("configured-before-export-native.json",configuredSnapshot());
      const int exportUndo=source->document()->availableUndoSteps();
      acceptanceView(window);
      const auto exportStatus=window.runMission();events();
      save("export-run.messages.txt",messages->toPlainText().toUtf8());
      require(exportStatus==MainWindow::RunResult::Completed
              && Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,
              "Save consumer export-only mission/following statement failed");
      require(source->toPlainText()==exportScript && source->document()->availableUndoSteps()==exportUndo,
              "Save consumer export-only Run changed source/history");
      const auto before=savedRuntimeSnapshot();json("saved-runtime-native.json",before);
      const auto path=files.filePath("FM_Prop_Sat.data");
      const auto firstExport=read(path);save("export.data",firstExport);
      require(!firstExport.isEmpty() && !firstExport.contains("BeginMissionSequence") && !firstExport.contains("Keep"),
              "Save consumer export includes unselected mission/support state");
      QJsonArray observations;
      int checked=0;
      for (const auto &duration:catalog["durations_seconds"].toArray()) {
         const double seconds=duration.toDouble();
         const auto label=QString::number(seconds,'g',17);
         const auto continuation="Propagate 'Consumer' Prop(Sat, STM) {Sat.ElapsedSecs = "+label+"};\nKeep = Keep + 1; % following sentinel Ω\n";
         const auto initial=spec["setup"].toString()+spec["spacecraft_setup"].toString()+keep+
            "BeginMissionSequence;\n"+continuation;
         save(label+"-original.script",initial.toUtf8());
         source->setPlainText(initial);
         const bool originalBuilt=window.buildScript();
         save(label+"-original-build.messages.txt",messages->toPlainText().toUtf8());
         require(originalBuilt, "Save consumer original build failed");
         json(label+"-original-native.json",before);
         const int undo=source->document()->availableUndoSteps();
         acceptanceView(window);
         const auto originalStatus=window.runMission();events();
         save(label+"-original-run.messages.txt",messages->toPlainText().toUtf8());
         require(originalStatus==MainWindow::RunResult::Completed,
                 "Save consumer original five-force/STM mission failed");
         require(source->toPlainText()==initial && source->document()->availableUndoSteps()==undo,
                 "Save consumer original Run changed source/history");
         require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,
                 "Save consumer original following statement failed");
         const auto a=consumer(seconds,catalog["elapsed_bound_seconds"].toDouble());json(label+"-original-consumer.json",a);
         const auto raw=read(path);save(label+"-export.data",raw);
         require(!raw.isEmpty() && !raw.contains("BeginMissionSequence") && !raw.contains("Keep"),
                 "Save consumer export includes unselected mission/support state");
         require(firstExport==raw, "Save consumer pre-propagation export changed across probe durations");
         const bool reloaded=window.loadScript(path) && window.buildScript();
         save(label+"-reload.messages.txt",messages->toPlainText().toUtf8());
         require(reloaded, "Save consumer export alone cannot rebuild");
         require(source->toPlainText().toUtf8()==raw, "Save consumer standalone reload changed raw export");
         const auto after=configuredSnapshot();json(label+"-reloaded-native.json",after);
         QJsonArray differences,gaps;
         int properties=0;
         for (auto it=before.begin();it!=before.end();++it)
            compare(it.value().toObject(),after[it.key()].toObject(),it.key(),differences,gaps,properties);
         save(label+"-native-differences.json",QJsonDocument(differences).toJson());
         save(label+"-native-getter-gaps.json",QJsonDocument(gaps).toJson());
         require(properties>0 && differences.isEmpty() && gaps.isEmpty(),
                 "Save consumer changed writable native fields/metadata/owned identity");
         checked+=properties;
         const auto rebuilt=QString::fromUtf8(raw)+"\n"+keep+"BeginMissionSequence;\n"+continuation;
         source->setPlainText(rebuilt);save(label+"-reloaded.script",rebuilt.toUtf8());
         const bool probeBuilt=window.buildScript();
         save(label+"-probe-build.messages.txt",messages->toPlainText().toUtf8());
         require(probeBuilt, "Save consumer rebuilt probe build failed");
         const int reloadedUndo=source->document()->availableUndoSteps();
         acceptanceView(window);
         const auto reloadedStatus=window.runMission();events();
         save(label+"-reloaded-run.messages.txt",messages->toPlainText().toUtf8());
         require(reloadedStatus==MainWindow::RunResult::Completed,
                 "Save consumer rebuilt five-force/STM mission failed");
         require(source->toPlainText()==rebuilt && source->document()->availableUndoSteps()==reloadedUndo,
                 "Save consumer rebuilt Run changed source/history");
         require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,
                 "Save consumer rebuilt following statement failed");
         const auto b=consumer(seconds,catalog["elapsed_bound_seconds"].toDouble());json(label+"-reloaded-consumer.json",b);
         for (const auto &field:{"epoch","state","STM","packed_state","derivative","integrator_seconds","mass","DragArea","SRPArea"})
            require(same(a[field],b[field]), "Save consumer original/rebuilt strict numerical comparison failed");
         const auto originalForces=a["force_derivatives"].toObject(),reloadedForces=b["force_derivatives"].toObject();
         require(originalForces.keys()==reloadedForces.keys(), "Save consumer force identities changed");
         for (const auto &identity:originalForces.keys())
            require(same(originalForces[identity],reloadedForces[identity]),
                    "Save consumer original/rebuilt strict individual-force comparison failed");
         observations.append(QJsonObject{{"seconds",seconds},{"writable_properties_checked",properties},
            {"original_order",a["force_order"]},{"reloaded_order",b["force_order"]},
            {"state_STM_derivative_and_force_components_equal_with_original_bound",true}});
         save("checks.json",QJsonDocument(observations).toJson());
      }
      source->document()->setModified(false);
      const QJsonObject proof{{"checkpoint","save-force-consumer"},{"outcome","passed"},
         {"case",spec["key"]},{"checks",observations},{"writable_native_properties_checked",checked},
         {"relative_bound",1e-12},{"missions",int(1+2*observations.size())},
         {"export_missions",1},{"consumer_missions",int(2*observations.size())},
         {"facets",QJsonArray{"actual-GUI-Build-Run-and-Save-before-propagation",
            "standalone-export-rebuild-and-complete-writable-native-property-comparison",
            "active-five-force-original-and-rebuilt-Cartesian-STM-and-derivative-consumers",
            "unchanged-relative-comparison-and-retained-clock-fixture-bound",
            "exact-source-history-and-following-statement-preservation"}},
         {"scope","Only the retained CombinedNRL force parameters and three native state/STM/derivative consumers. Original and rebuilt engineering comparison, not independent physical truth or universal force-order equivalence. Complete FRC!E132 remains open."}};
      json("proof.json",proof);
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      return 0;
   } catch (BaseException &error) {std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}
     catch (std::exception &error) {std::cerr<<"FAIL "<<error.what()<<std::endl;}
   return 1;
}
