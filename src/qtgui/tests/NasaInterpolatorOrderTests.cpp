#include "Interpolator.hpp"
#include "LagrangeInterpolator.hpp"
#include "LinearInterpolator.hpp"
#include "CubicSplineInterpolator.hpp"
#include "NotAKnotInterpolator.hpp"
#include "HermiteInterpolator.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

static void require(bool value,const char *message)
{
   if (!value) throw std::runtime_error(message);
}

static std::unique_ptr<Interpolator> make(const QString &kind)
{
   const auto name=("Acceptance-"+kind).toStdString();
   if (kind=="Lagrange") return std::make_unique<LagrangeInterpolator>(name,3,3);
   if (kind=="Linear") return std::make_unique<LinearInterpolator>(name,3);
   if (kind=="CubicSpline") return std::make_unique<CubicSplineInterpolator>(name,3);
   if (kind=="NotAKnot") return std::make_unique<NotAKnotInterpolator>(name,3);
   if (kind=="Hermite") return std::make_unique<HermiteInterpolator>(name,3,5);
   throw std::runtime_error("Unknown native interpolator");
}

static QJsonArray result(Interpolator &interpolator,double x)
{
   double actual[3]={-999,-999,-999};
   require(interpolator.Interpolate(x,actual),"Monotonic linear-polynomial interpolation failed");
   QJsonArray values;
   for (int j=0;j<3;++j) {
      require(std::isfinite(actual[j])&&std::abs(actual[j]-((j+1)*x+10*j))<1e-10,
              "Native interpolation disagrees with independent linear polynomial");
      values.append(actual[j]);
   }
   return values;
}

int main(int argc,char **argv)
{
   QCoreApplication app(argc,argv); QJsonArray trials,failures;
   for (const auto &kind:QStringList{"Lagrange","Linear","CubicSpline","NotAKnot","Hermite"})
      for (const int direction:{1,-1}) {
         QJsonObject trial{{"kind",kind},{"direction",direction}};
         try {
            auto interpolator=make(kind); QJsonArray points;
            auto add=[&](double x) {
               const double values[3]={x,2*x+10,3*x+20};
               require(interpolator->AddPoint(x,values),"Valid native point rejected");
               points.append(x);
            };
            for (int i=0;i<5;++i) add(direction*i);
            trial["points"]=points;
            const int count=interpolator->GetPointCount();
            const double query=direction*3.5;
            trial["before"]=result(*interpolator,query);
            const double reversal=direction*2.5,invalid[3]={997,991,983};
            bool rejected=false; QString diagnostic;
            try { interpolator->AddPoint(reversal,invalid); }
            catch (BaseException &e) { rejected=true; diagnostic=QString::fromStdString(e.GetFullMessage()); }
            trial["reversal"]=reversal; trial["rejected"]=rejected; trial["diagnostic"]=diagnostic;
            require(rejected&&diagnostic.contains("not monotonic")&&diagnostic.contains(QString::fromStdString(interpolator->GetName())),
                    "Nonmonotonic native input was accepted or lacks an identified diagnostic");
            require(interpolator->GetPointCount()==count,"Rejected input changed native buffer count");
            trial["after"]=result(*interpolator,query);
            require(trial["after"]==trial["before"],"Rejected nonmonotonic data contaminated interpolation");
            std::unique_ptr<Interpolator> copy(interpolator->Clone());
            require(result(*copy,query)==trial["before"].toArray(),"Clone lost valid native interpolation buffer");
            bool cloneRejected=false;
            try { copy->AddPoint(reversal,invalid); } catch (BaseException &) { cloneRejected=true; }
            require(cloneRejected&&copy->GetPointCount()==count,"Clone did not preserve monotonicity validation");
            if (kind!="Hermite") {
               auto assigned=make(kind);
               if (kind=="Lagrange") *static_cast<LagrangeInterpolator *>(assigned.get())=*static_cast<LagrangeInterpolator *>(interpolator.get());
               if (kind=="Linear") *static_cast<LinearInterpolator *>(assigned.get())=*static_cast<LinearInterpolator *>(interpolator.get());
               if (kind=="CubicSpline") *static_cast<CubicSplineInterpolator *>(assigned.get())=*static_cast<CubicSplineInterpolator *>(interpolator.get());
               if (kind=="NotAKnot") *static_cast<NotAKnotInterpolator *>(assigned.get())=*static_cast<NotAKnotInterpolator *>(interpolator.get());
               require(result(*assigned,query)==trial["before"].toArray(),"Assignment lost populated interpolation state");
               const double next=direction*5,values[3]={next,2*next+10,3*next+20};
               require(assigned->AddPoint(next,values),"Assigned buffer rejected a valid continuation");
               trial["assigned_continuation"]=result(*assigned,direction*4.5);
               bool assignedRejected=false;
               try { assigned->AddPoint(reversal,invalid); } catch (BaseException &) { assignedRejected=true; }
               require(assignedRejected,"Assignment lost independent-data direction");
            }
            interpolator->Clear(); require(interpolator->GetPointCount()==0,"Clear retained native point count");
            for (int i=0;i<5;++i) add(-direction*(i+10));
            trial["recovery_query"]=-direction*13.5;
            trial["recovery"]=result(*interpolator,-direction*13.5);
            // Test the first possible reversal without requiring enough points
            // to interpolate, then force the ring to forget its distinct pairs.
            // The stream direction must survive both wrapping and equal points.
            interpolator->Clear(); add(0); add(direction*2);
            bool earlyRejected=false;
            try { interpolator->AddPoint(direction,invalid); } catch (BaseException &) { earlyRejected=true; }
            require(earlyRejected&&interpolator->GetPointCount()==2,"First possible direction reversal was not atomic");
            for (int i=3;i<40;++i) add(direction*i);
            for (int i=0;i<interpolator->GetBufferSize()+2;++i) add(direction*39);
            const int wrappedCount=interpolator->GetPointCount(); bool wrappedRejected=false;
            try { interpolator->AddPoint(direction*38,invalid); } catch (BaseException &) { wrappedRejected=true; }
            require(wrappedRejected&&interpolator->GetPointCount()==wrappedCount,"Wrapped/repeated inputs lost the stream direction");
            trial["early_rejected"]=earlyRejected;trial["wrapped_rejected"]=wrappedRejected;trial["wrapped_count"]=wrappedCount;
            trial["outcome"]="passed";
         } catch (BaseException &e) { trial["failure"]=QString::fromStdString(e.GetFullMessage()); failures.append(trial); }
         catch (std::exception &e) { trial["failure"]=e.what(); failures.append(trial); }
         trials.append(trial);
      }
   const QJsonObject payload{{"trials",trials},{"failures",failures}};
   const auto folder=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");
   if (!folder.isEmpty()) {
      QFile file(QDir(folder).filePath("native-order.json"));
      if (!file.open(QIODevice::WriteOnly)) return 2;
      file.write(QJsonDocument(payload).toJson());
   }
   std::cout<<QJsonDocument(payload).toJson(QJsonDocument::Compact).constData()<<std::endl;
   if (!failures.isEmpty()) return 1;
   const QJsonObject proof{{"checkpoint","native-interpolator-order"},{"outcome","passed"},{"trials",trials},
      {"facets",QJsonArray{"five-native-interpolators-increasing-and-decreasing-linear-oracles","nonmonotonic-error-atomic-buffer-and-clone","clear-opposite-direction-recovery-and-early-wrapped-repeated-reversals"}},
      {"scope","Direct native scientific companion, not GUI event evidence. Five interpolator classes, three dependent dimensions, increasing/decreasing linear polynomials, reversal rejection before buffer mutation, clone and Clear recovery, first possible reversal and direction across wrapped/repeated input. General duplicate/nonfinite interpolation, derivative Hermite science and arbitrary stream stress remain unqualified."}};
   if (!folder.isEmpty()) { QFile f(QDir(folder).filePath("proof.json"));if(!f.open(QIODevice::WriteOnly))return 2;f.write(QJsonDocument(proof).toJson()); }
   std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;
   std::cout<<"Native interpolator direction, atomic refusal, clone and Clear recovery passed; this is not GUI workflow evidence."<<std::endl;
   return 0;
}
