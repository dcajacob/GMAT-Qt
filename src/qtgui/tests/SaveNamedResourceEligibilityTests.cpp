#include "Moderator.hpp"
#include "GmatBase.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <iostream>
#include <sstream>
#include <stdexcept>
static void require(bool ok,const char *reason){if(!ok)throw std::runtime_error(reason);}
int main(int argc,char **argv){
 QCoreApplication app(argc,argv);
 try {
  require(argc==4,"Expected startup, fixture and named resource case");
  QFile input(QString::fromLocal8Bit(argv[2]));require(input.open(QIODevice::ReadOnly),"Named resource fixture missing");
  const auto spec=QJsonDocument::fromJson(input.readAll()).object();require(spec["case"]==QString(argv[3]),"Wrong named resource case");
  const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
  auto *m=Moderator::Instance();require(m->Initialize(startup.toStdString(),false),"Native resource interpreter unavailable");
  QJsonArray observations;
  for(const auto &v:spec["types"].toArray()){
   const auto type=v.toString();require(QRegularExpression("^[A-Za-z][A-Za-z0-9]+$").match(type).hasMatch(),"Invalid native type fixture");
   const auto script="Create "+type+" ResourceProbe;\nBeginMissionSequence;\n";
   std::istringstream source(script.toStdString());bool built=false;QString diagnostic;
   try {built=m->InterpretScript(&source,true);}catch(BaseException &error){diagnostic=QString::fromStdString(error.GetFullMessage());}
   auto *object=m->GetConfiguredObject("ResourceProbe");
   observations.append(QJsonObject{{"requested",type},{"build_accepted",built},{"configured",object!=nullptr},{"actual_type",object?QString::fromStdString(object->GetTypeName()):QString()},{"exception",diagnostic}});
  }
  require(observations.size()==spec["types"].toArray().size(),"Missing named resource observation");
  const QJsonObject proof{{"checkpoint","save-named-resource-eligibility"},{"outcome","passed"},{"actual_native_interpreter_results",observations},{"scope","A Build rejection may require a configured dependency. Creation alone earns no persistence credit."},{"new_requirement_credit",0}};
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");QFile output(QDir(evidence).filePath("proof.json"));const auto data=QJsonDocument(proof).toJson();require(output.open(QIODevice::WriteOnly)&&output.write(data)==data.size(),"Named resource proof write failed");
  std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &error){std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl;}catch(std::exception &error){std::cerr<<"FAIL "<<error.what()<<std::endl;}
 return 1;
}
