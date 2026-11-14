Finite-burn read-only metadata and actual GUI write rejection
============================================================

Coverage remains **1,019/1,158 (88.0%)**, 139 open and 82 more needed for 1,101.
Historical coverage remains 1,816/2,367. Original authorities, denominators,
59 prerequisite hold identities and all qualification gates remain. No exact
whole row or FRR!E491 conjunction is newly qualified.

Seven derived FiniteBurn parameters now correctly advertise IsSettable=false:
TotalMassFlowRate, TotalAcceleration1/2/3 and TotalThrust1/2/3. Actual Assignment
destination browsing omits them, typed destinations cannot be selected, typed
invalid Apply and raw-source Build reject writes, while Report browsing and
Assignment RHS reads remain available. Ordinary writable Variables and the
original impulsive-burn GUI writes remain supported. All 321 factory names and
readable roles remain; this is a corrected writable classification, not a
factory/owner-role exclusion.

Demonstrated mismatch and production increment
----------------------------------------------

The calculation-parameter reference marks these seven fields N under Settable.
The native setter implementations already reject them through BurnData::SetReal's
Unknown ELEMENT id exception. Pre-fix probes directly record the contradictory
factory/cache/runtime flags as true, each property offered in the actual writable
chooser and the typed derived reference accepted there. Native attempted writes
are rejected atomically, with value, owner source and reference unchanged.

The production increment changes only three constructor arguments true->false
in src/base/parameter/BurnParameters.cpp, covering the seven types. Units from
the preceding repair, numerical formulas, setter implementations, owner/dependency,
member/virtual layout, factory membership and reportability/plottability remain.
The writable metadata/API behavior changes deliberately to match the implemented
read-only operations and reference. The old direct-native Unknown ELEMENT id
diagnostic is retained; no new native error policy is claimed.

Exact preimage/core-v5-readonly-flags.patch remain. The file was already pending
from the unit-label repair: current schema-2 core inventory remains 238 paths
(225 tracked differences, 13 untracked), with 237 prior working hashes unchanged
and this one existing path incremented. No new pending core path is introduced.
The inventory is build/nasa-coverage/core-change-review/20261103-finite-burn-readonly/.
Technical scrutiny remains open.

Actual GUI and native evidence
-----------------------------

Each of seven new cases verifies all 321 advertised system types plus Array/
String/Variable. The unmanaged factory, ParameterInfo cache and actual main
runtime Parameter report false. Its actual reference points to the live FiniteBurn
owner. Three attempted native SetReal(17.25) calls fail atomically; the original
value and owner/reference/source remain. These are inactive zero reads; the
separate unchanged affected regressions verify nonzero active/function behavior.

The actual Mission Tree Assignment opens the destination chooser. Property and
typed-reference rejection are recorded, with a writable Keep Variable as positive
control. The actual Report chooser retains the same derived property and accepts
its existing readable reference through Cancel and OK. A keyboard-typed invalid
left destination cannot Apply and preserves the pending editor/draft, source,
history, prototypes and saved file. Restoring writable Out and using the derived
RHS Applies successfully; clean Apply, exact Undo/Redo, Unicode reopening and a
last mission without explicit Build remain checked. Actual report viewing matches
raw data. Deliberate raw invalid-source Build fails, preserves its supplied text/
saved file, and explicitly restores the accepted model afterward; that deliberate
editor replacement resets history outside the Run invariants.

The seven original finite-burn tests are unchanged and pass again in both modes:
active/stopped main/private/Global and nested calls, actual owner/thruster/tank
identity, corrected units, nonzero flow, raw Writes/Reports and the prior bounds.
The original canonical impulsive-burn test also passes unchanged in both modes,
including its writable browser and independent VNB velocity calculation.

Final selection has **30 GUI passes**: 14 new read-only, 14 affected finite-burn
and two original writable-impulse controls. The new cases check 42 missions,
42 native atomic setter refusals, 126 raw values, 14 factory and 14 cache flags,
42 runtime flags, 14 destination rejections, 28 readable Report dialogs, 14 typed
invalid Apply and 14 raw write Build rejections, plus 14 positive RHS recoveries.
The affected family adds 42 missions and 672 owner/unit/Write/Report observations
per respective metric. Writable controls add six maneuvers and 204 raw scalars.
No numerical assertion or bound is relaxed.

Visible-v1 passes fifteen actual mapped processes with stable source/image hashes,
initially minimized/resizable/nonfullscreen windows, software GL and zero
screenshots/runtime/kernel copies. Native exposure count is in the audit; no
human or physical GPU claim. Ten corruptions are rejected: factory/cache/runtime
writable flags, offered/typed left references, blocked Variable control, missing
readable selection, mutated native value, accepted raw write and omitted no-Build
repeat. Offline audit passes on its first invocation. All handles are terminal.

Retained fixture failures and custody
------------------------------------

Sixteen failed CTest records and one interrupted record remain, with exact sources:

* v1: seven baseline isolation failures, lacking an after-snapshot diagnostic.
* v2: one focused diagnostic failure records only Sat's automatically assigned
  NAIFId/reference-frame IDs changing; source/history/file remain unchanged.
  Explicit fixture IDs preserve the original prototype equality assertion.
* v3: one modal timeout and one interrupted next case. The zero timer fired in
  reveal(), before the actual click opened its dialog. The exact runner was
  interrupted; five unstarted cases are neither failures nor passes. Polling
  starts after reveal and stops when an actual modal is available. A safe editor
  lifetime check precedes rejected-Apply inspection.
* v4: seven actual metadata/chooser failures after those fixture corrections,
  directly demonstrating the writable mismatch. Only then are production flags
  corrected. v5's passing mass-flow case is reused; v6 runs the remaining six
  plus the eight unchanged affected/writable controls.

No compile failure, metadata-failure relabeling or missing original diagnostic is
invented. Old source versions and the interrupted receipt retain their original
status. Native/source helper and review files are preserved as regular text.

Authoritative files are in build/nasa-coverage/coverage95-20261103-finite-burn-readonly/:
evidence-audit-v1.json, evidence-inputs-v1.json,
finite-burn-readonly-matrix-v1.json,
remaining-parameter-binding-worklist-v1.json, fixture-scope-review-v1.json,
core-identity-continuity-v5.json, final-source-identities-v1.json,
stage-receipt-v1.json and seal-verification-v1.json. Earlier sealed witnesses stay
attached to their original source/binary identities.

Replay and remaining scope
--------------------------

Replay only new/affected work or an explicit watch request::

  cmake --build build/linux-gui --target GmatQtNasaFiniteBurnReadOnlyTests -j4
  python3 src/qtgui/tests/RunFiniteBurnReadOnly.py --filter '^QtGui.NasaFiniteBurnReadOnlyTotalMassFlowRate$'
  python3 src/qtgui/tests/RunFiniteBurnReadOnly.py --visible --filter '^QtGui.NasaFiniteBurnReadOnlyTotalThrust2$'
  python3 src/qtgui/tests/AuditFiniteBurnReadOnly.py --stage build/nasa-coverage/coverage95-20261103-finite-burn-readonly --output /tmp/finite-burn-readonly-audit.json

Seven ordinary CTest registrations use native X/software GL, the engine lock and
both standard acceptance dependencies. Prior fields of all 566 role rows and
original settable=true metadata remain as historical evidence. Additive fields
record current false flags and the bounded factory/cache/main-GUI resolution.
Actual function-local/global read-only flags and attempted function writes are
not directly observed by this new driver; wider function/owner/domain/physics
qualification remains open despite the unchanged function-read regression passes.

Native-bound types remain 57 of 321, with the complete 264-entry queue unchanged:
246 Spacecraft, 14 SpacePoint, two Solver and two ThrustAngle metadata owners.
All 321 retain whole obligations. Preserve finite hardware-frame/provider gaps,
nine unmodeled System probes, strict-time/custom-Star failures, E491 conjunctions,
E477/E406, prior science/source conflicts and all 59 holds. Scientific/technical/
full-current-build/full-NASA approval stays false. Full 95% goal active/incomplete.
