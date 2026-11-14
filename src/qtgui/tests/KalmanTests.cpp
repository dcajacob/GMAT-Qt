#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "CommandForm.hpp"
#include "EpochIntervalDialog.hpp"
#include "TimeSystemConverter.hpp"
#include "Moderator.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QPlainTextEdit>
#include <QDockWidget>
#include <QTemporaryDir>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QRegularExpression>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QTableWidget>
#include <QTreeWidget>
#include <QMdiSubWindow>
#include <QWindow>
#include <QListWidget>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Kalman fixture/report unavailable"); return QString::fromUtf8(file.readAll()); }
static QString diagnostics(MainWindow &window) { QApplication::processEvents(); return window.findChild<QDockWidget *>("messages")->findChild<QPlainTextEdit *>()->toPlainText().right(10000); }
static void run(MainWindow &window) { if (window.runMission()!=MainWindow::RunResult::Completed) throw std::runtime_error(diagnostics(window).toStdString()); }
static QVector<double> values(const QString &path) { QVector<double> result; for (const auto &entry:read(path).simplified().split(' ')) { bool ok; const auto value=entry.toDouble(&ok); require(ok && std::isfinite(value),"State report contains invalid values"); result.append(value); } return result; }
static void same(const QVector<double> &a,const QVector<double> &b) { require(a.size()==b.size(),"State report dimensions changed"); for (int i=0;i<a.size();++i) if (std::abs(a[i]-b[i])>=1e-8) throw std::runtime_error(("GUI changed state column "+QString::number(i)+" by "+QString::number(a[i]-b[i],'g',16)).toStdString()); }
static int row(ResourceEditor &panel,const QString &name) { auto *table=panel.findChild<QTableWidget *>(); for (int r=0;r<table->rowCount();++r) if (table->item(r,0)->text()==name) return r; throw std::runtime_error(("Missing Kalman property: "+name).toStdString()); }
static void later(QWidget *owner,std::exception_ptr &failure,std::function<void()> action) { QTimer::singleShot(0,owner,[owner,&failure,action] { try { action(); } catch (...) { failure=std::current_exception(); for (auto *dialog:owner->findChildren<QDialog *>()) dialog->reject(); } }); }
static void capturePanel(ResourceEditor &panel,const QString &path,const QString &property={})
{
   if (path.isEmpty()) return;
   auto *owner=panel.window(); owner->resize(1050,740); panel.resize(1050,740); owner->show(); panel.show();
   QEventLoop loop; QElapsedTimer elapsed; elapsed.start(); QTimer timer; timer.setInterval(10); QObject::connect(&timer,&QTimer::timeout,&loop,[&] { if ((owner->windowHandle() && owner->windowHandle()->isExposed()) || elapsed.elapsed()>3000) loop.quit(); }); timer.start(); loop.exec();
   require(owner->windowHandle() && owner->windowHandle()->isExposed() && panel.isVisible(),"Native Kalman panel did not expose");
   if (!property.isEmpty()) { auto *table=panel.findChild<QTableWidget *>(); table->scrollToItem(table->item(row(panel,property),0)); }
   QApplication::processEvents(); require(panel.grab().save(path),"Kalman panel capture failed");
}
// Forward each original Apply exactly once; capture invalid-input outcomes
// before an unchanged compound assertion can terminate the fixture.
class DiagnosticWindow final : public MainWindow
{
public:
   bool recordInvalidEdits=false;
   int invalidEdit=0;
   QString applyResourceChanges(const QString &name,const QMap<QString,QString> &changes,const QString &source)
   {
      const auto error=MainWindow::applyResourceChanges(name,changes,source);
      if (recordInvalidEdits) {
         ++invalidEdit; const auto actual=findChild<QPlainTextEdit *>("scriptEditor")->toPlainText();
         QJsonObject inputs; for (auto it=changes.cbegin();it!=changes.cend();++it) inputs[it.key()]=it.value();
         const QJsonObject result{{"resource",name},{"input",inputs},{"error",error},{"source_unchanged",actual==source}};
         std::cerr<<"Invalid Kalman input "<<invalidEdit<<' '<<QJsonDocument(result).toJson(QJsonDocument::Compact).constData()<<'\n';
         const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
         if (!evidence.isEmpty()) {
            const QString prefix=QDir(evidence).filePath("invalid-"+QString::number(invalidEdit));
            const QMap<QString,QByteArray> files{{".json",QJsonDocument(result).toJson()},{"-before.script",source.toUtf8()},{"-after.script",actual.toUtf8()}};
            for (auto it=files.cbegin();it!=files.cend();++it) { QFile file(prefix+it.key()); if (file.open(QIODevice::WriteOnly)) file.write(it.value()); }
         }
      }
      return error;
   }
};
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setAttribute(Qt::AA_DontUseNativeDialogs); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtKalman"); if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Kalman fixtures unavailable"); DiagnosticWindow window; window.show(); require(window.initialize(startup),"Kalman runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      auto fixture=read(QDir::current().absoluteFilePath("../samples/Navigation/Ex_FilterSmoother_GpsPosVec.script"));
      auto replace=[&](const QString &key,const QString &value) { const QRegularExpression assignment("^"+QRegularExpression::escape(key)+"[ \\t]*=[^;\\n]*(?:;)?",QRegularExpression::MultilineOption); require(assignment.match(fixture).hasMatch(),"Shipped Kalman setting missing"); fixture.replace(assignment,key+" = "+value+";"); };
      const auto measurements=files.filePath("gps ü.gmd"),filterReport=files.filePath("filter.txt"),smoothReport=files.filePath("smoother.txt"),warm=files.filePath("warm.csv"),states=files.filePath("states.txt");
      fixture.replace("'Ex_FilterSmoother_GpsPosVec.gmd'","'"+measurements+"'"); replace("Sim.FinalEpoch","'10 Jun 2010 01:00:00.000'"); replace("Sim.AddNoise","Off"); replace("EKF.ReportFile","'"+filterReport+"'"); replace("FPS.ReportFile","'"+smoothReport+"'"); replace("EKF.OutputWarmStartFile","'"+warm+"'"); replace("FilterCdReport.Filename","'"+files.filePath("cd.csv")+"'");
      fixture.replace("BeginMissionSequence","Create ReportFile StateReport;\nStateReport.Filename = '"+states+"';\nStateReport.WriteHeaders = false;\nStateReport.Precision = 16;\nBeginMissionSequence");
      const QString state="EstSat.A1ModJulian EstSat.X EstSat.Y EstSat.Z EstSat.VX EstSat.VY EstSat.VZ EstSat.Cd";
      fixture.replace("RunEstimator EKF;","RunEstimator 'Fit GPS' EKF; % keep filter comment\nReport StateReport "+state+";"); fixture.replace("RunSmoother FPS;","RunSmoother 'Smooth GPS' FPS; % keep smoother comment\nReport StateReport "+state+";");
      editor->setPlainText(fixture); run(window); const auto expected=values(states); require(expected.size()==16,"Filter/smoother state report dimensions wrong"); require(QFileInfo(measurements).size()>0 && QFileInfo(filterReport).size()>0 && QFileInfo(smoothReport).size()>0 && QFileInfo(warm).size()>0,"Kalman outputs missing");
      const auto observations=read(measurements),warmReference=read(warm),saved=files.filePath("filter smoother ü.script");
      auto base=fixture; base.replace("SNC.AccelNoiseSigma  = [1.0e-9 1.0e-8 1.0e-8];","SNC.AccelNoiseSigma = [2e-9 2e-8 2e-8];"); base.replace("FogmCd.HalfLife           = 86400","FogmCd.HalfLife = 43200"); base.replace("EKF.OutputWarmStartFile = '"+warm+"';","EKF.OutputWarmStartFile = '';"); base.replace("{CartesianState, FogmCd}","{CartesianState}");
      require(base!=fixture,"Editable Kalman fixture unchanged"); editor->setPlainText(base); require(window.buildScript(),"Editable Kalman fixture rejected"); std::exception_ptr failure; QString error;
      auto applyPanel=[&](const QString &name,const std::function<void(ResourceEditor &)> &edit) {
         const auto source=editor->toPlainText(); QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject(name.toStdString()),[&](const auto &changes) { error=window.applyResourceChanges(name,changes,source); return error; },&owner,source); panel.show(); edit(panel); require(panel.hasChanges(),"Kalman edit not pending"); error="Apply not invoked"; panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); if (!error.isEmpty()) throw std::runtime_error((name+": "+error+"\n"+diagnostics(window)).toStdString()); const auto applied=editor->toPlainText(); editor->undo(); require(editor->toPlainText()==source,"Kalman Undo changed source"); editor->redo(); require(editor->toPlainText()==applied,"Kalman Redo changed source");
      };
      auto *resources=window.findChild<QTreeWidget *>("Resources");
      for (const auto &name:QStringList{"SNC","FogmCd","EKF","FPS"}) {
         const auto items=resources->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Kalman resource inaccessible from Resources"); resources->itemDoubleClicked(items.first(),0);
         for (auto *child:window.findChildren<QMdiSubWindow *>()) if (child->property("resourceName").toString()==name) { require(child->findChild<QTableWidget *>(),"Kalman resource panel missing"); child->close(); }
         QApplication::processEvents();
      }
      for (const auto &entry:QList<QPair<QString,QString>>{{"FPS","Filter"},{"EstSat","ProcessNoiseModel"}}) {
         QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject(entry.first.toStdString()),[](const auto &) { return QString(); },&owner,editor->toPlainText()); auto *choose=panel.findChild<QPushButton *>("chooseProperty_"+entry.second); if (!choose) throw std::runtime_error(("Missing selector: "+entry.first+"."+entry.second).toStdString());
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QInputDialog *>(); require(dialog && dialog->comboBoxItems().contains(entry.first=="FPS" ? "EKF" : "SNC") && !dialog->comboBoxItems().contains("Sim") && !dialog->comboBoxItems().contains("FPS"),"Kalman reference selector includes wrong resource types"); dialog->reject(); }); choose->click(); if (failure) std::rethrow_exception(failure); require(!panel.hasChanges(),"Kalman reference Cancel changed source");
      }
      applyPanel("SNC",[&](ResourceEditor &panel) {
         auto *table=panel.findChild<QTableWidget *>(); require(row(panel,"CoordinateSystem")>=0 && row(panel,"AccelNoiseSigma")>=0 && row(panel,"UpdateTimeStep")>=0,"Owned process-noise properties hidden");
         auto *type=qobject_cast<QComboBox *>(table->cellWidget(row(panel,"Type"),1)); require(type && type->findText("StateNoiseCompensation")>=0 && type->findText("LinearTime")<0,"Process-noise choices include a testing-only model");
         require(table->item(row(panel,"AccelNoiseSigma"),2)->text()=="km/s^(3/2)","Noise units missing"); table->item(row(panel,"AccelNoiseSigma"),1)->setText("1e-9 1e-8 1e-8");
         auto *choose=panel.findChild<QPushButton *>("chooseProperty_CoordinateSystem"); require(choose,"Process-noise frame selector missing");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QInputDialog *>(); require(dialog && dialog->comboBoxItems().contains("EstSatVNB") && !dialog->comboBoxItems().contains("EKF"),"Noise frame picker includes wrong resource types"); dialog->setTextValue("EarthMJ2000Eq"); dialog->reject(); }); choose->click(); if (failure) std::rethrow_exception(failure);
         require(table->item(row(panel,"CoordinateSystem"),1)->text()=="EstSatVNB","Noise frame Cancel changed selection");
         capturePanel(panel,capture.isEmpty() ? QString() : capture+".snc.png");
      });
      applyPanel("FogmCd",[&](ResourceEditor &panel) {
         auto *table=panel.findChild<QTableWidget *>(); auto *solve=qobject_cast<QComboBox *>(table->cellWidget(row(panel,"SolveFor"),1)); require(solve && solve->findText("Cd")>=0 && solve->findText("AtmosDensityScaleFactor")>=0 && solve->findText("Cr")<0,"Gauss-Markov solve-for choices mismatch the model"); require(row(panel,"SteadyStateValue")>=0 && row(panel,"SteadyStateSigma")>=0,"Owned Gauss-Markov properties hidden"); require(table->item(row(panel,"HalfLife"),2)->text()=="s","Half-life unit missing"); table->item(row(panel,"HalfLife"),1)->setText("86400");
         capturePanel(panel,capture.isEmpty() ? QString() : capture+".fogm.png");
      });
      applyPanel("EstSat",[&](ResourceEditor &panel) {
         auto *choose=panel.findChild<QPushButton *>("chooseProperty_SolveFors"); require(choose,"Spacecraft solve-for selector missing");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("resourceSelectionDialog"); require(dialog,"Solve-for dialog missing"); auto *list=dialog->findChild<QListWidget *>("resourceSelectionList"); bool found=false; for (int i=0;i<list->count();++i) if (list->item(i)->text()=="FogmCd") { found=true; list->item(i)->setCheckState(Qt::Checked); } require(found,"Estimated parameter missing from spacecraft solve-for choices"); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); }); choose->click(); if (failure) std::rethrow_exception(failure);
      });
      applyPanel("EKF",[&](ResourceEditor &panel) {
         const auto props=resourceProperties(*Moderator::Instance()->GetConfiguredObject("EKF"));
         for (const auto &name:QStringList{"InputWarmStartFile","OutputWarmStartFile"}) { bool found=false; for (const auto &field:props) if (field.name==name) { found=field.filename && (name.startsWith("Input") ? field.fileInput : field.fileOutput); } require(found,"Warm-start file semantics missing"); }
         auto *choose=panel.findChild<QPushButton *>("chooseProperty_OutputWarmStartFile"); require(choose,"Warm-start output picker missing");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QFileDialog *>(); require(dialog && dialog->acceptMode()==QFileDialog::AcceptSave,"Warm output picker is not a save dialog"); dialog->selectFile(warm); dialog->reject(); }); choose->click(); if (failure) std::rethrow_exception(failure); require(!panel.hasChanges(),"Warm output Cancel changed resource");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QFileDialog *>(); dialog->setOption(QFileDialog::DontConfirmOverwrite); dialog->selectFile(warm); QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection); }); choose->click(); if (failure) std::rethrow_exception(failure);
         capturePanel(panel,capture.isEmpty() ? QString() : capture+".ekf.png","InputWarmStartFile");
      });
      require(window.saveScriptTo(saved) && window.loadScript(saved),"Unicode Kalman save/reopen failed"); run(window); same(values(states),expected); require(read(measurements)==observations && read(warm)==warmReference,"GUI settings changed observations or warm-start covariance/state output");
      auto *outputs=window.findChild<QTreeWidget *>("Output"); for (const auto &name:QStringList{"EKF","FPS"}) { const auto items=outputs->findItems(name,Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Filter/smoother report absent from Output"); outputs->itemDoubleClicked(items.first(),0); auto *viewer=window.findChild<QPlainTextEdit *>("report:"+name); require(viewer && !viewer->toPlainText().isEmpty(),"Filter/smoother report viewer empty"); }
      for (const auto &entry:QList<QPair<QString,QString>>{{"RunEstimator","EKF"},{"RunSmoother","FPS"}}) {
         QString changed; CommandForm form([&](const QString &text) { changed=text; }); const QString type=entry.first=="RunEstimator" ? "Estimator" : "Smoother"; form.setStatement(entry.first+" 'Keep label' Missing; % keep comment"); auto *button=form.findChild<QPushButton *>("commandChoose_"+type); require(button,"Kalman command selector missing");
         later(&form,failure,[&] { auto *dialog=form.findChild<QInputDialog *>(); require(dialog && dialog->comboBoxItems().contains(entry.second) && !dialog->comboBoxItems().contains(entry.second=="EKF" ? "FPS" : "EKF") && !dialog->comboBoxItems().contains("Sim"),"Kalman command selector includes wrong solver types"); dialog->setTextValue(entry.second); dialog->accept(); }); button->click(); if (failure) std::rethrow_exception(failure); require(changed==entry.first+" 'Keep label' "+entry.second+"; % keep comment","Kalman command selector lost label/comment");
         const auto snapshot=window.missionSnapshot(); int index=-1; for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type==entry.first) index=i; require(index>=0,"Kalman mission command missing"); const auto error=window.applyMissionChange(snapshot,index,MissionEdit::Replace,changed); if (!error.isEmpty()) throw std::runtime_error((entry.first+": "+error+"\n"+diagnostics(window)).toStdString());
      }
      run(window); same(values(states),expected);
      // Restore observations without a simulator: failed input must not be
      // silently recreated by the mission being tested.
      auto estimationOnly=fixture; estimationOnly.remove(QRegularExpression("^RunSimulator[^\\n]*\\n",QRegularExpression::MultilineOption));
      editor->setPlainText(estimationOnly); require(window.buildScript(),"Filter-only missing-file fixture rejected"); require(QFile::rename(measurements,measurements+".held"),"Observation failure fixture unavailable"); require(window.runMission()==MainWindow::RunResult::Failed && diagnostics(window).contains(QFileInfo(measurements).fileName()),"Missing GPS observations did not fail clearly"); require(QFile::rename(measurements+".held",measurements),"Observation restore failed"); run(window); same(values(states),expected);
      const auto seed=files.filePath("seed.csv"); require(QFile::copy(warm,seed),"Warm-start seed copy failed");
      auto warmBase=estimationOnly; warmBase.remove(QRegularExpression("^RunSmoother[^\\n]*\\n",QRegularExpression::MultilineOption));
      const auto seedAssignments="EKF.InputWarmStartFile = '"+seed+"';\nEKF.WarmStartEpochFormat = 'UTCGregorian';\nEKF.WarmStartEpoch = '10 Jun 2010 00:00:00.000';\n";
      auto warmFixture=warmBase; warmFixture.replace("BeginMissionSequence",seedAssignments+"BeginMissionSequence"); editor->setPlainText(warmFixture); run(window); const auto warmExpected=values(states); const auto warmOutput=read(warm); std::cout<<"Warm reference passed\n";
      editor->setPlainText(warmBase); require(window.buildScript(),"Warm-start editable fixture rejected");
      applyPanel("EKF",[&](ResourceEditor &panel) {
         auto *table=panel.findChild<QTableWidget *>(); auto *choose=panel.findChild<QPushButton *>("chooseProperty_InputWarmStartFile"); require(choose,"Warm-start input picker missing");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QFileDialog *>(); require(dialog && dialog->fileMode()==QFileDialog::ExistingFile && dialog->acceptMode()==QFileDialog::AcceptOpen,"Warm input picker must select an existing file"); dialog->selectFile(seed); QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection); }); choose->click(); if (failure) std::rethrow_exception(failure);
         auto *format=qobject_cast<QComboBox *>(table->cellWidget(row(panel,"WarmStartEpochFormat"),1)); require(format,"Warm-start date format selector missing"); auto *epoch=table->item(row(panel,"WarmStartEpoch"),1);
         const auto sentinel=epoch->text(); format->setCurrentText("UTCGregorian"); require(epoch->text()==sentinel,"Warm-start sentinel converted as a date"); epoch->setText("10 Jun 2010 00:00:00.000");
         for (const auto &name:TimeSystemConverter::Instance()->GetValidTimeRepresentations()) format->setCurrentText(QString::fromStdString(name)); format->setCurrentText("UTCGregorian"); require(epoch->text()=="10 Jun 2010 00:00:00.000","Warm-start format cycle changed physical epoch");
         epoch->setText("bad date"); format->setCurrentText("TAIModJulian"); require(format->currentText()=="UTCGregorian" && epoch->text()=="bad date","Failed warm-start conversion partially changed fields"); epoch->setText("10 Jun 2010 00:00:00.000"); format->setCurrentText("TAIModJulian");
         capturePanel(panel,capture.isEmpty() ? QString() : capture+".warm.png","InputWarmStartFile");
      });
      const auto warmSaved=files.filePath("warm-start ü.script"); require(window.saveScriptTo(warmSaved) && window.loadScript(warmSaved),"Warm-start mission save/reopen failed"); run(window); same(values(states),warmExpected); require(read(warm)==warmOutput,"Warm-start GUI changed state/covariance output"); std::cout<<"Warm GUI passed\n";
      const auto source=editor->toPlainText(); require(window.applyResourceChanges("EKF",{{"WarmStartEpochFormat","UTCGregorian"}},source).isEmpty(),"Warm-start format-only edit failed"); run(window); same(values(states),warmExpected);
      require(QFile::rename(seed,seed+".held"),"Warm-start missing-file fixture unavailable"); require(window.runMission()==MainWindow::RunResult::Failed && diagnostics(window).contains("seed.csv"),"Missing warm-start file did not fail clearly"); require(QFile::rename(seed+".held",seed),"Warm-start restore failed"); run(window); same(values(states),warmExpected);
      const auto seedData=read(seed); { QFile file(seed); require(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"Malformed warm-start fixture unavailable"); file.write("NotAnEpoch,State\n0,1\n"); }
      require(window.runMission()==MainWindow::RunResult::Failed && diagnostics(window).contains("no epoch column"),"Malformed warm-start header did not fail clearly"); { QFile file(seed); require(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"Warm-start restore unavailable"); file.write(seedData.toUtf8()); } run(window); same(values(states),warmExpected);
      // FirstMeasurement requires a seed strictly before the first observed
      // epoch. The original file starts at the same epoch and must fail; then
      // use the remaining six observations to exercise a valid continuation.
      auto noPrior=warmFixture; noPrior.replace("EKF.WarmStartEpoch = '10 Jun 2010 00:00:00.000';","EKF.WarmStartEpoch = 'FirstMeasurement';"); editor->setPlainText(noPrior); require(window.runMission()==MainWindow::RunResult::Failed && diagnostics(window).contains("no epoch found"),"Warm start without a prior seed did not fail clearly"); editor->setPlainText(warmFixture); run(window); same(values(states),warmExpected);
      auto continued=observations; const auto first=QRegularExpression("^[ \\t]*[0-9]+\\.[0-9]+[^\\n]*GPS_PosVec[^\\n]*\\n",QRegularExpression::MultilineOption).match(continued); require(first.hasMatch(),"GPS continuation record unavailable"); continued.remove(first.capturedStart(),first.capturedLength()); { QFile file(measurements); require(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"GPS continuation file unavailable"); file.write(continued.toUtf8()); }
      for (const auto &boundary:QStringList{"FirstMeasurement","LastWarmStartRecord"}) {
         std::cout<<"Boundary "<<boundary.toStdString()<<"\n";
         auto reference=warmFixture; reference.replace("EKF.WarmStartEpoch = '10 Jun 2010 00:00:00.000';","EKF.WarmStartEpoch = '"+boundary+"';");
         if (boundary=="LastWarmStartRecord") {
            editor->setPlainText(reference); require(window.runMission()==MainWindow::RunResult::Failed && diagnostics(window).contains("No measurements remain"),"End-of-data warm start did not fail clearly");
            auto rows=seedData.trimmed().split('\n'); const auto lastEpoch=rows.last().section(',',0,0); while (rows.size()>1 && rows.last().startsWith(lastEpoch+",")) rows.removeLast(); require(rows.size()>1,"Earlier warm-start record unavailable"); QFile file(seed); require(file.open(QIODevice::WriteOnly|QIODevice::Truncate),"Warm continuation seed unavailable"); file.write((rows.join('\n')+'\n').toUtf8());
         }
         editor->setPlainText(reference); run(window); const auto boundaryExpected=values(states);
         require(window.loadScript(warmSaved) && window.buildScript(),"Boundary fixture restore failed"); applyPanel("EKF",[&](ResourceEditor &panel) { auto *choose=panel.findChild<QPushButton *>("chooseProperty_WarmStartEpoch"); require(choose,"Warm-start boundary selector missing"); later(&panel,failure,[&] { auto *dialog=panel.findChild<QInputDialog *>(); require(dialog && dialog->comboBoxItems().contains("FirstMeasurement") && dialog->comboBoxItems().contains("LastWarmStartRecord"),"Warm-start boundaries missing"); dialog->setTextValue(boundary); dialog->accept(); }); choose->click(); if (failure) std::rethrow_exception(failure); }); run(window); same(values(states),boundaryExpected);
      }
      require(window.loadScript(saved),"Final cold-start restore failed"); run(window); same(values(states),expected); require(read(measurements)==observations,"Cold-start rerun did not recover all observations");
      // Unicode filenames are supported by the repaired filename contract.
      // Exercise real estimator output rather than labeling a valid path bad.
      const auto unicodeOutput=files.filePath("supported ü.csv");
      require(window.applyResourceChanges("EKF",{{"OutputWarmStartFile",unicodeOutput}},editor->toPlainText()).isEmpty(),"Unicode warm-start output path was rejected");
      run(window); same(values(states),expected);
      require(read(unicodeOutput)==warmReference&&read(measurements)==observations,"Unicode warm-start output changed covariance/state bytes or observations");
      const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      if (!evidence.isEmpty()) {
         QFile csv(QDir(evidence).filePath("unicode-warm.csv")); require(csv.open(QIODevice::WriteOnly),"Unicode covariance evidence unavailable"); csv.write(read(unicodeOutput).toUtf8());
         QFile reference(QDir(evidence).filePath("reference-warm.csv")); require(reference.open(QIODevice::WriteOnly),"Reference covariance evidence unavailable"); reference.write(warmReference.toUtf8());
      }
      require(window.loadScript(saved) && window.buildScript(),"Cold-start fixture restore failed");
      window.recordInvalidEdits=true;
      for (const auto &bad:QList<QPair<QString,QMap<QString,QString>>>{{"SNC",{{"AccelNoiseSigma","0 1e-8 1e-8"}}},{"SNC",{{"AccelNoiseSigma","1e-9 1e-8"}}},{"SNC",{{"CoordinateSystem","EKF"}}},{"SNC",{{"Type","Bogus"}}},{"SNC",{{"UpdateTimeStep","-1"}}},{"FogmCd",{{"HalfLife","0"}}},{"FogmCd",{{"SteadyStateSigma","-1"}}},{"FogmCd",{{"SolveFor","Cr"}}},{"FPS",{{"Filter","Sim"}}},{"EKF",{{"WarmStartEpochFormat","Bogus"}}},{"EKF",{{"WarmStartEpoch","bad epoch"}}},{"EKF",{{"OutputWarmStartFile",files.filePath("invalid\nfile.csv")}}}}) {
         const auto source=editor->toPlainText(); require(!window.applyResourceChanges(bad.first,bad.second,source).isEmpty() && editor->toPlainText()==source,"Invalid Kalman edit changed source");
      }
      run(window); same(values(states),expected);
      std::cout<<"PASS: one-hour GPS filter/smoother GUI with SNC and Gauss-Markov drag, typed references/solve-fors/run commands, file picker Cancel, independent states and exact covariance CSV, report viewers, paired warm-start epoch conversion and both continuation boundaries, exact Undo/Redo and Unicode mission reopen, invalid edit rollback and missing/malformed/late-seed recovery\n";
   } catch (BaseException &error) { std::cerr<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
   return 0;
}
