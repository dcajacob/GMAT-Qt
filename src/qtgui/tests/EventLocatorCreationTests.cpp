#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "ResourceDraft.hpp"
#include "ResourcePreview.hpp"
#include "EventLocator.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static GmatBase &configured(const QString &name)
{
   auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString());
   require(object,"Expected configured resource is absent"); return *object;
}
static void closePanels(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList())
      if (!child->property("resourceName").toString().isEmpty()) child->close();
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication app(argc,argv);
   if (!acceptanceWidgetPlatform() || argc!=2) return 2;
   QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("EventLocatorCreation");
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Temporary fixtures unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Event creation runtime unavailable"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Source editor absent");
      const QString mission="BeginMissionSequence;\nKeep = Keep + 2; % retain mission α\n";
      const QString source="% keep this source and its configuration order\nCreate Spacecraft ZuluFirst;\n"
         "Create Spacecraft AlphaSecond;\nCreate PlanetographicRegion Area;\nCreate Variable Keep;\nKeep = 7;\n"+mission;
      editor->setPlainText(source); require(window.buildScript(),"Event creation fixture failed to build");
      const auto spacecraft=Moderator::Instance()->GetListOfObjects(Gmat::SPACECRAFT);
      require(spacecraft.size()==2,"Fixture must contain exactly two configured spacecraft");
      QString expected;
      for (const auto &type:QStringList{"EclipseLocator","ContactLocator","IntrusionLocator"}) {
         const auto field=type=="ContactLocator" ? "Target" : "Spacecraft";
         // Independent authoritative wx creation path: its default=true flag
         // uses Moderator::GetDefaultSpacecraft, rather than the Qt helper.
         QtResourcePreview legacy(Moderator::Instance()->CreateEventLocator(type.toStdString(),"",true));
         require(bool(legacy),"wx-style event default unavailable");
         const auto wanted=QString::fromStdString(legacy->GetStringParameter(field));
         require(wanted==QString::fromStdString(spacecraft.front()),"wx default differs from first configured spacecraft");
         auto draft=resourceDraft(type,"Unregistered");
         require(QString::fromStdString(draft->GetStringParameter(field))==wanted,"Qt draft disagrees with wx spacecraft default");
         require(!Moderator::Instance()->GetConfiguredObject("Unregistered") && editor->toPlainText()==source,
            "Preparing an event creator registered a resource or changed source");
         expected=wanted;
      }
      auto creator=[&](const QString &type,const QString &name,bool commit) {
         bool accepted=false; std::exception_ptr failure;
         QTimer::singleShot(0,&window,[&] {
            auto *dialog=window.findChild<QDialog *>("newResourceDialog");
            try {
               require(dialog,"New-resource dialog absent");
               dialog->findChild<QComboBox *>("resourceType")->setCurrentText(type);
               auto *target=dialog->findChild<QComboBox *>("event_"+(type=="ContactLocator" ? QString("Target") : QString("Spacecraft")));
               require(target && target->currentText()==expected,"Creation form did not display the wx default");
               dialog->findChild<QLineEdit *>("resourceName")->setText(name);
               if (commit) {
                  // Leave the target untouched: ResourceEditor filters unchanged
                  // fields, so the MainWindow transaction must persist it itself.
                  auto *buttons=dialog->findChild<QDialogButtonBox *>("resourceCreationButtons");
                  require(buttons && buttons->button(QDialogButtonBox::Ok),"Create button absent");
                  buttons->button(QDialogButtonBox::Ok)->click();
                  accepted=dialog->result()==QDialog::Accepted;
                  require(accepted,qPrintable(dialog->findChild<QLabel *>("resourceCreationStatus")->text()));
               }
            } catch (...) { failure=std::current_exception(); }
            if (dialog && (!commit || !accepted)) dialog->reject();
         });
         auto *action=window.findChild<QAction *>("createResource");
         require(action,"Create resource action absent"); action->trigger();
         if (failure) std::rethrow_exception(failure); return accepted;
      };
      for (const auto &type:QStringList{"EclipseLocator","ContactLocator"}) {
         const auto name="New"+type,field=type=="ContactLocator" ? QString("Target") : QString("Spacecraft");
         const auto before=editor->toPlainText();
         creator(type,name,false);
         require(editor->toPlainText()==before && !Moderator::Instance()->GetConfiguredObject(name.toStdString()),"Cancel mutated the event mission");
         require(creator(type,name,true),"Untouched default creation did not accept"); closePanels(window);
         const auto created=editor->toPlainText();
         require(QString::fromStdString(configured(name).GetStringParameter(field.toStdString()))==expected,
            "Untouched displayed event target was lost by creation");
         require(created.contains(name+"."+field+" = "+expected+";") && created.endsWith(before),"Creation did not persist target or changed unrelated source");
         const auto saved=files.filePath(name+" α.script");
         require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==created,
            "Created event source changed across Unicode save/reopen");
         require(QString::fromStdString(configured(name).GetStringParameter(field.toStdString()))==expected,"Reopen lost the created event target");
      }
      auto stable=editor->toPlainText();
      const auto alternate=QString::fromStdString(spacecraft.front())==expected ? QString::fromStdString(spacecraft.back()) : QString::fromStdString(spacecraft.front());
      require(alternate!=expected,"Explicit target must differ from the default");
      require(window.createResource("EclipseLocator","Explicit",stable,1,1,std::nullopt,{},{},{{"Spacecraft",alternate}}).isEmpty(),
         "Explicit alternate spacecraft creation failed");
      require(QString::fromStdString(configured("Explicit").GetStringParameter("Spacecraft"))==alternate,"Default replaced an explicit spacecraft");
      stable=editor->toPlainText();
      require(!window.createResource("EclipseLocator","Wrong",stable,1,1,std::nullopt,{},{},{{"Spacecraft","Area"}}).isEmpty() &&
         !window.createResource("ContactLocator","Wrong",stable,1,1,std::nullopt,{},{},{{"Target","Earth"}}).isEmpty() &&
         !window.createResource("EclipseLocator","Wrong",stable,1,1,std::nullopt,{},{},{{"Spacecraft",""}}).isEmpty() &&
         editor->toPlainText()==stable && !Moderator::Instance()->GetConfiguredObject("Wrong"),"Invalid/cleared target bypassed atomic creation rejection");
      const QString empty="Create PlanetographicRegion Area;\nCreate Variable Keep;\nKeep = 7;\n"+mission;
      editor->setPlainText(empty); require(window.buildScript(),"No-spacecraft event fixture failed");
      {
         auto draft=resourceDraft("EclipseLocator","Absent");
         require(draft->GetStringParameter("Spacecraft").empty() && Moderator::Instance()->GetListOfObjects(Gmat::SPACECRAFT).empty(),
            "Preparing a locator implicitly created a spacecraft");
      }
      require(window.createResource("EclipseLocator","Absent",empty).contains("Create a Spacecraft") &&
         window.createResource("ContactLocator","Absent",empty).contains("Create a Spacecraft") &&
         editor->toPlainText()==empty && !Moderator::Instance()->GetConfiguredObject("Absent"),"Missing target was not correctable before mutation");
      require(window.createResource("ContactLocator","RegionContact",empty,1,1,std::nullopt,{},{},{{"Target","Area"}}).isEmpty(),
         "Explicit valid Contact region was rejected without spacecraft");
      require(configured("RegionContact").GetStringParameter("Target")=="Area","Contact region was overwritten by default selection");
      const auto regionSource=editor->toPlainText(),saved=files.filePath("region contact α.script");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==regionSource &&
         configured("RegionContact").GetStringParameter("Target")=="Area","Explicit region target did not survive save/reopen");
      std::cout<<"PASS: wx-consistent existing-spacecraft event drafts, untouched Create/Cancel, explicit target source/readback/Unicode reopen, alternate and region choices, missing/invalid target rejection without implicit spacecraft\n";
      return 0;
   } catch (BaseException &error) { std::cerr<<"Event creation check failed: "<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<"Event creation check failed: "<<error.what()<<'\n'; return 1; }
}
