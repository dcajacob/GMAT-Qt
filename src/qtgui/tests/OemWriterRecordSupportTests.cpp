#include "CCSDSOEMWriter.hpp"
#include "FileManager.hpp"
#include "LeapSecsFileReader.hpp"
#include "TimeSystemConverter.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <iostream>
#include <memory>
#include <stdexcept>

static void require(bool ok, const char *message)
{
   if (!ok) throw std::runtime_error(message);
}

static void metadata(CCSDSOEMWriter &writer, const std::string &start,
                     const std::string &stop, int degree)
{
   for (const auto &field : {
      std::make_pair("OBJECT_NAME", std::string("Sat")),
      {"OBJECT_ID", "GMAT-Qt-OEM-record-support"}, {"CENTER_NAME", "EARTH"},
      {"REF_FRAME", "EME2000"}, {"TIME_SYSTEM", "UTC"},
      {"START_TIME", start}, {"STOP_TIME", stop},
      {"USEABLE_START_TIME", start}, {"USEABLE_STOP_TIME", stop},
      {"INTERPOLATION", "LAGRANGE"}, {"INTERPOLATION_DEGREE", std::to_string(degree)}})
      require(writer.SetMetaDataForWriting(field.first, field.second), "OEM metadata rejected");
   require(writer.WriteMetaData() && writer.WriteDataSegment(), "OEM segment write failed");
}

int main(int argc, char **argv)
{
   QCoreApplication app(argc, argv);
   try
   {
      require(argc == 2, "Supply startup file");
      auto *manager = FileManager::Instance();
      manager->ReadStartupFile(argv[1]);
      LeapSecsFileReader leap(manager->GetFullPathname("LEAP_SECS_FILE"));
      require(leap.Initialize(), "OEM leap table failed");
      auto *time = TimeSystemConverter::Instance();
      time->SetLeapSecsFileReader(&leap);
      const Real origin = time->Convert(21545., TimeSystemConverter::UTC, TimeSystemConverter::A1);
      const auto directory = qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      require(!directory.isEmpty(), "Supply acceptance output directory");
      // The campaign observer acknowledges live /proc maps before this short
      // support executable exits. Ordinary CTest runs do not use the handshake.
      const auto ready = qEnvironmentVariable("GMAT_OEM_MAP_READY");
      if (!ready.isEmpty())
      {
         QFile marker(ready);
         require(marker.open(QIODevice::WriteOnly) &&
                 marker.write(QByteArray::number(QCoreApplication::applicationPid())) > 0,
                 "OEM mapping handshake failed");
         marker.close();
         QElapsedTimer timer;
         timer.start();
         const auto acknowledged = qEnvironmentVariable("GMAT_OEM_MAP_ACK");
         while (!QFile::exists(acknowledged) && timer.elapsed() < 5000) QThread::msleep(5);
         require(QFile::exists(acknowledged), "OEM live maps were not acknowledged");
      }
      auto add = [&](CCSDSOEMWriter &writer, double seconds, int marker) {
         Rvector state(6, 1000. + marker, 2. * marker, 3. * marker, 4., 5., 6.);
         require(writer.AddDataForWriting(origin + seconds / 86400., state), "OEM data rejected");
      };
      auto header = [&](CCSDSOEMWriter &writer, const QString &name) {
         require(writer.OpenFile(QDir(directory).filePath(name).toStdString()), "OEM open failed");
         require(writer.SetHeaderForWriting("ORIGINATOR", "GMAT record-support test") &&
                 writer.WriteHeader("CCSDS_OEM_VERS"), "OEM header failed");
      };
      QJsonObject proof;
      {
         CCSDSOEMWriter writer("1.0");
         header(writer, "duplicate-epochs.oem");
         proof["initial_count"] = writer.GetNumberOfSerializedDataPoints();
         int marker = 0;
         for (double seconds : {0., .0001, .0002, .0011, .0012, 1.})
            add(writer, seconds, ++marker);
         proof["duplicate_count"] = writer.GetNumberOfSerializedDataPoints();
         CCSDSOEMWriter copied(writer), assigned("1.0");
         // Existing writer copies copy configuration, not the pending segment.
         assigned = writer;
         std::unique_ptr<CCSDSOEMWriter> cloned(writer.Clone());
         proof["copy_count"] = copied.GetNumberOfSerializedDataPoints();
         proof["assignment_count"] = assigned.GetNumberOfSerializedDataPoints();
         proof["clone_count"] = cloned->GetNumberOfSerializedDataPoints();
         metadata(writer, "2000-01-01T12:00:00.000", "2000-01-01T12:00:01.000", 2);
         proof["after_duplicate_write_count"] = writer.GetNumberOfSerializedDataPoints();
         add(writer, 2., 7);
         writer.ClearDataStore();
         proof["after_explicit_clear_count"] = writer.GetNumberOfSerializedDataPoints();
      }
      {
         CCSDSOEMWriter writer("1.0");
         header(writer, "accumulated-and-reset.oem");
         for (int i = 0; i < 1000; ++i) add(writer, i * .5, i + 1);
         proof["first_chunk_count"] = writer.GetNumberOfSerializedDataPoints();
         for (int i = 1000; i < 1003; ++i) add(writer, i * .5, i + 1);
         proof["complete_segment_count"] = writer.GetNumberOfSerializedDataPoints();
         metadata(writer, "2000-01-01T12:00:00.000", "2000-01-01T12:08:21.000", 7);
         proof["after_long_write_count"] = writer.GetNumberOfSerializedDataPoints();
         add(writer, 600., 2001);
         proof["next_segment_count"] = writer.GetNumberOfSerializedDataPoints();
         metadata(writer, "2000-01-01T12:10:00.000", "2000-01-01T12:10:00.000", 0);
         proof["after_single_write_count"] = writer.GetNumberOfSerializedDataPoints();
      }
      QFile output(QDir(directory).filePath("writer-proof.json"));
      const auto bytes = QJsonDocument(proof).toJson(QJsonDocument::Indented);
      require(output.open(QIODevice::WriteOnly) && output.write(bytes) == bytes.size(), "OEM proof write failed");
      return 0;
   }
   catch (BaseException &error) { std::cerr << error.GetFullMessage(); }
   catch (const std::exception &error) { std::cerr << error.what(); }
   return 1;
}
