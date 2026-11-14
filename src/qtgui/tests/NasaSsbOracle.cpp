// Standalone reference: no GMAT state, coordinate or clock helper is called.
// Execute in a child process so furnishing a reference kernel cannot change
// the application's SPICE kernel pool or mask an ignored source selection.
extern "C" {
#include "SpiceUsr.h"
}
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>
#include <array>
#include <vector>
#include <QRegularExpression>
#include <iostream>
#include <stdexcept>

static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
static void spice()
{
 if(!failed_c())return;
 char message[1841];getmsg_c("LONG",sizeof(message),message);reset_c();throw std::runtime_error(message);
}
// Independent product-form degree-nine interpolation and Rodrigues rotation.
// The data and ten-node stencil policy are explicit shared model inputs.
static QJsonObject orientation(const QString &path,double epoch,const SpiceDouble *earth)
{
 QFile file(path);require(file.open(QIODevice::ReadOnly),"Orientation table unreadable");std::vector<std::array<double,4>> rows;
 while(!file.atEnd()){const auto words=QString::fromUtf8(file.readLine()).trimmed().split(',');require(words.size()==4,"Orientation table row malformed");std::array<double,4> row;for(int i=0;i<4;++i){bool ok=false;row[i]=words[i].toDouble(&ok);require(ok&&std::isfinite(row[i]),"Orientation table number invalid");}require(rows.empty()||row[0]>rows.back()[0],"Orientation table epochs unordered");rows.push_back(row);}
 require(rows.size()>=10&&epoch>=rows.front()[0]&&epoch<=rows.back()[0],"Reference orientation epoch outside table");
 // ICRFFile's source-defined stencil uses the last binary-search midpoint.
 // Reproduce only that input selection; neither its interpolation nor matrix
 // routines are called. Retain the actual selected nodes in the reference.
 int lo=0,hi=rows.size()-1,mid=-1;while(lo<hi-1){mid=(lo+hi)/2;if(epoch>rows[mid][0])lo=mid;else hi=mid;}
 const int begin=std::max(0,std::min(mid-4,int(rows.size())-10));std::array<double,3> v{};QJsonArray nodes;
 for(int i=begin;i<begin+10;++i){long double weight=1;for(int j=begin;j<begin+10;++j)if(i!=j)weight*=(static_cast<long double>(epoch)-rows[j][0])/(rows[i][0]-rows[j][0]);for(int k=0;k<3;++k)v[k]+=weight*rows[i][k+1];nodes.append(QJsonArray{rows[i][0],rows[i][1],rows[i][2],rows[i][3]});}
 const double angle=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);require(angle>0,"Reference orientation vector degenerate");std::array<double,3>a{v[0]/angle,v[1]/angle,v[2]/angle};QJsonArray transformed,reverse;
 for(int offset:{0,3}){const std::array<double,3> x{earth[offset],earth[offset+1],earth[offset+2]},cross{a[1]*x[2]-a[2]*x[1],a[2]*x[0]-a[0]*x[2],a[0]*x[1]-a[1]*x[0]};const double dot=a[0]*x[0]+a[1]*x[1]+a[2]*x[2];for(int k=0;k<3;++k){const double value=x[k]*std::cos(angle)+cross[k]*std::sin(angle)+a[k]*dot*(1-std::cos(angle));transformed.append(value);reverse.append(-value);}}
 return {{"table_sha256",QString::fromLatin1(QCryptographicHash::hash([&]{file.seek(0);return file.readAll();}(),QCryptographicHash::Sha256).toHex())},{"shared_stencil_nodes",nodes},{"rotation_vector",QJsonArray{v[0],v[1],v[2]}},{"earth_from_ssb_mj2000eq",transformed},{"ssb_from_earth_mj2000eq",reverse}};
}
static QString hash(const QString &path)
{
 QFile file(path);require(file.open(QIODevice::ReadOnly),"Reference input unreadable");QCryptographicHash digest(QCryptographicHash::Sha256);require(digest.addData(&file),"Reference input hash failed");return digest.result().toHex();
}
int main(int argc,char **argv)
{
 QCoreApplication app(argc,argv);
 try{
  require(argc>=5,"Expected reference SPK, LSK, orientation table and A1 MJD epochs");
  const QString kernel=QFileInfo(argv[1]).absoluteFilePath(),lsk=QFileInfo(argv[2]).absoluteFilePath();
  erract_c("SET",0,const_cast<char *>("RETURN"));errprt_c("SET",0,const_cast<char *>("NONE"));
  kclear_c();spice();furnsh_c(lsk.toUtf8().constData());spice();furnsh_c(kernel.toUtf8().constData());spice();QJsonArray samples;
  for(int a=4;a<argc;++a){bool ok=false;const double epoch=QString::fromLocal8Bit(argv[a]).toDouble(&ok);require(ok&&std::isfinite(epoch),"Reference epoch is not finite");
   // GMAT's A1 MJD has JD origin2430000.0. A1-TAI is0.0343817s;
   // the resulting TAI seconds are relative to J2000 before NAIF conversion.
   const double tai=(epoch-21545.)*86400.-0.0343817,et=unitim_c(tai,"TAI","ET");spice();SpiceDouble earth[6],ssb[6],lightTime;
   spkezr_c("EARTH",et,"J2000","NONE","SOLAR SYSTEM BARYCENTER",earth,&lightTime);spice();
   spkezr_c("SOLAR SYSTEM BARYCENTER",et,"J2000","NONE","EARTH",ssb,&lightTime);spice();QJsonArray e,s;
   for(int i=0;i<6;++i){require(std::isfinite(earth[i])&&std::isfinite(ssb[i])&&std::abs(earth[i]+ssb[i])<1e-10,"Reference reciprocal geometric states disagree");e.append(earth[i]);s.append(ssb[i]);}
   samples.append(QJsonObject{{"a1_mjd",epoch},{"tai_seconds_j2000",tai},{"et",et},{"earth_from_ssb",e},{"ssb_from_earth",s},{"orientation",orientation(QFileInfo(argv[3]).absoluteFilePath(),epoch,earth)}});
  }
  const QJsonObject output{{"reference","isolated direct CSPICE, J2000, geometric NONE"},{"kernel",kernel},{"kernel_sha256",hash(kernel)},{"lsk",lsk},{"lsk_sha256",hash(lsk)},{"orientation_table",QFileInfo(argv[3]).absoluteFilePath()},{"orientation_table_sha256",hash(QFileInfo(argv[3]).absoluteFilePath())},{"a1_minus_tai_seconds",.0343817},{"samples",samples}};
  std::cout<<QJsonDocument(output).toJson(QJsonDocument::Compact).constData()<<std::endl;return 0;
 }catch(std::exception &e){std::cerr<<"REFERENCE FAIL "<<e.what()<<std::endl;}return 1;
}
