SPK coverage start and precise reader repair — 2026-11-08
========================================================

Omitted and explicit EphemStart missions using a real SPK interval [-120,120]
failed initialization with SPKINSUFFDATA. FromSpacecraft controls passed. The
legacy A1 date returned for ET=-120 re-encoded just before coverage; the precise
reader also projected its GmatTime input back to an absolute-MJD Real.

Implementation
--------------

SpiceOrbitKernelReader shares one CSPICE interval enumeration between its legacy
and new nonvirtual precise coverage overloads. A start is projected inward onto
the selected clock, and an end inward from the other side. Real dates move only
by adjacent representable values when their round trip falls outside the raw
boundary. State-query epochs are not clamped, cached as boundary identities, or
matched by tolerance. Intervals with no representable epoch are omitted for that
clock, with a clear exception if none remain. Distinct actual gaps stay distinct.

SpiceInterface::SpiceTimeToA1GT builds the TDB epoch from J2000-relative seconds
and resolves the existing nonlinear clock inverse residual. The precise state
reader now uses its existing GmatTime-to-ET overload directly. SPKPropagator
selects precise coverage for its precise start companion and uses matching span
metadata. Legacy forward/reverse clock APIs, forces, frame/origin translations,
error handling, public instance layouts and virtual ordering are retained.

Evidence
--------

Stage: build/nasa-coverage/coverage95-20261108-spk-start-boundary-current-review.
Plan-v1 retains all14 FRC10 rows and71 positional FRR13/14 context rows.

* Corrected native baseline:2/8 pass. Five omitted/explicit start missions fail
  and the precise-reader observation fails; two spacecraft-epoch controls pass.
* Implemented initial8/8 pass. Final9 boundary/reader cases and all12 exact-count
  compatibility/copy/syntax/mission cases pass:21 distinct native executions.
* Reader guards contain26 state observations: four endpoints on both clocks,
  gaps/outside requests including50ns beyond an endpoint, copies/assignment,
  pool replacement, and a157.16ns interval unavailable on the Real clock.
* Raw endpoint ETs -120,-30,30,120 are recovered exactly by the precise coverage
  observations. Legacy starts/ends stay inside; maximum offset237.49ns.
* Three precise-reader affine observations have maximum state error9.1e-13.
  Both early nanosecond epochs share an MJD projection but remain distinct.
* Eleven selected checks pass in EACH runner profile: all nine existing SPK
  origin/start combinations, the canonical epoch contract and the exact-policy
  editor.22 executions, including the canonical contract's support route;
  this is not an assertion of native operating-system input qualification.
* All1334 protected original test inputs match their baseline.43 current source
  owners are archived and hash-verified. Actual loaded ELF identities are retained.

The first newly authored precise witness used raw ICRF fixture values against
GMAT's FK5-returning J2000 route. That witness was corrected to use an independent
CSPICE ECLIPJ2000 rotation and the native encoded clock epoch before the valid
baseline. Both witness versions and the reason are retained. No protected test
was changed. The two CTest wrappers used one output root; all21 distinct per-case
receipts survive, while its results.json contains only the second wrapper.
Native-final-aggregate-v1 records this explicitly.

Limits and continuation
-----------------------

No new complete requirement credit. Canonical acceptance remains1078/1158
(93.1%);80 open,23 additional passes needed for95%. Full provider/domain/science,
unlimited capacity, long-span timing, broad stop/gap behavior, all copy lifecycles,
platform and full-current-build conjunctions remain open. These finite checks do
not qualify the complete Propagate parents. No equation, numerical tolerance,
original fingerprint or denominator was changed. No push/package task performed.

Continue automatically with the current Worksheet RMAG/SemilatusRectum access
review in coverage95-20261108-worksheet-orbit-access-current-review/plan-v1.json,
then Save any resource and the remaining keyed audit groups. Whole goal active.
