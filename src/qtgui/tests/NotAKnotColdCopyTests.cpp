#include "NotAKnotInterpolator.hpp"
#include "BaseException.hpp"
#include <QCoreApplication>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>
#include <vector>

static void require(bool value,const char *message)
{
   if (!value) throw std::runtime_error(message);
}

// A diagnostic observes copied scalar caches before BuildSplines overwrites
// them. It does not require particular cache values or use them as a numerical
// oracle. Poisoned pre-lifetime storage exposes omitted initialization.
class CacheProbe final : public NotAKnotInterpolator
{
public:
   explicit CacheProbe(int dimension): NotAKnotInterpolator("ColdCopy",dimension) {}
   bool finiteCopiedScalars() const
   {
      for (double value:h) if (!std::isfinite(value)) return false;
      for (const auto &row:A) for (double value:row) if (!std::isfinite(value)) return false;
      return true;
   }
};

static constexpr std::array<double,5> knots{0,.5,1.75,3,5};
static double polynomial(double t,int column)
{
   return (column+1)*(1+2*t-3*t*t+.25*t*t*t)+10*column;
}
static void add(Interpolator &interpolator,int dimension,int direction,int index)
{
   const double t=direction*knots[index]; std::vector<double> value(dimension);
   for (int j=0;j<dimension;++j) value[j]=polynomial(t,j);
   require(interpolator.AddPoint(t,value.data()),"Cold copy rejected a valid knot");
}
static int check(Interpolator &interpolator,int dimension,int direction,int prefix)
{
   require(interpolator.GetPointCount()==prefix,"Cold copy changed the populated point count");
   if (prefix<5) {
      std::vector<double> untouched(dimension,-999); bool rejected=false;
      try { interpolator.Interpolate(direction*.9,untouched.data()); }
      catch (BaseException &) { rejected=true; }
      require(rejected&&interpolator.GetPointCount()==prefix,"Incomplete cold copy interpolated or changed its point count");
      for (double value:untouched) require(value==-999,"Incomplete cold interpolation changed caller output");
   }
   for (int i=prefix;i<5;++i) add(interpolator,dimension,direction,i);
   int values=0;
   for (double point:{.1,.9,2.4,4.2}) {
      const double t=direction*point; std::vector<double> actual(dimension,-999);
      require(interpolator.Interpolate(t,actual.data()),"Cold-copy continuation interpolation failed");
      for (int j=0;j<dimension;++j) {
         require(std::isfinite(actual[j])&&std::abs(actual[j]-polynomial(t,j))<1e-10,
            "Cold copy differs from independent cubic-polynomial oracle");
         ++values;
      }
   }
   return values;
}

int main(int argc,char **argv)
{
   QCoreApplication app(argc,argv);
   try {
      int scenarios=0,copies=0,values=0;
      for (int dimension:{1,3,6}) for (int direction:{1,-1}) for (int prefix=0;prefix<=5;++prefix) {
         alignas(CacheProbe) std::array<unsigned char,sizeof(CacheProbe)> storage;
         std::memset(storage.data(),0xff,storage.size());
         auto destroy=[](CacheProbe *pointer) { pointer->~CacheProbe(); };
         std::unique_ptr<CacheProbe,decltype(destroy)> source(new (storage.data()) CacheProbe(dimension),destroy);
         for (int i=0;i<prefix;++i) add(*source,dimension,direction,i);

         CacheProbe copied(*source);
         require(copied.finiteCopiedScalars(),"Cold copy read an uninitialized scalar cache before its first spline build");
         values+=check(copied,dimension,direction,prefix); ++copies;

         CacheProbe assigned(dimension); assigned=*source;
         require(assigned.finiteCopiedScalars(),"Cold assignment read an uninitialized scalar cache");
         values+=check(assigned,dimension,direction,prefix); ++copies;

         CacheProbe hotDestination(dimension+1);
         check(hotDestination,dimension+1,-direction,0);
         hotDestination=*source;
         require(hotDestination.finiteCopiedScalars(),"Hot-to-cold assignment propagated an uninitialized scalar cache");
         values+=check(hotDestination,dimension,direction,prefix); ++copies;

         std::unique_ptr<Interpolator> clone(source->Clone());
         require(bool(clone),"Cold native Clone returned no interpolator");
         values+=check(*clone,dimension,direction,prefix); ++copies;
         ++scenarios;
      }
      std::cout<<"PASS NotAKnot cold scalar initialization and independent cubic continuation: "
         <<scenarios<<" scenarios, "<<copies<<" copy/assignment/clone paths, "<<values
         <<" compared values; dimensions 1/3/6, both directions, 0..5 knots before the first spline build. No GUI or broad interpolation qualification.\n";
      return 0;
   } catch (BaseException &error) { std::cerr<<"FAIL "<<error.GetFullMessage()<<'\n'; }
   catch (const std::exception &error) { std::cerr<<"FAIL "<<error.what()<<'\n'; }
   return 1;
}
