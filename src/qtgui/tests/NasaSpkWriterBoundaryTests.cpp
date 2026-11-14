#include "SpiceOrbitKernelWriter.hpp"
#include "FileManager.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
#include <vector>

// Observe outstanding C++ array allocations across repeated rejected writes.
// Scalar/vector allocations and the SPICE C allocator are outside this counter.
static bool observeArrays = false;
static std::array<void *, 4096> arrays{};
static size_t outstandingArrays = 0;
void *operator new[](std::size_t n)
{
   void *p = std::malloc(n ? n : 1);
   if (!p) throw std::bad_alloc();
   if (observeArrays)
   {
      auto slot = std::find(arrays.begin(), arrays.end(), nullptr);
      if (slot == arrays.end()) { std::free(p); throw std::bad_alloc(); }
      *slot = p;
      ++outstandingArrays;
   }
   return p;
}
void operator delete[](void *p) noexcept
{
   if (!p) return;
   auto slot = std::find(arrays.begin(), arrays.end(), p);
   if (slot != arrays.end()) { *slot = nullptr; --outstandingArrays; }
   std::free(p);
}
void operator delete[](void *p, std::size_t) noexcept { operator delete[](p); }

static void require(bool ok, const char *why)
{
   if (!ok) throw std::runtime_error(why);
}
static int handles()
{
   // DAFHOF is exposed by this build's existing Fortran SPICE header.
   integer open[262] = {}, capacity = 256;
   ssizei_(&capacity, open);
   dafhof_(open);
   require(!failed_c(), "Cannot inspect open native DAF handles");
   return cardi_(open);
}
static void save(const QString &path, const QJsonObject &value)
{
   QFile f(path);
   require(f.open(QIODevice::WriteOnly), "Cannot preserve SPK boundary evidence");
   const auto bytes = QJsonDocument(value).toJson();
   require(f.write(bytes) == bytes.size(), "Incomplete SPK boundary evidence");
}
class Probe : public SpiceOrbitKernelWriter
{
public:
   Probe(const QString &path, const std::string &frame = "ECLIPJ2000") :
      SpiceOrbitKernelWriter("BoundaryProbe", "Earth", -100075, 399,
                             path.toStdString(), 7, frame) {}
   // A red pre-fix assertion must not itself leave an open test-fixture file.
   ~Probe()
   {
      if (fileOpen && !dataWritten) { dafcls_c(handle); reset_c(); fileOpen = false; }
   }
   bool open() const { return fileOpen; }
   bool written() const { return dataWritten; }
   void frame(const std::string &name) { frameName = name; referenceFrame = frameName.c_str(); }
   void missingFlags(const A1Mjd &start, const A1Mjd &end,
                     const StateArray &s, const EpochArray &e,
                     const std::vector<GmatTime> &p)
   { WriteSegmentImpl(start, end, s, e, &p, nullptr); }
};
struct Input
{
   std::vector<Rvector6> states;
   std::vector<A1Mjd> epochs;
   StateArray statePointers;
   EpochArray epochPointers;
   std::vector<GmatTime> precise;
   std::vector<bool> flags;
   std::vector<double> encoded;
   A1Mjd start, end;
   explicit Input(SpiceInterface &clock)
   {
      states.reserve(8); epochs.reserve(8); precise.reserve(8);
      GmatTime first(26088.0); first.AddSeconds(20e-9);
      for (int i = 0; i < 8; ++i)
      {
         auto epoch = first; epoch.AddSeconds(i * 20.0);
         precise.push_back(epoch); epochs.emplace_back(epoch.GetMjd());
         flags.push_back(i % 2 == 0);
         encoded.push_back(flags.back() ? clock.A1ToSpiceTime(epoch) : clock.A1ToSpiceTime(epoch.GetMjd()));
      }
      for (int i = 0; i < 8; ++i)
      {
         const double t = encoded[i] - encoded[0];
         states.emplace_back(100 + 2*t + .25*t*t, -30 + t, 4 - 3*t,
                             2 + .5*t, 1, -3);
         statePointers.push_back(&states.back());
         epochPointers.push_back(&epochs[i]);
      }
      start = epochs.front(); end = epochs.back();
   }
   void write(SpiceOrbitKernelWriter &writer, bool legacy = false)
   {
      if (legacy) writer.WriteSegment(start, end, statePointers, epochPointers);
      else writer.WriteSegment(start, end, statePointers, epochPointers, precise, flags);
   }
};
static int readBack(const QString &path, const Input &in)
{
   SpiceInt loaded;
   spklef_c(path.toUtf8().constData(), &loaded);
   require(!failed_c(), "Recovered SPK could not be loaded");
   int components = 0;
   for (int i = 0; i < 15; ++i)
   {
      const int k = i/2;
      const double et = i%2 ? (in.encoded[k]+in.encoded[k+1])/2 : in.encoded[k];
      const double t = et-in.encoded[0];
      const double expected[6] = {100+2*t+.25*t*t, -30+t, 4-3*t, 2+.5*t, 1, -3};
      SpiceDouble actual[6], lightTime;
      spkez_c(-100075, et, "ECLIPJ2000", "NONE", 399, actual, &lightTime);
      require(!failed_c(), "Recovered SPK query failed");
      for (int j = 0; j < 6; ++j)
      {
         require(std::abs(actual[j]-expected[j]) < (j<3 ? 1e-9 : 1e-10),
                 "Recovered SPK differs from its prescribed quadratic data");
         ++components;
      }
   }
   spkuef_c(loaded);
   require(!failed_c(), "Recovered SPK unload failed");
   return components;
}
int main(int argc, char **argv)
{
   QCoreApplication app(argc, argv);
   try
   {
      require(argc==3, "Expected startup file and Storage/Recovery case");
      const QString family=argv[2];
      require(family=="Storage" || family=="Recovery", "Unknown SPK boundary case");
      FileManager::Instance()->ReadStartupFile(argv[1]);
      QTemporaryDir fallback;
      QString folder=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
      if (folder.isEmpty()) folder=fallback.path();
      folder=QDir(folder).filePath("native-files");
      require(QDir().mkpath(folder), "Cannot create native SPK fixture directory");
      FileManager::Instance()->SetAbsPathname("OUTPUT_PATH", folder.toStdString());
      QJsonArray observations;
      int rejected=0, componentChecks=0, repeated=0, abandoned=0;
      const double nan=std::numeric_limits<double>::quiet_NaN();
      if (family=="Storage")
      {
         const auto path=QDir(folder).filePath("storage.bsp");
         Probe writer(path);
         const int before=handles();
         using Mutation=std::function<void(Input&)>;
         const std::vector<std::pair<const char*,Mutation>> cases={
            {"state-count",[](Input &v){v.statePointers.pop_back();}},
            {"epoch-count",[](Input &v){v.epochPointers.pop_back();}},
            {"precise-count",[](Input &v){v.precise.pop_back();}},
            {"flag-count",[](Input &v){v.flags.pop_back();}},
            {"precise-mismatch",[](Input &v){v.precise[2].AddSeconds(1);}},
            {"null-state",[](Input &v){v.statePointers[2]=nullptr;}},
            {"null-epoch",[](Input &v){v.epochPointers[2]=nullptr;}},
            {"nonfinite-state",[&](Input &v){v.states[2][1]=nan;}},
            {"nonfinite-epoch",[&](Input &v){v.epochs[2]=A1Mjd(nan);}},
            {"duplicate-epoch",[](Input &v){v.epochs[2]=v.epochs[0];v.precise[2]=v.precise[0];}},
            {"reversed-epochs",[](Input &v){std::swap(v.epochs[2],v.epochs[4]);std::swap(v.precise[2],v.precise[4]);}},
            {"reversed-descriptor",[](Input &v){std::swap(v.start,v.end);}},
            {"descriptor-before-records",[](Input &v){v.start=A1Mjd(v.start.Get()-1);}},
            {"descriptor-after-records",[](Input &v){v.end=A1Mjd(v.end.Get()+1);}},
            {"empty-records",[](Input &v){v.statePointers.clear();v.epochPointers.clear();v.precise.clear();v.flags.clear();}},
            {"too-few-records",[](Input &v){v.statePointers.resize(3);v.epochPointers.resize(3);v.precise.resize(3);v.flags.resize(3);}},
            {"nonfinite-start",[&](Input &v){v.start=A1Mjd(nan);}},
            {"nonfinite-end",[&](Input &v){v.end=A1Mjd(nan);}}
         };
         for (const auto &test:cases)
         {
            Input in(writer); test.second(in);
            bool threw=false;std::string diagnostic;
            try { in.write(writer); }
            catch (BaseException &e) { threw=true;diagnostic=e.GetFullMessage(); }
            const QJsonObject observed{{"case",test.first},{"rejected",threw},{"file_created",QFile::exists(path)},{"writer_open",writer.open()},{"data_written",writer.written()},{"native_handles_delta",handles()-before},{"diagnostic",QString::fromStdString(diagnostic)}};
            observations.append(observed);save(QDir(folder).filePath("observations.json"),QJsonObject{{"observations",observations}});
            std::cout<<"NASA_OBSERVATION "<<QJsonDocument(observed).toJson(QJsonDocument::Compact).constData()<<std::endl;
            require(threw && !diagnostic.empty(), "Invalid SPK record metadata was accepted");
            require(!QFile::exists(path) && !writer.open() && !writer.written() && handles()==before,
                    "Rejected SPK metadata opened/changed native output before validation");
            require(!failed_c(), "Rejected SPK metadata poisoned SPICE global error state");
            ++rejected;
         }
         Input in(writer);
         bool threw=false;
         try { writer.missingFlags(in.start,in.end,in.statePointers,in.epochPointers,in.precise); }
         catch (BaseException &) { threw=true; }
         require(threw && !QFile::exists(path) && !writer.open() && handles()==before,
                 "Missing precise flags reached native output");
         ++rejected;
         // Unflagged precise entries are intentionally ignored for legacy data.
         in.precise[1].AddSeconds(100);
         in.write(writer);writer.FinalizeKernel();
         componentChecks+=readBack(path,in);
         require(handles()==before, "Successful retry retained a native DAF handle");
      }
      else
      {
         const int before=handles();
         const auto path=QDir(folder).filePath("recovery.bsp");
         {
            Probe writer(path,"NO_SUCH_FRAME_BOUNDARY");Input in(writer);
            observeArrays=true;
            for (int i=0;i<256;++i)
            {
               bool threw=false;
               try { in.write(writer); } catch (BaseException &) { threw=true; }
               require(threw && !failed_c(), "Native SPK rejection did not reset SPICE error state");
               ++repeated;
            }
            observeArrays=false;
            const QJsonObject observed{{"case","repeated-native-rejection"},{"rejections",repeated},{"outstanding_cpp_arrays",int(outstandingArrays)},{"native_handles_delta",handles()-before}};
            observations.append(observed);save(QDir(folder).filePath("observations.json"),QJsonObject{{"observations",observations}});
            std::cout<<"NASA_OBSERVATION "<<QJsonDocument(observed).toJson(QJsonDocument::Compact).constData()<<std::endl;
            require(outstandingArrays==0, "Rejected native SPK writes leaked C++ arrays");
            require(handles()==before+1, "Repeated rejected writes lost or multiplied native handles");
            writer.frame("ECLIPJ2000");in.write(writer);writer.FinalizeKernel();
            componentChecks+=readBack(path,in);
         }
         require(handles()==before, "Recovered writer retained a native handle");
         // Exercise the actual production destructor, without Probe's red-test cleanup.
         for (int i=0;i<4;++i)
         {
            const auto empty=QDir(folder).filePath(QString("abandoned-%1.bsp").arg(i));
            {
               SpiceOrbitKernelWriter writer("AbandonedBoundary","Earth",-100076,399,empty.toStdString(),7,"NO_SUCH_FRAME_BOUNDARY");
               Input in(writer);bool threw=false;
               try { in.write(writer); } catch (BaseException &) { threw=true; }
               require(threw && !failed_c(), "Abandoned native rejection did not restore SPICE");
            }
            require(handles()==before, "Abandoned rejected SPK writer leaked its native handle");
            ++abandoned;
         }
      }
      const QJsonObject proof{{"checkpoint","spk-writer-boundary-"+family},{"outcome","passed"},{"family",family},{"rejected_input_classes",rejected},{"repeated_native_rejections",repeated},{"abandoned_writer_checks",abandoned},{"decoded_components",componentChecks},{"outstanding_cpp_arrays",int(outstandingArrays)},{"facets",QJsonArray{"invalid-native-records-before-file-open","native-rejection-allocation-and-error-recovery","mixed-precise-legacy-record-retry-and-decoding"}},{"scope","Native writer structural validation and failure recovery on prescribed quadratic ECLIPJ2000 data;not GUI input delivery,canonical state sampling or physical batch SPK acceptance"}};
      save(QDir(folder).filePath("proof.json"),proof);
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      return 0;
   }
   catch (BaseException &e) { observeArrays=false;std::cerr<<e.GetFullMessage()<<std::endl; }
   catch (std::exception &e) { observeArrays=false;std::cerr<<e.what()<<std::endl; }
   return 1;
}
