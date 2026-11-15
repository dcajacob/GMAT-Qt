#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "GmatInterface.hpp"
#include "Moderator.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include "GmatBase.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSet>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QTextDocument>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace formaccept {
static void require(bool x,const char *m){if(!x)throw std::runtime_error(m);}
static QByteArray read(const QString &p){QFile f(p);require(f.open(QIODevice::ReadOnly),"Fmincon scope input missing");return f.readAll();}
static void write(const QString &p,const QByteArray &b){QFile f(p);require(f.open(QIODevice::WriteOnly)&&f.write(b)==b.size(),"Fmincon scope evidence write failed");}
}
#include "MatlabAcceptanceStartup.hpp"
using namespace formaccept;
int main(int argc,char **argv){
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("FunctionFminconScope");
 try {
  require(argc==4&&acceptanceWidgetPlatform(),"Expected startup, fixture and Fmincon scope case");
  const auto spec=QJsonDocument::fromJson(read(argv[2])).object();require(spec["case"]==QString(argv[3]),"Wrong Fmincon scope case");
  TestSettings settings;QTemporaryDir files;require(files.isValid(),"Fmincon scope fixture unavailable");
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &n,const QByteArray &v){write(QDir(evidence).filePath(n),v);};
  const QDir bin(QFileInfo(argv[1]).absolutePath());const auto validator=QFileInfo(bin.filePath("GmatQt")).canonicalFilePath();require(QFileInfo(validator).isExecutable(),"Actual function validator missing");FileManager::Instance()->SetBinDirectory("GmatQt",validator.toStdString());
  const auto plugin=bin.absoluteFilePath("../plugins/libMatlabInterface.so");const auto startup=isolatedStartup(QFileInfo(argv[1]).absoluteFilePath(),files.path(),plugin);
  QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;window.show();require(window.initialize(startup),"Licensed fmincon runtime unavailable");QApplication::processEvents();
  require(window.availableEngineTypes().contains("FminconOptimizer"),"Native fmincon factory missing");
  auto *source=window.findChild<QPlainTextEdit*>("scriptEditor"),*messages=window.findChild<QPlainTextEdit*>("messageWindow");require(source&&messages,"Fmincon scope workspace missing");
  const auto functions=spec["functions"].toObject();for(auto i=functions.begin();i!=functions.end();++i){const auto data=i.value().toString().toUtf8();write(files.filePath(i.key()),data);save(i.key(),data);}
  const auto script=spec["caller"].toString().replace("@FILES@",files.path());source->setPlainText(script);save("caller.script",script.toUtf8());const bool built=window.buildScript();QApplication::processEvents();QApplication::sendPostedEvents();QApplication::processEvents();save("build.messages.txt",messages->toPlainText().toUtf8());if(!built){const auto log=files.filePath("output/GmatLog.txt");if(QFileInfo(log).isFile())save("native-build.log",read(log));std::cerr<<window.statusBar()->currentMessage().toStdString()<<"\n"<<messages->toPlainText().toStdString();}require(built,"Fmincon scope Build failed");
  const auto filename=files.filePath("Fmincon scoped resources Ω.script");require(window.saveScriptTo(filename),"Fmincon scope Unicode Save failed");
  const auto result=window.runMission();QApplication::processEvents();QApplication::sendPostedEvents();QApplication::processEvents();save("run.messages.txt",messages->toPlainText().toUtf8());if(result!=MainWindow::RunResult::Completed)std::cerr<<messages->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed,"Actual fmincon in GMAT functions failed");
  auto *m=Moderator::Instance();auto val=[&](const char *n){auto *x=m->GetInternalObject(n);require(x&&x->GetTypeName()=="Variable","Fmincon scoped numeric result missing");const auto v=x->GetRealParameter("Value");require(std::isfinite(v),"Fmincon scoped numeric result nonfinite");return v;};
  QJsonObject native;for(auto *n:{"PrivateValue","PrivateCost","PrivateCalls","SharedValue","SharedCost","SharedCalls","Keep"})native[n]=val(n);
  const auto field=QRegularExpression(R"(SharedOptimizer\.MaximumIterations\s*=\s*([0-9]+))").match(messages->toPlainText());require(field.hasMatch(),"Scoped Write did not expose the actual shared optimizer property");native["ObservedLimit"]=field.captured(1).toInt();save("native-values.json",QJsonDocument(native).toJson());
  const auto expected=spec["expected"].toObject(),bounds=spec["absolute_bounds"].toObject();for(auto i=expected.begin();i!=expected.end();++i)require(std::abs(native[i.key()].toDouble()-i.value().toDouble())<=bounds[i.key()].toDouble(),"Fmincon scoped solution differs from independent bounded quadratic optimum");
  require(val("PrivateCalls")>=3&&val("SharedCalls")>=3,"Real optimizer evaluations missing in a function frame");
  auto *shared=m->GetInternalObject("SharedOptimizer");require(shared&&shared->GetTypeName()=="FminconOptimizer"&&shared->GetIntegerParameter("MaximumIterations")==61,"Shared resource identity or configuration differs");
  require(!m->GetConfiguredObject("LocalOptimizer"),"Private optimizer escaped into configured resources");
  bool absent=false;
  try {absent=m->GetInternalObject("LocalOptimizer")==nullptr;}
  catch(BaseException &error){const auto message=error.GetFullMessage();if(message.find("Could not find \"LocalOptimizer\" in the Sandbox.")==std::string::npos)throw;absent=true;save("private-lookup.txt",QByteArray::fromStdString(message));}
  require(absent,"Private optimizer escaped its function frame");
  require(!GmatInterface::Instance()->ExecuteCallback(),"Function return retained an executable optimizer callback");
  require(source->toPlainText()==script&&read(filename)==script.toUtf8(),"Scope execution changed caller or saved source");
  const QJsonObject proof{{"checkpoint","real-fmincon-private-and-global-function-scope"},{"outcome","passed"},{"native",native},{"private_resource_did_not_escape",true},{"shared_resource_retained",true},{"callback_retired",true},{"new_requirement_credit",0}};
  save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}
 return 1;
}
