#pragma once
#include <QDialog>
#include <QMap>
#include <QStringList>
class QTableWidget;
class QComboBox;
struct QtCameraSetting;
class OrbitObjectDrawingDialog final : public QDialog
{
public:
   OrbitObjectDrawingDialog(const QStringList &names,const QMap<QString,bool> &labels,const QMap<QString,bool> &trajectories,QWidget *parent=nullptr,const QtCameraSetting *drawing=nullptr);
   QMap<QString,QString> settings() const;
private:
   QTableWidget *objects,*markers,*fonts,*guides,*modelFiles;
   QComboBox *axesLabels,*frameLabel;
};
