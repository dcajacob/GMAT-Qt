OEM endpoint sparse-support diagnosis, 2026-11-08
====================================================

The unchanged NarrowAtLast failure reproduces on current sources. Its adjacent
NarrowInterior control passes. The failure remains an original scientific
non-pass; no test, fixture, tolerance, sample, state equation or admission gate
is changed.

Cause established from current data
----------------------------------

The original10-second mission supplies three native states at approximately
0,5 and10 seconds. Its requested degree7 Lagrange interpolation is reduced to
degree2 under the existing ReduceOrder policy. The two requested OEM records
are at5.25 and5.75 seconds. The exported file correctly recommends degree1
for its two records; this is distinct from the producer's internal degree2.

A diagnostic compiled against the current public GmatUtil API performs the same
UTCGregorian-to-A1 conversion as EphemerisFile, then feeds the captured native
samples to the real LagrangeInterpolator. Its results match the emitted records.
An independent80-digit Decimal Newton divided-difference calculation also
matches those records within floating-point/serialization rounding bounds.
It uses a different calculation from the native product-form implementation.

The observed maximum component errors are2.694209288e-5km and2.9043781224e-8km/s.
An ideal exact-clock mesh with exact circular states still has degree2 errors
2.6786926367e-5km and2.8876510538e-8km/s. Both exceed the unchanged original bounds
2e-5km and2e-8km/s. Thus sparse-polynomial truncation alone fails the gate;
a clock-shift patch is not justified. The prior microsecond-shift hypothesis
is not used to alter the trajectory or its timing.

Contract and disposition
------------------------

Current help explicitly specifies Lagrange generation and order reduction,
without switching method or extrapolating. Quietly using Hermite or adding
native samples would change this configured behavior and the original scenario.
No such change is made. Help now clarifies that reduced order does not retain
the requested-order accuracy, and that tight integration accuracy does not add
interpolation constraints. Its XML parses successfully.

CCSDS502.0-B-1 section1.2 leaves orbit accuracy to the participants' ICD, while
Table4-3 describes recommended interpolation of the exported records. It does
not supply a2cm guarantee for a three-sample producer fit. This contextualizes
the conflict; it does not waive or reclassify the original accuracy failure.
Primary source:
https://spsweb.fltops.jpl.nasa.gov/portaldataops/mpg/MPG_Docs/Source%20Docs/CCSDS-OrbitDataMessages-502x0b1.pdf

The captured inputs/outputs, authored/native epochs, probe source/executable,
independent analysis, original result and source preimages are retained in
build/nasa-coverage/coverage95-20261108-oem-endpoint-epoch-current-review.
All1,334 protected original inputs remain unchanged. No new acceptance credit;
canonical1078/1158 and all full-scope gates remain unchanged.

Continue automatically with a complete current acceptance-consumer build,
then implemented-feature regression and repair of demonstrated integration or
runtime defects. The sparse-support failure stays open for an explicit data/
interpolation-contract decision; it is not treated as a product repair achieved.
