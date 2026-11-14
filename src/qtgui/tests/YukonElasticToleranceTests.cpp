#include "Yukon.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

class IntervalProblem final : public YukonUserProblem
{
public:
   explicit IntervalProblem(Real upper) : upper(upper) {}
   void GetNLPInfo(Integer &n, Integer &m) override { n = 1; m = 2; }
   Rvector GetStartingPoint() override { return Rvector(1, 0.5); }
   void GetBoundsInfo(Integer, Integer, Rvector &low, Rvector &high,
      Rvector &clow, Rvector &chigh) override
   {
      const Real inf = std::numeric_limits<Real>::infinity();
      low = Rvector(1, -10.0); high = Rvector(1, 10.0);
      clow = Rvector(2, 1.0, -inf); chigh = Rvector(2, inf, upper);
   }
   Real EvaluateCostFunc(Integer, Rvector x, bool) override
      { return (x[0] - 2) * (x[0] - 2); }
   Rvector EvaluateCostJac(Integer, Rvector x, bool) override
      { return Rvector(1, 2 * (x[0] - 2)); }
   Rvector EvaluateConFunc(Integer, Rvector x, bool) override
      { return Rvector(2, x[0], x[0]); }
   Rmatrix EvaluateConJac(Integer, Rvector, bool) override
      { Rmatrix a(2, 1); a(0, 0) = a(1, 0) = 1; return a; }
   void EvaluateConJacDimensions(Integer, Rvector, bool,
      Integer &rows, Integer &cols) override { rows = 2; cols = 1; }
   std::vector<Real> GetMaxVarStepSize() override { return {10.0}; }
private:
   Real upper;
};

int check(const std::string &key)
{
   const Real tolerance = 1e-12;
   const Real upper = key == "feasible" ? 1 + 1e-6 :
      key == "infeasible-wide" ? 1 - 1e-5 :
      key == "infeasible-tight" ? 1 - 1e-11 :
      std::numeric_limits<Real>::quiet_NaN();
   if (!std::isfinite(upper)) return 2;
   const bool feasibleWithinTolerance = 1 - tolerance <= upper + tolerance;
   try
   {
      IntervalProblem problem(upper);
      Yukon solver(&problem, "SelfScaledBFGS", 100, 1000,
         tolerance, tolerance, tolerance, 10000);
      solver.PrepareToOptimize(); solver.PrepareLineSearch();
      Rvector answer; Integer state = 0, functions = 0, iteration = 0;
      bool changed = false; int ticks = 0;
      do
      {
         solver.RespondToData();
         solver.CheckStatus(state, functions, iteration, answer, changed, &problem);
         if (++ticks > 10000) throw std::runtime_error("State watchdog expired");
      } while (state <= 0);
      Real cost; Integer flag; OutputData output{};
      solver.PrepareOutput(answer, cost, flag, output);
      const Real x = answer.GetSize() ? answer[0] :
         std::numeric_limits<Real>::quiet_NaN();
      const Real violation = std::isfinite(x) ?
         std::max({Real(0), 1 - x, x - upper}) :
         std::numeric_limits<Real>::infinity();
      std::cout << std::setprecision(17)
         << "ELASTIC_TOLERANCE case=" << key << " lower=1 upper=" << upper
         << " tolerance=" << tolerance << " feasible_within_tolerance="
         << feasibleWithinTolerance << " state=" << state << " flag=" << flag
         << " x=" << x << " cost=" << cost << " original_violation="
         << violation << " transitions=" << ticks << std::endl;
      if (!feasibleWithinTolerance && state == 1)
         throw std::runtime_error("Solver reported success for original bounds with no common point within the requested tolerance");
      if (feasibleWithinTolerance &&
         (state != 1 || answer.GetSize() != 1 || !std::isfinite(x) ||
          !std::isfinite(cost) || violation > tolerance ||
          std::abs(x - upper) > 1e-8))
         throw std::runtime_error("Feasible interval result differs from the independent optimum or original-bound tolerance");
      if (!feasibleWithinTolerance && (state != 6 || flag != -1))
         throw std::runtime_error("Contradictory original bounds did not produce the native infeasible state and flag");
      std::cout << "PASS ELASTIC_TOLERANCE " << key << std::endl;
      return 0;
   }
   catch (BaseException &error) { std::cerr << error.GetFullMessage() << std::endl; }
   catch (std::exception &error) { std::cerr << error.what() << std::endl; }
   return 1;
}

int main(int argc, char **argv)
{
   if (argc != 2) return 2;
   const std::string key=argv[1];
   const int status=check(key);
   if (status || key=="feasible") return status;
   // A failed problem must not contaminate a subsequent independent solver.
   return check("feasible");
}
