#pragma once
#include <QGroupBox>
#include <QStringList>
#include <functional>
class QComboBox;
class QCheckBox;
class QLineEdit;
class PropagationForm final : public QGroupBox
{
public:
   PropagationForm(const QStringList &propagators, const QStringList &spacecraft,
                   std::function<void(const QString &)> changed, QWidget *parent=nullptr);
   void setStatement(const QString &statement);
private:
   QComboBox *propagator, *spacecraft, *units;
   QLineEdit *duration,*stopParameter,*tolerance;
   QCheckBox *backward,*singleStep;
   QString original;
   bool synchronizing=false;
};
