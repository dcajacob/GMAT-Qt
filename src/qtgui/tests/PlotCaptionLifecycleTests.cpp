#include "PlotPresentation.hpp"
#include "MainWindow.hpp"
#include "TestSettings.hpp"
#include "BaseException.hpp"
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <iostream>
#include <stdexcept>
static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char **argv){
 QApplication app(argc,argv);app.setOrganizationName("GMATTests");app.setApplicationName("PlotCaptionLifecycle");
 try{
  require(argc==2,"Supply startup");TestSettings settings;PlotModel model(PlotModel::Kind::Orbit);model.simulationEpoch=21545+32.0343817/86400.;
  require(!Moderator::Instance()->IsInitialized(),"Lifecycle probe must begin without an engine");require(plotUtcCaption(model,1)=="UTC epoch unavailable","Uninitialized engine must not dereference a time reader or invent UTC");
  const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());MainWindow window;require(window.initialize(startup),"Caption lifecycle runtime unavailable");
  require(plotUtcCaption(model,1)=="UTC: 01 Jan 2000 12:00:00.000","Initialized caption differs from independent J2000 UTC literal");model.kind=PlotModel::Kind::GroundTrack;require(plotUtcCaption(model,1)=="UTC: 01 Jan 2000 12:00:00.000","Ground caption lost initialized UTC");model.kind=PlotModel::Kind::XY;require(plotUtcCaption(model,1).isEmpty(),"XY plot acquired an orbital UTC caption");
  std::cout<<"PASS uninitialized UTC fallback, initialized Orbit/Ground J2000 literal and XY exclusion; no time-conversion algorithm changed\n";return 0;
 }catch(BaseException&e){std::cerr<<e.GetFullMessage()<<std::endl;}catch(std::exception&e){std::cerr<<e.what()<<std::endl;}return 1;
}
