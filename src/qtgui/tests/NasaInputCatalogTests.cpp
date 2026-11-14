// Documentation census only. No mission or prior GUI acceptance case is run.
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "FactoryManager.hpp"
#include "ResourceDraft.hpp"
#include "ResourceProperties.hpp"
#include "ResourceEditor.hpp"
#include "TestSettings.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QAbstractSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>
#include <iostream>
#include <stdexcept>
#include "NestedInputCatalog.hpp"
static void require(bool ok,const QString &message){if(!ok)throw std::runtime_error(message.toStdString());}
static QJsonArray names(const StringArray &values){QJsonArray result;for(const auto &v:values)result.append(QString::fromStdString(v));return result;}
static QJsonArray metadata(GmatBase &object){
 QJsonArray result;
 for(int id=0;id<object.GetParameterCount();++id){
  QJsonObject item{{"id",id}};
  try{item["field"]=QString::fromStdString(object.GetParameterText(id));item["parameter_type"]=QString::fromStdString(object.GetParameterTypeString(id));item["native_unit"]=QString::fromStdString(object.GetParameterUnit(id));item["read_only"]=object.IsParameterReadOnly(id);item["visible"]=object.IsParameterVisible(id);}
  catch(BaseException &e){item["error"]=QString::fromStdString(e.GetFullMessage());}
  catch(std::exception &e){item["error"]=e.what();}
  result.append(item);
 }
 return result;
}
static QJsonArray fields(GmatBase &object){
 QJsonArray result;
 for(const auto &f:resourceProperties(object)){
  result.append(QJsonObject{{"field",f.name},{"unit",f.unit},{"help",f.help},{"choices",QJsonArray::fromStringList(f.choices)},{"references",QJsonArray::fromStringList(f.references)},{"filename",f.filename},{"file_input",f.fileInput},{"file_output",f.fileOutput},{"file_list",f.fileList},{"list",f.list},{"color",f.color},{"rows",f.rows},{"columns",f.columns}});
 }
 return result;
}
static QJsonArray controls(QWidget &owner){
 QJsonArray result;
 for(auto *w:owner.findChildren<QWidget *>()){
  if(!qobject_cast<QLineEdit *>(w)&&!qobject_cast<QComboBox *>(w)&&!qobject_cast<QAbstractSpinBox *>(w)&&!qobject_cast<QCheckBox *>(w)&&!qobject_cast<QPlainTextEdit *>(w)&&!qobject_cast<QTableWidget *>(w))continue;
  const auto name=w->objectName();if(name=="qt_spinbox_lineedit"||name=="qt_scrollarea_viewport")continue;
  QString label;
  for(auto *form:owner.findChildren<QFormLayout *>())if(auto *l=qobject_cast<QLabel *>(form->labelForField(w)))label=l->text();
  QJsonArray ancestors;for(auto *p=w->parentWidget();p&&p!=&owner;p=p->parentWidget())ancestors.append(p->objectName());
  result.append(QJsonObject{{"class",w->metaObject()->className()},{"object_name",name},{"label",label},{"tooltip",w->toolTip()},{"enabled",w->isEnabled()},{"explicitly_hidden",w->isHidden()},{"ancestors",ancestors}});
 }
 return result;
}
int main(int argc,char **argv){
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("InputDocumentationCatalog");
 try{
  require(argc==4,"Supply startup, catalog/type and output path");TestSettings settings;const auto startup=QFileInfo(argv[1]).absoluteFilePath(),output=QFileInfo(argv[3]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;
  require(window.initialize(startup),"Input catalog initialization failed");auto *moderator=Moderator::Instance();QJsonObject report{{"schema_version",1},{"requested",argv[2]},{"platform",app.platformName()},{"mission_executions",0},{"GUI_behavior_qualified",false},{"all_input_documentation_qualified",false}};
  const QString requested=argv[2];
  if(requested=="--catalog"){
   QJsonArray categories;QStringList types;
   for(const auto category:{Gmat::SPACECRAFT,Gmat::FORMATION,Gmat::SPACE_POINT,Gmat::REGION,Gmat::HARDWARE,Gmat::BURN,Gmat::PROP_SETUP,Gmat::ODE_MODEL,Gmat::COORDINATE_SYSTEM,Gmat::SOLVER,Gmat::SUBSCRIBER,Gmat::FUNCTION,Gmat::EVENT_LOCATOR,Gmat::CALCULATED_POINT,Gmat::CELESTIAL_BODY,Gmat::MEASUREMENT_MODEL,Gmat::ERROR_MODEL,Gmat::INTERFACE,Gmat::DATA_FILTER,Gmat::FIELD_OF_VIEW}){
    auto values=moderator->GetListOfViewableItems(category);categories.append(QJsonObject{{"category_id",int(category)},{"types",names(values)}});for(const auto &v:values)types.append(QString::fromStdString(v));
   }
   for(const auto *n:{"Smoother","ProcessNoiseModel","EstimatedParameter"}){const auto c=GmatType::GetTypeId(n);if(c==Gmat::UNKNOWN_OBJECT)continue;auto values=moderator->GetListOfViewableItems(c);categories.append(QJsonObject{{"category",n},{"category_id",int(c)},{"types",names(values)}});for(const auto &v:values)types.append(QString::fromStdString(v));}
   types.append({"Variable","String","Array"});types.removeAll("Star");types.removeDuplicates();types.sort();report["resource_types"]=QJsonArray::fromStringList(types);report["native_categories"]=categories;
   QJsonObject nested;for(const auto &[name,kind]:QList<QPair<QString,UnsignedInt>>{{"propagators",Gmat::PROPAGATOR},{"axes",Gmat::AXIS_SYSTEM},{"attitudes",Gmat::ATTITUDE},{"physical_models",Gmat::PHYSICAL_MODEL},{"atmospheres",Gmat::ATMOSPHERE},{"commands",Gmat::COMMAND}})nested[name]=names(moderator->GetListOfFactoryItems(kind));report["nested_factory_types"]=nested;
  }else if(requested=="--nested-inputs"){
   NestedInputCatalog::populate(report,*moderator);
  }else{
   auto draft=resourceDraft(requested,"InputDocumentationProbe");require(bool(draft),"Input documentation draft unavailable");report["native_type"]=QString::fromStdString(draft->GetTypeName());report["native_parameters"]=metadata(*draft);report["default_GUI_fields"]=fields(*draft);
   ResourceEditor editor(*draft,[](const QMap<QString,QString>&){return QString();});report["default_GUI_controls"]=controls(editor);
   QJsonArray variants;
   for(const auto &field:resourceProperties(*draft)){
    if(field.choices.size()<2||!field.references.isEmpty()||field.list||field.filename||field.color)continue;
    for(const auto &choice:field.choices){
     if(choice==field.value)continue;auto variant=resourceDraft(requested,"InputDocumentationVariant");QJsonObject v{{"field",field.name},{"choice",choice}};
     try{setResourceProperty(*variant,field.name,choice);v["native_parameters"]=metadata(*variant);v["GUI_fields"]=fields(*variant);}
     catch(BaseException &e){v["error"]=QString::fromStdString(e.GetFullMessage());}
     catch(std::exception &e){v["error"]=e.what();}variants.append(v);
    }
   }
   report["single_selector_variants"]=variants;
  }
  const auto bytes=QJsonDocument(report).toJson();QFile file(output);require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Input documentation census write failed");std::cout<<"INPUT_CATALOG "<<requested.toStdString()<<" sha256="<<QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex().constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<e.what()<<std::endl;}return 1;
}
