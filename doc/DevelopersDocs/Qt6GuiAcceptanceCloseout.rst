Linux GUI acceptance closeout
============================

Repair follow-up — 2026-10-31
-----------------------------

The bounded follow-up repairs all 21 effective failures recorded in the sealed
handoff: Paths/two Debugger checks, then the 18 remaining registrations and five
associated findings. See ``Qt6ParityValidation/remaining-failures-20261031.rst``
for exact passing receipts, replay commands, unchanged assertions and gaps.
The original sealed result below remains historical (1,722 pass / 21 fail),
with its integrity preserved. It is not relabeled as a new all-green suite.
Full current-build/NASA qualification and technical approval remain deferred.

This handoff covers the reusable Linux acceptance suite and the bounded
2026-10-31 execution. It does not declare full NASA qualification or technical
approval of the accumulated core changes. The broader qualification effort is
deferred after this handoff, as requested.

Run the complete suite
----------------------

From a terminal in the logged-in desktop, watch the suite with::

  cmake --build build/linux-gui --config Release --target watch-qt-acceptance

Restore the initially minimized test desktop to watch. It is resizable and
does not enter fullscreen. Automatic observer screenshots are disabled;
images that are themselves result assertions remain enabled.

For unattended execution::

  cmake --build build/linux-gui --config Release --target check-qt-acceptance

Both targets build the acceptance executables, execute the eleven separate
support checks, and then run the 1,732 GUI-catalogue checks. Their selections
are disjoint. The catalogue includes 1,646 visible routes, 47 headless-only
routes and 39 noninteractive support routes. These figures describe the
2026-10-31 catalogue, not percentages of all possible GUI behavior.

The Python runner prints a fresh result directory containing results.json,
report.md and junit.xml. An execution failure returns a failing exit status.
Passing selected tests alone does not mean all NASA requirements are covered.
Use ``--strict-coverage`` with the Python runner when a CI gate must fail on
the remaining coverage gaps as well.

``Qt6GuiAcceptance.rst`` documents explicit output paths, filtering, snapshots,
display scaling and interruption/resumption. Resume frozen runs with their
recorded display profile. A new build requires separate evidence for changed
cases; it cannot inherit a passing result merely by keeping the same test name.

Closeout execution and results
------------------------------

The bounded execution is finished: 1,743 unique registered checks are
accounted for, with 1,722 passed and 21 failed. None is skipped, interrupted,
unexecuted or invalid-evidence in the effective result set. Failed cases may
stop before their later assertions; those unexecuted phases remain explicit
in the failure triage. This is a completed results handoff, not an all-green
suite or NASA certification.

All 895 previously unfinished cases executed: 875 passed and 20 failed. The
retained baseline VNB rate failure makes 21 effective failures. The three
prepared fixture repairs and both changed tutorial binaries passed. All eleven
separate support checks now pass in the versioned result set. The initial
RunnerContract failure is preserved; only that failed check and the directly
affected NasaTraceability check were requalified after the metadata repair
(32 and 41 internal unit tests respectively). The nine other support outcomes
retain their original guarded version. No unrelated passing GUI case was
replayed. The original 833 baseline passes remain archived; the two tutorials
have newer effective evidence because their rebuilt binaries changed.

The machine-readable handoff and per-case evidence are under::

  build/nasa-coverage/closeout-20261031/closeout.json
  build/nasa-coverage/closeout-20261031/case-results.csv
  build/nasa-coverage/closeout-20261031/closeout-junit.xml
  build/nasa-coverage/closeout-20261031/closeout-report.rst
  build/nasa-coverage/closeout-20261031/known-findings.json

To check the retained handoff without replaying tests::

  python3 build/nasa-coverage/closeout-20261031/verify_handoff_evidence.py

This reads the sealed package's file hashes and writes nothing. Exit zero
means the evidence is intact; its output still reports the 21 stored failures.
It does not mean the acceptance suite passed or qualify a newer build.

The generated report explicitly says whether execution is complete. Its
per-case rows identify the receipt and snapshot supplying each result.
It combines preserved passing evidence with newly executed unfinished and
repaired cases; it is not represented as one all-passing run of a single new
build. All original receipts and earlier failures remain available.

Run history remains separate from effective case outcomes. The original
baseline receipt and first pilot receipt remain unfinalized after interruption;
their individual evidence and frozen inputs were verified separately. The
pilot contributed 73 passes, and its recovery executed the other 27 first-100
cases with no replay. That recovery's exit 3 is the runner's documented
case-limit result. The later 795-case phase finished with exit 1 for its twenty
failures. The original support exit 8 and RunnerContract failure remain archived
beside the passing two-check metadata requalification. The final collector
returns 1 for the 21 retained failures, with no integrity/completeness errors.

The interrupted baseline had 833 passes, four failures and 895 unfinished
cases. The closeout executes the unfinished cases, three repaired fixtures,
two tutorials with changed rebuilt binaries, and eleven separate support
checks. It does not replay unrelated passing scenarios. The first resume
attempt regenerated some passing reports, executed zero tests, and was stopped
to select unfinished cases directly.

The fixture repairs preserve every original assertion and tolerance:

* Sky controls follow the documented Stars/Constellations dependency and check
  the disabled control and retained selection before continuing the original
  toggle/cache/reopen assertions.
* Unified resource creation finds the celestial-body radius field by its
  actual widget name; the radius and exact Undo assertions are retained.
* The reverted gravity draft performs the existing no-op Apply action to clear
  the dirty-input latch before closing. Source/history invariants remain
  checked, and two assertions verify the latch transition.

The scheduling correction replaces the old name-based support prepass, which
repeated thirteen GUI cases and omitted two tutorial support cases. The visible
target now includes the same complete support selection and the requested
minimized/no-automatic-screenshot defaults. No production/core repair is part
of this closeout batch.

The metadata repair adds exact existing-case alternatives for 81 omitted
mechanism mappings, with no catch-all, new GUI scenario or NASA scope credit.
Only the global contract-hash reference in NasaGuiTraceability changes with
it. Archived GUI reports keep their old contract version and may still show
the former unmapped entries. ``support-mapping-finished.json`` records the
qualified repair; ``support-requalification-scope-review.json`` explains the
two-check selection. Original receipts and failure history are preserved.

The gravity fixture source is shared with tide branches. Their archived
passing evidence remains tied to the old frozen source/binary version;
updating source-hash references does not requalify those branches on the
rebuilt binary. Full clean/current-build and unattended qualification remain
open. The commands above are reusable complete-suite entry points and will
still fail on the remaining assertions until those defects are repaired.

Scientific defects and review findings
--------------------------------------

These findings remain explicit even if the test infrastructure passes. New
execution failures appear separately in the generated report.

VNB attitude angular rate
  ``QtGui.NativeNasaAttitudeFixedFrameOrbitVelocity`` observes a zero angular
  rate where the independent oracle expects 0.061765286477323995 degrees/s.
  The original rate tolerance is 5e-9 degrees/s. Orientation success does not
  excuse the rate failure. A partial ObjectReferencedAxes derivative repair
  has independent evidence, but actual spacecraft acceleration delivery is
  still missing. A complete repair needs force/epoch/origin, ownership,
  accepted-state and error-path qualification, followed by the original GUI
  case. Evidence: ``stocktake-20261031/vnb-rate-baseline-reproduction.json``
  and ``object-acceleration-derivative-review/`` under ``build/nasa-coverage``.

Yukon false convergence
  An independent native diagnostic reproduces success for contradictory
  constraints whose original violation exceeds the requested 1e-12 tolerance.
  The isolated candidate reconstructs and checks the original constraints;
  the feasible control and twelve unchanged analytic cases pass, and the
  tight contradiction is rejected. The candidate is not applied to production
  or registered in the acceptance catalogue. Promotion requires review,
  registered independent diagnostics and affected GUI verification. Evidence:
  ``stocktake-20261031/yukon-elastic-tolerance-diagnostic.json`` and
  ``stocktake-20261031/yukon-original-bounds-candidate/validation.json``.

Yukon wide infeasibility
  Both the original and isolated candidate throw a Cholesky positive-definite
  exception on the wider contradictory interval. This remains an unresolved
  infeasibility/diagnostic contract, not a passing rejection.

VF13ad report consistency
  Earlier evidence shows a native/CSV optimum of 2 with report values 2.01
  or 1.99 for forward/central differences. A repair must reconcile the provider
  and report state, then pass the original numerical assertions. Proprietary
  provider source is unavailable. This finding is separate from the interrupted
  baseline's four observed failures; final execution results determine which
  registered cases reproduce it during closeout.

  Closeout subsequently reproduced the forward-mode failure: native and CSV
  final variable 1.9999999999999996, optimizer report 2.01. The raw trial table
  includes the final perturbation near 2.01 followed by the accepted value
  near 2. This run stops at that first mismatch; it does not newly qualify
  central mode or later phases. See
  ``closeout-20261031/VF13ad.current-run.triage.json`` for hashed raw evidence.

NotAKnot cold-cache initialization
  Source review found uninitialized cache arrays copied before interpolation
  has populated them. A minimal initialization candidate exists, but is
  unapplied and dynamically unqualified. The populated-cache companion does
  not establish empty/cold-copy safety. Evidence:
  ``stocktake-20261031/core-interpolation-focused-review.json``.

Additional clone-policy failures
  The frozen run also fails ``NativeNasaClonePolicySpacecraftAttitude`` and
  ``NativeNasaClonePolicySpacecraftVisual`` at the combined exact-script and
  configured-property assertion after accepting a clone. Saved evidence proves
  the original edits were accepted (AngularVelocityX 0.2 and ModelScale 5).
  Source inspection identifies an oracle conflict: the clone implementation
  materializes effective OrbitColor/TargetColor defaults when absent from the
  authored script, while this fixture expects only copied explicit assignment
  lines. The actual post-clone script and individual native comparisons were
  not saved before failure, so additional mismatches are not dynamically ruled
  out. Both failures remain active; later history/reopen/runtime steps are
  unqualified. No application fix or assertion relaxation is applied. Evidence:
  ``closeout-20261031/clone-policy-triage/triage.json``. A future targeted
  diagnostic should record those values before the unchanged assertion and
  retain the implicit-palette obligation.

  ``QtGui.ResourceClone`` subsequently fails its exact unchanged-GroundStation
  copied-source check. Its oracle also omits implicit palette assignments,
  but the actual post-clone script/native values are not saved. This remains
  a failed check with the same source-supported conflict and an unclosed
  diagnostic gap; later array/frame/display clone, reopen and runtime checks
  are not qualified by that failed execution. Evidence:
  ``closeout-20261031/clone-policy-triage/ResourceClone.triage.json``.

Command geometry fixture context
  ``QtGui.CommandGeometry`` fails at its first "Template absent" assertion:
  the fixture inserts before a root-level Stop but requests Achieve, which
  NASA's guide permits only inside Target. Current insertion filtering removes
  solver-only leaves at root level. Declaring a DifferentialCorrector does not
  create a Target sequence. The later geometry, reachability and manual-size
  checks were not reached, and no geometry success is inferred. A future
  fixture correction needs a legal Target insertion context while preserving
  every original geometry/source assertion and root-exclusion coverage.
  Evidence: ``closeout-20261031/CommandGeometry.triage.json``.

Debugger independent solver failure
  ``QtGui.NativeDebugger`` fails its ordinary independent solver reference
  before that reference is compared with debugger stepping. The engine log
  reports an ArrayTemplate bounds exception and a Double.gmf location; it
  does not preserve the throwing stack frame or identify which solver loop
  iteration failed. That location alone does not establish a function defect.
  The fixture's temporary reports were removed at exit. Exact cause remains
  unproved; later solver, script-event, Pause/Stop and failure-recovery phases
  are unqualified. Original assertions and bounds remain intact. Evidence:
  ``closeout-20261031/NativeDebugger.triage.json`` binds the engine log, test
  log, JUnit, native trace and frozen fixture source. A follow-up needs
  diagnostic persistence before the unchanged assertion.

  The separately registered ``QtGui.Debugger`` entry subsequently fails the
  same independent-reference assertion with an ArrayTemplate bounds exception.
  Its own log, JUnit, trace and engine log are bound by
  ``closeout-20261031/Debugger.triage.json``. Both registered entries were
  previously unexecuted; this is not replay of a passing case. Shared fixture
  and diagnostic do not prove a unique throwing call site or identical cause.

Tree activation cleanup
  ``QtGui.TreeActivation`` fails rebuilding its Unicode report fixture after
  an edited GroundStation field is reverted. Source inspection shows the same
  sticky dirty-input contract: hasChanges becomes false on text reversion,
  while hasUnappliedInput remains true until Apply/discard; Build guards that
  draft. The fixture does not acknowledge it before rebuilding. The failure
  does not dump the latch or status bar, so this is a source-supported cleanup
  mismatch with a diagnostic limit. Unicode overwrite and append checks are
  not reached. This case explicitly requires offscreen and provides no host
  native-input qualification. Evidence:
  ``closeout-20261031/TreeActivation.triage.json``. A future fixture correction
  must retain every original assertion and byte comparison.

Fallback epoch rendering
  ``QtGui.EpochPlaybackRendering`` measures centroids 359.5 and 363.399 pixels
  for objects expected at the same physical X at epoch 110. The unchanged
  bound is 1.5 pixels. Both unequal-interval fixture motions independently
  interpolate to X=0, and projection is orthographic. The retained evidence
  does not isolate pose resolution, fallback rendering or pixel classification;
  no failing framebuffer/mask is saved. The separate native software-X11
  companion passed, which does not override this fallback failure or qualify
  host GPU behavior. Later reset-arc, vector, tracked-camera and final retained
  sample checks were not reached. Evidence:
  ``closeout-20261031/EpochPlaybackRendering.triage.json``. Follow-up must retain
  every original pixel tolerance and capture direct assertion diagnostics.

Mission and compatibility failures
  ``QtGui.Mission`` expects function-call forms before initializing the
  interpreter, while CommandForm requires a configured/registered function
  context. The failing statement is not printed, so the exact loop entry
  remains a diagnostic limit. All subsequent initialized mission phases
  are unqualified by this failed case.

  ``QtGui.MissionInsertion`` fails a compound check for exact appended source,
  visible retained editor and clean state. The individual values and branch
  iteration are not saved. Its cause is unproved; later branch, retained edit,
  Undo/Redo and Toggle insertion phases remain unqualified.

  ``QtGui.Compatibility`` fails its custom report delimiter check after reusing
  an already accepted standalone ResourceEditor. Current source treats that
  snapshot as applied and documents standalone close-on-success; the workspace
  normally reconstructs it. This supports a lifecycle mismatch, but callback
  value, button state and insert policy were not individually dumped. Later
  compatibility phases are unqualified despite an earlier SPICE phase PASS
  message. All original assertions remain active. Evidence and detailed later
  phase limits: ``closeout-20261031/MissionCompatibility.triage.json``.

Bulirsch reduction controls
  ``QtGui.BulirschControls`` fails its compound literal-source check after
  Apply: the edited script must differ and contain the exact 0.6/0.0001
  assignments. Actual edited source, individual conditions, panel status and
  native reduction values were not dumped. The failure message alone does not
  prove defaults were serialized or isolate a numerical versus source-text
  defect. Later source preservation, configured/nested/direct/runtime/command
  clone checks, Undo/Redo, rollback, Unicode save/reopen and endpoint mission
  phases are unqualified. Evidence:
  ``closeout-20261031/BulirschControls.triage.json``. All original source and
  numerical assertions remain intact; this case explicitly requires offscreen.

Path-settings crash
  ``QtGui.Paths`` exits with signal 11. The process journal confirms the
  matching frozen executable and this case's startup file, with no core dump
  generated. The last native trace records path chooser/tab activity and a
  Set Paths action; there is no throwing call site or assertion text. Exact
  cause and failing phase remain unproved. Later function precedence, report
  and log relocation, startup rollback/export/read/apply/child launch and
  Stop-recovery checks are unqualified. Small temporary script/function/report
  and relocated-log files survived the crash and were copied with verified
  hashes. They show an earlier completed mission with matching relative and
  explicit reports; this does not isolate the later crash. Their association
  uses fixture layout, timestamps and frozen runtime paths; a PID-to-directory
  assignment was not logged. Evidence:
  ``closeout-20261031/Paths.triage.json`` and ``Paths.crash-journal.txt``.
  ``closeout-20261031/Paths-recovered-manifest.json`` indexes the recovered
  artifacts. No rerun, production change or additional core dump was made.

Inspection validation and Kalman rollback
  ``QtGui.Inspection`` requires a callback-captured error for ClearPlot Missing,
  but current form validation can reject the missing XYPlot before invoking
  the commit callback. The failed compound check does not dump the callback,
  source-equality, draft-state or panel-error values. This supports a fixture
  routing mismatch; later generic-command correction and save/reopen are
  unqualified.

  ``QtGui.KalmanFilter`` fails in a 12-entry invalid-edit loop after earlier
  warm/boundary milestones. Each entry requires both an error and unchanged
  source; the failing map and individual values were not printed. Its failure
  message alone does not prove source mutation or a numerical filter defect.
  Remaining rejection entries and the final ordinary-state comparison are
  unqualified. Evidence: ``closeout-20261031/InspectionKalman.triage.json``.
  All original assertions and numerical bounds remain active.

Script editor failure diagnostic
  ``QtGui.ScriptEditing`` fails a compound Save/build/run guard. Its fixture
  searches case-sensitively for "Build failed", while executeMission sets
  "Mission failed — build failed; see Message Window" after rejecting a build.
  The engine log confirms rejection of CommandEcho Invalid. The individual
  saved/editor/report/status values are not dumped, so other conjuncts are
  not independently proved, and the message does not prove invalid execution.
  Later corrected reopen and empty-script guards are unqualified. Evidence:
  ``closeout-20261031/ScriptEditing.triage.json``. Original assertions remain.

Polyhedron and Global scopes
  ``QtGui.Polyhedron`` stops with a generic build failure from its shared run
  helper. The exact invocation, edited source and panel guard values are not
  persisted; earlier malformed/missing mesh failures are expected negative
  phases and are not the terminal failure. Later phases cannot be treated as
  qualified from those earlier runs. The separate --body-source helper is
  outside this default case's execution.

  ``QtGui.GlobalScopes`` expects Global MissingResource to commit and resolve
  at runtime. Current CommandForm prevalidation requires an existing configured
  object, conflicting with that deferred-resolution expectation. Individual
  post-Apply source/panel values are not dumped, and broader NASA authority
  reconciliation remains open. Expected runtime rejection and correction/rerun
  are not reached. Evidence:
  ``closeout-20261031/PolyhedronGlobalScopes.triage.json``. Both cases retain
  all original assertions and bounds.

Optimizer objective uniqueness route
  ``QtGui.NasaOptimizerRuleUnique`` expects Minimize in the insertion chooser
  so it can reject a second objective through Apply. Current MainWindow removes
  that template when the enclosing Optimize already has an objective. This
  source-supported route conflict stops the case before duplicate rejection,
  draft/source/history/model rollback, correction and recovered result checks.
  Chooser prevention does not qualify those later obligations. Earlier
  separate-sequence results are preserved without turning the case into a pass.
  Evidence: ``closeout-20261031/NasaOptimizerRuleUnique.triage.json``.
  All original uniqueness, diagnostic and numerical assertions remain active.

Core scrutiny
--------------

The accumulated change inventory has 88 core files. Eighteen have focused
patch-review records; none has technical approval. The closeout verifies that
all 88 hashes remain unchanged. Whole-file, scientific, external-plugin ABI,
platform and stress qualification remain open.

The exact preserved patches, file hashes and purposes are indexed by::

  build/nasa-coverage/core-change-review/20261031-object-relative-acceleration/inventory.json
  build/nasa-coverage/stocktake-20261031/core-review-brief.json
  build/nasa-coverage/stocktake-20261031/focused-core-review-index.json

The handoff exports ``core-changes.csv`` and ``core-changes.json`` under
``build/nasa-coverage/closeout-20261031``. Every file retains its exact source
and preserved-patch hashes, observed purpose, review domain and outstanding
review obligations. Eighteen have focused reviews and seventy do not; all
eighty-eight remain technically unapproved.

NASA coverage backlog
----------------------

Requirement scopes come from the requirements workbook: the capabilities and
conditions GMAT is expected to support. Historical-operation scopes come from
NASA's 2012 test matrix: particular checks such as Clone, Rename, Apply or
reopen. One scenario can support several requirements and historical operations;
the two inventories are different views of overlapping obligations.

The last audited cumulative collection demonstrates 925/1,158 requirement
scopes (79.9%) and 1,833/2,367 historical-operation scopes (77.4%). These are
versioned, bounded proofs across builds. The denominators overlap; adding them
does not produce a meaningful total test count. They are not current-build
qualification or a claim to exercise every widget/input domain.

The source-linked remaining inventory is::

  build/nasa-coverage/stocktake-20261031/backlog.json
  build/nasa-coverage/stocktake-20261031/stocktake.json
  build/nasa-coverage/coverage-ledger.json

It retains 233 requirement scopes and 534 historical-operation scopes without
credit, including 95 GUI-applicable requirement entries. The audit also retains
42 formal source conflicts and twelve narrow disabled-positive holds. These
categories overlap and should not be summed. The requirements backlog groups
GUI work, source reconciliation, external dependencies and non-GUI obligations.

Further qualification needs independent scientific oracles, the full supported
input domains, native OS input/portal behavior, host GPU/compositor evidence,
Windows/macOS, stress/lifecycle and clean CI/build provenance. The available
NASA requirements corpus is the shipped 2012 baseline; current private NASA
sources were not available. Missing authority is not interpreted as retirement
or permission to waive a requirement.

Before resuming broader qualification, select a named backlog work package,
define its exact exit evidence and estimate it separately. Keep application
repairs, new acceptance coverage and technical approval separately reviewable.

The handoff's ``prioritized-backlog.json`` and ``prioritized-backlog.rst``
preserve all 233 requirement and 534 historical source keys/fingerprints.
They prioritize known scientific defects, core scrutiny/source reconciliation,
independent numerical domains, remaining finite GUI operations, and external
or platform obligations. These priorities describe deferred work; creating
the backlog adds no coverage credit and starts no additional tests.
