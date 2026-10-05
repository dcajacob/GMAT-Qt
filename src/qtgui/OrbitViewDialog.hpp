#pragma once
#include <QDialog>
#include <QMap>
#include <QSet>
#include <QString>
#include <array>
#include <optional>
class GmatBase;
class QComboBox;
class QLineEdit;
class QCheckBox;
class QSpinBox;
class QListWidget;
class QLabel;
std::optional<int> orbitSolverLastN(const QString &value);
QSet<QString> applyOrbitViewProperties(GmatBase &plot,const QMap<QString,QString> &values);
QString orbitViewScript(GmatBase &plot);
void validateOrbitViewProperties(GmatBase &plot);
class OrbitViewDialog final : public QDialog
{
public:
   OrbitViewDialog(GmatBase &plot,const QMap<QString,QString> &pending,QWidget *parent=nullptr);
   QMap<QString,QString> settings() const;
private:
   struct CameraInput { QComboBox *choice; std::array<QLineEdit *,3> vector; };
   QListWidget *objects;
   QMap<QString,CameraInput> cameras;
   QMap<QString,QComboBox *> choices;
   QMap<QString,QCheckBox *> flags;
   QMap<QString,QSpinBox *> counts;
   QMap<QString,QString> initialSettings;
   QLineEdit *scale;
   QSpinBox *solverLastN;
   QLabel *error;
};
