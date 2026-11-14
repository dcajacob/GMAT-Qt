Direction-dependent SPAD drag
=============================

Evidence is stored in
``build/nasa-coverage/complete18-20261112-stm-drag-spad-angular-v1``.

Two additional missions exercise angular variation in SPAD drag. The first
uses an affine angular area table with Bilinear interpolation and an
OrbitNormal-constrained NadirPointing attitude. The second uses quadratic
angular terms, Bicubic interpolation and a Velocity-constrained attitude.
Both use tilted body alignment vectors and a finite-height exponential
atmosphere.

The missions pass 90 native full and partitioned STM parameter reads, including
original, clone and copy routes. Cartesian trajectories, variational flow and
captured force derivatives agree with the independent polynomial area model.
Maximum STM error is 3.02e-12 and maximum acceleration derivative error is
2.74e-17. Removing the angular area terms exceeds the original trajectory
tolerance in both cases.

The independent complex-step derivatives were checked against 80-digit
Decimal finite differences before native execution. Three saved cases pass
offline replay with the extended oracle. This stage changes the supplementary
collector and oracle, with no production changes. Original numerical bounds
and protected test inputs are retained.

Canonical coverage remains 1,113/1,158, with 12/18 agreed rows complete. The
remaining force, coordinate and augmented-state obligations require their
combined audit before FRR!E525 can receive credit.
