#include "Moderator.hpp"
#include "FileManager.hpp"
#include "CustomFOV.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool ok,const char *error){if(!ok)throw std::runtime_error(error);}
void write(const QString &path,const QByteArray &bytes){QFile file(path);require(file.open(QIODevice::WriteOnly)&&file.write(bytes)==bytes.size(),"Native FOV evidence write failed");}
}
int main(int argc,char **argv)
{
   QCoreApplication app(argc,argv);
   const QString directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
   QJsonObject proof{{"passed",false}};
   try {
      require(argc==3&&!directory.isEmpty(),"Supply startup, native FOV case and evidence directory");
      const QString name=argv[2];proof["case"]=name;
      const auto startup=QFileInfo(argv[1]).absoluteFilePath();QDir::setCurrent(QFileInfo(startup).absolutePath());
      require(Moderator::Instance()->Initialize(startup.toStdString(),false),"Native FOV startup failed");
      // Reuse the native process-map handshake from RunNestedSolverValidation.
      write(qEnvironmentVariable("GMAT_SOLVER_MAP_READY"),QByteArray::number(QCoreApplication::applicationPid()));
      QElapsedTimer timer;timer.start();while(!QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK"))&&timer.elapsed()<10000)QThread::msleep(5);
      require(QFile::exists(qEnvironmentVariable("GMAT_SOLVER_MAP_ACK")),"Native FOV maps not acknowledged");
      auto *files=FileManager::Instance();const auto masks=QDir(directory).filePath("masks");require(QDir().mkpath(masks),"Mask directory unavailable");
      write(QDir(masks).filePath("native-mask.txt"),"ConeClockAngles\n20 0\n20 90\n20 180\n20 270\n");
      const bool defaults=name=="Default",missing=name=="Missing";
      require(defaults||missing||name=="Absolute"||name=="Alias","Unknown native FOV case");
      if(!defaults){files->AddFileType("ACCEPTANCE_FOV_PATH",masks.toStdString());files->AddFileType("FOV_DATA_PATH",name=="Absolute"?masks.toStdString():"ACCEPTANCE_FOV_PATH");}
      std::string expected;
      if(defaults){bool absent=false;try{files->GetPathDefinition("FOV_DATA_PATH");}catch(BaseException &){absent=true;}require(absent,"Default fixture must begin without a FOV path");expected="../data/hardware/";}
      else expected=files->GetPathDefinition("FOV_DATA_PATH");
      proof["expected_definition"]=QString::fromStdString(expected);QJsonArray observations;
      for(int instance=0;instance<2;++instance){
         CustomFOV mask("NativeMask"+std::to_string(instance));
         const auto actual=files->GetPathDefinition("FOV_DATA_PATH");proof["observed_definition"]=QString::fromStdString(actual);
         require(actual==expected,"CustomFOV construction replaced the configured path definition");
         if(defaults)continue;
         mask.SetStringParameter("FOVFileName",missing?"missing-mask.txt":"native-mask.txt");bool initialized=false;QString error;
         try{initialized=mask.Initialize();}catch(BaseException &e){error=QString::fromStdString(e.GetFullMessage());}
         if(missing){require(!initialized&&!error.isEmpty(),"Missing native mask was accepted");observations.append(QJsonObject{{"rejected",true},{"error",error}});}
         else{
            require(initialized,"Configured relative native mask failed to initialize");
            const auto cones=mask.GetRvectorParameter("ConeAngles"),clocks=mask.GetRvectorParameter("ClockAngles");
            require(cones.GetSize()==4&&clocks.GetSize()==4,"Native mask vertex count differs");QJsonArray vertices;
            for(int i=0;i<4;++i){require(std::abs(cones[i]-20)<=1e-10&&std::abs(clocks[i]-90*i)<=1e-10,"Native mask angles differ from literal file truth");vertices.append(QJsonArray{cones[i],clocks[i]});}
            observations.append(vertices);
         }
         require(files->GetPathDefinition("FOV_DATA_PATH")==expected,"FOV initialization replaced the path definition");
      }
      proof["observations"]=observations;proof["passed"]=true;
   }catch(BaseException &e){proof["error"]=QString::fromStdString(e.GetFullMessage());}
   catch(std::exception &e){proof["error"]=QString::fromUtf8(e.what());}
   write(QDir(directory).filePath("proof.json"),QJsonDocument(proof).toJson());
   std::cout<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
   return proof["passed"].toBool()?0:1;
}
