Ephemeris interpolation boundaries — implementation, 2026-11-06
===============================================================

Strict interpolation now checks input sufficiency before processing the last
requests of a closed segment, including discontinuities where canFinalize is
false. Previously the end-of-run guard could be skipped at those boundaries,
and remaining requests could be discarded when the interpolation buffer cleared.
The common closure path covers OEM, STK and Code-500 sampling. Ordinary startup
still collects points; the default ReduceOrder compatibility behavior remains.

Lagrange also replaces the latest sample at an identical epoch instead of
counting another independent node. This preserves the established time direction
and prevents repeated publications from falsely satisfying the point minimum or
creating duplicate interpolation abscissae. Clear restores the configured base
order and its reset flag, so a temporary short-segment reduction does not become
the next segment's initial order. No interpolation polynomial formula changed.

SPK Hermite minimum
--------------------

SpiceOrbitKernelWriter's minimum-state query previously returned degree+1,
although its preflight used the Hermite count. The query and preflight now share
max(2, degree/2+1). NAIF's spkw13_c specification requires at least (degree+1)/2
position/velocity states for positive odd degree. It also requires a positive
descriptor interval, strictly increasing epochs and record coverage of that
interval. GMAT uses the first/last record epochs for the descriptor, so two
distinct epochs are required even at degree 1. This two-epoch floor is the
combined-constraint inference, not an additional stated NAIF polynomial rule.

Authority: https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/spkw13_c.html
(accessed 2026-11-06; concise source notes retained). Background scratch writers
retain their existing insufficient-data policy while using the corrected
eligibility count. Degree, interpolation math and user-output exception paths
are preserved. Runtime/scientific behavior remains unverified.

Original requirement scope
--------------------------

The unchanged worksheet names OEM and SPK (E374-E376), Lagrange (E383) and
odd-order Hermite (E394). E404 requires an error and stopped interpolation when
the requested type/order lacks data. Nineteen owning-source routes cover policy
configuration, normal startup, explicit/fixed sampling, segment closure, OEM
exported-record metadata, SPK preflight and exception propagation. Code-500/STK
also reach the repaired common closure path. The strict worksheet behavior is
available through InsufficientDataAction=Error under the user's instruction to
support both behaviors. Default ReduceOrder remains explicit compatibility.

E404 is now tracked as implementation present, bringing the provisional tally
to 1093/1158 (94.39%), with 65 other original rows. This is source implementation
presence, not acceptance credit or a completed current-source implementation
audit. Recorded source-versioned verification stays 1053/1158 (90.9%); all 105
acceptance rows remain open. Full B-1/axes, final iterative output and function
ownership, other all-body/frame/command/provider/specification scopes remain.

Compilation and retained artifacts
----------------------------------

The private integrated build passes 67 steps / 2.32 s (v1) and 68 / 1.92 s (v2).
Core, Qt, console, plugins and Python bindings compile/link. Final source matches
v2. Sessions 5368 and 51270 are closed; no build remains live. No new fields,
virtual slots or public signatures changed.

Four exact preimages, before/at-build/final source hashes, focused patch,
19-route source inventory with original requirement identities, concise NAIF
reference, notes, build logs/receipts and summary are under
``build/nasa-coverage/coverage95-20261106-implementation-ephemeris-interpolation-boundaries``.

No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals ran. Original assertions, fixtures, tolerances,
requirement texts/fingerprints, denominator, ledger and qualification gates are
unchanged. No installed/shared runtime, package, commit or push changed. Finish
the remaining implementation before returning to verification.
