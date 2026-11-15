Tesseral gravity in owner-dependent STM frames
=============================================

The partial evidence stage
``build/nasa-coverage/complete18-20261112-stm-tesseral-v1`` adds one native
mission for the longitude-dependent gravity portion of FRR!E525. The mission
uses a synthetic degree-two GravityField with nonzero C21, S21, C22 and S22,
along with J2. All 45 reads pass across three defined ObjectReferenced frames,
five STM Parameters and original, clone and copy instances.

The independent oracle expands the fully normalized potential as a Cartesian
quadratic polynomial. It calculates the force analytically, uses complex-step
spatial derivatives and integrates the variational equations with RK4. A
separate high-precision frame differential checks the state-dependent output
frames. The oracle shares sampled native body orientations. Earth-orientation
accuracy remains outside this evidence stage.

The original numerical bounds are retained. The maximum native Cartesian STM
error is recorded in ``replay-audit-v2.json``. Omitting each of the four
longitude-dependent coefficients independently exceeds the frozen trajectory
bound. Three corrupted-evidence controls are rejected.

The native mission completed on its first run. Its original audit failed while
reading adjacent fixed-width coefficient fields containing negative values.
The corrected reader reuses the saved native output. The original failure,
reader source and correction receipt remain in the stage. No native mission
was repeated, and no production source changed.

This stage earns zero whole-row credit. FRR!E525 still requires the complete
frame, parameter, body and domain conjunction, tide and non-gravity force
coverage, the remaining augmented-STM history checks and reconciliation of
the original ``FR-xxx`` cross-reference. The sealed coverage remains
1,113 of 1,158 requirements, with 12 of the 18 agreed requirements complete.

Run ``python3 verify-stage-v1.py`` from the evidence directory to verify its
manifest and replay the frozen numerical reader without launching native tests.
