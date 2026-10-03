#pragma once
#include <QDialog>
#include <QMap>
class GmatBase;
class QComboBox;
class QLineEdit;
class QTableWidget;
class CustomFovDialog final : public QDialog
{
public:
   explicit CustomFovDialog(GmatBase &object,const QString &script={},QWidget *parent=nullptr);
   QMap<QString,QString> settings() const;
private:
   QComboBox *source,*coordinates;
   QLineEdit *file,*interpolation;
   QTableWidget *vertices;
};
