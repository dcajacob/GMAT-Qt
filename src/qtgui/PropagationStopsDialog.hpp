#pragma once
#include <QDialog>
#include <QStringList>
class QTableWidget;
class PropagationStopsDialog final : public QDialog
{
public:
   PropagationStopsDialog(const QString &source,QWidget *parent=nullptr);
   static bool supports(const QString &source);
   QString statement() const;
private:
   QString source;
   QStringList options;
   int start=0,length=0,braceStart=-1,braceLength=0;
   QTableWidget *table;
};
