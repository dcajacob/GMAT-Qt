Interpolation sufficiency verification and interval repair, 2026-11-08
====================================================================

All 24 final registered checks pass on the current integrated runtime. Five
new verification modes supply 385 observations, and 19 existing OEM, sampling,
SPK and Code-500 regressions pass without changing their assertions or fixtures.
The batch repaired explicit-window export defects in three production sources.
The campaign remains active. Exact requirement admission is the next step;
this support verification assigns no new NASA row credit.

Export defects and bounded repair
---------------------------------

A requested output window can fall entirely between two native integration
samples. STK then left firstTimeWriting true, skipped end-of-run processing,
and reported a successful mission with an empty file. The common end-of-run
path now finalizes a pending explicit initial state after its epoch is reached.

The first-output scheduler could also queue an already completed final epoch
again, or queue only the start of a positive-length window. It now rechecks
final-epoch completion after processing waiting requests. For a whole fixed-step
window crossed before the first normal write, it queues the remaining requested
grid before the ordinary write path can consume the initial-epoch setting.
Ordinary cadence and event-segment paths retain their existing behavior.

Deferred OEM output exposed two initialization gaps. An empty start timestamp
was never replaced because the writer expected a previously written placeholder.
It now initializes either empty or placeholder metadata from the first buffered
record. Finalization also initializes the OEM header when no normal first write
occurred. The resulting single-record and positive-length files have checked
headers, timestamps, record counts and independently interpolated states.

Production changes are confined to EphemerisWriter.cpp, EphemWriterCCSDS.cpp
and EphemWriterWithInterpolator.cpp. No class field, virtual slot, public API,
interpolation polynomial or original strict/compatibility selection changed.
Requested-order sufficiency guards still run before final interpolation.
Earlier campaign repairs remain intact.

Verification scope
-------------------

The new modes cover:

* Twenty Lagrange order/direction observations, orders 1 through 10. Independent
  polynomial values check the curve, exact minimum sample counts, replacement
  at duplicate epochs, compatibility reduction and restored base order.
* Five odd Hermite degrees, 1 through 9, written to actual type-13 SPK files.
  Fifteen insufficient/duplicate/backward preflights reject output, and 85
  CSPICE queries match independent polynomial positions and derivatives.
* Ten actual user/background SPK writer observations. User output rejects short
  data; background output retains its buffer and completes when sufficient
  samples arrive. Native frame normalization is explicit and does not qualify
  the physical FK5 model independently.
* Ninety short/exact EOF missions across OEM, STK and Code-500, orders 1 through
  10. All 27 expected strict failures report the requested order, file and count.
  OEM success states use an independent Newton polynomial on analytic samples.
* Two hundred sixty boundary missions: assignment, maneuver and toggle across
  the three formats, plus single-record and positive-length explicit windows
  for OEM/STK. All 91 expected strict failures are checked. Code-500 retains its
  documented continuous-span behavior across events; its EOF strict failures
  remain separately exercised. Explicit-window states, epochs and counts are
  checked independently.

The 19 existing regressions comprise all six OEM record-support cases, all nine
sampling cases, two SPK storage/recovery cases, the independent Hermite oracle
contract and the independent Code-500 epoch/interpolation audit. All pass on the
same final source/runtime hashes. The previously authorized ShortArc expectation
is preserved. All 1,334 protected original inputs and the original requirements,
worklist and tracker remain byte-identical.

Evidence and next action
------------------------

Stage: ``build/nasa-coverage/coverage95-20261108-interpolation-sufficiency``.
Read ``validation-summary-v1.json``, ``final-current-v13-results.json``,
``retained-failures-v1.json``, ``tested-source-capsules-v1.json``,
``explicit-interval-repair-and-verification-v1.patch`` and ``publication-v1.json``.
Build/test receipts, private startup/output directories, raw proofs, preimages
and all tested changed-source versions are retained. Earlier failed registrations
and incomplete candidate repairs retain their actual outcomes. Corrections to
new fixtures/harness checks are identified separately from product repairs;
no protected test or tolerance was changed. All stage handles are terminal.

Private Xvfb establishes bounded native-widget execution. It does not establish
user-visible native input/exposure, arbitrary body/frame science, complete B-1
or final-iterative output qualification. All 105 original open acceptance rows
and qualification gates remain open. Carried verified coverage stays
1053/1158 (90.9%); the engineering presence estimate stays 1093/1158 (94.39%).

Follow ``next-row-admission-plan-v1.json`` for exact FRR!E404. Review its inherited
clauses and existing admission/profile conventions, reuse this passing evidence,
complete only missing configuration/GUI/profile proof, and replay the sealed
prefix before admitting the exact row. Do not replay unrelated function,
command or plot matrices, or substitute support-test counts for row acceptance.
