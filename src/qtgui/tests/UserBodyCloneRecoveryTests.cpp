#include "FormationAcceptance.hpp"

using namespace formaccept;

int main(int argc, char **argv)
{
   QApplication app(argc, argv);
   app.setOrganizationName("GMATTests");
   app.setApplicationName("UserBodyCloneRecovery");
   try
   {
      require(argc == 3, "Supply startup and user-body type");
      const QString type = argv[2];
      require(QStringList{"Asteroid","Comet","Moon","Planet"}.contains(type), "Unknown body type");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      const QString initial = "Create " + type + " Original;\nCreate Variable Keep;\n"
         "Keep = 41;\nBeginMissionSequence;\nKeep = Keep + 1;\n";
      rig.source->setPlainText(initial);
      require(rig.window.buildScript(), "User-body clone baseline failed");
      auto *original = Moderator::Instance()->GetSolarSystemInUse()->GetBody("Original");
      require(original && original->IsUserDefined(), "User-defined body missing");
      const QString canonical = QString::fromStdString(original->GetGeneratingString(Gmat::SHOW_SCRIPT,"","Subject"));
      rig.save("original-native.script",canonical.toUtf8());
      const auto error = rig.window.cloneResource("Original","Twin",initial);
      rig.save("clone-error.txt",error.toUtf8());
      rig.save("clone-messages.txt",rig.messages->toPlainText().toUtf8());
      if (!error.isEmpty()) std::cerr << error.toStdString() << std::endl;
      require(error.isEmpty(), "Untouched user-defined body clone rejected");
      auto *copy = Moderator::Instance()->GetSolarSystemInUse()->GetBody("Twin");
      original = Moderator::Instance()->GetSolarSystemInUse()->GetBody("Original");
      require(original && copy && original != copy && copy->GetTypeName()==type.toStdString(),
              "User-defined body copy identity differs");
      require(QString::fromStdString(copy->GetGeneratingString(Gmat::SHOW_SCRIPT,"","Subject"))==canonical &&
              QString::fromStdString(original->GetGeneratingString(Gmat::SHOW_SCRIPT,"","Subject"))==canonical,
              "User-defined body copy lost native configuration");
      rig.save("cloned.script",rig.source->toPlainText().toUtf8());
      require(rig.window.runMission()==MainWindow::RunResult::Completed &&
              Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42,
              "User-defined body copy mission failed");
      rig.save("proof.json",QJsonDocument(QJsonObject{{"status","passed"},{"type",type},
         {"native_configuration_equal",true},{"sentinel",42}}).toJson());
      return 0;
   }
   catch (BaseException &error) { std::cerr << error.GetFullMessage() << std::endl; }
   catch (std::exception &error) { std::cerr << error.what() << std::endl; }
   return 1;
}
