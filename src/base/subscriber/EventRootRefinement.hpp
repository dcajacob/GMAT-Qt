// SPDX-License-Identifier: Apache-2.0
#ifndef EventRootRefinement_hpp
#define EventRootRefinement_hpp

#include "gmatdefs.hpp"
#ifdef __USE_SPICE__
#include "SpiceInterface.hpp"
#include <functional>
#include <vector>

// Continuous geometry and time refinement shared by event locators.
namespace GmatEventRoot
{
using Function = std::function<Real(Real)>;
// Export the scalar entry point for the event plugin's region search.
GMAT_API Real Refine(const Function &value, Real a, Real b, Real tolerance);
void RefineWindow(SpiceCell &result, SpiceCell &confinement,
                  const std::vector<Function> &functions, Real tolerance);
void RefineOccultation(SpiceCell &result, SpiceCell &confinement,
      const std::string &type, const std::string &front,
      const std::string &frontShape, const std::string &frontFrame,
      const std::string &back, const std::string &backShape,
      const std::string &backFrame, const std::string &correction,
      const std::string &observer, Real tolerance);
void RefineLatitude(SpiceCell &result, SpiceCell &confinement,
      const std::string &target, const std::string &frame,
      const std::string &correction, const std::string &observer,
      Real latitude, Real tolerance, bool maximum = false);
void RefineFieldOfView(SpiceCell &result, SpiceCell &confinement,
      const std::string &instrument, const std::string &target,
      const std::string &shape, const std::string &frame,
      const std::string &correction, const std::string &observer,
      Real tolerance);
void RefinePhase(SpiceCell &result, SpiceCell &confinement,
      const std::string &target, const std::string &illuminator,
      const std::string &observer, const std::string &correction,
      Real phase, Real tolerance);
}
#endif
#endif
