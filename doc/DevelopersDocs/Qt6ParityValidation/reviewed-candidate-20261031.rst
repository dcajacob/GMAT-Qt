Bounded Linux GUI acceptance handoff
===================================

Baseline: all 1,753 unique registrations accounted for; 1,741 actually executed. Corrected counts: {'passed': 1717, 'failed': 24, 'not-executed-source-conflict': 12}. Twelve preexisting disabled source conflicts did not execute. The original parser incorrectly called those passes; the raw receipts remain unchanged and baseline-result-correction.json records the correction. Candidate inputs stayed unchanged throughout the sweep.

After repairs, 95 affected checks executed: {'passed': 95}. Reused passing baseline checks: 1646, retaining their original source/binary versions. Effective registered outcomes: {'passed': 1741, 'skipped': 12}. Remaining active failures: 0. The twelve disabled cases remain named coverage gaps.

This is a complete registration accounting, with twelve explicit nonexecution gaps, plus a targeted rebuilt repair pass. It is not a claim that every final binary passed an uninterrupted full replay. The original full baseline and every failed diagnostic remain preserved. Broader NASA qualification is deferred.

Repairs and core scrutiny
-------------------------

* Activate the native parent before synthetic tree-context and batch-format popup input. Four test drivers change; their original assertions stay intact.
* Classify CTest status=disabled as skipped/unexecuted, never passed. New parser and real-CTest regression checks preserve the existing source-conflict exclusions and all assertions.
* Select each plot-command editor by its unique mission-node identity, then activate it. Two test drivers change; exact source and uniqueness checks are added.
* Add explicit geometry-only ObjectReferenced orientation for covariance consumers. Estimator VNB covariance and both process-noise time overloads request that matrix instead of an unused acceleration-dependent frame rate. Normal rate calculations retain the strict accepted-epoch and live-force checks.

Six core files change in this followup; the complete core/plugin/utility review inventory now contains 97 files relative to HEAD. No independent technical/scientific approval is claimed. See core-inventory/review.md, orientation-fix.patch and the archived before sources. No solver, integration or smoother state-selection formula is rewritten by this followup.

All 1190 original assertions across 16 original fixtures retain their exact call text and multiplicities. Added tests check 864 geometry-only cases with throwing acceleration providers, no live-force calls at a nonaccepted epoch, cached-rate preservation, converter direction and FK5/ICRF round trip. All 864 existing full-rate rotation/derivative bit patterns match the baseline.

The newly added ICRF support fixture initially failed because coordinate copies deliberately clear runtime reference pointers. Rebinding its solar system, origin and J2000 body fixed the setup; every old and new assertion remained unchanged. Only that failed registration was rerun. The final fixture source/executable are the only two inputs differing from the preceding frozen repair pass; all production libraries and GUI test binaries retain identical bytes.

Visibility and limits
---------------------

The baseline and repair desktops started minimized, resizable and nonfullscreen, using the host-aware scale. Restore the window to watch. No automatic screenshots or full binary snapshots were retained. Explicit visual-assertion artifacts and numerical/runtime evidence remain available. Native Qt exposure does not qualify every intermediate screen, physical OS input, host GPU or other operating systems.

Visible repair checks with zero native exposure: [].

The same twelve disabled raw records are also present in the earlier sealed handoff. Its stored labels remain untouched; older-sealed-disabled-observations.json records the corrected interpretation. Sealed artifact integrity and execution semantics are separate checks.
The original 2012 requirements workbook/historical corpus remains the available source baseline; current private NASA material is unavailable. Requirement scopes, historical operation scopes, regression registrations and complete widget/physics qualification are distinct measures. The separate run estimates are retained in repair-completion-audit.json; they are not added together or relabeled as current full-NASA coverage.

The handoff's source-versioned evidence verifies 742/1158 requirement scopes (64.1%) and 1803/2367 historical operation scopes (76.2%). Each included scope was already verified within one report; partial fragments are not combined. This union is not current-single-binary or full NASA qualification. Earlier project-ledger versions outside this handoff are not merged.
All remaining coverage gaps
---------------------------

* SUBUNIT-DPI-RASTER: A manual 0.5x XY raster check still fails the original full-color marker oracle after logical-coordinate normalization. Subpixel raster coverage at scales below 1 remains unqualified; registered XY checks cover 1x, 1.5x, 2x and 3x, and a manual 4x check passed. This is retained as a gap, not converted into a passing check.
* PER-CONTROL-ORACLES: No verified assertion linkage for every individual widget, menu action and control state. Existing executable assertions are reused and their source files/logs are linked.
* INPUT-BOUNDARIES: Valid, invalid and boundary inputs are checked in existing scenarios; a reviewed input-domain matrix for every editable field is incomplete.
* DYNAMIC-UNNAMED-CONTROLS: Runtime traces include encountered unnamed widgets, but the static inventory cannot enumerate every generated property row, menu choice, delegate or platform-native control.
* NATIVE-INPUT-PORTALS: In-process Qt events and calls do not verify OS mouse/keyboard routing, native desktop file portals or accessibility. Native-dialog suppression in existing tests is preserved.
* PHYSICAL-GPU-LIFECYCLE: Physical Intel/Wayland viewer close/reopen, resize, minimize/restore and normal exit remain distinct acceptance items. Software GL/XWayland/private-display passes do not close them.
* SCIENTIFIC-INTEGRATION-HOLDS: See current-acceptance-20261002.md for held tutorial/example objectives, unsupported scientific regimes, legacy data and proprietary/MATLAB dependencies. These are not silently skipped as passes. Custom rotation fixtures isolate frames with an Earth-coincident origin; 1e-8 km relative-input precision at distant astronomical origins remains unqualified, with Ceres-offset failure reports preserved.
* TUTORIAL-CORPUS-REPLAY: This suite runs registered regression checks. Prior tutorial/corpus evidence is linked in the acceptance ledger but their complete authored GUI replay is not automated here.
* STRESS-PLATFORM-MATRIX: No full-duration stress/soak, Windows/macOS distribution matrix or all hardware/scale/input combinations is established by this run.
* ARTIFACT-PROVENANCE: Receipts hash source, binaries and startup inputs; matching source-to-binary provenance requires a controlled build/release receipt.
* VISIBLE-STEP-COVERAGE: Automatic visible checkpoints are capped per process. Fast modal callbacks and intermediate edits can finish before native exposure; complete step-by-step visual replay still needs explicit scenario checkpoints. Assertions still execute for those steps.
* WINDOW-CONTENT-VISIBILITY: Native exposure does not prove that every control is inside the visible workspace. An earlier host-scale SearchRouting capture exposed off-edge MDI placement; that fixture now asserts viewport containment. Other fixture layouts, popups, readable presentation and all supported scale combinations still need direct visibility assertions.
* SUBMICROSECOND-STOP-PRECISION: The Propagate guide documents epoch time quantization and a warning when requested stopping tolerance cannot be met. The new variational cases request 1e-9 s and observe about 6.98e-8 s reported stop error; they verify the warning and its requested/achieved values. Their state/STM oracles use the actual integration clock. Passing these quantity checks does not qualify nanosecond stopping precision.
* LIBRATION-VELOCITY-EPHEMERIS-PLANE-DERIVATIVE: The shipped mathematical specification explicitly assumes the relative-acceleration terms are zero in libration velocity, and SpacePoint supplies zero acceleration. New six-state checks qualify that local linear model using an independent position derivative. They do not qualify the full derivative of changing ephemerides or arbitrary nonplanar/nonzero-acceleration primary-secondary motion.
* HISTORICAL-USER-BODY-TWO-BODY: Historical user-body two-body and orbital-element requirements conflict with the current shipped SPICE-only guide and explicit runtime deprecation/read-only guards. The source instances remain reviewed-unverified pending authoritative baseline/retirement reconciliation; no test pass substitutes SPICE for the named removed mode.
* SOLVER-MATHEMATICAL-SPEC-INCOMPLETE: The shipped Solvers.tex has empty Differential Correction, Broyden and Newton sections. The new solver cases derive independent finite-difference/Newton oracles for one specified polynomial problem and check all presentation values; they do not replace a complete authoritative NASA numerical solver V&V corpus or qualify general conditioning, scaling, bounds, Broyden or dynamical targeting.
* SPK-PENDING-TYPE-AND-ORIGIN: Pending drafts, spacecraft kernel selection/cache, mixed numeric/SPK modes and configured STM/A-matrix/covariance/cardinality boundaries have bounded cases. Remaining: inactive Origin, other segments/general frame-time science, native input/drag-drop, user-body SPK state/cache/precedence (configured list controls now have separate bounded cases) and full current regression.
* PROPAGATE-GROUP-CAPACITY-AND-EPHEMERIS-COUNT: FRC!E113 unlimited numerical groups requires capacity/stress qualification. FRC!E114 historical ephemeris-count wording needs resource-versus-clone reconciliation with current guide/examples and repeated/distinct SPK support. Both reviewed-unverified.
* USER-BODY-ROTATION-DRIFT-DISPOSITION: FRR!E227 historical rotation drift rate and parent E221 remain reviewed-unverified: current guide/core supports spin angle and constant spin rate, with no separate spin acceleration. Bounded orientation GUI/analytic-state cases do not resolve historical nomenclature or retirement.
* VF13AD-FINAL-TEXT-REPORT-TRUTH: Original report-truth failure repaired on 2026-10-31 without changing assertions or numerical bounds: Optimize/Vary refresh the physical reporting cache before internal-solver completion. The original visible ReportTruth and new Forward/Central scaled/unscaled contracts pass; proprietary provider binary unchanged. Remaining gap: arbitrary optimizer providers, nested/dynamic scaling/report schemas and full current-build qualification. See Qt6ParityValidation/remaining-failures-20261031.rst and repair receipts; selected passing evidence is not general provider qualification.
* UNMAPPED-TESTS: Registered tests lack a mechanism mapping.
* VISIBILITY-GUARDS: These tests still execute, but require offscreen Qt.
* UNOBSERVED-NAMED-CONTROLS: No runtime observation of these literal object names. Dynamic/unnamed controls are an additional gap.
* NASA-CASE-TRACEABILITY: 564 historical operation scopes lack verified evidence within this source-versioned handoff. Unspecified input domains and unmatched cases remain open; this is not current-single-binary qualification.
* NASA-REQUIREMENT-TRACEABILITY: 416 workbook requirement scopes lack verified evidence within this source-versioned handoff. Historical operation scopes are counted separately; broad and current-private NASA qualification remains open.
* NASA-SOURCE-CONFLICTS: Unresolved source contradictions remain in nasa-traceability.md; setting-property proofs do not close their input-domain decisions.
* NASA-REQUIREMENTS-BASELINE: Shipped workbook modification metadata: 2012-05-06T14:39:36Z. Its completeness against current documented features and the NASA requirements/test corpus remains unqualified.
* DISABLED-SOLVER-SOURCE-CONFLICT: Twelve preexisting positive nested-solver hypotheses do not execute: NASA menu and native interpreter behavior disagree. Original assertions and CMake exclusions retained; named below.
* FINAL-BINARY-FULL-REPLAY: The final rebuilt binaries have affected-scope evidence. Unaffected passes retain their baseline binary/source versions; no uninterrupted all-green final-binary replay is claimed.

Additional held findings include the manual 0.5x XY raster failure, distant-origin precision, broad scientific/frame/force domains and implicit force/attitude coupling. Smoother state/covariance scientific correctness beyond the preserved test oracles remains unqualified. These are not erased by resolving registered test failures.

Disabled source-conflict cases
------------------------------

* QtGui.NativeNasaSolverLeafVaryTargetBranchAppend: did not execute; original positive assertions retained, NASA menu/interpreter conflict unresolved.
* QtGui.NativeNasaSolverLeafVaryOptimizeBranchAppend: did not execute; original positive assertions retained, NASA menu/interpreter conflict unresolved.
* QtGui.NativeNasaSolverLeafAchieveBranchAppend: did not execute; original positive assertions retained, NASA menu/interpreter conflict unresolved.
* QtGui.NativeNasaSolverLeafAchieveEndBefore: did not execute; original positive assertions retained, NASA menu/interpreter conflict unresolved.
* QtGui.NativeNasaSolverLeafMinimizeBranchAppend: did not execute; original positive assertions retained, NASA menu/interpreter conflict unresolved.
* QtGui.NativeNasaSolverLeafMinimizeEndBefore: did not execute; original positive assertions retained, NASA menu/interpreter conflict unresolved.
* QtGui.NativeNasaSolverLeafMinimizeLeafAfter: did not execute; original positive assertions retained, NASA menu/interpreter conflict unresolved.
* QtGui.NativeNasaSolverLeafMinimizeLeafBefore: did not execute; original positive assertions retained, NASA menu/interpreter conflict unresolved.
* QtGui.NativeNasaSolverLeafNonlinearConstraintBranchAppend: did not execute; original positive assertions retained, NASA menu/interpreter conflict unresolved.
* QtGui.NativeNasaSolverLeafNonlinearConstraintEndBefore: did not execute; original positive assertions retained, NASA menu/interpreter conflict unresolved.
* QtGui.NativeNasaSolverLeafNonlinearConstraintLeafAfter: did not execute; original positive assertions retained, NASA menu/interpreter conflict unresolved.
* QtGui.NativeNasaSolverLeafNonlinearConstraintLeafBefore: did not execute; original positive assertions retained, NASA menu/interpreter conflict unresolved.

Failed baseline case dispositions
---------------------------------

* QtGui.NativeNasaElseParentControlForElseParent: failed -> passed
* QtGui.NativeNasaSolverTreeTargetInterior: failed -> passed
* QtGui.NativeNasaCommandTreePropagateRoot: failed -> passed
* QtGui.NativeNasaCommandTreePropagateInterior: failed -> passed
* QtGui.NativeNasaCommandTreeManeuverBoundary: failed -> passed
* QtGui.NativeNasaCommandTreeToggleRoot: failed -> passed
* QtGui.NativeNasaCommandTreeToggleInterior: failed -> passed
* QtGui.NativeNasaCommandTreeSaveBoundary: failed -> passed
* QtGui.NativeNasaCommandTreeBeginFiniteBurnRoot: failed -> passed
* QtGui.NativeNasaCommandTreeEndFiniteBurnRoot: failed -> passed
* QtGui.NativeNasaCommandTreeEndFiniteBurnBoundary: failed -> passed
* QtGui.NativeNasaCommandTreeGlobalRoot: failed -> passed
* QtGui.NativeNasaFilterFormatSmootherCode500: failed -> passed
* QtGui.NativeNasaFilterFormatSmootherStk: failed -> passed
* QtGui.NativeNasaBatchFormatStkKilometers: failed -> passed
* QtGui.NativeNasaBatchFormatStkMeters: failed -> passed
* QtGui.NativeNasaBatchFormatCode500Little: failed -> passed
* QtGui.NativeNasaBatchFormatCode500Big: failed -> passed
* QtGui.KalmanFilter: failed -> passed
* QtGui.PropagationCovariance: failed -> passed
* QtGui.NativeNasaPlotFieldControlPenUp: failed -> passed
* QtGui.NativeNasaPlotFieldControlPenDown: failed -> passed
* QtGui.NativeNasaPlotFieldControlClearPlot: failed -> passed
* QtGui.NativeNasaPlotFieldControlMarkPoint: failed -> passed

Reproduce a deliberate full run
------------------------------

These commands are documentation; the handoff did not replay the full suite
after its affected-scope repair. Use new output names. From the repository root::

   cmake --build build/linux-gui --target build-qt-acceptance -j4
   python3 src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
     --visible --start-minimized --max-screenshots 0 \
     --output build/gui-acceptance/NEW-ID
   ctest --test-dir build/linux-gui -C Release \
     -L '^qt-(acceptance|tutorial)-support$' -LE '^qt-gui$' \
     --no-tests=error --output-on-failure \
     --output-junit build/gui-acceptance/NEW-ID-support.xml

The catalogue and the exclusive support label selection are disjoint: 1,732
catalogue registrations plus 21 exclusive support registrations. Twelve
catalogue source-conflict registrations remain disabled and report as skipped,
not passed. Zero active failures does not imply complete NASA coverage.

Restore the minimized nested desktop to watch, resize it normally, and close it
after completion. Host-aware scale is the default; fullscreen is not used.
For slower observation add ``--pause-ms 350 --max-checkpoints 5000``. The default
checkpoint cap limits pacing overhead and does not promise every intermediate
control is readable. Omit ``--visible --start-minimized`` for automated execution.
``--strict-coverage`` exits nonzero while any coverage gap remains, including
broader qualification gaps intentionally deferred here. It is stronger than an
active-regression pass gate. Read ``--help`` for filters and other options.
Do not use ``--snapshot`` casually: it copies binaries/runtime data. This handoff
used input hashes and retained numerical/runtime evidence without that copy.

Review and evidence index
-------------------------

All paths below are relative to
``build/release-check/20261031-reviewed-candidate/``.

* ``final-report.rst`` and ``repair-completion-audit.json``: corrected counts,
  every failed baseline case disposition, preserved assertions and named gaps.
* ``effective-junit.xml`` and ``effective-outcomes.json``: derived source-versioned
  accounting, not a claim of one uninterrupted final-binary run.
* ``candidate.json``, ``candidate-integrity.json`` and ``catalogue/``: frozen
  baseline identities, raw JUnit/results and exact runtime/visibility evidence.
* ``baseline-result-correction.json``: the twelve disabled results were
  incorrectly called passes by the former parser. Original receipts are intact.
* ``repaired-candidate.json``, ``repair-catalogue/`` and ``repair-support.xml``:
  the rebuilt 89-case GUI/catalogue pass and its initial support results.
* ``extra-support.xml``: runner and snapshot contracts, including real CTest
  disabled-case reporting and cache/reuse behavior.
* ``fixture-force.xml`` and ``fixture-final-candidate.json``: successful rerun
  after the new ICRF fixture binds its required runtime references. Only the
  fixture source and executable differ; production/GUI binaries are identical.
* ``handoff-source-scope-union.json``: complete verified rows from the two
  reports, with source versions retained. No partial proof fragments are joined.
* ``core-inventory/review.md`` and ``core-inventory/inventory.json``: all 97
  changed core/plugin/utility files relative to HEAD, pending technical review.
  That inventory includes earlier work and does not assign all changes to this
  task. This followup changes six core files.
* ``package/final-core.patch``, ``final-test-fixtures.patch``, ``final-runner.patch``
  and ``final-metadata.patch``: independently readable final increments, with
  before/after identities in ``final-input-delta.json``. They are already applied
  in this worktree. Their baseline is the frozen candidate, not HEAD; do not
  apply them blindly to a different checkout. Read-only reverse-apply checks
  are recorded in ``final-patch-checks.json``. Earlier package files describe
  the previous bounded repair goal and remain historical.
* ``older-sealed-integrity.log``: 36,174 older retained files verified read-only,
  executing zero tests. ``older-sealed-disabled-observations.json`` corrects
  interpretation of its same twelve disabled records without altering the seal.

The permanent before sources are under
``doc/DevelopersDocs/Qt6ParityValidation/reviewed-candidate-20261031/source-baseline/``.
No commit, merge, deployment, clean-release provenance or independent scientific
approval is represented by this developer handoff.

Retained evidence integrity
---------------------------

Verify the retained handoff without launching tests or rewriting receipts::

   python3 build/release-check/20261031-reviewed-candidate/verify_final_handoff.py

This checks the files bound by ``final-handoff.json`` and symbolic-link targets.
It executes zero product tests and does not qualify a later changed current
build. Temporary diagnostic executables/objects were removed after normal-build
qualification; their sources, recipes, hashes and logs remain. Current normal
application/test binaries and every raw registered-test receipt remain intact.
