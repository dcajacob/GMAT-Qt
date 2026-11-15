#include "AcceptancePlatform.hpp"
#include "AcceptanceView.hpp"
#include "MainWindow.hpp"
#include "QtMatlabCallbackServer.hpp"
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
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
namespace formaccept {
static void require(bool x,const char *m){if(!x)throw std::runtime_error(m);}
static QByteArray read(const QString &p){QFile f(p);require(f.open(QIODevice::ReadOnly),"Reentry input missing");return f.readAll();}
static void write(const QString &p,const QByteArray &b){QFile f(p);require(f.open(QIODevice::WriteOnly)&&f.write(b)==b.size(),"Reentry evidence write failed");}
}
#include "MatlabAcceptanceStartup.hpp"
using namespace formaccept;
class ReentryProbe final:public GmatBase {
public:
 int calls=0,writes=0;
 ReentryProbe():GmatBase(Gmat::UNKNOWN_OBJECT,"ReentryProbe","Probe"){}
 GmatBase *Clone()const override{return new ReentryProbe(*this);}
 bool RenameRefObject(UnsignedInt,const std::string&,const std::string&)override{return false;}
 bool HasLocalClones()override{return false;}
 bool ExecuteCallback()override{++calls;return true;}
 bool PutCallbackData(std::string&)override{++writes;return true;}
};
int main(int argc,char **argv){
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("MatlabFunctionReentry");
 try {
  require(argc==4&&acceptanceWidgetPlatform(),"Expected startup, fixture and reentry case");
  const auto spec=QJsonDocument::fromJson(read(argv[2])).object();require(spec["case"]==QString(argv[3]),"Wrong reentry case");
  TestSettings settings;QTemporaryDir files;require(files.isValid(),"Reentry fixture unavailable");
  const auto evidence=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");auto save=[&](const QString &n,const QByteArray &v){write(QDir(evidence).filePath(n),v);};
  // Publish before the real engine is opened, so MATLAB inherits the endpoint.
  QtMatlabCallbackServer server;server.start();
  const auto plugin=QFileInfo(argv[1]).dir().absoluteFilePath("../../plugins/libMatlabInterface.so");
  const auto startup=isolatedStartup(QFileInfo(argv[1]).absoluteFilePath(),files.path(),plugin);
  QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;window.show();require(window.initialize(startup),"Licensed MATLAB runtime unavailable");QApplication::processEvents();
  auto *source=window.findChild<QPlainTextEdit*>("scriptEditor"),*messages=window.findChild<QPlainTextEdit*>("messageWindow");require(source&&messages,"Reentry workspace missing");
  const auto function=files.filePath("MatGuard.m");write(function,spec["matlab_function"].toString().toUtf8());save("MatGuard.m",read(function));
  const QString script="Create Variable Input Out Keep;\nInput = 4;\nOut = -1;\nKeep = 41;\nCreate MatlabFunction MatGuard;\nMatGuard.FunctionPath = '"+function+"';\nBeginMissionSequence;\n[Out] = MatGuard(Input);\nKeep = Keep + 1;\n";
  source->setPlainText(script);save("caller.script",script.toUtf8());const bool built=window.buildScript();save("build.messages.txt",messages->toPlainText().toUtf8());require(built,"Reentry mission Build failed");
  ReentryProbe probe;auto *interface=GmatInterface::Instance();require(interface->RegisterCallbackServer(&probe),"Reentry probe registration failed");
  struct Registration{GmatInterface *i;GmatBase *p;~Registration(){i->UnregisterCallbackServer(p);}} registration{interface,&probe};
  const auto result=window.runMission();QApplication::processEvents();save("run.messages.txt",messages->toPlainText().toUtf8());if(result!=MainWindow::RunResult::Completed)std::cerr<<messages->toPlainText().toStdString();require(result==MainWindow::RunResult::Completed,"Real MATLAB reentry/recovery mission failed");
  auto *out=Moderator::Instance()->GetInternalObject("Out"),*keep=Moderator::Instance()->GetInternalObject("Keep");require(out&&keep&&out->GetRealParameter("Value")==14&&keep->GetRealParameter("Value")==42,"Reentry altered independent output or successor");
  require(probe.calls==0&&probe.writes==0,"MATLAB socket request entered callback/callee");
  require(interface->ExecuteCallback()&&probe.calls==1,"Real MATLAB scope did not restore normal callbacks");
  require(source->toPlainText()==script&&read(function)==spec["matlab_function"].toString().toUtf8(),"Reentry changed caller or external function");
  const QJsonObject proof{{"checkpoint","licensed-matlab-function-reentry"},{"outcome","passed"},{"actual_matlab_result",14},{"successor",42},{"callback_dispatches_during_matlab",0},{"callback_writes_during_matlab",0},{"callback_dispatches_after_return",1},{"transport","authenticated-real-loopback-socket-from-MATLAB"},{"native_tests_reused",false},{"new_requirement_credit",0}};
  save("proof.json",QJsonDocument(proof).toJson());std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;source->document()->setModified(false);return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}
 return 1;
}
