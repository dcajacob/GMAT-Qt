CK final-pass membership and clock repair, 2026-11-11
====================================================

The remaining FRR!E406 writer census includes native-script Quaternion CK
output. The Cartesian GUI's existing CK rejection remains valid, but it does
not exclude that separate script route. Its saved passing GUI evidence was
reused without another GUI run.

Two supplementary Target/DifferentialCorrector and Optimize/Yukon missions
vary both the orbit radius and a constant Euler attitude. Their actual trial
reports contain distinct controls and attitudes. Each final CK contains exactly
25 packets for the accepted 50-degree attitude over 120 seconds. Independent
DAF decoding and quaternion polynomials verify every packet, with unchanged
control, circle, attitude and clock bounds.

Three additional native witnesses cover BatchEstimator, ExtendedKalmanFilter
and Smoother CK output, using the existing noiseless missions and independent
circle/nadir geometry. The first filter and smoother attempts found a real
cleanup failure. A final boundary at an epoch already covered by the generated
SCLK attempted to append duplicate coefficients after unloading that clock.
SpiceAttitudeKernelWriter now retains the clock when its rounded end already
covers the segment. The later-epoch extension path and public APIs are unchanged.

All five CK contexts pass after the repair. Batch output has 1,436 packets over
1,435 distinct final epochs; its two segments share one endpoint. Filter and
smoother each have 22 packets over 11 distinct final epochs, in 12 native
measurement-boundary segments. Every quaternion matches the selected final
publication and the independent nadir geometry. Maximum batch attitude error
is 2.21e-12, below the frozen 2e-8 bound. Maximum CK epoch error is 5.37e-7
seconds, below 5e-5 seconds. The existing background intrusion/clock check
also passes. These are six directly affected checks, not a full regression.

The initial supplementary one-segment prediction for filter/smoother omitted
native measurement discontinuities. That prediction is retained in the first
frozen plan. The second plan, frozen before repaired execution, requires every
packet's final membership and every independently scheduled epoch while
allowing shared segment boundaries. Numerical expectations and tolerances
were not relaxed. Both original CK cleanup failures, the fixture-name collision,
the recorder failure and the missing-include build failure remain recorded.

Evidence is in
``build/nasa-coverage/complete18-20261111-iterative-ephemeris-scope-v1``.
Independent audits launch no native tests. All 1,334 protected original inputs
remain unchanged. Existing OEM, SPK, Code-500, STK, nested, interrupted and
terminal evidence retains its own source versions and original failures.

The inherited seal had one mutable artifact binding: the generated CTest
registration file. Adding the five CK checks changed that file. Its exact old
bytes were recovered using the archived CMake source and source-line offsets,
and match the inherited SHA-256. A read-only historical artifact view supplies
those bytes to the unchanged frozen readers. The live build, expected digest,
original prefix and coverage records are unchanged. Corrupt bytes, substituted
expected hashes and write attempts are rejected. The first failed replay is
retained beside the successful replay through this explicit archival view.

FRR!E406 is not admitted yet. Its old wording says only the iterative process's
"solution" may be written. Whether a clearly reported nonconverged final
estimate counts is awaiting the user's interpretation; the default native
policy is unchanged. Canonical coverage remains 1,104/1,158, with 3 of the 18
agreed rows complete. No whole-current-build or scientific qualification follows
from this bounded engineering batch.
