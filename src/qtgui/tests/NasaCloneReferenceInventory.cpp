#include "FormationAcceptance.hpp"
#include "ResourceEditor.hpp"
#include "ResourceProperties.hpp"
#include <QTableWidget>
#include <QComboBox>
using namespace formaccept;
int main(int argc,char **argv){QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("NasaCloneReferenceInventory");try{
 require(argc==3,"Supply startup and diagnostic inventory output");Rig rig(QFileInfo(argv[1]).absoluteFilePath());rig.source->setPlainText("Create Barycenter ClonePoint;\nClonePoint.BodyNames = {Earth};\nCreate ReportFile CloneReport;\nBeginMissionSequence;\n");require(rig.window.buildScript(),"Reference inventory build failed");QJsonObject result;
 for(const auto &name:QStringList{"ClonePoint","CloneReport"}){auto *object=Moderator::Instance()->GetConfiguredObject(name.toStdString());require(object,"Reference inventory object absent");QJsonArray fields;for(const auto &p:resourceProperties(*object))fields.append(QJsonObject{{"name",p.name},{"value",p.value},{"choices",QJsonArray::fromStringList(p.choices)},{"references",QJsonArray::fromStringList(p.references)},{"type",QString::fromStdString(object->GetParameterTypeString(object->GetParameterID(p.name.toStdString())))}});ResourceEditor panel(*object,[](const QMap<QString,QString>&){return QString();},nullptr,rig.source->toPlainText());QJsonArray widgets;for(auto *c:panel.findChildren<QComboBox *>()){QJsonArray options;for(int i=0;i<c->count();++i)options.append(QJsonObject{{"label",c->itemText(i)},{"data",c->itemData(i).toString()}});widgets.append(QJsonObject{{"name",c->objectName()},{"options",options},{"value_data",c->property("resourceValueData").toBool()}});}result[name]=QJsonObject{{"fields",fields},{"choices",widgets}};}
 write(QFileInfo(argv[2]).absoluteFilePath(),QJsonDocument(result).toJson());std::cout<<"Diagnostic current resource properties only;no acceptance credit."<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<e.what()<<std::endl;}return 1;}
