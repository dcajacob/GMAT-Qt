#include "CCSDSOEMWriter.hpp"
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
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace
{
void require(bool value, const char *message)
{
   if (!value) throw std::runtime_error(message);
}

class Writer : public CCSDSOEMWriter
{
public:
   explicit Writer(const std::string &version = "1.0") : CCSDSOEMWriter(version) {}
   std::streampos Position() { emOutStream.flush(); return emOutStream.tellp(); }
   void Position(std::streampos position) { emOutStream.seekp(position); }
};

QByteArray read(const QString &path)
{
   QFile file(path);
   require(file.open(QIODevice::ReadOnly), "Cannot read invariant output");
   return file.readAll();
}

void save(const QString &path, const QByteArray &bytes)
{
   QFile file(path);
   require(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size(), "Cannot save invariant output");
}

void header(CCSDSOEMWriter &writer, const QString &path)
{
   require(writer.OpenFile(path.toStdString()), "Cannot open invariant output");
   writer.SetHeaderForWriting("ORIGINATOR", "GMAT FILE INVARIANT TEST");
   require(writer.WriteHeader("CCSDS_OEM_VERS"), "Cannot write invariant header");
}

Real origin;
void metadata(Writer &writer, double start, const std::string &identity)
{
   for (const auto &field : std::vector<std::pair<std::string, std::string>>{
         {"OBJECT_NAME", "InvariantProbe"}, {"OBJECT_ID", identity},
         {"CENTER_NAME", "EARTH"}, {"REF_FRAME", "EME2000"}, {"TIME_SYSTEM", "UTC"},
         {"START_TIME", writer.FormatEpoch(origin + start / 86400.)},
         {"STOP_TIME", writer.FormatEpoch(origin + (start + 1) / 86400.)},
         {"INTERPOLATION", "LAGRANGE"}, {"INTERPOLATION_DEGREE", "1"}})
      require(writer.SetMetaDataForWriting(field.first, field.second), "Cannot set invariant metadata");
}

void buffer(Writer &writer, double start)
{
   for (int i = 0; i < 2; ++i)
   {
      // Thirty components also permit an OEM2 compatibility control. Only
      // the first six belong to B1. This is a wire fixture, not a trajectory.
      Rvector state(30);
      for (int j = 0; j < 30; ++j) state[j] = j + 1;
      state[0] = 7000; state[1] = start + i; state[2] = 0;
      state[3] = 0; state[4] = 1; state[5] = 0;
      require(writer.AddDataForWriting(origin + (start + i) / 86400., state), "Cannot buffer invariant state");
   }
}

void segment(Writer &writer, double start, const std::string &identity)
{
   metadata(writer, start, identity);
   require(writer.WriteMetaData(), "Cannot write invariant metadata");
   buffer(writer, start);
   require(writer.WriteDataSegment(), "Cannot write invariant data");
}

QJsonObject rejection(Writer &writer, const QString &path, const std::function<void()> &operation)
{
   const auto position = writer.Position();
   const auto before = read(path);
   const auto count = writer.GetNumberOfSerializedDataPoints();
   bool rejected = false;
   QString diagnostic;
   try { operation(); }
   catch (BaseException &error) { rejected = true; diagnostic = QString::fromStdString(error.GetFullMessage()); }
   const bool unchanged = writer.Position() == position && read(path) == before &&
         writer.GetNumberOfSerializedDataPoints() == count;
   return {{"expected_rejection", true}, {"rejected", rejected},
      {"unchanged", unchanged}, {"diagnostic", diagnostic}};
}
}

int main(int argc, char **argv)
{
   QCoreApplication app(argc, argv);
   try
   {
      require(argc == 2, "Supply startup file");
      const auto directory = qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      require(!directory.isEmpty(), "Supply invariant output directory");
      auto *files = FileManager::Instance(); files->ReadStartupFile(argv[1]);
      LeapSecsFileReader leap(files->GetFullPathname("LEAP_SECS_FILE"));
      require(leap.Initialize(), "Cannot initialize invariant leap table");
      auto *converter = TimeSystemConverter::Instance(); converter->SetLeapSecsFileReader(&leap);
      origin = converter->Convert(21545., TimeSystemConverter::UTC, TimeSystemConverter::A1);
      const auto ready = qEnvironmentVariable("GMAT_OEM_MAP_READY");
      if (!ready.isEmpty())
      {
         save(ready, QByteArray::number(QCoreApplication::applicationPid()));
         const auto ack = qEnvironmentVariable("GMAT_OEM_MAP_ACK");
         QElapsedTimer timer; timer.start();
         while (!QFile::exists(ack) && timer.elapsed() < 5000) QThread::msleep(5);
         require(QFile::exists(ack), "Invariant live images were not acknowledged");
      }
      const QStringList names{"SameObject", "EquivalentObjectId", "ChangedObject", "ChangedObjectRewrite",
         "ReopenOtherObject", "CloneOtherObject", "B1Acceleration", "B1Covariance", "B1BothExtensions", "OEM2Extensions"};
      QJsonArray results;
      int failures = 0;
      for (const auto &name : names)
      {
         const auto path = QDir(directory).filePath(name + ".oem");
         QJsonObject result{{"case", name}, {"expected_rejection", false}};
         try
         {
            Writer writer(name == "OEM2Extensions" ? "2.0" : "1.0");
            header(writer, path);
            if (name.contains("Extensions") || name == "B1Acceleration" || name == "B1Covariance")
            {
               metadata(writer, 0, "2000-001A");
               require(writer.WriteMetaData(), "Cannot write extension metadata");
               buffer(writer, 0);
               writer.SetUseAcceleration(name != "B1Covariance");
               writer.SetUseCovariance(name != "B1Acceleration");
               if (name == "OEM2Extensions") require(writer.WriteDataSegment(), "Cannot write OEM2 control");
               else
               {
                  result = rejection(writer, path, [&] { writer.WriteDataSegment(); });
                  if (result["rejected"].toBool())
                  {
                     writer.SetUseAcceleration(false); writer.SetUseCovariance(false);
                     require(writer.WriteDataSegment(), "Cannot recover B1 state buffer");
                  }
               }
            }
            else
            {
               const auto firstMetadata = writer.Position();
               const std::string identity = name == "EquivalentObjectId" ? "GMAT  ONE" : "2000-001A";
               segment(writer, 0, identity);
               const auto end = writer.Position();
               if (name.startsWith("ChangedObject"))
               {
                  if (name == "ChangedObjectRewrite") writer.Position(firstMetadata);
                  metadata(writer, name == "ChangedObjectRewrite" ? 0 : 2, "2000-002A");
                  result = rejection(writer, path, [&] { writer.WriteMetaData(); });
                  if (result["rejected"].toBool())
                  {
                     if (name == "ChangedObjectRewrite")
                     {
                        metadata(writer, 0, identity); require(writer.WriteMetaData(), "Cannot restore rewritten metadata");
                        writer.Position(end);
                     }
                     segment(writer, 2, identity);
                  }
                  else if (name == "ChangedObject") { buffer(writer, 2); writer.WriteDataSegment(); }
               }
               else if (name == "ReopenOtherObject")
               {
                  save(path + ".original", read(path)); header(writer, path); segment(writer, 0, "2000-002A");
               }
               else if (name == "CloneOtherObject")
               {
                  Writer copy(writer); header(copy, path + ".clone"); segment(copy, 0, "2000-002A");
               }
               else segment(writer, 2, name == "EquivalentObjectId" ? "gmat_one" : identity);
            }
            const bool passed = !result["expected_rejection"].toBool() ||
                  (result["rejected"].toBool() && result["unchanged"].toBool() && !result["diagnostic"].toString().isEmpty());
            result["passed"] = passed;
         }
         catch (BaseException &error) { result["passed"] = false; result["unexpected_error"] = QString::fromStdString(error.GetFullMessage()); }
         catch (std::exception &error) { result["passed"] = false; result["unexpected_error"] = error.what(); }
         result["case"] = name;
         failures += !result["passed"].toBool(); results.append(result);
      }
      save(QDir(directory).filePath("proof.json"), QJsonDocument(QJsonObject{{"cases", results},
         {"failures", failures}, {"native_missions", 0}, {"scope", "B1 file identity and six-component wire contract; OEM2 compatibility; no numerical trajectory qualification"}}).toJson());
      std::cout << "OEM file invariant cases=" << results.size() << " failures=" << failures << '\n';
      return failures ? 1 : 0;
   }
   catch (BaseException &error) { std::cerr << error.GetFullMessage(); }
   catch (std::exception &error) { std::cerr << error.what(); }
   return 1;
}
