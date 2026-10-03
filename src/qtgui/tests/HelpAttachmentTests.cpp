#include "HelpDialog.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QFile>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QTextBrowser>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *text) { if (!ok) throw std::runtime_error(text); }
static void write(const QString &path,const QByteArray &bytes)
{
   QFile file(path);
   require(file.open(QIODevice::WriteOnly) && file.write(bytes)==bytes.size(),"Help attachment fixture write failed");
}
static QByteArray read(const QString &path)
{
   QFile file(path); require(file.open(QIODevice::ReadOnly),"Help attachment fixture read failed"); return file.readAll();
}
int main(int argc,char **argv)
{
   QApplication app(argc,argv); QApplication::setOrganizationName("GMATTests"); QApplication::setApplicationName("QtHelpAttachments");
   try {
      TestSettings settings; QTemporaryDir fixture; require(fixture.isValid(),"Help attachment fixture directory missing");
      const auto chapter=fixture.filePath("chapter.html"),script=fixture.filePath("literal.script"),function=fixture.filePath("literal.gmf");
      const QString scriptText="\n\n% literal attachment α <tag> & \"quote\"\nCreate Variable V;\n\n   V = 1; % indented β\n\tV = V + 1;\nBeginMissionSequence;\n";
      const QString functionText="function [result] = Literal(input);\n% literal γ <b>plain</b> & <a href='other.html'>text</a>\n\n  result = input;";
      const auto scriptBytes=scriptText.toUtf8(),functionBytes=functionText.toUtf8();
      write(chapter,"<html><body><h1>Fixture chapter</h1><a href='literal.script'>Script</a><a href='literal.gmf'>Function</a></body></html>");
      write(script,scriptBytes); write(function,functionBytes);
      QSettings configuration; configuration.setValue("Help/AttachmentFixture",QUrl::fromLocalFile(chapter).toString());
      HelpDialog help("AttachmentFixture"); help.show(); QApplication::processEvents();
      auto *browser=help.findChild<QTextBrowser *>("helpBrowser"); auto *status=help.findChild<QLabel *>("helpStatus");
      require(browser && browser->isReadOnly() && browser->source().toLocalFile()==chapter && browser->toPlainText().contains("Fixture chapter"),"Fixture chapter did not load read-only");
      auto link=[&](const QString &relative) {
         require(QMetaObject::invokeMethod(browser,"anchorClicked",Qt::DirectConnection,Q_ARG(QUrl,QUrl(relative))),"Help attachment link could not activate");
      };
      auto check=[&](const QString &path,const QString &literal) {
         if (browser->toPlainText()!=literal) std::cerr<<"Literal mismatch at "<<path.toStdString()<<": actual="<<browser->toPlainText().toUtf8().toStdString()<<"\n";
         require(browser->source().toLocalFile()==path && status->text().isEmpty(),"Attachment changed source URL or failed to load");
         require(browser->toPlainText()==literal,"Attachment line breaks, whitespace, Unicode or literal markup changed");
         require(browser->isReadOnly() && browser->document()->toHtml().contains("<pre"),"Attachment is not read-only preformatted text");
      };
      link("literal.script"); check(script,scriptText);
      auto *back=help.findChild<QPushButton *>("helpBack"),*forward=help.findChild<QPushButton *>("helpForward");
      auto *find=help.findChild<QLineEdit *>("helpFindText");
      auto enterFind=[&](QPushButton *previousFocus,Qt::Key key,const QString &path,const QString &text) {
         // Focus a navigation/search button first, as a clicked attachment leaves
         // the dialog's previous auto-default button eligible to handle Return.
         previousFocus->setFocus(); QApplication::processEvents();
         find->setText(text); find->setFocus(); QApplication::processEvents();
         QKeyEvent press(QEvent::KeyPress,key,Qt::NoModifier,"\r"),release(QEvent::KeyRelease,key,Qt::NoModifier,"\r");
         QApplication::sendEvent(find,&press); QApplication::sendEvent(find,&release); QApplication::processEvents();
         require(help.isVisible() && browser->source().toLocalFile()==path,"Find Return activated an unrelated dialog button or navigated away");
         require(status->text().isEmpty() && browser->textCursor().selectedText()==text,"Find Return did not select the literal attachment text");
      };
      enterFind(back,Qt::Key_Return,script,"BeginMissionSequence"); check(script,scriptText);
      require(back->isEnabled(),"Attachment lacks backward history"); back->click();
      require(browser->source().toLocalFile()==chapter && browser->toPlainText().contains("Fixture chapter") && forward->isEnabled(),"Back did not restore chapter or enable Forward");
      forward->click(); check(script,scriptText);
      back->click(); link("literal.gmf"); check(function,functionText);
      back->click(); require(browser->source().toLocalFile()==chapter,"Function Back lost chapter");
      forward->click(); check(function,functionText);
      enterFind(help.findChild<QPushButton *>("helpFindNext"),Qt::Key_Enter,function,"<b>plain</b>");
      find->setText("<b>plain</b>");
      help.findChild<QPushButton *>("helpFindNext")->click(); require(browser->textCursor().selectedText()=="<b>plain</b>","Literal attachment search interpreted markup");
      // File validation must reject a pending URL without replacing the current attachment.
      link("missing.script"); require(status->text().contains("Cannot read") && browser->source().toLocalFile()==function,"Missing attachment silently loaded or lost current page");
      const auto invalid=fixture.filePath("invalid.gmf"); write(invalid,QByteArray("invalid UTF-8 \xff")); link("invalid.gmf");
      require(status->text().contains("UTF-8") && browser->source().toLocalFile()==function,"Invalid attachment UTF-8 accepted or lost current page");
      const QString recovered="% recovered δ <>&\n\nCreate Variable R;\n"; write(invalid,recovered.toUtf8());
      help.findChild<QPushButton *>("helpRetry")->click(); check(invalid,recovered);
      back->click(); check(function,functionText); forward->click(); check(invalid,recovered);
      require(read(script)==scriptBytes && read(function)==functionBytes,"Viewing attachment rewrote its source bytes");
      std::cout<<"PASS: local script/function Help attachments retain literal UTF-8 lines, spaces, markup, source URLs, read-only search/Return and Back/Forward; missing/invalid pending URL recovery preserves current content and source bytes\n";
   } catch (const std::exception &error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
   return 0;
}
