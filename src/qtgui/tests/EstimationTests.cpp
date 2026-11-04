#include "MainWindow.hpp"
#include "AcceptanceView.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include "TrackingConfigDialog.hpp"
#include "EpochIntervalDialog.hpp"
#include "TimeSystemConverter.hpp"
#include "CommandForm.hpp"
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
#include <QElapsedTimer>
#include <QEventLoop>
#include <QWindow>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QTreeWidget>
#include <QMdiSubWindow>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QScreen>
#include <QScrollArea>
#include <QTabWidget>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QJsonArray estimationViews,estimationComparisons;
static int successfulEstimationRuns=0;
static void estimationView(QWidget &widget) {
   if(qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")!="visible")return;
   auto *window=widget.window();
   if(window!=&widget&&!window->layout()&&!qobject_cast<QMdiSubWindow *>(window)) {
      window->resize(widget.sizeHint().expandedTo(QSize(1100,700)).boundedTo(window->screen()->availableGeometry().size()-QSize(40,60)));
      widget.setGeometry(window->rect());
   }
   acceptanceView(widget);
   require(widget.isVisible()&&window->windowHandle()&&window->windowHandle()->isExposed(),"Estimation viewing boundary did not expose");
   estimationViews.append(QJsonObject{{"stage",estimationViews.size()+1},{"class",QString::fromLatin1(widget.metaObject()->className())},{"name",widget.objectName()},{"visible",widget.isVisible()},{"exposed",window->windowHandle()->isExposed()}});
}
static void estimationReveal(QWidget &control) {
   if(qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")!="visible")return;
   for(auto *parent=&control;parent;parent=parent->parentWidget()) {
      for(auto *tabs:control.window()->findChildren<QTabWidget *>())if(tabs->indexOf(parent)>=0)tabs->setCurrentWidget(parent);
      if(auto *scroll=qobject_cast<QScrollArea *>(parent))scroll->ensureWidgetVisible(&control);
   }
   QApplication::processEvents();estimationView(control);
}
static void retain(const QString &name,const QByteArray &bytes) {
   const auto root=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(root.isEmpty())return;
   QFile file(root+"/"+name);require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Estimation evidence write failed");
}
static void retainFile(const QString &name,const QString &path) {
   QFile file(path);require(file.open(QIODevice::ReadOnly),"Estimation evidence source missing");retain(name,file.readAll());
}
static void recordComparison(const QVector<double> &actual,const QVector<double> &expected,const char *stage) {
   QJsonArray a,b;for(double value:actual)a.append(value);for(double value:expected)b.append(value);
   estimationComparisons.append(QJsonObject{{"stage",QString::fromUtf8(stage)},{"actual",a},{"expected",b},{"tolerance",1e-8}});
   retain("state-comparisons.json",QJsonDocument(estimationComparisons).toJson());
}
static QString read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Estimation fixture/report unavailable"); return QString::fromUtf8(file.readAll()); }
static QString diagnostics(MainWindow &window) { QApplication::processEvents(); return window.findChild<QDockWidget *>("messages")->findChild<QPlainTextEdit *>()->toPlainText().right(6000); }
static void run(MainWindow &window) { if (window.runMission()!=MainWindow::RunResult::Completed) throw std::runtime_error(diagnostics(window).toStdString()); ++successfulEstimationRuns; }
static QVector<double> values(const QString &path) { QVector<double> result; for (const auto &entry:read(path).simplified().split(' ')) { bool ok; const auto value=entry.toDouble(&ok); require(ok && std::isfinite(value),"State report contains invalid values"); result.append(value); } return result; }
static void same(const QVector<double> &a,const QVector<double> &b,const char *stage="GUI configuration") { require(a.size()==b.size(),"State report dimensions changed"); for (int i=0;i<a.size();++i) if (std::abs(a[i]-b[i])>=1e-8) throw std::runtime_error((QString(stage)+" changed state column "+QString::number(i)+" by "+QString::number(a[i]-b[i],'g',16)).toStdString()); recordComparison(a,b,stage); }
static void later(QWidget *owner,std::exception_ptr &failure,std::function<void()> action) { QTimer::singleShot(0,owner,[owner,&failure,action] { try { if(qEnvironmentVariable("GMAT_QT_ACCEPTANCE_MODE")=="visible") { auto *target=QApplication::activeModalWidget();if(!target)target=QApplication::activePopupWidget();require(target,"Estimation modal driver has no active window");estimationView(*target);if(auto *picker=qobject_cast<QFileDialog *>(target)){auto *filename=picker->findChild<QLineEdit *>("fileNameEdit");require(filename,"Estimation filename edit missing");filename->clearFocus();require(!filename->hasFocus(),"Estimation filename edit remains focus-blocked");} } action(); } catch (...) { failure=std::current_exception(); for (auto *dialog:owner->findChildren<QDialog *>()) dialog->reject(); } }); }
static void close(QDialog *dialog,bool accept=true) { dialog->findChild<QDialogButtonBox *>()->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click(); }
static int row(ResourceEditor &panel,const QString &name) { auto *table=panel.findChild<QTableWidget *>(); for (int r=0;r<table->rowCount();++r) if (table->item(r,0)->text()==name) return r; throw std::runtime_error(("Missing estimation property: "+name).toStdString()); }
static bool waitUntil(const std::function<bool()> &ready) { if (ready()) return true; QEventLoop loop; QElapsedTimer elapsed; elapsed.start(); QTimer timer; timer.setInterval(10); QObject::connect(&timer,&QTimer::timeout,&loop,[&] { if (ready() || elapsed.elapsed()>3000) loop.quit(); }); timer.start(); loop.exec(); return ready(); }
int main(int argc,char **argv)
{
   QApplication app(argc,argv); app.setAttribute(Qt::AA_DontUseNativeDialogs); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtEstimation"); if (argc!=2 && argc!=3) return 2;
   const auto startup=QFileInfo(argv[1]).absoluteFilePath(),capture=argc==3 ? QFileInfo(argv[2]).absoluteFilePath() : QString(); QDir::setCurrent(QFileInfo(startup).absolutePath());
   try {
      TestSettings settings; QTemporaryDir files; require(files.isValid(),"Estimation fixtures unavailable"); MainWindow window; window.show(); require(window.initialize(startup),"Estimation runtime unavailable"); estimationView(window); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      auto fixture=read(QDir::current().absoluteFilePath("../samples/Navigation/Ex_Estimate_RangeSkin.script"));
      auto replace=[&](const QString &key,const QString &value) { const QRegularExpression assignment("^"+QRegularExpression::escape(key)+"\\s*=[^;\\n]*;",QRegularExpression::MultilineOption); require(assignment.match(fixture).hasMatch(),"Shipped estimation setting missing"); fixture.replace(assignment,key+" = "+value+";"); };
      auto measurements=files.filePath("skin.gmd"); auto report=files.filePath("estimation.txt"); const auto states=files.filePath("states.txt");
      fixture.replace("'Ex_Estimate_RangeSkin.gmd'","'"+measurements+"'"); replace("sim.InitialEpoch","'10 Jun 2012 00:00:00.000'"); replace("sim.FinalEpoch","'10 Jun 2012 08:00:00.000'"); replace("sim.MeasurementTimeStep","60"); replace("sim.AddNoise","Off"); replace("bat.ReportFile","'"+report+"'");
      replace("bat.OLSEAdditiveConstant","1");
      fixture.replace("BeginMissionSequence","Create Propagator AlternateProp;\nAlternateProp.FM = ODProp_ForceModel;\nAlternateProp.Type = 'RungeKutta89';\nAlternateProp.InitialStepSize = 60;\nAlternateProp.Accuracy = 1e-13;\nAlternateProp.MinStep = 0;\nAlternateProp.MaxStep = 60;\nBeginMissionSequence");
      replace("sim.Propagator","ODProp;\nsim.Propagator = {ODProp, SimSat}");
      replace("bat.Propagator","ODProp;\nbat.Propagator = {ODProp, EstSat}");
      fixture.replace("BeginMissionSequence","Create ReportFile StateReport;\nStateReport.Filename = '"+states+"';\nStateReport.WriteHeaders = false;\nStateReport.Precision = 16;\nBeginMissionSequence");
      fixture.replace("RunSimulator sim;","RunSimulator 'Simulate tracking' sim; % preserve simulation comment"); fixture.replace("RunEstimator bat;","RunEstimator 'Fit orbit' bat; % preserve estimation comment");
      fixture+="\nReport StateReport SimSat.A1ModJulian SimSat.X SimSat.Y SimSat.Z SimSat.VX SimSat.VY SimSat.VZ EstSat.A1ModJulian EstSat.X EstSat.Y EstSat.Z EstSat.VX EstSat.VY EstSat.VZ;\n";
      editor->setPlainText(fixture); run(window); require(QFileInfo(measurements).size()>0 && QFileInfo(report).size()>0,"Simulation/estimation outputs missing"); const auto expected=values(states); const auto data=read(measurements); require(expected.size()==14,"State report columns incorrect"); retainFile("baseline-report.txt",report);retainFile("baseline-states.txt",states);retainFile("baseline-observations.gmd",measurements);retain("baseline.script",fixture.toUtf8());
      const double truth[]={576.869556,-5701.142761,-4170.593691,-1.76450794,4.18128798,-5.96578986};
      for (int i=0;i<6;++i) require(std::abs(expected[8+i]-truth[i])<(i<3 ? 1e-5 : 1e-8),"Noise-free estimated initial state does not recover input truth");
      const auto baseline=files.filePath("reference.script"),saved=files.filePath("estimation ü.script"); require(window.saveScriptTo(baseline),"Baseline save failed");
      auto base=fixture; base.remove(QRegularExpression("^.*AddTrackingConfig.*(?:CAN|MAD).*\\n",QRegularExpression::MultilineOption));
      base.replace(QRegularExpression("^RangeModel.NoiseSigma[^\\n]*",QRegularExpression::MultilineOption),"RangeModel.NoiseSigma = 0.02;"); base.replace(QRegularExpression("^EstSat.SolveFors[^\\n]*",QRegularExpression::MultilineOption),"EstSat.SolveFors = {Cr};");
      base.remove(QRegularExpression("^(GDS|CAN|MAD).ErrorModels[^\\n]*\\n",QRegularExpression::MultilineOption));
      base.replace(QRegularExpression("^sim.AddData[^\\n]*",QRegularExpression::MultilineOption),"sim.AddData = {estData};"); base.replace(QRegularExpression("^bat.Measurements[^\\n]*",QRegularExpression::MultilineOption),"bat.Measurements = {simData};");
      editor->setPlainText(base); if (!window.buildScript()) throw std::runtime_error(diagnostics(window).toStdString()); QString error; std::exception_ptr failure;
      auto *resources=window.findChild<QTreeWidget *>("Resources");
      for (const auto &name:QStringList{"simData","estData","RangeModel"}) require(resources->findItems(name,Qt::MatchExactly|Qt::MatchRecursive).size()==1,"Estimation resource inaccessible from Resources tree");
      resources->itemDoubleClicked(resources->findItems("simData",Qt::MatchExactly|Qt::MatchRecursive).first(),0);
      auto *visibleButton=window.findChild<QPushButton *>("editTrackingConfigs"); require(visibleButton,"Resource navigation does not open tracking controls");
      for (auto *child:window.findChildren<QMdiSubWindow *>()) if (child->property("resourceName").toString()=="simData") child->close(); QApplication::processEvents();
      auto applyPanel=[&](const QString &name,const std::function<void(ResourceEditor &)> &edit) {
         const auto source=editor->toPlainText(); QWidget owner; ResourceEditor panel(*Moderator::Instance()->GetConfiguredObject(name.toStdString()),[&](const auto &changes) { error=window.applyResourceChanges(name,changes,source); return error; },&owner,source); estimationView(panel); edit(panel); estimationView(panel); require(panel.hasChanges(),"Estimation edit not pending"); error="Apply not invoked"; panel.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); estimationView(panel); if (!error.isEmpty()) throw std::runtime_error((name+": "+error+"\n"+diagnostics(window)).toStdString()); const auto applied=editor->toPlainText(); editor->undo(); require(editor->toPlainText()==source,"Estimation Undo changed unrelated source"); editor->redo(); require(editor->toPlainText()==applied,"Estimation Redo changed unrelated source");
      };
      for (const auto &entry:QList<QPair<QString,QString>>{{"simData","SimSat"},{"estData","EstSat"}}) {
         const auto name=entry.first,sat=entry.second;
         applyPanel(name,[&](ResourceEditor &panel) {
            auto *button=panel.findChild<QPushButton *>("editTrackingConfigs"); require(button,"Tracking configuration editor missing"); const auto old=trackingConfigurations(*Moderator::Instance()->GetConfiguredObject(name.toStdString()));
            later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("trackingConfigDialog"); require(dialog,"Tracking dialog missing"); dialog->findChild<QPushButton *>("trackingAdd")->click(); close(dialog,false); }); button->click(); if (failure) std::rethrow_exception(failure); require(!panel.hasChanges(),"Tracking Cancel changed resource");
            later(&panel,failure,[&] {
               auto *dialog=panel.findChild<QDialog *>("trackingConfigDialog"); auto *table=dialog->findChild<QTableWidget *>("trackingConfigs"); require(table->rowCount()==1,"Existing tracking configuration not loaded");
               table->setCurrentCell(0,0);
               later(dialog,failure,[&] { auto *path=dialog->findChild<QDialog *>("trackingPathDialog"); auto *list=path->findChild<QListWidget *>("trackingSignalPath"); list->clear(); auto *choices=path->findChild<QComboBox *>("trackingParticipant"); require(choices->findText("Earth")<0 && choices->findText("GDS.Antenna1")>=0,"Participant choices omit attached hardware or include planets"); for (const auto &name:QStringList{"GDS",sat,"GDS"}) { choices->setCurrentText(name); path->findChild<QPushButton *>("trackingParticipantAdd")->click(); } require(list->count()==3 && list->item(0)->text()==list->item(2)->text(),"Repeated path participant not retained"); close(path); }); dialog->findChild<QPushButton *>("trackingPath")->click(); if (failure) std::rethrow_exception(failure);
               const auto path=table->item(0,0)->text(); later(dialog,failure,[&] { auto *path=dialog->findChild<QDialog *>("trackingPathDialog"); path->findChild<QListWidget *>("trackingSignalPath")->clear(); close(path,false); }); dialog->findChild<QPushButton *>("trackingPath")->click(); if (failure) std::rethrow_exception(failure); require(table->item(0,0)->text()==path,"Path Cancel changed signal order");
               later(dialog,failure,[&] { auto *types=dialog->findChild<QDialog *>("trackingTypesDialog"); auto *list=types->findChild<QListWidget *>("trackingMeasurementTypes"); for (int i=0;i<list->count();++i) list->item(i)->setCheckState(list->item(i)->text()=="Range_Skin" ? Qt::Checked : Qt::Unchecked); close(types); }); dialog->findChild<QPushButton *>("trackingTypes")->click(); if (failure) std::rethrow_exception(failure);
               for (const auto &station:QStringList{"CAN","MAD"}) { dialog->findChild<QPushButton *>("trackingAdd")->click(); const int row=table->rowCount()-1; table->item(row,0)->setText(station+", "+sat+", "+station); table->item(row,1)->setText("Range_Skin"); }
               const int width=table->columnWidth(0)+27; table->setColumnWidth(0,width); dialog->findChild<QPushButton *>("trackingAdd")->click(); dialog->findChild<QPushButton *>("trackingRemove")->click(); require(table->columnWidth(0)==width && table->horizontalHeader()->sectionResizeMode(0)==QHeaderView::Interactive,"Tracking columns cannot be adjusted or lost width"); table->setCurrentCell(2,0); dialog->findChild<QPushButton *>("trackingUp")->click(); require(table->item(1,0)->text().startsWith("MAD"),"Tracking row did not move up"); dialog->findChild<QPushButton *>("trackingDown")->click(); require(table->item(2,0)->text().startsWith("MAD"),"Tracking row did not move down");
               require(trackingConfigurations(*Moderator::Instance()->GetConfiguredObject(name.toStdString()))==old,"Pending tracking edit changed engine");
               if (!capture.isEmpty() && name=="simData") { require(waitUntil([&] { return dialog->windowHandle() && dialog->windowHandle()->isExposed(); }),"Tracking dialog did not expose"); require(dialog->grab().save(capture+".tracking.png"),"Tracking capture failed"); }
               close(dialog);
            }); button->click(); if (failure) std::rethrow_exception(failure);
            later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("trackingConfigDialog"); require(dialog->findChild<QTableWidget *>("trackingConfigs")->rowCount()==3,"Reopened tracking dialog lost pending rows"); close(dialog,false); }); button->click(); if (failure) std::rethrow_exception(failure);
         });
      }
      auto selectList=[&](ResourceEditor &panel,const QString &name,const QStringList &selected) {
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("resourceSelectionDialog"); require(dialog,"Reference list dialog missing"); auto *list=dialog->findChild<QListWidget *>("resourceSelectionList"); for (const auto &entry:selected) require(!list->findItems(entry,Qt::MatchExactly).isEmpty(),"Typed reference missing from picker"); for (int i=0;i<list->count();++i) { const auto text=list->item(i)->text(); require(text!="Earth" && text!="StateReport","Unrelated type in estimation picker"); list->item(i)->setCheckState(selected.contains(text) ? Qt::Checked : Qt::Unchecked); } close(dialog); }); auto *button=panel.findChild<QPushButton *>("chooseProperty_"+name); require(button,qPrintable("Reference picker unavailable: "+name)); button->click(); if (failure) std::rethrow_exception(failure);
      };
      const auto selectedFile=files.filePath("selected observations.gmd"); require(!QFileInfo::exists(selectedFile),"New output path already exists");
      for (const auto &name:QStringList{"simData","estData"}) applyPanel(name,[&](ResourceEditor &panel) {
         auto *table=panel.findChild<QTableWidget *>();
         later(&panel,failure,[&] {
            auto *dialog=panel.findChild<QDialog *>("kernelFileDialog"); auto *list=dialog->findChild<QListWidget *>("kernelFileList"); list->clear();
            later(dialog,failure,[&] { auto *picker=dialog->findChild<QFileDialog *>("kernelFilePicker"); require(picker && picker->fileMode()==QFileDialog::AnyFile && picker->acceptMode()==QFileDialog::AcceptSave,"Observation picker cannot select a new simulation output"); picker->selectFile(selectedFile); require(QMetaObject::invokeMethod(picker,"accept",Qt::DirectConnection),"Observation output selection failed"); }); dialog->findChild<QPushButton *>("kernelFileAdd")->click(); if (failure) std::rethrow_exception(failure); require(list->count()==1 && list->item(0)->text()==selectedFile,"Observation filename not added"); close(dialog);
         }); panel.findChild<QPushButton *>("chooseProperty_FileName")->click(); if (failure) std::rethrow_exception(failure); require(table->item(row(panel,"FileName"),1)->text()==selectedFile && !QFileInfo::exists(selectedFile),"Pending output selection wrote the file");
      }); measurements=selectedFile;
      for (const auto &station:QStringList{"GDS","CAN","MAD"}) applyPanel(station,[&](ResourceEditor &panel) { selectList(panel,"ErrorModels",{"RangeModel"}); });
      applyPanel("GDS",[&](ResourceEditor &panel) { selectList(panel,"ErrorModels",{}); }); require(Moderator::Instance()->GetConfiguredObject("GDS")->GetStringArrayParameter("ErrorModels").empty(),"Cleared station error models did not persist");
      applyPanel("GDS",[&](ResourceEditor &panel) { selectList(panel,"ErrorModels",{"RangeModel"}); });
      applyPanel("EstSat",[&](ResourceEditor &panel) { selectList(panel,"SolveFors",{"CartesianState"}); });
      applyPanel("sim",[&](ResourceEditor &panel) { selectList(panel,"AddData",{"simData"}); require(panel.findChild<QPushButton *>("chooseProperty_Propagator"),"Simulator default propagator selector missing"); });
      applyPanel("bat",[&](ResourceEditor &panel) { selectList(panel,"Measurements",{"estData"}); require(panel.findChild<QPushButton *>("chooseProperty_Propagator"),"Estimator default propagator selector missing"); });
      for (const auto &name:QStringList{"sim","bat"}) applyPanel(name,[&](ResourceEditor &panel) {
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QInputDialog *>(); require(dialog && dialog->comboBoxItems().contains("AlternateProp") && !dialog->comboBoxItems().contains("RangeModel"),"Default propagator choices include wrong resource types"); dialog->setTextValue("AlternateProp"); dialog->accept(); });
         panel.findChild<QPushButton *>("chooseProperty_Propagator")->click(); if (failure) std::rethrow_exception(failure);
      });
      for (const auto &entry:QList<QPair<QString,QString>>{{"sim","SimSat"},{"bat","EstSat"}}) {
         const auto script=editor->toPlainText();
         require(script.contains(entry.first+".Propagator = AlternateProp") && script.contains(entry.first+".Propagator = {ODProp, "+entry.second+"}"),"Default propagator edit lost an explicit spacecraft mapping");
      }
      applyPanel("RangeModel",[&](ResourceEditor &panel) { auto *table=panel.findChild<QTableWidget *>(); auto *type=qobject_cast<QComboBox *>(table->cellWidget(row(panel,"Type"),1)); require(type && type->findText("Range_Skin")>=0 && type->findText("DSN_TCP")>=0,"Measurement type choices missing"); type->setCurrentText("DSN_TCP"); require(table->item(row(panel,"Bias"),2)->text()=="Hz" && table->item(row(panel,"NoiseSigma"),2)->text()=="Hz","Changed measurement type left stale units"); type->setCurrentText("Range_Skin"); require(table->item(row(panel,"Bias"),2)->text()=="km" && table->item(row(panel,"NoiseSigma"),2)->text()=="km","Range-skin units missing"); table->item(row(panel,"NoiseSigma"),1)->setText("0.01"); });
      const auto asciiReport=report,unicodeReport=files.filePath("estimation ü Δ.txt");
      require(!QFileInfo::exists(unicodeReport),"Unicode estimator report existed before its GUI edit");
      applyPanel("bat",[&](ResourceEditor &panel){auto *filename=panel.findChild<QLineEdit *>("resource_ReportFile");require(filename&&filename->isEnabled()&&!filename->isReadOnly(),"Primary estimator ReportFile input is unavailable");estimationReveal(*filename);filename->setText(unicodeReport);require(panel.findChild<QTableWidget *>()->item(row(panel,"ReportFile"),1)->text()==unicodeReport,"Primary ReportFile edit did not reach its pending backing field");estimationView(panel);});
      require(!QFileInfo::exists(unicodeReport)&&editor->toPlainText().contains(unicodeReport),"Pending/applied report edit created output early or lost its Unicode path");
      report=unicodeReport;
      require(window.saveScriptTo(saved) && window.loadScript(saved),"Unicode estimation save/reopen failed"); const auto qualified=editor->toPlainText(); require(qualified.contains("preserve simulation comment") && qualified.contains("preserve estimation comment"),"Resource edits lost mission comments"); run(window); same(values(states),expected); require(read(measurements)==data,"GUI tracking configuration changed generated observations"); require(QFileInfo::exists(report)&&QFileInfo(report).size()>0&&QString::fromStdString(Moderator::Instance()->GetConfiguredObject("bat")->GetStringParameter("ReportFile"))==report,"Unicode estimator report was not created at its configured path");
      retainFile("unicode-report.txt",report);retainFile("unicode-states.txt",states);retainFile("unicode-observations.gmd",measurements);retain("unicode-qualified.script",editor->toPlainText().toUtf8());
      auto *tree=window.findChild<QTreeWidget *>("Output"); const auto reports=tree->findItems("StateReport",Qt::MatchExactly|Qt::MatchRecursive); require(reports.size()==1,"Estimation state report unavailable in Output"); tree->itemDoubleClicked(reports[0],0); require(window.findChild<QPlainTextEdit *>("report:StateReport"),"Estimation output viewer unavailable");
      for (const auto &entry:QList<QPair<QString,QString>>{{"RunSimulator","sim"},{"RunEstimator","bat"}}) {
         QString changed; CommandForm form([&](const QString &text) { changed=text; }); const auto type=entry.first=="RunSimulator" ? "Simulator" : "Estimator"; const auto statement=entry.first+" 'Keep label' Missing; % keep comment"; form.setStatement(statement); auto *button=form.findChild<QPushButton *>("commandChoose_"+QString(type)); require(button,"Run command typed selector missing"); estimationView(form);
         later(&form,failure,[&] { auto *dialog=form.findChild<QInputDialog *>(); require(dialog->comboBoxItems().contains(entry.second) && !dialog->comboBoxItems().contains(entry.second=="sim" ? "bat" : "sim") && !dialog->comboBoxItems().contains("ODProp"),"Run command includes wrong solver types"); dialog->setTextValue(entry.second); dialog->accept(); }); button->click(); if (failure) std::rethrow_exception(failure); require(changed==entry.first+" 'Keep label' "+entry.second+"; % keep comment","Run selector changed label/comment");
         const auto snapshot=window.missionSnapshot(); int index=-1; for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type==entry.first) index=i; require(index>=0 && window.applyMissionChange(snapshot,index,MissionEdit::Replace,changed).isEmpty(),"Run command edit did not apply");
      }
      run(window); same(values(states),expected);
      for (const auto &bad:QList<QPair<QString,QMap<QString,QString>>>{{"sim",{{"AddData","RangeModel"}}},{"bat",{{"Measurements","GDS"}}},{"GDS",{{"ErrorModels","SimSat"}}},{"sim",{{"Propagator","RangeModel"}}},{"RangeModel",{{"NoiseSigma","-1"}}},{"RangeModel",{{"SolveFors","Bias, PassBiases"}}},{"bat",{{"ReportFile",files.filePath("unsupported ?.txt")}}}}) { const auto source=editor->toPlainText(); std::cerr<<"Checking invalid estimation input "<<bad.first.toStdString()<<" "; for(auto item=bad.second.cbegin();item!=bad.second.cend();++item)std::cerr<<item.key().toStdString()<<"="<<item.value().toStdString()<<" "; std::cerr<<std::endl; require(!window.applyResourceChanges(bad.first,bad.second,source).isEmpty() && editor->toPlainText()==source,"Invalid estimation resource changed source"); }
      require(window.loadScript(saved) && window.buildScript(),"Recovery fixture restore failed"); const auto source=editor->toPlainText();
      for (const auto &bad:QStringList{"[{\"path\":\"Earth, SimSat, Earth\",\"types\":\"Range_Skin\"}]","[{\"path\":\"GDS, Missing, GDS\",\"types\":\"Range_Skin\"}]","[{\"path\":\"GDS, SimSat, GDS\",\"types\":\"Unknown\"}]","[{\"path\":\"GDS, SimSat, GDS\",\"types\":\"Range_Skin, Range_Skin\"}]"}) require(!window.applyResourceChanges("simData",{{"@TrackingConfigs",bad}},source).isEmpty() && editor->toPlainText()==source,"Invalid tracking definition changed source");
      auto withFov=source; withFov.prepend("Create ConicalFOV AntennaField OtherField;\n"); withFov.replace("BeginMissionSequence","Antenna1.FieldOfView = AntennaField;\nBeginMissionSequence"); editor->setPlainText(withFov); require(window.buildScript(),"Explicit antenna FOV fixture rejected");
      require(window.applyResourceChanges("bat",{{"MaximumIterations","12"}},withFov).isEmpty() && editor->toPlainText().contains("Antenna1.FieldOfView = AntennaField") && !editor->toPlainText().contains("UndefinedFieldOfView"),"Script reconstruction lost an explicit antenna FOV or wrote an unset placeholder");
      auto namedMarker=QString(withFov).replace("AntennaField","UndefinedFieldOfView"); editor->setPlainText(namedMarker); require(window.buildScript(),"Named FOV matching the diagnostic placeholder rejected");
      require(window.applyResourceChanges("bat",{{"MaximumIterations","12"}},namedMarker).isEmpty() && editor->toPlainText().contains("Antenna1.FieldOfView = UndefinedFieldOfView") && !editor->toPlainText().contains("SpacecraftAntenna.FieldOfView = UndefinedFieldOfView"),"Unset FOV acquired a real same-name object or explicit reference was removed");
      // An unset antenna must also be able to select that real object's name;
      // cleanup must inspect the edited clone, rather than its old empty FOV.
      applyPanel("SpacecraftAntenna",[&](ResourceEditor &panel) {
         auto *table=panel.findChild<QTableWidget *>(); require(table->item(row(panel,"FieldOfView"),1)->text().isEmpty(),"Unset FOV is presented as a selectable object name");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QInputDialog *>(); require(dialog && dialog->comboBoxItems().contains("UndefinedFieldOfView"),"Real FOV absent from antenna picker"); dialog->setTextValue("UndefinedFieldOfView"); dialog->accept(); });
         auto *button=panel.findChild<QPushButton *>("chooseProperty_FieldOfView"); require(button,"Antenna FOV picker missing"); button->click(); if (failure) std::rethrow_exception(failure);
      });
      require(Moderator::Instance()->GetConfiguredObject("SpacecraftAntenna")->GetRefObjectName(Gmat::FIELD_OF_VIEW)=="UndefinedFieldOfView" && editor->toPlainText().contains("SpacecraftAntenna.FieldOfView = UndefinedFieldOfView"),"Applying a new explicit FOV was removed by unset-reference cleanup");
      applyPanel("SpacecraftAntenna",[&](ResourceEditor &panel) { panel.findChild<QTableWidget *>()->item(row(panel,"FieldOfView"),1)->setText("OtherField"); });
      require(Moderator::Instance()->GetConfiguredObject("SpacecraftAntenna")->GetRefObjectName(Gmat::FIELD_OF_VIEW)=="OtherField", "Changing an existing antenna FOV retained its old object");
      applyPanel("SpacecraftAntenna",[&](ResourceEditor &panel) { panel.findChild<QTableWidget *>()->item(row(panel,"FieldOfView"),1)->setText(""); });
      require(Moderator::Instance()->GetConfiguredObject("SpacecraftAntenna")->GetRefObjectName(Gmat::FIELD_OF_VIEW).empty() && !editor->toPlainText().contains("SpacecraftAntenna.FieldOfView"),"Clearing an antenna FOV retained its old assignment");
      const auto clearedFov=editor->toPlainText(); require(!window.applyResourceChanges("SpacecraftAntenna",{{"FieldOfView","RangeModel"}},clearedFov).isEmpty() && editor->toPlainText()==clearedFov,"Wrong-type antenna FOV changed source");
      require(window.loadScript(saved) && window.buildScript(),"Explicit-FOV test restore failed");
      for (const auto &entry:QList<QPair<QString,QString>>{{"TrackingFileSet","NewTracking"},{"ErrorModel","NewErrorModel"},{"FileInterface","NewInterface"}}) {
         const auto before=editor->toPlainText(); require(window.createResource(entry.first,entry.second,before).isEmpty(),"Plugin resource absent from creation choices"); require(resources->findItems(entry.second,Qt::MatchExactly|Qt::MatchRecursive).size()==1,"New plugin resource missing from Resources"); require(window.deleteResource(entry.second,editor->toPlainText()).isEmpty(),"Unused plugin resource deletion failed"); require(resources->findItems(entry.second,Qt::MatchExactly|Qt::MatchRecursive).isEmpty(),"Deleted plugin resource remains in Resources");
      }
      require(window.loadScript(saved) && window.buildScript(),"Creation test restore failed");
      // Estimation-only runs must report a missing observation file and recover
      // after it is restored; the simulator must not silently recreate it.
      auto estimationOnly=source; estimationOnly.remove(QRegularExpression("^RunSimulator[^\\n]*\\n",QRegularExpression::MultilineOption)); editor->setPlainText(estimationOnly); require(window.buildScript(),"Estimation-only fixture failed build"); require(QFile::rename(measurements,measurements+".held"),"Observation-file failure fixture unavailable"); require(window.runMission()==MainWindow::RunResult::Failed && diagnostics(window).contains(QFileInfo(measurements).fileName()),"Missing observation file did not fail clearly"); require(QFile::rename(measurements+".held",measurements),"Observation restore failed"); run(window); same(values(states).mid(7),expected.mid(7));
      require(window.loadScript(saved),"Final estimation restore failed"); run(window); same(values(states),expected);
      applyPanel("sim",[&](ResourceEditor &panel) {
         auto *button=panel.findChild<QPushButton *>("editEpochInterval"); require(button,"Simulator interval editor missing");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("epochIntervalDialog"); dialog->findChild<QComboBox *>("intervalEpochFormat")->setCurrentText("TAIModJulian"); close(dialog,false); }); button->click(); if (failure) std::rethrow_exception(failure); require(!panel.hasChanges(),"Interval Cancel changed resource");
         later(&panel,failure,[&] {
            auto *dialog=panel.findChild<QDialog *>("epochIntervalDialog"); auto *format=dialog->findChild<QComboBox *>("intervalEpochFormat"); auto *start=dialog->findChild<QLineEdit *>("intervalInitialEpoch"),*end=dialog->findChild<QLineEdit *>("intervalFinalEpoch");
            const auto originalStart=start->text(),originalEnd=end->text();
            for (const auto &name:TimeSystemConverter::Instance()->GetValidTimeRepresentations()) { format->setCurrentText(QString::fromStdString(name)); const auto error=dialog->findChild<QLabel *>("epochIntervalError")->text(); if (!error.isEmpty()) throw std::runtime_error((QString::fromStdString(name)+": "+error+"; dates="+start->text()+", "+end->text()).toStdString()); }
            format->setCurrentText("UTCGregorian"); require(start->text()==originalStart && end->text()==originalEnd,"Epoch format cycle changed physical interval");
            end->setText("bad epoch"); format->setCurrentText("TAIModJulian"); require(format->currentText()=="UTCGregorian" && start->text()==originalStart && end->text()=="bad epoch" && !dialog->findChild<QLabel *>("epochIntervalError")->text().isEmpty(),"Failed conversion partially changed the interval");
            end->setText(originalEnd); format->setCurrentText("TAIModJulian");
            if (!capture.isEmpty()) { require(waitUntil([&] { return dialog->windowHandle() && dialog->windowHandle()->isExposed(); }),"Interval dialog did not expose"); require(dialog->grab().save(capture+".interval.png"),"Interval capture failed"); }
            close(dialog);
         }); button->click(); if (failure) std::rethrow_exception(failure);
      });
      const auto intervalSave=files.filePath("numeric interval ü.script"); require(window.saveScriptTo(intervalSave) && window.loadScript(intervalSave),"Numeric interval save/reopen failed"); run(window); same(values(states),expected,"Numeric interval"); require(read(measurements)==data,"Epoch conversion changed generated observations");
      auto numeric=editor->toPlainText(); require(window.applyResourceChanges("sim",{{"EpochFormat","UTCGregorian"}},numeric).isEmpty(),"Format-only edit failed to convert both dates"); run(window); same(values(states),expected,"Format-only interval"); require(read(measurements)==data,"Format-only edit changed observations");
      for (const auto &bad:QList<QMap<QString,QString>>{{{"FinalEpoch","09 Jun 2012 00:00:00.000"}},{{"EpochFormat","Invalid"}},{{"InitialEpoch","not a date"}}}) { const auto source=editor->toPlainText(); require(!window.applyResourceChanges("sim",bad,source).isEmpty() && editor->toPlainText()==source,"Invalid interval edit changed source"); }
      // Compare GUI filters with independently configured filter assignments
      // on the same bounded fixture; the unfiltered observations remain intact.
      auto filterReference=editor->toPlainText(); filterReference.replace("BeginMissionSequence","Create AcceptFilter KeepRange;\nKeepRange.Trackers = {'All'};\nKeepRange.ObservedObjects = {'EstSat'};\nKeepRange.DataTypes = {'Range_Skin'};\nKeepRange.RecordNumbers = {'All'};\nKeepRange.ThinMode = 'Frequency';\nKeepRange.ThinningFrequency = 2;\nCreate RejectFilter DropRecords;\nDropRecords.RecordNumbers = {'1-3'};\nbat.DataFilters = {KeepRange, DropRecords};\nBeginMissionSequence");
      filterReference.replace("KeepRange.Trackers", "KeepRange.FileNames = {'"+measurements+"'};\nKeepRange.EpochFormat = 'UTCGregorian';\nKeepRange.InitialEpoch = '09 Jun 2012 00:00:00.000';\nKeepRange.FinalEpoch = '11 Jun 2012 00:00:00.000';\nKeepRange.Trackers");
      editor->setPlainText(filterReference); run(window); const auto filteredExpected=values(states); const auto filteredReport=read(report); retainFile("filtered-reference-report.txt",report);retainFile("filtered-reference-states.txt",states); require(read(measurements)==data,"Estimator filters changed simulation output");
      auto filterBase=filterReference; filterBase.replace("bat.DataFilters = {KeepRange, DropRecords};","bat.DataFilters = {};"); filterBase.replace("KeepRange.ThinningFrequency = 2;","KeepRange.ThinningFrequency = 1;"); filterBase.replace("DropRecords.RecordNumbers = {'1-3'};","DropRecords.RecordNumbers = {'All'};"); filterBase.replace("KeepRange.DataTypes = {'Range_Skin'};","KeepRange.DataTypes = {'All'};"); filterBase.replace("KeepRange.ObservedObjects = {'EstSat'};","KeepRange.ObservedObjects = {'All'};"); editor->setPlainText(filterBase); require(window.buildScript(),"Editable filter fixture rejected");
      applyPanel("KeepRange",[&](ResourceEditor &panel) {
         auto *table=panel.findChild<QTableWidget *>(); auto *format=qobject_cast<QComboBox *>(table->cellWidget(row(panel,"EpochFormat"),1)); require(format && panel.findChild<QPushButton *>("editEpochInterval"),"Filter paired epoch controls missing"); format->setCurrentText("TAIModJulian"); bool numeric=false; table->item(row(panel,"FinalEpoch"),1)->text().toDouble(&numeric); require(numeric,"Main filter format selector did not convert both dates");
         later(&panel,failure,[&] { auto *dialog=panel.findChild<QDialog *>("kernelFileDialog"); auto *list=dialog->findChild<QListWidget *>("kernelFileList"); dialog->findChild<QPushButton *>("filterTrackingFiles")->click(); require(list->count()==1 && list->item(0)->text()=="From_AddTrackingConfig","Tracking-config file sentinel missing"); dialog->findChild<QPushButton *>("filterAllFiles")->click(); require(list->count()==1 && list->item(0)->text()=="All","All-files sentinel did not replace the list"); close(dialog,false); });
         panel.findChild<QPushButton *>("chooseProperty_FileNames")->click(); if (failure) std::rethrow_exception(failure);
         selectList(panel,"DataTypes",{"Range_Skin"}); selectList(panel,"ObservedObjects",{"EstSat"}); selectList(panel,"Trackers",{"All"});
         table->item(row(panel,"ThinningFrequency"),1)->setText("2"); auto *mode=qobject_cast<QComboBox *>(table->cellWidget(row(panel,"ThinMode"),1)); require(mode && mode->findText("Time")>=0,"Filter thinning mode choices missing");
      });
      applyPanel("DropRecords",[&](ResourceEditor &panel) { panel.findChild<QTableWidget *>()->item(row(panel,"RecordNumbers"),1)->setText("1-3"); });
      applyPanel("bat",[&](ResourceEditor &panel) { selectList(panel,"DataFilters",{"KeepRange","DropRecords"}); });
      const auto filtersSaved=files.filePath("filtered estimation ü.script"); require(window.saveScriptTo(filtersSaved) && window.loadScript(filtersSaved),"Filtered estimation save/reopen failed"); run(window); same(values(states),filteredExpected,"GUI filters"); require(read(measurements)==data,"GUI filters changed simulated observations");
      const QRegularExpression observationRow("^[ \\t]*(?:[0-9]+[ \\t]+)?[0-9]+[ \\t]+[0-9]{2}[ \\t]+[A-Za-z]{3}[ \\t]+[0-9]{4}[^\\n]*$",QRegularExpression::MultilineOption);
      auto observationRows=[&](const QString &text) { QStringList rows; auto matches=observationRow.globalMatch(text); while (matches.hasNext()) rows.append(matches.next().captured()); return rows; };
      const auto referenceRows=observationRows(filteredReport),guiRows=observationRows(read(report)); retainFile("filtered-gui-report.txt",report);retainFile("filtered-gui-states.txt",states);
      require(!referenceRows.isEmpty() && guiRows==referenceRows,"GUI filter selection changed observation flags/residual report");
      require(!referenceRows.filter(QRegularExpression("\\bUSER\\b")).isEmpty(),"Accept/reject filters did not mark any observations as edited");
      for (const auto &bad:QList<QPair<QString,QMap<QString,QString>>>{{"bat",{{"DataFilters","RangeModel"}}},{"KeepRange",{{"Trackers","Earth"}}},{"KeepRange",{{"DataTypes","Unknown"}}},{"KeepRange",{{"ThinningFrequency","0"}}},{"DropRecords",{{"RecordNumbers","3-1"}}},{"DropRecords",{{"RecordNumbers","-2"}}}}) { const auto source=editor->toPlainText(); require(!window.applyResourceChanges(bad.first,bad.second,source).isEmpty() && editor->toPlainText()==source,"Invalid filter edit changed source"); }
      applyPanel("bat",[&](ResourceEditor &panel) { selectList(panel,"DataFilters",{}); }); run(window); same(values(states),expected);
      require(QFileInfo::exists(asciiReport)&&QFileInfo::exists(report)&&report==unicodeReport,"ASCII baseline or Unicode final report evidence is missing");
      require(!QFileInfo::exists(files.filePath("unsupported ?.txt")),"Rejected report filename created a file");
      retainFile("final-report.txt",report);retainFile("final-states.txt",states);retainFile("final-observations.gmd",measurements);retain("final.script",editor->toPlainText().toUtf8());
      const QJsonObject proof{{"checkpoint","estimation-filename-reconciled"},{"outcome","passed"},
         {"ascii_baseline_report",asciiReport},{"unicode_report",report},{"invalid_report",files.filePath("unsupported ?.txt")},
         {"unicode_created_at_configured_path",true},{"reserved_question_mark_rejected",true},
         {"successful_mission_runs",successfulEstimationRuns},{"state_comparisons",estimationComparisons},
         {"noise_free_truth",QJsonArray{truth[0],truth[1],truth[2],truth[3],truth[4],truth[5]}},
         {"truth_position_tolerance_km",1e-5},{"truth_velocity_tolerance_km_s",1e-8},{"state_equivalence_tolerance",1e-8},
         {"viewing_stages",estimationViews},{"viewing_stage_count",estimationViews.size()},
         {"facets",QJsonArray{"positive-unicode-estimator-report-gui-apply-history-save-reopen-and-native-output","reserved-filename-invalid-source-rollback","original-noiseless-state-observation-filter-and-recovery-assertions","raw-report-state-and-observation-evidence"}},
         {"scope","Existing bounded RangeSkin estimation workflow and filename expectation reconciliation. All original assertions and numeric bounds remain;Unicode report output is positive and forbidden question-mark basename remains negative. Current Linux Qt replay only;not arbitrary estimation/filter science,native OS input,Windows/macOS,hardware/stress/current-private NASA or full current-build qualification."}};
      retain("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      std::cout<<"PASS: Qt tracking paths/types, pending/Cancel/columns, simulator/estimator/solve-for/station lists, typed run commands, paired epoch conversion and exact numeric observation boundaries, accept/reject filter lists/ranges/thinning and residual edit-flag equivalence, noise-free six-state fit and independent reports/observations, exact Undo/Redo/Unicode save/reopen, invalid edit rollback and missing observation recovery\n";
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; } return 0;
}
