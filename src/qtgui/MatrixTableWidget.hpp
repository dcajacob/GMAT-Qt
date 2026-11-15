#pragma once
#include <QTableWidget>
#include <QVector>

class MatrixTableWidget final : public QTableWidget
{
public:
   explicit MatrixTableWidget(int rows,int columns,QWidget *parent=nullptr);
   bool editAction(const char *action);
   static bool editFocusedWidget(QWidget *focused,const char *action);
protected:
   void keyPressEvent(QKeyEvent *event) override;
   void contextMenuEvent(QContextMenuEvent *event) override;
   bool eventFilter(QObject *object,QEvent *event) override;
   void changeEvent(QEvent *event) override;
private:
   void updateMinimumHeight();
   struct CellChange { int row,column; QString before,after; };
   using Change=QVector<CellChange>;
   QVector<Change> history;
   qsizetype historyPosition=0;
   bool paste();
   bool copy();
   bool canStep(bool forward) const;
   bool step(bool forward);
   void showEditorMenu(QWidget *editor,QContextMenuEvent *event);
};
