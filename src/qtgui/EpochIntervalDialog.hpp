#pragma once
#include <QDialog>
#include <QMap>
#include <QSet>
class GmatBase;
class QComboBox;
class QLineEdit;
class QLabel;
bool hasEpochInterval(GmatBase &object);
QMap<QString,QString> epochIntervalSettings(GmatBase &object);
QMap<QString,QString> convertEpochInterval(const QMap<QString,QString> &values,const QString &format);
QSet<QString> applyEpochIntervalProperties(GmatBase &object,const QMap<QString,QString> &values);
QString convertWarmStartEpoch(const QString &epoch,const QString &from,const QString &to);
QString convertEphemerisEpoch(const QString &epoch,const QString &from,const QString &to);
QSet<QString> applyWarmStartProperties(GmatBase &object,const QMap<QString,QString> &values);
class EpochIntervalDialog final : public QDialog
{
public:
   EpochIntervalDialog(GmatBase &object,const QMap<QString,QString> &pending,QWidget *parent=nullptr);
   QMap<QString,QString> settings() const;
private:
   QComboBox *format;
   QLineEdit *start,*finish;
   QLabel *error;
   QString previousFormat;
};
