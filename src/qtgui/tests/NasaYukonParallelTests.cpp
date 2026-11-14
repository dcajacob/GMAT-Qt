#include "Yukon.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
// Direct analytic callbacks bypass GUI,command wrappers and finite differences.
// Every inequality is independently checked in its original,unmerged form.
class AnalyticProblem final : public YukonUserProblem
{
public:
 explicit AnalyticProblem(const std::string &kind):kind(kind) {}
 void GetNLPInfo(Integer &vars,Integer &cons) override {vars=1;cons=kind=="Single"?1:2;}
 Rvector GetStartingPoint() override {return Rvector(1,1.0);}
 void GetBoundsInfo(Integer,Integer,Rvector &low,Rvector &high,Rvector &clow,Rvector &chigh) override {
  const int cons=kind=="Single"?1:2;
  low=Rvector(1,0.0);high=Rvector(1,10.0);clow.SetSize(cons);chigh.SetSize(cons);
  for(int i=0;i<cons;++i)chigh[i]=std::numeric_limits<Real>::infinity();
  clow[0]=kind=="ReverseLower"||kind=="EqualLower"?4:kind=="OffsetParallel"?0:kind=="NonlinearInitialParallel"?25:3;
  if(cons==2)clow[1]=kind=="ReverseLower"?3:kind=="OffsetParallel"?0:kind=="NonlinearInitialParallel"?8:4;
 }
 Real EvaluateCostFunc(Integer,Rvector x,bool) override {return (x[0]-2)*(x[0]-2);}
 Rvector EvaluateCostJac(Integer,Rvector x,bool) override {return Rvector(1,2*(x[0]-2));}
 Rvector EvaluateConFunc(Integer,Rvector x,bool) override {
  Rvector c(kind=="Single"?1:2);c[0]=kind=="OffsetParallel"?x[0]-3:kind=="NonlinearInitialParallel"?x[0]*x[0]:x[0];
  if(c.GetSize()==2)c[1]=kind=="OffsetParallel"?x[0]-4:kind=="NonlinearInitialParallel"?2*x[0]:x[0];return c;
 }
 Rmatrix EvaluateConJac(Integer,Rvector x,bool) override {Rmatrix a(kind=="Single"?1:2,1);a(0,0)=kind=="NonlinearInitialParallel"?2*x[0]:1;if(a.GetNumRows()==2)a(1,0)=kind=="NonlinearInitialParallel"?2:1;return a;}
 void EvaluateConJacDimensions(Integer,Rvector,bool,Integer &rows,Integer &cols) override {rows=kind=="Single"?1:2;cols=1;}
 std::vector<Real> GetMaxVarStepSize() override {return {10.0};}
 double expected()const{return kind=="Single"?3:kind=="NonlinearInitialParallel"?5:4;}
private:
 std::string kind;
};

// Upper/mixed/scaled rows independently exercise multiplier sign and original
// bound identity. The earlier problem definitions and assertions stay intact.
class BoundProblem final : public YukonUserProblem
{
public:
 explicit BoundProblem(const std::string &key):key(key) {}
 bool upper()const{return key=="UpperDuplicate"||key=="UpperReverse";}
 Real center()const{return upper()||key=="MixedUpper"?8:2;}
 Real expected()const{return upper()?6:key=="MixedLower"?3:4;}
 void GetNLPInfo(Integer &n,Integer &m)override{n=1;m=2;}
 Rvector GetStartingPoint()override{return Rvector(1,1.0);}
 void GetBoundsInfo(Integer,Integer,Rvector &l,Rvector &u,Rvector &cl,Rvector &cu)override{
  l=Rvector(1,0.0);u=Rvector(1,10.0);cl.SetSize(2);cu.SetSize(2);const Real inf=std::numeric_limits<Real>::infinity();
  if(upper()){cl[0]=cl[1]=-inf;cu[0]=key=="UpperReverse"?6:7;cu[1]=key=="UpperReverse"?7:6;}
  else if(key=="ScaledLower"){cl[0]=6;cl[1]=4;cu[0]=cu[1]=inf;}
  else{cl[0]=3;cu[0]=inf;cl[1]=-inf;cu[1]=4;}
 }
 Real EvaluateCostFunc(Integer,Rvector x,bool)override{return (x[0]-center())*(x[0]-center());}
 Rvector EvaluateCostJac(Integer,Rvector x,bool)override{return Rvector(1,2*(x[0]-center()));}
 Rvector EvaluateConFunc(Integer,Rvector x,bool)override{return Rvector(2,key=="ScaledLower"?2*x[0]:x[0],x[0]);}
 Rmatrix EvaluateConJac(Integer,Rvector,bool)override{Rmatrix a(2,1);a(0,0)=key=="ScaledLower"?2:1;a(1,0)=1;return a;}
 void EvaluateConJacDimensions(Integer,Rvector,bool,Integer &r,Integer &c)override{r=2;c=1;}
 std::vector<Real> GetMaxVarStepSize()override{return {10.0};}
private:std::string key;
};
static int checkBoundProblem(const std::string &key)
{
 BoundProblem problem(key);Yukon solver(&problem,"SelfScaledBFGS",100,1000,1e-8,1e-8,1e-8,10000);solver.PrepareToOptimize();solver.PrepareLineSearch();Rvector answer;Integer state=0,fun=0,iter=0;bool changed=false;int ticks=0;
 do{solver.RespondToData();solver.CheckStatus(state,fun,iter,answer,changed,&problem);if(++ticks>10000)throw std::runtime_error("Bound-problem state watchdog expired");}while(state<=0);
 Real cost;Integer flag;OutputData info{};solver.PrepareOutput(answer,cost,flag,info);const Real x=answer.GetSize()?answer[0]:std::numeric_limits<Real>::quiet_NaN(),want=problem.expected(),expectedCost=(want-problem.center())*(want-problem.center());
 std::cout.precision(17);std::cout<<"YUKON_BOUND case="<<key<<" x="<<x<<" cost="<<cost<<" expected_x="<<want<<" expected_cost="<<expectedCost<<" state="<<state<<" flag="<<flag<<std::endl;
 if(state!=1||answer.GetSize()!=1||!std::isfinite(x)||!std::isfinite(cost)||std::abs(x-want)>2e-6||std::abs(cost-expectedCost)>1e-5)throw std::runtime_error("Bound-problem result differs from independent interval optimum");
 Rvector l,u,cl,cu;problem.GetBoundsInfo(0,0,l,u,cl,cu);const auto values=problem.EvaluateConFunc(1,answer,true);for(int i=0;i<2;++i)if(!std::isfinite(values[i])||values[i]<cl[i]-1e-7||values[i]>cu[i]+1e-7)throw std::runtime_error("Bound-problem violates an original constraint");
 std::cout<<"PASS YUKON_BOUND "<<key<<std::endl;return 0;
}


// A QP can reach a bound while MaxStep truncates the accepted NLP step inside
// that bound. Stationarity alone then gives a false optimum without dual
// complementarity. Analytic callbacks isolate this from finite differences.
class VariableBoundSteps final : public YukonUserProblem
{
public:
 explicit VariableBoundSteps(bool lower):lower(lower) {}
 void GetNLPInfo(Integer &n,Integer &m) override {n=1;m=0;}
 Rvector GetStartingPoint() override {return Rvector(1,lower?5.:1.);}
 void GetBoundsInfo(Integer,Integer,Rvector &l,Rvector &u,Rvector &cl,Rvector &cu) override {
  l=Rvector(1,lower?4.:-100.);u=Rvector(1,lower?100.:2.);cl.SetSize(0);cu.SetSize(0);
 }
 Real EvaluateCostFunc(Integer,Rvector x,bool) override {return (x[0]-3)*(x[0]-3);}
 Rvector EvaluateCostJac(Integer,Rvector x,bool) override {return Rvector(1,2*(x[0]-3));}
 Rvector EvaluateConFunc(Integer,Rvector,bool) override {Rvector r;r.SetSize(0);return r;}
 Rmatrix EvaluateConJac(Integer,Rvector,bool) override {Rmatrix r;r.SetSize(0,1);return r;}
 void EvaluateConJacDimensions(Integer,Rvector,bool,Integer &r,Integer &c) override {r=0;c=1;}
 std::vector<Real> GetMaxVarStepSize() override {return {0.5};}
private: bool lower;
};
static int checkVariableBoundSteps(bool lower)
{
 VariableBoundSteps problem(lower);Yukon solver(&problem,"SelfScaledBFGS",100,1000,1e-10,1e-10,1e-10,10000);
 solver.PrepareToOptimize();solver.PrepareLineSearch();Rvector answer;Integer state=0,fun=0,iter=0;bool changed=false;int ticks=0;
 do {solver.RespondToData();solver.CheckStatus(state,fun,iter,answer,changed,&problem);if(++ticks>10000)throw std::runtime_error("Variable bound watchdog expired");}while(state<=0);
 Real cost;Integer flag;OutputData info{};solver.PrepareOutput(answer,cost,flag,info);const Real expected=lower?4:2;
 std::cout.precision(17);std::cout<<"YUKON_VARIABLE_BOUND lower="<<lower<<" x="<<answer[0]<<" cost="<<cost<<" state="<<state<<" flag="<<flag<<std::endl;
 if(state!=1||answer.GetSize()!=1||!std::isfinite(answer[0])||!std::isfinite(cost)||std::abs(answer[0]-expected)>1e-8||std::abs(cost-1)>1e-8)
  throw std::runtime_error("MaxStep-truncated bound problem falsely converged before analytic bound optimum");
 std::cout<<"PASS YUKON_VARIABLE_BOUND"<<std::endl;return 0;
}

int main(int argc,char **argv)
{
 const std::vector<std::string> names{"Single","DuplicateLower","ReverseLower","EqualLower","OffsetParallel","NonlinearInitialParallel","ElasticSlackEndpoints","UpperDuplicate","UpperReverse","MixedLower","MixedUpper","ScaledLower","UpperBoundSteps","LowerBoundSteps"};
 if(argc!=2||std::find(names.begin(),names.end(),argv[1])==names.end()){std::cerr<<"Supply analytic Yukon problem key"<<std::endl;return 2;}
 const std::string kind=argv[1];
 try{
  if(kind=="UpperBoundSteps"||kind=="LowerBoundSteps")return checkVariableBoundSteps(kind=="LowerBoundSteps");
  if(kind=="UpperDuplicate"||kind=="UpperReverse"||kind=="MixedLower"||kind=="MixedUpper"||kind=="ScaledLower")return checkBoundProblem(kind);
  if(kind=="ElasticSlackEndpoints"){
   AnalyticProblem problem("DuplicateLower");NLPFunctionGenerator functions(&problem,true,1);auto values=functions.GetNLPStartingPoint();
   if(values.GetSize()!=5)throw std::runtime_error("Elastic endpoint fixture dimension differs");
   for(int i=0;i<values.GetSize();++i)values[i]=0;
   values[4]=0.125;if(functions.GetMaxElasticVar(values)!=0.125)throw std::runtime_error("Last elastic slack omitted from maximum");
   values[4]=-0.25;values[1]=0.075;if(functions.GetMaxElasticVar(values)!=0.25)throw std::runtime_error("Elastic slack maximum lost final component magnitude");
   values[4]=0;values[1]=0.5;if(functions.GetMaxElasticVar(values)!=0.5)throw std::runtime_error("First elastic slack maximum differs");
   std::cout<<"PASS YUKON_ELASTIC_SLACK first=0.5 last=0.125 magnitude=0.25"<<std::endl;return 0;
  }
  AnalyticProblem problem(kind);Yukon optimizer(&problem,"SelfScaledBFGS",100,1000,1e-8,1e-8,1e-8,10000);Rvector answer;Real cost=std::numeric_limits<Real>::quiet_NaN();Integer flag=0;OutputData info{};// Match the public transition protocol used by Yukonad::CheckCompletion.
  optimizer.PrepareToOptimize();optimizer.PrepareLineSearch();Integer state=0,funTypes=0,iterations=0;bool isNew=false;int transitions=0;
  do {optimizer.RespondToData();optimizer.CheckStatus(state,funTypes,iterations,answer,isNew,&problem);if(++transitions>10000)throw std::runtime_error("Direct Yukon state-machine watchdog expired");}while(state<=0);
  optimizer.PrepareOutput(answer,cost,flag,info);
  if(state!=1){std::cerr<<"YUKON_STATE case="<<kind<<" state="<<state<<" flag="<<flag<<" cost="<<cost<<" iterations="<<info.iter<<" x="<<(answer.GetSize()?answer[0]:0)<<std::endl;throw std::runtime_error("Direct Yukon state machine did not finish successfully");}
  const double x=answer.GetSize()==1?answer[0]:std::numeric_limits<Real>::quiet_NaN();
  std::cout.precision(17);std::cout<<"YUKON_ANALYTIC case="<<kind<<" x="<<x<<" cost="<<cost<<" expected_x="<<problem.expected()<<" expected_cost="<<(problem.expected()-2)*(problem.expected()-2)<<" flag="<<flag<<" iterations="<<info.iter<<std::endl;
  if(answer.GetSize()!=1||!std::isfinite(x)||!std::isfinite(cost)||flag<1||flag>3||std::abs(x-problem.expected())>2e-6||std::abs(cost-(problem.expected()-2)*(problem.expected()-2))>1e-5)throw std::runtime_error("Direct Yukon result differs from independent original-constraint optimum");
  Integer vars,cons;problem.GetNLPInfo(vars,cons);Rvector low,high,clow,chigh;problem.GetBoundsInfo(vars,cons,low,high,clow,chigh);const auto values=problem.EvaluateConFunc(vars,answer,true);for(int i=0;i<cons;++i)if(!std::isfinite(values[i])||values[i]<clow[i]-1e-7||values[i]>chigh[i]+1e-7)throw std::runtime_error("Direct Yukon violates an original unmerged constraint");
  std::cout<<"PASS YUKON_ANALYTIC "<<kind<<std::endl;return 0;
 }catch(BaseException &error){std::cerr<<"FAIL YUKON_ANALYTIC "<<kind<<' '<<error.GetFullMessage()<<std::endl;}catch(std::exception &error){std::cerr<<"FAIL YUKON_ANALYTIC "<<kind<<' '<<error.what()<<std::endl;}return 1;
}
