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
#include <iostream>
#include <stdexcept>
#include <vector>

static void require(bool ok, const char *message)
{
   if (!ok) throw std::runtime_error(message);
}

struct TextCase
{
   std::string name, field, value, commentSection, comment;
   bool reject = false, minimal = false;
};

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
      require(leap.Initialize(), "B1-text leap table failed");
      auto *converter = TimeSystemConverter::Instance();
      converter->SetLeapSecsFileReader(&leap);
      const auto ready = qEnvironmentVariable("GMAT_OEM_MAP_READY");
      if (!ready.isEmpty())
      {
         QFile marker(ready);
         require(marker.open(QIODevice::WriteOnly) &&
                 marker.write(QByteArray::number(QCoreApplication::applicationPid())) > 0,
                 "B1-text mapping handshake failed");
         marker.close();
         const auto ack = qEnvironmentVariable("GMAT_OEM_MAP_ACK");
         QElapsedTimer timer;
         timer.start();
         while (!QFile::exists(ack) && timer.elapsed() < 5000) QThread::msleep(5);
         require(QFile::exists(ack), "B1-text live maps were not acknowledged");
      }
      const Real origin = converter->Convert(21545., TimeSystemConverter::UTC, TimeSystemConverter::A1);
      const std::string unicode = "GMAT-\xCE\xA9";
      std::vector<TextCase> cases = {{"Full"}, {"Minimal", "", "", "", "", false, true},
         {"SpacedId", "OBJECT_ID", "A  B"},
         {"IdAt254", "OBJECT_ID", std::string(254-std::string("OBJECT_ID            = ").size(), 'A')},
         {"IdOver254", "OBJECT_ID", std::string(255-std::string("OBJECT_ID            = ").size(), 'A'), "", "", true},
         {"UnicodeId", "OBJECT_ID", unicode, "", "", true},
         {"TabId", "OBJECT_ID", "A\tB", "", "", true},
         {"NewlineFrame", "REF_FRAME", "EME2000\nOBJECT_ID = injected", "", "", true},
         {"UnderscoreId", "OBJECT_ID", "___", "", "", true},
         {"OriginatorAt254", "ORIGINATOR", std::string(254-std::string("ORIGINATOR     = ").size(), 'A')},
         {"OriginatorOver254", "ORIGINATOR", std::string(255-std::string("ORIGINATOR     = ").size(), 'A'), "", "", true},
         {"UnicodeOriginator", "ORIGINATOR", unicode, "", "", true},
         {"BlankOriginator", "ORIGINATOR", "   ", "", "", true},
         {"MethodWithoutDegree", "INTERPOLATION_DEGREE", "", "", "", true}};
      for (const auto *field : {"OBJECT_NAME", "OBJECT_ID", "CENTER_NAME", "REF_FRAME", "TIME_SYSTEM", "START_TIME", "STOP_TIME"})
         cases.push_back({std::string("Empty")+field, field, "", "", "", true});
      for (const auto &section : {std::string("header"), std::string("metadata"), std::string("data")})
      {
         cases.push_back({section+"Multiline", "", "", section, " first  \r\nsecond\n\r third\rlast\n"});
         cases.push_back({section+"At254", "", "", section, std::string(254-std::string("COMMENT ").size(), 'C')});
         cases.push_back({section+"Over254", "", "", section, std::string(255-std::string("COMMENT ").size(), 'C'), true});
         cases.push_back({section+"Tab", "", "", section, "bad\tcomment", true});
         cases.push_back({section+"Unicode", "", "", section, unicode, true});
      }
      QJsonArray records;
      int failures = 0;
      for (const auto &test : cases)
      {
         bool rejected = false;
         std::string error;
         try
         {
            CCSDSOEMWriter writer("1.0");
            require(writer.OpenFile(QDir(directory).filePath(QString::fromStdString(test.name+".oem")).toStdString()), "B1-text file open failed");
            writer.SetHeaderForWriting("ORIGINATOR", test.field == "ORIGINATOR" ? test.value : "GMAT B1 text test");
            if (test.commentSection == "header") writer.SetHeaderForWriting("COMMENT", test.comment);
            require(writer.WriteHeader("CCSDS_OEM_VERS"), "B1-text header failed");
            for (const auto &field : {
               std::make_pair("OBJECT_NAME", std::string("TextSat")),
               {"OBJECT_ID", "GMAT-B1-text-native"}, {"CENTER_NAME", "EARTH"},
               {"REF_FRAME", "EME2000"}, {"TIME_SYSTEM", "UTC"},
               {"START_TIME", "2000-01-01T12:00:00.000"}, {"STOP_TIME", "2000-01-01T12:00:01.000"}})
               writer.SetMetaDataForWriting(field.first, test.field == field.first ? test.value : field.second);
            if (!test.minimal)
            {
               writer.SetMetaDataForWriting("INTERPOLATION", "LAGRANGE");
               writer.SetMetaDataForWriting("INTERPOLATION_DEGREE", test.field == "INTERPOLATION_DEGREE" ? test.value : "1");
            }
            if (test.commentSection == "metadata") writer.AddMetaComment(test.comment);
            if (test.commentSection == "data") writer.AddDataComment(test.comment);
            Rvector state(6, 7000., 0., 0., 0., 7.5, 0.);
            writer.AddDataForWriting(origin, state);
            state[1] = 7.5;
            writer.AddDataForWriting(origin+1./86400., state);
            require(writer.WriteMetaData() && writer.WriteDataComments() && writer.WriteDataSegment(), "B1-text segment failed");
         }
         catch (BaseException &caught) { rejected = true; error = caught.GetFullMessage(); }
         const bool passed = rejected == test.reject;
         if (!passed) ++failures;
         records.append(QJsonObject{{"case", QString::fromStdString(test.name)},
            {"field", QString::fromStdString(test.field)}, {"expected_rejection", test.reject},
            {"actual_rejection", rejected}, {"error", QString::fromStdString(error)},
            {"native_expectation_passed", passed}, {"minimal", test.minimal},
            {"comment_section", QString::fromStdString(test.commentSection)}});
      }
      QFile output(QDir(directory).filePath("text-proof.json"));
      const auto bytes = QJsonDocument(QJsonObject{{"cases", records}, {"failures", failures},
         {"scope", "B1 text serialization only;no row or orbit-science credit"}}).toJson(QJsonDocument::Indented);
      require(output.open(QIODevice::WriteOnly) && output.write(bytes) == bytes.size(), "B1-text proof write failed");
      return failures ? 1 : 0;
   }
   catch (BaseException &error) { std::cerr << error.GetFullMessage(); }
   catch (const std::exception &error) { std::cerr << error.what(); }
   return 1;
}
