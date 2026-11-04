#include "MinQP.hpp"
#include "NLPFunctionGenerator.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

static void require(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
// Elastic mode is also used for unconstrained optimization. Its empty set of
// user constraints has zero violation and no elastic variables, even when
// decision-variable bounds add rows to the internal constraint vector.
class UnconstrainedProblem final : public YukonUserProblem
{
public:
 explicit UnconstrainedProblem(bool bounded): bounded(bounded) {}
 void GetNLPInfo(Integer &variables,Integer &constraints) override { variables=1;constraints=0; }
 Rvector GetStartingPoint() override { return Rvector(1,1.0); }
 void GetBoundsInfo(Integer,Integer,Rvector &lower,Rvector &upper,Rvector &cl,Rvector &cu) override {
  const auto inf=std::numeric_limits<Real>::infinity();
  lower=Rvector(1,bounded ? 0.0 : -inf);upper=Rvector(1,bounded ? 10.0 : inf);cl.SetSize(0);cu.SetSize(0);
 }
 Real EvaluateCostFunc(Integer,Rvector x,bool) override { return (x[0]-2)*(x[0]-2); }
 Rvector EvaluateCostJac(Integer,Rvector x,bool) override { return Rvector(1,2*(x[0]-2)); }
 Rvector EvaluateConFunc(Integer,Rvector,bool) override { return Rvector(0); }
 Rmatrix EvaluateConJac(Integer,Rvector,bool) override { return Rmatrix(0,1); }
 void EvaluateConJacDimensions(Integer,Rvector,bool,Integer &rows,Integer &cols) override { rows=0;cols=1; }
 std::vector<Real> GetMaxVarStepSize() override { return {10.0}; }
private:
 bool bounded;
};
static void checkEmptyElasticConstraints()
{
 for (bool bounded:{false,true}) {
  UnconstrainedProblem problem(bounded); NLPFunctionGenerator functions(&problem,true,1.0);
  const Rvector x(1,42.0),violations(bounded ? 1 : 0);
  require(functions.MaxUserConViolation(violations,x)==0,"Empty user constraints have nonzero violation");
  require(functions.GetMaxElasticVar(x)==0,"Empty elastic set read a decision variable");
  require(functions.ShiftCost(1600.0,x)==1600.0,"Empty elastic set changed the objective");
  std::cout<<"PASS elastic mode without user constraints, bounded="<<bounded<<std::endl;
 }
}
static void check(const char *name,const Rvector &start,const Rmatrix &matrix,
                  const Rvector &lower,const Rvector &upper,const Rvector &expected,
                  const Rvector &gradient)
{
 const int n=start.GetSize(); Rmatrix hessian(n,n); for (int i=0;i<n;++i) hessian(i,i)=1;
 Rvector active(0),answer,multipliers,activeOut; Real cost; Integer flag,iterations;
 MinQP qp(start,hessian,gradient,matrix,lower,upper,active,2,false);
 qp.Optimize(answer,cost,multipliers,flag,iterations,activeOut);
 require(flag==1,"QP did not report a solution"); require(answer.GetSize()==n,"QP result dimension differs");
 for (int i=0;i<n;++i) require(std::isfinite(answer[i])&&std::abs(answer[i]-expected[i])<1e-7,"QP differs from independent optimum");
 for (int row=0;row<matrix.GetNumRows();++row) { Real value=0; for (int col=0;col<n;++col) value+=matrix(row,col)*answer[col]; require(value>=lower[row]-1e-7&&value<=upper[row]+1e-7,"Successful QP result violates an original bound"); }
 std::cout<<"PASS QP "<<name<<" variables"; for (int i=0;i<n;++i) std::cout<<' '<<answer[i]; std::cout<<std::endl;
}
int main()
{
 try {
  checkEmptyElasticConstraints();
  const Real inf=std::numeric_limits<Real>::infinity(); Rmatrix one(1,1);one(0,0)=1;
  check("lower",Rvector(1,0.0),one,Rvector(1,1.0),Rvector(1,inf),Rvector(1,1.0),Rvector(1,0.0));
  check("upper",Rvector(1,0.0),one,Rvector(1,-inf),Rvector(1,-1.0),Rvector(1,-1.0),Rvector(1,0.0));
  check("two-sided-below",Rvector(1,0.0),one,Rvector(1,2.0),Rvector(1,3.0),Rvector(1,2.0),Rvector(1,0.0));
  check("two-sided-above",Rvector(1,0.0),one,Rvector(1,-3.0),Rvector(1,-2.0),Rvector(1,-2.0),Rvector(1,0.0));
  Rmatrix nonlinear(3,1);nonlinear(0,0)=4;nonlinear(1,0)=6;nonlinear(2,0)=1;
  check("nonlinear-linearization",Rvector(1,0.0),nonlinear,Rvector(3,-4.0,-inf,-12.0),Rvector(3,inf,-5.0,8.0),Rvector(1,-5.0/6),Rvector(1,1e-6));
  Rmatrix independent(2,2);independent(0,0)=1;independent(1,1)=1;
  check("narrow-bound-large-slack",Rvector(2,2.0,0.0),independent,Rvector(2,0.0,100.0),Rvector(2,1.0,inf),Rvector(2,0.0,100.0),Rvector(2,0.0,0.0));
  Rmatrix equality(3,2);equality(0,0)=1;equality(0,1)=1;equality(1,0)=1;equality(2,1)=1;
  check("equality-and-bounds",Rvector(2,0.0,0.0),equality,Rvector(3,3.0,0.0,0.0),Rvector(3,3.0,inf,inf),Rvector(2,1.5,1.5),Rvector(2,0.0,0.0));
  return 0;
 } catch (std::exception &e) { std::cerr<<"FAIL QP "<<e.what()<<std::endl; } catch (BaseException &e) { std::cerr<<"FAIL QP "<<e.GetFullMessage()<<std::endl; } return 1;
}
