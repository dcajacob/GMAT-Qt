Relativistic force derivatives in STM frames
===========================================

The partial stage
``build/nasa-coverage/complete18-20261112-stm-relativistic-v1`` records and repairs
two related defects. RelativisticCorrection supplied zero STM and A-matrix
derivatives, and the owner-dependent frame path could not obtain its
acceleration gradient. The captured pre-repair mission rejects all 45 frame
reads and exceeds every frozen STM block error bound.

The repair differentiates the existing Schwarzschild, geodesic and
Lense-Thirring force expressions. The native STM and A-matrix assembly use the
associated spacecraft state. The frame sampling path captures all six
Cartesian derivatives. The existing gravity capture adapter supplies zero
velocity columns for its position-only gradient.

The repaired solar mission passes 45 reads across three defined frames, five
STM Parameter types and original, clone and copy routes. An independent
post-Newtonian force implementation, complex-step variational integration and
high-precision frame differential check the numerical output. The maximum
Cartesian STM error is recorded in ``native-after-v2/independent-audit.json``.
The direct captured gradient agrees with the independent derivative to the
frozen bound. This mission exercises Schwarzschild and solar spin terms.
Non-solar geodesic combinations remain outside this mission's coverage.

The A-matrix-only mission separately checks the actual force derivative array.
The initially attempted instantaneous comparison against the spacecraft's
stored OrbitAMatrix failed because that field is accumulated during
propagation. That original receipt remains in ``amatrix-only-v1``. The revised
witness records the force's derivative array through the existing propagation
state associations and uses the original numerical tolerance.

The affected tesseral gravity path passes one dependent mission with the new
six-column capture buffer. The original and repaired relativistic runs have
identical raw spacecraft states and Cartesian accelerations. No full
regression was run.

The stage retains the initial fixture errors, original numerical failures,
source snapshots, mapped-image hashes and reader corrections. The source
changes affect three production files. The protected original assertions
retain their prior values.

This stage earns zero whole-row credit. FRR!E525 still needs its remaining
force, frame, body, domain and augmented-state conjunctions. The sealed count
remains 1,113 of 1,158, with 12 of the 18 agreed requirements complete.
