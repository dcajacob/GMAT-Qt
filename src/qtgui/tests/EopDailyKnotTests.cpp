#include "EopFile.hpp"
#include "LeapSecsFileReader.hpp"
#include "TimeSystemConverter.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char **argv){
 QCoreApplication app(argc,argv);
 try{
  require(argc==2,"Supply leap-second file");LeapSecsFileReader leap(QFileInfo(argv[1]).absoluteFilePath().toStdString());require(leap.Initialize(),"Leap reader failed");TimeSystemConverter::Instance()->SetLeapSecsFileReader(&leap);
  QTemporaryDir directory;require(directory.isValid(),"Temporary EOP directory absent");const auto name=directory.filePath("daily.eop");QFile file(name);require(file.open(QIODevice::WriteOnly),"Cannot write EOP fixture");const QByteArray bytes="(0h UTC)\n2000 1 1 51544 0.1 0.2 0.3 0.001\n2000 1 2 51545 0.3 0.4 0.29 0.006\n2000 1 3 51546 -0.1 -0.2 0.28 0.010\n";require(file.write(bytes)==bytes.size(),"Incomplete EOP fixture");file.close();
  const std::vector<std::vector<double>> orders{{51544.5,51545,51545.5,51546,51544,51545},{51546,51545,51544,51545,51546},{51543,51544,51544.5,51545,51545.5,51546,51547}};int checks=0,failures=0;
  for(size_t order=0;order<orders.size();++order){EopFile eop(name.toStdString());for(double epoch:orders[order]){double x=0,y=0,lod=0;require(eop.GetPolarMotionAndLod(GmatTime(epoch),x,y,lod),"EOP interpolation rejected");const int day=epoch<51545?0:epoch<51546?1:2;const double fraction=epoch<=51544||epoch>=51546?0:epoch-std::floor(epoch);const std::array<double,3> xs{.1,.3,-.1},ys{.2,.4,-.2},ls{.001,.006,.010};const double ex=xs[day]+(day<2?fraction*(xs[day+1]-xs[day]):0),ey=ys[day]+(day<2?fraction*(ys[day+1]-ys[day]):0);const bool passed=std::abs(x-ex)<1e-14&&std::abs(y-ey)<1e-14&&lod==ls[day];std::cout<<"EOP_CHECK order="<<order<<" utc="<<epoch<<" lod="<<lod<<" expected="<<ls[day]<<" passed="<<passed<<'\n';++checks;if(!passed)++failures;}}
  std::cout<<"EOP_RESULT checks="<<checks<<" failures="<<failures<<std::endl;require(failures==0,"Daily EOP knot depends on previous query");return 0;
 }catch(BaseException&e){std::cerr<<e.GetFullMessage()<<std::endl;}catch(std::exception&e){std::cerr<<e.what()<<std::endl;}return 1;
}
