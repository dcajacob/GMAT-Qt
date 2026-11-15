#include "AcceptancePlatform.hpp"
#include "MainWindow.hpp"
#include "Moderator.hpp"
#include "SNCProcessNoise.hpp"
#include "BaseException.hpp"
#include "TestSettings.hpp"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStatusBar>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
static int assertions=0;
static void require(bool ok,const char *m){++assertions;if(!ok)throw std::runtime_error(m);}
static void same(SNCProcessNoise &actual,const SNCProcessNoise &expected){
 require(actual.GetRealParameter("ElapsedTime")==expected.GetRealParameter("ElapsedTime"),"Copy lost elapsed-time state");
 require(actual.GetRealParameter("CurrentEpoch")==expected.GetRealParameter("CurrentEpoch"),"Copy lost ordinary epoch");
 require(actual.GetGmatTimeParameter("CurrentEpochGT")==expected.GetGmatTimeParameter("CurrentEpochGT"),"Copy lost split epoch");
 const auto &a=actual.GetRmatrixParameter("CurrentNoiseMatrix"),&b=expected.GetRmatrixParameter("CurrentNoiseMatrix");require(a.GetNumRows()==6&&a.GetNumColumns()==6&&a.GetNumRows()==b.GetNumRows()&&a.GetNumColumns()==b.GetNumColumns(),"Copy lost covariance shape");for(int i=0;i<6;++i)for(int j=0;j<6;++j)require(a(i,j)==b(i,j),"Copy lost computed covariance");
}
int main(int argc,char **argv){
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("SncState");
 try{
  require(argc==2&&acceptanceWidgetPlatform(),"Supply private startup");
  SNCProcessNoise typed("Typed");GmatTime precise(21545.25);precise.AddSeconds(1e-7);typed.SetGmatTimeParameter("CurrentEpochGT",precise);
  require(typed.GetGmatTimeParameter("CurrentEpochGT")==precise,"Label precise-time getter lost100ns epoch");
  require(typed.GetGmatTimeParameter(typed.GetParameterID("CurrentEpochGT"))==precise,"ID precise-time getter lost100ns epoch");
  SNCProcessNoise fresh("Fresh");require(fresh.GetRealParameter("ElapsedTime")==0,"Default elapsed time is not zero");require(fresh.GetRealParameter("CurrentEpoch")==21545&&fresh.GetGmatTimeParameter("CurrentEpochGT")==GmatTime(21545.),"Default ordinary/split epochs disagree");
  QDir::setCurrent(QFileInfo(QString::fromLocal8Bit(argv[1])).absolutePath());TestSettings settings;MainWindow window;require(window.initialize(QString::fromLocal8Bit(argv[1])),"SNC runtime initialization failed");window.show();QApplication::processEvents();auto *source=window.findChild<QPlainTextEdit *>("scriptEditor");require(source,"Native SNC source missing");source->setPlainText("Create ProcessNoiseModel Noise;\nNoise.AccelNoiseSigma = [0.001 0.002 0.003];\nCreate Variable ProbeAlive;\nProbeAlive = 41;\nBeginMissionSequence;\nProbeAlive = ProbeAlive + 1;\n");if(!window.buildScript()||window.runMission()!=MainWindow::RunResult::Completed){QApplication::processEvents();std::cerr<<window.statusBar()->currentMessage().toStdString()<<"\n"<<window.findChild<QPlainTextEdit *>("messageWindow")->toPlainText().toStdString();throw std::runtime_error("Native SNC owner initialization failed");}
  auto *owner=Moderator::Instance()->GetInternalObject("Noise");require(owner&&owner->GetOwnedObjectCount()>0,"Runtime noise owner missing");auto *noise=dynamic_cast<SNCProcessNoise *>(owner->GetOwnedObject(owner->GetOwnedObjectCount()-1));require(noise,"Actual native SNC child missing");noise->SetSolarSystem(Moderator::Instance()->GetSolarSystemInUse());auto *frame=Moderator::Instance()->GetInternalObject("EarthMJ2000Eq");require(frame&&noise->SetRefObject(frame,Gmat::COORDINATE_SYSTEM,"EarthMJ2000Eq"),"Native identity-frame binding failed");noise->SetRealParameter("CurrentEpoch",21545.125);noise->SetGmatTimeParameter("CurrentEpochGT",precise);noise->SetRealParameter("ElapsedTime",2.5);
  const auto &q=noise->GetRmatrixParameter("CurrentNoiseMatrix");require(q.GetNumRows()==6&&q.GetNumColumns()==6,"Computed noise shape changed");for(int i=0;i<6;++i)for(int j=0;j<6;++j){double expected=0;for(int axis=0;axis<3;++axis){const double variance=(axis+1)*(axis+1)*1e-6;if(i==axis&&j==axis)expected=variance*2.5*2.5*2.5/3;if((i==axis&&j==axis+3)||(i==axis+3&&j==axis))expected=variance*2.5*2.5/2;if(i==axis+3&&j==axis+3)expected=variance*2.5;}require(std::isfinite(q(i,j))&&std::abs(q(i,j)-expected)<=1e-16,"Independent white-acceleration covariance changed");}
  SNCProcessNoise constructed(*noise);same(constructed,*noise);SNCProcessNoise assigned("Assigned");assigned=*noise;same(assigned,*noise);std::unique_ptr<GmatBase> clone(noise->Clone());auto *cloned=dynamic_cast<SNCProcessNoise *>(clone.get());require(cloned,"Native Clone sliced noise");same(*cloned,*noise);SNCProcessNoise copied("Copied");GmatBase *base=&copied;base->Copy(noise);same(copied,*noise);
  assigned=fresh;require(!assigned.GetRmatrixParameter("CurrentNoiseMatrix").IsSized()&&assigned.GetRealParameter("ElapsedTime")==0&&assigned.GetRealParameter("CurrentEpoch")==21545,"Copy from fresh model retained old covariance/timing cache");
  source->document()->setModified(false);std::cout<<"SNC_STATE_SUMMARY assertions="<<assertions<<" precise_offset_seconds=1e-7 elapsed_seconds=2.5 matrix_components=36 copy_routes=4\n";return 0;
 }catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
