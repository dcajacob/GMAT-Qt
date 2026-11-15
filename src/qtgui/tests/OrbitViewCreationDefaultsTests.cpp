#include "MainWindow.hpp"
#include "ResourceDraft.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTimer>
#include <array>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message)
{
   if (!value) throw std::runtime_error(message);
}

static GmatBase &plot(const char *name)
{
   auto *object=Moderator::Instance()->GetConfiguredObject(name);
   require(object,"OrbitView was not created");
   return *object;
}

static void camera(GmatBase &object,const std::array<double,3> &eye,
                   const char *frame,const char *axis)
{
   auto value=QString::fromStdString(object.GetStringParameter("ViewPointVector"));
   value.remove('['); value.remove(']');
   const auto components=value.split(QRegularExpression("[\\s,]+"),Qt::SkipEmptyParts);
   require(components.size()==3,"Camera vector has incorrect dimensions");
   for (int i=0;i<3;++i)
      if (components[i].toDouble()!=eye[i])
         throw std::runtime_error(object.GetName()+": unexpected camera position "+value.toStdString());
   require(object.GetStringParameter("ViewUpCoordinateSystem")==frame,"Camera up frame differs");
   require(object.GetStringParameter("ViewUpAxis")==axis,"Camera up axis differs");
}

int main(int argc,char **argv)
{
   QApplication app(argc,argv);
   QApplication::setOrganizationName("GMATTests");
   QApplication::setApplicationName("OrbitViewCreationDefaults");
   try {
      require(argc==2,"Supply a startup file");
      TestSettings settings;
      QTemporaryDir files;
      require(files.isValid(),"Temporary directory unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window;
      require(window.initialize(startup),"Runtime initialization failed");
      window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      require(editor,"Script editor missing");
      const QString baseline=
         "% Existing plot cameras retain their saved semantics\n"
         "Create Spacecraft Sat;\n"
         "Create OrbitView Legacy Explicit;\n"
         "Explicit.ViewPointVector = [1 2 3];\n"
         "Explicit.ViewUpCoordinateSystem = EarthMJ2000Eq;\n"
         "Explicit.ViewUpAxis = -X;\n"
         "BeginMissionSequence;\n";
      editor->setPlainText(baseline);
      require(window.buildScript(),"Baseline build failed");
      auto existingCameras=[] {
         camera(plot("Legacy"),{0,0,30000},"EarthMJ2000Eq","Z");
         camera(plot("Explicit"),{1,2,3},"EarthMJ2000Eq","-X");
      };
      existingCameras();
      {
         auto preview=resourceDraft("OrbitView","Preview");
         camera(*preview,{12000,-16000,10000},"EarthFixed","Z");
         require(!Moderator::Instance()->GetConfiguredObject("Preview"),"Preview registered an object");
      }

      auto create=[&](bool accept) {
         std::exception_ptr failure;
         QTimer::singleShot(0,&window,[&] {
            auto *dialog=window.findChild<QDialog *>("newResourceDialog");
            try {
               require(dialog && dialog->isVisible(),"New OrbitView dialog missing");
               dialog->findChild<QComboBox *>("resourceType")->setCurrentText("OrbitView");
               dialog->findChild<QLineEdit *>("resourceName")->setText("Fresh");
               const auto buttons=dialog->findChildren<QDialogButtonBox *>(QString(),Qt::FindDirectChildrenOnly);
               require(buttons.size()==1,"Creation buttons missing");
               buttons.front()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
               require(!dialog->isVisible(),"Creation dialog did not complete");
            } catch (...) {
               failure=std::current_exception();
               if (dialog) dialog->reject();
            }
         });
         window.findChild<QAction *>("createResource")->trigger();
         if (failure) std::rethrow_exception(failure);
      };
      create(false);
      require(editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject("Fresh"),
              "Cancel changed the mission");
      create(true);
      camera(plot("Fresh"),{12000,-16000,10000},"EarthFixed","Z");
      existingCameras();
      const auto created=editor->toPlainText();
      require(created.endsWith(baseline),"Creation changed existing source");
      require(created.contains("Fresh.ViewPointVector = [12000 -16000 10000]") &&
              created.contains("Fresh.ViewUpCoordinateSystem = EarthFixed") &&
              created.contains("Fresh.ViewUpAxis = Z"),"Camera defaults were not persisted");
      auto *workspace=window.findChild<QMdiArea *>();
      require(workspace,"Workspace missing");
      for (auto *child:workspace->subWindowList())
         if (child->property("resourceName").toString()=="Fresh") child->close();
      QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      editor->undo();
      require(editor->toPlainText()==baseline && window.buildScript(),"Creation Undo failed");
      existingCameras();
      editor->redo();
      require(editor->toPlainText()==created && window.buildScript(),"Creation Redo failed");
      camera(plot("Fresh"),{12000,-16000,10000},"EarthFixed","Z");

      const QMap<QString,QString> custom={{"ViewPointVector","[6000 -9000 4000]"},
         {"ViewUpCoordinateSystem","EarthMJ2000Eq"},{"ViewUpAxis","Y"}};
      require(window.createResource("OrbitView","Custom",created,1,1,std::nullopt,{},{},custom).isEmpty(),
              "Explicit creation camera rejected");
      camera(plot("Custom"),{6000,-9000,4000},"EarthMJ2000Eq","Y");
      require(window.createResource("OrbitView","Direct",editor->toPlainText()).isEmpty(),
              "Default creation without settings rejected");
      camera(plot("Direct"),{12000,-16000,10000},"EarthFixed","Z");
      const auto saved=files.filePath("north-up.script");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Save/reopen failed");
      camera(plot("Fresh"),{12000,-16000,10000},"EarthFixed","Z");
      camera(plot("Custom"),{6000,-9000,4000},"EarthMJ2000Eq","Y");
      camera(plot("Direct"),{12000,-16000,10000},"EarthFixed","Z");
      existingCameras();
      std::cout<<"PASS: oblique north-up preview and New OrbitView, Cancel, persisted defaults, Undo/Redo, explicit overrides, and Save/reopen\n";
   } catch (BaseException &error) {
      std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1;
   } catch (std::exception &error) {
      std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1;
   }
   return 0;
}
