#pragma once
#include <QDialog>
#include <QMap>
#include <QSet>
#include <QString>
#include <memory>
class GmatBase;
class QComboBox;
class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
QSet<QString> applyEventLocatorProperties(GmatBase &object,const QMap<QString,QString> &values,bool creating=false);
class EventLocatorDialog final : public QDialog
{
public:
   EventLocatorDialog(GmatBase &object,const QMap<QString,QString> &pending,QWidget *parent=nullptr);
   ~EventLocatorDialog() override;
   QMap<QString,QString> values() const { return acceptedValues; }
   QMap<QString,QString> settings() const;
private:
   void updateDependencies();
   void convertEpochs();
   std::unique_ptr<GmatBase> original;
   QMap<QString,QLineEdit *> text;
   QMap<QString,QComboBox *> choices;
   QMap<QString,QCheckBox *> checks;
   QMap<QString,QListWidget *> lists;
   QLabel *error;
   QString previousFormat;
   QMap<QString,QString> acceptedValues;
};
