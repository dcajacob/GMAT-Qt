#pragma once
#include <QWidget>
#include <QMap>
#include <QSet>
#include <QString>
#include <functional>
class PropSetup;
class Propagator;
class QComboBox;
class QLineEdit;
class QFormLayout;
class PropagatorSettings final : public QWidget
{
public:
   explicit PropagatorSettings(PropSetup &setup,QWidget *parent=nullptr);
   QMap<QString,QString> values() const;
   QMap<QString,QString> changes() const;
   QSet<QString> initialFields() const {return originalFields;}
   void setValue(const QString &name,const QString &value);
   std::function<void(const QString &,const QString &)> onValueChanged;
   std::function<void(const QString &)> onError;
private:
   void populate(Propagator &propagator,const QMap<QString,QString> &pending);
   void selectType(const QString &selected);
   void changed(const QString &name,const QString &value);
   QComboBox *type;
   QLineEdit *forceModel;
   QWidget *forceRow,*owned=nullptr;
   QFormLayout *form;
   QMap<QString,QWidget *> editors;
   QMap<QString,int> fieldTypes;
   QMap<QString,QMap<QString,QString>> cached;
   QMap<QString,QString> original;
   QSet<QString> originalFields;
   QString activeType;
   bool numeric=true,updating=false;
};
