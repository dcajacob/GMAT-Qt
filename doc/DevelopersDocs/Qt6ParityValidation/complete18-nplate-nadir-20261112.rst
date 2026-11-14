N-plate state-dependent attitude evidence
========================================

The targeted stage is stored in
``build/nasa-coverage/complete18-20261112-nplate-nadir-v1``.

The original fixed-body plate mission exposed an incorrect derivative layout
in ``NadirPointing::GetRotationMatrixDerivative``. Its six returned matrices
grouped derivatives by body axis. Plate reflectance consumers require one full
rotation derivative per Cartesian state coordinate. The repair transposes that
grouping while retaining the existing alignment rotation and public API.

The captured per-spacecraft N-plate force gradient also exceeded the frozen
``1e-20`` bound when calculated by numerical differencing. It now uses the
existing analytic plate derivatives with inverse-square pressure and the
existing illumination gradient. Native N-plate STM and A-matrix shadow-gradient
coverage remains open.

Two source-versioned native missions passed the frozen trajectory, STM and
force-gradient bounds. They exercise an OrbitNormal constraint with specular
reflection and a rotated Velocity constraint with mixed specular/diffuse
reflection and a partial lit fraction. Together they provide 90 owner-frame
parameter reads, 108 direct attitude-derivative entries and 36 force-gradient
entries. Three relevant saved cases also passed an offline reader replay.

All three failed native receipts remain available. The source, executed
collector, executed libraries, numerical readers and frozen fixtures are
retained. The metadata-only reader revision corrects the force description and
identifies native endpoint/STM inputs used by the separate frame-conversion
check. Independent Cartesian propagation still supplies the trajectory and
variational-flow checks.

The stage adds evidence for FRR!E525. Canonical coverage remains 1,113/1,158,
with 12/18 agreed rows complete. Other force, attitude, augmented-state and
frame combinations remain open. The formation-cloning and historical-function
acceptance decisions also remain pending. Original protected assertions,
tolerances and failure statuses are retained. Validation used targeted native
missions and offline replay.
