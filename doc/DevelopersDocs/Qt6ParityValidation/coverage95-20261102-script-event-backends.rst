Script Event backend command acceptance
=======================================

Four new contexts pass unattended and visibly, covering seven backend command
types: Set, BeginFileThrust, EndFileThrust, FindEvents, RunSimulator,
RunEstimator and RunSmoother. The visible run records 32 native exposure events.
No production source changed in this batch; 289 recorded production files remain
identical. Previously passing contexts were reused at their recorded versions.

Original requirements coverage remains **1,002/1,158 (86.5%)**, with **99** more
whole requirements needed for 95%. Historical coverage remains
**1,816/2,367 (76.7%)**. This batch earns zero whole-requirement credit: FRC!E138
still requires seven explicit alias/framework applicability decisions. Its
original fingerprint, denominator and assertions remain intact.

What the checks establish
-------------------------

All four contexts use the actual Script Event dialog, grouped Mission-tree
entry, Cancel and pending Apply, idempotent edits, Undo/Redo, Unicode Save/Save As
and reopening. Native command containment and source/history retention are
checked alongside independent report and runtime-object results.

* Set imports only Cr from the shipped TVHF example. Before propagation, the
  test checks Cr=1.4 and unchanged epoch/Cartesian state, distinguishing a
  selective import from an ignored command or an accidental full-state import.
* FileThrust applies zero acceleration with a 0.5 kg/s mass rate for ten seconds,
  then coasts ten seconds. Fuel must be 95 kg at both boundaries; an independent
  circular-orbit reference checks the state. This verifies activation and
  deactivation, without claiming thrust-vector dynamics qualification.
* FindEvents runs a manual, 6,500-second contact search on the independently
  derived SingleStation geometry. It checks the one East-station interval,
  both endpoints within 2 ms, duration, final Cartesian state, empty occulting
  body list and the actual ICRF/PCK selections against retained physical hashes.
* Navigation runs the simulator, EKF and smoother against a noiseless,
  Earth-point-mass circular orbit. The reports must contain all eleven distinct
  GPS observations at 60-second spacing, correct epochs and sensor, finite
  three-component values, radius, residual bounds and consistent O-C arithmetic.
  Three phase reports check the independently calculated final state. Both
  configured and propagator-owned force models must contain exactly one
  PointMassForce. These checks do not qualify general estimation algorithms.

The windows are automatically scaled, resizable, initially minimized and never
fullscreen. Restore the nested test desktop to watch. These are programmatic Qt
interactions with native exposure; physical host mouse/keyboard, compositor and
GPU qualification remain separate. No screenshots or runtime copies were kept.

Failures, corrections and quarantine
-----------------------------------

All intermediate failures and build logs are retained under
``build/nasa-coverage/coverage95-20261101/``.

* The initial FindEvents fixture used ``OccultingBodies = {}``, which native GMAT
  interpreted as a nonexistent celestial body. The passing fixture omits the
  assignment and explicitly verifies the default empty list. Explicit empty-list
  syntax remains a reported gap, not a repaired or qualified behavior.
* The initial Navigation fixture accidentally renamed the ``.FM`` property.
  After correcting that generator error, it still retained non-point-mass forces
  in the propagator's earlier clone. The force definition now precedes cloning,
  and both force models are inspected. Numerical assertions were preserved.
* Navigation text reports require ShowProgress=True. The report parser also
  needed to recognize the EKF's Pre-Update (O-C) heading separately from the
  smoother's Residual (O-C) heading. It rejects covariance pages as residuals.
* Two C++ builds failed: mixed auto declarations and a hidden string-parameter
  overload. Both are test-driver corrections, with successful build logs retained.
* Headless-v3 was mistakenly launched after a compilation failure, using the
  preceding executable. Its entire run is quarantined and never selected as
  passing evidence. A build guard now requires the executable and 149 local
  dependency hashes to match a recorded successful build before execution.

Selected evidence uses headless-v1 for Set/FileThrust, headless-v4 for FindEvents,
headless-v5 for Navigation and visible-v1 for all four contexts. No failing
assertion was disabled or weakened. Every selected proof is bound to its full
process log, passing JUnit case, exact archived source and runtime identity maps.
The runtime plan is not a trace of every loaded library.

Both successful-build dependency archives contain all 149 hash-matching files.
Only 27 intersect the original pre-run input table; the later recovery is
identified separately. Original guards were preserved, with derived receipts
binding the successful build logs and recovered bytes.

Evidence, retention and remaining scope
--------------------------------------

``event-backend-evidence-audit-v4.json`` checks 195 consumed hashes. Independent
review passes the provenance repairs and bounded results. Six negative controls
reject duplicate/missing observations, wrong epoch, covariance-page substitution,
fake zero residuals and an incorrect contact boundary, without rerunning GMAT.

``event-backend-supported-inventory.json`` records bounded witnesses for 46 of
53 factory entries. This inventory count is not a NASA coverage percentage.
Seven applicability decisions remain: BeginMissionSequence, CallFunction,
Create, Equation, NoOp, ScriptEvent and PrepareMissionSequence. They require
source-backed legal-context/alias evidence; unsupported literal syntax alone
cannot silently remove an obligation. Whole FRC!E138 remains open until those
decisions and the existing contained-command witnesses are conjoined.

``event-backend-final-audit.json`` binds this batch and the preceding sealed
source versions. Thirty redundant generated summaries were losslessly compressed,
saving about 51.5 MiB; retained stage allocation before the final source seal was
25.0 MiB. All failures, selected raw results and consumed artifacts remain.

Other unresolved original requirements, external/historical prerequisites,
the critical Brouwer conversion gap and GroundTrack skip boundary remain open.
This is not broader scientific, full current-build or NASA qualification.

Reusable execution
------------------

Build and watch only the four new contexts from the checkout root::

   cmake --build build/linux-gui --target GmatQtNasaScriptEventCommandTests -j 4
   python3 src/qtgui/tests/RunScriptEventCommandAcceptance.py --visible \
     --filter '^QtGui.NasaScriptEventBackend'

Omit ``--visible`` for unattended execution. The helper defaults to 350 ms pauses
and no screenshots; its default filter selects all 31 additional command
contexts. The six original branch tests remain registered separately. Build
success must precede execution. The final helper is supplied for reuse; this
batch's selected executions used the retained private controller and same native
test registrations.

Verify retained evidence without running GMAT::

   python3 src/qtgui/tests/AuditScriptEventCommandAcceptance.py \
     --verify-receipt build/nasa-coverage/coverage95-20261101/event-backend-evidence-audit-v4.json
