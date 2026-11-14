#include "TimeSystemConverter.hpp"
#include "LeapSecsFileReader.hpp"
#include "GmatTime.hpp"
#include "BaseException.hpp"
extern "C" {
#include "SpiceUsr.h"
}
#include <QCoreApplication>
#include <QFileInfo>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool value,const char *message) { if(!value)throw std::runtime_error(message); }
void spiceCheck()
{
   if(!failed_c())return;
   char message[1841];getmsg_c("LONG",sizeof(message),message);reset_c();throw std::runtime_error(message);
}
std::string convert(TimeSystemConverter &converter,bool precise,const std::string &from,
                    const std::string &input,const std::string &to,bool *flag,Real mjd=-999.999)
{
   std::string text;
   if(precise) { GmatTime output;converter.Convert(from,GmatTime(mjd),input,to,output,text,1,flag); }
   else { Real output;converter.Convert(from,mjd,input,to,output,text,1,flag); }
   return text;
}
struct Date { const char *leap;const char *next; };
const Date dates[]={{"31 Dec 2008","01 Jan 2009"},{"30 Jun 2012","01 Jul 2012"},
                    {"30 Jun 2015","01 Jul 2015"},{"31 Dec 2016","01 Jan 2017"}};
void resetFlag(TimeSystemConverter &converter)
{
   int checked=0;
   for(const auto &date:dates)for(bool precise:{false,true}) {
      bool flag=false;
      convert(converter,precise,"UTCGregorian",std::string(date.leap)+" 23:59:60.125","TAIModJulian",&flag);
      require(flag,"Positive UTC input did not set the leap flag");
      const std::string normal=std::string(date.next)+" 00:00:00.125";
      const auto text=convert(converter,precise,"UTCGregorian",normal,"UTCGregorian",&flag);
      if(flag||text!=normal)std::cerr<<"STALE_LEAP_FLAG precise="<<precise<<" expected="<<normal<<" actual="<<text<<" flag="<<flag<<'\n';
      require(!flag&&text==normal,"Normal UTC identity conversion retained prior leap state");++checked;
   }
   require(checked==8,"Leap flag reset domain incomplete");
   std::cout<<"PASS leap-state reset: "<<checked<<" sequential leap/normal conversions\n";
}
void implicitFlag(TimeSystemConverter &converter)
{
   int checked=0;
   for(const auto &date:dates)for(const char *fraction:{"000","001","125","999"}) {
      const std::string expected=std::string(date.leap)+" 23:59:60."+fraction;
      SpiceDouble et;str2et_c((expected+" UTC").c_str(),&et);spiceCheck();
      for(const std::string system:{"TAI","A1","TT","TDB"}) {
         const auto uniform=system=="TDB"?et:unitim_c(et,"TDB",system=="A1"?"TAI":system.c_str());spiceCheck();
         const Real mjd=static_cast<Real>(21545.L+(static_cast<long double>(uniform)+(system=="A1"?.0343817L:0.L))/86400.L);
         for(bool precise:{false,true})for(int mode:{0,1,2}) {
            bool flag=mode==2;const auto text=convert(converter,precise,system+"ModJulian","","UTCGregorian",mode?&flag:nullptr,mjd);
            if(text!=expected)std::cerr<<"IMPLICIT_LEAP_LABEL source="<<system<<" precise="<<precise<<" flag_mode="<<mode<<" expected="<<expected<<" actual="<<text<<'\n';
            require(text==expected,"UTC leap label depends on optional output flag");
            require(!mode||flag,"Uniform instant inside UTC leap did not set output flag");++checked;
         }
      }
   }
   require(checked==384,"Implicit leap-label domain incomplete");
   std::cout<<"PASS leap-state implicit: "<<checked<<" NAIF-derived uniform inputs with null/false/true output flags\n";
}
void uniformNoTable(TimeSystemConverter &converter)
{
   // Uniform-to-uniform conversions without a leap flag must not acquire a
   // dependency on a leap table merely to format a non-UTC result.
   converter.SetLeapSecsFileReader(nullptr);int checked=0;
   for(bool precise:{false,true}) {
      const auto same=convert(converter,precise,"TAIGregorian","01 Jan 2000 12:00:00.000","TAIGregorian",nullptr);
      require(same=="01 Jan 2000 12:00:00.000","Uniform identity formatting needs a leap table");++checked;
      const auto tt=convert(converter,precise,"TAIGregorian","01 Jan 2000 12:00:00.000","TTGregorian",nullptr);
      require(tt=="01 Jan 2000 12:00:32.184","Uniform conversion changed or needs a leap table");++checked;
   }
   require(checked==4,"Uniform no-table domain incomplete");
   std::cout<<"PASS leap-state uniform-no-table: "<<checked<<" uniform formatting checks\n";
}
void identityFlag(TimeSystemConverter &converter)
{
   int checked=0;
   for(const auto &date:dates)for(bool inLeap:{false,true}) {
      const std::string utc=inLeap?std::string(date.leap)+" 23:59:60.125":std::string(date.next)+" 00:00:00.125";
      SpiceDouble et;str2et_c((utc+" UTC").c_str(),&et);spiceCheck();
      for(const std::string system:{"TAI","A1","TT","TDB"}) {
         const auto seconds=system=="TDB"?et:unitim_c(et,"TDB",system=="A1"?"TAI":system.c_str());spiceCheck();
         const Real input=static_cast<Real>(21545.L+(static_cast<long double>(seconds)+(system=="A1"?.0343817L:0.L))/86400.L);
         for(bool precise:{false,true})for(bool initial:{false,true}) {
            bool flag=initial;std::string text;
            if(precise) {
               const GmatTime value(input);GmatTime output;
               converter.Convert(system+"ModJulian",value,"",system+"ModJulian",output,text,1,&flag);
               require(output==value,"Uniform identity changed the precise numeric epoch");
            } else {
               Real output;converter.Convert(system+"ModJulian",input,"",system+"ModJulian",output,text,1,&flag);
               require(output==input,"Uniform identity changed the numeric epoch");
            }
            require(flag==inLeap,"Uniform identity flag retained caller state or lost leap truth");++checked;
         }
      }
   }
   require(checked==128,"Uniform identity flag domain incomplete");
   std::cout<<"PASS leap-state identity: "<<checked<<" NAIF-derived instants, unchanged numeric epochs\n";
}
}
int main(int argc,char **argv)
{
   QCoreApplication app(argc,argv);
   try {
      require(argc==4,"Supply leap table, NAIF LSK and leap-state scenario");
      LeapSecsFileReader leap(QFileInfo(argv[1]).absoluteFilePath().toStdString());require(leap.Initialize(),"Leap table initialization failed");
      auto &converter=*TimeSystemConverter::Instance();converter.SetLeapSecsFileReader(&leap);
      furnsh_c(QFileInfo(argv[2]).absoluteFilePath().toUtf8().constData());spiceCheck();
      const std::string mode=argv[3];
      if(mode=="ResetFlag")resetFlag(converter);
      else if(mode=="ImplicitFlag")implicitFlag(converter);
      else if(mode=="UniformNoTable")uniformNoTable(converter);
      else if(mode=="IdentityFlag")identityFlag(converter);
      else throw std::runtime_error("Unknown leap-state scenario");
      return 0;
   } catch(BaseException &e) { std::cerr<<"FAIL "<<e.GetFullMessage()<<'\n'; }
   catch(std::exception &e) { std::cerr<<"FAIL "<<e.what()<<'\n'; }
   return 1;
}
