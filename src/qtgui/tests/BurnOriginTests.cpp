#include "AcceptanceView.hpp"
#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "BurnDialog.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message)
{
   if (!value) throw std::runtime_error(message);
}

static GmatBase &configured(const char *name)
{
   auto *object=Moderator::Instance()->GetConfiguredObject(name);
   require(object,"Expected resource is missing");
   return *object;
}

static void checkOriginReferences(GmatBase &object)
{
   QStringList bodies;
   // Moderator reuses its returned list; retain it before querying each object.
   const auto names=Moderator::Instance()->GetListOfObjects(Gmat::CELESTIAL_BODY);
   for (const auto &name:names) {
      auto *body=Moderator::Instance()->GetConfiguredObject(name);
      require(body && body->IsOfType(Gmat::CELESTIAL_BODY),"Origin inventory contains a non-body");
      bodies.append(QString::fromStdString(name));
   }
   bodies.removeDuplicates(); bodies.sort();
   require(bodies.contains("Earth") && bodies.contains("Mars") && bodies.contains("Luna"),"Required origin bodies unavailable");
   bool found=false;
   for (const auto &field:resourceProperties(object)) if (field.name=="Origin") {
      found=true;
      require(field.references==bodies,"Origin metadata omits valid celestial bodies or includes other resources");
   }
   require(found,"Local Origin metadata absent");
}

static void selectMars(ResourceEditor &panel)
{
   acceptanceView(panel);
   auto *frame=panel.findChild<QComboBox *>("burn_CoordinateSystem");
   auto *axes=panel.findChild<QComboBox *>("burn_Axes");
   auto *origin=panel.findChild<QComboBox *>("burn_Origin");
   require(frame && axes && origin,"Inline burn frame/origin controls absent");
   require(!origin->isEditable() && origin->findText("Mars")>=0 && origin->findText("Earth")>=0,
      "Burn Origin does not offer Mars and Earth as body choices");
   require(origin->findText("Vehicle")<0 && origin->findText("EarthMJ2000Eq")<0,
      "Burn Origin offers a spacecraft or coordinate system");
   require(!origin->isEnabled(),"Origin should be inactive for a named coordinate system");
   frame->setCurrentText("Local"); axes->setCurrentText("VNB");
   require(origin->isEnabled(),"Local VNB origin remains inactive");
   origin->setCurrentText("Mars");
   require(origin->currentText()=="Mars","Selecting Mars did not change the pending origin");
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication app(argc,argv);
   if (!acceptanceWidgetPlatform() || argc!=2) return 2;
   QApplication::setOrganizationName("GMATTests");
   QApplication::setApplicationName("BurnOrigin");
   try {
      TestSettings settings; QTemporaryDir files;
      require(files.isValid(),"Temporary source directory unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();
      QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      require(editor,"Source editor absent");
      const QString mission="BeginMissionSequence;\nKeep = Keep + 2; % retain mission and Mars comment α\n";
      const QString source="% Retain source: Origin = Earth is only a comment\n"
         "Create Spacecraft Vehicle;\nCreate Variable Keep;\nKeep = 7;\n"
         "Create ChemicalThruster Engine;\nCreate ImpulsiveBurn MOI;\n"
         "MOI.CoordinateSystem = EarthMJ2000Eq;\nMOI.Element1 = -0.02;\n"+mission;
      editor->setPlainText(source); require(window.buildScript(),"Origin fixture did not build");
      acceptanceView(window);
      checkOriginReferences(configured("MOI"));
      checkOriginReferences(configured("Engine"));
      {
         BurnDialog dialog(configured("MOI"),{{"CoordinateSystem","Local"},{"Axes","VNB"}});
         dialog.show();
      acceptanceView(dialog);
         dialog.findChild<QComboBox *>("burn_Origin")->setCurrentText("Mars");
         require(dialog.settings().value("Origin")=="Mars","Detached dialog cannot select Mars");
         dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Cancel)->click();
         require(dialog.result()==QDialog::Rejected && !dialog.isVisible(),"Burn dialog Cancel did not close/reject");
         require(configured("MOI").GetStringParameter("Origin")=="Earth" && editor->toPlainText()==source,
            "Burn dialog Cancel mutated the applied mission");
      }
      QString error="Apply was not called";
      int applied=0;
      {
         ResourceEditor panel(configured("MOI"),[&](const auto &changes) {
            ++applied; error=window.applyResourceChanges("MOI",changes,source); return error;
         },nullptr,source);
         panel.show(); selectMars(panel);
         require(panel.hasChanges(),"Mars selection was not retained as a pending edit");
         require(configured("MOI").GetStringParameter("Origin")=="Earth" && editor->toPlainText()==source,
            "Pending Mars selection mutated the applied mission");
      }
      require(applied==0 && editor->toPlainText()==source && configured("MOI").GetStringParameter("Origin")=="Earth",
         "Discarding an unapplied editor changed source or engine origin");
      {
         ResourceEditor panel(configured("MOI"),[&](const auto &changes) {
            ++applied; error=window.applyResourceChanges("MOI",changes,source); return error;
         },nullptr,source);
         panel.show(); selectMars(panel); panel.requestApply();
         require(error.isEmpty(),qPrintable(error));
      }
      const auto changed=editor->toPlainText();
      require(applied==1 && configured("MOI").GetStringParameter("CoordinateSystem")=="Local" &&
         configured("MOI").GetStringParameter("Origin")=="Mars","Apply did not preserve the selected local Mars origin");
      require(changed.contains("MOI.Origin = Mars;") && changed.contains("% Retain source: Origin = Earth is only a comment") &&
         changed.endsWith(mission),"Origin Apply changed unrelated source/comments/mission");
      const auto saved=files.filePath("Mars origin α.script");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Mars origin save/reopen failed");
      require(editor->toPlainText()==changed && configured("MOI").GetStringParameter("Origin")=="Mars",
         "Save/reopen lost Mars origin or changed source bytes");
      {
         BurnDialog dialog(configured("MOI"),{{"Origin","Luna"}});
         dialog.show();
      acceptanceView(dialog);
         auto *origin=dialog.findChild<QComboBox *>("burn_Origin");
         require(origin && origin->currentText()=="Luna" && origin->findText("Mars")>=0,
            "Reopened dialog lost the pending origin or other celestial-body choices");
         dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Cancel)->click();
         require(!dialog.isVisible() && configured("MOI").GetStringParameter("Origin")=="Mars" && editor->toPlainText()==changed,
            "Pending-origin Cancel failed to close or changed the reopened mission");
      }
      std::cout<<"PASS: celestial-body Origin metadata for burn/thruster, inline Local Mars selection, pending discard and dialog Cancel, Apply/source preservation, Unicode save/reopen. No numerical mission or OS input qualification.\n";
      return 0;
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
