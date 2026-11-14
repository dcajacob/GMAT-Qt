#include "EventRootRefinement.hpp"
#include "BaseException.hpp"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
static void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
int main()
{
   try{
      struct Case{const char *name;std::function<double(double)> f;double a,b,root;};
      const Case cases[]={{"exponential",[](double x){return std::exp(5*x)-3;},0,1,std::log(3.)/5},
         {"negative-exponential",[](double x){return 3-std::exp(-5*x);},-1,0,-std::log(3.)/5},
         {"cosine",[](double x){return std::cos(x)-x;},0,1,.7390851332151606416553120876738734},
         {"cubic",[](double x){return x*x*x-2;},0,2,std::cbrt(2.)}};
      QJsonArray observations;int rejected=0;
      for(const auto &c:cases)for(double tolerance:{1e-2,1e-4,1e-6,1e-10}){
         int calls=0;const double root=GmatEventRoot::Refine([&](double x){++calls;return c.f(x);},c.a,c.b,tolerance);
         require(std::abs(root-c.root)<=tolerance+4*std::numeric_limits<double>::epsilon()*std::abs(root),"Brent result exceeds requested root tolerance");
         require(calls>=2,"Brent did not evaluate native residual");
         observations.append(QJsonObject{{"case",c.name},{"tolerance",tolerance},{"root",root},{"expected",c.root},{"function_evaluations",calls}});
      }
      for(bool left:{false,true}){
         const double root=GmatEventRoot::Refine([](double x){return x-.125;},left?.125:-1.,left?1.:.125,1e-10);
         require(root==.125,"Exact endpoint root changed");observations.append(QJsonObject{{"case",left?"left-endpoint":"right-endpoint"},{"root",root}});
      }
      auto reject=[&](const std::function<void()> &f){bool caught=false;try{f();}catch(BaseException &){caught=true;}require(caught,"Invalid Brent input accepted");++rejected;};
      for(double tolerance:{0.,-1.,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()})reject([&]{GmatEventRoot::Refine([](double x){return x;},-1,1,tolerance);});
      reject([]{GmatEventRoot::Refine([](double x){return x;},1,-1,1e-6);});
      reject([]{GmatEventRoot::Refine([](double x){return x;},-1,std::numeric_limits<double>::infinity(),1e-6);});
      reject([]{GmatEventRoot::Refine([](double x){return x*x+1;},-1,1,1e-6);});
      reject([]{GmatEventRoot::Refine([](double){return std::numeric_limits<double>::quiet_NaN();},-1,1,1e-6);});
      reject([]{GmatEventRoot::Refine([](double x){return x==.5?std::numeric_limits<double>::quiet_NaN():x-.5;},0,1,1e-6);});
      const auto path=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");require(!path.isEmpty(),"Scalar policy evidence directory missing");
      QFile file(QDir(path).filePath("proof.json"));require(file.open(QIODevice::WriteOnly),"Cannot write scalar policy evidence");
      file.write(QJsonDocument(QJsonObject{{"status","passed"},{"scope","Exported native Brent scalar API; no GUI or physical geometry approval"},
         {"observations",observations},{"invalid_rejections",rejected},{"new_requirement_credit",0}}).toJson());return 0;
   }catch(BaseException &e){std::cerr<<e.GetFullMessage()<<'\n';}catch(std::exception &e){std::cerr<<e.what()<<'\n';}return 1;
}
