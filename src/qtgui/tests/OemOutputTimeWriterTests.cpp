#include "CCSDSOEMWriter.hpp"
#include "EphemerisFile.hpp"
#include "FileManager.hpp"
#include "LeapSecsFileReader.hpp"
#include "TimeSystemConverter.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <iostream>
#include <memory>
#include <stdexcept>

static void require(bool ok,const char *message)
{
   if(!ok)throw std::runtime_error(message);
}

int main(int argc,char **argv)
{
   QCoreApplication app(argc,argv);
   try
   {
      require(argc==3,"Supply startup and output-time support cases");
      QFile input(argv[2]);require(input.open(QIODevice::ReadOnly),"Time support catalog missing");
      const auto catalog=QJsonDocument::fromJson(input.readAll()).object();
      const auto directory=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");require(!directory.isEmpty(),"Supply output directory");
      auto *manager=FileManager::Instance();manager->ReadStartupFile(argv[1]);
      LeapSecsFileReader leap(manager->GetFullPathname("LEAP_SECS_FILE"));require(leap.Initialize(),"Time support leap table failed");
      TimeSystemConverter::Instance()->SetLeapSecsFileReader(&leap);
      const auto ready=qEnvironmentVariable("GMAT_OEM_MAP_READY");
      if(!ready.isEmpty())
      {
         QFile marker(ready);require(marker.open(QIODevice::WriteOnly)&&marker.write(QByteArray::number(QCoreApplication::applicationPid()))>0,"Time support mapping handshake failed");marker.close();
         const auto ack=qEnvironmentVariable("GMAT_OEM_MAP_ACK");QElapsedTimer timer;timer.start();
         while(!QFile::exists(ack)&&timer.elapsed()<5000)QThread::msleep(5);
         require(QFile::exists(ack),"Time support live maps were not acknowledged");
      }
      QJsonArray records;
      for(const auto &entry:catalog["cases"].toArray())
      {
         const auto c=entry.toObject();const auto system=c["system"].toString().toStdString();
         const auto labels=c["expected_labels"].toArray(),offsets=c["offsets_seconds"].toArray();
         const auto groups=c["expected_groups"].toArray();const Real origin=c["input_a1_mjd"].toDouble();
         EphemerisFile model("ClockExport");const auto id=model.GetParameterID("OutputTimeSystem");
         require(model.GetStringParameter(id)=="UTC"&&!model.IsParameterCommandModeSettable(id),"Output-time default or configuration-only contract differs");
         require(model.GetPropertyEnumStrings(id)==StringArray({"A1","TAI","UTC","TT","TDB"}),"Original five-system native inventory differs");
         model.SetStringParameter("EpochFormat","TTGregorian");model.SetStringParameter(id,system);
         require(model.GetStringParameter("EpochFormat")=="TTGregorian","Output clock changed input-boundary clock");
         std::unique_ptr<GmatBase> modelCopy(model.Clone());EphemerisFile modelAssigned("AssignedClock");modelAssigned=model;
         require(modelCopy->GetStringParameter("OutputTimeSystem")==system&&modelAssigned.GetStringParameter(id)==system,"Ephemeris clone/assignment lost output clock");
         bool badModel=false;try{model.SetStringParameter(id,"NasaBadClock");}catch(BaseException &){badModel=true;}
         require(badModel&&model.GetStringParameter(id)==system,"Invalid native clock assignment mutated selection");
         model.SetStringParameter("FileFormat","SPK");require(model.IsParameterReadOnly(id)&&model.GetStringParameter(id)=="UTC","Non-OEM format did not reset/disable output clock");

         CCSDSOEMWriter writer("1.0");writer.SetTimeSystem(system);
         CCSDSOEMWriter copied(writer),assigned("1.0");assigned=writer;std::unique_ptr<CCSDSOEMWriter> cloned(writer.Clone());
         require(copied.FormatEpoch(origin)==labels[0].toString().toStdString()&&assigned.FormatEpoch(origin)==labels[0].toString().toStdString()&&cloned->FormatEpoch(origin)==labels[0].toString().toStdString(),"Writer clone/assignment lost selected epoch formatter");
         bool badWriter=false;try{writer.SetTimeSystem("NasaBadClock");}catch(BaseException &){badWriter=true;}
         require(badWriter&&writer.FormatEpoch(origin)==labels[0].toString().toStdString(),"Invalid writer clock assignment mutated selection");
         require(writer.OpenFile(QDir(directory).filePath(QString::fromStdString(system+".oem")).toStdString()),"Time support output open failed");
         writer.SetHeaderForWriting("ORIGINATOR","GMAT output-time writer support");require(writer.WriteHeader("CCSDS_OEM_VERS"),"Time support header failed");
         const auto other=system=="UTC"?"TAI":"UTC";
         bool midFile=false;try{writer.SetTimeSystem(other);}catch(BaseException &){midFile=true;}
         require(midFile&&writer.FormatEpoch(origin)==labels[0].toString().toStdString(),"Open-file clock change was accepted");
         bool mismatched=false;try{writer.SetMetaDataForWriting("TIME_SYSTEM",other);}catch(BaseException &){mismatched=true;}
         require(mismatched,"Mismatched metadata time system was accepted");
         CCSDSOEMWriter opened("1.0");opened.SetTimeSystem(other);require(opened.OpenFile(QDir(directory).filePath(QString::fromStdString(system+"-assignment-guard.oem")).toStdString()),"Assignment guard file open failed");
         const auto oldOpenedEpoch=opened.FormatEpoch(origin); bool reassigned=false;try{opened=writer;}catch(BaseException &){reassigned=true;}
         require(reassigned&&opened.FormatEpoch(origin)==oldOpenedEpoch,"Open-file assignment changed its clock");
         QJsonArray actualLabels;
         for(int i=0;i<offsets.size();++i)
         {
            const Real epoch=origin+offsets[i].toDouble()/86400.;const auto label=writer.FormatEpoch(epoch);
            require(label==labels[i].toString().toStdString(),"Selected-system sub-millisecond label differs from independent fixture");actualLabels.append(QString::fromStdString(label));
            Rvector state(6,7001.+i,2.,3.,4.,5.,6.);require(writer.AddDataForWriting(epoch,state),"Time support sample rejected");
         }
         const Integer count=writer.GetNumberOfSerializedDataPoints();require(count==c["expected_count"].toInt(),"Record count used another clock's rounding groups");
         for(const auto &field:{std::make_pair("OBJECT_NAME",std::string("ClockSat")),{"OBJECT_ID","GMAT-OEM-clock-support"},{"CENTER_NAME","EARTH"},{"REF_FRAME","EME2000"},{"TIME_SYSTEM",system},{"START_TIME",groups.first().toObject()["epoch"].toString().toStdString()},{"STOP_TIME",groups.last().toObject()["epoch"].toString().toStdString()},{"INTERPOLATION","LAGRANGE"},{"INTERPOLATION_DEGREE",std::to_string(count-1)}})
            writer.SetMetaDataForWriting(field.first,field.second);
         require(writer.WriteMetaData()&&writer.WriteDataSegment()&&writer.GetNumberOfSerializedDataPoints()==0,"Selected-clock segment write/count reset failed");
         records.append(QJsonObject{{"system",QString::fromStdString(system)},{"actual_labels",actualLabels},{"serialized_count",count},{"copy_assignment_preserved",true},{"invalid_assignments_rejected",true},{"mid_file_change_rejected",true},{"metadata_mismatch_rejected",true},{"open_file_assignment_rejected",true},{"native_parameter_clone_preserved",true},{"input_epoch_format_preserved",true},{"inactive_format_reset",true}});
      }
      QFile proof(QDir(directory).filePath("writer-time-proof.json"));const auto bytes=QJsonDocument(QJsonObject{{"cases",records},{"outcome","passed"},{"new_requirement_credit",0}}).toJson(QJsonDocument::Indented);
      require(proof.open(QIODevice::WriteOnly)&&proof.write(bytes)==bytes.size(),"Time support proof failed");
      return 0;
   }
   catch(BaseException &error){std::cerr<<error.GetFullMessage();}
   catch(const std::exception &error){std::cerr<<error.what();}
   return 1;
}
