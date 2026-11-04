#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include <QApplication>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   try {
      require(argc==2,"Expected startup path"); TestSettings settings; QTemporaryDir files;
      MainWindow window; window.show(); require(window.initialize(QString::fromLocal8Bit(argv[1])),"Initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString source="% Preserve R N and Craft in this comment Δ\nCreate Spacecraft Craft;\nCreate CoordinateSystem Rot;\nRot.Origin = Earth;\nRot.Axes = ObjectReferenced;\nRot.XAxis = R;\nRot.YAxis = '';\nRot.ZAxis = N;\nRot.Primary = Earth;\nRot.Secondary = Craft;\nCreate CoordinateSystem Fixed;\nFixed.Origin = Luna;\nFixed.Axes = BodyFixed;\nCreate OrbitView Orb;\nOrb.Add = {Earth, Craft};\nOrb.CoordinateSystem = Rot;\nBeginMissionSequence;\nStop;\n";
      editor->setPlainText(source); const bool built=window.buildScript(); app.processEvents(); require(built,qPrintable("Forwarded-axis fixture failed to build:\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()));
      acceptanceView(window);
      const auto rename=[&](const QString &old,const QString &name,const QString &expected) {
         const auto before=editor->toPlainText(); const auto error=window.renameResource(old,name,before);
         require(error.isEmpty(),qPrintable(error)); require(editor->toPlainText()==expected,"Rename changed literal axes or unrelated source");
         editor->undo(); require(editor->toPlainText()==before && window.buildScript(),"Forwarded-axis Rename Undo failed");
         editor->redo(); require(editor->toPlainText()==expected && window.buildScript(),"Forwarded-axis Rename Redo failed");
      };
      auto expected=source; expected.replace("Create OrbitView Orb;","Create OrbitView EarthView;").replace("Orb.","EarthView."); rename("Orb","EarthView",expected);
      expected.replace("Create Spacecraft Craft;","Create Spacecraft Vehicle;").replace("Rot.Secondary = Craft;","Rot.Secondary = Vehicle;").replace("{Earth, Craft}","{Earth, Vehicle}"); rename("Craft","Vehicle",expected);
      auto *axes=Moderator::Instance()->GetConfiguredObject("Rot")->GetOwnedObject(0);
      require(axes && axes->GetStringParameter("Secondary")=="Vehicle","Forwarded AxisSystem reference stranded");
      expected.prepend("Create Variable R N;\nR = 1;\nN = 2;\n"); expected.replace("Rot.XAxis = R;","Rot.XAxis = 'R';").replace("Rot.ZAxis = N;","Rot.ZAxis = 'N';");
      editor->setPlainText(expected); require(window.buildScript(),"Quoted axis collision fixture rejected");
      expected.replace("Create Variable R N;","Create Variable Radial N;").replace("\nR = 1;","\nRadial = 1;"); rename("R","Radial",expected);
      expected.replace("Create Variable Radial N;","Create Variable Radial Normal;").replace("\nN = 2;","\nNormal = 2;"); rename("N","Normal",expected);
      expected.replace("Create CoordinateSystem Rot;","Create CoordinateSystem LocalRot;").replace("Rot.","LocalRot.").replace("= Rot;","= LocalRot;"); rename("Rot","LocalRot",expected);
      const auto saved=files.filePath("forwarded axes α.script");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && editor->toPlainText()==expected && window.buildScript(),"Forwarded-axis Unicode Save/reopen failed");
      axes=Moderator::Instance()->GetConfiguredObject("LocalRot")->GetOwnedObject(0);
      require(axes && axes->GetStringParameter("XAxis")=="R" && axes->GetStringParameter("ZAxis")=="N" && axes->GetStringParameter("Secondary")=="Vehicle","Axes literals or references changed on reopen");
      std::cout<<"PASS unrelated viewer Rename with forwarded coordinate axes, spacecraft/frame references, literal R/N collisions, exact Undo/Redo and Unicode Save/reopen. No numerical or host qualification.\n";
   } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
