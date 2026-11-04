#include "FormationAcceptance.hpp"
#include "MatlabAcceptanceStartup.hpp"
#include <QRegularExpression>
#include <QSet>
using namespace formaccept;

int main(int argc, char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication app(argc, argv);
   app.setOrganizationName("GMATTests");
   app.setApplicationName("NasaFminconCallbackStartup");
   try
   {
      require(argc == 3, "Supply startup and actual MATLAB plugin");
      QTemporaryDir runtime;
      require(runtime.isValid(), "Fmincon startup isolation failed");
      const auto startup = isolatedStartup(QFileInfo(argv[1]).absoluteFilePath(), runtime.path(),
         QFileInfo(argv[2]).absoluteFilePath());
      const auto optimizer = qEnvironmentVariable("GMAT_QT_FMINCON_PLUGIN");
      if (!optimizer.isEmpty()) {
         require(QFileInfo(optimizer).isFile(), "Configured fmincon plugin is missing");
         write(startup,read(startup)+"\nPLUGIN = "+optimizer.toUtf8()+"\n");
      }
      Rig rig(startup);
      rig.save("startup.txt", read(startup));
      const auto types = rig.window.availableEngineTypes();
      const bool available = types.contains("FminconOptimizer");
      QString error;
      bool started = false;
      try { started = Moderator::Instance()->StartMatlabServer(); }
      catch (BaseException &e) { error = QString::fromStdString(e.GetFullMessage()); }
      const QString source = "% Actual fmincon factory/startup witness\n"
         "Create FminconOptimizer Opt;\nCreate Variable Keep;\nKeep = 41;\nBeginMissionSequence;\n";
      rig.source->setPlainText(source);
      const auto history = rig.source->document()->availableUndoSteps();
      const bool built = rig.window.buildScript();
      events();
      rig.save("factory-build-messages.txt", rig.messages->toPlainText().toUtf8());
      rig.save("factory.script", source.toUtf8());
      const QJsonObject observed{{"checkpoint", "fmincon-callback-startup"},
         {"factory_available", available}, {"server_started", started}, {"server_error", error},
         {"factory_build_succeeded", built}, {"native_types", QJsonArray::fromStringList(types)},
         {"source_unchanged", rig.source->toPlainText() == source},
         {"history_unchanged", rig.source->document()->availableUndoSteps() == history},
         {"new_requirement_credit", 0}};
      rig.save("startup-observed.json", QJsonDocument(observed).toJson());
      std::cout << "NASA_CHECK " << QJsonDocument(observed).toJson(QJsonDocument::Compact).constData() << std::endl;
      require(rig.source->toPlainText() == source && rig.source->document()->availableUndoSteps() == history,
         "Fmincon startup/build changed caller source or history");
      require(started && error.isEmpty(), "Actual Qt interpreter cannot start the fmincon callback server");
      require(available && built, "Actual fmincon optimizer factory/build unavailable");
      return 0;
   }
   catch (BaseException &e) { std::cerr << e.GetFullMessage() << '\n'; return 1; }
   catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
