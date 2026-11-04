====================================
OEM explicit-window boundary repair
====================================

Coverage remains **1,051/1,158 (90.8%)**. There are 107 original requirements
open and 50 needed for 1,101. Historical operations remain 1,816/2,367. All 59
original hold identities remain intact. This component earns no requirement
credit; FRR!E375, its parent E374 and all broader gates remain open.

Repaired behavior
-----------------

Explicit OEM start/end epochs require interpolation even when StepSize is
IntegratorSteps. The former finalization path only reduced order and drained
waiting data for fixed-step output. Short windows consequently lost boundaries
or produced only a header. A long initial-window case appended an earlier
boundary after newer native samples, producing decreasing timestamps.

One existing pending core file, EphemWriterCCSDS.cpp, now keeps native samples
behind pending explicit boundaries, enqueues a crossed final bound even when
one step crosses both limits, finishes explicit-window interpolation through
the existing order-reduction path, and drains the exact current sample at run
finalization. The existing canFinalize guard is retained. A new propagator-switch
case passed both profiles without changing event finalization speculatively.

The pending core inventory stays at 283: one existing file incremented and 282
unchanged. There are no new members, virtual slots or APIs in this increment.
Exact preimages, core-increment-v3.patch, core-increment-v4.json and the complete
core-change-inventory-v1 preserve every pending source and HEAD diff. The original
sampling driver/catalog and the prior six-case record-support catalog are byte
identical. All 56 require expressions in the shared new driver are preserved.
The only driver addition records raw native inputs for the failing diagnostic case.

Results and remaining scientific failure
----------------------------------------

Nine new window cases pass both headless and visible profiles:

* EqualBetween: one off-grid record, recommended degree 0.
* InitialOnly: three records, degree 2.
* FinalOnly: four records, degree 3.
* Interior: four records, degree 3.
* NarrowInterior: two interpolated boundary records, degree 1.
* LongInitial: 99 records, degree 7, including the previously dropped EOF sample.
* AtInitial: one record at the initial spacecraft epoch, degree 0.
* FinalAligned: five records including the exact final epoch, degree 4.
* WindowSplit: two propagator segments with 9 and 10 records, degrees 7 and 7.

The prior aligned OneRecord case also passes both profiles. The independent B-1
syntax/count checker accepts these outputs under the explicit controlled test
identifier agreement. It does not certify creation-clock accuracy or full NASA scope.

NarrowAtLast remains a **scientific failure** in both profiles. Only three native
samples exist at 0, 5 and 10 seconds. The native warning records reduction from
order 7 to 2 for generating samples at 5.25 and 5.75 seconds. The exported two-record
block correctly recommends degree 1 for a recipient, a separate metadata question.

The strict circular-state errors reach 2.6942092880233304e-5 km in position and
2.9043781227255927e-8 km/s in velocity, above the unchanged 2e-5 and 2e-8 bounds.
OemShortWindowScience.py independently evaluates a quadratic through the three
actual raw native input states. It reproduces the serialized states at nearby
representable query clocks. That is a diagnostic explanation of truncation,
not a passing science result. No assertion, expected physical state or tolerance
is relaxed. Raw inputs, outputs, warnings and both scientific diagnostics survive.

There are 30 actual GUI contexts: 20 passes and 10 retained failures. Ten distinct
cases pass both profiles, including the prior aligned case. The main visible
selection runs ten cases and the additional split selection runs one. Both use
initially minimized, resizable, nonfullscreen nested desktops. All 30 native
identities and both display servers were observed terminal. There are no native
binary or kernel copies; runtime links are temporal observations only.

Source versions 1 and 2 retain the six baseline failures and first repair results.
Source 3 is a successful, unexecuted EOF-fix build: a fixture preparation command
failed before changing the test, while the subsequent capture/build succeeded.
The failure is explicit in preparation-failure-v3.json. Source 4 owns the final
EOF/accuracy diagnostic and main visible selection. Source 5 adds only the new
split-case registration/catalog and owns its two passing profiles.

Confirmed next format defect
----------------------------

CCSDS 502.0-B-1 section 4.3.7(b) requires the actual creation time in UTC.
The final visible receipt bounds are 18:41:54 through 18:42:12 UTC, but an OEM
CREATION_DATE field reads 11:41:56. Adding seven hours places it inside the run.
CCSDSEMWriter::WriteHeader calls GmatTimeUtil::FormatCurrentTime(2), which uses
localtime. The process runs in MST. This confirms a semantic format defect that
the existing syntax checker deliberately does not certify.

creation-clock-source-review-v1.json binds the actual field, UTC receipt bounds,
owning sources and original September 2004 standard. The standard PDF SHA256 is
fab306a994e5bce9cad8c64d77af172a5819af38faf459faab1b7e2f6163223c.
The next repair should make CCSDS creation headers use UTC and verify several
process timezone settings without changing generic local-time reporting.

The original ShortArc expected_degree=2 versus repaired degree=1 conflict is still
unchanged. Its exact one-value patch is reviewable in the prior record-support
stage. An explicit exception was requested because the campaign requires preserving
original assertions. No approval is inferred. E404 mandatory-error/fallback and
E381 incomplete-frame wording remain open. This stage changes no hold resolution.

Replay and reuse
----------------

The frozen audit checks all five source capsules, matching successful builds,
actual loaded-image observations, independent file checks, every retained pass
and failure, unchanged original expressions, exact core changes, 107 remaining
rows, 59 hold identities and the UTC creation-field counterexample::

   python3 build/nasa-coverage/coverage95-20261104-oem-short-windows/verify-component-v1.py

The prior record-support component and canonical 1,051 prefix also replay after
the new builds without native reruns. Their fresh replay receipts are retained.
The source-specific headless passes are reused with the final visible contexts;
there is no generic original-test replay or complete-current-build claim.

The reusable registrations are QtGui.NasaOemShortWindow followed by each of the
ten catalog keys. Existing drivers, controlled identifier profiles and independent
circular bounds remain intact. Technical, scientific, full-build and full-NASA
approval are not implied by this bounded repair or its replay.
