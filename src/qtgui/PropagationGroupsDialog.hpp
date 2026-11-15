#pragma once
#include <QDialog>
#include <QStringList>
#include <functional>
// Resolve capabilities only in a configured mission context. Standalone
// statement editors may supply no resolver and retain syntax-only behavior.
QString configuredPropagationError(const QString &source);
class QTableWidget;
class QComboBox;
class QCheckBox;
class PropagationGroupsDialog final : public QDialog
{
public:
   PropagationGroupsDialog(const QString &source,const QStringList &propagators,const QStringList &objects,QWidget *parent=nullptr,std::function<QString(const QString &)> validate={});
   static bool supports(const QString &source);
   static QString appendGroup(const QString &source,const QString &propagator,const QString &spacecraft);
   QString statement() const;
private:
   QString source;
   int start=0,length=0;
   QTableWidget *table;
   QComboBox *mode;
   QComboBox *ephemerisPolicy;
   QCheckBox *backward;
   QCheckBox *stm;
   QCheckBox *aMatrix;
   QCheckBox *covariance;
   bool explicitSTM=false;
};
