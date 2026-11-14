Thruster force-direction repair, 2026-11-08
=========================================

A valid commanded +Y force vector was rejected because ThrustDirection1-3
shared the storage used by Hardware to construct a sensor frame. It became
parallel to the default secondary sensor direction; hardware initialization
returned false and the local thrust frame was never created.

Thruster now owns its commanded force vector separately. Existing parameter
IDs, labels, GetDirection callers, selected-frame rotations, scaling and mass
flow equations are preserved. Clone, assignment and native restoration copy
the owned value. Hardware sensor orientation validation remains unchanged;
the hardware orientation is independent of the commanded force vector.
The concrete Thruster layout grows, so all native consumers require a coherent
rebuild. GmatBase/Solver/Optimizer/VF13ad layouts and virtual ordering are intact.

The unchanged original GUI thruster test now passes after its retained original
failure was reproduced on a current build. All 71 original GUI/hardware/derived
thrust/thrust-angle cases pass headless and visible (142 executions); four
original native angle controls also pass. Seven additional +Y burn cases pass
independent equations for chemical/electric thrust, fixed/variable mass,
restart and nonlinear stopping. Their original numerical bounds and strict
1e-10 stop residual are preserved under the X-to-Y benchmark rotation.

Only Thruster.cpp and Thruster.hpp changed in this repair. All 1,334 protected
original inputs and the canonical prefix remain intact. Evidence:
build/nasa-coverage/coverage95-20261108-thruster-direction-current-review.

No new NASA row credit: canonical 1,078/1,158 (93.1%). Remaining implementation,
long combined plot crashes/history, unresolved requirements, final coherent
full rebuild/regression and platform qualification remain open.
