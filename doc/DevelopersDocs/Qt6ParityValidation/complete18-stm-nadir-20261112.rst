State-dependent SPAD derivatives
================================

The partial stage ``build/nasa-coverage/complete18-20261112-stm-nadir-v1``
repairs the native SPAD STM and A-matrix derivatives for state-dependent
attitudes. The first NadirPointing mission completed its trajectory and
45 parameter reads. Its Cartesian STM failed three of the four original
block tolerances. The largest error was 1.89e-5 seconds in the position
sensitivity to initial velocity. The original failure remains recorded.

The native derivative code held attitude fixed while perturbing position
and supplied zero velocity partials. It now evaluates all six Cartesian
columns using an isolated spacecraft at the integrator stage state and
epoch. Its private attitude and SPAD cache are reused across samples with
forced coordinate conversions. Successive fourth-order differences and
one-sided consistency checks govern publication of the complete gradient.
The propagated spacecraft is left unchanged.

Five targeted native missions pass. Constant and affine angular SPAD tables
use Earth NadirPointing with the OrbitNormal constraint, alignment -X and
body constraint +Y. Existing Bicubic and SunFacing perfect-specular N-plate
cases exercise the affected paths. These four missions provide 180 original,
clone and copy frame reads. A separate A-matrix-only mission checks all
36 raw derivative entries. Its first probe used elapsed duration after the
force base epoch had advanced to the endpoint. The corrected probe aligns
to the actual force epoch, and the failed receipt remains unchanged.

The independent oracle integrates all six variational columns. Decimal
finite differences separately cross-check its complex-step derivatives.
A negative control omitting velocity-dependent attitude terms fails the
original STM bounds. Four earlier SPAD/N-plate cases are replayed from saved
results. The oldest receipt uses its original reader because its metadata
predates the interpolation field.

Frame conversion consumes the native Cartesian STM and endpoint states,
after those inputs pass independent dynamics checks. Sun ephemeris remains
a shared native input. Neither independent ephemeris qualification nor
whole-current-build qualification is claimed.

Other attitudes, nonlinear SPAD boundaries, plate optical and shadow
domains, other force providers and the complete frame/body conjunction
remain open. This stage adds no whole-row credit. Coverage remains 1,113 of
1,158, with 12 of the 18 agreed requirements complete.
