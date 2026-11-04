#include "InspectionDialog.hpp"
#include "FindReplaceDialog.hpp"
#include "BaseException.hpp"
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QCheckBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QSaveFile>
#include <QFontDatabase>
#include <QShortcut>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <stdexcept>
#include <memory>
#include <QSignalBlocker>
#include <tuple>

InspectionDialog::InspectionDialog(const QString &title,const QString &contents,const QString &context,
                                   QWidget *parent,ProtectedPaths paths)
   : QDialog(parent),protectedPaths(std::move(paths))
{
   setObjectName("inspectionDialog"); setWindowTitle(title); resize(900,650);
   auto *layout=new QVBoxLayout(this);
   auto *description=new QLabel(context,this); description->setWordWrap(true); layout->addWidget(description);
   text=new QPlainTextEdit(contents,this); text->setObjectName("inspectionText"); text->setReadOnly(true);
   text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont)); text->setLineWrapMode(QPlainTextEdit::NoWrap);
   layout->addWidget(text,1);
   status=new QLabel(this); status->setObjectName("inspectionStatus"); status->setWordWrap(true); layout->addWidget(status);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Close,this); layout->addWidget(buttons);
   auto *find=new QPushButton("Find…",this); find->setObjectName("inspectionFind"); buttons->addButton(find,QDialogButtonBox::ActionRole);
   auto *search=new FindReplaceDialog(text,this);
   connect(find,&QPushButton::clicked,search,&FindReplaceDialog::openSearch);
   auto *shortcut=new QShortcut(QKeySequence::Find,this); connect(shortcut,&QShortcut::activated,search,&FindReplaceDialog::openSearch);
   if (protectedPaths) {
      auto *save=new QPushButton("Save as…",this); save->setObjectName("inspectionSave"); buttons->addButton(save,QDialogButtonBox::ActionRole);
      connect(save,&QPushButton::clicked,this,[this] {
         const auto path=QFileDialog::getSaveFileName(this,"Save summary",{},"Text files (*.txt);;All files (*)");
         if (path.isEmpty()) return;
         const auto error=exportText(path); status->setText(error.isEmpty() ? "Summary saved to "+path : error);
      });
   }
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
}
void InspectionDialog::setText(const QString &contents) { text->setPlainText(contents); }
QString InspectionDialog::exportText(const QString &path) const
{
   if (path.isEmpty()) return "Choose a file for the summary.";
   const QFileInfo destination(path);
   if (protectedPaths) for (const auto &protectedPath:protectedPaths()) {
      if (protectedPath.isEmpty()) continue;
      const QFileInfo input(protectedPath);
      if (destination.absoluteFilePath()==input.absoluteFilePath() ||
          (!destination.canonicalFilePath().isEmpty() && destination.canonicalFilePath()==input.canonicalFilePath()))
         return "Choose a different file; the summary cannot overwrite the mission script.";
   }
   QSaveFile file(path);
   if (!file.open(QIODevice::WriteOnly)) return "Cannot save summary: "+file.errorString();
   const auto bytes=text->toPlainText().toUtf8();
   if (file.write(bytes)!=bytes.size() || !file.commit()) return "Cannot save summary: "+file.errorString();
   return {};
}
SummaryDialog::SummaryDialog(const QString &title,const QStringList &frames,bool mission,Loader loader,
                             const QString &context,QWidget *parent,ProtectedPaths paths)
   : InspectionDialog(title,{},context,parent,std::move(paths))
{
   setObjectName("summaryDialog");
   auto *controls=new QWidget(this); auto *row=new QHBoxLayout(controls); row->setContentsMargins(0,0,0,0);
   row->addWidget(new QLabel("Coordinate system",controls));
   auto *frame=new QComboBox(controls); frame->setObjectName("summaryFrame"); frame->addItems(frames);
   frame->setCurrentText("EarthMJ2000Eq"); row->addWidget(frame,1);
   auto *physics=new QCheckBox("Physics commands only",controls); physics->setObjectName("summaryPhysics");
   physics->setVisible(mission); row->addWidget(physics);
   auto *a1=new QCheckBox("Include A1ModJulian",controls); a1->setObjectName("summaryA1Epoch");
   row->addWidget(a1);
   static_cast<QVBoxLayout *>(layout())->insertWidget(1,controls);
   auto previous=std::make_shared<std::tuple<QString,bool,bool>>(frame->currentText(),false,false);
   auto refresh=[this,frame,physics,a1,loader,previous] {
      auto *status=findChild<QLabel *>("inspectionStatus");
      try {
         setText(loader(frame->currentText(),physics->isChecked(),a1->isChecked()));
         *previous={frame->currentText(),physics->isChecked(),a1->isChecked()}; status->clear();
      } catch (BaseException &error) {
         status->setText(QString::fromStdString(error.GetFullMessage()));
      } catch (const std::exception &error) { status->setText(QString::fromUtf8(error.what())); }
      if (!status->text().isEmpty()) {
         const QSignalBlocker frameBlock(frame),physicsBlock(physics),a1Block(a1);
         frame->setCurrentText(std::get<0>(*previous)); physics->setChecked(std::get<1>(*previous));
         a1->setChecked(std::get<2>(*previous));
      }
   };
   connect(frame,&QComboBox::currentTextChanged,this,[refresh] { refresh(); });
   connect(physics,&QCheckBox::toggled,this,[refresh] { refresh(); }); refresh();
   connect(a1,&QCheckBox::toggled,this,[refresh] { refresh(); });
}
