#include "AcceptanceView.hpp"
#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include "PropSetup.hpp"
#include "Propagator.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTreeWidget>
#include <QTableWidget>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static void setField(QWidget *owner,const QString &name,const QString &value)
{
   if (auto *choice=owner->findChild<QComboBox *>("resource_"+name)) { require(choice->findText(value)>=0,"Requested propagator selector absent"); choice->setCurrentText(value); }
   else if (auto *line=owner->findChild<QLineEdit *>("resource_"+name)) line->setText(value);
   else throw std::runtime_error(("Missing propagator field "+name).toStdString());
}
static QMdiSubWindow *propWindow(MainWindow &window)
{
   for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList())
      if (child->property("resourceName").toString()=="DeepSpace") return child;
   throw std::runtime_error("Propagator settings window missing");
}
static ResourceEditor *propPanel(MainWindow &window)
{
   auto *panel=dynamic_cast<ResourceEditor *>(propWindow(window)->widget()); require(panel,"Propagator settings form missing"); return panel;
}
static void openPanel(MainWindow &window)
{
   auto *tree=window.findChild<QTreeWidget *>("Resources");
   const auto items=tree->findItems("DeepSpace",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Propagator tree item missing"); tree->itemDoubleClicked(items.front(),0);
   acceptanceView(window);
}
static void closePanel(MainWindow &window)
{
   require(propWindow(window)->close(),"Clean propagator form did not close"); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
static void checkProp(const char *type,const char *model,double maxStep)
{
   auto *prop=Moderator::Instance()->GetConfiguredObject("DeepSpace"); require(prop,"Propagator missing");
   const auto equal=[](double a,double b) { return std::abs(a-b)<=std::max(std::abs(b)*1e-13,1e-25); };
   if (prop->GetStringParameter("Type")!=type || prop->GetStringParameter("FM")!=model ||
       !equal(prop->GetRealParameter("InitialStepSize"),600) || !equal(prop->GetRealParameter("Accuracy"),1e-12) ||
       !equal(prop->GetRealParameter("MinStep"),0) || !equal(prop->GetRealParameter("MaxStep"),maxStep) ||
       prop->GetIntegerParameter("MaxStepAttempts")!=75 || prop->GetBooleanParameter("StopIfAccuracyIsViolated")) {
      std::cerr<<"Actual PropSetup Type="<<prop->GetStringParameter("Type")<<" FM="<<prop->GetStringParameter("FM")
         <<" InitialStepSize="<<prop->GetRealParameter("InitialStepSize")<<" Accuracy="<<prop->GetRealParameter("Accuracy")
         <<" MinStep="<<prop->GetRealParameter("MinStep")<<" MaxStep="<<prop->GetRealParameter("MaxStep")<<'\n';
      throw std::runtime_error("Propagator selectors or owned numeric settings were lost");
   }
}
static QString unrelatedSource(QString source)
{
   source.remove(QRegularExpression("^(?:GMAT[ \\t]+)?DeepSpace\\.(?:Type|FM|MaxStep)[ \\t]*=[ \\t]*[^;\\n]*;[ \\t]*\\n",QRegularExpression::MultilineOption)); return source;
}
static Propagator *ownedIntegrator()
{
   auto *setup=dynamic_cast<PropSetup *>(Moderator::Instance()->GetConfiguredObject("DeepSpace"));
   require(setup && setup->GetPropagator(),"Owned numerical integrator missing"); return setup->GetPropagator();
}
static QMap<QString,QString> integratorValues()
{
   auto *p=ownedIntegrator(); QMap<QString,QString> result{{"Type",QString::fromStdString(p->GetTypeName())}};
   for (const auto &field:resourceProperties(*p)) result.insert(field.name,field.value);
   return result;
}
static void editIntegrator(ResourceEditor *panel,const QString &field,const QString &value)
{
   auto *table=panel->findChild<QTableWidget *>("resourceProperties"); require(table,"Integrator property table missing");
   for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==field) { table->scrollToItem(table->item(row,0)); break; }
   QApplication::processEvents(); setField(panel,field,value);
}
static void integratorControlChecks(MainWindow &window,QPlainTextEdit &editor,const QString &directory)
{
   const QStringList methods{"RungeKutta89","RungeKutta68","RungeKutta56","PrinceDormand45","PrinceDormand78","AdamsBashforthMoulton","BulirschStoer"};
   const double mu=398600.4418,radius=7000,speed=std::sqrt(mu/radius),duration=600;
   QStringList rejected;
   for (const auto &method:methods) {
      for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (auto *panel=dynamic_cast<EditablePanel *>(child->widget())) { panel->discardChanges(); child->close(); }
      QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      const auto report=directory+"/circle-"+method+".txt";
      const QString mission="BeginMissionSequence; % preserve numerical mission\nPropagate DeepSpace(Vehicle) {Vehicle.ElapsedSecs = 600};\nReport Values Vehicle.ElapsedSecs Vehicle.X Vehicle.Y Vehicle.Z Vehicle.VX Vehicle.VY Vehicle.VZ;\n";
      const QString source="% NASA integrator controls: independent circular orbit\nEarth.Mu = 398600.4418;\nCreate Spacecraft Vehicle;\nVehicle.CoordinateSystem = EarthMJ2000Eq;\nVehicle.DisplayStateType = Cartesian;\nVehicle.X = 7000;\nVehicle.Y = 0;\nVehicle.Z = 0;\nVehicle.VX = 0;\nVehicle.VY = "+QString::number(speed,'g',17)+";\nVehicle.VZ = 0;\nCreate ForceModel Gravity;\nGravity.CentralBody = Earth;\nGravity.PrimaryBodies = {};\nGravity.PointMasses = {Earth};\nGravity.Drag = None;\nGravity.SRP = Off;\nCreate Propagator DeepSpace;\nDeepSpace.FM = Gravity;\nDeepSpace.Type = RungeKutta89;\nDeepSpace.InitialStepSize = 20;\nDeepSpace.Accuracy = 1e-9;\nDeepSpace.MinStep = 0;\nDeepSpace.MaxStep = 60;\nDeepSpace.MaxStepAttempts = 50;\nDeepSpace.StopIfAccuracyIsViolated = false;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.WriteReport = false;\nValues.Precision = 17;\n"+mission;
      editor.setPlainText(source); require(window.buildScript(),"Integrator circle fixture did not build");
      openPanel(window); auto *panel=propPanel(window); editIntegrator(panel,"Type",method); panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      require(ownedIntegrator()->GetTypeName()==method.toStdString(),qPrintable("Selected integrator did not apply: "+method)); closePanel(window);
      const auto before=editor.toPlainText(); const auto beforeValues=integratorValues(); openPanel(window); panel=propPanel(window);
      const QMap<QString,QString> common{{"InitialStepSize","10"},{"Accuracy","1e-12"},{"MinStep","0"},{"MaxStep","30"},{"MaxStepAttempts","80"},{"StopIfAccuracyIsViolated","true"}};
      QMap<QString,QString> settings=common;
      if (method=="AdamsBashforthMoulton") { settings.insert("LowerError","1e-16"); settings.insert("TargetError","1e-14"); }
      if (method=="BulirschStoer") { settings.insert("MinimumReduction","0.6"); settings.insert("MaximumReduction","0.0001"); }
      for (auto it=settings.cbegin();it!=settings.cend();++it) editIntegrator(panel,it.key(),it.value());
      require(panel->hasChanges() && editor.toPlainText()==before && integratorValues()==beforeValues,"Pending integrator controls escaped to source/model");
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); require(!propPanel(window)->hasChanges(),"Valid integrator controls were not applied");
      const auto after=editor.toPlainText(); const auto applied=integratorValues();
      for (auto it=settings.cbegin();it!=settings.cend();++it) {
         require(applied.contains(it.key()),"Applied integrator property disappeared");
         require(it.key()=="StopIfAccuracyIsViolated" ? applied.value(it.key())==it.value() : applied.value(it.key()).toDouble()==it.value().toDouble(),qPrintable("Integrator value not retained: "+method+"."+it.key()));
      }
      require(after.endsWith(mission),"Integrator controls changed unrelated mission source"); closePanel(window);
      editor.undo(); require(editor.toPlainText()==before && window.buildScript() && integratorValues()==beforeValues,"Integrator Undo did not restore exact source/settings");
      editor.redo(); require(editor.toPlainText()==after && window.buildScript() && integratorValues()==applied,"Integrator Redo did not restore exact source/settings");
      const auto saved=directory+"/integrator-"+method+".script"; require(window.saveScriptTo(saved) && window.loadScript(saved) && editor.toPlainText()==after && integratorValues()==applied,"Integrator save/reopen lost settings");
      // Move both bounds above the old maximum, then below the old minimum.
      // Each pair is valid as a transaction, regardless of setter ordering.
      for (const auto &bounds:QList<QPair<QString,QString>>{{"40","50"},{"1","20"},{"0","30"}}) {
         openPanel(window); panel=propPanel(window); editIntegrator(panel,"MinStep",bounds.first); editIntegrator(panel,"MaxStep",bounds.second);
         panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
         require(!propPanel(window)->hasChanges() && ownedIntegrator()->GetRealParameter("MinStep")==bounds.first.toDouble() && ownedIntegrator()->GetRealParameter("MaxStep")==bounds.second.toDouble(),"Valid paired step bounds were rejected or changed");
         closePanel(window);
      }
      QFile::remove(report); require(!QFileInfo::exists(report) && window.runMission()==MainWindow::RunResult::Completed,qPrintable("Circular orbit run failed: "+method));
      QFile output(report); require(output.open(QIODevice::ReadOnly),"Circular orbit report absent"); const auto columns=QString::fromUtf8(output.readAll()).trimmed().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(columns.size()==7,"Circular orbit report has wrong shape");
      require(std::abs(columns[0].toDouble()-duration)<1e-6,"Integrator did not reach the specified duration");
      const double angle=std::sqrt(mu/(radius*radius*radius))*duration;
      const QVector<double> truth{radius*std::cos(angle),radius*std::sin(angle),0,-speed*std::sin(angle),speed*std::cos(angle),0};
      for (int i=0;i<6;++i) { bool ok=false; const double value=columns[i+1].toDouble(&ok); require(ok && std::isfinite(value) && std::abs(value-truth[i])<(i<3 ? 2e-5 : 2e-8),qPrintable("Analytic circular orbit mismatch: "+method+" component "+QString::number(i))); }
      auto *runtime=dynamic_cast<PropSetup *>(Moderator::Instance()->GetInternalObject("DeepSpace")); require(runtime && runtime->GetPropagator()->GetTypeName()==method.toStdString(),"Mission did not use the selected integrator");
      const auto validSource=editor.toPlainText(); const auto validValues=integratorValues(); auto steps=editor.document()->availableUndoSteps();
      openPanel(window); panel=propPanel(window);
      QMap<QString,QStringList> invalid{{"InitialStepSize",{"","not-a-number","nan","inf","0","0.00001"}},{"Accuracy",{"","nan","inf","0","-1"}},{"MinStep",{"","nan","inf","-1","100"}},{"MaxStep",{"","nan","inf","0","-1"}},{"MaxStepAttempts",{"","oops","0","-1","1.5"}}};
      if (method=="AdamsBashforthMoulton") { invalid.insert("LowerError",{"0","-1","1e-14"}); invalid.insert("TargetError",{"0","-1","1e-16","1e-12"}); }
      for (auto it=invalid.cbegin();it!=invalid.cend();++it) for (const auto &bad:it.value()) {
         editIntegrator(panel,it.key(),bad); panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); panel=propPanel(window);
         const bool safe=panel->hasChanges() && editor.toPlainText()==validSource && editor.document()->availableUndoSteps()==steps && integratorValues()==validValues && !panel->findChild<QLabel *>("resourceStatus")->text().isEmpty();
         if (!safe) {
            rejected.append(method+"."+it.key()+"="+bad); panel->discardChanges(); closePanel(window); editor.setPlainText(validSource); require(window.buildScript(),"Could not restore integrator after rejected-check failure"); steps=editor.document()->availableUndoSteps(); openPanel(window); panel=propPanel(window);
         } else editIntegrator(panel,it.key(),validValues.value(it.key()));
      }
      panel->discardChanges(); closePanel(window);
      std::cout<<"Integrator analytic/control case completed: "<<method.toStdString()<<'\n';
   }
   require(rejected.isEmpty(),qPrintable("Invalid integrator input was not rejected atomically: "+rejected.join(", ")));
   for (const auto &method:methods) {
      const QJsonObject proof{{"checkpoint","integrator."+method},{"type",method},{"outcome","passed"},{"facets",QJsonArray{"gui-settings","pending-isolation","invalid-input","dependent-ranges","undo-redo","save-reopen","runtime-method","analytic-orbit"}}};
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
   }
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("PropSetupApply");
   if (argc!=2 || !acceptanceWidgetPlatform()) return 2;
   try {
      TestSettings isolated; QTemporaryDir files; require(files.isValid(),"Temporary files unavailable");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; require(window.initialize(startup),"Propagator runtime initialization failed"); window.show();
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); require(editor,"Script editor missing");
      const QString baseline="% Preserve source α and independent resources\nCreate Spacecraft Vehicle;\nCreate ForceModel DeepSpace_ForceModel;\nDeepSpace_ForceModel.PrimaryBodies = {};\nDeepSpace_ForceModel.PointMasses = {Earth};\nCreate Propagator DeepSpace;\nDeepSpace.InitialStepSize = 600; % retain exact input\nDeepSpace.Accuracy = 1e-12; % retain accuracy spelling\nDeepSpace.MinStep = 0;\nDeepSpace.MaxStep = 864000;\nDeepSpace.MaxStepAttempts = 75;\nDeepSpace.StopIfAccuracyIsViolated = false;\nCreate Variable Keep;\nKeep = 7; % preserve initializer\nBeginMissionSequence;\nKeep = Keep + 2; % preserve mission\n";
      editor->setPlainText(baseline); require(window.buildScript(),"Propagator baseline failed");
      const auto originalModel=Moderator::Instance()->GetConfiguredObject("DeepSpace")->GetStringParameter("FM");
      checkProp("RungeKutta89",originalModel.c_str(),864000);
      openPanel(window); auto *panel=propPanel(window);
      auto *type=panel->findChild<QComboBox *>("resource_Type"); auto *model=panel->findChild<QLineEdit *>("resource_FM");
      const auto fields=resourceProperties(*Moderator::Instance()->GetConfiguredObject("DeepSpace"));
      const auto fm=std::find_if(fields.cbegin(),fields.cend(),[](const auto &field) { return field.name=="FM"; });
      require(type && type->findText("PrinceDormand78")>=0 && model && fm!=fields.cend() && fm->references.contains("DeepSpace_ForceModel") && !fm->references.contains("Vehicle"),"Propagator factory and force-model choices incorrect");
      setField(panel,"Type","PrinceDormand78"); setField(panel,"FM","DeepSpace_ForceModel"); setField(panel,"MaxStep","720000");
      require(panel->hasChanges() && editor->toPlainText()==baseline,"Pending propagator fields committed before Apply");
      panel->requestApply(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
      const auto edited=editor->toPlainText();
      if (edited==baseline) std::cerr<<propPanel(window)->findChild<QLabel *>("resourceStatus")->text().toStdString()<<'\n';
      require(edited!=baseline && !propPanel(window)->hasChanges(),"Propagator Apply did not commit a clean form");
      checkProp("PrinceDormand78","DeepSpace_ForceModel",720000);
      require(edited.contains("DeepSpace.Type = PrinceDormand78;") && edited.contains("DeepSpace.FM = DeepSpace_ForceModel;"),"Propagator Apply omitted Type or FM from source");
      require(edited.indexOf("DeepSpace.Type")<edited.indexOf("DeepSpace.InitialStepSize"),"Changed Type follows owned numeric settings and would reset them");
      require(unrelatedSource(edited)==unrelatedSource(baseline),"Propagator Apply changed unrelated source or unchanged owned settings");
      const auto undoSteps=editor->document()->availableUndoSteps(); auto *original=Moderator::Instance()->GetConfiguredObject("DeepSpace");
      require(!window.applyResourceChanges("DeepSpace",{{"Type","MissingPropagator"}},edited).isEmpty() &&
         !window.applyResourceChanges("DeepSpace",{{"FM","Vehicle"}},edited).isEmpty() && editor->toPlainText()==edited &&
         Moderator::Instance()->GetConfiguredObject("DeepSpace")==original && editor->document()->availableUndoSteps()==undoSteps,"Invalid selector changed source, model or Undo history");
      closePanel(window); editor->undo(); require(editor->toPlainText()==baseline && window.buildScript(),"Propagator Apply was not one exact Undo"); checkProp("RungeKutta89",originalModel.c_str(),864000);
      editor->redo(); require(editor->toPlainText()==edited && window.buildScript(),"Propagator Apply Redo failed"); checkProp("PrinceDormand78","DeepSpace_ForceModel",720000);
      // Rebuilding for an unrelated creation reproduced the lost selectors in
      // the actual GUI. Assert source and reopen controls retain both choices.
      require(window.createResource("GroundStation","Station",edited).isEmpty(),"Unrelated resource creation failed");
      const auto withStation=editor->toPlainText(); require(withStation=="Create GroundStation Station;\n"+edited,"Unrelated creation rewrote propagator source"); checkProp("PrinceDormand78","DeepSpace_ForceModel",720000);
      openPanel(window); panel=propPanel(window);
      require(panel->findChild<QComboBox *>("resource_Type")->currentText()=="PrinceDormand78" && panel->findChild<QLineEdit *>("resource_FM")->text()=="DeepSpace_ForceModel" && !panel->hasChanges(),"Reopened propagator form lost Type or FM"); closePanel(window);
      const auto saved=files.filePath("propagator α.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && editor->toPlainText()==withStation,"Propagator Unicode save/reopen changed source"); checkProp("PrinceDormand78","DeepSpace_ForceModel",720000);
      std::cout<<"PASS pending PropSetup factory/force-model choices, mixed Type/FM/MaxStep Apply, unchanged step/accuracy and source retained, invalid-selector rollback, exact Undo/Redo, unrelated creation/rebuild, reopened controls and Unicode save/reopen; no mission numerical or native desktop run.\n";
      integratorControlChecks(window,*editor,files.path());
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; }
   catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
}
