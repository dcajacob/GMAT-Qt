#include "FolderRunDialog.hpp"
#include "QtPlotReceiver.hpp"
#include "WorkspaceLayout.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QOpenGLWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QScreen>
#include <QFontDatabase>
FolderRunDialog::FolderRunDialog(const QString &initialOutput,Run run,std::function<void()> stopAction,QWidget *parent)
   : QDialog(parent),stop(std::move(stopAction))
{
   setObjectName("folderRunDialog"); setWindowTitle("Run scripts from folder"); resize(940,760);
   setProperty("helpTopic","ResourceTree");
   // Establish the OpenGL surface type before this native dialog is mapped.
   auto *anchor=new QOpenGLWidget(this); anchor->resize(1,1); anchor->hide();
   auto *layout=new QVBoxLayout(this); auto *tabs=new QTabWidget(this); tabs->setObjectName("folderRunTabs"); layout->addWidget(tabs,1);
   auto *scroll=new QScrollArea(tabs); scroll->setWidgetResizable(true); auto *settings=new QWidget(scroll); scroll->setWidget(settings); auto *form=new QFormLayout(settings); tabs->addTab(scroll,"Options");
   auto path=[&](const QString &label,const QString &name,bool file=false) {
      auto *row=new QWidget(settings); auto *box=new QHBoxLayout(row); box->setContentsMargins(0,0,0,0);
      auto *value=new QLineEdit(row); value->setObjectName(name); box->addWidget(value,1);
      auto *browse=new QPushButton("Browse…",row); browse->setObjectName(name+"Browse"); box->addWidget(browse);
      connect(browse,&QPushButton::clicked,this,[this,value,file] {
         const auto selected=file ? QFileDialog::getSaveFileName(this,"Save comparison results",value->text(),"Text files (*.txt);;All files (*)") : QFileDialog::getExistingDirectory(this,"Select folder",value->text());
         if (!selected.isEmpty()) value->setText(selected);
      }); form->addRow(label,row); return value;
   };
   directory=path("Script folder","folderDirectory"); output=path("Output folder","folderOutput"); output->setText(initialOutput);
   auto number=[&](const QString &label,const QString &name) { auto *value=new QSpinBox(settings); value->setObjectName(name); value->setRange(1,1000000); form->addRow(label,value); return value; };
   first=number("Starting script number","folderFirst"); count=number("Number of scripts","folderCount"); repeats=number("Repeats per script","folderRepeats");
   connect(directory,&QLineEdit::textChanged,this,[this] { count->setValue(qMax(1,folderRunScripts(directory->text()).size())); });
   auto filterRow=[&](const QString &label,const QString &name,QCheckBox **excluded) {
      auto *row=new QWidget(settings); auto *box=new QHBoxLayout(row); box->setContentsMargins(0,0,0,0); auto *value=new QLineEdit(row); value->setObjectName(name); box->addWidget(value,1);
      *excluded=new QCheckBox("Exclude matches",row); (*excluded)->setObjectName(name+"Exclude"); box->addWidget(*excluded); form->addRow(label,row); return value;
   };
   filter=filterRow("Filename contains","folderFilter",&exclude); secondFilter=filterRow("Second filename filter","folderSecondFilter",&secondExclude);
   copies=new QCheckBox("Save script copies and run from that folder",settings); copies->setObjectName("folderSaveCopies"); form->addRow(copies); copy=path("Saved script folder","folderCopyDirectory");
   runFolders=new QCheckBox("Use Run_1, Run_2, … output folders",settings); runFolders->setObjectName("folderRunFolders"); form->addRow(runFolders);
   compare=new QCheckBox("Compare ReportFile outputs",settings); compare->setObjectName("folderCompare"); form->addRow(compare); baseline=path("Comparison folder","folderBaseline");
   replacement=new QLineEdit("GMAT",settings); replacement->setObjectName("folderReplacement"); form->addRow("Replace GMAT in reference filenames with",replacement);
   tolerance=new QDoubleSpinBox(settings); tolerance->setObjectName("folderTolerance"); tolerance->setDecimals(12); tolerance->setRange(0,1e100); tolerance->setValue(1e-6);
   auto *toleranceUnit=new QLabel("Absolute numerical tolerance (column units)",settings);toleranceUnit->setObjectName("folderToleranceUnit");form->addRow(toleranceUnit,tolerance);
   tolerance->setToolTip("Applied to each numeric value as stored. Convert or normalize differing units before comparing; no automatic unit conversion is performed.");
   saveComparison=new QCheckBox("Save comparison results",settings); saveComparison->setObjectName("folderSaveComparison"); form->addRow(saveComparison); comparisonFile=path("Comparison results file","folderComparisonFile",true);
   auto *note=new QLabel("The range refers to the sorted mission list before filters. Functions and backup files are excluded. Saved copies retain exact source bytes; relative assets continue to resolve from the original script folder. Explicit output filenames retain their destinations.",settings); note->setWordWrap(true); form->addRow(note);
   const auto dependencies=[this] {
      copy->parentWidget()->setEnabled(copies->isChecked()); baseline->parentWidget()->setEnabled(compare->isChecked());
      replacement->setEnabled(compare->isChecked()); tolerance->setEnabled(compare->isChecked()); saveComparison->setEnabled(compare->isChecked());
      comparisonFile->parentWidget()->setEnabled(compare->isChecked() && saveComparison->isChecked());
   };
   connect(copies,&QCheckBox::toggled,this,dependencies); connect(compare,&QCheckBox::toggled,this,dependencies); connect(saveComparison,&QCheckBox::toggled,this,dependencies); dependencies();
   results=new QPlainTextEdit(tabs); results->setObjectName("folderResults"); results->setReadOnly(true); results->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont)); results->setLineWrapMode(QPlainTextEdit::NoWrap); tabs->addTab(results,"Results");
   auto *plotPage=new QWidget(tabs); auto *plotLayout=new QVBoxLayout(plotPage); plotLayout->setContentsMargins(0,0,0,0);
   auto *arrange=new QHBoxLayout; plotLayout->addLayout(arrange);
   auto *tile=new QPushButton("Tile windows",plotPage),*cascade=new QPushButton("Cascade windows",plotPage); arrange->addWidget(tile); arrange->addWidget(cascade); arrange->addStretch();
   auto *workspace=new QMdiArea(plotPage); workspace->setObjectName("folderPlots"); plotLayout->addWidget(workspace,1); tabs->addTab(plotPage,"Plots"); plots=std::make_unique<QtPlotReceiver>(workspace);
   connect(tile,&QPushButton::clicked,this,[workspace] { tileWorkspaceWindows(workspace); }); connect(cascade,&QPushButton::clicked,workspace,&QMdiArea::cascadeSubWindows);
   status=new QLabel(this); status->setObjectName("folderStatus"); status->setWordWrap(true); layout->addWidget(status);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Close,this); layout->addWidget(buttons); closeButton=buttons->button(QDialogButtonBox::Close);
   runButton=buttons->addButton("Run",QDialogButtonBox::ActionRole); runButton->setObjectName("folderRun"); stopButton=buttons->addButton("Stop",QDialogButtonBox::ActionRole); stopButton->setObjectName("folderStop"); stopButton->setEnabled(false);
   connect(buttons,&QDialogButtonBox::rejected,this,&FolderRunDialog::reject);
   connect(stopButton,&QPushButton::clicked,this,[this] { cancelled=true; if (stop) stop(); status->setText("Stopping the folder run…"); });
   connect(runButton,&QPushButton::clicked,this,[this,settings,tabs,run] {
      if (busy) return;
      const auto selected=options(); const auto error=validateFolderRun(selected); if (!error.isEmpty()) { status->setText(error); return; }
      busy=true; cancelled=false; settings->setEnabled(false); runButton->setEnabled(false); closeButton->setEnabled(false); stopButton->setEnabled(true); results->clear(); plots->clear(true); tabs->setCurrentWidget(results);
      const auto result=run(selected,*plots,cancelled,[this](int complete,int total,const FolderRunItem &item) {
         status->setText(QString("Run %1 of %2: %3 — %4").arg(complete).arg(total).arg(QFileInfo(item.script).fileName(),item.category));
         results->appendPlainText(QString("%1 — repeat %2 — %3\n%4").arg(item.script).arg(item.repeat).arg(item.category,item.details)); QApplication::processEvents();
      });
      results->setPlainText(result.summary()); status->setText(!result.error.isEmpty() ? result.error : QString(result.cancelled ? "Folder run interrupted. " : "Folder run complete. ")+"Summary: "+result.summaryFile);
      busy=false; settings->setEnabled(true); runButton->setEnabled(true); closeButton->setEnabled(true); stopButton->setEnabled(false);
   });
}
FolderRunDialog::~FolderRunDialog() = default;
void FolderRunDialog::reject() { if (busy) { cancelled=true; if (stop) stop(); } else QDialog::reject(); }
FolderRunOptions FolderRunDialog::options() const
{
   FolderRunOptions value; value.directory=directory->text(); value.outputDirectory=output->text(); value.copyDirectory=copy->text(); value.baselineDirectory=baseline->text(); value.replacement=replacement->text(); value.comparisonFile=comparisonFile->text();
   value.first=first->value(); value.count=count->value(); value.repeats=repeats->value(); value.filter=filter->text(); value.secondFilter=secondFilter->text(); value.exclude=exclude->isChecked(); value.secondExclude=secondExclude->isChecked();
   value.saveCopies=copies->isChecked(); value.runFolders=runFolders->isChecked(); value.compare=compare->isChecked(); value.saveComparison=saveComparison->isChecked(); value.tolerance=tolerance->value(); return value;
}
