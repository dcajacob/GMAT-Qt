GUI acceptance failure fixes, 2026-10-26
========================================

The eleven failures from the initial comprehensive acceptance run were fixed
in production code. The 136 registered C++ test sources were left unchanged.
The focused rerun passed 23/23 checks, including all eleven original failures
and related batching, reference-validation, source-preservation and resource
round-trip checks.

The subsequent full visible-mode run passed **183/183**, with all eleven
original failures passing. It recorded native window exposure for 123 cases
and saved 478 snapshots. The runner's owned Xephyr server stopped normally.

Causes and changes
------------------

* Seven plot failures shared a subscriber/receiver contract mismatch. XY
  ``Deactivate``/``Activate`` suspend redraw for owned residual batching; they
  must keep accepting residual samples. Subscriber Toggle now also sends
  explicit ``ToggleOff``/``ToggleOn`` actions to control data admission and
  break the trajectory across the disabled interval. Residual batching retains
  its original behavior. The affected checks are SolverToggle,
  OptimizerToggle, EphemerisToggle, SolverEphemerisToggle,
  BinaryEphemerisToggle, TogglePlots and CompletedDisabledPlots.

* ObjectDrawing merged pending per-object display settings after pruning
  removed objects, allowing stale names to return. Pruning now happens after
  the merge. Drawing and sensor-mask dialogs also use the current pending
  object list rather than overwriting Advanced edits with an unchanged
  primary-form value.

* EventLocatorCreation rejected a new ContactLocator with a valid region
  target because its spacecraft observer had not been created yet. Creation
  can retain that incomplete reference configuration. Explicit observer edits
  and edits to existing locators still enforce the observer constraints. This
  establishes creation and save/reopen behavior; it does not establish a
  scientific event-search result for an incomplete resource.

* PluginCreation exposed a missing distinction between incremental GUI
  creation and the direct creation API's required-target contract. The
  interactive New dialog can create an event locator when no default target
  exists and explains that a target must be configured before searching.
  The API defaults to requiring references; explicit cleared/invalid targets
  remain errors. Available defaults still persist explicitly in the source.
  No implicit spacecraft is created.

* ExternalForces had already passed its numerical comparisons. Its failure
  came from a module assignment retaining aligned spacing while the source
  assertion required the canonical edited assignment spelling. Editing an
  external-force module now normalizes the horizontal spacing before the
  equals sign. Comments, continuations, other force contributors, unknown
  fields and mission-time source remain preserved by the mapped-source
  patcher. The original numerical and source assertions remain intact.

Evidence
--------

The evidence directory is
``build/gui-acceptance/20261026-failure-fixes/``. It contains the original
worktree snapshot, before-source copies, the task-specific ``fixes.patch``,
build logs, both focused runs, diagnostic output, the eleven-failure map and
``assertion-integrity.json``. Concurrent tutorial-probe work is outside these
183 regression checks and was left untouched.

Final verification also confirmed that the 139 selected source files, test
executables, startup file and CTest registrations retained their hashes
throughout the visible run. Unselected tutorial tools changed concurrently;
those changes are listed separately in ``validation-summary.json``. This is
not a controlled release-build or complete provenance qualification.

The full visible rerun is recorded separately in
``build/gui-acceptance/20261026-failures-fixed-visible/``. Its ``report.md``
and ``results.json`` are the authoritative results, with logs, screenshots,
runtime traces, JUnit and the unchanged coverage-gap inventory. The earlier
172-pass/11-fail run remains in ``20261026-qualified-run``.

Remaining coverage gaps
-----------------------

All fourteen coverage-gap categories remain open in the generated report.
Regression success does not establish complete NASA GUI acceptance.

* All 2,367 historical NASA matrix rows still need reviewed case-level
  applicability and traceability. The 26 local mechanism groups do not close
  those rows. Individual-control assertion linkage, complete input boundary
  domains and dynamic/unnamed controls remain incomplete.
* 47 cases require offscreen Qt; five are noninteractive support checks.
  Eight visible-route cases recorded no native exposure: PropSetupApply,
  BurnOrigin, AxisRename, ResourceVisibility, CameraReset, MarsGravity,
  ImplicitGravity and PythonScalarOutput. Fast callbacks and capped automatic
  checkpoints also leave intermediate visible steps unverified.
* OS mouse/keyboard routing, accessibility and native file portals are not
  qualified by in-process Qt events. Physical GPU/Wayland lifecycle, platform
  and scaling combinations, and full-duration stress/soak remain separate.
* Complete authored tutorial/corpus GUI replay, held scientific integration
  regimes and proprietary dependencies remain open. Incremental creation of
  an incomplete locator is not evidence of a valid FindEvents result.
* Controlled source-to-binary release provenance remains unqualified.

The generated report lists every gap, affected test and named-control source
location; ``validation-summary.json`` records the final integrity checks.

Repeat the comprehensive suite with::

  python3 src/qtgui/tests/RunAcceptance.py --build --visible

See ``Qt6GuiAcceptance.rst`` for pacing, unattended operation, strict coverage
handling and the distinction between regression success and NASA acceptance.

Integrator validation found during coverage expansion
----------------------------------------------------

The new seven-method control test exposed two further validation defects.
Individual engine setters allowed MinStep to exceed MaxStep and allowed
nonpositive or unordered predictor-corrector error bounds. Conversely, the
old sequential application could reject a valid downward change of both
step limits because MaxStep was assigned before the old MinStep changed.

The Qt preview now applies the step pair together and validates the final
relationship before committing. Predictor-corrector settings require
0 < LowerError < TargetError < Accuracy. Rejected input preserves source,
configured values and Undo history. The original PropSetupApply assertions
remain intact. All seven methods pass valid/invalid controls, exact Undo/Redo,
save/reopen and a 600-second independent circular-orbit comparison. The full
248-case unattended and full visible snapshots both passed and were audited.
The additive five-mode companion passed both routes too. See
Qt6NasaCoverage.rst for the remaining coverage limits.

Accuracy-policy runtime fixes
-----------------------------

The expanded test found that Adams-Bashforth-Moulton and Bulirsch-Stoer
reported an accuracy warning but then failed the mission even when
StopIfAccuracyIsViolated was false. Predictor-corrector retried the same
unaccepted correction until the attempt limit; Bulirsch returned failure
from its warning branch. Both now accept and advance the computed minimum
step only when the user explicitly permits an accuracy violation. The warning
is retained; true still stops before the following mission command.

Bulirsch rejection now honors the minimum step. Its loose-tolerance depth
selection also retains the three extrapolation levels required by its own
convergence test, preventing an endless retry loop. Predictor-corrector keeps
its adaptive-step sign synchronized with actual forward/backward propagation,
preventing a backward reduction from restoring a forward step.

The new tests exercise 70 policy scenarios across all seven integrators,
including fixed-step bypass, loose tolerance, direct and reduced minimum
steps, both directions, post-propagation reachability, and sampled step bounds.
All original error-control test lines remain. The initial test-development
corrections and actual engine failures are distinguished in
``build/nasa-coverage/accuracy-behavior/failure-review.json``. The 70 scenarios
and existing nominal propagator checks passed. Those 250-check snapshots
were superseded by the short-final-step correction below.

Short-final-step correction
---------------------------

An additional off-grid stopping test showed that the initial minimum-step
repair could complete at 2100 seconds when 1850 was requested. The first
registered policy cases used targets divisible by MinStep and did not expose
this interaction. The old 250-case visible run was stopped and its results
retained rather than presented as final validation of the revised engine.

The RK, predictor-corrector and Bulirsch interval drivers now use a temporary
minimum for a short final event interval and restore the configured bound on
exit. The Nystrom driver delegates to the shared RK interval driver. A further
backward ABM case exposed a signed step-growth comparison that could exceed
MaxStep; it now compares magnitudes. Diagnostic instrumentation was removed.

The terminal test checks 84 cases: all seven methods, forward/backward,
warning-only/fixed/loose-tolerance behavior, and requested times of 50 and
1850 seconds with a 300-second minimum. Backward first-step cases use a
positive InitialStepSize with BackProp; later-stop cases also cover negative
InitialStepSize. Exact target arrival, the following mission command, finite
output, sampled bounds and restoration of configured MinStep/MaxStep are
asserted. The 77 other policy cases and original five analytic mode checks
remain. The console reproducer now reports 1849.9999999534339 seconds.

All focused preflight checks pass. The full 251-case unattended snapshot
passed its receipt audit, retaining 186 earlier same-snapshot passes and
executing 65 remaining checks after a SIGTERM interruption. The full visible
snapshot remains in progress. Broad scientific/platform qualification stays
open independently of this fix.

Gravity degree/order Apply validation
-------------------------------------

A new GUI regression showed that Qt accepted Degree 3 / Order 4. The wx
propagation panel rejects Order greater than Degree, while the engine can
truncate Order later during initialization. Allowing the GUI to commit this
pair therefore lost the intended validation step.

The pending-model validator now checks the final coupled values before source,
model or history changes. The check remains active when the same transaction
replaces polyhedron contributors. The new test contains 12 GUI cases covering
valid paired updates, reversed bounds, negatives, fractions and non-finite
inputs, with correction and exact Undo/Redo, plus one mixed-polyhedron
transaction check. All 11 focused regressions passed after rebuilding.

The ForceModel help table reverses the inequality. Its original text remains
unchanged and the discrepancy is recorded as GRAVITY-ORDER-INEQUALITY; the
implementation follows the wx GUI contract. The new 255-case full validation
is pending. The older visible suite uses its own pre-fix snapshot.

Gravity degree/order limits from the selected file
-------------------------------------------------

The new file-boundary test reproduced an accepted invalid Apply: degree 71
with the shipped JGM2 potential, whose POTFIELD header declares 70 by 70.
The existing Order <= Degree check does not catch this. The engine rejects
such a model when initialized later, after the invalid GUI edit has already
been committed.

``validateResourceProperties`` now reads the selected potential metadata
through the existing HarmonicGravity parser with coefficient loading disabled.
It compares Degree and Order to that file's independent limits on the detached
pending model before source commit. The original Order <= Degree guard remains.
Invalid input retains its pending text, diagnostic and unchanged model/source
history; correction can then be applied normally.

``QtGui.NasaGravityValidation`` retains every original assertion and adds
16 cases: file degree boundaries, independent order boundaries with a derived
70-by-2 COF fixture, StmLimit zero/default/maximum integer, negative/fractional/
empty/nonnumeric/nonfinite input and integer overflow. All 16 and the original
13 proofs passed the focused unattended run. Ten related force, atmosphere,
clone, window and external/polyhedron regressions passed on the rebuilt frontend.
The focused visible run also passed all 29 proofs and retained 102 PNGs.
The full 255-check unattended rebuild is running and its visible replay is
queued; the prior preserved visible run is allowed to finish as historical
build evidence. No new scientific-model accuracy claim follows from these checks.

Tide input reader and file validation
------------------------------------

Isolated transaction diagnostics found three distinct defects: a directory
named with a .tide suffix spun in ``HarmonicGravity::LM_LoadTide``; an empty
file silently fell back to built-in Earth values; and a wrong-extension file
was silently ignored. The stopped diagnostic's stack is preserved in
``build/nasa-coverage/tide-review/hang-stack-symbols.log``. The intermediate
full qualification run was stopped and preserved so these fixes can be
qualified together, rather than treating its passing subset as completion.

The parser now rejects directories and unsupported tide extensions, accepts
extension case consistently, terminates on stream read failure, requires at
least one coefficient, and rejects incomplete/nonfinite coefficient records.
Normal built-in tides still work without a file. The existing numerical tide
formulas and coefficient precedence are unchanged.

The expanded ``QtGui.NasaGravityValidation`` keeps all preceding assertions
and adds 23 editor cases for Earth/lunar model availability, file selection
and Cancel, invalid file/content handling, correction, atomic Apply and exact
Undo/Redo. These join the 13 original and 16 selected-file/STM checks for 52
proofs. Full rebuilt qualification remains pending.

The first visible tide-picker test waited indefinitely after calling
``selectFile`` followed immediately by ``accept``. The corrected test fills
the displayed filename entry, clicks the enabled Open button and asserts
that the dialog accepted. All existing assertions remain. Its 52 proofs pass
unattended and visibly; the visible rerun retained 160 PNGs. Only this test's
source/binary differ from the full unattended snapshot, so its corrected
result can replace that one result without re-running the other 254 checks.
The source-delta audit is ``build/nasa-coverage/tide-review/harness-review.json``.

Dependent gravity and tide controls
----------------------------------

``Gravity and tides…`` now exposes the tide source and its dependent controls
described in NASA's ForceModel help and the wx PropagationConfigPanel. Model
choices and degree/order limits come from the selected potential/tide files.
None clears model/file settings; Inherited clears the external tide file;
Tide File enables its picker and requires a file. Lunar COF without Love
numbers does not offer inherited tides, and lunar file data does not offer
Earth-only pole tides. The source choice is transient GUI state and is not
written as a synthetic script property.

Dialog OK keeps edits pending in the parent ForceModel transaction. Cancel,
body switching, reopening, reverting edits, adding/removing a pending primary,
invalid input correction and both file pickers are covered by 12 new control
proofs. Four additional mission proofs check actual propagator-owned settings
and equivalent-script six-component states after 600 seconds. All 16 proofs
pass unattended and visibly. The preserved-build audit verifies five new
passing cases in each mode, 16 structured proofs, matching project inputs,
native exposure for each visible case and 56 valid PNG snapshots. None of the
255 existing checks was selected in these focused qualification runs.

The new cases have separate CTest names so development can select only the
unverified case. For example::

   ctest --test-dir build/linux-gui -C Release -R '^QtGui.NasaTideLunarFile$' --output-on-failure

To watch that single case in a minimized, resizable desktop::

   python3 src/qtgui/tests/RunAcceptance.py --visible --start-minimized --filter '^QtGui.NasaTideLunarFile$'

The older full visible replay was intentionally stopped at 50 passes on its
superseded snapshot. No new full suite is started during this feature batch.
Independent tide accuracy, native OS input/portals, all file/input domains,
and broader platform/hardware/stress qualification remain open.

User-defined harmonic-gravity metadata
------------------------------------

The new ``QtGui.NasaUserGravity`` workflow reproduced rejection of a valid
custom Ceres gravity configuration with ``Property is read-only``. The gravity
dialog retains TideModel=None while validating its detached candidate, but
the engine metadata marked that property read-only for every body except
Earth, Luna, Mercury, Venus and Mars. The documented capability and existing
SetStringParameter/HaveTideModel validation are data-dependent for all bodies.

``GravityField::IsParameterReadOnly`` now exposes TideModel for every body.
The setter still rejects unsupported choices. The new case checks None in
the GUI transaction, Solid with supplied custom-body Love numbers, rejection
of Solid without data, and rejection of non-Earth pole tides. The production
diff changes only this metadata and retains the numerical force equations.

The same new acceptance case covers user-primary selection, custom potential
selection/Cancel, over-file rejection/correction, source preservation,
Undo/Redo, Unicode persistence, actual running-force settings, a same-engine
script comparison and an independent synthetic equatorial J2 orbit. Both
unattended and visible runs pass; none of the other 260 cases was selected.
The final full regression run remains deferred to the completed batch.

Variational acceptance oracles and visible editor selection
----------------------------------------------------------

Two new independently selectable cases verify Cartesian state and all 36
orbit STM entries for signed 600-second circular point-mass arcs. The existing
covariance and propagation tests are unchanged. Exact circular/variational
solutions, runtime matrix/report agreement, the canonical symplectic form and
disabled-identity controls provide result oracles beyond flag readback.

The initial new state oracle assumed exact arrival at the requested time.
Its failed millimetre-level comparison was caused by a 0.167-microsecond
integration-clock offset. The source guide documents this time quantization.
The test retains its numerical thresholds, compares the physics at actual
integration time, and separately checks reported elapsed time and the warning's
requested/achieved values. Requested nanosecond stopping precision remains
an explicit acceptance gap rather than receiving credit from these passes.

Visible development also exposed a test-harness assumption: active/current
MDI window queries can select the script while the outer desktop is minimized,
even though the correct command editor has been created. The new test uses
the commandIndex and actual editor type, selects that child in the inner
workspace, and asserts source identity and visibility. It also reacquires
the replacement editor after Apply. No application code changed in this batch.

The two new cases pass both modes with 52 valid visible PNGs. The passing
forward visible result was reused in the audited two-case collection while
the backward case ran separately. No preceding regression cases were selected.

Station creation preserves coupled coordinates
----------------------------------------------

Ten new station-coordinate cases independently verify the actual embedded
New/resource forms and sandbox body-fixed positions. The four nominal cases
passed initially, but all six zero-height pole/equator cases failed with the
old default radius. Creation discarded height zero because it equaled the
original Cartesian Z, even though switching StateType had changed its meaning.

``ResourceEditor`` now includes CentralBody, StateType, HorizonReference and
all three Location fields together when any location field changes during
creation. An unrelated station edit does not force this tuple. This preserves
the explicit zero height after the detached candidate converts its selectors.
The test source, geometry tolerance and all assertions remain unchanged.

The six failing boundaries pass after the fix, and their preserved results
are reused during unattended completion of the ten-case batch. All ten also
pass visibly in a minimized, resizable desktop, with 296 valid PNGs. The
preceding 263 checks are not selected. Nine bounded requirement instances
are added; existing historical Add/Apply/Load mappings are reused without
duplicate credit. Broad validation and independent contact/measurement
accuracy remain open. The exact production diff and audit are preserved in
``build/nasa-coverage/station-coordinates-review/``.

Calculated-point preview defaults
--------------------------------

The new six-state/reference-kind tests found that New LibrationPoint offered
an empty sixth point choice. Its detached preview came directly from the
factory, whose constructor leaves Primary, Secondary and Point blank; normal
Moderator creation and the NASA guide specify Sun, Earth and L1. Barycenter
factory previews similarly lacked the documented Earth/Luna defaults.

``ResourceDraft`` now supplies these defaults without registering resources
or mutating configured bodies. The new tests retain their five-choice
assertion and add explicit default checks, including untouched Create.
Nineteen new cases verify L1--L5 with celestial/barycenter reference kinds,
mixed default/user and single-user barycenters, independent six-component
states at four epochs, invalid correction and exact GUI transactions.
The existing calculated-point test is byte-identical to its pre-batch backup.

All nineteen pass unattended and visibly, with 746 valid PNGs. No preceding
273-case regression was selected. The audit independently checks equilibrium,
GM-weighted position/velocity and an exact local linear velocity formula;
the latter agrees within 2.14e-14 km/s. The documented zero-relative-acceleration
assumption, historical two-body deprecation conflict and broad native/input/
platform/stress limitations remain explicit. The exact production diff and
source-bound audit are in ``build/nasa-coverage/calculated-state-review/``.

Snapshot source review for unmapped conflicts
-------------------------------------------

``RunAcceptance.input_sources`` previously copied reference files attached
to executable claims, but could omit a source used only by an unverified
or deprecated-requirement conflict. It now also includes the conflict's
hashed references. One new runner contract creates an otherwise unmapped
conflict, freezes its file, edits the working copy and checks that the
preserved file still verifies. Only this new contract was selected.

The already completed nineteen-case GUI snapshots were left unchanged.
A separate source-review collection retains the omitted deprecation source
and all other hashed conflict references. The audit links this supplement
and labels its scope; it adds no passing GUI/requirement credit and required
no repeated GUI execution.

Differential-corrector Unicode output and iteration boundary
---------------------------------------------------------

New DC creation/report tests reproduced rejection of a valid selected Unicode
report path. ``StringUtil`` filename/full-path validation treated every byte
outside printable ASCII as invalid. It now preserves non-ASCII bytes while
retaining ASCII-control, reserved-character and drive syntax checks; drive
classification also uses an unsigned byte. The new utility case checks five
actual Linux create/reopen paths and 114 negative scenarios. Its passing
source/binary/library-matched result is retained without a second execution.

The new initially solved MaximumIterations=1 case then reproduced contradictory
messages: exceeded maximum iterations and converged in the same pass.
``DifferentialCorrector`` now emits the limit-failure diagnostic only when
CheckCompletion has not converged. Its numerical equations are unchanged.

Eighteen new GUI cases pass both modes, checking every iteration, perturbation,
finite-difference Newton update, reported matrix/inverse and progress policy
for an independently specified coupled polynomial target. Cap-one/two states,
post-loop continuation, live four-row status, exact GUI transactions and
Unicode persistence are checked. The existing solver assertions are unchanged;
the first unattended GUI pass is reused for completion of the new batch.
No preceding 292-case regression was selected. Eleven bounded requirement
scopes and one DC ShowScript operation are added. General numerical targeting
and full current regression remain unqualified.

Safe viewing boundaries after MDI painting
----------------------------------------

The DC visual audit found that window-show captures could precede final MDI
painting. Increasing the capture budget did not repair that timing gap.
``acceptanceView`` now sends a synchronous typed event after processing paint
events. The opt-in test observer records and captures that explicit boundary,
honoring its existing budgets and OpenGL safeguards. Unattended calls still
return immediately. No application production code or numerical assertion
changed for this capture fix.

One new observer contract verifies event delivery without headless screenshots.
One focused DC case verifies native late-stage capture and correct readable
table/status values; its 63 PNGs and fifteen boundaries are separately audited.
The other seventeen solver passes and utility pass are retained. Earlier
capture attempts remain preserved without late-stage visual credit. The
eighteen-case cohort and capture follow-up keep separate harness identities.
Evidence and exact production diffs are in
``build/nasa-coverage/differential-review/``.

SPK start epochs and step validation, 2026-10-27
------------------------------------------------

A new SPK control case reproduced missing typed epoch/start-source selectors.
Resource metadata now supplies all ten supported formats and the editable
FromSpacecraft/EphemStart choices. The editor converts explicit dates when the
format changes and restores the visible selector if conversion fails. The
resource transaction applies EpochFormat/StartEpoch as one validated pair;
format-only edits preserve the physical instant. The core StepSize setter now
rejects zero, negative and nonfinite user magnitudes, replacing silent zero
ignore and negative acceptance. Backward direction still uses the existing
InitialStepSize/internal signed-step contract.

Eight new independent/control cases pass visibly and have preserved unattended
passes across development versions. A Python audit checks synthetic SPK file
metadata/states and reported cubic/derivative values independently. Original
ephemeris assertions remain unchanged. The preceding 311 checks were not
selected. One additional visible case checks a new result-view layout fix in
the test itself; production and numerical assertions are unchanged.
See ``build/nasa-coverage/spk-review/validation-summary.json``. General SPK
science, Origin, pending type-switch creation fields and file-picker/cache
coverage remain open; no full current regression claim is made.

Pending propagator family controls, 2026-10-27
---------------------------------------------

A new Create test reproduced that selecting SPK left the numeric draft's
controls in place, with no StepSize field. A type-dependent PropagatorSettings
panel now prepares detached factory prototypes, displays active controls
immediately, retains pending values per type and carries compatible numeric
settings across families. Inactive fields are excluded from Apply and dirty
checks. The existing hidden table remains synchronized. SPK's ignored Origin
is read-only, and unsupported Origin API edits reject explicitly.

Three new creation/edit/family cases pass unattended and visibly on identical
frozen inputs, including actual force-model picker Cancel/OK, invalid Create
correction, exact Undo/Redo and Unicode save/reopen. All preceding 319 checks
were unselected, and the prior SPK/numeric propagator assertions are unchanged.
See ``build/nasa-coverage/spk-draft-review/validation-summary.json``. This
qualifies the pending control mechanism; prior scientific propagation results
remain version-specific until the final batched regression.

Configured propagation group validation, 2026-10-27
--------------------------------------------------

A new actual-dialog case reproduced accepting STM with an SPK group, which
failed only later in core initialization. Configured mission editors now use
the same read-only semantic validation for the assignment dialog and raw
pending-command Apply. The check rejects nonnumeric STM/A-matrix/covariance,
multiple spacecraft or formations under one SPK instance, empty formations,
and duplicate spacecraft hidden in formation membership. The dialog refreshes
on flag changes and retains invalid pending inputs for correction. Standalone
syntax-only editors keep their previous optional-context behavior. Core
propagation is unchanged.

Fifteen new grouped/mode/direction/constraint cases pass visibly and across
retained unattended development results. They check one/two coupled 12-state
numeric groups plus separate SPK clones against independent circular/affine
oracles, and core rejection/recovery. Twelve passes were retained; only the
corrected failing actual-button action and two new numeric-group cases ran
next. The previous state-transition, covariance and workflow assertions are
unchanged, and the preceding 326 checks were unselected. Post-run source-bound
mapping review adds four scopes without replaying the GUI for metadata.
See ``build/nasa-coverage/spk-groups-review/validation-summary.json``; full
current regression, general science/native/platform/stress limits remain open.
