#pragma once
#include <QWidget>
#include <QMap>
#include <QSet>
#include <functional>
class GmatBase;
class QTabWidget;
class QDialog;
class ResourceForm final : public QWidget
{
public:
   explicit ResourceForm(GmatBase &object,QWidget *parent=nullptr,const QString &script={});
   QMap<QString,QString> values() const;
   QMap<QString,QString> changes() const;
   bool isEmpty() const { return readers.isEmpty(); }
   QSet<QString> fields() const;
protected:
   bool eventFilter(QObject *object,QEvent *event) override;
private:
   QTabWidget *tabs;
   QList<std::function<QMap<QString,QString>()>> readers;
   QList<QMap<QString,QString>> originals;
   void add(const QString &label,QDialog *dialog,std::function<QMap<QString,QString>()> read);
};
