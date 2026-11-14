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
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTreeWidget>
#include <iostream>
#include <stdexcept>

static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static PhysicalModel *gravity(ODEModel &model)
{
   for (int i=0;i<model.GetNumForces();++i) if (model.GetForce(i)->IsOfType("GravityField") && model.GetForce(i)->GetBodyName()=="Mars") return model.GetForce(i);
   return nullptr;
}
static QTableWidgetItem *field(ResourceEditor &panel,const QString &name)
{
   auto *table=panel.findChild<QTableWidget *>(); require(table,"Force properties table missing");
   for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==name) {
      auto *value=table->item(row,1); require(value,"Force list text control missing"); return value;
   }
   throw std::runtime_error(("Missing force field "+name).toStdString());
}
static QMdiSubWindow *forceWindow(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("resourceName").toString()=="MarsForces") return child;
   throw std::runtime_error("Retained Mars force editor missing");
}
static ResourceEditor *forcePanel(MainWindow &window)
{
   auto *panel=dynamic_cast<ResourceEditor *>(forceWindow(window)->widget()); require(panel,"Force settings form missing"); return panel;
}
static ODEModel *configured()
{
   auto *model=dynamic_cast<ODEModel *>(Moderator::Instance()->GetConfiguredObject("MarsForces")); require(model,"Configured Mars force model missing"); return model;
}
static QString unchangedSource(QString source)
{
   // Remove only newly inserted Mars-owned settings; retain selector formatting,
   // comments, all other configuration bytes and the complete mission sequence.
   source.remove(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?MarsForces\\.GravityField\\.Mars\\.[A-Za-z]+[ \\t]*=[^;\\n]*;[ \\t]*\\n",QRegularExpression::MultilineOption));
   source.replace(QRegularExpression("(MarsForces\\.PrimaryBodies[ \\t]*=[ \\t]*)\\{Mars\\}"),"\\1{}");
   source.replace(QRegularExpression("(MarsForces\\.PointMasses[ \\t]*=[ \\t]*)\\{\\}"),"\\1{Mars}");
   return source;
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("MarsGravity");
   if (argc!=2 || !acceptanceWidgetPlatform()) return 2;
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Temporary files unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Mars gravity runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Script editor missing");
      const QString fixed="% Preserve independent configuration and spelling α\nCreate CoordinateSystem MarsFrame;\nMarsFrame.Origin = Mars;\nMarsFrame.Axes = MJ2000Eq;\nCreate Spacecraft Vehicle;\nVehicle.CoordinateSystem = MarsFrame;\nVehicle.DateFormat = UTCGregorian;\nVehicle.Epoch = '01 Jan 2024 12:00:00.000';\nVehicle.X = 4000;\nVehicle.Y = 0;\nVehicle.Z = 0;\nVehicle.VX = 0;\nVehicle.VY = 3.3;\nVehicle.VZ = 0.2;\nCreate ForceModel MarsForces;\nMarsForces.CentralBody = Mars;\n";
      const QString tail="MarsForces.Drag.AtmosphereModel = None; % retain drag choice\nMarsForces.SRP = Off;\nMarsForces.RelativisticCorrection = Off;\nCreate Propagator Prop;\nProp.FM = MarsForces;\nProp.Accuracy = 1e-13;\nProp.InitialStepSize = 10;\nBeginMissionSequence;\nPropagate Prop(Vehicle) {Vehicle.ElapsedSecs = 180}; % preserve mission β\n";
      const auto baseline=fixed+"MarsForces.PrimaryBodies = {}; % retain primary comment\nMarsForces.PointMasses = {Mars}; % retain point comment\n"+tail;
      const auto independent=fixed+"MarsForces.PrimaryBodies = {Mars};\nMarsForces.PointMasses = {};\nMarsForces.GravityField.Mars.Degree = 4;\nMarsForces.GravityField.Mars.Order = 4;\n"+tail;
      // This independent script delegates the default model to the existing
      // interpreter rather than seeding the Qt editor with a potential path.
      editor->setPlainText(independent); require(window.buildScript(),"Independent Mars default configuration failed");
      const auto expectedPath=QString::fromStdString(Moderator::Instance()->GetPotentialFileName("MARS50C"));
      require(QFileInfo(expectedPath).isFile() && QFileInfo(expectedPath).fileName()=="Mars50c.cof","Official startup Mars potential fixture unavailable");
      auto *reference=gravity(*configured()); require(reference,"Interpreter did not prepare Mars gravity");
      require(QFileInfo(QString::fromStdString(reference->GetStringParameter("PotentialFileFullPath"))).canonicalFilePath()==QFileInfo(expectedPath).canonicalFilePath(),"Independent interpreter Mars default did not match startup resolution");
      editor->setPlainText(baseline); require(window.buildScript(),"Point-mass Mars baseline failed");
      auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto items=tree->findItems("MarsForces",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Mars force resource missing"); tree->itemDoubleClicked(items.front(),0);
      auto *panel=forcePanel(window); auto *previous=configured(); const auto undoSteps=editor->document()->availableUndoSteps();
      acceptanceView(*panel);
      field(*panel,"PrimaryBodies")->setText("MissingBody"); field(*panel,"PointMasses")->setText(""); panel->requestApply();
      require(editor->toPlainText()==baseline && configured()==previous && panel==forcePanel(window) && panel->hasChanges() && editor->document()->availableUndoSteps()==undoSteps,"Invalid gravity selection changed source/model/Undo or lost pending fields");
      field(*panel,"PrimaryBodies")->setText("Mars");
      require(editor->toPlainText()==baseline && !gravity(*configured()),"Pending Mars primary changed configured gravity before Apply");
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      const auto applied=editor->toPlainText();
      if (applied==baseline) std::cerr<<forcePanel(window)->findChild<QLabel *>("resourceStatus")->text().toStdString()<<'\n';
      require(applied!=baseline && !forcePanel(window)->hasChanges(),"Mars primary Apply failed to commit a clean retained editor");
      auto *force=gravity(*configured()); require(force,"Mars primary Apply omitted the owned gravity field");
      require(QFileInfo(QString::fromStdString(force->GetStringParameter("PotentialFileFullPath"))).canonicalFilePath()==QFileInfo(expectedPath).canonicalFilePath(),"Qt Mars default did not resolve the configured case-sensitive potential path");
      require(force->GetIntegerParameter("Degree")==4 && force->GetIntegerParameter("Order")==4,"Mars default degree/order changed");
      require(unchangedSource(applied)==baseline,"Mars Apply rewrote unrelated source/comments or mission");
      require(forceWindow(window)->close(),"Clean Mars editor did not close"); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      editor->undo(); require(editor->toPlainText()==baseline && window.buildScript() && !gravity(*configured()),"Mars gravity Apply Undo was not exact");
      editor->redo(); require(editor->toPlainText()==applied && window.buildScript(),"Mars gravity Apply Redo was not exact");
      // An existing user-selected potential must not be reset when another
      // gravity contributor changes. This is a detached preview only.
      const auto customPath=files.filePath("custom-mars.cof"); require(QFile::copy(expectedPath,customPath),"Cannot stage custom Mars potential");
      QtResourcePreview pending(configured()->Clone()); auto *copy=dynamic_cast<ODEModel *>(pending.get()); auto *owned=gravity(*copy); require(owned,"Detached Mars gravity missing");
      owned->SetStringParameter("PotentialFile",customPath.toStdString());
      applyGravityBodyProperties(*copy,{{"PrimaryBodies","Mars"},{"PointMasses","Luna"}});
      require(gravity(*copy)==owned && QString::fromStdString(owned->GetStringParameter("PotentialFileFullPath"))==customPath && editor->toPlainText()==applied,"Adding a contributor replaced retained custom gravity or changed source");
      const auto saved=files.filePath("Mars gravity Δ.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==applied,"Mars gravity Unicode save/reopen changed source");
      std::cout<<"PASS actual pending Mars primary Apply, startup-resolved Mars50c potential, invalid-selection recovery, retained custom potential, unchanged source/comments/mission, exact Undo/Redo/Unicode save-reopen; no numerical mission or native desktop run.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
