#pragma once
#include <QDialog>
#include <QMap>
#include <functional>
#include <memory>
class QLineEdit;
class QLabel;
class QTimer;
class PlotCanvas;
struct PlotModel;
class PovModelLoad;

class VisualModelDialog final : public QDialog
{
public:
   using Normalize=std::function<QMap<QString,QString>(const QMap<QString,QString> &)>;
   VisualModelDialog(const QMap<QString,QString> &values,Normalize normalize,QWidget *parent=nullptr);
   QMap<QString,QString> values() const { return acceptedValues; }
   QMap<QString,QString> settings() const;
   std::shared_ptr<PlotModel> previewModel() const { return model; }
   void reject() override;
private:
   bool refreshPreview();
   QMap<QString,QLineEdit *> fields;
   QMap<QString,QString> acceptedValues;
   Normalize normalize;
   std::shared_ptr<PlotModel> model;
   PlotCanvas *canvas=nullptr;
   QLabel *status=nullptr;
   QTimer *refreshTimer=nullptr;
   QString checkedPath;
   QString checkedError;
   bool checkedReadable=false;
   std::shared_ptr<PovModelLoad> modelLoad;
   bool acceptWhenReady=false;
};
