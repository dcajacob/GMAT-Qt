#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "PolyhedronDialog.hpp"
#include <QJsonObject>
#include <QLabel>
#include <QEventLoop>
#include <QWindow>
#include <QStatusBar>
#include "ReportParameterDialog.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QPlainTextEdit>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QTableWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QTimer>
#include <QDialog>
#include <QComboBox>
#include <QTreeWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void write(const QString &path,const QString &text) { QFile file(path); require(file.open(QIODevice::WriteOnly),"Fixture write failed"); require(file.write(text.toUtf8())==text.toUtf8().size(),"Fixture write incomplete"); }
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Report unavailable"); return QString::fromUtf8(file.readAll()); }
static QVector<double> numbers(const QString &path) { QVector<double> result; for (const auto &word:read(path).split(QRegularExpression("\\s+"),Qt::SkipEmptyParts)) { bool ok; const double number=word.toDouble(&ok); require(ok && std::isfinite(number),"Nonnumeric polyhedron report"); result.append(number); } return result; }
static void run(MainWindow &window) {
   static int phase=0; ++phase; std::cout<<"Polyhedron run phase "<<phase<<std::endl;
   if (window.runMission()!=MainWindow::RunResult::Completed) {
      QApplication::processEvents(); const auto message=window.statusBar()->currentMessage()+"\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText();
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      if (!evidence.isEmpty()) {
         write(QDir(evidence).filePath("failed-phase-"+QString::number(phase)+".script"),window.findChild<QPlainTextEdit *>("scriptEditor")->toPlainText());
         write(QDir(evidence).filePath("failed-phase-"+QString::number(phase)+".txt"),message);
      }
      throw std::runtime_error(("Polyhedron phase "+QString::number(phase)+": "+message).toStdString());
   }
}
static void equivalent(const QVector<double> &a,const QVector<double> &b) { require(a.size()==b.size(),"Polyhedron report dimensions changed"); for (int i=0;i<a.size();++i) require(std::abs(a[i]-b[i])<1e-11*std::max(1.0,std::abs(b[i])),"GUI polyhedron result differs from script reference"); }
static QTableWidgetItem *field(ResourceEditor &panel,const QString &name) { auto *table=panel.findChild<QTableWidget *>(); require(table,"Properties table missing"); for (int i=0;i<table->rowCount();++i) if (table->item(i,0)->text()==name) return table->item(i,1); throw std::runtime_error(("Missing property "+name).toStdString()); }
static void bodySourceCases(MainWindow &window,QPlainTextEdit &editor,QTemporaryDir &files,const QString &shape,const QString &report,const QString &reference)
{
   auto source=reference; source.remove("FM.BodyDensity = 2000;\n"); source.remove("FM.Drag = None;\n"); source.remove("FM.SRP = Off;\n");
   source.replace("FM.CentralBody = Earth;","  GMAT FM.CentralBody = Earth; % retained central spelling"); source.replace(" Sat.FM.SurfaceHeight;",";");
   source.replace("FM.PolyhedralBodies = {Earth};","FM.PointMasses = {Sun}; % retained point mass\nFM.UserDefined = {PolyhedronGravityModel}; % legacy creator"); source.prepend("% implicit polyhedron density and unrelated defaults\n");
   const auto suffix=source.mid(source.indexOf("BeginMissionSequence;")),copied=files.filePath("moved cube Δ.txt"); require(QFile::copy(shape,copied),"Moved shape fixture copy failed");
   editor.setPlainText(source); run(window); const auto earth=numbers(report); require(earth.size()==6,"Moved body state report incomplete");
   auto independent=source; independent.replace("FM.UserDefined = {PolyhedronGravityModel};","FM.PolyhedralBodies = {Mars};"); independent.replace("FM.CreateForceBody = Earth;","FM.CreateForceBody = Mars;"); independent.replace(shape,copied); independent.insert(independent.indexOf("BeginMissionSequence;"),"FM.ErrorControl = LargestStep;\n");
   editor.setPlainText(independent); run(window); const auto mars=numbers(report);
   editor.setPlainText(source); require(window.buildScript(),"Legacy body source restore failed");
   auto *model=Moderator::Instance()->GetConfiguredObject("FM"); const auto pending=QJsonArray{QJsonObject{{"body","Mars"},{"shape",shape},{"density","1000"}}};
   auto qualified=source; qualified.replace("FM.ShapeFileName =", "FM.PolyhedronGravityModel.Earth.ShapeFileName =");
   const auto mapped=polyhedronScript(qualified,*model,pending,"",{{"Earth","Mars"}});
   require(mapped.contains("FM.PolyhedronGravityModel.Mars.ShapeFileName") && !mapped.contains("FM.PolyhedronGravityModel.Earth.") && !mapped.contains("BodyDensity ="),"Mapped creator lost qualified ownership or implicit density");
   const auto future=source.left(source.indexOf("BeginMissionSequence;"))+"FM.PolyhedronGravityModel.Earth.FutureOption = 'keep exactly'; % unknown option\n"+suffix;
   require(polyhedronScript(future,*model,pending,"",{{"Earth","Mars"}}).contains("FM.PolyhedronGravityModel.Mars.FutureOption = 'keep exactly';"),"Moved body dropped an unrelated owned source option");
   {
      PolyhedronDialog fresh({}); fresh.findChild<QPushButton *>("polyhedronAdd")->click(); auto *rows=fresh.findChild<QTableWidget *>("polyhedronTable");
      qobject_cast<QComboBox *>(rows->cellWidget(0,0))->setCurrentText("Earth"); rows->item(0,1)->setText(shape); fresh.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
      require(fresh.values().size()==1 && fresh.values().first().toObject().contains("previousBody") && fresh.values().first().toObject().value("previousBody").toString().isEmpty(),"New pending row lacks an empty origin");
      PolyhedronDialog reopened(fresh.values()); rows=reopened.findChild<QTableWidget *>("polyhedronTable"); qobject_cast<QComboBox *>(rows->cellWidget(0,0))->setCurrentText("Mars"); reopened.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
      require(reopened.values().first().toObject().value("previousBody").toString().isEmpty(),"Reopened new row impersonates an existing contributor");
   }
   auto open=[&] {
      auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto matches=tree->findItems("FM",Qt::MatchExactly|Qt::MatchRecursive); require(matches.size()==1,"Force resource missing"); tree->itemDoubleClicked(matches.first(),0);
      for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("resourceName").toString()=="FM") return child;
      throw std::runtime_error("Actual polyhedron panel missing");
   };
   auto *child=open(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"Polyhedron editor missing");
   field(*panel,"PolyhedronGravityModel.Earth.CreateForceBody")->setText("Sat"); field(*panel,"PolyhedronGravityModel.Earth.ShapeFileName")->setText(copied);
   auto *table=panel->findChild<QTableWidget *>(); for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="ErrorControl") qobject_cast<QComboBox *>(table->cellWidget(row,1))->setCurrentText("LargestStep");
   auto *apply=panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply); apply->click();
   require(editor.toPlainText()==source && child->widget()==panel && panel->hasChanges(),"Invalid moved body changed source or lost pending values");
   field(*panel,"PolyhedronGravityModel.Earth.CreateForceBody")->setText("Mars"); apply->click();
   const auto moved=editor.toPlainText(); if (moved==source) throw std::runtime_error(window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString());
   require(child->isVisible() && child->widget()!=panel && !dynamic_cast<EditablePanel *>(child->widget())->hasChanges(),"Moved body did not refresh retained panel");
   require(moved.contains("  GMAT FM.CentralBody = Earth; % retained central spelling") && moved.contains("FM.PointMasses = {Sun}; % retained point mass") && moved.endsWith(suffix) && moved.contains("% legacy creator") && moved.contains("LargestStep"),"Moved body lost unrelated source/comments or mixed field");
   for (const auto &property:QStringList{"PrimaryBodies","Drag","SRP","RelativisticCorrection","BodyDensity"}) require(!moved.contains("FM."+property+" ="),"Moved body made an implicit force default explicit");
   require(!moved.contains(".BodyDensity ="),"Moved body made implicit density explicit");
   const auto settings=polyhedronSettings(*Moderator::Instance()->GetConfiguredObject("FM")); require(settings.size()==1 && settings.first().toObject().value("body")=="Mars" && settings.first().toObject().value("shape")==copied,"Moved contributor settings were not retained");
   editor.undo(); require(editor.toPlainText()==source,"Moved body Undo was not exact"); editor.redo(); require(editor.toPlainText()==moved,"Moved body Redo was not exact");
   const auto saved=files.filePath("moved body ü.script"); require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Moved body Unicode save/reopen failed"); run(window); equivalent(numbers(report),mars);
   const auto error=window.applyResourceChanges("FM",{{"PolyhedronGravityModel.Mars.CreateForceBody","Earth"},{"PolyhedronGravityModel.Mars.ShapeFileName",shape},{"ErrorControl","RSSStep"}},editor.toPlainText()); if (!error.isEmpty()) throw std::runtime_error(error.toStdString());
   require(window.saveScriptTo(saved) && window.loadScript(saved),"Restored body save/reopen failed"); run(window); equivalent(numbers(report),earth);
   const auto restored=editor.toPlainText(); child=open(); panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"Restored polyhedron editor missing");
   std::exception_ptr failure;
   auto dialog=[&](bool change) {
      QTimer::singleShot(0,panel,[&] { auto *controls=dynamic_cast<PolyhedronDialog *>(panel->findChild<QDialog *>("polyhedronDialog")); try {
         require(controls,"Polyhedron contributor dialog missing"); auto *rows=controls->findChild<QTableWidget *>("polyhedronTable"); auto *body=qobject_cast<QComboBox *>(rows->cellWidget(0,0));
         if (change) { body->setCurrentText("Mars"); rows->item(0,1)->setText(copied); }
         else require(body->currentText()=="Mars","Reopened pending body was not retained");
         controls->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
      } catch (...) { failure=std::current_exception(); if (controls) controls->reject(); } });
      panel->findChild<QPushButton *>("forcePolyhedron")->click(); if (failure) std::rethrow_exception(failure);
   };
   dialog(true); dialog(false); table=panel->findChild<QTableWidget *>(); for (int row=0;row<table->rowCount();++row) if (table->item(row,0)->text()=="ErrorControl") qobject_cast<QComboBox *>(table->cellWidget(row,1))->setCurrentText("LargestStep");
   require(panel->hasChanges() && editor.toPlainText()==restored,"Dialog body move did not stay pending"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
   const auto dialogMoved=editor.toPlainText(); require(dialogMoved!=restored && child->widget()!=panel && !dialogMoved.contains(".BodyDensity =") && dialogMoved.endsWith(suffix),"Dialog body move lost retained panel, implicit density or mission");
   editor.undo(); require(editor.toPlainText()==restored,"Dialog body move Undo was not exact"); editor.redo(); require(editor.toPlainText()==dialogMoved,"Dialog body move Redo was not exact");
   require(window.saveScriptTo(saved) && window.loadScript(saved),"Dialog moved body save/reopen failed"); run(window); equivalent(numbers(report),mars);
   std::cout<<"PASS: actual per-field polyhedron body/path move with mixed ErrorControl, invalid-body correction and retained panel, legacy creator conversion, unrelated source/defaults and implicit density, owned unknown-option mapping, exact Undo/Redo/Unicode save/reopen and independent Earth/Mars six-state reports.\n";
}
static void pickShape(ResourceEditor &panel,const QString &path,bool accept) {
   std::exception_ptr failure; auto *button=panel.findChild<QPushButton *>("chooseProperty_PolyhedronGravityModel.Earth.ShapeFileName"); require(button,"Shape file picker missing");
   QTimer::singleShot(0,&panel,[&] { auto *dialog=panel.findChild<QFileDialog *>(); try { require(dialog && dialog->fileMode()==QFileDialog::ExistingFile,"Shape picker is not an existing-input picker"); dialog->selectFile(path); if (accept) QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection); else dialog->reject(); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } });
   button->click(); if (failure) std::rethrow_exception(failure);
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtPolyhedron");
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid() && argc>1,"Fixture setup failed"); const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Initialization failed"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const auto shape=files.filePath("cube shape ü.txt"),report=files.filePath("polyhedron state.txt");
      write(shape,"8\n1 -10 -10 -10\n2 10 -10 -10\n3 10 10 -10\n4 -10 10 -10\n5 -10 -10 10\n6 10 -10 10\n7 10 10 10\n8 -10 10 10\n12\n1 1 3 2\n2 1 4 3\n3 5 6 7\n4 5 7 8\n5 1 5 8\n6 1 8 4\n7 2 3 7\n8 2 7 6\n9 1 2 6\n10 1 6 5\n11 4 8 7\n12 4 7 3\n");
      const QString reference="Create Spacecraft Sat;\nSat.DisplayStateType = Cartesian;\nSat.X = 100;\nSat.Y = 50;\nSat.Z = 30;\nSat.VX = 0;\nSat.VY = 0;\nSat.VZ = 0;\nCreate ForceModel FM;\nFM.CentralBody = Earth;\nFM.PolyhedralBodies = {Earth};\nFM.CreateForceBody = Earth;\nFM.ShapeFileName = '"+shape+"';\nFM.BodyDensity = 2000;\nFM.Drag = None;\nFM.SRP = Off;\nCreate Propagator Prop;\nProp.FM = FM;\nProp.Accuracy = 1e-12;\nProp.InitialStepSize = 1;\nProp.MaxStep = 1;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.Precision = 16;\nBeginMissionSequence;\nPropagate Prop(Sat) {Sat.ElapsedSecs = 60};\nReport Values Sat.EarthMJ2000Eq.X Sat.EarthMJ2000Eq.Y Sat.EarthMJ2000Eq.Z Sat.EarthMJ2000Eq.VX Sat.EarthMJ2000Eq.VY Sat.EarthMJ2000Eq.VZ Sat.FM.SurfaceHeight;\n";
      if (argc==3 && QString::fromLocal8Bit(argv[2])=="--body-source") { bodySourceCases(window,*editor,files,shape,report,reference); return 0; }
      editor->setPlainText(reference); require(window.buildScript(),"Polyhedron reference failed to build"); run(window); const auto expected=numbers(report); require(expected.size()==7,"Polyhedron state report missing");
      require(expected[3]<0 && expected[4]<0 && expected[5]<0,"Cube gravity does not point toward its center");
      {
         ReportParameterDialog dialog({}); dialog.findChild<QComboBox *>("reportPropertyObject")->setCurrentText("Sat");
         auto *property=dialog.findChild<QComboBox *>("reportPropertyType"); require(property->findText("SurfaceHeight")>=0,"Polyhedron output parameter absent from browser"); property->setCurrentText("SurfaceHeight");
         auto *dependency=dialog.findChild<QComboBox *>("reportPropertyDependency"); require(dependency->findText("FM")>=0,"Force-model parameter dependency absent"); dependency->setCurrentText("FM");
         dialog.findChild<QPushButton *>("reportUseReference")->click(); dialog.findChild<QPushButton *>("reportAddParameter")->click(); require(dialog.selection()==QStringList{"Sat.FM.SurfaceHeight"},"SurfaceHeight browser generated wrong reference"); dialog.reject(); require(editor->toPlainText()==reference,"Output browser Cancel changed mission");
      }
      // At this distance, the uniform cube approaches a point mass with its
      // independently calculated volume (20^3 km^3) and density (kg/m^3).
      const double radius=std::sqrt(100.0*100+50.0*50+30.0*30),mu=6.673e-20*8000.0*1e9*2000.0;
      const double initial[]={100,50,30};
      for (int i=0;i<3;++i) { const double velocity=-mu*initial[i]*60/(radius*radius*radius); require(std::abs(expected[i+3]-velocity)<0.002*std::abs(velocity),"Cube result disagrees with independent far-field mass check"); }
      const auto decoy=files.filePath("smaller cube.txt"),saved=files.filePath("polyhedron mission Δ.script");
      QString small=read(shape); small.replace("-10","-5"); small.replace(" 10"," 5"); write(decoy,small);
      QString source=reference; source.replace(shape,decoy); source.replace("BodyDensity = 2000","BodyDensity = 1000"); editor->setPlainText(source); require(window.buildScript(),"Decoy cube fixture failed");
      QString error;
      {
         QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject("FM"),[&](const auto &changes) { error=window.applyResourceChanges("FM",changes,source); return error; },&owner,source);
         auto *table=panel.findChild<QTableWidget *>(); const QString prefix="PolyhedronGravityModel.Earth.";
         for (int i=0;i<table->rowCount();++i) if (table->item(i,0)->text()==prefix+"BodyDensity") require(table->item(i,2)->text()=="kg/m^3","Density units absent or incorrect");
         require(field(panel,prefix+"ShapeFileName")->toolTip().contains("kilometres"),"Shape coordinate units unexplained");
         auto *choose=panel.findChild<QPushButton *>("chooseProperty_"+prefix+"CreateForceBody"); require(choose,"Typed gravity-body selector missing");
         std::exception_ptr failure;
         QTimer::singleShot(0,&panel,[&] { auto *dialog=panel.findChild<QInputDialog *>(); try { require(dialog && dialog->comboBoxItems().contains("Earth") && !dialog->comboBoxItems().contains("Sat"),"Gravity body choices are not celestial bodies"); dialog->setTextValue("Mars"); dialog->reject(); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } }); choose->click(); if (failure) std::rethrow_exception(failure);
         require(field(panel,prefix+"CreateForceBody")->text()=="Earth","Gravity-body Cancel changed pending value");
         pickShape(panel,shape,false); require(field(panel,prefix+"ShapeFileName")->text()==decoy,"Shape chooser Cancel changed pending path");
         pickShape(panel,shape,true); field(panel,prefix+"BodyDensity")->setText("2000"); require(editor->toPlainText()==source,"Pending polyhedron changes altered source");
         if (argc>2) { panel.resize(900,680); panel.show(); app.processEvents(); panel.grab().save(QString::fromLocal8Bit(argv[2])); }
         panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); if (!error.isEmpty()) { app.processEvents(); throw std::runtime_error((error+"\n"+window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText()).toStdString()); }
      }
      const auto configured=editor->toPlainText(); run(window); equivalent(numbers(report),expected);
      require(configured.count("FM.PolyhedralBodies = {Earth}")==1 && !configured.contains("FM.UserDefined = {PolyhedronGravityModel}"),"Serialized cube contributor duplicated");
      editor->undo(); require(editor->toPlainText()==source,"Polyhedron Undo not exact"); editor->redo(); require(editor->toPlainText()==configured,"Polyhedron Redo not exact"); run(window); equivalent(numbers(report),expected);
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Polyhedron Unicode save/reopen failed"); run(window); equivalent(numbers(report),expected);
      const QString prefix="PolyhedronGravityModel.Earth.";
      require(window.applyResourceChanges("FM",{{prefix+"CreateForceBody","Mars"},{prefix+"ShapeFileName",shape}},editor->toPlainText()).isEmpty(),"Paired gravity-body/path edit failed");
      require(editor->toPlainText().contains("FM.PolyhedralBodies = {Mars}"),"Changed gravity body was not serialized");
      require(window.applyResourceChanges("FM",{{"PolyhedronGravityModel.Mars.CreateForceBody","Earth"},{"PolyhedronGravityModel.Mars.ShapeFileName",shape}},editor->toPlainText()).isEmpty(),"Gravity-body restoration failed"); run(window); equivalent(numbers(report),expected);
      auto *outputs=window.findChild<QTreeWidget *>("Output"); const auto reports=outputs->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive); require(reports.size()==1,"Polyhedron report missing from Output"); outputs->itemDoubleClicked(reports.first(),0); app.processEvents();
      const auto viewers=window.findChild<QMdiArea *>()->subWindowList(); bool visibleReport=false; for (auto *viewer:viewers) if (viewer->windowTitle().contains("polyhedron state")) visibleReport=viewer->isVisible(); require(visibleReport,"Polyhedron report could not be opened through Output");
      const auto before=editor->toPlainText();
      for (const auto &value : {"0","-1","nan"}) require(!window.applyResourceChanges("FM",{{prefix+"BodyDensity",value}},before).isEmpty() && editor->toPlainText()==before,"Invalid density did not roll back");
      require(!window.applyResourceChanges("FM",{{prefix+"CreateForceBody","Sat"}},before).isEmpty() && editor->toPlainText()==before,"Invalid gravity-body type did not roll back");
      require(!window.applyResourceChanges("FM",{{prefix+"ShapeFileName",files.filePath("missing shape.txt")}},before).isEmpty() && editor->toPlainText()==before,"Missing shape did not roll back");
      const QString valid=read(shape); QString truncated=valid.left(valid.indexOf("12\n")); QString outOfRange=valid; outOfRange.replace("1 1 3 2","1 99 3 2"); QString degenerate=valid; degenerate.replace("1 1 3 2","1 1 1 2"); QString open=valid; open.replace("12\n","11\n"); open=open.left(open.lastIndexOf("12 4 7 3"));
      QString nonfinite=valid; nonfinite.replace("1 -10 -10 -10","1 nan -10 -10"); QString reversed=valid; reversed.replace("1 1 3 2","1 1 2 3");
      QString collinear=valid; collinear.replace("3 10 10 -10","3 30 -10 -10");
      auto reverseRows=valid.trimmed().split('\n'); for (int i=10;i<reverseRows.size();++i) { auto columns=reverseRows[i].split(' '); std::swap(columns[2],columns[3]); reverseRows[i]=columns.join(' '); }
      const QStringList badMeshes={"invalid count\n",truncated,outOfRange,degenerate,open,nonfinite,reversed,collinear,reverseRows.join('\n'),valid+"unexpected\n"};
      for (int i=0;i<badMeshes.size();++i) {
         const auto bad=files.filePath("malformed "+QString::number(i)+".txt"); write(bad,badMeshes[i]);
         const auto snapshot=editor->toPlainText(); require(!window.applyResourceChanges("FM",{{prefix+"ShapeFileName",bad}},snapshot).isEmpty(),"Malformed shape accepted by GUI"); require(editor->toPlainText()==snapshot,"Rejected shape changed source");
         editor->setPlainText(QString(reference).replace(shape,bad)); if (window.buildScript()) require(window.runMission()==MainWindow::RunResult::Failed,"Malformed mesh did not fail execution");
         require(window.loadScript(saved) && window.buildScript(),"Malformed mesh recovery failed"); run(window); equivalent(numbers(report),expected);
      }
      const auto missing=files.filePath("missing shape.txt"); editor->setPlainText(QString(reference).replace(shape,missing)); if (window.buildScript()) require(window.runMission()==MainWindow::RunResult::Failed,"Missing mesh did not fail execution"); require(window.loadScript(saved) && window.buildScript(),"Missing mesh recovery failed"); run(window); equivalent(numbers(report),expected);
      // The loader accepts normal whitespace and a final line without newline.
      const auto whitespace=files.filePath("cube CRLF tabs.txt"); QString changed=valid.trimmed(); changed.replace(' ','\t'); changed.replace("\n","\r\n"); write(whitespace,changed);
      require(window.applyResourceChanges("FM",{{prefix+"ShapeFileName",whitespace}},editor->toPlainText()).isEmpty(),"CRLF/tab/no-final-newline mesh rejected"); run(window); equivalent(numbers(report),expected);
      // Record labels are decorative in the legacy format; connectivity still
      // indexes vertex rows from one. Preserve files with nonsequential labels.
      auto relabeledRows=valid.trimmed().split('\n'); for (int i=1;i<relabeledRows.size();++i) if (i!=9) { auto columns=relabeledRows[i].split(' '); columns[0]=QString::number(100+i); relabeledRows[i]=columns.join(' '); }
      const auto relabeled=files.filePath("cube decorative labels.txt"); write(relabeled,relabeledRows.join('\n'));
      require(window.applyResourceChanges("FM",{{prefix+"ShapeFileName",relabeled}},editor->toPlainText()).isEmpty(),"Legacy decorative record labels rejected"); run(window); equivalent(numbers(report),expected);
      const auto relative=files.filePath("relative polyhedron.script"); write(relative,QString(reference).replace(shape,QFileInfo(shape).fileName())); require(window.loadScript(relative) && window.buildScript(),"Relative shape mission failed"); run(window); equivalent(numbers(report),expected);
      require(window.applyResourceChanges("Values",{{"Precision","15"}},editor->toPlainText()).isEmpty(),"Unrelated resource edit lost relative shape context"); run(window); equivalent(numbers(report),expected);
      require(window.saveScriptTo(saved) && window.loadScript(saved) && window.buildScript(),"Relative shape round trip failed"); run(window); equivalent(numbers(report),expected);
      // Exercise structural editing in the actual retained force-model panel.
      // Keep an unrelated solar point mass so removing the final polyhedron
      // leaves a runnable model, and avoid a SurfaceHeight reference after removal.
      QString creationReference=reference;
      creationReference.replace("FM.CentralBody = Earth;","FM.CentralBody = Earth;\nFM.PointMasses = {Sun};");
      creationReference.replace(" Sat.FM.SurfaceHeight;",";"); creationReference.prepend("% retained polyhedron source α\n");
      creationReference.replace("FM.BodyDensity = 2000;","FM.BodyDensity = 2000; % retain density comment");
      editor->setPlainText(creationReference); run(window); const auto creationExpected=numbers(report); require(creationExpected.size()==6,"Creation report dimensions wrong");
      auto without=creationReference; without.remove(QRegularExpression("^FM\\.(?:PolyhedralBodies|CreateForceBody|ShapeFileName|BodyDensity)[^\\n]*(?:\\n|$)",QRegularExpression::MultilineOption));
      editor->setPlainText(without); run(window); const auto withoutExpected=numbers(report);
      require(polyhedronSettings(*Moderator::Instance()->GetConfiguredObject("FM")).isEmpty(),"Creation fixture already has a polyhedron");
      auto openPanel=[&] {
         auto *tree=window.findChild<QTreeWidget *>("Resources"); const auto items=tree->findItems("FM",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Polyhedron model missing from Resources"); tree->itemDoubleClicked(items.first(),0);
         for (auto *child:window.findChild<QMdiArea *>("workspace")->subWindowList()) if (child->property("resourceName").toString()=="FM") return child;
         throw std::runtime_error("Real polyhedron editor unavailable");
      };
      auto openForces=[&](ResourceEditor &panel,std::function<void(PolyhedronDialog *)> action) {
         std::exception_ptr failure; auto *button=panel.findChild<QPushButton *>("forcePolyhedron"); require(button && button->isEnabled(),"Polyhedron creation button unavailable");
         QTimer::singleShot(0,&panel,[&] { auto *dialog=dynamic_cast<PolyhedronDialog *>(panel.findChild<QDialog *>("polyhedronDialog")); try { require(dialog,"Polyhedron controls unavailable"); action(dialog); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } }); button->click(); if (failure) std::rethrow_exception(failure);
      };
      auto add=[&](PolyhedronDialog *dialog,const QString &body,const QString &path,const QString &density) {
         dialog->findChild<QPushButton *>("polyhedronAdd")->click(); auto *table=dialog->findChild<QTableWidget *>("polyhedronTable"); const auto row=table->rowCount()-1; auto *choice=qobject_cast<QComboBox *>(table->cellWidget(row,0)); require(choice && choice->findText(body)>=0 && choice->findText("Sat")==-1,"Polyhedron choices are not celestial bodies"); choice->setCurrentText(body); table->item(row,1)->setText(path); table->item(row,2)->setText(density);
      };
      auto accept=[](PolyhedronDialog *dialog) { dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); };
      auto apply=[&](QMdiSubWindow *child,ResourceEditor *previous) {
         previous->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
         if (child->widget()==previous) throw std::runtime_error(("Polyhedron Apply rejected: "+previous->findChild<QLabel *>("resourceStatus")->text()).toStdString());
         require(child->isVisible() && !dynamic_cast<EditablePanel *>(child->widget())->hasChanges() && child->property("sourceScript").toString()==editor->toPlainText(),"Polyhedron Apply did not refresh retained panel");
      };
      const auto shapeFile=files.filePath("creation cube ü.txt"); write(shapeFile,valid);
      {
         auto *child=openPanel(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); require(panel,"Force panel unavailable");
         openForces(*panel,[&](PolyhedronDialog *dialog) { require(dialog->findChild<QTableWidget *>("polyhedronTable")->rowCount()==0,"First creation table not empty"); add(dialog,"Earth",shapeFile,"2000"); dialog->reject(); });
         require(!panel->hasChanges() && editor->toPlainText()==without,"Creation Cancel changed pending source");
         openForces(*panel,[&](PolyhedronDialog *dialog) {
            add(dialog,"Earth",shapeFile,"nan"); accept(dialog); require(dialog->isVisible() && !dialog->findChild<QLabel *>("polyhedronError")->text().isEmpty(),"Creation accepted nonfinite density");
            auto *table=dialog->findChild<QTableWidget *>("polyhedronTable"); table->item(0,2)->setText("2000");
            add(dialog,"Earth",shapeFile,"1000"); accept(dialog); require(dialog->isVisible() && dialog->findChild<QLabel *>("polyhedronError")->text().contains("one"),"Duplicate body accepted"); table->selectRow(1); dialog->findChild<QPushButton *>("polyhedronRemove")->click();
            table->selectRow(0); auto *browse=dialog->findChild<QPushButton *>("polyhedronBrowse");
            QTimer::singleShot(0,dialog,[&] { auto *chooser=dialog->findChild<QFileDialog *>(); require(chooser,"Shape creation chooser unavailable"); chooser->selectFile(decoy); chooser->reject(); }); browse->click(); require(table->item(0,1)->text()==shapeFile,"Creation Browse Cancel changed shape");
            require(dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Help),"Polyhedron dialog did not inherit Help");
            if (argc>2) { QEventLoop wait; QTimer::singleShot(250,&wait,&QEventLoop::quit); wait.exec(); require(dialog->windowHandle() && dialog->windowHandle()->isExposed() && dialog->grab().save(QString::fromLocal8Bit(argv[2])+".create.png"),"Native polyhedron controls not exposed or capture failed"); }
            accept(dialog);
         });
         require(panel->hasChanges() && editor->toPlainText()==without && polyhedronSettings(*Moderator::Instance()->GetConfiguredObject("FM")).isEmpty(),"Pending creation changed engine/source"); apply(child,panel);
      }
      const auto created=editor->toPlainText(); require(created.startsWith("% retained polyhedron source α") && created.contains("FM.PointMasses = {Sun}") && !created.contains("FM.PrimaryBodies") && !created.contains("FM.External") && !created.contains("FM.RelativisticCorrection"),"Creation changed unrelated implicit force configuration");
      run(window); equivalent(numbers(report),creationExpected); editor->undo(); require(editor->toPlainText()==without,"Creation Undo not exact"); editor->redo(); require(editor->toPlainText()==created,"Creation Redo not exact");
      const auto createdFile=files.filePath("Created polyhedron Δ.script"); require(window.saveScriptTo(createdFile) && window.loadScript(createdFile),"Created force save/reopen failed"); run(window); equivalent(numbers(report),creationExpected);
      // A rejected mesh leaves both original configuration and Undo history;
      // correct the still-pending dialog and apply through the same panel.
      {
         auto *child=openPanel(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget());
         const auto bad=files.filePath("bad creation cube.txt"); write(bad,"broken shape\n");
         openForces(*panel,[&](PolyhedronDialog *dialog) { dialog->findChild<QTableWidget *>("polyhedronTable")->item(0,1)->setText(bad); accept(dialog); });
         panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); require(child->widget()==panel && panel->hasChanges() && editor->toPlainText()==created,"Malformed creation edit did not roll back"); require(polyhedronSettings(*Moderator::Instance()->GetConfiguredObject("FM"))[0].toObject().value("shape").toString()==shapeFile,"Rejected mesh changed the configured force");
         openForces(*panel,[&](PolyhedronDialog *dialog) { dialog->findChild<QTableWidget *>("polyhedronTable")->item(0,1)->setText(shapeFile); accept(dialog); }); require(!panel->hasChanges(),"Restoring identical polyhedron settings left pending changes");
         // Reverted values retain the dirty-input latch until normal Apply.
         panel->requestApply();
         require(!panel->hasUnappliedInput()&&editor->toPlainText()==created,"Polyhedron no-op Apply failed to accept the reverted draft without source changes");
      }
      auto twoReference=creationReference; twoReference.replace("FM.PolyhedralBodies = {Earth};","FM.PolyhedralBodies = {Earth, Mars};");
      // Qualify source ownership after adding a second body to legacy root
      // aliases: independent reference uses explicit body-qualified settings.
      twoReference.replace("FM.CreateForceBody = Earth;\nFM.ShapeFileName = '"+shape+"';\nFM.BodyDensity = 2000;", "FM.PolyhedronGravityModel.Earth.CreateForceBody = Earth;\nFM.PolyhedronGravityModel.Earth.ShapeFileName = '"+shapeFile+"';\nFM.PolyhedronGravityModel.Earth.BodyDensity = 2000;\nFM.PolyhedronGravityModel.Mars.CreateForceBody = Mars;\nFM.PolyhedronGravityModel.Mars.ShapeFileName = '"+shapeFile+"';\nFM.PolyhedronGravityModel.Mars.BodyDensity = 1000;");
      editor->setPlainText(twoReference); run(window); const auto twoExpected=numbers(report);
      editor->setPlainText(creationReference.replace(shape,shapeFile)); require(window.buildScript(),"Legacy source restore failed"); const auto legacySource=editor->toPlainText();
      {
         auto *child=openPanel(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget());
         openForces(*panel,[&](PolyhedronDialog *dialog) { add(dialog,"Mars",shapeFile,"1000"); accept(dialog); }); apply(child,panel);
      }
      const auto two=editor->toPlainText(); require(two.contains("% retain density comment") && two.contains("FM.PolyhedronGravityModel.Earth.BodyDensity = 2000") && !two.contains("FM.BodyDensity ="),"Second body lost density comment or legacy ownership"); run(window); equivalent(numbers(report),twoExpected);
      editor->undo(); require(editor->toPlainText()==legacySource,"Multiple contributor Undo not exact"); editor->redo(); require(editor->toPlainText()==two,"Multiple contributor Redo not exact");
      const auto twoFile=files.filePath("Two polyhedra ü.script"); require(window.saveScriptTo(twoFile) && window.loadScript(twoFile),"Multiple contributors save/reopen failed"); run(window); equivalent(numbers(report),twoExpected);
      {
         auto *child=openPanel(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget());
         openForces(*panel,[&](PolyhedronDialog *dialog) { auto *table=dialog->findChild<QTableWidget *>("polyhedronTable"); require(table->rowCount()==2,"Two contributors not restored"); table->selectRow(0); dialog->findChild<QPushButton *>("polyhedronRemove")->click(); dialog->reject(); }); require(!panel->hasChanges() && editor->toPlainText()==two,"Removal Cancel changed pending/source");
         openForces(*panel,[&](PolyhedronDialog *dialog) { auto *table=dialog->findChild<QTableWidget *>("polyhedronTable"); for (int row=0;row<table->rowCount();++row) if (qobject_cast<QComboBox *>(table->cellWidget(row,0))->currentText()=="Mars") { table->selectRow(row); break; } dialog->findChild<QPushButton *>("polyhedronRemove")->click(); accept(dialog); }); apply(child,panel);
      }
      run(window); equivalent(numbers(report),creationExpected); const auto one=editor->toPlainText(); require(!one.contains("FM.PolyhedronGravityModel.Mars"),"Removed contributor retained settings");
      {
         auto *child=openPanel(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); openForces(*panel,[&](PolyhedronDialog *dialog) { dialog->findChild<QTableWidget *>("polyhedronTable")->selectRow(0); dialog->findChild<QPushButton *>("polyhedronRemove")->click(); accept(dialog); }); require(panel->hasChanges() && editor->toPlainText()==one,"Final removal changed source before Apply"); apply(child,panel);
      }
      const auto removed=editor->toPlainText(); require(polyhedronSettings(*Moderator::Instance()->GetConfiguredObject("FM")).isEmpty() && !removed.contains("FM.PolyhedronGravityModel.") && removed.contains("% retain density comment"),"Final removal retained force or lost comment"); run(window); equivalent(numbers(report),withoutExpected);
      editor->undo(); require(editor->toPlainText()==one,"Removal Undo not exact"); editor->redo(); require(editor->toPlainText()==removed,"Removal Redo not exact"); const auto removedFile=files.filePath("Removed polyhedra Δ.script"); require(window.saveScriptTo(removedFile) && window.loadScript(removedFile),"Removed forces save/reopen failed"); run(window); equivalent(numbers(report),withoutExpected);
      // Preserve an implicit density and convert the legacy UserDefined
      // creator without leaving a duplicate contributor behind.
      auto legacy=creationReference.replace(shape,shapeFile); legacy.replace("FM.PolyhedralBodies = {Earth};","FM.UserDefined = {PolyhedronGravityModel};"); legacy.remove(QRegularExpression("^FM\\.BodyDensity[^\\n]*(?:\\n|$)",QRegularExpression::MultilineOption)); legacy.prepend("% implicit legacy density\n");
      auto legacyExpectedSource=twoReference; legacyExpectedSource.replace("Earth.BodyDensity = 2000","Earth.BodyDensity = 1000"); editor->setPlainText(legacyExpectedSource); run(window); const auto legacyExpected=numbers(report);
      editor->setPlainText(legacy); run(window);
      {
         auto *child=openPanel(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); openForces(*panel,[&](PolyhedronDialog *dialog) { add(dialog,"Mars",shapeFile,"1000"); accept(dialog); }); apply(child,panel);
      }
      const auto legacyTwo=editor->toPlainText(); require(legacyTwo.contains("FM.UserDefined = {}") && !legacyTwo.contains("Earth.BodyDensity") && legacyTwo.contains("% implicit legacy density") && polyhedronSettings(*Moderator::Instance()->GetConfiguredObject("FM")).size()==2,"Legacy creator duplicated forces or made retained density explicit"); run(window); equivalent(numbers(report),legacyExpected);
      editor->undo(); require(editor->toPlainText()==legacy,"Legacy creator Undo not exact"); editor->redo(); require(editor->toPlainText()==legacyTwo,"Legacy creator Redo not exact");
      const auto legacyFile=files.filePath("Legacy polyhedra Δ.script"); require(window.saveScriptTo(legacyFile) && window.loadScript(legacyFile),"Legacy creator save/reopen failed"); run(window); equivalent(numbers(report),legacyExpected);
      auto wrongBody=legacyTwo; wrongBody.replace("BeginMissionSequence;","FM.PolyhedronGravityModel.Venus.BodyDensity = 1000;\nBeginMissionSequence;"); editor->setPlainText(wrongBody); require(!window.buildScript(),"Qualified setting accepted a body without a contributor"); require(window.loadScript(legacyFile),"Wrong-body source recovery failed"); run(window); equivalent(numbers(report),legacyExpected);
      {
         auto *child=openPanel(); auto *panel=dynamic_cast<ResourceEditor *>(child->widget()); openForces(*panel,[&](PolyhedronDialog *dialog) { dialog->findChild<QTableWidget *>("polyhedronTable")->selectAll(); dialog->findChild<QPushButton *>("polyhedronRemove")->click(); require(dialog->findChild<QTableWidget *>("polyhedronTable")->rowCount()==0,"Multi-row removal skipped contributors"); accept(dialog); }); apply(child,panel);
      }
      run(window); equivalent(numbers(report),withoutExpected);
      std::cout<<"PASS: typed body/shape selection and Cancel, density units, pending Apply, independent cube far-field check and script state agreement, exact Undo/Redo/Unicode save/reopen, invalid density/body/missing/malformed shape rollback and recovery, whitespace and legacy record labels, SurfaceHeight browser, Output access, relative paths and unrelated resource editing; real-panel first and multiple contributor creation/removal, typed bodies and duplicate/invalid-density rejection, pending and Cancel/Help, malformed shape rollback/correction, retained-panel refresh, legacy alias ownership/comments, exact Undo/Redo and independent report/save-reopen equivalence, legacy UserDefined conversion with implicit density retained, wrong-body qualified source recovery and multi-row removal.\n"; return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; }
     catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
