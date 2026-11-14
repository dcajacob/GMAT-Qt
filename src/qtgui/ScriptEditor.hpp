#pragma once
#include <QPlainTextEdit>
#include <QStringList>
class ScriptHighlighter;

class ScriptEditor final : public QPlainTextEdit
{
public:
   explicit ScriptEditor(QWidget *parent=nullptr);
   void setKeywords(const QStringList &words);
   void commentSelection();
   void uncommentSelection();
   void indentSelection();
   void unindentSelection();
   void setLineNumbersVisible(bool visible);
   bool lineNumbersVisible() const { return showNumbers; }
protected:
   void resizeEvent(QResizeEvent *event) override;
   void changeEvent(QEvent *event) override;
   bool eventFilter(QObject *object,QEvent *event) override;
private:
   enum class LineEdit { Comment, Uncomment, Indent, Unindent };
   void editLines(LineEdit operation);
   void updateMargin();
   bool showNumbers=true;
   QWidget *numbers;
   ScriptHighlighter *highlighter;
};
