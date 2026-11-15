Higher-degree gravity in owner-dependent STM frames
==================================================

One new native mission verifies a degree-four zonal GravityField with nonzero
J2, J3 and J4. All 45 selected-frame Parameter reads pass: three defined frames,
five matrix Parameters, and original/clone/copy routes. Existing J2 and point-mass
passes are reused separately. No production source changed for this witness.

The independent oracle differentiates the explicit zonal potential, integrates
state and variational equations with RK4, and differentiates the full Cartesian
frame map with Decimal arithmetic. The original matrix bounds remain unchanged.
Removing J3 or J4 changes the final position by 0.00415 km or 0.00303 km, both
well above the 0.00002 km bound. Native body-pole samples are shared input; this
does not independently qualify Earth orientation.

An accidental MJ2000Eq name replacement in the new oracle was corrected. The
original audit failure is retained, and the saved native result was replayed
without rerunning the mission or changing its numerical expectations.

The evidence stage is
``build/nasa-coverage/complete18-20261112-stm-higher-zonal-v1``.
FRR!E525 remains open: higher-degree zonal coverage does not complete every
frame, force, domain or source-reference obligation. Coverage stays at
1,112/1,158 and 11/18 agreed rows. No full regression was run.
