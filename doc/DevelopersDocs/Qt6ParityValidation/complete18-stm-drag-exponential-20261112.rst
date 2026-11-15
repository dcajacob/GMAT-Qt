Altitude-dependent drag derivatives
===================================

Evidence is stored in
``build/nasa-coverage/complete18-20261112-stm-drag-exponential-v1``.

A spherical-Earth exponential atmosphere with a 200 km scale height exposes
first-order error in the default forward-difference drag STM. The low-altitude
case exceeded the original matrix bounds. DragForce now uses its existing
centered-difference path by default. The public interface is retained.

The repaired case passes 45 full/partitioned owner-frame parameter reads with
independent trajectories and variational flow. A separate A-matrix-only mission
passes all 72 entries for two spacecraft with different ballistic and density
coefficients. One previously passing constant-density case was rerun because
the derivative method changed and passes another 45 frame reads.

The native density is checked against the independent exponential formula.
The admitted fixture explicitly requires a non-negligible density interval,
uses an explicit matching monopole gravity file, and declares spherical Earth
geometry. Native atmosphere angular velocity remains a shared environmental
input. Empirical atmosphere and wind models are outside this stage.

The first build needed an accessor correction, and an attempted run used the
previous executable. That run is excluded from evidence. An inherited 30,000 km
transverse offset also produced a high-altitude pass with negligible density.
That pass is retained separately. The low-altitude replacement and its original
failure are preserved alongside the repaired runs. Original numerical bounds,
protected test inputs and failure statuses remain unchanged.

The stage adds evidence to FRR!E525. Canonical coverage remains 1,113/1,158,
with 12/18 agreed rows complete. Whole-row force/frame coverage remains open.
