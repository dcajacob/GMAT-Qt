#include "FormationAcceptance.hpp"
#include "ReportViewer.hpp"
#include "ObjectManagedFunction.hpp"
#include <QRegularExpression>
using namespace formaccept;

static QString pointer(const void *p) { return QString::number(quintptr(p), 16); }

int main(int argc, char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication app(argc, argv);
   app.setOrganizationName("GMATTests");
   app.setApplicationName("FunctionGlobalArray");
   try
   {
      require(argc == 2 && acceptanceWidgetPlatform(), "Supply global Array startup");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const auto folder = rig.files.filePath("Global Array Ω");
      require(QDir().mkpath(folder), "Global Array directory absent");
      const auto scalar = folder + "/Scalar.gmf", whole = folder + "/Whole.gmf";
      const auto outer = folder + "/Outer.gmf", saved = folder + "/Caller.script";
      const auto report = folder + "/Values.txt", trace = folder + "/Trace.txt";
      const QString scalarBody = "function [out] = Scalar()\nCreate Variable out;\nBeginMissionSequence;\nout = 13;\n";
      const QString wholeBody = "function [Shared, Mark] = Whole()\nCreate Variable Mark Keep;\nKeep = 59;\nGlobal Shared;\nBeginMissionSequence;\nShared(2,1) = Shared(2,1) + 7;\nMark = Keep;\n";
      const QString outerBody = "function [out] = Outer()\nCreate Variable out Keep;\nCreate Array Local[2,2];\nCreate GmatFunction Scalar Whole;\nScalar.FunctionPath = '" + scalar + "';\nWhole.FunctionPath = '" + whole + "';\nGlobal Shared Trace;\nBeginMissionSequence;\nKeep = 41;\nLocal(1,1) = 5;\nLocal(1,2) = 6;\nLocal(2,1) = 7;\nLocal(2,2) = 8;\n[Shared(1,2)] = Scalar();\n[Shared, out] = Whole();\nout = out + Keep;\nReport Trace Keep Local(1,1) Local(1,2) Local(2,1) Local(2,2);\n";
      write(scalar, scalarBody.toUtf8()); write(whole, wholeBody.toUtf8()); write(outer, outerBody.toUtf8());
      rig.save("scalar.gmf", scalarBody.toUtf8()); rig.save("whole.gmf", wholeBody.toUtf8()); rig.save("outer.gmf", outerBody.toUtf8());
      const QString caller = "Create ChemicalTank Fuel;\nFuel.FuelMass = 23;\nCreate Spacecraft Host;\nHost.Tanks = {Fuel};\nCreate Array Shared[2,2] Returned[2,2] Decoy[2,2];\nShared(1,1) = 11;\nShared(1,2) = -2;\nShared(2,1) = 3;\nShared(2,2) = 17;\nDecoy(1,1) = 31;\nDecoy(1,2) = 32;\nDecoy(2,1) = 33;\nDecoy(2,2) = 34;\nReturned(1,1) = 99;\nCreate Variable First Second Keep;\nKeep = 103;\nCreate GmatFunction Outer Whole;\nOuter.FunctionPath = '" + outer + "';\nWhole.FunctionPath = '" + whole + "';\nCreate ReportFile Values Trace;\nValues.Filename = '" + report + "';\nTrace.Filename = '" + trace + "';\nValues.WriteHeaders = false;\nTrace.WriteHeaders = false;\nValues.FixedWidth = false;\nTrace.FixedWidth = false;\nValues.Precision = 17;\nTrace.Precision = 17;\nBeginMissionSequence;\nGlobal Shared Trace;\nWrite Host.Fuel {Style = Verbose, MessageWindow = true, LogFile = false};\n[First] = Outer();\n[Returned, Second] = Whole();\nReport Values First Second Keep Shared(1,1) Shared(1,2) Shared(2,1) Shared(2,2) Returned(1,1) Returned(1,2) Returned(2,1) Returned(2,2) Decoy(1,1) Decoy(1,2) Decoy(2,1) Decoy(2,2);\nKeep = Keep + 1;\nReport Values Keep;\n";
      auto prototypes = [&] { QJsonObject v; for (auto name : {"Shared", "Returned", "Decoy"}) v[name] = QString::fromStdString(Moderator::Instance()->GetConfiguredObject(name)->GetGeneratingString(Gmat::NO_COMMENTS)); return v; };
      rig.source->setPlainText(caller); rig.save("caller.script", caller.toUtf8());
      const bool built = rig.window.buildScript(); rig.save("build-messages.txt", rig.messages->toPlainText().toUtf8());
      require(built, "Global Array caller Build failed");
      const auto before = prototypes(); rig.save("prototypes-before.json", QJsonDocument(before).toJson());
      require(rig.window.saveScriptTo(saved), "Global Array Unicode Save failed");
      const QJsonArray expected{100,59,103,11,13,17,17,11,13,17,17,31,32,33,34,104};
      const QJsonArray expectedTrace{41,5,6,7,8};
      QJsonArray phases;
      for (int phase = 0; phase < 3; ++phase)
      {
         if (phase == 1) require(rig.window.loadScript(saved) && rig.window.buildScript(), "Global Array Unicode reopen failed");
         const int history = rig.source->document()->availableUndoSteps();
         rig.messages->clear(); acceptanceView(rig.window);
         const auto result = rig.window.runMission(); events();
         rig.save(QString::number(phase) + "-messages.txt", rig.messages->toPlainText().toUtf8());
         if (result != MainWindow::RunResult::Completed) std::cerr << rig.messages->toPlainText().toStdString();
         require(result == MainWindow::RunResult::Completed, "Nested global Array return failed");
         auto checked = [&](const QString &path, const QString &suffix, const QJsonArray &wanted) {
            const auto raw = read(path); rig.save(QString::number(phase) + suffix, raw);
            const auto words = QString::fromUtf8(raw).split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            require(words.size() == wanted.size(), "Global Array report width differs"); QJsonArray values;
            for (int i = 0; i < words.size(); ++i) { bool ok; const double value = words[i].toDouble(&ok); require(ok && std::isfinite(value) && value == wanted[i].toDouble(), "Global Array differs from independent literal/frame oracle"); values.append(value); }
            return values;
         };
         const auto attached = QRegularExpression("FuelMass\\s*=\\s*23(?:\\.0*)?;").globalMatch(rig.messages->toPlainText()); auto attachedMatches = attached; int attachedCount = 0; while (attachedMatches.hasNext()) { attachedMatches.next(); ++attachedCount; } require(attachedCount == 1, "Existing attached-tank serialization changed");
         const auto values = checked(report, "-values.txt", expected), frame = checked(trace, "-trace.txt", expectedTrace);
         auto *shared = Moderator::Instance()->GetInternalObject("Shared");
         auto *configured = Moderator::Instance()->GetConfiguredObject("Shared");
         require(shared && shared != configured && shared->IsGlobal() && !shared->IsLocal(), "Global Array identity/flags differ");
         QJsonArray identities;
         for (auto *command = Moderator::Instance()->GetFirstCommand(); command; command = command->GetNext())
         {
            if (!command->IsOfType("CallFunction")) continue;
            auto *function = dynamic_cast<ObjectManagedFunction*>(command->GetRefObject(Gmat::FUNCTION, ""));
            require(function && function->GetGlobalObjectStore(), "Global Array function store absent");
            const auto *store = function->GetGlobalObjectStore(); const auto found = store->find("Shared");
            require(found != store->end() && found->second == shared, "Global Array function identity differs");
            identities.append(QJsonObject{{"function", QString::fromStdString(function->GetName())}, {"global_store_object_pointer", pointer(found->second)}, {"global_object_pointer", pointer(shared)}, {"configured_prototype_pointer", pointer(configured)}});
         }
         require(identities.size() == 2 && prototypes() == before && rig.source->toPlainText() == caller && rig.source->document()->availableUndoSteps() == history && !rig.window.isRunning(), "Global Array caller/prototype/history/lock changed");
         require(read(scalar) == scalarBody.toUtf8() && read(whole) == wholeBody.toUtf8() && read(outer) == outerBody.toUtf8(), "Global Array function files changed");
         phases.append(QJsonObject{{"phase",phase},{"explicit_rebuild",phase<2},{"values",values},{"local_frame",frame},{"identities",identities},{"attached_mass_values",attachedCount}});
         rig.save("phases.json", QJsonDocument(phases).toJson());
      }
      auto *tree = rig.window.findChild<QTreeWidget*>("Output"); const auto items = tree->findItems("Values", Qt::MatchExactly|Qt::MatchRecursive);
      require(items.size() == 1, "Global Array output absent"); tree->itemDoubleClicked(items[0],0); events();
      auto *viewer = rig.area->activeSubWindow(); require(viewer && dynamic_cast<ReportViewer*>(viewer->widget()), "Global Array report viewer absent");
      auto *text = viewer->findChild<QPlainTextEdit*>(); require(text && text->isReadOnly() && text->toPlainText().trimmed() == QString::fromUtf8(read(report)).trimmed(), "Global Array viewer differs from raw bytes"); acceptanceView(*viewer);
      const QJsonObject proof{{"checkpoint","function-global-array-return"},{"outcome","passed"},{"missions",3},{"function_calls",12},{"phases",phases},{"scope","Nested scalar output to global Array element,nested global whole-Array return,root whole-Array copy,independent literal components,local/caller/decoy/prototype isolation,Unicode reopen,no-explicit-Build repeat and actual report viewer. No universal resource scope or scientific qualification."}};
      rig.save("proof.json", QJsonDocument(proof).toJson()); std::cout << "NASA_CHECK " << QJsonDocument(proof).toJson(QJsonDocument::Compact).constData() << std::endl; return 0;
   }
   catch (BaseException &e) { std::cerr << "FAIL " << e.GetFullMessage() << std::endl; }
   catch (std::exception &e) { std::cerr << "FAIL " << e.what() << std::endl; }
   return 1;
}
