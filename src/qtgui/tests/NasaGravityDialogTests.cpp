#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "GravityDialog.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "FileManager.hpp"
#include "ODEModel.hpp"
#include "PhysicalModel.hpp"
#include "PropSetup.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTimer>
#include <QTreeWidget>
#include <exception>
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void events() { QApplication::processEvents(); QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QMap<QString,QString> properties()
{
   auto *model=Moderator::Instance()->GetConfiguredObject("Gravity"); require(model,"Gravity model missing");
   QMap<QString,QString> result;for (const auto &property:resourceProperties(*model)) result[property.name]=property.value;return result;
}
static QComboBox *choice(QDialog &dialog,const QString &name)
{
   auto *control=dialog.findChild<QComboBox *>("gravity_"+name);require(control && control->isVisible(),qPrintable("Gravity choice missing: "+name));return control;
}
static QLineEdit *entry(QDialog &dialog,const QString &name)
{
   auto *control=dialog.findChild<QLineEdit *>("gravity_"+name);require(control && control->isVisible(),qPrintable("Gravity entry missing: "+name));return control;
}
static void select(QDialog &dialog,const QString &name,const QString &value)
{
   auto *control=choice(dialog,name);require(control->isEnabled() && control->findText(value)>=0,qPrintable("Gravity choice unavailable: "+name+" = "+value));control->setCurrentText(value);events();
}
static void set(QDialog &dialog,const QString &name,const QString &value)
{
   auto *control=entry(dialog,name);require(control->isEnabled(),qPrintable("Gravity entry disabled: "+name));control->setText(value);events();
}
static void ok(QDialog &dialog)
{
   auto *buttons=dialog.findChild<QDialogButtonBox *>("gravityButtons");require(buttons,"Gravity buttons missing");buttons->button(QDialogButtonBox::Ok)->click();events();
}
static void rejected(QDialog &dialog,const QString &input={})
{
   ok(dialog);auto *diagnostic=dialog.findChild<QLabel *>("gravityError");
   require(dialog.isVisible() && diagnostic && !diagnostic->text().isEmpty(),"Invalid gravity dialog input accepted or lacks diagnostic");
   if (!input.isEmpty()) require(entry(dialog,"TideFile")->text()==input,"Rejected tide path was not retained");
}
static void browse(QDialog &dialog,const QString &name,const QString &filename,bool accept)
{
   const auto initial=entry(dialog,name)->text();std::exception_ptr failure;bool visited=false;
   QTimer::singleShot(0,&dialog,[&] {
      auto *picker=dialog.findChild<QFileDialog *>("gravityFileDialog");
      try {
         require(picker && picker->fileMode()==QFileDialog::ExistingFile,"Gravity picker does not require an existing file");acceptanceView(*picker);visited=true;
         if (!accept) {picker->reject();return;}
         picker->selectFile(filename);auto *path=picker->findChild<QLineEdit *>("fileNameEdit");require(path && path->isVisible(),"Gravity picker filename entry missing");path->setText(filename);events();
         auto *buttons=picker->findChild<QDialogButtonBox *>();auto *open=buttons ? buttons->button(QDialogButtonBox::Open) : nullptr;require(open && open->isEnabled(),"Gravity picker Open unavailable");open->click();
         require(picker->result()==QDialog::Accepted,"Gravity picker did not accept the selected file");
      } catch (...) {failure=std::current_exception();if (picker) picker->reject();}
   });
   auto *button=dialog.findChild<QPushButton *>("gravityBrowse_"+name);require(button && button->isEnabled(),"Gravity file Browse unavailable");button->click();if (failure) std::rethrow_exception(failure);
   require(visited && entry(dialog,name)->text()==(accept ? filename : initial),"Gravity picker selection/Cancel changed the wrong value");
}
static void proof(const QString &name,const QJsonArray &facets,QJsonObject result={})
{
   result["checkpoint"]="gravity-dialog."+name;result["outcome"]="passed";result["facets"]=facets;
   std::cout<<"NASA_CHECK "<<QJsonDocument(result).toJson(QJsonDocument::Compact).constData()<<std::endl;
}

int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
   app.setOrganizationName("GMATTests");app.setApplicationName("NasaGravityDialog");
   try {
      require((argc==2 || argc==3) && acceptanceWidgetPlatform(),"Expected startup, optional case and supported GUI platform");TestSettings settings;
      const QString selected=argc==3 ? QString::fromLocal8Bit(argv[2]) : QString("all");
      require(QStringList{"all","controls","runtime-earth-inherited","runtime-earth-file","runtime-earth-pole","runtime-lunar-file"}.contains(selected),"Unknown gravity dialog case");
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      QTemporaryDir files;require(files.isValid(),"Gravity dialog fixture directory unavailable");
      const auto lunar=QFileInfo("../data/gravity/luna/grgm900c.tide").canonicalFilePath();require(!lunar.isEmpty(),"NASA lunar tide fixture missing");
      MainWindow window;require(window.initialize(startup),"Gravity dialog runtime initialization failed");window.show();
      const auto jgm2=QFileInfo(QString::fromStdString(FileManager::Instance()->GetFullPathname("JGM2_FILE"))).canonicalFilePath();require(!jgm2.isEmpty(),"NASA JGM2 fixture missing");
      auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");auto *tree=window.findChild<QTreeWidget *>("Resources");auto *area=window.findChild<QMdiArea *>("workspace");require(editor && tree && area,"Workspace missing");
      const QString base="% Preserve gravity dialog source Δ\nCreate ForceModel Gravity;\nGravity.PrimaryBodies = {Earth};\nGravity.GravityField.Earth.Degree = 4;\nGravity.GravityField.Earth.Order = 4;\nCreate Variable Keep;\nKeep = 13; % preserve unrelated source\nBeginMissionSequence;\n";
      const QString earth="GravityField.Earth.",moon="GravityField.Luna.";
      const auto tide=files.filePath("Love numbers Δ.tide");
      {QFile file(tide);require(file.open(QIODevice::WriteOnly),"Could not create tide fixture");file.write("k 2 0 0.30190\nk 2 1 0.29830\nk 2 2 0.30102\n");}
      const auto directory=files.filePath("Directory.tide");require(QDir().mkdir(directory),"Could not create tide directory fixture");
      ResourceEditor *panel=nullptr;QMdiSubWindow *child=nullptr;QString source;QMap<QString,QString> before;int undo=0;
      auto open=[&](const QString &script) {
         source=script;editor->setPlainText(source);require(window.buildScript(),"Gravity dialog fixture did not build");before=properties();undo=editor->document()->availableUndoSteps();
         const auto items=tree->findItems("Gravity",Qt::MatchExactly|Qt::MatchRecursive);require(items.size()==1,"Gravity tree entry missing");tree->itemDoubleClicked(items.front(),0);events();child=area->currentSubWindow();panel=child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr;require(panel,"Gravity resource editor missing");acceptanceView(window);
      };
      auto unchanged=[&] {require(properties()==before && editor->toPlainText()==source && editor->document()->availableUndoSteps()==undo,"Gravity dialog changed configured model/source/history before Apply");};
      auto dialog=[&](const std::function<void(QDialog &)> &action) {
         auto *button=panel->findChild<QPushButton *>("forceGravity");require(button,"Gravity and tides action missing");
         for (auto *page=button->parentWidget();page;page=page->parentWidget()) for (auto *tabs:panel->findChildren<QTabWidget *>()) if (tabs->indexOf(page)>=0) tabs->setCurrentWidget(page);
         events();require(button->isVisible() && button->isEnabled(),"Gravity and tides action is not visible/enabled");std::exception_ptr failure;bool visited=false;
         QTimer::singleShot(0,panel,[&] {
            auto *surface=panel->findChild<QDialog *>("gravityDialog");
            try {require(surface,"Gravity dialog did not open");acceptanceView(*surface);visited=true;action(*surface);unchanged();}
            catch (...) {failure=std::current_exception();if (surface) surface->reject();}
         });
         button->click();if (failure) std::rethrow_exception(failure);require(visited,"Gravity dialog callback was not exercised");unchanged();
      };
      auto apply=[&](const QMap<QString,QString> &expected,const QString &name) {
         require(panel->hasChanges(),"Gravity dialog OK lost pending edits");unchanged();panel->requestApply();events();panel=dynamic_cast<ResourceEditor *>(child->widget());
         if (!panel || panel->hasChanges()) std::cerr<<"Gravity Apply diagnostic: "<<(panel ? panel->findChild<QLabel *>("resourceStatus")->text().toStdString() : "panel missing")<<std::endl;
         require(panel && !panel->hasChanges(),"Gravity dialog pending settings did not Apply");const auto after=editor->toPlainText();const auto values=properties();
         for (auto it=expected.cbegin();it!=expected.cend();++it) {
            const auto actual=values.value(it.key());const bool file=it.key().endsWith("File") && !it.value().isEmpty();
            require(file ? QFileInfo(actual).canonicalFilePath()==QFileInfo(it.value()).canonicalFilePath() : actual==it.value(),qPrintable("Applied gravity value differs: "+it.key()+" expected "+it.value()+" got "+actual));
         }
         require(!after.contains("TideDataSource") && after.contains("Keep = 13; % preserve unrelated source") && after.mid(after.indexOf("BeginMissionSequence;"))==source.mid(source.indexOf("BeginMissionSequence;")),"Gravity dialog serialized transient source or rewrote unrelated source");
         require(child->close(),"Applied gravity editor did not close");events();editor->undo();require(editor->toPlainText()==source && window.buildScript() && properties()==before,"Gravity dialog Undo was not exact");
         editor->redo();require(editor->toPlainText()==after && window.buildScript() && properties()==values,"Gravity dialog Redo was not exact");
         const auto saved=files.filePath(name+" Δ.script");require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript() && properties()==values,"Gravity dialog Unicode save/reopen changed values");
         QFile file(saved);require(file.open(QIODevice::ReadOnly) && QString::fromUtf8(file.readAll())==after && editor->toPlainText()==after,"Gravity dialog save/reopen changed source");
      };

      if (selected=="all" || selected=="controls") {
      open(base);
      dialog([&](QDialog &d) {
         require(choice(d,"Body")->currentText()=="Earth" && choice(d,"PotentialModel")->currentText()=="JGM-2","Default body/potential preset differs");
         auto *limits=d.findChild<QLabel *>("gravity_FileLimits");require(limits && limits->text()=="Degree 70 / Order 70","Selected JGM2 file limits differ");
         require(choice(d,"TideSource")->currentText()=="None" && !choice(d,"TideModel")->isEnabled() && !entry(d,"TideFile")->isEnabled() && !d.findChild<QPushButton *>("gravityBrowse_TideFile")->isEnabled(),"Default None dependent controls differ");
         select(d,"TideSource","Inherited");require(choice(d,"TideModel")->count()==3 && choice(d,"TideModel")->findText("SolidAndPole")>=0 && !entry(d,"TideFile")->isEnabled(),"Inherited Earth model/file controls differ");
         select(d,"TideModel","Solid");set(d,"Degree","6");set(d,"StmLimit","2");d.reject();
      });
      require(!panel->hasChanges(),"Gravity dialog Cancel dirtied parent");proof("cancel-default",{"source-inference","file-limits","dependent-controls","cancel-isolation"});
      dialog([&](QDialog &d) {select(d,"TideSource","Inherited");select(d,"TideModel","Solid");set(d,"Degree","6");set(d,"StmLimit","2");ok(d);require(d.result()==QDialog::Accepted,"Inherited settings rejected");});
      dialog([&](QDialog &d) {require(choice(d,"TideSource")->currentText()=="Inherited" && choice(d,"TideModel")->currentText()=="Solid" && entry(d,"Degree")->text()=="6" && entry(d,"StmLimit")->text()=="2","Reopened gravity dialog lost pending values");set(d,"Degree","7");d.reject();});
      apply({{earth+"TideModel","Solid"},{earth+"TideFile",""},{earth+"Degree","6"},{earth+"StmLimit","2"}},"inherited");proof("inherited-apply",{"pending-reopen","cancel-retains-pending","atomic-apply","exact-undo-redo","unicode-save-reopen"});

      auto imported=base;imported.insert(imported.indexOf("Create Variable"),"Gravity.GravityField.Earth.TideFile = '"+tide+"';\nGravity.GravityField.Earth.TideModel = SolidAndPole;\n");open(imported);
      dialog([&](QDialog &d) {require(choice(d,"TideSource")->currentText()=="Tide File" && choice(d,"TideModel")->currentText()=="SolidAndPole" && entry(d,"TideFile")->isEnabled(),"Imported tide file/model inference differs");select(d,"TideSource","None");require(entry(d,"TideFile")->text().isEmpty() && choice(d,"TideModel")->currentText()=="None" && !choice(d,"TideModel")->isEnabled(),"None did not clear/disable imported settings");d.reject();});
      require(!panel->hasChanges(),"Imported None Cancel changed parent");
      dialog([&](QDialog &d) {require(choice(d,"TideSource")->currentText()=="Tide File","Cancel lost imported source");select(d,"TideSource","None");ok(d);require(d.result()==QDialog::Accepted,"None source rejected");});
      apply({{earth+"TideFile",""},{earth+"TideModel","None"}},"none");proof("imported-none",{"imported-inference","source-clears-model-file","cancel-isolation","atomic-apply","exact-undo-redo","unicode-save-reopen"});

      open(imported);dialog([&](QDialog &d) {select(d,"TideSource","Inherited");require(entry(d,"TideFile")->text().isEmpty() && !entry(d,"TideFile")->isEnabled(),"Inherited did not clear external file");select(d,"TideModel","Solid");ok(d);require(d.result()==QDialog::Accepted,"Imported to inherited rejected");});
      apply({{earth+"TideFile",""},{earth+"TideModel","Solid"}},"external-to-inherited");proof("external-to-inherited",{"source-clears-external-file","atomic-apply","exact-undo-redo","unicode-save-reopen"});

      auto lunarSource=base;lunarSource.replace("Earth","Luna");lunarSource.insert(lunarSource.indexOf("Gravity.PrimaryBodies"),"Gravity.CentralBody = Luna;\n");open(lunarSource);
      dialog([&](QDialog &d) {
         require(choice(d,"PotentialModel")->currentText()=="LP165P" && choice(d,"TideSource")->findText("Inherited")<0,"Lunar COF incorrectly offers inherited Love numbers");
         select(d,"TideSource","Tide File");require(entry(d,"TideFile")->isEnabled() && !choice(d,"TideModel")->isEnabled(),"Empty tide-file dependent controls differ");rejected(d);
         browse(d,"TideFile",lunar,false);browse(d,"TideFile",lunar,true);
         require(choice(d,"TideModel")->isEnabled() && choice(d,"TideModel")->findText("Solid")>=0 && choice(d,"TideModel")->findText("SolidAndPole")<0,"Lunar tide file offers incorrect supported models");
         select(d,"TideModel","Solid");ok(d);require(d.result()==QDialog::Accepted,"Lunar external tide rejected");
      });
      apply({{moon+"TideFile",lunar},{moon+"TideModel","Solid"}},"lunar");proof("lunar-file",{"data-dependent-models","empty-file-rejection","file-picker-cancel-open","atomic-apply","exact-undo-redo","unicode-save-reopen"});

      open(base);dialog([&](QDialog &d) {
         select(d,"TideSource","Tide File");set(d,"TideFile",directory);rejected(d,directory);
         set(d,"TideFile",files.filePath("missing.tide"));rejected(d,files.filePath("missing.tide"));
         set(d,"TideFile",tide);select(d,"TideModel","SolidAndPole");
         select(d,"PotentialModel","EGM-96");require(d.findChild<QLabel *>("gravity_FileLimits")->text()=="Degree 360 / Order 360","Changed potential did not refresh file limits");set(d,"Degree","100");
         select(d,"PotentialModel","JGM-2");require(d.findChild<QLabel *>("gravity_FileLimits")->text()=="Degree 70 / Order 70" && entry(d,"Degree")->text()=="100","Lower-limit potential clamped or lost pending degree");rejected(d);
         browse(d,"PotentialFile",jgm2,false);browse(d,"PotentialFile",jgm2,true);require(entry(d,"Degree")->text()=="100","Potential selection silently clamped invalid degree");rejected(d);
         set(d,"Degree","6");ok(d);require(d.result()==QDialog::Accepted,"Corrected gravity/tide settings rejected");
      });
      apply({{earth+"TideFile",tide},{earth+"TideModel","SolidAndPole"},{earth+"Degree","6"}},"corrected");proof("invalid-correction",{"invalid-file-retained","file-bounds-rejection","potential-picker-cancel-open","no-silent-clamp","correct-and-apply","exact-undo-redo","unicode-save-reopen"});

      auto multi=base;multi.replace("{Earth}","{Earth, Luna}");multi.insert(multi.indexOf("Create Variable"),"Gravity.GravityField.Luna.Degree = 4;\nGravity.GravityField.Luna.Order = 4;\n");open(multi);
      auto multiEdit=[&](QDialog &d,bool accept) {
         require(choice(d,"Body")->count()==2,"Multi-primary gravity body selector incomplete");select(d,"Body","Earth");set(d,"Degree","6");select(d,"TideSource","Inherited");select(d,"TideModel","Solid");
         select(d,"Body","Luna");set(d,"Degree","8");select(d,"TideSource","Tide File");set(d,"TideFile",lunar);select(d,"TideModel","Solid");
         select(d,"Body","Earth");require(entry(d,"Degree")->text()=="6" && choice(d,"TideSource")->currentText()=="Inherited" && choice(d,"TideModel")->currentText()=="Solid","Switching bodies lost pending Earth inputs");
         select(d,"Body","Luna");require(entry(d,"Degree")->text()=="8" && entry(d,"TideFile")->text()==lunar && choice(d,"TideModel")->currentText()=="Solid","Switching bodies lost pending lunar inputs");
         if (accept) {ok(d);require(d.result()==QDialog::Accepted,"Multi-primary gravity settings rejected");} else d.reject();
      };
      dialog([&](QDialog &d){multiEdit(d,false);});require(!panel->hasChanges(),"Multi-primary dialog Cancel changed parent");dialog([&](QDialog &d){multiEdit(d,true);});
      apply({{earth+"Degree","6"},{earth+"TideModel","Solid"},{moon+"Degree","8"},{moon+"TideFile",lunar},{moon+"TideModel","Solid"}},"multi");proof("multi-primary",{"body-switch-preserves-input","cancel-isolation","atomic-multi-body-apply","exact-undo-redo","unicode-save-reopen"});

      auto pointOnly=base;pointOnly.replace("Gravity.PrimaryBodies = {Earth};\nGravity.GravityField.Earth.Degree = 4;\nGravity.GravityField.Earth.Order = 4;","Gravity.PrimaryBodies = {};\nGravity.PointMasses = {Earth, Sun};");open(pointOnly);
      auto *primary=panel->findChild<QLineEdit *>("resource_PrimaryBodies");auto *points=panel->findChild<QLineEdit *>("resource_PointMasses");require(primary && points,"Primary/point-mass controls missing");primary->setText("Earth");points->setText("Sun");events();
      dialog([&](QDialog &d) {require(choice(d,"Body")->currentText()=="Earth","Pending primary body not available in gravity dialog");set(d,"Degree","6");set(d,"Order","2");select(d,"TideSource","Inherited");select(d,"TideModel","Solid");ok(d);require(d.result()==QDialog::Accepted,"Pending primary gravity settings rejected");});
      apply({{earth+"Degree","6"},{earth+"Order","2"},{earth+"TideModel","Solid"}},"new-primary");proof("pending-primary",{"new-primary-preview","same-transaction-body-and-gravity","exact-undo-redo","unicode-save-reopen"});

      open(pointOnly);dialog([&](QDialog &d) {require(choice(d,"Body")->count()==0 && !entry(d,"Degree")->isEnabled() && !choice(d,"TideSource")->isEnabled() && !d.findChild<QLabel *>("gravityError")->text().isEmpty(),"Point-mass-only model exposes gravity/tide editing");ok(d);require(d.result()==QDialog::Accepted,"Empty gravity dialog did not close cleanly");});
      require(!panel->hasChanges(),"Empty gravity dialog created pending edits");require(child->close(),"Unchanged empty gravity editor did not close");events();proof("no-primary",{"no-primary-disables-controls","explanatory-message","unchanged-ok"});

      open(base);dialog([&](QDialog &d) {set(d,"Degree","6");select(d,"TideSource","Inherited");select(d,"TideModel","Solid");ok(d);require(d.result()==QDialog::Accepted,"Staged gravity settings rejected");});require(panel->hasChanges(),"Staged gravity changes missing");
      dialog([&](QDialog &d) {set(d,"Degree","4");select(d,"TideSource","None");ok(d);require(d.result()==QDialog::Accepted,"Reverting pending gravity settings rejected");});
      require(!panel->hasChanges(),"Reverted gravity dialog remains dirty");unchanged();
      require(panel->hasUnappliedInput(),"Reverted gravity draft did not retain its dirty-input latch");
      panel->requestApply();events();unchanged();
      require(!panel->hasUnappliedInput(),"No-op gravity Apply did not clear dirty input");
      require(child->close(),"Reverted gravity editor did not close");events();proof("revert-pending",{"pending-reopen","revert-to-applied","clean-parent","source-history-isolation"});

      for (bool reopen:{false,true}) {
         open(base);dialog([&](QDialog &d) {set(d,"Degree","6");select(d,"TideSource","Inherited");select(d,"TideModel","Solid");ok(d);require(d.result()==QDialog::Accepted,"Gravity settings before primary removal rejected");});
         primary=panel->findChild<QLineEdit *>("resource_PrimaryBodies");points=panel->findChild<QLineEdit *>("resource_PointMasses");require(primary && points,"Primary removal controls missing");primary->setText("");points->setText("Earth");events();
         if (reopen) dialog([&](QDialog &d) {require(choice(d,"Body")->count()==0,"Removed pending primary remains selectable");ok(d);require(d.result()==QDialog::Accepted,"Removed-primary dialog did not close");});
         apply({},reopen ? "removed-primary-reopened" : "removed-primary");
         const auto removed=properties();for (auto it=removed.cbegin();it!=removed.cend();++it) require(!it.key().startsWith(earth),"Removed primary retained a staged gravity field");
         proof(reopen ? "removed-primary-reopened" : "removed-primary",{"staged-gravity-pruned","primary-point-mass-transaction","exact-undo-redo","unicode-save-reopen"});
      }
      }
      // An equivalent authored mission is a configuration-fidelity oracle.
      // It shares GMAT's force equations and is not independent tide accuracy.
      struct RuntimeCase {QString name,body,source,model,file;};
      for (const auto &test:QList<RuntimeCase>{{"earth-inherited","Earth","Inherited","Solid",""},{"earth-file","Earth","Tide File","Solid",tide},{"earth-pole","Earth","Inherited","SolidAndPole",""},{"lunar-file","Luna","Tide File","Solid",lunar}}) {
         if (selected!="all" && selected!="runtime-"+test.name) continue;
         const auto report=files.filePath(test.name+"-states.txt");const auto frame=test.body=="Earth" ? QString("EarthMJ2000Eq") : QString("MoonFrame");
         auto runtime=base;
         if (test.body=="Luna") {runtime.replace("Earth","Luna");runtime.insert(runtime.indexOf("Gravity.PrimaryBodies"),"Gravity.CentralBody = Luna;\n");}
         QString setup;
         if (test.body=="Luna") setup="Create CoordinateSystem MoonFrame;\nMoonFrame.Origin = Luna;\nMoonFrame.Axes = MJ2000Eq;\n";
         setup+="Create Spacecraft Sat;\nSat.CoordinateSystem = "+frame+";\nSat.DateFormat = UTCGregorian;\nSat.Epoch = '01 Jan 2024 12:00:00.000';\nCreate Propagator Prop;\nProp.FM = Gravity;\nProp.Accuracy = 1e-13;\nProp.InitialStepSize = 10;\nCreate ReportFile States;\nStates.Filename = '"+report+"';\nStates.WriteHeaders = false;\nStates.Precision = 16;\n";
         runtime.replace("BeginMissionSequence;\n",setup+"BeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 600};\nReport States Sat."+frame+".X Sat."+frame+".Y Sat."+frame+".Z Sat."+frame+".VX Sat."+frame+".VY Sat."+frame+".VZ;\n");
         auto reference=runtime;reference.insert(reference.indexOf("Create Variable"),"Gravity.GravityField."+test.body+".Degree = 6;\nGravity.GravityField."+test.body+".StmLimit = 2;\n"+(test.file.isEmpty() ? QString() : "Gravity.GravityField."+test.body+".TideFile = '"+test.file+"';\n")+"Gravity.GravityField."+test.body+".TideModel = "+test.model+";\n");
         auto run=[&] {
            require(window.runMission()==MainWindow::RunResult::Completed,"Gravity tide reference/GUI mission did not complete");QFile file(report);require(file.open(QIODevice::ReadOnly),"Gravity tide state report missing");
            const auto words=QString::fromUtf8(file.readAll()).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts);require(words.size()==6,"Gravity tide report is not a six-component state");QList<double> state;
            for (const auto &word:words) {bool valid=false;const double value=word.toDouble(&valid);require(valid && std::isfinite(value),"Gravity tide state report has nonfinite/non-numeric result");state.append(value);}return state;
         };
         editor->setPlainText(reference);
         if (!window.buildScript()) {events();std::cerr<<"Authored tide fixture:\n"<<reference.toStdString()<<"\nBuild diagnostic:\n"<<window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString()<<std::endl;throw std::runtime_error("Authored tide reference did not build");}
         const auto expected=run();
         open(runtime);dialog([&](QDialog &d) {select(d,"TideSource",test.source);if (!test.file.isEmpty()) set(d,"TideFile",test.file);select(d,"TideModel",test.model);set(d,"Degree","6");set(d,"StmLimit","2");ok(d);require(d.result()==QDialog::Accepted,"Runtime gravity dialog settings rejected");});
         const auto prefix="GravityField."+test.body+".";apply({{prefix+"Degree","6"},{prefix+"StmLimit","2"},{prefix+"TideModel",test.model},{prefix+"TideFile",test.file}},"runtime-"+test.name);
         const auto actual=run();for (int i=0;i<6;++i) require(std::abs(actual[i]-expected[i])<=1e-10*std::max(1.0,std::abs(expected[i])),"GUI tide trajectory differs from equivalent authored mission");
         auto *prop=dynamic_cast<PropSetup *>(Moderator::Instance()->GetInternalObject("Prop"));auto *forces=prop ? prop->GetODEModel() : nullptr;require(forces,"Running tide propagator/force model missing");int count=0;
         for (int i=0;i<forces->GetNumForces();++i) if (auto *force=forces->GetForce(i);force->IsOfType("GravityField")) {
            ++count;require(force->GetBodyName()==test.body.toStdString() && force->GetIntegerParameter("Degree")==6 && force->GetIntegerParameter("Order")==4 && force->GetIntegerParameter("StmLimit")==2 && force->GetStringParameter("TideModel")==test.model.toStdString(),"Running gravity/tide settings differ from GUI values");
            const auto runtimeFile=QString::fromStdString(force->GetStringParameter("TideFile"));require(test.file.isEmpty() ? runtimeFile.isEmpty() : QFileInfo(runtimeFile).canonicalFilePath()==QFileInfo(test.file).canonicalFilePath(),"Running tide file differs from selected source");
         }
         require(count==1,"Runtime tide mission did not contain exactly one selected gravity field");proof("runtime-"+test.name,{"gui-to-runtime-owned-force","six-state-script-reference","finite-results","600-second-mission","exact-undo-redo","unicode-save-reopen","shared-engine-not-independent-physics"},{{"body",test.body},{"tide_model",test.model},{"data_source",test.source},{"elapsed_seconds",600}});
      }
      return 0;
   } catch (BaseException &failure) {std::cerr<<"FAIL "<<failure.GetFullMessage()<<std::endl;}
     catch (const std::exception &failure) {std::cerr<<"FAIL "<<failure.what()<<std::endl;}
   return 1;
}
