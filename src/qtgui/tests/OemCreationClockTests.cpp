#include "CCSDSOEMWriter.hpp"
#include "FileManager.hpp"
#include "LeapSecsFileReader.hpp"
#include "TimeSystemConverter.hpp"
#include "TimeTypes.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <iostream>
#include <stdexcept>

static void require(bool ok, const char *message)
{
   if (!ok) throw std::runtime_error(message);
}

int main(int argc, char **argv)
{
   QCoreApplication app(argc, argv);
   try
   {
      require(argc == 2, "Supply startup file");
      const auto directory = qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      require(!directory.isEmpty(), "Supply output directory");
      auto *manager = FileManager::Instance();
      manager->ReadStartupFile(argv[1]);
      LeapSecsFileReader leap(manager->GetFullPathname("LEAP_SECS_FILE"));
      require(leap.Initialize(), "Creation-clock leap table failed");
      auto *converter = TimeSystemConverter::Instance();
      converter->SetLeapSecsFileReader(&leap);
      const auto ready = qEnvironmentVariable("GMAT_OEM_MAP_READY");
      if (!ready.isEmpty())
      {
         QFile marker(ready);
         require(marker.open(QIODevice::WriteOnly) &&
                 marker.write(QByteArray::number(QCoreApplication::applicationPid())) > 0,
                 "Creation-clock mapping handshake failed");
         marker.close();
         const auto acknowledged = qEnvironmentVariable("GMAT_OEM_MAP_ACK");
         QElapsedTimer timer;
         timer.start();
         while (!QFile::exists(acknowledged) && timer.elapsed() < 5000) QThread::msleep(5);
         require(QFile::exists(acknowledged), "Creation-clock live maps were not acknowledged");
      }
      CCSDSOEMWriter closed("1.0");
      require(!closed.WriteHeader("CCSDS_OEM_VERS"), "Closed writer header behavior changed");
      const Real origin = converter->Convert(21545., TimeSystemConverter::UTC, TimeSystemConverter::A1);
      for (const auto &version : {std::string("1.0"), std::string("2.0")})
      {
         CCSDSOEMWriter writer(version);
         const auto filename = QDir(directory).filePath(QString::fromStdString("clock-"+version+".oem"));
         require(writer.OpenFile(filename.toStdString()), "Creation-clock output open failed");
         require(writer.SetHeaderForWriting("ORIGINATOR", "GMAT UTC creation-clock test") &&
                 writer.WriteHeader("CCSDS_OEM_VERS"), "Creation-clock header failed");
         for (const auto &field : {
            std::make_pair("OBJECT_NAME", std::string("ClockSat")),
            {"OBJECT_ID", "GMAT-UTC-header-test"}, {"CENTER_NAME", "EARTH"},
            {"REF_FRAME", "EME2000"}, {"TIME_SYSTEM", "UTC"},
            {"START_TIME", "2000-01-01T12:00:00.000"},
            {"USEABLE_START_TIME", "2000-01-01T12:00:00.000"},
            {"USEABLE_STOP_TIME", "2000-01-01T12:00:01.000"},
            {"STOP_TIME", "2000-01-01T12:00:01.000"},
            {"INTERPOLATION", "LAGRANGE"}, {"INTERPOLATION_DEGREE", "1"}})
            require(writer.SetMetaDataForWriting(field.first, field.second), "Creation-clock metadata failed");
         Rvector state(6, 7000., 0., 0., 0., 7.5, 0.);
         require(writer.AddDataForWriting(origin, state), "Creation-clock first state failed");
         state[1] = 7.5;
         require(writer.AddDataForWriting(origin+1./86400., state), "Creation-clock second state failed");
         require(writer.WriteMetaData() && writer.WriteDataSegment(), "Creation-clock segment failed");
      }
      const QJsonObject proof{{"generic_local_time", QString::fromStdString(GmatTimeUtil::FormatCurrentTime(2))},
                              {"observed_timezone", qEnvironmentVariable("TZ")},
                              {"closed_writer_rejected", true}, {"native_files", 2},
                              {"scope", "UTC header creation,format versions1/2,and unchanged generic local clock;no orbit-science credit"}};
      QFile output(QDir(directory).filePath("clock-proof.json"));
      const auto bytes = QJsonDocument(proof).toJson(QJsonDocument::Indented);
      require(output.open(QIODevice::WriteOnly) && output.write(bytes) == bytes.size(), "Creation-clock proof failed");
      return 0;
   }
   catch (BaseException &error) { std::cerr << error.GetFullMessage(); }
   catch (const std::exception &error) { std::cerr << error.what(); }
   return 1;
}
