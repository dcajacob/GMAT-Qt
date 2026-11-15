#include "ComparisonPanel.hpp"
#include "ReportViewer.hpp"
#include "BaseException.hpp"
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFile>
#include <QSaveFile>
#include <QFileInfo>
#include <QDir>
#include <QThread>
#include <QMessageBox>
#include <QSplitter>
#include <QScrollArea>
#include <cmath>

ComparisonPanel::ComparisonPanel(const QString &baseline,QWidget *parent):QWidget(parent)
{
   setObjectName("fileComparisonPanel");
   auto *layout=new QVBoxLayout(this);
   auto *split=new QSplitter(Qt::Vertical,this); split->setObjectName("comparisonSplitter"); split->setChildrenCollapsible(false); layout->addWidget(split,1);
   auto *setupScroll=new QScrollArea(split); setupScroll->setWidgetResizable(true); setupScroll->setFrameShape(QFrame::NoFrame);
   auto *setup=new QWidget; setupScroll->setWidget(setup); auto *form=new QFormLayout(setup);
   scope=new QComboBox(setup); scope->setObjectName("comparisonScope"); scope->addItems({"Two files","Directories"}); form->addRow("Compare",scope);
   auto pathRow=[&](QLineEdit *entry,const QString &id,bool baselineEntry) {
      auto *row=new QWidget(setup); auto *horizontal=new QHBoxLayout(row); horizontal->setContentsMargins(0,0,0,0); horizontal->addWidget(entry,1);
      auto *browse=new QPushButton("Browse…",row); browse->setObjectName(id); horizontal->addWidget(browse);
      connect(browse,&QPushButton::clicked,this,[this,entry,baselineEntry] {
         const auto chosen=scope->currentIndex()==1 && baselineEntry ? QFileDialog::getExistingDirectory(this,"Baseline directory",entry->text()) :
            QFileDialog::getOpenFileName(this,"Comparison file",entry->text(),"Text and reports (*.txt *.report *.data *.script *.eph *.oem *.e *.truth);;All files (*)");
         if (!chosen.isEmpty()) entry->setText(chosen);
      }); return row;
   };
   base=new QLineEdit(baseline,setup); base->setObjectName("comparisonBaseline"); form->addRow("Baseline",pathRow(base,"comparisonBrowseBaseline",true));
   candidate=new QLineEdit(setup); candidate->setObjectName("comparisonCandidate"); fileRow=pathRow(candidate,"comparisonBrowseCandidate",false); form->addRow("Candidate",fileRow);
   directoryControls=new QWidget(setup); auto *dirLayout=new QVBoxLayout(directoryControls); dirLayout->setContentsMargins(0,0,0,0);
   auto *options=new QHBoxLayout; dirLayout->addLayout(options);
   options->addWidget(new QLabel("Baseline prefix",directoryControls)); prefix=new QLineEdit(directoryControls); prefix->setObjectName("comparisonBasePrefix"); options->addWidget(prefix,1);
   options->addWidget(new QLabel("Files",directoryControls)); limit=new QSpinBox(directoryControls); limit->setObjectName("comparisonFileLimit"); limit->setRange(0,1000000); limit->setSpecialValueText("All"); options->addWidget(limit);
   directories=new QTableWidget(0,3,directoryControls); directories->setObjectName("comparisonDirectories"); directories->setHorizontalHeaderLabels({"Candidate directory","Prefix",""});
   directories->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive); directories->horizontalHeader()->setStretchLastSection(false);
   directories->setColumnWidth(0,420); directories->setColumnWidth(1,140); directories->setColumnWidth(2,95);
   directories->setSelectionBehavior(QAbstractItemView::SelectRows); directories->setMinimumHeight(130); directories->setMaximumHeight(150); dirLayout->addWidget(directories);
   auto *dirButtons=new QHBoxLayout; dirLayout->addLayout(dirButtons);
   add=new QPushButton("Add directory",directoryControls); add->setObjectName("comparisonAddDirectory"); dirButtons->addWidget(add);
   remove=new QPushButton("Remove directory",directoryControls); remove->setObjectName("comparisonRemoveDirectory"); dirButtons->addWidget(remove); dirButtons->addStretch();
   connect(add,&QPushButton::clicked,this,&ComparisonPanel::addDirectory);
   connect(remove,&QPushButton::clicked,this,[this] { const int row=directories->currentRow(); if (row>=0 && directories->rowCount()>1) directories->removeRow(row); add->setEnabled(directories->rowCount()<3); remove->setEnabled(directories->rowCount()>1); });
   addDirectory(); form->addRow(directoryControls);
   mode=new QComboBox(setup); mode->setObjectName("comparisonMode"); mode->addItems({"Text lines","Numeric lines (ignore strings)","Numeric data columns"}); form->addRow("Mode",mode);
   tolerance=new QLineEdit("1e-6",setup); tolerance->setObjectName("comparisonTolerance");
   auto *toleranceUnit=new QLabel("Absolute tolerance (column units)",setup);toleranceUnit->setObjectName("comparisonToleranceUnit");form->addRow(toleranceUnit,tolerance);
   tolerance->setToolTip("Applied to each numeric value as stored. Convert or normalize differing units before comparing; no automatic unit conversion is performed.");
   skipBlank=new QCheckBox("Skip blank lines for text comparison",setup); skipBlank->setObjectName("comparisonSkipBlank"); form->addRow(skipBlank);
   auto *description=new QLabel("Directory comparison matches text/report files by replacing the baseline prefix with each candidate prefix. Missing matches also try .truth. Numeric columns skip initial headers and report the maximum difference in each column.",setup); description->setWordWrap(true); form->addRow(description);
   connect(scope,&QComboBox::currentIndexChanged,this,[this,form](int index) {
      fileRow->setVisible(index==0); form->labelForField(fileRow)->setVisible(index==0); directoryControls->setVisible(index==1);
   });
   connect(mode,&QComboBox::currentIndexChanged,this,[this](int index) { tolerance->setEnabled(index!=0); skipBlank->setEnabled(index==0); });
   directoryControls->hide(); tolerance->setEnabled(false);
   auto *placeholder=new QLabel("Choose files or directories, then Compare.",split); placeholder->setObjectName("comparisonPlaceholder"); placeholder->setAlignment(Qt::AlignCenter);
   auto *buttons=new QHBoxLayout; layout->addLayout(buttons);
   compare=new QPushButton("Compare",this); compare->setObjectName("comparisonStart"); buttons->addWidget(compare);
   stop=new QPushButton("Stop",this); stop->setObjectName("comparisonStop"); stop->setEnabled(false); buttons->addWidget(stop);
   save=new QPushButton("Save results…",this); save->setObjectName("comparisonSave"); save->setEnabled(false); buttons->addWidget(save); buttons->addStretch();
   auto *close=new QPushButton("Close",this); close->setObjectName("comparisonClose"); buttons->addWidget(close);
   status=new QLabel(this); status->setObjectName("comparisonStatus"); status->setWordWrap(true); layout->addWidget(status);
   connect(compare,&QPushButton::clicked,this,&ComparisonPanel::start);
   connect(stop,&QPushButton::clicked,this,[this] { if (cancellation) cancellation->store(true); status->setText("Stopping comparison… Previous results are retained."); });
   connect(close,&QPushButton::clicked,this,[this] { if (cancellation) cancellation->store(true); if (parentWidget()) parentWidget()->close(); else this->close(); });
   connect(save,&QPushButton::clicked,this,[this] {
      const auto path=QFileDialog::getSaveFileName(this,"Save comparison results",{},"Comparison results (*.txt);;All files (*)");
      if (path.isEmpty()) return;
      const auto error=saveResults(path,true); status->setText(error.isEmpty() ? "Comparison results saved to "+path : error);
   });
   split->setStretchFactor(1,1); split->setSizes({380,250}); resize(900,750);
}
ComparisonPanel::~ComparisonPanel()
{
   if (worker) { cancellation->store(true); worker->wait(); delete worker; }
}
bool ComparisonPanel::isComparing() const { return worker!=nullptr; }
void ComparisonPanel::addDirectory()
{
   if (directories->rowCount()==3) return;
   const int row=directories->rowCount(); directories->insertRow(row);
   directories->setItem(row,0,new QTableWidgetItem); directories->setItem(row,1,new QTableWidgetItem);
   auto *browse=new QPushButton("Browse…",directories); directories->setCellWidget(row,2,browse);
   // Resolve the current row at click time: removing earlier rows changes indices.
   connect(browse,&QPushButton::clicked,this,[this,browse] {
      for (int row=0;row<directories->rowCount();++row) if (directories->cellWidget(row,2)==browse) {
         const auto path=QFileDialog::getExistingDirectory(this,"Candidate directory",directories->item(row,0)->text());
         if (!path.isEmpty()) directories->item(row,0)->setText(path); break;
      }
   });
   add->setEnabled(directories->rowCount()<3); remove->setEnabled(directories->rowCount()>1);
}
void ComparisonPanel::setBusy(bool busy)
{
   findChild<QSplitter *>("comparisonSplitter")->widget(0)->setEnabled(!busy);
   compare->setEnabled(!busy); stop->setEnabled(busy); save->setEnabled(!busy && !completedPath.isEmpty());
}
void ComparisonPanel::start()
{
   if (worker) return;
   auto fail=[this](const QString &message) { status->setText(message+" Previous results are retained."); };
   bool valid; FileCompareOptions options; options.mode=FileCompareMode(mode->currentIndex()); options.skipBlankLines=skipBlank->isChecked();
   options.tolerance=tolerance->text().toDouble(&valid);
   if (options.mode!=FileCompareMode::Text && (!valid || !std::isfinite(options.tolerance) || options.tolerance<0)) { fail("Tolerance must be finite and nonnegative."); return; }
   if (options.mode==FileCompareMode::Text) options.tolerance=0;
   QVector<FileComparePair> pairs;
   if (scope->currentIndex()==0) {
      if (!QFileInfo(base->text()).isFile() || !QFileInfo(candidate->text()).isFile()) { fail("Select an existing baseline and candidate file."); return; }
      pairs.append({base->text(),candidate->text()});
   } else {
      if (!QFileInfo(base->text()).isDir()) { fail("Select an existing baseline directory."); return; }
      for (int row=0;row<directories->rowCount();++row)
         if (!QFileInfo(directories->item(row,0)->text()).isDir()) { fail(QString("Candidate directory %1 is unavailable.").arg(row+1)); return; }
      auto names=comparisonBaseFiles(base->text(),prefix->text());
      if (limit->value()>0 && names.size()>limit->value()) names=names.mid(0,limit->value());
      for (const auto &name:names) for (int row=0;row<directories->rowCount();++row) {
         auto matched=QDir(directories->item(row,0)->text()).filePath(directories->item(row,1)->text()+name.mid(prefix->text().size()));
         if (!QFileInfo::exists(matched)) {
            const QFileInfo info(matched); const auto truth=info.dir().filePath(info.completeBaseName()+".truth");
            if (QFileInfo::exists(truth)) matched=truth;
         }
         pairs.append({QDir(base->text()).filePath(name),matched});
      }
      if (pairs.isEmpty()) { fail("No matching text/report files in the baseline directory."); return; }
   }
   if (!scratch.isValid()) { fail("Cannot create temporary comparison storage."); return; }
   const auto resultPath=scratch.filePath(QString("comparison-%1.txt").arg(++serial));
   QStringList inputs; for (const auto &pair:pairs) { inputs.append(pair.baseline); inputs.append(pair.candidate); }
   cancellation=std::make_shared<std::atomic_bool>(false); const auto cancel=cancellation;
   setBusy(true); status->setText(QString("Comparing %1 file pairs…").arg(pairs.size()));
   worker=QThread::create([this,pairs,options,resultPath,cancel,inputs] {
      FileCompareResult result;
      try {
         result=compareFiles(pairs,options,resultPath,*cancel,[this](int done,int count) {
            if (done==count || done%16==0)
               QMetaObject::invokeMethod(this,[this,done,count] { if (worker) status->setText(QString("Compared %1 of %2 file pairs…").arg(done).arg(count)); },Qt::QueuedConnection);
         });
      } catch (const BaseException &error) { result.error=QString::fromStdString(error.GetFullMessage()); }
        catch (const std::exception &error) { result.error=QString::fromUtf8(error.what()); }
        catch (...) { result.error="Comparison failed unexpectedly."; }
      QMetaObject::invokeMethod(this,[this,result,resultPath,inputs] {
         worker->wait(); delete worker; worker=nullptr; setBusy(false);
         const bool stopped=result.cancelled || cancellation->load();
         if (stopped || !result.error.isEmpty()) {
            status->setText((stopped ? "Comparison stopped." : result.error)+" Previous results are retained."); QFile::remove(resultPath); return;
         }
         auto *split=findChild<QSplitter *>("comparisonSplitter");
         auto *view=new ReportViewer(resultPath,"Comparison");
         auto *old=split->replaceWidget(1,view); delete old; view->show();
         if (!completedPath.isEmpty()) QFile::remove(completedPath);
         completedPath=resultPath; completedInputs=inputs; save->setEnabled(true);
         status->setText(QString("File pairs compared: %1 · Different: %2 · Errors: %3").arg(result.compared).arg(result.different).arg(result.errors));
      },Qt::QueuedConnection);
   });
   worker->start();
}
QString ComparisonPanel::saveResults(const QString &path,bool replaceExisting) const
{
   if (isComparing()) return "Wait until comparison stops before saving results.";
   if (completedPath.isEmpty()) return "No completed comparison to save.";
   auto identity=[](const QString &name) { const QFileInfo info(name); return info.exists() ? info.canonicalFilePath() : info.absoluteFilePath(); };
   for (const auto &input:completedInputs) if (identity(path)==identity(input)) return "Cannot overwrite a comparison input file.";
   if (identity(path)==identity(completedPath)) return "Choose a destination outside the temporary result file.";
   if (QFileInfo::exists(path) && !replaceExisting) return "Destination exists; confirm replacement before saving.";
   QFile source(completedPath); if (!source.open(QIODevice::ReadOnly)) return "Comparison results unavailable: "+source.errorString();
   QSaveFile output(path); if (!output.open(QIODevice::WriteOnly)) return "Cannot save results: "+output.errorString();
   while (!source.atEnd()) {
      const auto block=source.read(1024*1024);
      if (source.error()!=QFileDevice::NoError || output.write(block)!=block.size()) return "Could not write complete comparison results.";
   }
   if (!output.commit()) return "Cannot save results: "+output.errorString();
   return {};
}
