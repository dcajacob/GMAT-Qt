#include "AcceptanceView.hpp"
#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "ResourcePreview.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "ODEModel.hpp"
#include "PhysicalModel.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <algorithm>
#include <iostream>
#include <stdexcept>
static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static PhysicalModel *earth(ODEModel &model)
{
   for (int i=0;i<model.GetNumForces();++i) if (model.GetForce(i)->IsOfType("GravityField") && model.GetForce(i)->GetBodyName()=="Earth") return model.GetForce(i);
   return nullptr;
}
static StringArray bodies(ODEModel &model,const std::string &field)
{
   auto result=model.GetStringArrayParameter(field); std::sort(result.begin(),result.end()); return result;
}
static ODEModel *configured()
{
   auto *model=dynamic_cast<ODEModel *>(Moderator::Instance()->GetConfiguredObject("FM")); require(model,"Force model missing"); return model;
}
static QTableWidgetItem *field(ResourceEditor &panel,const QString &name)
{
   auto *table=panel.findChild<QTableWidget *>(); require(table,"Force property table missing");
   for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==name) { auto *value=table->item(row,1); require(value,"Force text property missing"); return value; }
   throw std::runtime_error(("Missing force property "+name).toStdString());
}
static QMdiSubWindow *open(MainWindow &window)
{
   auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto items=tree->findItems("FM",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Force resource missing"); tree->itemDoubleClicked(items.front(),0);
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("resourceName").toString()=="FM") return child;
   throw std::runtime_error("Force editor missing");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("ImplicitGravity");
   if (argc!=2 || !acceptanceWidgetPlatform()) return 2;
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Temporary files unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Initialization failed"); window.show(); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Script editor missing");
      const QString mission="BeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedDays = 0.1}; % retained mission\n";
      const QString source="% Retain implicit Earth gravity and unrelated text\nCreate Spacecraft Sat;\nCreate ForceModel FM;\nFM.ErrorControl = RSSStep; % retain spelling\nCreate Propagator Prop;\nProp.FM = FM;\nProp.InitialStepSize = 30;\n"+mission;
      editor->setPlainText(source); require(window.buildScript(),"Implicit gravity fixture did not build"); auto *originalEarth=earth(*configured()); require(originalEarth && originalEarth->GetName()=="_DefaultInternalForce_","Fixture does not exercise actual implicit default gravity");
      const auto potential=originalEarth->GetStringParameter("PotentialFileFullPath");
      // Point-mass-only editing must retain the same owned field and settings.
      QtResourcePreview detached(configured()->Clone()); auto *pending=dynamic_cast<ODEModel *>(detached.get()); auto *retained=earth(*pending); require(retained,"Detached implicit Earth missing");
      retained->SetIntegerParameter("Degree",12); retained->SetIntegerParameter("Order",12);
      applyGravityBodyProperties(*pending,{{"PointMasses","Luna, Sun"}});
      require(earth(*pending)==retained && retained->GetIntegerParameter("Degree")==12 && retained->GetIntegerParameter("Order")==12 && retained->GetStringParameter("PotentialFileFullPath")==potential,"Point-mass edit deleted or reset retained implicit gravity");
      auto *child=open(window); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"Force editor panel missing");
      acceptanceView(*panel);
      field(*panel,"PointMasses")->setText("Luna, Sun"); field(*panel,"GravityField.Earth.Degree")->setText("0"); field(*panel,"GravityField.Earth.Order")->setText("0");
      require(panel->hasChanges() && editor->toPlainText()==source && earth(*configured())==originalEarth,"Pending paired edit changed source/model");
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); const auto applied=editor->toPlainText();
      if (applied==source) std::cerr<<panel->findChild<QLabel *>("resourceStatus")->text().toStdString()<<'\n';
      require(applied!=source && child->widget()!=panel && !dynamic_cast<ResourceEditor *>(child->widget())->hasChanges(),"Paired point-mass and degree/order Apply failed");
      auto *gravity=earth(*configured()); require(gravity && gravity->GetIntegerParameter("Degree")==0 && gravity->GetIntegerParameter("Order")==0 && gravity->GetStringParameter("PotentialFileFullPath")==potential,"Paired Apply omitted or reset Earth gravity");
      require(bodies(*configured(),"PrimaryBodies")==StringArray{"Earth"} && bodies(*configured(),"PointMasses")==StringArray({"Luna","Sun"}),"Paired Apply changed effective gravity contributors");
      require(applied.contains("FM.ErrorControl = RSSStep; % retain spelling") && applied.endsWith(mission) && !applied.contains("PrimaryBodies = {}"),"Paired Apply rewrote unrelated source/mission or emptied Earth");
      editor->undo(); require(editor->toPlainText()==source,"Paired Apply Undo was not exact"); editor->redo(); require(editor->toPlainText()==applied,"Paired Apply Redo was not exact");
      const auto saved=files.filePath("Implicit Earth gravity.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==applied,"Paired gravity save/reopen changed source");
      gravity=earth(*configured()); require(gravity && gravity->GetIntegerParameter("Degree")==0 && bodies(*configured(),"PrimaryBodies")==StringArray{"Earth"} && bodies(*configured(),"PointMasses")==StringArray({"Luna","Sun"}),"Reopen changed paired gravity settings");
      // Removing the retained primary in a later transaction must still work.
      child=open(window); panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"Reopened force panel missing");
      field(*panel,"PrimaryBodies")->setText(""); field(*panel,"PointMasses")->setText("Earth, Luna, Sun");
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); const auto exchanged=editor->toPlainText();
      require(exchanged!=applied && !earth(*configured()) && bodies(*configured(),"PrimaryBodies").empty() && bodies(*configured(),"PointMasses")==StringArray({"Earth","Luna","Sun"}),"Explicit empty-primary exchange did not remove obsolete implicit gravity");
      require(exchanged.endsWith(mission) && !exchanged.contains("FM.GravityField.Earth."),"Removed primary retained its gravity assignments or changed mission");
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==exchanged && !earth(*configured()) && bodies(*configured(),"PrimaryBodies").empty(),"Empty-primary exchange save/reopen recreated obsolete gravity");
      std::cout<<"PASS implicit Earth gravity survives point-mass-only pending edits and actual paired point-mass/degree/order Apply, retains potential/settings, exact Undo/Redo/save/reopen and explicit empty-primary exchange; no numerical mission run.\n"; return 0;
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
