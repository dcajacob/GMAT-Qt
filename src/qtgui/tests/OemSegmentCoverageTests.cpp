#include "CCSDSEphemerisFile.hpp"
#include "CCSDSOEMReader.hpp"
#include "FileManager.hpp"
#include "LeapSecsFileReader.hpp"
#include "TimeSystemConverter.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool passed, const char *message)
{
   if (!passed) throw std::runtime_error(message);
}

QString timestamp(int second)
{
   return QString("2018-01-02T00:%1:%2.123456789")
      .arg(second / 60, 2, 10, QLatin1Char('0')).arg(second % 60, 2, 10, QLatin1Char('0'));
}

GmatTime instant(int second)
{
   return CCSDSEMSegment::ParseEpochGT(timestamp(second).toStdString());
}

QString segment(int start, int end, const QString &frame = "EME2000",
      const QString &body = "Earth", int offset = 0, int usableStart = -1, int usableEnd = -1)
{
   QString text = "META_START\nOBJECT_NAME = Test\nOBJECT_ID = Test\nCENTER_NAME = " + body +
         "\nREF_FRAME = " + frame + "\nTIME_SYSTEM = UTC\nSTART_TIME = " + timestamp(start) +
         "\nSTOP_TIME = " + timestamp(end) + "\n";
   if (usableStart >= 0)
      text += "USEABLE_START_TIME = " + timestamp(usableStart) + "\nUSEABLE_STOP_TIME = " +
            timestamp(usableEnd) + "\n";
   text += "INTERPOLATION = LAGRANGE\nINTERPOLATION_DEGREE = 1\nMETA_STOP\n";
   for (int t = start; t <= end; t += 10)
      text += timestamp(t) + " " + QString::number(7000 + t + offset) + " 10 20 1 0 0\n";
   return text;
}

std::string writeOem(const QTemporaryDir &dir, const QString &name, const QString &segments)
{
   QFile file(dir.filePath(name + ".oem"));
   const QByteArray bytes = ("CCSDS_OEM_VERS = 1.0\nCREATION_DATE = 2018-01-02T00:00:00\n"
         "ORIGINATOR = GMAT\n" + segments).toUtf8();
   check(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size(), "Cannot write OEM fixture");
   return file.fileName().toStdString();
}

template<typename Action>
void rejects(Action action, const std::string &message)
{
   try { action(); }
   catch (BaseException &error) {
      if (error.GetFullMessage().find(message) != std::string::npos) return;
      throw;
   }
   throw std::runtime_error("Expected rejection containing " + message);
}

void frames(const QTemporaryDir &dir)
{
   CCSDSEphemerisFile file;
   file.OpenForRead(writeOem(dir,"uniform",segment(0,30) + segment(40,60)), "TimePosVel");
   check(std::abs(file.InterpolatePoint(instant(1))[0] - 7001) < 1e-8, "Uniform frame state changed");
   for (const auto &later : {segment(40,60,"GRC"), segment(40,60,"EME2000","Mars")}) {
      auto path = writeOem(dir, later.contains("Mars") ? "body" : "frame", segment(0,30) + later);
      rejects([&] { file.OpenForRead(path, "TimePosVel"); }, "same CENTER_NAME and REF_FRAME");
      rejects([&] { file.InterpolatePoint(instant(1)); }, "outside");
      writeOem(dir, later.contains("Mars") ? "body" : "frame",segment(0,30));
      check(file.OpenForRead(path,"TimePosVel"), "Corrected OEM at the same path was not reloaded");
   }
}

void usableInterval(const QTemporaryDir &dir)
{
   CCSDSEphemerisFile file;
   const auto path = writeOem(dir,"padding",segment(0,40,"EME2000","Earth",0,5,35));
   file.OpenForRead(path,"TimePosVel");
   CCSDSOEMReader reader;
   reader.SetFile(path);
   const auto start = instant(5), stop = instant(35);
   Real first, last;
   Rvector6 initial, final;
   std::string body, frame;
   check(file.GetInitialAndFinalStates(first,last,initial,final,body,frame), "Missing usable endpoint states");
   check(first == start.GetMjd() && last == stop.GetMjd(), "Published span includes padding");
   check(std::abs(initial[0]-7005) < 1e-8 && std::abs(final[0]-7035) < 1e-8, "Endpoints lost interpolation support");
   check(file.theEphem.front().points.size() == 5, "Padding samples were discarded");
   for (const int t : {5, 35}) {
      auto query = instant(t);
      check(std::abs(file.InterpolatePoint(query)[0]-(7000+t)) < 1e-8, "Precise usable boundary rejected");
      check(std::abs(file.InterpolatePoint(query.GetMjd())[0]-(7000+t)) < 1e-8, "Rounded usable boundary rejected");
      check(std::abs(reader.GetState(query.GetMjd())[0] - (7000+t)) < 1e-5, "Direct reader lost interpolation support");
      const Real outside = std::nextafter(query.GetMjd(), query.GetMjd() + (t == 5 ? -1 : 1));
      rejects([&] { reader.GetState(outside); }, "not contained");
      query.AddSeconds(t == 5 ? -1e-8 : 1e-8);
      rejects([&] { file.InterpolatePoint(query); }, "outside");
   }
   for (const int t : {0,40})
      rejects([&] { file.InterpolatePoint(instant(t).GetMjd()); }, "outside");
}

void gapEndpoint(const QTemporaryDir &dir)
{
   CCSDSEphemerisFile file;
   file.OpenForRead(writeOem(dir,"gap",segment(0,30) + segment(40,60)),"TimePosVel");
   const auto end = instant(30);
   check(file.FindSegment(end.GetMjd()) == 0 && file.InterpolatePoint(end)[0] == 7030 &&
         file.InterpolatePoint(end.GetMjd())[0] == 7030, "Lost the last sample before a gap");
   rejects([&] { file.InterpolatePoint(instant(31)); }, "outside");
   const auto shared = writeOem(dir,"shared",segment(0,30) + segment(30,60,"EME2000","Earth",100));
   file.OpenForRead(shared,"TimePosVel");
   CCSDSOEMReader reader;
   reader.SetFile(shared);
   check(reader.GetState(end.GetMjd())[0] == 7130, "Direct reader used the earlier shared endpoint");
   check(file.FindSegment(end.GetMjd()) == 1 && file.InterpolatePoint(end)[0] == 7130 &&
         file.InterpolatePoint(end.GetMjd())[0] == 7130, "Shared endpoint used the preceding segment");
}

void orderedSegments(const QTemporaryDir &dir)
{
   CCSDSOEMReader reader;
   const auto bad = writeOem(dir,"overlap",segment(0,20) + segment(30,60) + segment(40,70));
   rejects([&] { reader.SetFile(bad); }, "not properly ordered");
   rejects([&] { reader.SetFile(bad); }, "not properly ordered");
   rejects([&] { reader.GetState(instant(50).GetMjd()); }, "not contained");
   // Support records may overlap when the declared usable intervals do not.
   reader.SetFile(writeOem(dir,"ordered",segment(0,20) + segment(30,60,"EME2000","Earth",0,30,45) +
         segment(40,70,"EME2000","Earth",0,45,70)));
   check(reader.GetNumberOfSegments() == 3, "Rejected ordered usable intervals");
}
}

int main(int argc, char **argv)
{
   QCoreApplication application(argc,argv);
   try {
      check(argc == 3, "Expected a startup file and a test case");
      auto *manager = FileManager::Instance();
      manager->ReadStartupFile(argv[1]);
      LeapSecsFileReader leap(manager->GetFullPathname("LEAP_SECS_FILE"));
      check(leap.Initialize(), "Missing leap-second data");
      TimeSystemConverter::Instance()->SetLeapSecsFileReader(&leap);
      QTemporaryDir dir;
      check(dir.isValid(), "Cannot create temporary directory");
      const std::string name(argv[2]);
      if (name == "Frames") frames(dir);
      else if (name == "UsableInterval") usableInterval(dir);
      else if (name == "GapEndpoint") gapEndpoint(dir);
      else if (name == "OrderedSegments") orderedSegments(dir);
      else throw std::runtime_error("Unknown case " + name);
      return 0;
   } catch (BaseException &error) { std::cerr << error.GetFullMessage() << '\n'; }
   catch (const std::exception &error) { std::cerr << error.what() << '\n'; }
   return 1;
}
