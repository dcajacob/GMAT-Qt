#include "MainWindow.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTimer>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
static QByteArray read(const QString &path) { QFile file(path); require(file.open(QIODevice::ReadOnly),"Named function evidence missing"); return file.readAll(); }
static void saveNew(QWidget *owner,QPushButton *button,const QString &path,bool expectEditor=true)
{
   std::exception_ptr failure; bool chosen=false,saved=false;
   QTimer::singleShot(0,owner,[&] { auto *chooser=owner->findChild<QFileDialog *>("newFunctionFileDialog"); try { require(chooser,"Named function chooser missing"); chooser->selectFile(path); chosen=true; if (expectEditor) QTimer::singleShot(0,owner,[&] { auto *dialog=owner->findChild<QDialog *>("functionFileDialog"); try { require(dialog,"Named function editor missing"); require(!owner->findChild<QFileDialog *>("newFunctionFileDialog"),"Hidden file chooser survives into the function editor"); auto *text=dialog->findChild<QPlainTextEdit *>("functionFileText"); require(text && text->toPlainText().startsWith("function [output] = "+QFileInfo(path).completeBaseName()+"(input)\n"),"Function template declaration does not match chosen filename"); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click(); saved=dialog->result()==QDialog::Accepted; if (!saved) dialog->reject(); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } }); QMetaObject::invokeMethod(chooser,"accept",Qt::DirectConnection); } catch (...) { failure=std::current_exception(); if (chooser) chooser->reject(); } });
   button->click(); if (failure) std::rethrow_exception(failure); require(chosen && saved==expectEditor,"Named function file save/validation failed");
}
int main(int argc,char **argv)
{
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv); app.setOrganizationName("GMATTests"); app.setApplicationName("QtFunctionFileNames");
   try {
      TestSettings settings; QTemporaryDir files; require(argc==2 && files.isValid(),"Named function setup failed"); const auto startup=QFileInfo(argv[1]).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath()); MainWindow window; window.show(); require(window.initialize(startup),"Named function runtime unavailable"); auto *editor=window.findChild<QPlainTextEdit *>("scriptEditor"); auto *area=window.findChild<QMdiArea *>("workspace"); require(QDir(files.path()).mkpath("named functions Δ"),"Named function Unicode folder failed"); const auto first=files.filePath("named functions Δ/FirstFile.gmf"),second=files.filePath("named functions Δ/SecondFile.gmf"),invalid=files.filePath("named functions Δ/bad name.gmf"),report=files.filePath("function value.txt"),saved=files.filePath("named function Δ.script");
      const QString baseline="% retain source around function alias α\nCreate Variable Keep;\nKeep = 9;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nBeginMissionSequence;\nReport Values Keep;\n"; editor->setPlainText(baseline); require(window.buildScript(),"Named function baseline failed");
      std::exception_ptr failure; bool accepted=false;
      QTimer::singleShot(0,&window,[&] { auto *dialog=window.findChild<QDialog *>("newResourceDialog"); try { require(dialog,"Named function creator missing"); dialog->findChild<QComboBox *>("resourceType")->setCurrentText("GmatFunction"); dialog->findChild<QLineEdit *>("resourceName")->setText("Alias"); auto *path=dialog->findChild<QLineEdit *>("resourceFunctionPath"); auto *button=dialog->findChild<QPushButton *>("resourceFunctionNew"); saveNew(dialog,button,invalid,false); require(path->text().isEmpty() && !QFileInfo::exists(invalid) && editor->toPlainText()==baseline && dialog->findChild<QLabel *>("resourceCreationStatus")->text().contains("filename"),"Invalid template filename wrote a file or changed mission"); saveNew(dialog,button,first); require(path->text()==first && editor->toPlainText()==baseline,"Named template save changed source before Create"); dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click(); accepted=dialog->result()==QDialog::Accepted; if (!accepted) dialog->reject(); } catch (...) { failure=std::current_exception(); if (dialog) dialog->reject(); } }); window.findChild<QAction *>("createResource")->trigger(); if (failure) std::rethrow_exception(failure); require(accepted && Moderator::Instance()->GetConfiguredObject("Alias"),"New function filename alias did not build"); const auto initialSource=editor->toPlainText(); const auto originalFile=read(first); require(originalFile.startsWith("function [output] = FirstFile(input)\n") && initialSource=="Create GmatFunction Alias;\nGMAT Alias.FunctionPath = '"+first+"';\n"+baseline,"New filename alias lost source identity");
      auto *child=area->activeSubWindow(); auto *panel=child ? dynamic_cast<ResourceEditor *>(child->widget()) : nullptr; require(panel,"Created aliased function MDI panel missing"); auto *newFile=panel->findChild<QPushButton *>("newFunctionFile"); require(newFile,"Aliased resource New function file action missing"); saveNew(panel,newFile,invalid,false); require(!panel->hasChanges() && !QFileInfo::exists(invalid) && editor->toPlainText()==initialSource,"Existing resource invalid template filename changed pending/source"); saveNew(panel,newFile,second); require(panel->hasChanges() && editor->toPlainText()==initialSource && read(first)==originalFile,"New differently named file did not remain pending"); const auto secondFile=read(second); require(secondFile.startsWith("function [output] = SecondFile(input)\n"),"Resource file template used its alias instead of filename"); panel->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click(); const auto updated=editor->toPlainText(); auto unrelated=initialSource; unrelated.remove("GMAT Alias.FunctionPath = '"+first+"';\n"); auto actualUnrelated=updated; const QRegularExpression assignment("^[ \t]*(?:GMAT[ \t]+)?Alias\\.FunctionPath[ \t]*=[^;\n]*;[ \t]*\n",QRegularExpression::MultilineOption); actualUnrelated.remove(assignment); require(actualUnrelated==unrelated && !updated.contains(first) && updated.contains(second) && QString::fromStdString(Moderator::Instance()->GetConfiguredObject("Alias")->GetStringParameter("FunctionPath"))==second,"Aliased new function path Apply failed or changed unrelated source"); editor->undo(); require(editor->toPlainText()==initialSource && window.buildScript(),"Named function path Undo failed"); editor->redo(); require(editor->toPlainText()==updated && window.buildScript(),"Named function path Redo failed"); require(read(first)==originalFile && read(second)==secondFile,"Mission Undo/Redo altered saved function files");
      auto snapshot=window.missionSnapshot(); int index=-1; for (int i=0;i<snapshot.nodes.size();++i) if (snapshot.nodes[i].type=="Report") index=i; require(index>=0 && window.applyMissionChange(snapshot,index,MissionEdit::InsertBefore,"Keep = Alias(Keep);").isEmpty(),"Named function alias call insertion failed"); const auto executionSource=editor->toPlainText(); require(window.saveScriptTo(saved) && read(saved)==executionSource.toUtf8() && window.loadScript(saved),"Named function Unicode source save/reopen failed"); require(window.runMission()==MainWindow::RunResult::Completed && QString::fromUtf8(read(report)).trimmed().toDouble()==9 && editor->toPlainText()==executionSource && read(first)==originalFile && read(second)==secondFile,"Generated aliased function did not preserve independent input/source/file results");
      std::cout<<"PASS: actual New resource and existing MDI New function file templates follow chosen FirstFile/SecondFile basenames while resource Alias is retained, invalid filename correction without writes, pending Apply, exact source Undo/Redo/Unicode save/reopen, unchanged external files and independently expected pass-through result 9. No prior creation/import or native layout repeat.\n";
   } catch (BaseException &e) { std::cerr<<"FAIL: "<<e.GetFullMessage()<<'\n'; return 1; } catch (const std::exception &e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
