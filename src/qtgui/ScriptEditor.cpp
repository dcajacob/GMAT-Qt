#include "ScriptEditor.hpp"
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QSet>
#include <QPainter>
#include <QPaintEvent>
#include <QTextBlock>
#include <QFontDatabase>

class ScriptHighlighter final : public QSyntaxHighlighter
{
public:
   explicit ScriptHighlighter(ScriptEditor *owner) : QSyntaxHighlighter(owner->document()),editor(owner) {}
   QSet<QString> keywords;
protected:
   void highlightBlock(const QString &text) override
   {
      const bool dark=editor->palette().color(QPalette::Base).lightness()<128;
      const QColor command=dark ? QColor("#82b7ff") : QColor("#164a9c");
      const QColor comment=dark ? QColor("#9bbd91") : QColor("#386638");
      const QColor literal=dark ? QColor("#e7b980") : QColor("#8b410c");
      static const QRegularExpression tokens(
         "(%.*)|('(?:''|[^'])*'?)|((?<![\\w.])(?:[0-9]+(?:\\.[0-9]*)?|\\.[0-9]+)(?:[eE][+-]?[0-9]+)?(?![\\w.]))|([A-Za-z_][A-Za-z_0-9]*)");
      auto matches=tokens.globalMatch(text);
      while (matches.hasNext()) {
         const auto match=matches.next();
         QTextCharFormat format;
         if (match.capturedStart(1)>=0) format.setForeground(comment);
         else if (match.capturedStart(2)>=0 || match.capturedStart(3)>=0) format.setForeground(literal);
         else if (keywords.contains(match.captured(4)) &&
                  (match.capturedStart()==0 || text[match.capturedStart()-1]!='.')) {
            format.setForeground(command); format.setFontWeight(QFont::Bold);
         } else continue;
         setFormat(match.capturedStart(),match.capturedLength(),format);
      }
   }
private:
   ScriptEditor *editor;
};

ScriptEditor::ScriptEditor(QWidget *parent) : QPlainTextEdit(parent),numbers(new QWidget(this)),highlighter(new ScriptHighlighter(this))
{
   setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
   setLineWrapMode(QPlainTextEdit::NoWrap);
   numbers->setObjectName("lineNumbers");
   numbers->installEventFilter(this);
   connect(this,&QPlainTextEdit::blockCountChanged,this,[this] { updateMargin(); });
   connect(this,&QPlainTextEdit::updateRequest,this,[this](const QRect &rect,int delta) {
      if (delta) numbers->scroll(0,delta);
      else numbers->update(0,rect.y(),numbers->width(),rect.height());
   });
   setKeywords({"GMAT","Create","Global","BeginMissionSequence","If","Else","EndIf","For","EndFor","While","EndWhile"});
   updateMargin();
}
void ScriptEditor::setKeywords(const QStringList &words)
{
   for (const auto &word:words) highlighter->keywords.insert(word);
   highlighter->rehighlight();
}
void ScriptEditor::commentSelection() { editLines(LineEdit::Comment); }
void ScriptEditor::uncommentSelection() { editLines(LineEdit::Uncomment); }
void ScriptEditor::indentSelection() { editLines(LineEdit::Indent); }
void ScriptEditor::unindentSelection() { editLines(LineEdit::Unindent); }
void ScriptEditor::setLineNumbersVisible(bool visible)
{
   showNumbers=visible;
   numbers->setVisible(visible);
   updateMargin();
}
void ScriptEditor::editLines(LineEdit operation)
{
   if (isReadOnly()) return;
   auto original=textCursor();
   const bool selected=original.hasSelection(),backward=original.position()<original.anchor();
   auto first=document()->findBlock(original.selectionStart());
   auto last=document()->findBlock(original.selectionEnd());
   // A selection ending at column zero does not contain the next line.
   const bool endsAtBoundary=selected && last.position()==original.selectionEnd();
   if (endsAtBoundary) last=last.previous();
   const int firstNumber=first.blockNumber(),lastNumber=last.blockNumber();
   struct Change { int position; QString before,after; int column,removed,inserted; };
   QList<Change> changes;
   for (auto block=first;block.isValid() && block.blockNumber()<=lastNumber;block=block.next()) {
      const auto before=block.text(); auto after=before;
      int column=0,removed=0,inserted=0;
      if (operation==LineEdit::Comment) { after.prepend("% "); inserted=2; }
      else if (operation==LineEdit::Indent) { after.prepend("   "); inserted=3; }
      else if (operation==LineEdit::Unindent) {
         int count=0;
         if (after.startsWith('\t')) count=1;
         else while (count<3 && count<after.size() && after[count]==' ') ++count;
         after.remove(0,count);
         removed=count;
      } else {
         while (column<after.size() && (after[column]==' ' || after[column]=='\t')) ++column;
         if (column<after.size() && after[column]=='%') {
            int count=1;
            if (column+1<after.size() && after[column+1]==' ') ++count;
            after.remove(column,count);
            removed=count;
         }
      }
      if (before!=after) changes.append({block.position(),before,after,column,removed,inserted});
   }
   if (changes.isEmpty()) return;
   int caret=original.position();
   original.beginEditBlock();
   for (auto change=changes.crbegin();change!=changes.crend();++change) {
      const int start=change->position+change->column;
      if (caret>=start+change->removed) caret+=change->inserted-change->removed;
      else if (caret>=start) caret=start+change->inserted;
      QTextCursor edit(document()); edit.setPosition(change->position);
      edit.setPosition(change->position+change->before.size(),QTextCursor::KeepAnchor);
      edit.insertText(change->after);
   }
   original.endEditBlock();
   if (selected) {
      const int start=document()->findBlockByNumber(firstNumber).position();
      const auto endBlock=document()->findBlockByNumber(lastNumber);
      const int end=endsAtBoundary && endBlock.next().isValid() ? endBlock.next().position() : endBlock.position()+endBlock.text().size();
      original.setPosition(backward ? end : start);
      original.setPosition(backward ? start : end,QTextCursor::KeepAnchor);
   } else original.setPosition(caret);
   setTextCursor(original);
   ensureCursorVisible();
}
void ScriptEditor::updateMargin()
{
   const int width=showNumbers ? 12+fontMetrics().horizontalAdvance('9')*QString::number(blockCount()).size() : 0;
   setViewportMargins(width,0,0,0);
   const auto area=contentsRect();
   numbers->setGeometry(area.x(),area.y(),width,area.height());
   numbers->update();
}
void ScriptEditor::resizeEvent(QResizeEvent *event)
{
   QPlainTextEdit::resizeEvent(event); updateMargin();
}
void ScriptEditor::changeEvent(QEvent *event)
{
   QPlainTextEdit::changeEvent(event);
   if (event->type()==QEvent::FontChange || event->type()==QEvent::PaletteChange) {
      updateMargin(); highlighter->rehighlight();
   }
}
bool ScriptEditor::eventFilter(QObject *object,QEvent *event)
{
   if (object!=numbers || event->type()!=QEvent::Paint) return QPlainTextEdit::eventFilter(object,event);
   const auto *paint=static_cast<QPaintEvent *>(event);
   QPainter painter(numbers);
   painter.fillRect(paint->rect(),palette().color(QPalette::AlternateBase));
   painter.setPen(palette().color(QPalette::Text));
   painter.setOpacity(0.65);
   painter.setFont(font());
   auto block=firstVisibleBlock();
   int y=qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
   while (block.isValid() && y<=paint->rect().bottom()) {
      const int height=qRound(blockBoundingRect(block).height());
      if (block.isVisible() && y+height>=paint->rect().top())
         painter.drawText(0,y,numbers->width()-6,fontMetrics().height(),Qt::AlignRight,QString::number(block.blockNumber()+1));
      y+=height; block=block.next();
   }
   return true;
}
