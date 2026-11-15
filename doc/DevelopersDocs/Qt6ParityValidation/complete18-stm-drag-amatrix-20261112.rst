Drag A-matrix-only evidence
==========================

The stage is stored in
``build/nasa-coverage/complete18-20261112-stm-drag-amatrix-v1``.

One targeted mission exercises A-matrix-only propagation with different mass,
area, drag coefficient and density scale for two spacecraft. The collector
reads both actual force derivative blocks through their propagation-state
associations. All 72 entries pass the independently derived constant-density
drag differential. The two propagated trajectories also pass independent
integration checks.

The absolute derivative bound is ``4e-11``. It was frozen before execution
from the quadratic-drag Hessian bound and the existing native forward
velocity-difference step of ``1e-6 km/s``. The observed maximum error is
``1.1237563424292343e-11``. Existing trajectory and STM bounds are retained.
This stage changes supplementary test collection and oracle code only.

The result extends source-versioned drag evidence for FRR!E525. It grants zero
whole-row credit. Coverage remains 1,113/1,158, with 12/18 agreed rows complete.
Empirical atmosphere, wind, SPAD drag and the wider force/frame conjunction
remain open. Native atmosphere angular velocity is a shared environmental
input. No full regression was run.
