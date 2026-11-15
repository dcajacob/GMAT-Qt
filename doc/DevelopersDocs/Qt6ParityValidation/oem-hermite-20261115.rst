OEM Hermite option, 2026-11-15
=============================

The user approved an explicit Hermite option for the sparse OEM endpoint
accuracy failure. Lagrange remains the default. Its original NarrowAtLast
failure, protected fixture, assertions and tolerance are retained.

Implementation
--------------

CCSDS-OEM accepts ``Interpolator = Hermite`` with odd degrees 3, 5, 7 and 9.
The Qt ephemeris editor exposes the choice and preserves it through format
switching, Apply, undo, redo and save/reopen. The existing native Hermite
polynomial uses positions and velocities from a bounded rolling sample stencil.
Covariance, quaternion and acceleration fields retain value interpolation.
The Hermite writer requests committed integration timestamps and preserves
explicit epoch bounds through precision-time conversion.

The number of constraints is twice the number of Cartesian records. A requested
degree seven therefore needs four samples. ReduceOrder uses the available degree
and writes metadata supported by serialized records. Error rejects insufficient
input samples or exported records. Single-record output remains readable at its
recorded epoch. Both OEM reader entry points accept Hermite metadata and use the
declared polynomial degree.

The method is listed in CCSDS 502.0-B-1 Table 4-3. Its presence in the standard
does not impose a trajectory-error tolerance. The protected independent circular
orbit bounds remain the numerical acceptance criterion for exported states.
The standard is available at https://ccsds.org/Pubs/502x0b1s.pdf.

Targeted verification
---------------------

All five supplementary scenarios passed on the Linux development build.
They use the original fixture values and export tolerances, 2e-5 km and
2e-8 km/s. The sparse NarrowAtLast case reached maximum component errors
2.6131070285e-7 km and 1.1043243653e-8 km/s. Additional scenarios cover a
single record, a rolling buffer longer than its capacity, a propagator-change
segment boundary and fixed-step output.

The same executable checks all four supported degrees in both time directions
against analytic polynomials. It checks cloning, duplicate replacement, reset,
ancillary interpolation, nonfinite-input rejection, invalid GUI degrees and
strict input/output insufficiency. Both readback entry points are compared
against an independent long-double confluent divided-difference calculation.
Readback accuracy between serialized samples remains dependent on their spacing
and errors. These checks qualify the declared interpolation polynomial.

The narrow scenario also passed on private native X11 through Xvfb. No host
keyboard input was sent. The existing Lagrange sufficiency-policy test passed
its targeted cases across OEM, STK and Code-500. A broader existing Ephemeris
GUI test reproduced its previously saved stale-panel failure. Both the earlier
failure and the new log are retained in the companion receipts. That failure
remains outside the completed Hermite checks.

Scope and preservation
----------------------

The implementation is based on main e79b05937b42fddb66de7cc9f7a1025802ea9d17.
The protected NasaOemRecordSupportTests.cpp and NasaOemShortWindowCases.json are
unchanged. Their original Lagrange failure continues to describe that configured
method. No coverage credit or denominator changes were made.

Seventeen local copies of the published alpha.4 downloads were rehashed and
match their saved public-download verification. Existing release archives,
checksums and release records were preserved. This change is verified on Linux.
Windows and both Mac architectures have not run the new tests. Fresh release
archives were not requested or generated for this change.

The companion oem-hermite-20261115 directory contains source fingerprints,
verification.json, SHA256.json, exported files, strict-error diagnostics and
retained compatibility results. Development logs and intermediate failures are
retained under build/oem-hermite-20261115.
