#include "AcceptedStepHistory.hpp"
#include "Propagator.hpp"
#include "PropagatorException.hpp"
#include <cmath>
#include <iterator>

namespace { thread_local AcceptedStepHistory::Transaction *active = nullptr; }

AcceptedStepHistory::Transaction::Transaction(std::vector<Step> &output) :
   previous(active), destination(&output), committed(false)
{
   if (previous)
      throw PropagatorException("A nested accepted-step transaction must stage into its parent.");
   active = this;
}

AcceptedStepHistory::Transaction::Transaction() :
   previous(active), destination(previous ? &previous->staged : nullptr), committed(false)
{
   if (!previous || previous->committed)
      throw PropagatorException("A nested accepted-step transaction requires an active parent.");
   active = this;
}

AcceptedStepHistory::Transaction::~Transaction()
{
   active = previous;
}

void AcceptedStepHistory::Transaction::Commit()
{
   if (active != this || committed)
      throw PropagatorException("Commit the active accepted-step transaction exactly once.");
   if (staged.size() > destination->max_size() - destination->size())
      throw PropagatorException("Accepted-step history exceeds its storage limit.");
   // Allocate before moving records. A failed allocation leaves the private
   // trial intact, and each Step's vector/string move uses the same allocator.
   destination->reserve(destination->size() + staged.size());
   destination->insert(destination->end(), std::make_move_iterator(staged.begin()),
                       std::make_move_iterator(staged.end()));
   staged.clear();
   committed = true;
}

bool AcceptedStepHistory::IsCapturing()
{
   return active && !active->committed;
}

void AcceptedStepHistory::Record(const Propagator *integrator,
   const PhysicalModel *model, Real startSeconds, Real endSeconds,
   const Real *state, Integer dimension)
{
   if (!IsCapturing()) return;
   if (!integrator || !model || !state || dimension <= 0 ||
       !std::isfinite(startSeconds) || !std::isfinite(endSeconds))
      throw PropagatorException("Accepted-step recording requires valid model, time and state inputs.");
   if (startSeconds == endSeconds) return; // No representable time advance.
   active->staged.push_back(Step{model, integrator, integrator->GetTypeName(),
      startSeconds, endSeconds, RealArray(state, state + dimension)});
}
