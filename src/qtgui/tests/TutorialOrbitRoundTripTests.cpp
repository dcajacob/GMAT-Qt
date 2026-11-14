#include "MainWindow.hpp"
#include "SpacecraftOrbit.hpp"
#include "Moderator.hpp"
#include "Spacecraft.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   if (argc!=2) return 2;
   try {
      TestSettings isolated; QTemporaryDir files;
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Initialization failed");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QList<QMap<QString,QString>> cases={
         {{"SMA","7600"},{"ECC","0.0005"},{"INC","100.505"},{"RAAN","278.845"},{"AOP","0"},{"TA","89.95"}},
         {{"SMA","-32593.21599272796"},{"ECC","1.202872548116185"},{"INC","28.80241266404142"},
          {"RAAN","173.9693759331483"},{"AOP","240.9696529532764"},{"TA","359.9465533778069"}}};
      for (auto values:cases) {
         editor->setPlainText("Create Spacecraft Sat;\nBeginMissionSequence;\n");
         require(window.buildScript(),"Fixture build failed");
         values.insert("DisplayStateType","Keplerian");
         std::unique_ptr<GmatBase> preview(Moderator::Instance()->GetConfiguredObject("Sat")->Clone());
         applySpacecraftOrbitProperties(*preview,values);
         const Rvector6 expected=static_cast<Spacecraft *>(preview.get())->GetState().GetState();
         require(window.applyResourceChanges("Sat",values,editor->toPlainText()).isEmpty(),"GUI orbit transaction failed");
         const auto check=[&] {
            const auto *sat=static_cast<Spacecraft *>(Moderator::Instance()->GetConfiguredObject("Sat"));
            const auto actual=const_cast<Spacecraft *>(sat)->GetState().GetState();
            for (int i=0;i<6;++i) if (actual[i]!=expected[i]) {
               std::cerr<<"component "<<i<<" changed by "<<actual[i]-expected[i]<<'\n';
               throw std::runtime_error("GUI serialization changed the physical orbit");
            }
            require(sat->GetStringParameter("DisplayStateType")=="Keplerian","Display representation changed");
         };
         check();
         const auto path=files.filePath("orbit.script");
         require(window.saveScriptTo(path) && window.loadScript(path) && window.buildScript(),"Save/reopen failed"); check();
      }
      std::cout<<"Near-circular and hyperbolic GUI orbit edits preserve Cartesian state through Apply and Save/reopen.\n";
      return 0;
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; }
   return 1;
}
