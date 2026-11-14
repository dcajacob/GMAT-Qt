#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <iostream>
#include <stdexcept>

int main(int argc,char **argv)
{
   QApplication app(argc,argv); if (argc!=2) return 2;
   try {
      TestSettings settings; const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; if (!window.initialize(startup)) throw std::runtime_error("Initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      editor->setPlainText("Create Spacecraft S;\nCreate ForceModel F;\nF.PrimaryBodies = {Earth};\nF.Drag = None;\nF.SRP = Off;\nBeginMissionSequence;\n");
      if (!window.buildScript()) throw std::runtime_error("Fixture failed");
      const auto error=window.applyResourceChanges("F",{{"PointMasses","Luna, Sun"},{"SRP","On"},{"Drag.AtmosphereModel","JacchiaRoberts"}},editor->toPlainText());
      if (!error.isEmpty()) throw std::runtime_error(error.toStdString());
      auto *force=Moderator::Instance()->GetConfiguredObject("F"); QStringList types;
      for (int i=0;i<force->GetOwnedObjectCount();++i) types.append(QString::fromStdString(force->GetOwnedObject(i)->GetTypeName()));
      std::cout<<types.join(", ").toStdString()<<'\n';
      if (!(types.indexOf("PointMassForce")<types.indexOf("SolarRadiationPressure") &&
            types.indexOf("SolarRadiationPressure")<types.indexOf("DragForce")))
         throw std::runtime_error("GUI force edits changed the numerical contributor order");
      return 0;
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; }
   return 1;
}
