#include "ComparisonPanel.hpp"
#include "FileComparison.hpp"
#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "FileUtil.hpp"
#include <QApplication>
#include <QAction>
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QTreeWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QElapsedTimer>
#include <QTimer>
#include <QFileDialog>
#include <QPointer>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static void write(const QString &path,const QByteArray &data) { QFile file(path); require(file.open(QIODevice::WriteOnly) && file.write(data)==data.size(),"Fixture write failed"); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Fixture read failed"); return file.readAll(); }
static void wait(ComparisonPanel &panel)
{
   QElapsedTimer timer; timer.start();
   while (panel.isComparing() && timer.elapsed()<20000) QApplication::processEvents(QEventLoop::AllEvents,10);
   require(!panel.isComparing(),"Comparison stalled"); QApplication::processEvents();
}
static QPushButton *button(QWidget &panel,const QString &name) { auto *result=panel.findChild<QPushButton *>(name); require(result,"Comparison button missing"); return result; }
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
   QApplication app(argc,argv); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtComparison");
   try {
      TestSettings isolated; QTemporaryDir files; require(files.isValid(),"Temporary directory missing");
      const auto left=files.filePath("baseline Δ.txt"),right=files.filePath("candidate ü.txt"),report=files.filePath("results.txt");
      std::atomic_bool cancel=false; FileCompareOptions options;
      auto check=[&] { return compareFiles({{left,right}},options,report,cancel); };
      write(left,"\xef\xbb\xbf"+QString("café\r\n1 2\r\n").toUtf8()); write(right,QString("café\n1 2\n").toUtf8());
      auto result=check(); require(result.compared==1 && !result.different && !result.errors,"UTF-8/BOM/CRLF text comparison failed");
      write(right,QString("café\n1 2\nextra row\n").toUtf8()); result=check();
      require(result.different==1 && read(report).contains("Different rows: 1") && read(report).contains("<end of file>"),"Trailing text rows were ignored");
      write(left,"one\n\n two\n"); write(right,"one\n two\n\n"); options.skipBlankLines=true; require(check().different==0,"Skip blank lines failed");
      options.skipBlankLines=false; require(check().different==1,"Text comparison ignored blank lines without selection");
      options.mode=FileCompareMode::NumericLines; options.tolerance=0.125;
      write(left,"old header\n\n1, 2 % baseline\n3 4\n"); write(right,"new header\n1.125, 2 % candidate\n3 4\n");
      Real diff; require(GmatFileUtil::CompareLines("1, 2 % baseline","1.125, 2 % candidate",diff,0.125),"Engine reference unexpected");
      result=check(); require(!result.different && !result.errors,"Numeric line tolerance/string/comment semantics differ from engine");
      options.tolerance=0.124; require(check().different==1,"Tolerance below numeric difference was ignored");
      write(right,"new header\n1 nan\n3 4\n"); require(check().errors==1,"Non-finite numeric line was accepted");
      write(right,"new header\n1.125 2\n3 4\n5 6\n"); require(check().different==1 && read(report).contains("<end of file>"),"Trailing numeric rows were ignored");
      write(left,"7000 7.5\n"); write(right,"7000 N/A\n");
      require(check().different==1,"A missing numeric value compared equal");
      write(left,"7000 N/A\n"); write(right,"7000 7.5\n");
      require(check().different==1,"An unexpected numeric value compared equal");
      options.mode=FileCompareMode::NumericColumns; options.tolerance=0.125;
      write(left,"7000,7.5\n"); write(right,read(left));
      result=check(); require(result.compared==1 && !result.different && !result.errors,"Comma-delimited report could not be compared");
      write(right,"7000,8.5\n"); require(check().different==1,"Comma-delimited column difference was hidden");
      write(left,"1,2\n3,4\n"); write(right,"1,2\n3, ,4\n");
      require(check().errors==1,"A missing CSV column shifted subsequent values");
      write(left,"baseline heading\n01 Jan 2000 12:00:00.000 1 2\n01 Jan 2000 12:01:00.000 3 4\n");
      write(right,"candidate heading\n\n2000-01-01T12:00:00.000 1.125 2\n2000-01-01T12:01:00.000 3 4\n");
      result=check(); require(result.compared==1 && !result.different && !result.errors && read(report).contains("0.125   no"),"UTC column conversion or maxima failed");
      options.tolerance=0.124; require(check().different==1 && read(report).contains("0.125   yes"),"Column tolerance flag missing");
      write(left,"1 2\n3 4\n"); write(right,"1 2\n3\n"); require(check().different==1,"Unequal column counts were hidden");
      write(right,"1 2\ninvalid data\n"); require(check().errors==1 && read(report).contains("Invalid numeric data row"),"Malformed row was accepted");
      write(right,"header only\n"); require(check().errors==1 && read(report).contains("No numeric data records"),"Empty numeric report was accepted");
      write(right,"nan 2\n3 4\n"); require(check().errors==1 && read(report).contains("Non-finite"),"Non-finite column row was treated as a header");
      options.mode=FileCompareMode::Text; write(right,QByteArray(1,char(0xff))); require(check().errors==1,"Invalid UTF-8 accepted");
      write(right,"1 2\n3 4\n");
      result=compareFiles({{left,files.filePath("missing.txt")},{left,right}},options,report,cancel);
      require(result.compared==1 && result.errors==1 && read(report).contains("Cannot open file"),"Missing pair prevented independent pair comparison");
      write(left,QByteArray(256*1024,'x')+"\n"); write(right,read(left)); require(check().compared==1 && check().different==0,"Long text line stalled or differed");
      write(left,QByteArray(17*1024*1024,'x')); require(check().errors==1 && read(report).contains("Line exceeds 16 MiB"),"Oversized line failed without explicit diagnosis");
      cancel.store(true); require(check().cancelled,"Pre-cancelled comparison ran"); cancel.store(false);

      ComparisonPanel panel; panel.resize(940,760); panel.show();
      auto *base=panel.findChild<QLineEdit *>("comparisonBaseline"),*candidate=panel.findChild<QLineEdit *>("comparisonCandidate");
      auto *mode=panel.findChild<QComboBox *>("comparisonMode"),*scope=panel.findChild<QComboBox *>("comparisonScope");
      write(left,"1 2\n3 4\n"); write(right,"1 2\n3 5\n"); base->setText(left); candidate->setText(right);
      button(panel,"comparisonStart")->click(); wait(panel);
      const auto completed=panel.resultsPath(); const auto original=read(completed);
      require(!completed.isEmpty() && original.contains("Different rows: 1"),"Actual controls failed comparison");
      const auto saved=files.filePath("saved results Δ.txt"); require(panel.saveResults(saved).isEmpty() && read(saved)==original,"Unicode result export differs");
      require(!panel.saveResults(saved).isEmpty() && panel.saveResults(saved,true).isEmpty(),"Existing export replacement lacks explicit control");
      require(!panel.saveResults(left,true).isEmpty() && read(left)=="1 2\n3 4\n","Result export overwrote baseline");
      const auto alias=files.filePath("input alias.txt"); require(QFile::link(right,alias),"Input link fixture failed");
      require(!panel.saveResults(alias,true).isEmpty() && read(right)=="1 2\n3 5\n","Result export overwrote linked candidate");
      require(!panel.saveResults(files.filePath("absent/results.txt")).isEmpty() && read(saved)==original,"Failed export damaged existing result");
      const auto uiSaved=files.filePath("UI results ü.txt"); bool picked=false;
      QTimer::singleShot(0,&panel,[&] { if (auto *dialog=panel.findChild<QFileDialog *>()) { dialog->selectFile(uiSaved); picked=true; QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection); } });
      button(panel,"comparisonSave")->click(); require(picked && read(uiSaved)==original,"Save results control did not export the complete report");
      mode->setCurrentIndex(1); panel.findChild<QLineEdit *>("comparisonTolerance")->setText("-1"); button(panel,"comparisonStart")->click();
      require(panel.findChild<QLineEdit *>("comparisonTolerance")->isEnabled() && !panel.findChild<QCheckBox *>("comparisonSkipBlank")->isEnabled(),"Numeric-mode dependencies incorrect");
      require(!panel.isComparing() && panel.resultsPath()==completed && panel.findChild<QLabel *>("comparisonStatus")->text().contains("finite and nonnegative"),"Invalid tolerance discarded results");
      panel.findChild<QLineEdit *>("comparisonTolerance")->setText("0"); candidate->setText(files.filePath("missing.txt")); button(panel,"comparisonStart")->click();
      require(!panel.isComparing() && panel.resultsPath()==completed,"Missing input discarded results"); candidate->setText(right);
      // File picker Cancel must preserve the chosen path.
      QTimer::singleShot(0,&panel,[&] { if (auto *dialog=panel.findChild<QFileDialog *>()) dialog->reject(); });
      button(panel,"comparisonBrowseCandidate")->click(); require(candidate->text()==right,"Picker Cancel changed selected input");
      mode->setCurrentIndex(0); write(left,"one\n\n two\n"); write(right,"one\n two\n");
      panel.findChild<QCheckBox *>("comparisonSkipBlank")->setChecked(true); button(panel,"comparisonStart")->click(); wait(panel);
      require(read(panel.resultsPath()).contains("EQUAL"),"Text skip-blank control was ignored");
      panel.findChild<QCheckBox *>("comparisonSkipBlank")->setChecked(false); button(panel,"comparisonStart")->click(); wait(panel);
      require(read(panel.resultsPath()).contains("DIFFERENT"),"Text comparison did not restore blank-line checking"); mode->setCurrentIndex(1);
      const auto baselineDir=files.filePath("base"),dir1=files.filePath("first"),dir2=files.filePath("second"),dir3=files.filePath("third");
      for (const auto &dir:{baselineDir,dir1,dir2,dir3}) require(QDir().mkpath(dir),"Directory fixture failed");
      write(baselineDir+"/base-a.txt","1 2\n"); write(baselineDir+"/base-b.report","3 4\n");
      write(baselineDir+"/GmatLog.txt","skip\n"); write(baselineDir+"/base-c.txt.bak","skip\n"); write(baselineDir+"/base-d.bin","skip\n");
      write(dir1+"/one-a.txt","1 2\n"); write(dir1+"/one-b.report","3 4\n");
      write(dir2+"/two-a.truth","1 2\n"); write(dir2+"/two-b.report","3 5\n"); write(dir3+"/three-a.txt","1 2\n");
      require(comparisonBaseFiles(baselineDir,"base-").size()==2 && comparisonBaseFiles(baselineDir,"").size()==2,"Directory selection included logs/backups/binary files");
      scope->setCurrentIndex(1); base->setText(baselineDir); panel.findChild<QLineEdit *>("comparisonBasePrefix")->setText("base-");
      button(panel,"comparisonAddDirectory")->click(); button(panel,"comparisonAddDirectory")->click();
      auto *table=panel.findChild<QTableWidget *>("comparisonDirectories"); require(table->rowCount()==3 && !button(panel,"comparisonAddDirectory")->isEnabled(),"Three-directory control failed");
      for (int row=0;row<3;++row) { table->item(row,0)->setText(QStringList{dir1,dir2,dir3}[row]); table->item(row,1)->setText(QStringList{"one-","two-","three-"}[row]); }
      require(table->horizontalHeader()->sectionResizeMode(0)==QHeaderView::Interactive,"Directory widths cannot be adjusted");
      table->setColumnWidth(0,350); require(table->columnWidth(0)==350,"Manual directory width ignored");
      button(panel,"comparisonStart")->click(); wait(panel); auto directoryResults=read(panel.resultsPath());
      require(directoryResults.contains("Summary: 5 compared, 1 different, 1 errors") && directoryResults.contains("two-a.truth") && directoryResults.contains("three-b.report"),"Directory matching/truth/difference/missing summary failed");
      if (argc>2) { QApplication::processEvents(); panel.grab().save(QString::fromLocal8Bit(argv[2])); }
      panel.findChild<QSpinBox *>("comparisonFileLimit")->setValue(1); button(panel,"comparisonStart")->click(); wait(panel);
      require(read(panel.resultsPath()).contains("Summary: 3 compared, 0 different, 0 errors"),"Exact file limit was not honored");
      table->setCurrentCell(1,0); button(panel,"comparisonRemoveDirectory")->click(); require(table->rowCount()==2 && table->item(1,0)->text()==dir3,"Directory removal lost retained rows");
      picked=false;
      QTimer::singleShot(0,&panel,[&] { if (auto *dialog=panel.findChild<QFileDialog *>()) { dialog->setDirectory(dir2); picked=true; QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection); } });
      dynamic_cast<QPushButton *>(table->cellWidget(1,2))->click();
      require(picked && table->item(1,0)->text()==dir2 && table->item(0,0)->text()==dir1,"Directory picker edited the wrong row after removal");
      // A large complete result stays paged; export must include its end.
      scope->setCurrentIndex(0); base->setText(left); candidate->setText(right); mode->setCurrentIndex(0);
      require(!panel.findChild<QLineEdit *>("comparisonTolerance")->isEnabled() && panel.findChild<QCheckBox *>("comparisonSkipBlank")->isEnabled(),"Text-mode dependencies incorrect");
      QByteArray many=QByteArray(128*1024,'a')+"\n"; write(left,many.repeated(70)); many[0]='b'; write(right,many.repeated(70));
      int heartbeats=0; QTimer heartbeat; heartbeat.setInterval(0); QObject::connect(&heartbeat,&QTimer::timeout,[&] { ++heartbeats; }); heartbeat.start();
      button(panel,"comparisonStart")->click(); wait(panel); heartbeat.stop();
      require(heartbeats>0 && QFileInfo(panel.resultsPath()).size()>16*1024*1024,"Large comparison blocked events or omitted complete differences");
      button(panel,"reportLast")->click(); require(panel.findChild<QPlainTextEdit *>("report:Comparison")->toPlainText().contains("Different rows: 70"),"Cannot inspect end of large results");
      const auto retained=panel.resultsPath(); button(panel,"comparisonStart")->click(); button(panel,"comparisonStop")->click(); wait(panel);
      require(panel.resultsPath()==retained && panel.findChild<QLabel *>("comparisonStatus")->text().contains("stopped"),"Stop replaced previous complete result");
      require(panel.saveResults(saved,true).isEmpty() && read(saved)==read(retained),"Large export truncated results");
      { auto *closing=new ComparisonPanel(left); closing->findChild<QLineEdit *>("comparisonCandidate")->setText(right); button(*closing,"comparisonStart")->click(); delete closing; }
      panel.close();

      MainWindow window; require(argc>1 && window.initialize(QString::fromLocal8Bit(argv[1])),"Engine initialization failed");
      const auto missionReport=files.filePath("mission-report.txt"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor");
      const QString source="Create Variable V;\nV = 2;\nCreate ReportFile Values;\nValues.Filename = '"+missionReport+"';\nValues.WriteHeaders = false;\nValues.FixedWidth = false;\nValues.Precision = 16;\nBeginMissionSequence;\nReport Values V;\nV = V + 3;\nReport Values V;\n";
      editor->setPlainText(source); require(window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Reference report run failed");
      const auto reference=read(missionReport); require(QString::fromUtf8(reference).simplified()=="2 5","Literal report result wrong");
      const auto snapshot=files.filePath("reference.txt"); write(snapshot,reference);
      const bool dirty=editor->document()->isModified();
      auto *output=window.findChild<QTreeWidget *>("Output"); const auto items=output->findItems("Values",Qt::MatchExactly|Qt::MatchRecursive); require(items.size()==1,"Actual report output missing");
      QMetaObject::invokeMethod(output,"itemDoubleClicked",Qt::DirectConnection,Q_ARG(QTreeWidgetItem *,items.front()),Q_ARG(int,0));
      auto *reportCompare=window.findChild<QPushButton *>("reportCompare"); require(reportCompare,"Actual report lacks Compare action"); reportCompare->click();
      auto *workspace=window.findChild<QMdiArea *>("workspace"); ComparisonPanel *actual=nullptr;
      for (auto *child:workspace->subWindowList()) if (auto *candidate=dynamic_cast<ComparisonPanel *>(child->widget())) actual=candidate;
      require(actual && actual->findChild<QLineEdit *>("comparisonBaseline")->text()==missionReport,"Report Compare did not prefill baseline");
      actual->findChild<QLineEdit *>("comparisonCandidate")->setText(snapshot); actual->findChild<QComboBox *>("comparisonMode")->setCurrentIndex(2);
      button(*actual,"comparisonStart")->click(); wait(*actual); require(read(actual->resultsPath()).contains("EQUAL"),"Generated report comparison differs");
      require(editor->toPlainText()==source && editor->document()->isModified()==dirty,"Comparison changed source or dirty state");
      const auto savedScript=files.filePath("comparison mission ü.script"); require(window.saveScriptTo(savedScript) && window.loadScript(savedScript) && window.buildScript() && window.runMission()==MainWindow::RunResult::Completed,"Compared mission save/reopen failed");
      require(read(missionReport)==reference,"Comparison or round trip changed calculation/report");
      auto *action=window.findChild<QAction *>("compareFiles"); require(action,"File menu comparison action missing"); action->trigger();
      require(workspace->subWindowList().size()>=4,"File comparison action failed to open workspace");
      QApplication::processEvents();
      std::cout<<"PASS: text/numeric lines/UTC columns, tolerance and trailing rows, Unicode and invalid files, directory prefixes/three targets/truth/limits, adjustable widths, pending validation/Stop/close/picker Cancel, complete paged export and input protection, generated reports and calculation-preserving save/reopen.\n";
      return 0;
   } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
}
