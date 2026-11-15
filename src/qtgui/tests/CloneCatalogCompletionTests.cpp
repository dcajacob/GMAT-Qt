#include "FormationAcceptance.hpp"
#include "Interface.hpp"
#include "GmatGlobal.hpp"
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QPixmap>
#include <QTest>
#include <QTimer>

using namespace formaccept;

int main(int argc, char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");
   app.setApplicationName("CloneCatalogCompletion");
   try
   {
      require(argc==3 || argc==4,"Supply startup and clone catalog type");
      const QString kind=argv[2];
      const bool nameCollision=argc==4 && QString(argv[3])=="collision";
      require(QStringList{"Propagator","VF13ad","GmatFunction","MatlabFunction"}.contains(kind),
              "Unknown clone catalog case");
      const bool function=kind=="GmatFunction" || kind=="MatlabFunction";
      if (kind=="MatlabFunction")
         GmatGlobal::Instance()->SetMatlabMode(GmatGlobal::SHARED);
      Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      rig.save("loaded-images.txt",read("/proc/self/maps"));
      const auto firstPath=rig.files.filePath(kind=="MatlabFunction" ? "SharedOne.m" : "SharedOne.gmf");
      const auto secondPath=rig.files.filePath(kind=="MatlabFunction" ? "SharedTwo.m" : "SharedTwo.gmf");
      QByteArray firstFile,secondFile;
      QString setup;
      QString mission;
      if (function)
      {
         if (kind=="GmatFunction")
         {
            firstFile="function [out] = SharedOne(input)\nCreate Variable out;\nBeginMissionSequence;\nout = input + 7;\n";
            secondFile="function [out] = SharedTwo(input)\nCreate Variable out;\nBeginMissionSequence;\nout = input * 3;\n";
         }
         else
         {
            firstFile="function out = SharedOne(input)\nout = input + 7;\nend\n";
            secondFile="function out = SharedTwo(input)\nout = input * 3;\nend\n";
         }
         write(firstPath,firstFile);
         write(secondPath,secondFile);
         rig.save(QFileInfo(firstPath).fileName(),firstFile);
         rig.save(QFileInfo(secondPath).fileName(),secondFile);
         setup="Create "+kind+" Original;\nOriginal.FunctionPath = '"+firstPath+"';\n";
         if (kind=="GmatFunction")
            setup+="Original.SourceFormat = LegacyASCII;\nOriginal.RequireOutputBrackets = true;\n";
         mission="[OutA] = Original(5);\n";
         if (nameCollision)
         {
            require(kind=="MatlabFunction","Name collision fixture requires MATLAB");
            setup+="Create Variable SharedOne;\nSharedOne = 5;\n";
            mission="[OutA] = Original(SharedOne);\n";
         }
      }
      else if (kind=="VF13ad")
      {
         setup="Create VF13ad Original;\nOriginal.MaximumIterations = 25;\n"
            "Original.UseCentralDifferences = true;\nOriginal.ShowProgress = false;\n";
      }
      else
      {
         setup="Create Spacecraft A B;\n";
         const QStringList fields{"X","Y","Z","VX","VY","VZ"};
         const auto initial=circular(Orbit{"A",7000,0,0},0);
         for (const auto &name : {QString("A"),QString("B")})
         {
            setup+=name+".CoordinateSystem = EarthMJ2000Eq;\n"+name+".DisplayStateType = Cartesian;\n"+
               name+".DateFormat = A1ModJulian;\n"+name+".Epoch = '21545';\n";
            for (int i=0;i<6;++i) setup+=name+"."+fields[i]+" = "+number(initial[i])+";\n";
         }
         setup+="Create ForceModel Gravity;\nGravity.PrimaryBodies = {};\nGravity.PointMasses = {Earth};\n"
            "Gravity.Drag = None;\nGravity.SRP = Off;\nCreate Propagator Original;\nOriginal.FM = Gravity;\n"
            "Original.Type = PrinceDormand78;\nOriginal.Accuracy = 1e-12;\nOriginal.InitialStepSize = 10;\n"
            "Original.MinStep = 0.01;\nOriginal.MaxStep = 30;\n";
         mission="Propagate Original(A) {A.ElapsedSecs = 60};\n";
      }
      const QString initial=setup+"Create Variable Keep OutA OutB;\nKeep = 41;\nOutA = -1;\nOutB = -1;\n"
         "BeginMissionSequence;\n"+mission+"Keep = Keep + 1;\n";
      rig.source->setPlainText(initial);
      require(rig.window.buildScript(),"Clone catalog baseline failed Build");
      rig.save("initial.script",initial.toUtf8());
      auto object=[](const QString &name) {
         auto *value=Moderator::Instance()->GetConfiguredObject(name.toStdString());
         require(value,"Clone catalog native object missing");
         return value;
      };
      auto properties=[&](const QString &name) {
         auto *value=object(name);
         QJsonObject result{{"type",QString::fromStdString(value->GetTypeName())}};
         if (function)
         {
            result["FunctionPath"]=QString::fromStdString(value->GetStringParameter("FunctionPath"));
            if (kind=="GmatFunction")
            {
               result["SourceFormat"]=QString::fromStdString(value->GetStringParameter("SourceFormat"));
               result["RequireOutputBrackets"]=value->GetBooleanParameter("RequireOutputBrackets");
            }
         }
         else if (kind=="VF13ad")
         {
            result["MaximumIterations"]=value->GetIntegerParameter("MaximumIterations");
            result["UseCentralDifferences"]=value->GetBooleanParameter("UseCentralDifferences");
            result["Tolerance"]=value->GetRealParameter("Tolerance");
            result["FeasibilityTolerance"]=value->GetRealParameter("FeasibilityTolerance");
         }
         else
         {
            for (const auto *field : {"Type","FM"})
               result[field]=QString::fromStdString(value->GetStringParameter(field));
            for (const auto *field : {"InitialStepSize","MinStep","MaxStep","Accuracy"})
               result[field]=value->GetRealParameter(field);
         }
         return result;
      };
      const auto original=properties("Original");
      auto clone=[&](bool commit) {
         rig.closePanels();
         const auto items=rig.resources->findItems("Original",Qt::MatchExactly|Qt::MatchRecursive);
         require(items.size()==1,"Clone catalog tree item missing");
         auto *item=items.first();
         rig.resources->expandAll();
         rig.resources->scrollToItem(item);
         rig.resources->setCurrentItem(item);
         const auto before=rig.source->toPlainText();
         const int undo=rig.source->document()->availableUndoSteps();
         bool visited=false;
         std::exception_ptr failure;
         QTimer::singleShot(0,&rig.window,[&] {
            auto *menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());
            try
            {
               require(menu,"Clone catalog menu missing");
               QAction *action=nullptr;
               const auto id=kind=="Propagator" ? "clonePropagator" : "cloneResource";
               for (auto *candidate : menu->actions()) if (candidate->objectName()==id) action=candidate;
               require(action && action->isEnabled(),"Clone catalog action unavailable");
               QTimer::singleShot(0,&rig.window,[&] {
                  auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
                  try
                  {
                     require(dialog && dialog->objectName()=="cloneResourceDialog","Clone catalog dialog missing");
                     auto *input=dialog->findChild<QLineEdit*>("resourceName");
                     auto *buttons=dialog->findChild<QDialogButtonBox*>("resourceCloneButtons");
                     require(input && buttons,"Clone catalog controls missing");
                     input->setText("Twin");
                     acceptanceView(*dialog);
                     if (commit && !rig.evidence.isEmpty())
                        require(dialog->grab().save(rig.evidence+"/clone-dialog.png"),"Clone dialog screenshot failed");
                     QTest::mouseClick(buttons->button(commit ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel),Qt::LeftButton);
                     if (dialog->result()!= (commit ? QDialog::Accepted : QDialog::Rejected))
                        std::cerr << rig.messages->toPlainText().toStdString();
                     require(dialog->result()==(commit ? QDialog::Accepted : QDialog::Rejected),"Clone catalog dialog rejected");
                     visited=true;
                  }
                  catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
               });
               menu->setActiveAction(action);
               QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);
               QApplication::sendEvent(menu,&enter);
            }
            catch (...) { failure=std::current_exception(); if (menu) menu->close(); }
         });
         rig.resources->customContextMenuRequested(rig.resources->visualItemRect(item).center());
         if (failure) std::rethrow_exception(failure);
         require(visited,"Clone catalog dialog was not exercised");
         if (!commit)
            require(rig.source->toPlainText()==before && rig.source->document()->availableUndoSteps()==undo &&
                    !Moderator::Instance()->GetConfiguredObject("Twin"),"Clone Cancel mutated source or model");
         rig.closePanels();
      };
      clone(false);
      clone(true);
      require(object("Original")!=object("Twin") && properties("Twin")==original && properties("Original")==original,
              "Clone catalog object identity or copied properties differ");
      if (kind=="Propagator")
      {
         auto *one=dynamic_cast<PropSetup*>(object("Original"));
         auto *two=dynamic_cast<PropSetup*>(object("Twin"));
         require(one && two && one->GetPropagator()!=two->GetPropagator(),"Copied propagators share owned integrator");
      }
      const auto copied=rig.source->toPlainText();
      rig.save("copied.script",copied.toUtf8());
      const QString extra=function ? "[OutB] = Twin(8);\n" : kind=="Propagator" ? "Propagate Twin(B) {B.ElapsedSecs = 60};\n" : QString();
      auto paired=copied;
      paired.replace("Keep = Keep + 1;",extra+"Keep = Keep + 1;");
      rig.source->setPlainText(paired);
      require(rig.window.buildScript(),"Copied pair failed Build");
      QJsonArray runs;
      auto run=[&](const QString &phase,bool edited) {
         const auto source=rig.source->toPlainText();
         const auto status=rig.window.runMission();
         rig.save(phase+"-messages.txt",rig.messages->toPlainText().toUtf8());
         require(status==MainWindow::RunResult::Completed,"Copied resource mission failed");
         require(Moderator::Instance()->GetInternalObject("Keep")->GetRealParameter("Value")==42 &&
                 source==rig.source->toPlainText(),"Copied mission changed its source or sentinel");
         QJsonObject values{{"phase",phase},{"keep",42}};
         if (function)
         {
            const auto first=Moderator::Instance()->GetInternalObject("OutA")->GetRealParameter("Value");
            const auto second=Moderator::Instance()->GetInternalObject("OutB")->GetRealParameter("Value");
            require(first==12 && second==(edited ? 24 : 15),"Copied function result differs from independent arithmetic");
            require(read(firstPath)==firstFile && read(secondPath)==secondFile,"Copied function changed shared file contents");
            values["original_output"]=first;
            values["clone_output"]=second;
         }
         if (kind=="Propagator")
         {
            const auto expected=circular(Orbit{"A",7000,0,0},60);
            for (const auto &name : {QString("A"),QString("B")})
            {
               auto *sat=dynamic_cast<Spacecraft*>(Moderator::Instance()->GetInternalObject(name.toStdString()));
               require(sat,"Copied propagator spacecraft missing");
               QJsonArray state;
               for (int i=0;i<6;++i)
               {
                  require(std::abs(sat->GetState()[i]-expected[i])<(i<3 ? 2e-5 : 3e-8),"Copied propagator differs from analytic orbit");
                  state.append(sat->GetState()[i]);
               }
               values[name]=state;
            }
         }
         runs.append(values);
      };
      run("copied",false);
      QMap<QString,QString> change;
      if (function) change["FunctionPath"]=secondPath;
      else if (kind=="VF13ad") change["MaximumIterations"]="31";
      else change["InitialStepSize"]="5";
      const auto error=rig.window.applyResourceChanges("Twin",change,rig.source->toPlainText());
      if (!error.isEmpty()) std::cerr << error.toStdString();
      require(error.isEmpty() && properties("Original")==original,"Independent clone edit failed or changed original");
      auto expected=original;
      if (function) expected["FunctionPath"]=secondPath;
      else if (kind=="VF13ad") expected["MaximumIterations"]=31;
      else expected["InitialStepSize"]=5;
      require(properties("Twin")==expected,"Edited clone properties differ");
      run("edited",true);
      const auto edited=rig.source->toPlainText();
      rig.source->undo();
      require(rig.source->toPlainText()==paired && rig.window.buildScript() && properties("Twin")==original,
              "Clone edit Undo failed");
      rig.source->redo();
      require(rig.source->toPlainText()==edited && rig.window.buildScript() && properties("Twin")==expected,
              "Clone edit Redo failed");
      const auto saved=rig.files.filePath("Cloned catalog Ω.script");
      require(rig.window.saveScriptTo(saved) && rig.window.loadScript(saved) && rig.window.buildScript() &&
              properties("Original")==original && properties("Twin")==expected,"Clone catalog save/reopen failed");
      run("reopened",true);
      rig.save("final.script",rig.source->toPlainText().toUtf8());
      rig.save("proof.json",QJsonDocument(QJsonObject{{"status","passed"},{"kind",kind},
         {"source_name_input_collision",nameCollision},
         {"original",original},{"edited_clone",expected},{"cancel_isolated",true},
         {"runs",runs},{"owned_integrator_distinct",kind=="Propagator"}}).toJson());
      if (kind=="MatlabFunction")
         require(Moderator::Instance()->GetMatlabInterface()->Close()==1,"Copied MATLAB function engine did not close");
      return 0;
   }
   catch (BaseException &error) { std::cerr << error.GetFullMessage() << std::endl; }
   catch (std::exception &error) { std::cerr << error.what() << std::endl; }
   return 1;
}
