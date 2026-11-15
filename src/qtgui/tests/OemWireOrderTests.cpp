#include "FormationAcceptance.hpp"
#include "CCSDSOEMWriter.hpp"
#include <functional>
#include <memory>
using namespace formaccept;

namespace
{
class ProbeWriter : public CCSDSOEMWriter
{
public:
   ProbeWriter() : CCSDSOEMWriter("1.0") {}
   std::streampos Position() { return emOutStream.tellp(); }
   void Position(std::streampos value) { emOutStream.seekp(value); }
};

Real at(double seconds) { return 21545.0 + seconds / 86400.0; }

void header(CCSDSOEMWriter &writer, const QString &path)
{
   require(writer.OpenFile(path.toStdString()), "Wire-order output could not open");
   writer.SetHeaderForWriting("ORIGINATOR", "GMAT TEST");
   require(writer.WriteHeader("CCSDS_OEM_VERS"), "Wire-order header failed");
}

void metadata(CCSDSOEMWriter &writer, double start, double stop, bool usable = true, int interpolation = 2)
{
   for (const auto &entry : std::vector<std::pair<std::string, std::string>>{
         {"OBJECT_NAME", "WireOrderProbe"}, {"OBJECT_ID", "2000-001A"},
         {"CENTER_NAME", "EARTH"}, {"REF_FRAME", "EME2000"}, {"TIME_SYSTEM", "UTC"},
         {"START_TIME", writer.FormatEpoch(at(start))}, {"STOP_TIME", writer.FormatEpoch(at(stop))}})
      require(writer.SetMetaDataForWriting(entry.first, entry.second), "Wire-order metadata field failed");
   if (interpolation) writer.SetMetaDataForWriting("INTERPOLATION", "LAGRANGE");
   if (interpolation > 1) writer.SetMetaDataForWriting("INTERPOLATION_DEGREE", "1");
   if (usable)
   {
      writer.SetMetaDataForWriting("USEABLE_START_TIME", writer.FormatEpoch(at(start)));
      writer.SetMetaDataForWriting("USEABLE_STOP_TIME", writer.FormatEpoch(at(stop)));
   }
   require(writer.WriteMetaData(), "Wire-order metadata failed");
}

void data(CCSDSOEMWriter &writer, std::initializer_list<double> seconds)
{
   for (double t : seconds)
   {
      // Synthetic finite Cartesian records test the wire contract only.
      Rvector state(6);
      state[0] = 7000; state[1] = t; state[2] = 0;
      state[3] = 0; state[4] = 1; state[5] = 0;
      require(writer.AddDataForWriting(at(t), state), "Wire-order data buffering failed");
   }
   require(writer.WriteDataSegment(), "Wire-order data serialization failed");
}
}

int main(int argc, char **argv)
{
   QApplication app(argc, argv);
   app.setOrganizationName("GMATTests"); app.setApplicationName("OemWireOrder");
   try
   {
      require(argc == 2, "Supply wire-order test startup");
      Rig rig(QFileInfo(argv[1]).absoluteFilePath());
      struct Case { const char *name; bool reject; std::function<void(ProbeWriter&, const QString&)> run; };
      const std::vector<Case> cases{
         {"Disjoint", false, [](auto &w, const auto&) { metadata(w,0,1); data(w,{0,1}); metadata(w,2,3); data(w,{2,3}); }},
         {"SharedBoundary", false, [](auto &w, const auto&) { metadata(w,0,1); data(w,{0,1}); metadata(w,1,2); data(w,{1,2}); }},
         {"Overlap", true, [](auto &w, const auto&) { metadata(w,0,2); data(w,{0,2}); metadata(w,1,3); data(w,{1,3}); }},
         {"Descending", true, [](auto &w, const auto&) { metadata(w,0,2); data(w,{0,2,1}); }},
         {"ChunkRepeat", true, [](auto &w, const auto&) { metadata(w,0,2); data(w,{0,1}); data(w,{1,2}); }},
         {"MonotoneChunks", false, [](auto &w, const auto&) { metadata(w,0,3); data(w,{0,1}); data(w,{2,3}); }},
         {"OptionalOmitted", false, [](auto &w, const auto&) {
            metadata(w,0,1); data(w,{0,1}); metadata(w,2,3,false); data(w,{2,3}); }},
         {"OptionalInterpolationOmitted", false, [](auto &w, const auto&) {
            metadata(w,0,1); data(w,{0,1}); metadata(w,2,3,false,0); data(w,{2,3}); }},
         {"MissingDegree", true, [](auto &w, const auto&) {
            metadata(w,0,1); data(w,{0,1}); metadata(w,2,3,true,1); data(w,{2,3}); }},
         {"MetadataRewrite", false, [](auto &w, const auto&) {
            const auto begin=w.Position(); metadata(w,0,1); data(w,{0,1}); const auto end=w.Position();
            w.Position(begin); metadata(w,0,1); w.Position(end); metadata(w,1,2); data(w,{1,2}); }},
         {"RewriteOverlap", true, [](auto &w, const auto&) {
            const auto begin=w.Position(); metadata(w,0,1); data(w,{0,1}); const auto end=w.Position();
            w.Position(begin); metadata(w,0,1); w.Position(end); metadata(w,.5,1.5); data(w,{.5,1.5}); }},
         {"Reopen", false, [](auto &w, const auto &path) {
            metadata(w,10,11); data(w,{10,11}); header(w,path); metadata(w,0,1); data(w,{0,1}); }},
         {"Clone", false, [](auto &w, const auto &path) {
            metadata(w,10,11); data(w,{10,11}); std::unique_ptr<CCSDSOEMWriter> copy(w.Clone());
            header(*copy,path+".clone"); metadata(*copy,0,1); data(*copy,{0,1}); }}
      };
      QJsonArray results;
      int failures=0;
      for (const auto &spec : cases)
      {
         const auto path=rig.files.filePath(QString(spec.name)+".oem");
         bool rejected=false; QString error;
         try { ProbeWriter writer; header(writer,path); spec.run(writer,path); }
         catch (BaseException &exception) { rejected=true; error=QString::fromStdString(exception.GetFullMessage()); }
         const bool diagnostic=!rejected || (QString(spec.name)=="MissingDegree" ?
               error.contains("INTERPOLATION_DEGREE") : (error.contains("epoch",Qt::CaseInsensitive) &&
               (error.contains("increasing",Qt::CaseInsensitive) || error.contains("order",Qt::CaseInsensitive))));
         const bool optionalValid=!QString(spec.name).startsWith("Optional") ||
               (QFileInfo::exists(path) && !read(path).contains("UNSET"));
         const bool passed=rejected==spec.reject && diagnostic && optionalValid;
         failures+=!passed;
         if (QFileInfo::exists(path)) rig.save(QString(spec.name)+".oem",read(path));
         if (QFileInfo::exists(path+".clone")) rig.save(QString(spec.name)+"-clone.oem",read(path+".clone"));
         results.append(QJsonObject{{"case",spec.name},{"expected_rejection",spec.reject},
            {"rejected",rejected},{"diagnostic",error},{"optional_values_valid",optionalValid},{"passed",passed}});
         rig.save("results-values.json",QJsonDocument(results).toJson());
      }
      const QJsonObject proof{{"checkpoint","oem-wire-order"},{"outcome",failures?"failed":"passed"},
         {"cases",results},{"failed_cases",failures},
         {"scope","Native OEM1.0 record chronology across metadata blocks,buffer chunks,metadata rewrites,reopen and copy;shared block boundaries remain permitted. Synthetic finite states only;no trajectory accuracy,all metadata semantics or whole NASA row qualification."}};
      rig.save("proof.json",QJsonDocument(proof).toJson());
      std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
      return failures?1:0;
   }
   catch (BaseException &error) { std::cerr<<"FAIL "<<error.GetFullMessage()<<std::endl; }
   catch (const std::exception &error) { std::cerr<<"FAIL "<<error.what()<<std::endl; }
   return 1;
}
