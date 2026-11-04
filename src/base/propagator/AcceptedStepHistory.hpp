#ifndef AcceptedStepHistory_hpp
#define AcceptedStepHistory_hpp

#include "gmatdefs.hpp"
#include <string>
#include <vector>
class PhysicalModel;
class Propagator;

// Numerical steppers record accepted primitive endpoints only while a caller
// explicitly opens a transaction. Stopping-condition probes remain private
// until their caller commits; destroying a transaction discards its records.
// This does not publish, modify a spacecraft, or change an integration step.
class GMAT_API AcceptedStepHistory
{
public:
   struct Step
   {
      const PhysicalModel *model;
      const Propagator *integrator;
      std::string method;
      Real startSeconds;
      Real endSeconds;
      RealArray state; // Native force-origin integration coordinates, not J2000.
   };
   class GMAT_API Transaction
   {
   public:
      explicit Transaction(std::vector<Step> &destination);
      Transaction(); // A nested trial; Commit appends to its parent's staging.
      ~Transaction();
      Transaction(const Transaction &) = delete;
      Transaction &operator=(const Transaction &) = delete;
      const std::vector<Step> &Steps() const { return staged; }
      void Commit();
   private:
      friend class AcceptedStepHistory;
      Transaction *previous;
      std::vector<Step> *destination;
      std::vector<Step> staged;
      bool committed;
   };
   static bool IsCapturing();
   static void Record(const Propagator *integrator, const PhysicalModel *model,
      Real startSeconds, Real endSeconds, const Real *state, Integer dimension);
private:
   static thread_local Transaction *active;
};
#endif
