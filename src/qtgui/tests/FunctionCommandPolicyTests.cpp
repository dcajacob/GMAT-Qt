#include "FormationAcceptance.hpp"
#include "Function.hpp"
#include "GmatInterface.hpp"
#include <functional>
#include <thread>

using namespace formaccept;

namespace {
QString quoted(QString text) { text.replace("'", "''"); return "'" + text + "'"; }
double value(const char *name) {
   auto *object = Moderator::Instance()->GetInternalObject(name);
   require(object, "Policy runtime variable missing");
   return object->GetRealParameter("Value");
}
void build(Rig &rig, const QString &script, const QString &tag) {
   rig.source->setPlainText(script);
   rig.messages->clear();
   rig.save(tag + ".script", script.toUtf8());
   const bool accepted = rig.window.buildScript();
   rig.save(tag + ".build.txt", rig.messages->toPlainText().toUtf8());
   require(accepted, "Policy fixture did not Build");
}
MainWindow::RunResult run(Rig &rig, const QString &tag) {
   const auto source = rig.source->toPlainText();
   const auto undo = rig.source->document()->availableUndoSteps();
   const auto result = rig.window.runMission();
   events(); // Deliver the native receiver's queued error messages before observing the UI.
   rig.save(tag + ".run.txt", rig.messages->toPlainText().toUtf8());
   require(rig.source->toPlainText() == source &&
         rig.source->document()->availableUndoSteps() == undo &&
         !rig.window.isRunning() && !rig.source->isReadOnly(),
         "Policy Run changed source/history or left the editor locked");
   return result;
}
QJsonArray saveMission(Rig &rig) {
   QJsonArray observations;
   for (const auto &policy : QStringList{"LegacyTerminal", "Immediate"}) {
      for (const auto &context : QStringList{"Terminal", "BeforeReport", "Function", "FunctionLoop", "NamedTerminal", "DotDirectory"}) {
         const auto tag = policy + "-" + context;
         const bool terminal = context == "Terminal" || context == "NamedTerminal" || context == "DotDirectory";
         const auto directory = rig.files.filePath((context == "DotDirectory" ? "dot.parent/" : "") + tag);
         require(QDir().mkpath(directory), "Policy directory missing");
         const auto output = directory + (context == "NamedTerminal" ? "/Host Δ" : "/Host's Δ");
         const auto saved = output + ".script";
         const auto command = QString("SaveMission ") +
               (context == "NamedTerminal" ? "'Export caption' " : "") + quoted(output) +
               " {ExecutionPolicy = " + policy + "};\n";
         const auto function = directory + "/Exercise.gmf";
         QString body = "function [out] = Exercise(input)\nCreate Variable out PrivateMarker step;\n"
               "PrivateMarker = 991;\nBeginMissionSequence;\nout = input*input+3;\n";
         if (context == "FunctionLoop") body += "If input > 0;\nFor step = 1:2;\n";
         if (context.startsWith("Function")) body += command;
         if (context == "FunctionLoop") body += "EndFor;\nEndIf;\n";
         body += "out = out+1;\n";
         write(function, body.toUtf8());
         rig.save(tag + ".gmf", body.toUtf8());
         const auto report = directory + "/values.txt";
         const QString prefix = "Create Variable Result Keep HostMarker;\nKeep = 41;\nHostMarker = 17;\n"
               "Create GmatFunction Exercise;\nExercise.FunctionPath = " + quoted(function) + ";\n"
               "Create ReportFile Values;\nValues.Filename = " + quoted(report) + ";\n"
               "Values.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Precision = 17;\n"
               "BeginMissionSequence;\n[Result] = Exercise(2);\nKeep = Keep+1;\n";
         const auto reporting = "Report Values Result Keep HostMarker;\n";
         const auto script = prefix + (context == "BeforeReport" ? command : QString()) + reporting +
               (terminal ? command : QString());
         build(rig, script, tag);
         require(run(rig, tag) == MainWindow::RunResult::Completed, "Policy mission did not complete");
         require(value("Result") == 8 && value("Keep") == 42 && value("HostMarker") == 17,
               "Policy function or caller continuation differs");
         const bool expected = policy == "Immediate" || terminal;
         QJsonArray commands;
         for (auto *native = Moderator::Instance()->GetFirstCommand(); native; native = native->GetNext()) {
            if (native->GetTypeName() != "SaveMission") continue;
            auto *next = native->GetNextWhileExecuting();
            auto *owner = native->GetCurrentFunction();
            commands.append(QJsonObject{{"filename", QString::fromStdString(native->GetStringParameter("Filename"))},
                  {"policy", QString::fromStdString(native->GetStringParameter("ExecutionPolicy"))},
                  {"next_type", next ? QString::fromStdString(next->GetTypeName()) : QString()},
                  {"function", owner ? QString::fromStdString(owner->GetName()) : QString()}});
         }
         const auto files = QDir(directory).entryList(QDir::Files);
         rig.save(tag + ".output-diagnostic.json", QJsonDocument(QJsonObject{{"expected_path", saved},
               {"expected_exists", expected}, {"actual_exists", QFileInfo::exists(saved)},
               {"directory_files", QJsonArray::fromStringList(files)}, {"main_save_commands", commands}}).toJson());
         require(QFileInfo::exists(saved) == expected, "SaveMission policy effect differs");
         if (expected) {
            const auto data = read(saved);
            require(data.contains("HostMarker") && data.contains("BeginMissionSequence") &&
                  data.contains("Exercise(2)") && !data.contains("PrivateMarker"),
                  "SaveMission did not export the configured host mission");
            rig.save(tag + ".export.script", data);
            require(rig.window.loadScript(saved) && rig.window.buildScript(), "Exported host did not reopen/Build");
            require(run(rig, tag + "-replay") == MainWindow::RunResult::Completed &&
                  value("Result") == 8 && value("Keep") == 42 && value("HostMarker") == 17,
                  "Exported host replay differs");
         }
         observations.append(QJsonObject{{"policy", policy}, {"context", context},
               {"exported", expected}, {"independent_result", 8}, {"caller_keep", 42},
               {"unicode_path", true}, {"apostrophe_path", context != "NamedTerminal"},
               {"dotted_parent", context == "DotDirectory"}, {"native_label_control", context == "NamedTerminal"},
               {"export_replayed", expected}});
      }
   }
   const auto directory = rig.files.filePath("CannotOverwrite.script");
   require(QDir().mkpath(directory), "Write-failure directory missing");
   build(rig, "Create Variable Keep;\nKeep = 41;\nBeginMissionSequence;\nSaveMission " +
         quoted(directory) + " {ExecutionPolicy = Immediate};\nKeep = Keep+1;\n", "write-failure");
   const auto failedRun = run(rig, "write-failure");
   rig.save("write-failure.diagnostic.json", QJsonDocument(QJsonObject{{"run_result", int(failedRun)},
         {"keep", value("Keep")}, {"directory_intact", QFileInfo(directory).isDir()},
         {"diagnostic_delivered", rig.messages->toPlainText().contains("SaveMission could not open output file")}}).toJson());
   require(failedRun == MainWindow::RunResult::Failed && value("Keep") == 41 &&
         rig.messages->toPlainText().contains("SaveMission could not open output file") &&
         QFileInfo(directory).isDir(), "SaveMission write failure was lost or continued execution");
   build(rig, "Create Variable Keep;\nKeep = 41;\nBeginMissionSequence;\nKeep = Keep+1;\n", "recovery");
   require(run(rig, "recovery") == MainWindow::RunResult::Completed && value("Keep") == 42,
         "SaveMission failure poisoned the next mission");
   observations.append(QJsonObject{{"write_failure_stops_command", true}, {"next_build_run_recovers", true}});
   return observations;
}

class CallbackProbe : public GmatBase {
public:
   int executions = 0, writes = 0, iterations = 0;
   CallbackProbe() : GmatBase(Gmat::UNKNOWN_OBJECT, "PolicyCallbackProbe", "Probe") {}
   GmatBase *Clone() const override { return new CallbackProbe(*this); }
   bool RenameRefObject(const UnsignedInt, const std::string &, const std::string &) override { return false; }
   bool HasLocalClones() override { return false; }
   bool ExecuteCallback() override { ++executions; return true; }
   bool PutCallbackData(std::string &) override { ++writes; return true; }
   bool PutCallbackRealData(RealArray &) override { ++writes; return true; }
   bool TakeAction(const std::string &action, const std::string &) override {
      if (action == "MatlabOptimizerIteration") { ++iterations; return true; }
      return false;
   }
};
QJsonArray matlabGuard() {
   auto *interface = GmatInterface::Instance();
   CallbackProbe probe;
   require(interface->RegisterCallbackServer(&probe), "Probe callback registration failed");
   struct Registration {
      GmatInterface *interface; GmatBase *probe;
      ~Registration() { interface->UnregisterCallbackServer(probe); }
   } registration{interface, &probe};
   std::string data = "2"; RealArray numbers{2};
   interface->PutCallbackData(data); interface->PutCallbackRealData(numbers);
   require(interface->ExecuteCallback() && probe.executions == 1 && probe.writes == 2,
         "Normal native callback did not execute");
   QJsonArray observations;
   auto blocked = [&](const char *name, const std::function<void()> &operation) {
      bool rejected = false;
      try { operation(); }
      catch (BaseException &error) {
         rejected = error.GetFullMessage().find("GMAT-called MATLAB function cannot") != std::string::npos;
      }
      require(rejected, "Active MATLAB function allowed native callback/script control");
      observations.append(QJsonObject{{"blocked_operation", name}});
   };
   {
      GmatInterface::MatlabFunctionScope outer;
      try { GmatInterface::MatlabFunctionScope inner; throw std::runtime_error("unwind nested MATLAB evaluation"); }
      catch (const std::runtime_error &) {}
      blocked("execute", [&] { interface->ExecuteCallback(); });
      blocked("text-input", [&] { interface->PutCallbackData(data); });
      blocked("real-input", [&] { interface->PutCallbackRealData(numbers); });
      blocked("iteration", [&] { interface->PublishCallbackIteration("iter 1 1 0"); });
      blocked("open-script", [&] { interface->OpenScript(); });
      blocked("run-script", [&] { interface->RunScript(); });
      blocked("build-script", [&] { interface->BuildObject(); });
      blocked("update-script", [&] { interface->UpdateObject(); });
      blocked("clear-script", [&] { interface->ClearScript(); });
      char script[] = "Create Variable Intruder;";
      blocked("put-script", [&] { interface->PutScript(script); });
      bool crossThreadRejected = false;
      std::thread other([&] {
         try { interface->ExecuteCallback(); }
         catch (BaseException &error) {
            crossThreadRejected = error.GetFullMessage().find("GMAT-called MATLAB function cannot") != std::string::npos;
         }
      });
      other.join();
      require(crossThreadRejected && !GmatInterface::CanAcceptMatlabCommand("Callback;") &&
            !GmatInterface::CanAcceptMatlabCommand("Run;") &&
            GmatInterface::CanAcceptMatlabCommand("Close;"), "MATLAB cross-thread or receipt-time guard differs");
      require(probe.executions == 1 && probe.writes == 2 && probe.iterations == 0,
            "Blocked MATLAB operation invoked the callback service");
   }
   require(GmatInterface::CanAcceptMatlabCommand("Callback;") && interface->ExecuteCallback() &&
         probe.executions == 2, "MATLAB evaluation scope did not restore native callbacks");
   observations.append(QJsonObject{{"nested_exception_preserves_outer_guard", true},
         {"cross_thread_rejected", true}, {"receipt_guard_rejected", true},
         {"native_callback_resumes", true}, {"scope", "Native guard with a callback probe; no licensed MATLAB or socket end-to-end qualification"}});
   return observations;
}
}

int main(int argc, char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication app(argc, argv); app.setOrganizationName("GMATTests"); app.setApplicationName("FunctionCommandPolicies");
   try {
      require(argc == 3, "Supply startup and policy case");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const QString mode = argv[2]; QJsonArray observations;
      if (mode == "SaveMission") observations = saveMission(rig);
      else if (mode == "MatlabGuard") observations = matlabGuard();
      else require(false, "Unknown policy case");
      const QJsonObject proof{{"checkpoint", "function-command-policy-" + mode}, {"outcome", "passed"},
            {"observations", observations}, {"new_requirement_credit", 0}, {"whole_row_qualified", false}};
      rig.save("proof.json", QJsonDocument(proof).toJson());
      std::cout << "NASA_CHECK " << QJsonDocument(proof).toJson(QJsonDocument::Compact).constData() << std::endl;
      return 0;
   } catch (BaseException &error) { std::cerr << "FAIL " << error.GetFullMessage() << '\n'; }
   catch (const std::exception &error) { std::cerr << "FAIL " << error.what() << '\n'; }
   return 1;
}
