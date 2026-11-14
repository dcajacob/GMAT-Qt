#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourcePreview.hpp"
#include "ResourceProperties.hpp"
#include "TestSettings.hpp"
#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "Moderator.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include "Propagate.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTreeWidget>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static PropSetup *setup(GmatBase *object)
{
   auto *result=dynamic_cast<PropSetup *>(object); require(result && result->GetPropagator(),"Owned propagator missing"); return result;
}
static void checkSettings(GmatBase *object)
{
   auto *propagator=setup(object)->GetPropagator();
   require(propagator->GetTypeName()=="BulirschStoer" &&
      std::abs(propagator->GetRealParameter("MinimumReduction")-0.6)<1e-15 &&
      std::abs(propagator->GetRealParameter("MaximumReduction")-0.0001)<1e-18,
      "Bulirsch configuration was reset while cloning or rebuilding");
}
static QMdiSubWindow *panelWindow(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList())
      if (child->property("resourceName").toString()=="Prop") return child;
   throw std::runtime_error("Propagator panel missing");
}
static ResourceEditor *openPanel(MainWindow &window)
{
   auto *tree=window.findChild<QTreeWidget *>("Resources");
   const auto items=tree->findItems("Prop",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Propagator tree item missing");
   tree->itemDoubleClicked(items.front(),0);
   auto *panel=dynamic_cast<ResourceEditor *>(panelWindow(window)->widget()); require(panel,"Propagator form missing");
   acceptanceView(*panelWindow(window)); return panel;
}
static QString withoutReductions(QString source)
{
   source.remove(QRegularExpression("^(?:GMAT[ \\t]+)?Prop\\.(?:MinimumReduction|MaximumReduction)[ \\t]*=[^;\\n]*;[^\\n]*\\n",QRegularExpression::MultilineOption)); return source;
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("BulirschControls");
   // The same source/native assertions work on the opt-in visible widget
   // platform; unattended execution retains the ordinary offscreen route.
   if (argc!=2 || !acceptanceWidgetPlatform()) return 2;
   try {
      TestSettings isolated; QTemporaryDir files; require(files.isValid(),"Temporary files unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Runtime initialization failed"); window.show(); acceptanceView(window);
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Script editor missing");
      const auto report=files.filePath("endpoint.txt");
      const QString baseline=QString("%% Preserve source α and original numeric spelling\nCreate Spacecraft Vehicle;\nVehicle.X = 7100;\nVehicle.Y = 0;\nVehicle.Z = 1300;\nVehicle.VX = 0;\nVehicle.VY = 7.35;\nVehicle.VZ = 1;\nCreate ForceModel Gravity;\nGravity.PrimaryBodies = {};\nGravity.PointMasses = {Earth};\nCreate Propagator Prop;\nProp.FM = Gravity;\nProp.Type = BulirschStoer;\nProp.InitialStepSize = 60;\nProp.Accuracy = 1e-11;\nProp.MinimumReduction = 0.7;\nProp.MaximumReduction = 1e-05;\nCreate ReportFile Endpoint;\nEndpoint.Filename = '%1';\nEndpoint.Precision = 16;\nEndpoint.WriteReport = false;\nBeginMissionSequence;\nPropagate Prop(Vehicle) {Vehicle.ElapsedSecs = 60};\nReport Endpoint Vehicle.ElapsedSecs Vehicle.X Vehicle.Y Vehicle.Z;\n").arg(report);
      editor->setPlainText(baseline); require(window.buildScript(),"Bulirsch baseline failed to build");
      auto *object=Moderator::Instance()->GetConfiguredObject("Prop");
      const auto fields=resourceProperties(*object);
      require(std::any_of(fields.cbegin(),fields.cend(),[](const auto &field) { return field.name=="MinimumReduction"; }) &&
         std::any_of(fields.cbegin(),fields.cend(),[](const auto &field) { return field.name=="MaximumReduction"; }) &&
         std::none_of(fields.cbegin(),fields.cend(),[](const auto &field) { return field.name=="MinimumTolerance"; }),
         "Writable reduction fields missing or deprecated MinimumTolerance exposed");
      auto *panel=openPanel(window);
      auto *minimum=panel->findChild<QLineEdit *>("resource_MinimumReduction"),*maximum=panel->findChild<QLineEdit *>("resource_MaximumReduction");
      require(minimum && maximum && !panel->findChild<QWidget *>("resource_MinimumTolerance"),"Bulirsch controls incorrect");
      minimum->setText("0.6"); maximum->setText("0.0001");
      require(panel->hasChanges() && editor->toPlainText()==baseline,"Pending reduction fields committed before Apply");
      acceptanceView(*panelWindow(window));
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      acceptanceView(*panelWindow(window));
      const auto edited=editor->toPlainText();
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      if (!evidence.isEmpty()) {
         QFile script(QDir(evidence).filePath("post-apply.script")); if (script.open(QIODevice::WriteOnly)) script.write(edited.toUtf8());
         auto *configured=Moderator::Instance()->GetConfiguredObject("Prop");
         auto *current=dynamic_cast<ResourceEditor *>(panelWindow(window)->widget());
         QFile diagnostic(QDir(evidence).filePath("post-apply.txt"));
         if (diagnostic.open(QIODevice::WriteOnly)) diagnostic.write(("minimum="+QString::number(setup(configured)->GetPropagator()->GetRealParameter("MinimumReduction"),'g',17)+"\nmaximum="+QString::number(setup(configured)->GetPropagator()->GetRealParameter("MaximumReduction"),'g',17)+"\npending="+QString::number(current&&current->hasChanges())+"\n").toUtf8());
      }
      require(edited!=baseline && edited.contains("Prop.MinimumReduction = 0.6;") && edited.contains("Prop.MaximumReduction = 0.0001;"),"GUI Apply serialized default reduction values");
      require(withoutReductions(edited)==withoutReductions(baseline),"Reduction Apply changed unrelated source");
      checkSettings(Moderator::Instance()->GetConfiguredObject("Prop"));
      {
         QtResourcePreview copy(Moderator::Instance()->GetConfiguredObject("Prop")->Clone()); checkSettings(copy.get());
         QtResourcePreview nested(copy->Clone()); checkSettings(nested.get());
         const auto *owned=setup(nested.get())->GetPropagator();
         QtResourcePreview direct(owned->Clone());
         require(std::abs(direct->GetRealParameter("MinimumReduction")-0.6)<1e-15 && std::abs(direct->GetRealParameter("MaximumReduction")-0.0001)<1e-18,"Direct propagator Clone reset configured reductions");
      }
      require(panelWindow(window)->close(),"Clean form did not close"); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      editor->undo(); require(editor->toPlainText()==baseline && window.buildScript(),"Reduction Apply did not Undo exactly");
      editor->redo(); require(editor->toPlainText()==edited && window.buildScript(),"Reduction Apply did not Redo exactly"); checkSettings(Moderator::Instance()->GetConfiguredObject("Prop"));
      object=Moderator::Instance()->GetConfiguredObject("Prop"); const auto undoSteps=editor->document()->availableUndoSteps();
      require(!window.applyResourceChanges("Prop",{{"MaximumReduction","nan"}},edited).isEmpty() && editor->toPlainText()==edited && Moderator::Instance()->GetConfiguredObject("Prop")==object && editor->document()->availableUndoSteps()==undoSteps,"Invalid reduction changed source, configured model or Undo history");
      const auto saved=files.filePath("Bulirsch α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==edited,"Save/reopen changed the declared reduction input");
      panel=openPanel(window); minimum=panel->findChild<QLineEdit *>("resource_MinimumReduction"); maximum=panel->findChild<QLineEdit *>("resource_MaximumReduction");
      require(minimum && maximum && std::abs(minimum->text().toDouble()-0.6)<1e-15 && std::abs(maximum->text().toDouble()-0.0001)<1e-18 && !panel->hasChanges(),"Reopened form lost configured reductions");
      require(panelWindow(window)->close(),"Reopened clean form did not close"); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      require(window.runMission()==MainWindow::RunResult::Completed,"One short Bulirsch mission failed");
      checkSettings(Moderator::Instance()->GetInternalObject("Prop"));
      auto *command=Moderator::Instance()->GetFirstCommand(); while (command && command->GetTypeName()!="Propagate") command=command->GetNext();
      auto *propagate=dynamic_cast<Propagate *>(command); require(propagate,"Runtime propagation command missing"); checkSettings(propagate->GetClone(0));
      QFile result(report); require(result.open(QIODevice::ReadOnly),"Endpoint report missing");
      const auto rows=QString::fromUtf8(result.readAll()).trimmed().split('\n'); require(rows.size()==2,"Expected one explicit endpoint row");
      const auto values=rows.last().trimmed().split(QRegularExpression("\\s+")); require(values.size()==4,"Endpoint report columns incorrect");
      for (const auto &value:values) { bool valid=false; const auto number=value.toDouble(&valid); require(valid && std::isfinite(number),"Endpoint report contains nonfinite values"); }
      require(std::abs(values[0].toDouble()-60)<1e-6 && editor->toPlainText()==edited,"Run changed input or did not reach the declared 60 seconds");
      std::cout<<"PASS writable Bulirsch reductions pending/Apply, deprecated tolerance hidden, source preservation, nested PropSetup/direct/runtime/command clones, invalid rollback, exact Undo/Redo, Unicode save/reopen and one short finite endpoint report.\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
