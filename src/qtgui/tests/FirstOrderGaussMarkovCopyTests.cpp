#include "FirstOrderGaussMarkov.hpp"
#include "EstimatedParameter.hpp"
#include "BaseException.hpp"
#include <iostream>
#include <stdexcept>
static void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
int main(){
 try{
  FirstOrderGaussMarkov source("Source"),target("Destination"),peer("Independent");
  source.SetRealParameter("SteadyStateValue",1.25);source.SetRealParameter("SteadyStateSigma",.35);source.SetRealParameter("HalfLife",900);source.SetStringParameter("SolveFor","AtmosDensityScaleFactor");source.SetConversionFactor(7);
  const Real acceleration[3]={1e-3,-2e-3,4e-4};GmatBase *destination=&target;destination->Copy(&source);peer.Copy(&source);int components=0;
  auto same=[&](const Rmatrix &a,const Rmatrix &b){require(a.GetNumRows()==7&&a.GetNumColumns()==7&&b.GetNumRows()==7&&b.GetNumColumns()==7,"Copied model covariance shape differs");for(int i=0;i<7;++i)for(int j=0;j<7;++j){require(a(i,j)==b(i,j),"Copied model changes process-noise response");++components;}};
  for(double seconds:{.1,600.}){const auto expected=source.GetProcessNoise(seconds,acceleration);same(expected,target.GetProcessNoise(seconds,acceleration));same(expected,peer.GetProcessNoise(seconds,acceleration));}
  require(target.GetName()=="Destination"&&peer.GetName()=="Independent"&&target.GetStringParameter("SolveFor")=="AtmosDensityScaleFactor","Copy lost destination name or inherited solve-for");
  const auto retained=target.GetProcessNoise(600,acceleration);source.SetRealParameter("SteadyStateSigma",.07);same(retained,target.GetProcessNoise(600,acceleration));require(source.GetProcessNoise(600,acceleration)(6,6)!=retained(6,6),"Copy independence witness is not distinguishable");target.Copy(&target);same(retained,target.GetProcessNoise(600,acceleration));
  EstimatedParameter other("Other");int rejected=0;for(const GmatBase *invalid:{static_cast<const GmatBase*>(nullptr),static_cast<const GmatBase*>(&other)}){bool threw=false;try{destination->Copy(invalid);}catch(BaseException&){threw=true;}require(threw&&target.GetName()=="Destination","Copy accepted null/incompatible source or changed identity");same(retained,target.GetProcessNoise(600,acceleration));++rejected;}
  require(components==392&&rejected==2,"Copy contract cardinality differs");std::cout<<"PASS FOGM polymorphic Copy: "<<components<<" exact covariance-component comparisons;two invalid-source rejections;independent copies,self-copy,destination identity and inherited conversion/solve-for state preserved. No covariance-physics qualification.\n";return 0;
 }catch(BaseException&e){std::cerr<<"FAIL "<<e.GetFullMessage()<<'\n';}catch(std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';}return 1;
}
