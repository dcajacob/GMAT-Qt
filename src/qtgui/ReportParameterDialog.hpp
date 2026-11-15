#pragma once
#include <QDialog>
#include <QStringList>
class QListWidget;
class QComboBox;
class QLabel;
class QEvent;
// Keeps a local ordered selection. Engine validation happens on command Apply.
class ReportParameterDialog final : public QDialog
{
public:
   enum class Mode { Multiple, Single, Writable, WritableReal, StopParameter, FunctionInputs, FunctionOutputs, PlottableSingle, PlottableMultiple, NumericSingle, NumericReference, VaryControl };
   ReportParameterDialog(const QStringList &selected,QWidget *parent=nullptr,Mode mode=Mode::Multiple);
   QStringList selection() const;
   static bool isPlottableReference(const QString &reference);
private:
   bool event(QEvent *event) override;
   void updateUnitNotice();
   QListWidget *list;
   QComboBox *singleEntry=nullptr;
   QComboBox *unitEntry=nullptr;
   QLabel *unitNotice=nullptr;
};
