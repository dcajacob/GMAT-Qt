#include "MainWindow.hpp"
#include "ScriptEditor.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTest>
#include <QTextBlock>
#include <iostream>
#include <stdexcept>

static void require(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
static QString read(const QString &path) {
   QFile file(path); require(file.open(QIODevice::ReadOnly),"Expected file missing"); return QString::fromUtf8(file.readAll());
}
static void proof(const char *checkpoint,const QStringList &facets) {
   QJsonArray values; for (const auto &facet:facets) values.append(facet);
   const QJsonObject record{{"checkpoint",checkpoint},{"family","script-editor"},{"outcome","passed"},{"facets",values}};
   std::cout<<"NASA_CHECK "<<QJsonDocument(record).toJson(QJsonDocument::Compact).constData()<<std::endl;
}
int main(int argc,char **argv) {
   QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); QApplication app(argc,argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("NasaScriptEditor");
   try {
      require(argc==2,"Supply the startup file"); TestSettings settings; QTemporaryDir files; require(files.isValid(),"Fixture unavailable");
      const auto startup=QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath(); QDir::setCurrent(QFileInfo(startup).absolutePath());
      MainWindow window; window.show(); require(window.initialize(startup),"Initialization failed");
      require(QTest::qWaitForWindowExposed(&window),"Main window did not expose");
      auto *source=dynamic_cast<ScriptEditor *>(window.findChild<QPlainTextEdit *>("scriptEditor"));
      auto *area=window.findChild<QMdiArea *>("workspace"); require(source && area,"Script workspace missing");
      QMdiSubWindow *originalWindow=nullptr;
      for (auto *child:area->subWindowList()) if (child->widget()==source) originalWindow=child;
      require(originalWindow,"Script subwindow missing");
      QMenu *edit=nullptr;
      for (auto *action:window.menuBar()->actions()) if (action->text().remove('&')=="Edit") edit=action->menu();
      require(edit,"Edit menu missing");
      auto action=[&](const char *name) { auto *value=window.findChild<QAction *>(name); require(value,"Required editing action missing"); return value; };
      auto *comment=action("scriptComment"),*uncomment=action("scriptUncomment"),*indent=action("scriptIndent"),*unindent=action("scriptUnindent");
      auto *all=action("edit_selectAll"),*copy=action("edit_copy"),*cut=action("edit_cut"),*paste=action("edit_paste");
      auto *undo=action("edit_undo"),*redo=action("edit_redo"),*numbers=action("scriptLineNumbers");
      require(comment->shortcut()==QKeySequence("Ctrl+R") && uncomment->shortcut()==QKeySequence("Ctrl+T") &&
         indent->shortcut()==QKeySequence("Ctrl+I") && unindent->shortcut()==QKeySequence("Ctrl+Shift+I"),"NASA editing shortcuts differ");
      auto focus=[&](QWidget *target) { window.activateWindow(); target->setFocus(); QTest::qWait(15); require(target->hasFocus(),"Editing focus was not acquired"); };
      auto openMenu=[&] {
         if (edit->isVisible()) edit->hide();
         QTest::mouseClick(window.menuBar(),Qt::LeftButton,Qt::NoModifier,window.menuBar()->actionGeometry(edit->menuAction()).center());
         QTest::qWait(15); require(edit->isVisible(),"Edit menu did not open");
      };
      auto menu=[&](QAction *item) {
         openMenu(); require(item->isEnabled(),"Menu action disabled");
         QTest::mouseClick(edit,Qt::LeftButton,Qt::NoModifier,edit->actionGeometry(item).center()); QTest::qWait(15);
         require(!edit->isVisible(),"Menu action did not close its menu");
      };
      auto key=[&](QAction *item) {
         const auto chord=item->shortcut()[0]; QTest::keyClick(source,Qt::Key(chord.key()),chord.keyboardModifiers()); QTest::qWait(15);
      };
      const auto report=files.filePath("editor results.txt");
      const QString base="% retain Unicode Δ and unrelated source\nCreate Variable x;\nCreate ReportFile Values;\nValues.Filename = '"+report+"';\nValues.WriteHeaders = false;\nBeginMissionSequence;\nx = 2;\nx = x * 3;\nReport Values x;\n";
      source->setPlainText(base); require(window.buildScript(),"Reference mission failed to build"); focus(source);
      auto cursorAt=[&](int position) { QTextCursor cursor(source->document()); cursor.setPosition(position); source->setTextCursor(cursor); };
      // Actual key events establish typing history independently of block-edit APIs.
      source->document()->setModified(false); cursorAt(base.size());
      const auto typingMission=window.missionSnapshot();
      QTest::keyClicks(source,"% typed");
      require(source->toPlainText()==base+"% typed" && source->document()->isModified() && source->textCursor().position()==base.size()+7,"Typing did not insert exact text and advance the caret");
      menu(undo);
      require(source->toPlainText()==base && !source->document()->isModified() && source->textCursor().position()==base.size(),"Typing Undo did not restore exact source, clean state and caret");
      key(redo);
      require(source->toPlainText()==base+"% typed" && source->document()->isModified() && source->textCursor().position()==base.size()+7,"Typing Redo did not restore exact text, dirty state and caret");
      menu(undo);
      require(source->toPlainText()==base && window.missionSnapshot().sourceScript==typingMission.sourceScript && window.missionSnapshot().nodes.size()==typingMission.nodes.size(),"Typing history rebuilt the loaded mission");
      auto selectPair=[&](bool backward) {
         cursorAt(source->toPlainText().indexOf(backward ? "Report Values x;" : "x = 2;"));
         QTest::keyClick(source,backward ? Qt::Key_Up : Qt::Key_Down,Qt::ShiftModifier);
         QTest::keyClick(source,backward ? Qt::Key_Up : Qt::Key_Down,Qt::ShiftModifier);
         require(source->textCursor().hasSelection(),"Keyboard block selection missing");
      };
      const auto commented=QString(base).replace("x = 2;\nx = x * 3;","% x = 2;\n% x = x * 3;");
      selectPair(false); menu(comment);
      require(source->toPlainText()==commented && source->textCursor().hasSelection(),"Comment did not change only the selected lines");
      require(!source->toPlainText().contains("% Report Values"),"Selection ending at column zero changed the next line");
      key(undo); require(source->toPlainText()==base,"Comment Undo was not one exact operation");
      key(redo); require(source->toPlainText()==commented,"Comment Redo was not exact");
      selectPair(false); menu(uncomment); require(source->toPlainText()==base,"Uncomment did not restore the source");
      key(undo); require(source->toPlainText()==commented,"Uncomment Undo was not exact");
      key(redo); require(source->toPlainText()==base,"Uncomment Redo was not exact");
      selectPair(true); menu(indent);
      const auto indented=QString(base).replace("x = 2;\nx = x * 3;","   x = 2;\n   x = x * 3;");
      require(source->toPlainText()==indented && source->textCursor().position()<source->textCursor().anchor(),"Indent lost source or backwards selection");
      key(undo); require(source->toPlainText()==base,"Indent Undo was not exact"); key(redo);
      require(source->toPlainText()==indented,"Indent Redo was not exact");
      selectPair(true); key(unindent); require(source->toPlainText()==base,"Unindent shortcut failed");
      const auto steps=source->document()->availableUndoSteps(); key(unindent);
      require(source->toPlainText()==base && source->document()->availableUndoSteps()==steps,"Unindent on unindented lines created an edit");
      cursorAt(source->toPlainText().indexOf("x = 2;")+3); key(comment);
      require(source->toPlainText()==QString(base).replace("x = 2;","% x = 2;") && source->textCursor().positionInBlock()==5,"Current-line comment lost the caret");
      key(uncomment); require(source->toPlainText()==base && source->textCursor().positionInBlock()==3,"Current-line uncomment lost the caret");

      source->setPlainText("  % Unicode Δ\n\t%  keep one space\nx = '100%';\n"); focus(source); menu(all); menu(uncomment);
      require(source->toPlainText()=="  Unicode Δ\n\t keep one space\nx = '100%';\n","Uncomment damaged whitespace, Unicode or an embedded percent");
      source->setPlainText("\tx = 2;\n  x = 3;\n   x = 4;\nx = 5;\n"); focus(source); menu(all); menu(unindent);
      require(source->toPlainText()=="x = 2;\nx = 3;\nx = 4;\nx = 5;\n","Unindent did not remove one tab or up to three leading spaces");
      source->clear(); focus(source); key(comment); require(source->toPlainText()=="% ","Empty current line cannot be commented");
      key(uncomment); require(source->toPlainText().isEmpty(),"Empty current line cannot be uncommented");
      source->setPlainText(base); source->document()->setModified(false); focus(source); menu(all);
      require(source->textCursor().selectionStart()==0 && source->textCursor().selectionEnd()==base.size(),"Select All missed part of the document");
      menu(copy); require(QApplication::clipboard()->text()==base && source->toPlainText()==base,"Copy changed the source or copied the wrong text");
      menu(cut); require(source->toPlainText().isEmpty(),"Cut did not remove the selected source");
      key(undo); require(source->toPlainText()==base,"Cut Undo was not exact");
      menu(all); menu(cut); menu(paste); require(source->toPlainText()==base,"Paste did not restore the copied source");
      cursorAt(base.indexOf("x = 2;")); source->document()->setModified(false);
      const auto numberCursor=source->textCursor().position(),numberUndo=source->document()->availableUndoSteps();
      const auto wideMargin=source->viewport()->geometry().left();
      require(source->lineNumbersVisible() && source->findChild<QWidget *>("lineNumbers")->isVisible(),"Default line numbers missing");
      menu(numbers);
      require(!source->lineNumbersVisible() && !source->findChild<QWidget *>("lineNumbers")->isVisible() && source->viewport()->geometry().left()<wideMargin,"Line-number toggle did not reclaim the margin");
      require(source->toPlainText()==base && source->textCursor().position()==numberCursor && !source->document()->isModified() && source->document()->availableUndoSteps()==numberUndo,"Line-number toggle changed the document");
      menu(numbers); require(source->lineNumbersVisible() && source->findChild<QWidget *>("lineNumbers")->isVisible() && source->viewport()->geometry().left()==wideMargin,"Line numbers and margin did not return");

      const auto second=files.filePath("other ü.script"); { QFile file(second); require(file.open(QIODevice::WriteOnly),"Cannot write second document"); file.write("% other\nCreate Variable other;\n"); }
      require(window.openScriptDocument(second),"Second document did not open");
      auto *other=dynamic_cast<ScriptEditor *>(area->activeSubWindow()->widget()); require(other && other!=source,"Second editor missing"); focus(other);
      menu(numbers); require(!other->lineNumbersVisible() && source->lineNumbersVisible(),"Line-number state leaked across documents");
      const auto originalCaret=source->textCursor().position(); menu(all);
      require(other->textCursor().selectionStart()==0 && other->textCursor().selectionEnd()==other->toPlainText().size() && source->textCursor().position()==originalCaret && source->toPlainText()==base,"Select All changed the background document or selected the wrong editor");
      QTest::keyClick(other,Qt::Key_Home,Qt::ControlModifier); menu(comment);
      require(other->toPlainText().startsWith("% % other") && source->toPlainText()==base,"Block action edited the active mission instead of the selected document");
      area->setActiveSubWindow(originalWindow); focus(source); openMenu(); require(numbers->isChecked(),"Line-number menu did not follow the selected document"); edit->hide();
      source->setReadOnly(true); openMenu(); require(!comment->isEnabled() && !uncomment->isEnabled() && !indent->isEnabled() && !unindent->isEnabled(),"Read-only source exposes mutation actions"); edit->hide();
      source->commentSelection(); require(source->toPlainText()==base,"Read-only operation changed source"); source->setReadOnly(false); focus(source);
      auto *field=new QLineEdit("field text"); auto *fieldWindow=area->addSubWindow(field); fieldWindow->show(); area->setActiveSubWindow(fieldWindow); focus(field);
      openMenu(); require(!comment->isEnabled() && !numbers->isEnabled(),"Non-script field exposes script operations"); edit->hide();
      comment->trigger(); require(field->text()=="field text" && source->toPlainText()==base,"Script action changed a non-script field or background document");
      fieldWindow->close(); area->setActiveSubWindow(originalWindow); focus(source);

      auto runValue=[&](double expected) { require(window.runMission()==MainWindow::RunResult::Completed,"Edited mission did not run"); bool ok=false; const auto value=read(report).trimmed().toDouble(&ok); require(ok && value==expected,"Edited mission result differs from literal expected value"); };
      runValue(6);
      cursorAt(source->toPlainText().indexOf("x = x * 3;")); menu(comment); runValue(2);
      const auto commentPath=files.filePath("commented Δ.script"),commentText=source->toPlainText();
      require(window.saveScriptTo(commentPath) && window.loadScript(commentPath) && source->toPlainText()==commentText,"Commented script did not survive Unicode save/reopen"); runValue(2);
      cursorAt(source->toPlainText().indexOf("% x = x * 3;")); menu(uncomment); runValue(6);
      focus(source); selectPair(false); menu(indent); runValue(6);
      const auto saved=files.filePath("edited Δ.script"); const auto accepted=source->toPlainText();
      require(window.saveScriptTo(saved) && read(saved)==accepted && window.loadScript(saved) && source->toPlainText()==accepted,"Unicode save/reopen did not retain the edits"); runValue(6);
      proof("script.comment", {"menu","shortcut","selection-boundary","current-line","empty-line","undo-redo","unicode","document-isolation","read-only","execution-report","save-reopen"});
      proof("script.uncomment", {"menu","shortcut","current-line","empty-line","leading-whitespace","embedded-percent","undo-redo","unicode","execution-report","save-reopen"});
      proof("script.indent", {"menu","three-spaces","backward-selection","selection-boundary","undo-redo","execution-report","save-reopen"});
      proof("script.unindent", {"menu","shortcut","three-spaces","tabs","backward-selection","no-op","execution-report"});
      proof("script.select-all", {"menu","full-document-selection","clipboard-copy-cut-paste","undo","unicode","document-isolation"});
      proof("script.undo-typing", {"key-events","menu","exact-source","caret","dirty-state","mission-unchanged"});
      proof("script.redo-typing", {"key-events","shortcut","exact-source","caret","dirty-state","mission-unchanged"});
      proof("script.block-selection", {"keyboard","forward-backward-selection","selection-boundary","checked-edit-result"});
      proof("script.cut", {"menu","full-document-selection","clipboard-copy-cut-paste","undo","unicode"});
      proof("script.copy", {"menu","full-document-selection","clipboard-copy-cut-paste","no-source-change","unicode"});
      proof("script.paste", {"menu","full-document-selection","clipboard-copy-cut-paste","exact-source","unicode"});
      proof("script.line-numbers", {"menu","default-visible","margin-restored","no-source-change","no-undo-change","document-isolation"});
      std::cout<<"PASS: NASA Script Editor block menus/shortcuts, half-open selection boundaries, exact grouped Undo/Redo, local line-number state, Unicode clipboard/save/reopen, and independent 6/2/6 execution results\n";
      return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL: "<<error.GetFullMessage()<<'\n'; }
     catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; }
   return 1;
}
