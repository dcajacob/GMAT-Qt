#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "ResourcePreview.hpp"
#include "EventLocatorDialog.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "PlanetographicRegion.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static PlanetographicRegion *region()
{
   auto *object=dynamic_cast<PlanetographicRegion *>(Moderator::Instance()->GetConfiguredObject("Gate"));
   require(object,"Region was not registered"); return object;
}
static ResourceEditor *regionPanel(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList())
      if (child->property("resourceName").toString()=="Gate") {
         auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"Region form missing"); return panel;
      }
   throw std::runtime_error("Retained region editor missing");
}
static void closeRegion(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList())
      if (child->property("resourceName").toString()=="Gate") child->close();
   QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static void openRegion(MainWindow &window)
{
   auto *tree=window.findChild<QTreeWidget *>("Resources");
   const auto matches=tree->findItems("Gate",Qt::MatchExactly|Qt::MatchRecursive); require(matches.size()==1,"Region tree item missing");
   tree->itemDoubleClicked(matches.front(),0);
}
static void polygon(ResourceEditor *panel,const QStringList &latitude,const QStringList &longitude)
{
   require(latitude.size()==longitude.size(),"Test vertex data mismatch");
   auto *table=panel->findChild<QTableWidget *>("regionVertices"); require(table,"Paired inline vertex table missing");
   table->setRowCount(latitude.size());
   for (int i=0;i<latitude.size();++i) {
      table->setItem(i,0,new QTableWidgetItem(latitude[i])); table->setItem(i,1,new QTableWidgetItem(longitude[i]));
   }
}
static void checkVertices(const QStringList &latitude,const QStringList &longitude,const char *stage,const QString &source)
{
   const Rvector lat=region()->GetRvectorParameter("Latitude"),lon=region()->GetRvectorParameter("Longitude");
   if (lat.GetSize()!=latitude.size() || lon.GetSize()!=longitude.size()) {
      std::cerr<<"Region stage="<<stage<<" expected latitude="<<latitude.size()<<" longitude="<<longitude.size()
         <<" actual latitude="<<lat.GetSize()<<" longitude="<<lon.GetSize()<<"\nLatitude="<<lat.ToString(17)<<"\nLongitude="<<lon.ToString(17)<<'\n';
      for (const auto &line:source.split('\n')) if (line.contains("Gate")) std::cerr<<"source: "<<line.toStdString()<<'\n';
   }
   require(lat.GetSize()==latitude.size() && lon.GetSize()==longitude.size(),"Region vertex count does not match the requested paired input");
   for (int i=0;i<lat.GetSize();++i) require(std::abs(lat[i]-latitude[i].toDouble())<1e-12 && std::abs(lon[i]-longitude[i].toDouble())<1e-12,"Committed paired vertex changed");
   require(region()->GetStringParameter("AreaFileName").empty(),"Vertex mode retained an area file");
}
static QString unrelated(QString source)
{
   source.remove(QRegularExpression("^Create PlanetographicRegion Gate;\\n",QRegularExpression::MultilineOption));
   source.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?Gate\\.[A-Za-z][A-Za-z0-9_.]*[ \\t]*=[^\\n]*;[^\\n]*\\n",QRegularExpression::MultilineOption));
   // Only these exact Region-owned fixture comments may become standalone
   // when a geometry assignment is removed; all other source bytes stay exact.
   source.remove(QRegularExpression("^[ \\t]*% keep region (?:latitude γ|longitude δ|area ε)[ \\t]*\\n",QRegularExpression::MultilineOption));
   return source;
}
static QStringList choices(QListWidget *list)
{
   QStringList result; for (int i=0;i<list->count();++i) result.append(list->item(i)->text()); return result;
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("RegionWorkflow");
   if (argc!=2 || !acceptanceWidgetPlatform()) return 2;
   try {
      TestSettings isolated; QTemporaryDir files; require(files.isValid(),"Temporary files unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Region runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Script editor missing");
      const QString baseline="% Preserve owned unrelated mission α\nCreate Spacecraft Vehicle Vehicle2;\nCreate GroundStation Site;\nCreate ContactLocator Contacts;\nContacts.Target = Vehicle;\nContacts.Observers = {Site};\nContacts.UseLightTimeDelay = false;\nContacts.UseStellarAberration = false;\nCreate Variable Keep;\nKeep = 7; % unchanged initializer\nBeginMissionSequence;\nKeep = Keep + 2; % unchanged mission β\n";
      editor->setPlainText(baseline); require(window.buildScript(),"Region baseline failed");
      const QStringList lat{"10","10","-10","-10"},lon{"5","-5","-5","5"};
      std::exception_ptr failure;
      const auto creator=[&](bool cancel) {
         QTimer::singleShot(0,&window,[&] {
            auto *dialog=window.findChild<QDialog *>("newResourceDialog");
            try {
               require(dialog,"Region creator missing");
               auto *type=dialog->findChild<QComboBox *>("resourceType");
               require(type && type->findText("PlanetographicRegion")>=0,"Region is not offered by the resource creator");
               type->setCurrentText("PlanetographicRegion");
               dialog->findChild<QLineEdit *>("resourceName")->setText("Gate");
               ResourceEditor *panel=nullptr;
               // ResourceEditor has no Q_OBJECT macro; inspect QWidget owners.
               for (auto *widget:dialog->findChildren<QWidget *>())
                  if (auto *candidate=dynamic_cast<ResourceEditor *>(widget); candidate && candidate->findChild<QTableWidget *>("regionVertices")) panel=candidate;
               if (!panel) std::cerr<<"Creator type="<<type->currentText().toStdString()<<" status="<<dialog->findChild<QLabel *>("resourceCreationStatus")->text().toStdString()<<'\n';
               require(panel,"Region creator did not construct the paired vertex form");
               if (!cancel) {
                  dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok)->click();
                  require(dialog->result()!=QDialog::Accepted && editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject("Gate"),"Untouched Region creator registered blank geometry");
               }
               polygon(panel,lat,lon);
               require(editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject("Gate"),"Pending draft changed configured model/source");
               if (cancel) dialog->reject();
               else {
                  auto *create=dialog->findChild<QDialogButtonBox *>("resourceCreationButtons")->button(QDialogButtonBox::Ok);
                  panel->findChild<QTableWidget *>("regionVertices")->item(0,0)->setText("95");
                  const auto undo=editor->document()->availableUndoSteps(); create->click();
                  require(dialog->result()!=QDialog::Accepted && editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject("Gate") && panel->hasChanges() && editor->document()->availableUndoSteps()==undo,"Invalid latitude creation was not rolled back/pending");
                  panel->findChild<QTableWidget *>("regionVertices")->item(0,0)->setText("10"); create->click();
                  if (dialog->result()!=QDialog::Accepted) std::cerr<<dialog->findChild<QLabel *>("resourceCreationStatus")->text().toStdString()<<'\n';
                  require(dialog->result()==QDialog::Accepted,"Valid paired polygon creation failed");
               }
            } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
         });
         window.findChild<QAction *>("createResource")->trigger(); if (failure) std::rethrow_exception(failure);
      };
      const auto blankError=window.createResource("PlanetographicRegion","BlankRegion",baseline);
      require(blankError.contains("Enter between 3 and 100000 paired region vertices") && editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject("BlankRegion"),"Direct empty Region creation did not reach geometry validation");
      const auto categories=window.findChild<QTreeWidget *>("Resources")->findItems("Regions",Qt::MatchExactly|Qt::MatchRecursive);
      require(categories.size()==1 && categories.front()->data(0,Qt::UserRole+1).toUInt()==Gmat::REGION,"Region category creation route missing");
      creator(true); require(editor->toPlainText()==baseline && !Moderator::Instance()->GetConfiguredObject("Gate"),"Creator Cancel configured a polygon");
      creator(false); checkVertices(lat,lon,"creator accepted",editor->toPlainText()); auto created=editor->toPlainText();
      const auto regionItems=window.findChild<QTreeWidget *>("Resources")->findItems("Gate",Qt::MatchExactly|Qt::MatchRecursive);
      require(regionItems.size()==1 && regionItems.front()->parent() && regionItems.front()->parent()->text(0)=="Regions" && regionItems.front()->parent()->data(0,Qt::UserRole+1).toUInt()==Gmat::REGION,"Created Region is not grouped under its typed category");
      require(unrelated(created)==baseline,"Region creation changed unrelated source bytes/order"); closeRegion(window);
      editor->undo(); require(editor->toPlainText()==baseline && window.buildScript() && !Moderator::Instance()->GetConfiguredObject("Gate"),"Region creation was not one exact Undo");
      editor->redo(); require(editor->toPlainText()==created && window.buildScript(),"Region creation Redo failed"); checkVertices(lat,lon,"creation redo",editor->toPlainText());
      created.replace(QRegularExpression("(Gate\\.Latitude[^\\n]*;)"),"\\1 % keep region latitude γ");
      created.replace(QRegularExpression("(Gate\\.Longitude[^\\n]*;)"),"\\1 % keep region longitude δ");
      editor->setPlainText(created); require(window.buildScript(),"Commented Region source failed");
      openRegion(window); auto *panel=regionPanel(window); auto *configured=region(); const auto undo=editor->document()->availableUndoSteps();
      const QStringList nextLat{"8","8","-8"},nextLon{"-8","8","0"}; polygon(panel,nextLat,nextLon);
      panel->findChild<QTableWidget *>("regionVertices")->item(0,0)->setText("nan"); panel->requestApply();
      require(region()==configured && editor->toPlainText()==created && panel->hasChanges() && editor->document()->availableUndoSteps()==undo,"Invalid vertex Apply changed source/model/Undo or lost pending");
      QString applyStatus; panel->onStatus=[&applyStatus](const QString &message) { applyStatus=message; };
      polygon(panel,nextLat,nextLon); panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      if (editor->toPlainText()==created) std::cerr<<"Replacement Apply status="<<applyStatus.toStdString()<<'\n';
      checkVertices(nextLat,nextLon,"replacement Apply",editor->toPlainText()); const auto edited=editor->toPlainText(); require(edited!=created && unrelated(edited)==baseline,"Polygon replacement changed unrelated source or did not commit");
      require(!regionPanel(window)->hasChanges(),"Applied paired form reopened dirty"); closeRegion(window);
      editor->undo(); require(editor->toPlainText()==created && window.buildScript(),"Paired polygon edit was not one exact Undo"); checkVertices(lat,lon,"replacement Undo",editor->toPlainText());
      editor->redo(); require(editor->toPlainText()==edited && window.buildScript(),"Paired polygon edit Redo failed"); checkVertices(nextLat,nextLon,"replacement Redo",editor->toPlainText());
      // Narrow public core API regression: mismatch cannot mutate even the
      // detached owner; generic Rvector setter remains intentionally unused.
      QtResourcePreview preview(region()->Clone()); const Rvector before=preview->GetRvectorParameter("Latitude"); bool rejected=false;
      try { applyRegionProperties(*preview,{{"Latitude","1 2 3"},{"Longitude","1 2 3 4"}}); } catch (const std::exception &) { rejected=true; }
      require(rejected && preview->GetRvectorParameter("Latitude").GetSize()==before.GetSize(),"Mismatched vertex lists mutated preview");
      rejected=false; try { applyRegionProperties(*preview,{{"CentralBody","Mars"}}); } catch (const std::exception &) { rejected=true; }
      require(rejected && preview->GetStringParameter("CentralBody")=="Earth","Unsupported region body mutated preview");
      const auto area=files.filePath("owned-region.AT"); QFile data(area); require(data.open(QIODevice::WriteOnly),"Owned area file unavailable"); data.write("PlanetographicRegion\n10 5\n10 -5\n-10 -5\n-10 5\n"); data.close();
      openRegion(window); panel=regionPanel(window); auto *mode=panel->findChild<QComboBox *>("regionSourceMode"); mode->setCurrentIndex(1);
      auto *path=panel->findChild<QLineEdit *>("region_AreaFileName"); path->setText(area);
      require(!panel->findChild<QTableWidget *>("regionVertices")->isEnabled() && !panel->findChild<QPushButton *>("regionAddVertex")->isEnabled(),"File mode did not disable vertex editing");
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      auto fileSource=editor->toPlainText(); require(region()->GetStringParameter("AreaFileName")==area.toStdString(),"Area file was not committed");
      require(!fileSource.contains(QRegularExpression("Gate\\.(?:Latitude|Longitude)\\s*=")) && unrelated(fileSource)==baseline,"File mode serialized derived vertices or changed unrelated source");
      require(fileSource.count("keep region latitude γ")==1 && fileSource.count("keep region longitude δ")==1,"Vertex-to-file transition lost/duplicated inline comments");
      closeRegion(window); fileSource.replace(QRegularExpression("(Gate\\.AreaFileName[^\\n]*;)"),"\\1 % keep region area ε");
      editor->setPlainText(fileSource); require(window.buildScript(),"Commented area-file Region source failed"); openRegion(window);
      preview=QtResourcePreview(region()->Clone());
      // Build-only configuration has not gone through Sandbox context setup.
      preview->SetSolarSystem(Moderator::Instance()->GetSolarSystemInUse()); preview->Initialize(); const auto fileBlock=regionResourceScript(*preview);
      require(!fileBlock.contains(".Latitude") && !fileBlock.contains(".Longitude") && fileBlock.contains(".AreaFileName"),"Initialized file-backed serialization defined both inputs");
      panel=regionPanel(window); mode=panel->findChild<QComboBox *>("regionSourceMode"); require(mode->currentIndex()==1,"File mode reopening lost input choice");
      path=panel->findChild<QLineEdit *>("region_AreaFileName"); path->setText(files.filePath("missing.AT")); configured=region(); panel->requestApply();
      require(region()==configured && editor->toPlainText()==fileSource && panel->hasChanges(),"Missing area file Apply was not rolled back");
      mode->setCurrentIndex(0); polygon(panel,lat,lon); panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      checkVertices(lat,lon,"file-to-vertices Apply",editor->toPlainText()); const auto restored=editor->toPlainText(); require(!restored.contains("Gate.AreaFileName") && unrelated(restored)==baseline,"File-to-vertices left stale file/changed source");
      require(restored.count("keep region latitude γ")==1 && restored.count("keep region longitude δ")==1 && restored.count("keep region area ε")==1,"File-to-vertices transition lost/duplicated exact comments"); closeRegion(window);
      const auto saved=files.filePath("region α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==restored,"Unicode Region save/reopen changed exact source"); checkVertices(lat,lon,"Unicode reopen",editor->toPlainText());
      // Pending target changes expose the documented observer category. No bad
      // intermediate target/observer combination is applied to the model.
      auto *contacts=Moderator::Instance()->GetConfiguredObject("Contacts"); require(contacts,"Contact fixture missing");
      EventLocatorDialog locator(*contacts,{},&window); auto *target=locator.findChild<QComboBox *>("event_Target"); auto *observers=locator.findChild<QListWidget *>("event_Observers");
      require(choices(observers).contains("Site") && choices(observers).contains("Vehicle2") && !choices(observers).contains("Vehicle"),"Existing spacecraft-target observer choices regressed");
      target->setCurrentText("Gate"); require(choices(observers).contains("Vehicle") && choices(observers).contains("Vehicle2") && !choices(observers).contains("Site"),"Pending Region target did not expose spacecraft-only choices");
      preview=QtResourcePreview(contacts->Clone()); rejected=false;
      try { applyEventLocatorProperties(*preview,{{"Target","Gate"},{"Observers","Vehicle, Vehicle2"}}); } catch (const std::exception &) { rejected=true; }
      require(rejected && editor->toPlainText()==restored && contacts==Moderator::Instance()->GetConfiguredObject("Contacts"),"Region observer cardinality changed live model/source");
      preview=QtResourcePreview(contacts->Clone()); applyEventLocatorProperties(*preview,{{"Target","Gate"},{"Observers","Vehicle"}});
      require(preview->GetStringParameter("Target")=="Gate" && preview->GetStringArrayParameter("Observers")==StringArray{"Vehicle"},"Paired target/observer update required an invalid intermediate Apply");
      // Preserve an imported spacecraft observer on inspection/unrelated edit/Cancel;
      // its runtime Imager/FOV preconditions are outside this configuration check.
      preview=QtResourcePreview(contacts->Clone()); preview->TakeAction("Clear","Observers"); preview->SetStringParameter("Observers","Vehicle2");
      EventLocatorDialog imported(*preview,{},&window); auto *importedObservers=imported.findChild<QListWidget *>("event_Observers");
      require(choices(importedObservers).contains("Vehicle2") && imported.settings().value("Observers")=="Vehicle2","Opening unsupported imported observers silently removed source values");
      imported.reject(); applyEventLocatorProperties(*preview,{{"WriteReport","false"}});
      require(preview->GetStringArrayParameter("Observers")==StringArray{"Vehicle2"},"Unrelated contact edit rejected/removed an existing spacecraft observer");
      locator.reject(); require(editor->toPlainText()==restored,"Locator inspection/Cancel rewrote source");
      std::cout<<"PASS Region paired inline creation/Cancel, finite/core polygon validation rollback, replacement without appended vertices, exact source Undo/Redo/Unicode reopen, file dependencies/switching/derived serialization, Help observer filtering/cardinality and unsupported import Cancel; no numerical mission or native launch.\n";
   } catch (BaseException &failure) { std::cerr<<failure.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &failure) { std::cerr<<failure.what()<<'\n'; return 1; }
}
