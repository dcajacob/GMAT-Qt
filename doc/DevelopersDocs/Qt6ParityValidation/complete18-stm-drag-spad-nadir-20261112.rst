SPAD drag with state-dependent attitude
======================================

Evidence is stored in
``build/nasa-coverage/complete18-20261112-stm-drag-spad-nadir-v1``.

The original native STM sampled perturbed state arrays while the SPAD table
rotation used the spacecraft's unperturbed attitude. A low-altitude exponential
atmosphere and NadirPointing attitude exposed the missing state derivatives.
The original trajectories matched the independent propagation while the STM
failed its frozen bounds. That failure remains in the evidence stage.

DragForce now evaluates SPAD derivatives through an isolated spacecraft clone
at the integrator's actual stage state and epoch. This includes the state
dependence of the attitude and table coordinates. Temporary matrix storage
uses exception-safe ownership.

The final source version passes 45 native full and partitioned STM parameter
reads through three owner-dependent frames and original, clone and copy routes.
A separate A-matrix mission passes all 72 entries for two spacecraft with
different mass and density coefficients. Maximum STM error is
2.75e-12 and maximum acceleration derivative error is 2.02e-17. Original
numerical bounds remain unchanged.

This stage uses a constant body-frame SPAD vector with Bilinear interpolation,
NadirPointing with OrbitNormal constraint, and an independently evaluated
exponential atmosphere. Native atmosphere angular velocity is a shared input.
Other table, attitude and atmosphere domains remain part of the broader row
audit. Source versions, loaded-image hashes and original failure receipts are
retained. Offline replay launches no native tests.

Canonical coverage remains 1,113/1,158, with 12/18 agreed rows complete.
