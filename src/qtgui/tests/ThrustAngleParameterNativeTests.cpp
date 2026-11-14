#include "ThrustSegment.hpp"
#include "ThrustSegmentParameters.hpp"
#include "ParameterWrapper.hpp"
#include "FileManager.hpp"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

static void require(bool v,const char *m){if(!v)throw std::runtime_error(m);}
static Rvector seed(int n,double base){Rvector v(n);for(int i=0;i<n;++i)v[i]=base+.125*i;return v;}
static QJsonArray array(const Rvector &v){QJsonArray a;for(int i=0;i<v.GetSize();++i)a.append(v[i]);return a;}
static void same(const Rvector &a,const Rvector &b){require(a.GetSize()==b.GetSize(),"Thrust-angle native vector size mismatch");for(int i=0;i<a.GetSize();++i)require(std::isfinite(a[i])&&std::abs(a[i]-b[i])<=1e-12,"Thrust-angle native vector value mismatch");}
static void save(const QString &n,const QJsonObject &v){const auto d=qEnvironmentVariable("GMAT_QT_ACCEPTANCE_DIR");if(!d.isEmpty()){QFile f(QDir(d).filePath(n));require(f.open(QIODevice::WriteOnly)&&f.write(QJsonDocument(v).toJson())>0,"Native thrust-angle artifact write failed");}}

template<class T> static QJsonArray exercise(const std::string &key,bool bindingOnly)
{
   QJsonArray rows;
   const std::vector<int> lengths=bindingOnly?std::vector<int>{3}:std::vector<int>{1,3,4};
   for(int n:lengths)
   {
      ThrustSegment owner("NativeOwner"),other("OtherOwner");owner.SetRvectorParameter(key,seed(n,10));other.SetRvectorParameter(key,seed(n,90));
      T actual("NativeOwner."+key,&owner);const bool added=actual.AddRefObject(&owner);const QJsonObject diagnostic{{"type",QString::fromStdString(key)},{"length",n},{"added",added},{"owner_primary_type",int(owner.GetType())},{"owner_supports_thrust_segment",owner.IsOfType(Gmat::THRUST_SEGMENT)},{"parameter_owner_type",int(actual.GetOwnerType())},{"native_vector",array(owner.GetRvectorParameter(key))}};save("binding-diagnostic.json",diagnostic);
      require(added&&actual.GetRefObject(Gmat::THRUST_SEGMENT,"NativeOwner")==&owner&&actual.Validate(),"Native ThrustSegment Parameter rejected actual supported owner");
      if(bindingOnly){rows.append(diagnostic);continue;}
      require(actual.Initialize(),"Native thrust-angle Initialize failed");same(actual.EvaluateRvector(),seed(n,10));require(actual.GetIntegerParameter("VectorSize")==n,"Native dynamic VectorSize differs");
      for(int repeat=0;repeat<64;++repeat){Rvector churn(256);for(int j=0;j<256;++j)churn[j]=repeat+j;same(actual.EvaluateRvector(),seed(n,10));}
      Parameter *polymorphic=&actual;const auto update=seed(n,20);polymorphic->SetRvector(update);same(owner.GetRvectorParameter(key),update);same(actual.EvaluateRvector(),update);
      ParameterWrapper wrapper;wrapper.SetDescription("NativeOwner."+key);require(wrapper.SetRefObject(&actual),"Native ParameterWrapper reference failed");const auto wrapped=seed(n,30);require(wrapper.SetRvector(wrapped),"Native ParameterWrapper vector setter failed");same(owner.GetRvectorParameter(key),wrapped);same(wrapper.EvaluateRvector(),wrapped);
      const auto resized=seed(n==4?1:4,40);polymorphic->SetRvector(resized);same(owner.GetRvectorParameter(key),resized);same(actual.EvaluateRvector(),resized);require(actual.GetIntegerParameter("VectorSize")==resized.GetSize(),"Native setter resize metadata differs");polymorphic->SetRvector(wrapped);
      T copied(actual);same(copied.EvaluateRvector(),wrapped);T assigned("OtherOwner."+key,&other);assigned=actual;same(assigned.EvaluateRvector(),wrapped);same(other.GetRvectorParameter(key),seed(n,90));assigned=assigned;same(assigned.EvaluateRvector(),wrapped);
      rows.append(QJsonObject{{"type",QString::fromStdString(key)},{"length",n},{"before",array(seed(n,10))},{"polymorphic",array(update)},{"wrapper",array(wrapped)},{"copied",array(copied.EvaluateRvector())},{"assigned",array(assigned.EvaluateRvector())},{"other",array(other.GetRvectorParameter(key))},{"resized",array(resized)},{"repeated_reads",64},{"owner_reference",true}});
   }
   return rows;
}

int main(int argc,char **argv)
{
   try
   {
      require(argc==4,"Supply startup,type and Binding/Full mode");FileManager::Instance()->ReadStartupFile(argv[1]);const std::string key=argv[2];const bool bindingOnly=std::string(argv[3])=="Binding";require(bindingOnly||std::string(argv[3])=="Full","Unknown native mode");require(key=="ThrustAngle1"||key=="ThrustAngle2","Unknown native angle type");
      const auto rows=key=="ThrustAngle1"?exercise<ThrustAngle1>(key,bindingOnly):exercise<ThrustAngle2>(key,bindingOnly);const QJsonObject proof{{"checkpoint","thrust-angle-native-parameter-protocol"},{"outcome","passed"},{"type",QString::fromStdString(key)},{"binding_only",bindingOnly},{"rows",rows}};save("proof.json",proof);std::cout<<"NASA_CHECK "<<QJsonDocument(proof).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
   }
   catch(BaseException &e){std::cerr<<"FAIL "<<e.GetFullMessage()<<std::endl;}catch(std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;}return 1;
}
