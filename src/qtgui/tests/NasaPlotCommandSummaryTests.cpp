#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "BaseException.hpp"
#include "CommandEditor.hpp"
#include "InspectionDialog.hpp"
#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if(!ok)throw std::runtime_error(message); }
static void events() { QApplication::processEvents();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete); }
static QByteArray read(const QString &path) { QFile f(path);require(f.open(QIODevice::ReadOnly),"Plot summary artifact missing");return f.readAll(); }
static void write(const QString &path,const QByteArray &bytes) { QFile f(path);require(f.open(QIODevice::WriteOnly)&&f.write(bytes)==bytes.size(),"Plot summary artifact write failed"); }
// Same unique-field and finite-value criterion as the existing command-summary
// driver; that driver's assertions and its qualified cases remain unchanged.
static double number(const QString &text,const QString &label) {
 auto scan=QRegularExpression("(?:^|\\s)"+QRegularExpression::escape(label)+"\\s*=\\s*([-+0-9.eE]+)").globalMatch(text);
 require(scan.hasNext(),qPrintable("Plot summary numeric field missing: "+label));bool ok=false;const double value=scan.next().captured(1).toDouble(&ok);
 require(ok&&std::isfinite(value)&&!scan.hasNext(),"Plot summary numeric field invalid/duplicated");return value;
}
int main(int argc,char **argv) {
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);QApplication app(argc,argv);
 app.setOrganizationName("GMATTests");app.setApplicationName("NasaPlotCommandSummary");
 try {
  require(argc==2&&acceptanceWidgetPlatform(),"Supply plot-summary startup");TestSettings settings;QTemporaryDir files;require(files.isValid(),"Plot summary fixture missing");
  const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
  auto save=[&](const QString &name,const QByteArray &bytes){if(!evidence.isEmpty())write(QDir(evidence).filePath(name),bytes);};
  const auto report=files.filePath("plot summaries Δ.csv"),saved=files.filePath("plot summaries Ω.script");
  const QStringList commands{"ClearPlot","PenUp","PenDown","MarkPoint"};
  const QStringList fields{"EarthMJ2000Eq.X","EarthMJ2000Eq.Y","EarthMJ2000Eq.Z","EarthMJ2000Eq.VX","EarthMJ2000Eq.VY","EarthMJ2000Eq.VZ","Cd","DryMass","ElapsedSecs","A1ModJulian"};
  const QStringList labels{"X","Y","Z","VX","VY","VZ","Cd","Dry mass"};
  QString source="% Keep plot summary source Δ\nCreate Spacecraft Sat;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DateFormat = A1ModJulian;\nSat.Epoch = '21545.25';\nSat.DisplayStateType = Keplerian;\nSat.SMA = 9000;\nSat.ECC = 0.2;\nSat.INC = 35;\nSat.RAAN = 40;\nSat.AOP = 50;\nSat.TA = 60;\nCreate ForceModel Gravity;\nGravity.CentralBody = Earth;\nGravity.PrimaryBodies = {};\nGravity.PointMasses = {Earth};\nGravity.Drag = None;\nGravity.SRP = Off;\nCreate Propagator Coast;\nCoast.FM = Gravity;\nCreate XYPlot Trace Spare;\nTrace.XVariable = Sat.ElapsedSecs;\nTrace.YVariables = {Sat.EarthMJ2000Eq.X};\nTrace.ShowPlot = false;\nSpare.XVariable = Sat.ElapsedSecs;\nSpare.YVariables = {Sat.EarthMJ2000Eq.X};\nSpare.ShowPlot = false;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Delimiter = ',';\nValues.Precision = 17;\nBeginMissionSequence;\n";
  QStringList reports;for(const auto &f:fields)reports.append("Sat."+f);
  for(int i=0;i<4;++i)source+="Sat.Cd = "+QString::number(2.3+.1*i,'f',1)+";\nSat.DryMass = "+QString::number(810+10*i)+";\nPropagate Coast(Sat) {Sat.ElapsedSecs = 10};\n"+commands[i]+" 'Applied "+commands[i]+"' Trace;\nReport 'Witness "+commands[i]+"' Values "+reports.join(' ')+";\n";
  source+="% Keep plot summary tail Ω\n";save("mission.script",source.toUtf8());
  MainWindow window;require(window.initialize(startup),"Plot summary runtime unavailable");window.show();events();acceptanceView(window);
  auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");auto *tree=window.findChild<QTreeWidget *>("Mission");auto *workspace=window.findChild<QMdiArea *>("workspace");require(editor&&tree&&workspace,"Plot summary main controls missing");
  editor->setPlainText(source);require(window.saveScriptTo(saved)&&window.buildScript(),"Plot summary fixture Build failed");const int undo=editor->document()->availableUndoSteps();
  QByteArray raw;auto stable=[&]{require(editor->toPlainText()==source&&!editor->document()->isModified()&&editor->document()->availableUndoSteps()==undo&&read(saved)==source.toUtf8(),"Plot summary inspection changed source/history");if(!raw.isEmpty())require(read(report)==raw,"Plot summary inspection changed report output");};
  auto open=[&](int which){int node=-1;const auto nodes=window.missionSnapshot().nodes;for(int n=0;n<nodes.size();++n)if(nodes[n].statement.contains(commands[which]+" 'Applied "+commands[which]+"' Trace;")){require(node<0,"Plot summary node ambiguous");node=n;}require(node>=0,"Plot summary command absent");QTreeWidgetItem *item=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole).isValid()&&(*it)->data(0,Qt::UserRole).toInt()==node)item=*it;require(item,"Plot summary tree command absent");tree->itemDoubleClicked(item,0);events();auto *sub=workspace->currentSubWindow();auto *panel=sub?dynamic_cast<CommandEditor *>(sub->widget()):nullptr;require(panel,"Plot summary actual command editor absent");acceptanceView(*panel);return panel;};
  auto cancel=[&](CommandEditor *panel){QPointer<QMdiSubWindow> owner=qobject_cast<QMdiSubWindow *>(panel->parentWidget());auto *button=panel->findChild<QPushButton *>("cancelPanel");require(owner&&button,"Plot summary Cancel absent");QTest::mouseClick(button,Qt::LeftButton);events();require(!owner,"Plot summary Cancel retained panel");stable();};
  QJsonArray unavailable,observations;int compared=0;
  for(int i=0;i<4;++i){auto *panel=open(i);auto *button=panel->findChild<QPushButton *>("showCommandSummary");require(button&&button->isVisible()&&button->isEnabled(),"Plot summary button unreachable");QTest::mouseClick(button,Qt::LeftButton);events();require(!QApplication::activeModalWidget()&&window.statusBar()->currentMessage().contains("Run the current mission"),"Plot command showed stale summary before execution");unavailable.append(QJsonObject{{"command",commands[i]},{"status",window.statusBar()->currentMessage()}});cancel(panel);}
  require(window.runMission()==MainWindow::RunResult::Completed,"Plot summary fixture Run failed");raw=read(report);save("values.csv",raw);QList<QList<double>> values;
  for(const auto &line:QString::fromUtf8(raw).split('\n',Qt::SkipEmptyParts)){QList<double> row;for(const auto &field:line.trimmed().split(',')){bool ok=false;const double value=field.toDouble(&ok);require(ok&&std::isfinite(value),"Plot summary report invalid value");row.append(value);}require(row.size()==10,"Plot summary report width differs");values.append(row);}require(values.size()==4,"Plot summary report row count differs");
  for(int i=0;i<4;++i){require(std::abs(values[i][6]-(2.3+.1*i))<1e-12&&values[i][7]==810+10*i,"Plot summary independent literal witnesses differ");require(std::abs(values[i][8]-10*(i+1))<1e-5&&std::abs((values[i][9]-21545.25)*86400.-10*(i+1))<1e-5,"Plot summary independent time progression differs");if(i)require(std::abs(values[i][0]-values[i-1][0])>1,"Plot summary states failed to progress");}
  auto inspect=[&](int i,bool pending,const QString &tag){auto *panel=open(i);auto *name=panel->findChild<QLineEdit *>("commandName");auto *subscribers=panel->findChild<QLineEdit *>("commandField_Subscribers");auto *commandText=panel->findChild<QPlainTextEdit *>("commandSource");require(name&&subscribers&&commandText,"Plot summary typed fields absent");if(pending){name->setFocus();name->selectAll();QTest::keyClicks(name,"Pending "+commands[i]);subscribers=panel->findChild<QLineEdit *>("commandField_Subscribers");require(subscribers,"Plot summary pending fields absent");subscribers->setFocus();subscribers->selectAll();QTest::keyClicks(subscribers,"Spare");require(panel->hasChanges(),"Plot summary draft not pending");}
   const auto draft=commandText->toPlainText();const int draftUndo=commandText->document()->availableUndoSteps();auto *button=panel->findChild<QPushButton *>("showCommandSummary");require(button&&button->isEnabled(),"Plot summary pending route disabled");acceptanceView(*panel);std::exception_ptr error;bool visited=false;QTimer watcher;watcher.setInterval(20);
   QObject::connect(&watcher,&QTimer::timeout,&window,[&]{auto *dialog=dynamic_cast<SummaryDialog *>(QApplication::activeModalWidget());if(!dialog)return;watcher.stop();try{visited=true;acceptanceView(*dialog);auto *text=dialog->findChild<QPlainTextEdit *>("inspectionText");auto *frame=dialog->findChild<QComboBox *>("summaryFrame");auto *physics=dialog->findChild<QCheckBox *>("summaryPhysics");auto *status=dialog->findChild<QLabel *>("inspectionStatus");require(text&&text->isReadOnly()&&frame&&frame->currentText()=="EarthMJ2000Eq"&&physics&&!physics->isVisible()&&status&&status->text().isEmpty(),"Plot summary controls/status differ");const auto contents=text->toPlainText();save(tag+".txt",contents.toUtf8());require(dialog->windowTitle()=="Command summary — "+commands[i]+" — Applied "+commands[i]&&contents.contains(commands[i]+" Command: Applied "+commands[i])&&!contents.contains("Pending ")&&contents.count("Spacecraft       : Sat")==1,"Plot summary applied identity/payload differs");require(contents.contains("Coordinate System: EarthMJ2000Eq"),"Plot summary coordinate label differs");QJsonArray observed;for(int c=0;c<8;++c){const double actual=number(contents,labels[c]),expected=values[i][c];require(std::abs(actual-expected)<=std::max(1e-7,std::abs(expected)*1e-10),qPrintable("Plot command summary disagrees with report: "+commands[i]+" "+labels[c]));observed.append(actual);++compared;}observations.append(QJsonObject{{"command",commands[i]},{"pending",pending},{"tag",tag},{"values",observed}});stable();require(commandText->toPlainText()==draft&&commandText->document()->availableUndoSteps()==draftUndo&&panel->hasChanges()==pending,"Plot summary mutated draft/history");QTest::keyClick(dialog,Qt::Key_Escape);}catch(...){error=std::current_exception();dialog->reject();}});
   watcher.start();QTest::mouseClick(button,Qt::LeftButton);watcher.stop();if(error)std::rethrow_exception(error);require(visited&&!QApplication::activeModalWidget(),"Plot summary did not open/close actual modal");require(commandText->toPlainText()==draft&&panel->hasChanges()==pending,"Plot summary close changed pending source");cancel(panel);
  };
  for(int i=0;i<4;++i)inspect(i,true,"pending-"+commands[i]);for(int i=3;i>=0;--i)inspect(i,false,"revisited-"+commands[i]);stable();save("unavailable.json",QJsonDocument(unavailable).toJson());save("observations.json",QJsonDocument(observations).toJson());
  const QJsonObject proof{{"checkpoint","plot-command-summaries"},{"outcome","passed"},{"commands",QJsonArray::fromStringList(commands)},{"before_run_unavailable",4},{"command_views",8},{"compared_values",compared},{"raw_report_values",40},{"pending_drafts",4},{"old_cache_revisits",4},{"facets",QJsonArray{"actual-named-plot-command-panel-summary-route","explicit-before-run-unavailable-state","applied-command-identity-pending-name-subscriber-isolation","independent-literal-time-report-and-eight-cached-state-fields","reverse-order-old-cache-revisit","source-undo-unicode-file-and-report-isolation"}},{"scope","Four named plot commands in a nondegenerate Earth point-mass mission. Eight native cached fields per summary compared with command-adjacent reports, distinct literal Cd/mass and 10-second state progression. Pending name/subscriber edits remain isolated. Original keyword payloads, other summary representations/frames/export routes, arbitrary science/native OS input/hardware/platform/stress and full latest-build suite remain unqualified."}};
  save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
