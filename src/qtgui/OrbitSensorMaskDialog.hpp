#pragma once
#include "ScriptCompatibility.hpp"
#include <QDialog>
class QTableWidget;
class OrbitSensorMaskDialog final : public QDialog
{
public:
   OrbitSensorMaskDialog(const QStringList &sources,const QVector<QtSensorMaskSetting> &masks,QWidget *parent=nullptr);
   QVector<QtSensorMaskSetting> settings() const;
private:
   void append(const QtSensorMaskSetting &mask);
   QTableWidget *table;
   QStringList sources,hardware;
};
