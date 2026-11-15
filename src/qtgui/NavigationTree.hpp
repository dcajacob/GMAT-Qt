#pragma once
#include <QTreeWidget>

class NavigationTree final : public QTreeWidget
{
public:
   explicit NavigationTree(QWidget *parent=nullptr);
protected:
   void keyPressEvent(QKeyEvent *event) override;
};
