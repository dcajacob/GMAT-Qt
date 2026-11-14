#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "UserParameter.hpp"
#include "Moderator.hpp"
#include "ODEModel.hpp"
#include "PropSetup.hpp"
#include "PhysicalModel.hpp"
#include "CelestialBody.hpp"
#include "AcceptanceView.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QListWidget>
#include <QScrollArea>
#include <QSet>
#include <QTabWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
static void require(bool condition,const char *message) { if (!condition) throw std::runtime_error(message); }
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Cannot read gravity report"); return QString::fromUtf8(file.readAll()); }
static QTableWidgetItem *field(ResourceEditor &panel,const QString &name)
{
   auto *table=panel.findChild<QTableWidget *>(); require(table,"Property table missing");
   for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()==name) { require(table->item(row,1),"Text property has no value item"); return table->item(row,1); }
   throw std::runtime_error(("Missing property "+name).toStdString());
}
static void control(ResourceEditor &panel,const QString &value)
{
   auto *table=panel.findChild<QTableWidget *>(); for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="ErrorControl") {
      auto *choice=qobject_cast<QComboBox *>(table->cellWidget(row,1)); require(choice && choice->findText(value)>=0,"Error control choice missing"); choice->setCurrentText(value); return;
   }
   throw std::runtime_error("Error control missing");
}
static void atmosphere(ResourceEditor &panel,const QString &model)
{
   std::exception_ptr failure; QTimer::singleShot(0,&panel,[&] {
      auto *dialog=panel.findChild<QDialog *>("atmosphereDialog"); try {
         require(dialog,"Atmosphere dialog missing"); dialog->findChild<QComboBox *>("atmosphere_AtmosphereModel")->setCurrentText(model);
         dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); if (dialog->result()!=QDialog::Accepted) throw std::runtime_error("Atmosphere choice rejected: "+dialog->findChild<QLabel *>("atmosphereError")->text().toStdString());
      } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
   }); panel.findChild<QPushButton *>("forceAtmosphere")->click(); if (failure) std::rethrow_exception(failure);
}
// Exercise the displayed list chooser, not the hidden backing-table items.
static void chooseBodies(ResourceEditor &panel,const QString &property,const QStringList &names,bool cancel=false)
{
   auto *value=panel.findChild<QLineEdit *>("resource_"+property); require(value,"Body-list control missing");
   for (auto *page=value->parentWidget();page;page=page->parentWidget())
      for (auto *tabs:panel.findChildren<QTabWidget *>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page);
   for (auto *parent=value->parentWidget();parent;parent=parent->parentWidget())
      if (auto *scroll=qobject_cast<QScrollArea *>(parent)) scroll->ensureWidgetVisible(value);
   QApplication::processEvents(); require(value->isVisible(),"Body-list control is hidden");
   const auto original=value->text();
   const auto buttons=value->parentWidget()->findChildren<QPushButton *>(); require(buttons.size()==1 && buttons[0]->isVisible(),"Visible body chooser missing");
   std::exception_ptr failure; QTimer::singleShot(0,&panel,[&] {
      auto *dialog=panel.findChild<QDialog *>("resourceSelectionDialog");
      try {
         require(dialog,"Body chooser did not open"); acceptanceView(*dialog);
         auto *list=dialog->findChild<QListWidget *>("resourceSelectionList"); require(list && list->isVisible(),"Body chooser list is hidden");
         auto *clear=dialog->findChild<QPushButton *>("resourceClearSelection"); require(clear && clear->isVisible(),"Clear selection action missing"); clear->click();
         for (const auto &name:names) {
            const auto matches=list->findItems(name,Qt::MatchExactly); require(matches.size()==1,"Requested celestial body missing from chooser");
            list->scrollToItem(matches[0]); matches[0]->setCheckState(Qt::Checked); QApplication::processEvents();
         }
         auto *box=dialog->findChild<QDialogButtonBox *>(); require(box,"Body chooser buttons missing");
         box->button(cancel ? QDialogButtonBox::Cancel : QDialogButtonBox::Ok)->click();
      } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); }
   });
   buttons[0]->click(); if (failure) std::rethrow_exception(failure);
   if (cancel) require(value->text()==original,"Cancelled body chooser changed pending text");
   else { auto selected=value->text().split(',',Qt::SkipEmptyParts); for (auto &name:selected) name=name.trimmed(); require(QSet<QString>(selected.begin(),selected.end())==QSet<QString>(names.begin(),names.end()),"Body chooser committed the wrong selection"); }
}
static ODEModel *runtimeForces()
{
   auto *prop=dynamic_cast<PropSetup *>(Moderator::Instance()->GetInternalObject("Prop"));
   require(prop && prop->GetODEModel(),"Propagator runtime force model missing"); return prop->GetODEModel();
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtGravityBodies");
   try {
      require(argc==2,"Startup argument required"); TestSettings settings; QTemporaryDir files; require(files.isValid(),"Temporary directory unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Initialization failed"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); const auto report=files.filePath("states.txt");
      const QString fixed="  GMAT FM.SRP = On; % untouched SRP spelling\nFM.SRP.Flux = 1400; % untouched flux\nFM.RelativisticCorrection = On; % untouched correction\n";
      const QString mission="BeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 600}; % retained body mission α\nReport Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ;\n";
      auto fixture=[&](bool primary,const QString &points,int degree,bool drag,const QString &error) {
         QString gravity="FM.PrimaryBodies = {"+QString(primary ? "Earth" : "")+"}; % primary creator\nFM.PointMasses = {"+points+"}; % point creator\n";
         if (primary) gravity+="FM.GravityField.Earth.Degree = "+QString::number(degree)+"; % gravity degree\nFM.GravityField.Earth.Order = "+QString::number(degree==4 ? 4 : 0)+"; % gravity order\n";
         return "Create Spacecraft Sat;\nSat.DateFormat = UTCGregorian;\nSat.Epoch = '01 Jan 2024 12:00:00.000';\nSat.X = 6778.1363;\nSat.Y = 0;\nSat.Z = 0;\nSat.VX = 0;\nSat.VY = 7.668;\nSat.VZ = 0;\nCreate ForceModel FM;\n"+gravity+fixed+"FM.Drag.AtmosphereModel = "+QString(drag ? "NRLMSISE00" : "None")+"; % drag creator\nFM.ErrorControl = "+error+"; % mixed comment\nCreate Propagator Prop;\nProp.FM = FM;\nProp.Accuracy = 1e-13;\nProp.InitialStepSize = 10;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\n"+mission;
      };
      auto run=[&] { if (window.runMission()!=MainWindow::RunResult::Completed) { app.processEvents(); throw std::runtime_error(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()); } return read(report); };
      auto reference=[&](bool primary,const QString &points,int degree,bool drag,const QString &error) { editor->setPlainText(fixture(primary,points,degree,drag,error)); return run(); };
      const auto changedPoints=reference(true,"Mars",0,true,"LargestStep"),exchanged=reference(false,"Earth, Mars",0,false,"LargestStep"),created=reference(true,"Mars",4,true,"LargestStep"),empty=reference(true,"",4,true,"LargestStep");
      auto compare=[&](const QString &expected) {
         const auto actual=run().split(QRegularExpression("\\s+"),Qt::SkipEmptyParts),wanted=expected.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts); require(actual.size()==6 && wanted.size()==6,"Gravity state report incomplete");
         for (int i=0;i<6;++i) { bool valid=false; const auto a=actual[i].toDouble(&valid),b=wanted[i].toDouble(); require(valid && std::isfinite(a) && std::abs(a-b)<1e-11*std::max(1.0,std::abs(b)),"GUI gravity result differs from independent script"); }
      };
      auto open=[&] {
         auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto matches=tree->findItems("FM",Qt::MatchExactly|Qt::MatchRecursive); require(matches.size()==1,"Force model resource missing"); tree->itemDoubleClicked(matches.first(),0);
         for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("resourceName").toString()=="FM") return child;
         throw std::runtime_error("Actual force panel missing");
      };
      auto finish=[&](QMdiSubWindow &child,ResourceEditor *previous,const QString &source) {
         const auto applied=editor->toPlainText(); if (applied==source) { app.processEvents(); throw std::runtime_error("Body Apply failed: "+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()); }
         require(child.isVisible() && child.widget()!=previous && !dynamic_cast<EditablePanel *>(child.widget())->hasChanges(),"Body Apply did not refresh retained panel");
         require(applied.contains(fixed) && applied.endsWith(mission) && applied.contains("% primary creator") && applied.contains("% point creator") && applied.contains("% mixed comment"),"Body change rewrote unrelated source/comments/mission");
         require(!applied.contains("FM.PolyhedralBodies") && !applied.contains("FM.UserDefined") && !applied.contains("FM.External"),"Body change made other defaults explicit");
         editor->undo(); require(editor->toPlainText()==source,"Mixed body Undo was not exact"); editor->redo(); require(editor->toPlainText()==applied,"Mixed body Redo was not exact");
         const auto saved=files.filePath("Bodies "+QString::number(files.path().size())+" Δ.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Body Unicode save/reopen failed");
      };
      editor->setPlainText(fixture(true,"Luna",0,true,"RSSStep")); require(window.buildScript(),"Body fixture failed");
      {
         const auto source=editor->toPlainText(); auto *child=open(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); field(*panel,"PointMasses")->setText("Mars"); control(*panel,"LargestStep");
         require(panel->hasChanges() && editor->toPlainText()==source,"Mixed point-mass change bypassed pending Apply"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); finish(*child,panel,source);
      } compare(changedPoints);
      auto legacyDegree=editor->toPlainText(); legacyDegree.replace("FM.GravityField.Earth.Degree = 0;","FM.Degree = 0;"); editor->setPlainText(legacyDegree); require(window.buildScript(),"Legacy gravity leaf fixture failed");
      {
         const auto source=editor->toPlainText(); auto *child=open(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); field(*panel,"PrimaryBodies")->setText(""); field(*panel,"PointMasses")->setText("Earth, Mars");
         auto *apply=panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply); apply->click();
         require(child->widget()==panel && panel->hasChanges() && editor->toPlainText()==source,"Unsupported drag/primary removal changed source or lost pending values");
         atmosphere(*panel,"None"); apply->click(); finish(*child,panel,source); require(!editor->toPlainText().contains("FM.GravityField.Earth.") && !editor->toPlainText().contains("FM.Degree ="),"Removed primary retained owned/legacy gravity assignments");
      } compare(exchanged);
      auto withoutPrimary=editor->toPlainText(); withoutPrimary.replace(QRegularExpression("^[ \\t]*(?:GMAT[ \\t]+)?FM\\.PrimaryBodies[ \\t]*=[^;]*;[ \\t]*",QRegularExpression::MultilineOption),"");
      editor->setPlainText(withoutPrimary); require(window.buildScript(),"Implicit empty primary fixture failed");
      {
         const auto source=editor->toPlainText(); auto *child=open(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); field(*panel,"PrimaryBodies")->setText("Earth"); field(*panel,"PointMasses")->setText("Mars"); atmosphere(*panel,"NRLMSISE00");
         panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); finish(*child,panel,source);
      } compare(created);
      {
         auto *forces=runtimeForces(); int count=0;
         for (int i=0;i<forces->GetNumForces();++i) if (auto *force=forces->GetForce(i);force && force->IsOfType("GravityField")) {
            ++count; require(force->GetBodyName()=="Earth" && force->GetIntegerParameter("Degree")==4 && force->GetIntegerParameter("Order")==4,"Runtime nonspherical Earth model differs from GUI selection");
         }
         require(count==1,"Expected exactly one nonspherical Earth runtime force");
         const QJsonObject proof{{"checkpoint","gravity-body.legacy-earth"},{"outcome","passed"},{"body","Earth"},{"degree",4},{"order",4},
            {"facets",QJsonArray{"gui-primary-selection","runtime-force-readback","script-reference-trajectory","pending-apply","exact-undo-redo","unicode-save-reopen"}}};
         std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      }
      {
         const auto source=editor->toPlainText(); auto *child=open(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); field(*panel,"PointMasses")->setText("");
         panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); finish(*child,panel,source);
      } compare(empty);
      const auto stable=editor->toPlainText(); require(!window.applyResourceChanges("FM",{{"PointMasses","Earth"},{"ErrorControl","LargestState"}},stable).isEmpty() && editor->toPlainText()==stable,"Overlapping mixed body selection was accepted");
      require(!window.applyResourceChanges("FM",{{"PointMasses","MissingBody"},{"ErrorControl","LargestState"}},stable).isEmpty() && editor->toPlainText()==stable,"Unknown mixed body selection was accepted");
      // Legacy/missing selectors must precede retained force-dependent data.
      const QString before="Create ForceModel FM;\nFM.PrimaryBodies = {Earth};\nFM.PointMasses = {Luna};\nFM.GravityField.Earth.Degree = 0;\nFM.ErrorControl = RSSStep;\n";
      auto after=before; after.replace("PointMasses = {Luna}","PointMasses = {Mars}").replace("RSSStep","LargestStep");
      const auto legacy=patchResourceConfiguration("Create ForceModel FM;\nFM.Gravity = {Earth}; % legacy root\nFM.PointMasses = ... % continued root\n{Luna};\nFM.GravityField.Earth.Degree = 0; % exact dependent\nBeginMissionSequence;\n", "FM",before,after,{},true);
      require(legacy.contains("FM.Gravity = {Earth}; % legacy root") && legacy.contains("% continued root") && legacy.contains("FM.GravityField.Earth.Degree = 0; % exact dependent"),"Body selector rewrote unchanged legacy/dependent source");
      const auto removed=patchResourceConfiguration("Create ForceModel FM;\nFM.Gravity = {Earth}; % legacy root\nFM.GravityField.Earth.Degree = 0; % removed field comment\nPropagate P(S);\nFM.PrimaryBodies = {Earth};\n", "FM",before,"Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Luna};\nFM.ErrorControl = LargestStep;\n","Propagate P(S);",true);
      require(removed.contains("FM.Gravity = {};") && removed.contains("% legacy root") && removed.contains("% removed field comment") && !removed.contains("FM.GravityField.Earth.Degree") && removed.endsWith("Propagate P(S);\nFM.PrimaryBodies = {Earth};\n"),"Changed legacy primary or implicit mission removal mapping failed");
      // NASA's named major planets and Pluto must all be selectable and reach
      // the running propagator. The script comparison shares GMAT's physics;
      // this checks GUI configuration fidelity, not independent force accuracy.
      {
         const auto expected=reference(true,"Mars",4,false,"LargestStep");
         editor->setPlainText(fixture(false,"Mars",0,false,"RSSStep")); require(window.buildScript(),"Default-primary selection fixture failed");
         const auto source=editor->toPlainText(); auto *child=open(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"Primary force panel missing"); acceptanceView(window);
         chooseBodies(*panel,"PrimaryBodies",{"Earth"},true); require(!panel->hasChanges() && editor->toPlainText()==source,"Cancelled primary selection changed model or source");
         chooseBodies(*panel,"PrimaryBodies",{"Earth"});
         auto *mode=panel->findChild<QComboBox *>("resource_ErrorControl"); require(mode && mode->isVisible(),"Visible primary ErrorControl selector missing"); mode->setCurrentText("LargestStep");
         require(panel->hasChanges() && editor->toPlainText()==source,"Primary selection bypassed pending Apply");
         panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); finish(*child,panel,source); compare(expected);
         auto *forces=runtimeForces(); int count=0;
         for (int i=0;i<forces->GetNumForces();++i) if (auto *force=forces->GetForce(i);force && force->IsOfType("GravityField")) {
            ++count; require(force->GetBodyName()=="Earth" && force->GetIntegerParameter("Degree")==4 && force->GetIntegerParameter("Order")==4,"Runtime default Earth model differs from GUI selection");
         }
         require(count==1,"Expected exactly one default Earth runtime force");
         const QJsonObject proof{{"checkpoint","gravity-body.default-earth"},{"outcome","passed"},{"body","Earth"},{"degree",4},{"order",4},
            {"facets",QJsonArray{"visible-list-chooser","cancel-preserves-pending","runtime-force-readback","script-reference-trajectory","pending-apply","exact-undo-redo","unicode-save-reopen"}}};
         std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      }
      for (const auto &body:QStringList{"Mercury","Venus","Earth","Mars","Jupiter","Saturn","Uranus","Neptune","Pluto"}) {
         const auto names=body=="Earth" ? QStringList{"Earth"} : QStringList{"Earth",body};
         const auto expected=reference(false,names.join(", "),0,false,"LargestStep");
         editor->setPlainText(fixture(false,"Earth, Luna",0,false,"RSSStep")); require(window.buildScript(),"Planet selection fixture failed");
         const auto source=editor->toPlainText(); auto *child=open(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"Planet force panel missing"); acceptanceView(window);
         chooseBodies(*panel,"PointMasses",names,true); require(!panel->hasChanges() && editor->toPlainText()==source,"Cancelled planet selection changed model or source");
         chooseBodies(*panel,"PointMasses",names);
         auto *mode=panel->findChild<QComboBox *>("resource_ErrorControl"); require(mode && mode->isVisible(),"Visible ErrorControl selector missing"); mode->setCurrentText("LargestStep");
         require(panel->hasChanges() && editor->toPlainText()==source,"Planet selection bypassed pending Apply");
         panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); finish(*child,panel,source); compare(expected);
         QSet<QString> actual;
         auto *forces=runtimeForces();
         for (int i=0;i<forces->GetNumForces();++i) if (auto *force=forces->GetForce(i);force) {
            require(!force->IsOfType("GravityField"),"Point-mass model retained nonspherical gravity");
            if (force->IsOfType("PointMassForce")) { const auto name=QString::fromStdString(force->GetBodyName()); require(!actual.contains(name),"Duplicate runtime point-mass body"); actual.insert(name); }
         }
         require(actual==QSet<QString>(names.begin(),names.end()),"Runtime point masses differ from GUI selection");
         const QJsonObject proof{{"checkpoint","gravity-body.point-mass-"+body},{"outcome","passed"},{"body",body},
            {"facets",QJsonArray{"visible-list-chooser","cancel-preserves-pending","runtime-force-readback","script-reference-trajectory","pending-apply","exact-undo-redo","unicode-save-reopen"}}};
         std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      }
      {
         const auto kernel=QFileInfo(startup).absolutePath()+"/../data/planetary_ephem/spk/ceres_1900_2100.bsp";
         require(QFileInfo(kernel).isFile(),"User-body SPICE fixture missing");
         const QString definition="Create Asteroid Ceres;\nCeres.NAIFId = 2000001;\nCeres.OrbitSpiceKernelName = {'"+kernel+"'};\nCeres.Mu = 63;\nCeres.EquatorialRadius = 470;\n";
         editor->setPlainText(definition+fixture(false,"Earth, Ceres",0,false,"LargestStep")); const auto expected=run();
         editor->setPlainText(definition+fixture(false,"Earth, Luna",0,false,"RSSStep")); require(window.buildScript(),"User-body force selection fixture failed");
         const auto source=editor->toPlainText(); auto *child=open(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"User-body force panel missing"); acceptanceView(window);
         chooseBodies(*panel,"PointMasses",{"Earth","Ceres"},true); require(!panel->hasChanges() && editor->toPlainText()==source,"Cancelled user-body selection changed model or source");
         chooseBodies(*panel,"PointMasses",{"Earth","Ceres"});
         auto *mode=panel->findChild<QComboBox *>("resource_ErrorControl"); require(mode && mode->isVisible(),"Visible user-body ErrorControl selector missing"); mode->setCurrentText("LargestStep");
         require(panel->hasChanges() && editor->toPlainText()==source,"User-body selection bypassed pending Apply");
         panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); finish(*child,panel,source); compare(expected);
         require(editor->toPlainText().startsWith(definition),"Force selection altered the user-body definition");
         auto *body=dynamic_cast<CelestialBody *>(Moderator::Instance()->GetConfiguredObject("Ceres")); require(body && body->IsUserDefined(),"Selected Ceres is not the configured user body");
         QSet<QString> actual; auto *forces=runtimeForces();
         for (int i=0;i<forces->GetNumForces();++i) if (auto *force=forces->GetForce(i);force) {
            require(!force->IsOfType("GravityField"),"User point-mass model retained harmonic gravity");
            if (force->IsOfType("PointMassForce")) { const auto name=QString::fromStdString(force->GetBodyName()); require(!actual.contains(name),"Duplicate user-body point mass"); actual.insert(name); }
         }
         require(actual==QSet<QString>{"Earth","Ceres"},"Runtime user-body point masses differ from GUI selection");
         const QJsonObject proof{{"checkpoint","gravity-body.user-point-mass"},{"outcome","passed"},{"body","Ceres"},{"user_defined",true},
            {"facets",QJsonArray{"visible-list-chooser","cancel-preserves-pending","runtime-force-readback","user-body-preserved","script-reference-trajectory","pending-apply","exact-undo-redo","unicode-save-reopen"}}};
         std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      }
      std::cout<<"PASS: actual MDI mixed point-mass edits, primary/point exchange, invalid drag-dependent removal and correction, re-created primary plus drag, empty point list, overlapping/unknown body rejection, retained panels, unrelated source/defaults/comments, exact Undo/Redo/Unicode save/reopen and independent six-state reports.\n"; return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
     catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
