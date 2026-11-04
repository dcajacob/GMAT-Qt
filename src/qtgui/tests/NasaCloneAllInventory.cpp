#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ResourceForm.hpp"
#include "ResourceProperties.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>
using namespace formaccept;
int main(int argc,char **argv){QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaCloneAllInventory");try{
 require(argc==3,"Supply startup and inventory output file");TestSettings settings;QTemporaryDir files;const auto startup=QFileInfo(argv[1]).absoluteFilePath(),output=QFileInfo(argv[2]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;window.show();require(window.initialize(startup),"Clone inventory runtime absent");
 const QString script="Create Spacecraft CloneAttitude CloneMass CloneVisual;\nCloneAttitude.Attitude = Spinner;\nCloneAttitude.AttitudeDisplayStateType = Quaternion;\nCloneAttitude.AttitudeRateDisplayStateType = EulerAngleRates;\nCreate ChemicalTank CloneTank SupportA SupportB;\nCreate ChemicalThruster CloneThruster;\nCloneThruster.Tank = {SupportA};\nCloneThruster.CoordinateSystem = Local;\nCloneThruster.Axes = LVLH;\nCloneThruster.Origin = Mars;\nCreate ImpulsiveBurn CloneBurn;\nCloneBurn.Tank = {SupportA};\nCloneBurn.CoordinateSystem = Local;\nCloneBurn.Axes = LVLH;\nCloneBurn.Origin = Mars;\nCloneBurn.DecrementMass = true;\nCreate DifferentialCorrector CloneDC;\nCreate Variable PlotX PlotY PlotZ Keep;\nPlotX = 1;\nPlotY = 2;\nPlotZ = 3;\nKeep = 7;\nCreate XYPlot ClonePlot;\nClonePlot.XVariable = PlotX;\nClonePlot.YVariables = {PlotY};\nBeginMissionSequence;\nKeep = Keep + 1;\n";
 const auto saved=files.filePath("Inventory.script");write(saved,script.toUtf8());require(window.loadScript(saved)&&window.buildScript(),"Clone inventory build failed");QJsonObject result;
 for(const auto &name:QStringList{"CloneAttitude","CloneMass","CloneVisual","CloneTank","CloneThruster","CloneBurn","CloneDC","ClonePlot"}){
  auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString());require(object,"Clone inventory object absent");QJsonArray fields;for(const auto &p:resourceProperties(*object))fields.append(QJsonObject{{"name",p.name},{"value",p.value},{"choices",QJsonArray::fromStringList(p.choices)},{"references",QJsonArray::fromStringList(p.references)},{"list",p.list},{"rows",p.rows},{"columns",p.columns},{"type",QString::fromStdString(object->GetParameterTypeString(object->GetParameterID(p.name.toStdString())))}});
  QDialog dialog;auto *panel=new ResourceEditor(*object,[](const QMap<QString,QString>&){return QString();},&dialog,script);auto *layout=new QVBoxLayout(&dialog);layout->addWidget(panel);dialog.resize(1200,900);dialog.show();events();QJsonArray controls;
  for(auto *c:panel->findChildren<QWidget *>())if(qobject_cast<QLineEdit *>(c)||qobject_cast<QComboBox *>(c)||qobject_cast<QCheckBox *>(c)||qobject_cast<QSpinBox *>(c)){const auto n=c->objectName();if(n.isEmpty()||n.startsWith("qt_"))continue;QJsonObject v{{"name",n},{"class",QString(c->metaObject()->className())},{"enabled",c->isEnabled()}};if(auto *x=qobject_cast<QLineEdit *>(c))v["value"]=x->text();if(auto *x=qobject_cast<QComboBox *>(c)){v["value"]=x->currentText();QJsonArray choices;for(int i=0;i<x->count();++i)choices.append(x->itemText(i));v["choices"]=choices;}if(auto *x=qobject_cast<QCheckBox *>(c))v["value"]=x->isChecked();if(auto *x=qobject_cast<QSpinBox *>(c))v["value"]=x->value();controls.append(v);}
  QJsonArray advanced;auto *table=panel->findChild<QTableWidget *>("resourceProperties");if(table)for(int r=0;r<table->rowCount();++r)advanced.append(QJsonObject{{"field",table->item(r,0)->text()},{"hidden",table->isRowHidden(r)},{"value",table->item(r,1)?table->item(r,1)->text():QString()}});
  result[name]=QJsonObject{{"fields",fields},{"controls",controls},{"advanced",advanced}};dialog.hide();events();
 }
 write(output,QJsonDocument(result).toJson());std::cout<<"Wrote diagnostic active-property inventory for8families;not an acceptance pass."<<std::endl;return 0;
}catch(BaseException &e){std::cerr<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<e.what()<<std::endl;}return 1;}
