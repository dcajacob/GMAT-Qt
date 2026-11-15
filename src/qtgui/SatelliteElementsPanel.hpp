#pragma once
#include <QWidget>
#include <QMap>
class QLabel;
class QLineEdit;
class QPlainTextEdit;

class SatelliteElementsPanel final : public QWidget
{
public:
   explicit SatelliteElementsPanel(QWidget *parent=nullptr);
   // Returns whether this source is an orbital-elements file or needs repair.
   bool setSource(const QString &filename,const QString &catalogId);
private:
   QLabel *fileLabel,*status;
   QPlainTextEdit *source;
   QMap<QString,QLineEdit *> fields;
};
