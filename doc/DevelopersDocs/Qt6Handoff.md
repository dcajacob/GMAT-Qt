# Qt 6 Linux GUI replacement handoff

Prepared 2026-09-30 for a fresh Codex chat. This is a navigation and continuation
brief, not proof of qualification. Recheck the repository before making changes.

## Start here

1. Work in `/home/dan/GIT/GMAT-Qt` on `codex/qt6-gui`, the separate Qt
   checkout requested on 2026-10-01. See `Qt6Project.md` for launch/build details.
   The original `/home/dan/GIT/GMAT/GMAT` checkout and historical evidence are retained.
2. Read this file, `Qt6Gui.md`, and the acceptance gates and current inventory
   in `Qt6ReplacementQualification.md`. Use the
   [current nine-gate reconciliation](Qt6ParityValidation/current-acceptance-20261002.md)
   for remaining work; dated appendices retain historical checkpoint states.
   Do not treat an older pending item as a request to repeat covered work.
3. Prioritize viewer/window reliability on the user's Linux desktop, then
   finish the active wx workflows and the remaining qualification gates.
4. Make concrete fixes, validate them, record their evidence, and rebuild the
   actual application. Do not stop at another inventory or proposed plan.

## Current installed application — 2026-10-04

GmatQt is rebuilt at **6,718,016 bytes / SHA 6cfca843…**.
[Saved playback options](Qt6ParityValidation/playback-options-20261004.md) now implement
PlaybackTimeScale, TimeSyncParent and ShowToolbar, pending source edits, shared
absolute-epoch clocks, per-object arc sampling and reachable toolbar restoration.
Eight new and five directly affected checks pass over the recorded builds, including
private native/fallback pixels and one new short backward/forward mission with an
unchanged complete report and retained samples. The finite historical camera query
regression is repaired; first failures and fixture corrections remain recorded.
Base/util/Qt startup stay unchanged. The original checkout is untouched.

The preceding [Star options](Qt6ParityValidation/star-options-20261004.md) retain
four new and three affected passes, pending form/count repair and LastN carry-through
at their recorded runtime. Those checks were not repeated for playback options.

The preceding [sensor masks](Qt6ParityValidation/sensor-masks-20261004.md)
retain four new/three affected passes and their complete byte-identical 131-byte
report at their recorded runtime; they were not repeated for Star options.

[Native vector pixels](Qt6ParityValidation/native-vectors-20261004.md) previously
passed their new 1.50 s private Xvfb/llvmpipe check. The sensor-mask renderer change
also passes the directly affected check in 1.41 s; its earlier evidence remains
bound to the earlier runtime. No host/GPU acceptance is implied.

[LastN solver retention](Qt6ParityValidation/lastn-20261004.md) retains its
conversion, controls, per-object finalized limits and replay cleanup evidence:
new 0.51s and affected SolverPlots1.37s passes, byte-identical full report and
accepted samples, plus the explicit decorative-listener cleanup limit.
Its base SHA 5d455c6b… added only an opt-in display lifecycle callback.

The following evidence remains bound to its earlier runtimes.
[Coordinate labels](Qt6ParityValidation/coordinate-labels-20261004.md) now expose
independent XYZ/origin-name controls, conversion and native/fallback rendering.
Four affected checks pass after an Apply-map repair; a new short fixture keeps
its complete numerical report byte-identical. Base/util/Qt startup are unchanged.
The preceding runtime's
[Receiver ErrorModels](Qt6ParityValidation/receiver-error-models-20261004.md)
now supports ordered selection, clearing and reference/source recovery in creation
and editing. The missing-control baseline is reproduced; its new focused check
passes in 0.41 s, including duplicate measurement-type rejection and Rename/Undo.
Base/util/Qt startup remain unchanged. The earlier
[Transmitter SolveFors](Qt6ParityValidation/transmitter-solvefors-20261004.md)
now exposes the supported FrequencyBias/empty list in creation and editing.
The missing-control baseline is reproduced; the focused actual-form check passes
in 0.32 s, including rollback, Undo/Redo and Save/reopen. No numerical mission
or previously passing suite was repeated for this exact-type form change.
[Propagation segment labels](Qt6ParityValidation/segment-labels-20261004.md)
close the active OF Label Segments operation with conversion, resource controls,
accepted arc identity and native/fallback replay labels. Eight directly relevant
checks pass after a standalone dialog initialization guard. A short new fixture
has byte-identical complete numerical reports; base/util/Qt startup are unchanged.
[Two animation modes and speed buttons](Qt6ParityValidation/interpolated-playback-20261003.md)
retain their earlier seven-check and source-matched Mars evidence. No completed
audit, tutorial or corpus was repeated. Host/GPU acceptance remains open.
[The earlier segment-pause repair](Qt6ParityValidation/playback-segment-gaps-20261003.md)
retains its reproduced failure and original runtime evidence. Earlier
[Help attachment and Find Return evidence](Qt6ParityValidation/help-attachments-20261003.md)
remains bound to its original runtime. [Offline Help publication](Qt6ParityValidation/help-publication-20261003.md)
includes eighteen tutorial chapters. Numerical walkthroughs remain bound to
their recorded runtimes and were not repeated for the Help repairs.

## Remaining qualification scope — 2026-10-04

The two concrete gaps from the [targeted source review](Qt6ParityValidation/of-remaining-controls-20261004.md)
now have bounded implementations: independent coordinate labels and LastN solver
retention. Use their current evidence above instead of treating that earlier
source review as an unimplemented queue. LastN's decorative-listener cleanup
limit remains explicit; it is not a claim of every legacy listener behavior.
The subsequently identified sensor-mask operation and native-vector pixel gap
now have the bounded evidence above. Star options are also implemented. The
concrete remaining OF source preference is **per-plot DrawModelFile**. Saved
PlaybackTimeScale, TimeSyncParent and ShowToolbar now have the bounded evidence
above, superseding those entries in the [star-options continuation queue](Qt6ParityValidation/star-options-20261004.md#concrete-remaining-of-operations).
Ordinary/global history sweep and shared absolute-epoch playback remain distinct;
compact raw endpoints do not reconstruct every discarded propagation state. Device VR/MSAA and inactive AxesLength remain separate. Keep
all nine gates, held scientific cases and external dependencies in scope.
Host/GPU acceptance and GMAT client attribution remain unverified, with live
host testing stopped. Do not repeat completed audits/tutorials/examples or
passing tests without a concrete affected change, or turn this into a numerical
engine, Mutter or system-upgrade project.

## Current local Rename follow-up — 2026-10-03

The blanket include/function Rename ban is repaired for provably unrelated
literal dependencies; see [safe local Rename](Qt6ParityValidation/safe-local-rename-20261003.md).
Affected or unresolved external files still require coordinated script editing.
The actual rebuilt app, focused check and private F2/Undo/Redo/Save/reopen/known
report are verified. Six additional Help walkthroughs are complete and published, for eighteen tutorial chapters; Lunar remains held after two cases missed its capture tolerance. The full seven-additional-chapter requirement remains unmet. L2 remains held,
MATLAB and unresolved optimizer chapters remain deferred/unresolved, and live
host testing remains stopped. Keep all nine original acceptance gates.

## Additional walkthrough usability repairs — 2026-10-03

[Single-step propagation](Qt6ParityValidation/single-step-20261003.md) is available
in the primary form and by removing all stopping rows. Typed command controls
scroll. [Coordinate Rename and new-window size](Qt6ParityValidation/axis-command-usability-20261003.md)
repair further actual walkthrough blockers; their app SHA483ed090… was rebuilt and directly affected checks passed. The current Help follow-up above records the later runtime. [Implicit Earth gravity](Qt6ParityValidation/implicit-gravity-20261003.md)
retains the existing Earth field during paired force edits. Six of seven additional usable source walkthroughs are complete and published; [installed Help verification](Qt6ParityValidation/help-publication-20261003.md) covers their pages, links and illustrations. [Lunar Transfer](Qt6ParityValidation/help-lunar-transfer-20261003.md) remains HOLD after its original and one solver-setting variant missed ECC0±1e-5. Its final inclination is about88.07°, not proof of exact90° capture. Use the current [walkthrough ledger](Qt6TutorialWalkthrough.md) and individual runtime receipts, including its early-access disclosure.
Original twelve published passes and all nine gates remain intact; host/GPU stops
and external/deferred chapter dispositions remain visible.

## User intent and boundaries

Current priority, clarified on 2026-10-01: deliver a usable Qt GMAT GUI with broad
capability coverage. Prioritize ordinary resource, mission and output workflows
and failures that block using them; defer exhaustive visual and edge-case
refinements until that coverage is in place. Do not unnecessarily repeat
previous tests. The full Linux replacement scope remains, with native desktop
testing stopped after the recorded compositor crash.

Deliver a fully qualified Qt 6 replacement for the selected Linux GMAT GUI
runtime. Keep the familiar wx arrangement: Resources/Mission/Output navigation,
MDI workspace, messages, menus and toolbar. Improve from that recognizable base.

- Windows and macOS builds/deployment are deferred. MATLAB is outside the
  selected runtime; the current build has `PLUGIN_MATLABINTERFACE=OFF`.
- The user likes the current Qt viewer direction and dislikes the OF viewer.
  Preserve the Qt interaction model while matching required capabilities.
  Native rendering uses OSG inside Qt; this is distinct from embedding OF.
- Render textures and starfields and retain existing implemented viewer work.
- On opening an OF example that needs translation, automatically offer conversion
  instead of only reporting a build failure. Preserve compatible settings and
  explicitly handle unsupported settings. A warning alone is not parity.
- Tables should have sensible initial column widths and remain user adjustable.
- Preserve script comments, labels, expressions, unknown settings and implicit
  defaults through edits, deletion, Undo/Redo, Save/Save As and reopening.
- Keep the existing numerical engine. Check that GUI edits preserve calculations;
  do not expand this task into requalifying or rewriting numerical algorithms.
- Use independent judgment. Earlier experiments in `Old` are reference material,
  not a design constraint. Keep changes focused on the official checkout.
- Work autonomously within this scope; do not repeatedly ask permission for
  ordinary reversible fixes, builds and verification.

The user's most serious reported failures were: default example runs but Orbit
View does not open; ground-view minimization hangs; lingering stalls; Orbit View
shows only a vector triad. Several subsequent fixes and tests passed, but the
remaining native desktop acceptance gates must still be closed. Do not infer
that these exact bugs still reproduce, or that every desktop case is now fixed.

## Initial handoff repository checkpoint

Historical checkpoint from handoff preparation; use Qt6Project.md and the latest
qualification appendices for the active new checkout/build:

Verified when preparing this handoff:

- Branch: `codex/qt6-gui`.
- HEAD: `df97b3b` — Guard stale Qt event reports and activate Output viewers.
- Working tree was clean before adding this handoff.
- User launches `/home/dan/GIT/GMAT/GMAT/application/bin/GmatQt`.
  It resolves to `application/bin/GmatQt-R2026a` in this checkout.
- Build directory: `build/linux-gui`, Ninja, Release, both wx and Qt GUIs enabled.
- Qt startup: `application/bin/gmat_startup_qt.txt`. Use its selected native
  plugin inventory; wx OF widget providers are not Qt plugins.

Recent commits worth inspecting:

| Commit | Change |
| --- | --- |
| `df97b3b` | Guard stale event reports; explicitly activate Output windows |
| `72d6a7b` | Delete resources without regenerating unrelated script source |
| `3b7be21` | Preserve resource source and implicit defaults during edits |
| `172dc55` | Match wx body selection; preserve implicit configuration |
| `1034956` | Celestial-body pages and kernel/appearance edit qualification |

## Work completed at the initial handoff

Resource edits now patch changed source assignments while retaining unrelated
configuration and mission text. Canonical regeneration had made implicit power
defaults explicit and changed results slightly; source retention addresses that
case. Grouped Array declarations support dimension changes. Owned ODEModel
configuration retains dependent ordering and legacy aliases. FOV placeholder
cleanup respects a real object named `UndefinedFieldOfView`.

Deletion removes the selected declaration/assignments while retaining other
members of grouped declarations and comments. Referenced resources are protected.
Celestial bodies and SolarSystem remain protected in the GUI, matching wx
ownership restrictions. Unsupported custom-body deletion remains a text workflow.

Event output now consults the current model/run and running locator's
`FileWasWritten` state before displaying an existing file. Rebuilt, pending,
Disabled, WriteReport-off and unexecuted Manual configurations cannot present
stale results as current. Generated reports use ReportViewer; unwritten output
explains the required configuration/FindEvents action. Report, event placeholder
and binary-ephemeris windows now explicitly activate when opened.

Ten additional wx rows were audited. Several legacy solver dialogs are inactive
non-writing prototypes; MATLAB workflows are outside the selected runtime.
Active propagator selection maps to tested Qt controls. Those dispositions do
not qualify unrelated active workflows, About/license or batch-run reports.

## Evidence and its limits

The following is the historical handoff checkpoint; later appendices supersede
its pending dispositions. Evidence directory: `doc/DevelopersDocs/Qt6ParityValidation/`.

- `check-event-output.txt`: all 36 Qt suites passed, 155.72 seconds.
- `check-event-output-report-disabled.txt`: final WriteReport-off event case and
  explanation passed the focused event suite, 3.84 seconds. The application was
  rebuilt with the final change. The complete suite predates that last small
  event addition; do not describe it as a subsequent full-suite rerun.
- `event-output-wayland.txt` and `event-output-wayland.png`,
  `.png.generated.png`, `.png.unwritten.png`: native Wayland configuration and
  generated/unwritten Output windows, captured and inspected.
- `check-source-deletion.txt`: all 36 suites passed after deletion changes.
  `round-trip-deletion-wayland.txt` records native deletion coverage.
- At the initial handoff, top-level Wayland minimize/restore and portal
  responses were unqualified. Later private software Wayland lifecycle and
  complementary private portal evidence are recorded in the current gate map.
  The user's host GNOME/physical GPU/crash qualification remains separate.
- Negative SPICE runs emit `FILEOPENFAILED` / `IOSTAT 128` while writing
  `GMATSpiceKernelError.txt`. Correction/rerun succeeds; diagnostic-file handling
  remains unresolved/unqualified. Preserve the raw evidence of these messages.

Green suites establish their covered cases, not full GUI or scientific parity.

## Remaining work and suggested order

### 1. Native viewer/window acceptance

Reconcile the existing private X11/software Wayland viewer, minimize/restore,
close/reopen, run/stop/rerun, replay and pending-edit evidence before selecting
any new verification. Complementary private portal cases also have bounded
evidence. Read-only saved-core analysis now identifies a null Wayland event
resource in Mutter popup setup; see the [saved fault record](Qt6ParityValidation/compositor-popup-null-resource-20261002.md).
GMAT client attribution, a repaired host and physical-GPU acceptance remain
unqualified; private software rendering does not close that gate. Continue only
actual remaining viewer/OF requirements identified by the current gate map,
keeping the Qt viewer UX.

### 2. Finish source audits and active wx workflows

The current qualification inventory has source/caller audits for all 108 wx
entries. Its mapped rows and latest appendices distinguish implemented cases,
inactive/deferred workflows and remaining limits. Continue missing active
operations and their bounded qualification; do not repeat the completed audits.
Folder runs, About/license, shared Help/Apply, debugger, welcome, parameter
selection and mission navigation have subsequent implementation/evidence.
Resource Clone and local Rename are also implemented; use their latest evidence
for coverage and external-source boundaries.

### 3. Shared desktop/editor behavior

Shared Help, retained resource/command Apply and independent active/inactive
script documents have subsequent implementation and bounded evidence. Qualify
remaining desktop input, keyboard/focus and portal chooser behavior without
repeating the covered source/transaction checks. Dirty/pending Apply/Discard/
Cancel, mixed-page edits, failed-edit recovery and document differences remain
part of the full contract. Report search/options and broader combinations keep
the explicit limits recorded in the current inventory and appendices.

### 4. Selected plugins and file recovery

Use the 20-row selected runtime plugin table in the qualification document.
Each plugin has bounded evidence, but many rows remain partial. Finish required
GUI settings, creation/removal and representative runtime/report/round-trip cases;
explicitly resolve fixture requirements and unsupported scope. Do not silently
turn every possible numerical regime into a new algorithm qualification task.
Check relative assets/includes, kernel paths, filesystem/write failures and
recovery. The SPICE diagnostic-file repair has subsequent independent-process
evidence; preserve its remaining race/storage limits and historical failures.

### 5. Final acceptance audit

Reconcile every acceptance gate with current implementation and evidence.
Compare representative wx/Qt workflows and independent script-configured reports.
Verify calculation-preserving save/reopen, Undo/Redo and failed-edit recovery.
Run the relevant regression suite after fixes and rebuild the actual user binary.
Full replacement is unfinished until the requirements have affirmative evidence.

## Useful implementation locations

- `src/qtgui/MainWindow.cpp`, `MainWindow.hpp`: navigation, Output, MDI lifecycle.
- `src/qtgui/MissionModel.cpp`: engine/build/run integration and transactional edits.
- `src/qtgui/UserParameter.cpp`: source-preserving edit/deletion helpers.
- `src/qtgui/ResourceEditor.cpp`, `EditablePanel.cpp`: common panel behavior.
- `src/qtgui/OrbitRenderer.cpp`, `OrbitCamera.hpp`, `PlotWidget.cpp`:
  native rendering, camera and viewer behavior.
- `src/qtgui/ScriptCompatibility.cpp`: OF example translation.
- `src/qtgui/tests/WindowTests.cpp`, `OrbitRendererTests.cpp`,
  `OrbitSetupTests.cpp`, `ResourceRoundTripTests.cpp`, `EventLocatorTests.cpp`:
  relevant regression entry points.
- `src/qtgui/CMakeLists.txt`: test targets and `check-qt` dependency list.
- `src/qtgui/tests/RunTest.py`: configured headless test wrapper.

## Build and run

Run from `/home/dan/GIT/GMAT-Qt`. Its Qt-only build is configured and has completed
the first engine/frontend/plugin build. See `Qt6Project.md`; keep incremental
builds and select checks affected by current changes.

```bash
cmake --build build/linux-gui --target GmatQt --parallel 2
cmake --build build/linux-gui --target check-qt --parallel 2
```

Launch exactly as the user does:

```bash
cd /home/dan/GIT/GMAT-Qt/application/bin
./GmatQt
```

A focused CTest example, after rebuilding its executable:

```bash
cmake --build build/linux-gui --target GmatQtEventLocatorTests --parallel 2
ctest --test-dir build/linux-gui -R '^QtGui.EventLocators$' --output-on-failure
```

Native tests need desktop/display access. In the previous sandbox, Xvfb/X11
checks failed without that access, then passed when authorized. Distinguish
environment access failures from application failures. Offscreen screenshots
cannot establish native OpenGL or desktop window lifecycle correctness.
Preserve `SYS_PATH` if reconfiguring; retain no-MATLAB settings.

## Durable goal state

The previous chat's stored goal objective is:

> Accomplish items 3, 4, and 5 of the Qt replacement qualification plan on Linux:
> audit wx resource editors and mission commands against Qt and implement missing
> GUI workflows (including specialized controls, array resizing and expression
> cells); close required plotting/camera and OpenFrames conversion behavior gaps
> while retaining Qt viewer UX; qualify supported plugins and file handling through
> configuration, execution, reports, save/reopen, error recovery and
> calculation-preserving script round trips. Maintain an evidence-backed checklist
> of scope and verify all requirements before completion; rebuild the user's
> GmatQt executable. Windows and macOS remain deferred.

The goal tracker returned `blocked` during handoff preparation. That state is
not completion and does not establish that repository work is blocked: concrete
GUI and qualification work remains possible. Goals live in conversation state;
this Markdown does not transfer or resume a goal. A new chat should create a
goal only when the user explicitly invokes it there. Do not manually edit goal
storage, mark the old goal complete, or narrow the original acceptance scope.

## Shipped examples and latest continuation — 2026-10-02

Every one of 164 standalone shipped mission/tutorial candidates has a real build
attempt, and every one of 147 successful builds has a run attempt. Current stages
are 136 completed runs, six unexpected runtime failures, two expected Step 1 Stop
checkpoints, three timed-out runs and 17 failed builds. Raw interrupted status
is retained. See Qt6ParityValidation/shipped-examples-20261002.md, manifest and
177-row table, plus the latest qualification appendix. Keep the full acceptance
scope; these are build/execution checks, not complete native/scientific parity.

All work and rebuilt runtime are in /home/dan/GIT/GMAT-Qt. Coordinate pending axes,
branch/Toggle insertion, relative assets, OF lists/undrawn cameras/retained vectors,
public Python API and SRP string-array fixes remain. New Clone covers unified
optional naming, pending/atomic source and managed Array/Qt metadata. Local Rename
now updates typed references, arrays and Qt metadata with explicit external-file
boundaries. Clone/OpenFramesSyntax/OpenFramesVectors passed after their frontend
changes; the final rebuilt Rename workflow passes its new focused check. No old full matrix was
repeated. Initial new-test harness errors are preserved/corrected.

The free R2026a VF13ad binary is installed locally in the ignored
application/plugins/thinksys/vf13ad directory. GMAT_QT_EXTERNAL_PLUGINS in the local
preset keeps its entry through startup regeneration. All 19 formerly blocked
variants build; 15 complete, two requested Step 1 Stop, two Mars time bounds. The
external binary/README is not committed or a built plugin target. Preserve its
license. Remaining failed builds: deferred MATLAB 4, SNOPT 3, CSALT 9 (SNOPT needed)
and legacy MarsGRAM 1. Do not automatically waive absent public plugins/data.

Formation now contains a clearly labeled reconstruction of the intended chapter,
with missing historic states disclosed; actual target residuals meet tolerance.
L2's measured wider bracket fixes its initial guard but the next full run exposes
an intermediate trial missing the geometric entry box and exhausting DE405.
One bounded diagnostic establishes the event-domain problem; do not remove the
guard, cap-and-claim-success or keep retrying without substantive transfer analysis.
Five TLE examples still require matching absent public catalogs. Their exact data
requirements and all helper/fragment coverage limits remain recorded.

The existing Yukon 7200-second attempt is now terminal timeout, with 11 of
20 windows completed. Its selected result/history was merged after matching
source identity; all raw outputs are preserved in the ignored durable yukon
tree. No unchanged restart is scheduled. This is incomplete, not a full pass.
The VF13ad/MarsLaunch and MarsPatchConic 300-second attempts were progressing,
not proved hung; preserve as incomplete. SPAD's earlier 209.91-second completed
retry must not repeat. QualifyExamples.py is resumable; select failed examples
only after concrete fixes. Completed raw trees are preserved in
build/example-qualification/20261002 (raw, formation, vf13-tutorials, vf13-probe,
l2-bracket, yukon). Live native desktop testing remains stopped until the GNOME Shell
crash/safety issue is resolved. Windows/macOS and MATLAB stay deferred.


Latest active workflow: Rename is implemented and its focused check passes
0.44 seconds after rebuilding GmatQt. See resource-rename-20261002.md and the
latest qualification appendix. Includes/external GMFs and unsupported tracking
paths stay explicit script-editor boundaries. Resource popups now use value
selection and are destroyed before modal dispatch; the offscreen lifetime check
is not proof of a compositor fix. Read-only Qt source/crash review found no
established current GMAT lifetime defect. Keep the native acceptance gate open.

## Next phase: interactive Help tutorials — 2026-10-02

After the current qualification work, follow the Help tutorials by constructing
each scenario through the Qt application, starting from a new mission or an
independently built prerequisite tutorial. Do not use shipped tutorial mission
scripts to construct it; compare those only after preserving the authored
mission and its results. See [Qt6TutorialWalkthrough.md](Qt6TutorialWalkthrough.md)
for the twelve published chapters, twelve additional source chapters, dependency
notes and evidence requirements. All walkthroughs are pending. Earlier example
runs and tutorial repairs do not satisfy this new GUI acceptance phase. Resolve
a safe interactive display route before testing; the native acceptance gate and
the existing Linux replacement goal remain open. Avoid repeating passing tests.


## Isolated actual-input windows, object guides and Undo — 2026-10-02

A safe private authenticated X11 route now operates the actual Qt application
with mouse/keyboard input, private settings/output and software GL. It avoids
the user's desktop. New mission execution, one main minimize/restore and viewer
close/reopen cycle, category-specific GroundStation creation/Cancel/automatic
name/Save, and long-fixture pause/stop were exercised. The host GNOME Shell PID
and start time remain unchanged. These are bounded X11 results; host GNOME/
Wayland, portals, hardware-driver and repeated lifecycle acceptance remain open.

The attempt exposed Ctrl+Z doing nothing with plot/tree focus. Undo/Redo now
fall back to the selected script when non-text controls have focus, preserve
local/inactive-document histories and block background modal actions. Owned
modal fields now follow parentWidget ownership across the dialog window boundary.
The final focused check passes 0.28 seconds. An actual Mission-tree Ctrl+Z retry
restores the entire saved authored source byte for byte, and its short mission
completes 0.563 seconds. The first failed shortcut assumption and the actual
modal-routing failures are preserved; no old full matrix was repeated.

Per-object DrawGrid and DrawXYPlane conversion, pending controls, native/fallback
geometry and retained replay now exist. QtGui.ObjectGuides passes 0.31 seconds;
actual native X11 controls show Earth's spherical grid and filled 15R radial
plane. The saved guide source retains global Grid Off / XYPlane On. Unsupported
velocity/other decorations and wider conversion/camera regimes remain open.

See Qt6ParityValidation/object-guides-20261002.md and
Qt6ParityValidation/isolated-x11-input-20261002.md for identity, selected captures
and limits. Full raw sessions/check failures are retained under
build/example-qualification/20261002/isolated-x11. The actual application/bin/GmatQt
is rebuilt, final SHA256 9bc8115f4200bc4c933b72de2383cfe7722d79c463d148a64c029238b460ea8d.
All Help tutorial walkthroughs remain pending; this route can support their
future independent construction. Full Linux replacement remains unfinished.


## Viewer preview/shared displays, velocity and command names — 2026-10-02

All development continues in /home/dan/GIT/GMAT-Qt on codex/qt6-gui. The original
checkout remains untouched. New actual-input evidence found that clean OrbitView
settings close destroyed a same-name preview and cleared the live viewer's
objects. ResourcePreview frontend ownership now suppresses just that clone's
synchronous ClearObjects; genuine engine cleanup remains active.
QtGui.PreviewLifetime passes 0.89 seconds. Its private X11 retry retains textured
Earth/path/legend/rotated camera at 47.6% through two Output delete/reopens,
three main minimize/restores and a viewer minimize/restore. Preserve the original
failed captures; these passes do not establish the prior GNOME crash's cause.

Three actual-input displays (Orbit/Ground/GUI-created XY) share scrubbing and
Start/Play/Pause;27% pause captures remain identical after 500 ms. Confirmed
per-Orbit Local override/global Latest works. Do not claim LocalXY from early
mislabeled global-slider actions or save from screenshots still showing SaveAs.
A new retained XY settings Window activation clips controls; its correction
needs the focused workspace check and actual-input retry. See the accompanying
viewer-preview-shared-20261002.md for full identities and bounded observations.

DrawVelocity now converts source-ordered/default/prefix/Add-reset flags and
provides pending per-object controls. Recorded view-frame velocities drive exact
OF-style position+1000 s × velocity line segments, native/fallback/replay/trim/Fit
and source/rename retention. QtGui.ObjectVelocity passes 0.85 seconds; affected
ObjectGuides passes 0.34 seconds with DrawUsePropLabel as its unsupported fixture.
The rotating-frame comparison uses NutationUpdateInterval=0 in the fixture to
align Report and cached-plot paths; no production numerical changes. Actual
input Apply/run and Output reopen show velocity at Latest. The attempted native
middle replay ended at Latest and remains unqualified in that session. Relative
Velocity/ThrustVector and wider scientific/solver regimes remain open.

Command panels now support pending optional Name and runtime-gated Write with
ordered parameters/full source fallback. A real named-assignment serialization
failure with GMAT keyword OFF required an 11-line GmatCommand::InsertCommandName
fix; no numerical execution changes. QtGui.MissionLabelsWrite passes 0.89 seconds,
including keyword OFF/ON/OFF/repeated serialization and report 9/2.5. Read-only
focused peer review found no additional substantive defects. All first failures,
diagnostics and successful retries remain preserved in the ignored durable
build/example-qualification/20261002 trees. No old successful full matrix or
shipped corpus was repeated.

All Help tutorial walkthroughs remain Pending. Qualification fixtures in these
sessions are independently GUI-authored but are not Help tutorial constructions.
After current gaps, use the Help actually available, build each scenario through
visible GUI actions from a new mission/prerequisite authored tutorial, save/run/
reopen and preserve results before consulting a shipped reference. Full Linux
replacement acceptance remains open; Windows/macOS/MATLAB remain deferred.


### Final workspace/native retry for this checkpoint

WorkspaceReachability passes 0.34 seconds. Initial placement inherited the active
script's maximized state and later restored an unclamped old geometry; normalizing
new configuration panels plus settled/direct and guarded final placement fixes
that case. Actual private X11 input confirms Window and Resources restore the
same displaced pending XY form with full controls. A fresh accepted master
Start/47.6% seek and actual Output Orbit reopen retain native velocity at that
position. Earlier failed/inconclusive actions keep their dispositions; see
Qt6ParityValidation/workspace-reachability-20261002.md.

Application/bin/GmatQt is rebuilt, final SHA256
c15cb5c18930019978bb8c335ec374258c67a2e24857ca7d455975808fb432a3;
shared core 5eb86098fbc52890a95d3226c56a4edd8e090308a940bb097749e23237532ab9;
selected startup 5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a.
The authored fixture remains byte-identical. Host GNOME Shell remains PID399961,
start 2026-10-01 11:34:56 MDT. Full replacement and all Help walkthroughs remain open.

### Safe private Wayland route: preparation only

Read-only inspection found installed GNOME Shell/Mutter 50.1, Qt Wayland, Mesa
software EGL and Python Gio. A next isolated route may use an owned headless
compositor with --wayland --headless --no-x11 --virtual-monitor=1600x1000 and a
private --wayland-display under a fresh 0700 runtime/private HOME/config/cache and
its own D-Bus. Remove inherited DISPLAY/WAYLAND_DISPLAY/WAYLAND_SOCKET/XAUTHORITY/
DBUS session variables. Force Mesa/llvmpipe; bwrap may mask host sockets/dev/dri.
Namespace availability, compositor startup and Qt rendering have NOT been tested.
Do not use --display-server with headless. No compositor was launched here.

Actual input can use one persistent Gio connection to Mutter RemoteDesktop
CreateSession/Start/NotifyKeyboard and relative pointer/button/axis methods;
transient gdbus callers fail ownership checks. Private Shell --unsafe-mode allows
its screenshot service; relative input/screenshot need no PipeWire. Absolute
input requires ScreenCast/PipeWire. Keep these capabilities confined to the owned
private bus/runtime, with no physical devices or host desktop sockets.
Authoritative installed-version source:
[Mutter options](https://raw.githubusercontent.com/GNOME/mutter/50.1/src/core/meta-context-main.c),
[headless backend](https://raw.githubusercontent.com/GNOME/mutter/50.1/src/backends/native/meta-backend-native.c),
[GPU-less renderer](https://raw.githubusercontent.com/GNOME/mutter/50.1/src/backends/native/meta-renderer-native.c),
[virtual input](https://raw.githubusercontent.com/GNOME/mutter/50.1/src/backends/meta-remote-desktop-session.c),
[private screenshot](https://raw.githubusercontent.com/GNOME/gnome-shell/50.1/js/ui/screenshot.js).
This is implementation preparation, not native Wayland acceptance or a proved
fix for the user's earlier crash. Segment-arc identity and mixed solver cleanup
remain substantive active gaps after the current checkpoint. Tutorial construction
must still follow the Help and reserve shipped references for later comparison.


### Mixed hierarchy follow-up

The new mixed Yukon/DC check found a real outer-state leak; the five existing
Optimize publisher transitions now synchronize its member display state. Both
Yukon and installed VF13ad pass the new analytic/report/accepted-history/inner
Stop/recovery checks (0.99/1.03 s). Numerical Yukon report bytes match the original
failed run. See mixed-solver-scopes-20261002.md; all original failures are retained.
App remains c15cb5c...; rebuilt dynamically linked core is now 68a79c56... .
Private Wayland preparation has progressed to a mapped software-rendered mission,
but accurate actual pointer input and window lifecycle are still being verified.
Do not claim full Wayland or tutorial acceptance. Named regular-arc identity and
trim-independent camera endpoints are being implemented next.

VF13ad is an optional binary plugin but derives InternalOptimizer. The final
new checks assert that path and the actual inner index9/command text;0.99/1.03 s
passes supersede the weaker0.99/0.97 s Stop proof. ExternalOptimizer execution is
still unqualified. See boundary-verified artifacts and the revised mixed record.


### Private Wayland actual-input checkpoint — 2026-10-02

The earlier preparation-only status is superseded for the bounded private
software Wayland cases recorded in
[private-wayland-20261002.md](Qt6ParityValidation/private-wayland-20261002.md).
An owned GNOME Shell/Mutter/Qt Wayland route with private runtime, D-Bus,
PipeWire and software GL delivered actual virtual keyboard/pointer events,
without host display/socket/device exposure. Real F5 completed the authored
three-viewer fixture in 1.404 s; master/Orbit/Ground controls agreed at 48.6%,
with XY rendered. Three Super+h/Alt+Tab minimize/restore cycles and the confirmed
second Output activation retained native Orbit data at that position. Earlier
Alt+F9 and first Output/Enter attempts did not establish those results.

The XY pending X change survived actual Cancel, then Close without Saving
discarded it. The final native GTK Save As produced a byte-exact 5371-byte
fixture; the file hash and later window state prove Save, while screenshot024
still shows its chooser. This is in-process native GTK coverage, not portal
FileChooser/Response qualification or a later saved-file reopen/run.

Recorded pre arc/tree runtime: app c15cb5c18930019978bb8c335ec374258c67a2e24857ca7d455975808fb432a3,
core 68a79c56f22c22407ba93f462e73fad4c4050d0cc9f54b0203b79f0d471085c3,
helper ee8368a88d1b79be22bb127e6e04afe7f2a562443323374e896e6a6c0c2d944d.
All nine closed sessions, including first failures and missing-core-identity
preparation limits, are preserved in ignored
build/example-qualification/20261002/wayland; seven unchanged PNGs are tracked
with the record. Cleanup left host GNOME PID/start identity unchanged; this does
not diagnose or fix the earlier host crash. Host/hardware/portal, broader Linux
replacement and all Help tutorial construction gates remain open. Windows/macOS
and MATLAB remain deferred. No further GUI session was run to prepare this record.



### Reconciled public TLE input checkpoint — 2026-10-02

The two source-matched runtime retries after exact June 2020 input recovery are
now in the corpus ledger: Falconsat7Jupe completed in 1.172 s; FalconSats resolved
its catalogs but failed in 0.622 s with SPICE(BADMECCENTRICITY) while using
historical elements at SystemTime(now). FalconSats remains an unexpected runtime
failure, not a scientific pass or proprietary/expected-Stop exception. No
other mission, build or passing test was repeated for this targeted TLE work.

Current corpus totals are 147 passed builds / 17 failed builds, 137 completed
executions, five unexpected runtime failures, two expected tutorial Stops and
three timeouts (raw 137 passed / seven failed / three timed out). Prior attempts
remain in history. The earlier five-absent-TLE-catalog statement is superseded:
Jupe's exact input is recovered and its historical run completes; FalconSats has
a current-time/stale-element domain failure; Contacts uses the now-decayed
FalconSat-7; GSFCSats/Starlink still need public December 2019 data through the
recorded human/account access route. Matching legacy MarsGRAM integration/data
remains unestablished without a proprietary-only waiver. See
[tle-dependency-inputs-20261002.md](Qt6ParityValidation/tle-dependency-inputs-20261002.md)
and the source-matched shipped corpus summary/table/failure records. These are
offscreen execution/input-disposition results; independent scientific accuracy,
native viewer acceptance and human Help tutorial construction remain separate.


### Named regular arcs and keyboard navigation — 2026-10-02

[Named-arc/navigation evidence](Qt6ParityValidation/named-arc-navigation-20261002.md)
records first-regular-arc selection, same-provider resets, connected A→B→A joining,
trial/accepted ownership and copied endpoints/attitude through decimation/trim/
Output reopen. SegmentArc passes 0.61 s with exact independent reports. Compact
arc metadata survives MaxPlotPoints trimming and has no separate arc-count cap;
its growth until model/data reset remains an explicit limit. Metadata-disabled wx
collection/replay remains unchanged. The new Current accepted-duplicate review
fix was caught before shipping this implementation.

TreeActivation passes 0.59 s: focused Return/Enter uses existing guarded Resource/
Mission/Output routes once, retaining pending panels, category/run/unbuilt guards
and existing mouse callers. Actual private Wayland station Return is captured;
native Output Enter remains unverified. SegmentCameras passes 0.54 s after its old
exact-step 60-second fixture was corrected to 61+119 seconds, creating a real OF
flush boundary with the same total of 180 seconds and original camera tolerances. Both arc
endpoints/DCMs match independent reported values. First compile, timer and
camera-assumption failures remain preserved, not reclassified as product passes.

Rebuilt app SHA256 7b4a0ae720bb074c71ac05e73d351e987beae5c7fcda524e105d7fb1d55fdae3;
core cf147e234b57818fecf475e0516da86d9187e10066d981e6c893ae7fd761e016.
No earlier successful matrix/corpus repeated. Tutorial1 is in progress; this
record claims no tutorial completion. Full replacement and host/hardware/wider
viewer gates remain open; Windows/macOS/MATLAB remain deferred.


### Private portal FileChooser checkpoint — 2026-10-02

[private-portal-20261002.md](Qt6ParityValidation/private-portal-20261002.md)
records four actual owned-bus FileChooser request/Response cases: Open Cancel
(Response 1), accepted Save (Response 0, owned portal-authored.script), directory
Cancel (Response 1, path retained) and directory Select (Response 0, owned output
path). The saved 5371-byte fixture is byte-identical. Open acceptance/reopen and
Save cancellation are not proved: later labels still show Open, both later Open
requests return Cancel, and no second SaveFile request exists. Native station
leaf+Return opens Setup in raw screenshot 013; no new mission execution is claimed.

This private session used app 7b4a0ae7…/core cf147e23… and a temporarily extracted
matching Qt6.10.2 portal plugin bound read-only for GmatQt alone; full SHA256,
official package/license and isolation provenance are in the record. The first
outer protocol failure remains preserved separately from successful raw portal
traces and worker cleanup. Framed/correlated replies and owned screenshot checks
now pass nine synthetic pipe-only cases; their script/result stay in ignored
wayland/portal-first-attempt evidence. No additional GUI/test/build was run for
this promotion. Host GNOME/hardware/crash repair and wider qualification remain
unproved; no Help tutorial claim is added. Windows/macOS/MATLAB remain deferred.


### First completed Help walkthrough — 2026-10-02

[Tutorial 1](Qt6ParityValidation/help-tutorial-01-20261002.md) is Passed for bounded
private X11 actual-input construction: New mission→Help-driven resource/mission
controls→periapsis run→summary/frame/camera/animation→Save/reopen→post-freeze
reference GUI comparison. The first 600-second session resumed only its own saved
milestone. Frozen authored source remains 5672 bytes/SHA256 f0becc897f413aa8eb211813125e063ebd57e4e0d5fcf4136dd17142411dada4;
no shipped seed was used, and prior corpus exposure is disclosed. App 7b4a0ae7.../
core cf147e23... are recorded separately. Actual authored/reference runs complete
in 0.361/0.174 s with the same displayed UTC/radius/periapsis. Differences are
retained: 5.569 mm maximum position component, 6.366 mm norm, 3.178 micrometres/s
maximum velocity component. A locally chosen tighter assertion failed; Help
specifies no such threshold. Force-order causation and bitwise parity are unproved.

Unchanged screenshots establish EarthFixed summary, shared17% animation and
reopened Periapsis settings. The mistaken summary filename Save and first-reference
BadWindow helper failure remain explicit. Supported default Drag serialization
warns but the mission completes; Qt's separate ForceModel resource placement is
recorded against Help's integrated Propagator instructions. Qt6TutorialWalkthrough
now links row 1 Passed; other rows retain Pending while Tutorial 2 proceeds under
separate evidence. No broad matrix/corpus repeated. Full replacement and host/
hardware/portal gates remain open; Windows/macOS/MATLAB remain deferred.


### Second completed Help walkthrough — 2026-10-02

[Tutorial 2](Qt6ParityValidation/help-tutorial-02-20261002.md) is Passed for bounded
private actual-input construction from Welcome/Help/New: TOI/GOI/DC1 and the
ordered transfer mission were authored through controls. The first solve completed
in seven iterations/0.760 s; the Target editor's Apply Corrections button changed
only two Vary guesses, followed by one iteration/0.647 s. Actual own Ctrl+O reopen
retains both corrected fields and the unchanged 5978-byte source, SHA256
652c63e2559d6cda7e489f458c870ae8908fd8aa780adf8b518afd5cc4c4d20c.
Both achieved goals satisfy Help's 0.1 km radius and 0.0001 ECC tolerances. Initial
Achieve entries placed later input in Name and were corrected before F5; no
focus/layout cause or numerical failure is inferred.

Construction/results/reopen were frozen at 17:57:42.093446 UTC before reference
access. One later actual GUI reference run completed in 1.881 s; its full 4991-byte
solver report equals the authored seven-iteration report, SHA256
3edcbc50a784dcf587a7fb68c1d5b9a031e9de09b97beaddaa081f58a8a20d21.
The source comparison retains different Vary bounds and Target ExitMode; equality
is this report only, not every propagated sample or independent science. Five
unchanged PNGs and final authored source are copied with the record; raw traces,
reports and post-freeze source comparison remain in ignored Tutorial 2 evidence.
The ledger now has 1–2 Passed and 3 In progress (actual Help/resources/commands,
first 13-iteration authored convergence, no reference yet); remaining walkthroughs
and full replacement/host/hardware gates stay open. Windows/macOS/MATLAB are deferred.


### Third completed Help walkthrough — 2026-10-02

[Tutorial 3](Qt6ParityValidation/help-tutorial-03-20261002.md) is Passed for bounded
private actual-input construction from Welcome/Help/New: chemical hardware,
FiniteBurn1/DC1/BurnDuration and the ordered targeting mission were authored
through controls. The first F5 completed in 13 iterations/0.749 s. Four actual
exported command summaries show perigee/cutoff/apogee MA 0/25.131809686270/180°,
fuel 756→343.76990738327 kg and apogee radius 12000.000012291 km, within the Help's
0.1 km tolerance. Actual own Ctrl+O/editor checks retain C1=1000, tank mixture 1,
Vary initial 200/upper 10000/MaxStep 100 and final source. An unintended pending
coordinate wheel change was discarded before Apply; EarthMJ2000Eq remains saved.

Current default history did not supply Help's described trial view. The only
semantic model adjustment was explicit SolverIterations All; its necessary
0.736 s run retained the same complete report and enabled face-on trial viewing.
Temporary stars/constellations/XY plane viewer overrides were not written into
resource defaults. Final 6190-byte source SHA256
8a4a6914110f27b306ad256c6c0e4dc7a7f943d2c0100d7810f1b2a3d34e84fa
was frozen at 18:26:43.612951 UTC before reference access. One actual reference
Ctrl+O/Convert views/F5 run completed in 0.541 s without saving its original
source. All three whole 5583-byte solver reports are byte-identical, SHA256
809ed5bba4fe90001b83612fc2e1790935e09f129a2a13f0689bcabb97853f14.

Search lower bound −10 versus reference 0, All versus Current, and omitted
hardware/DC/default fields remain explicit. Equality is this report only;
every-state, pixel and independent science parity are unproved. Six unchanged
PNGs/final authored source accompany the record; raw evidence remains indexed
under ignored 03-finite-burn (114 files/69 PNGs/19,806,000 bytes). Runtime is app
7b4a0ae7… with separate controlled core cf147e23… checkpoint; X11 launch metadata
does not independently hash core per launch. Owned cleanup is 0/0/-15, with host
GNOME identity unchanged. Ledger 1–3 Passed, 4–5 In progress and 6–12 Pending.
Tutorial 4's actual resource construction encountered a tank creation-order issue
and failed milestone save; reconstruction is underway. Tutorial 5 has initial Help
blocks entered through a blank actual private GUI editor, with resource entry
continuing and no reference consulted. Neither is completed walkthrough evidence.
Full Linux replacement, host/hardware and other tutorial gates remain open.
No old matrix/corpus was repeated; Windows/macOS/MATLAB remain deferred.


### Help-discovered resource fixes and rebuilt runtime — 2026-10-02

[tutorial-resource-fixes-20261002.md](Qt6ParityValidation/tutorial-resource-fixes-20261002.md) records valid coupled ChemicalTank
settings, Local celestial-body Origin choices, owned propagator Type/FM retention
and scoped wx-style default force models for new numerical propagators. Explicit
existing model sharing and imported implicit models remain preserved. Actual creator
Cancel/name changes/Create, rollback, source/Undo/Redo/Unicode save-reopen pass four
new focused checks. Creation's initial exception was a test SRP getter mismatch;
both failure attempts remain retained, and only Creation was rerun after correction.
The rebuilt app is SHA256 7fb5bd6cdf8c2630599a2f8c4d20c5f1ce680c41f297ab8fc1ac158b4664f6d4,
with controlled core cf147e23… and selected startup 5f80be1f… unchanged. No old
passing matrix/corpus was repeated and no numerical-engine algorithm changed.
Tutorial 4's affected actual GUI steps resume separately. Tutorial 5's five own
stages now execute successfully, including a 5000.202164764 km lunar periapsis
exercise (11.291 s/four nominal passes); its GUI-authored final source has been
saved/reopened and frozen before reference comparison. Neither walkthrough is
marked Passed by this resource-fix checkpoint. Full Linux/host/hardware and other
tutorial gates remain open; Windows/macOS/MATLAB remain deferred.


### Case-sensitive Mars gravity default repaired — 2026-10-02

[mars-gravity-default-20261002.md](Qt6ParityValidation/mars-gravity-default-20261002.md)
records the actual Tutorial 4 NearMars Apply failure and Qt's corrected startup
resolution of MARS50C to official Mars50c.cof. Retained custom files and unrelated
forces remain intact. New MarsGravity passes 0.21 s; directly affected GravityBodies
passes 0.97 s. No extra Mars numerical mission or other passing suite/corpus was
repeated. The rebuilt app is 3ba673f1b2060dc6957d34f94ca11e9f48db3273374afb7d58da7c6654f52ef1,
with base cf147e23…/selected startup 5f80be1f… unchanged. Actual Help 4 resumes
from its own saved resource/view milestone; Help 6 begins independently from its
Help instructions. Full replacement/host/hardware and remaining tutorial gates
stay open; Windows/macOS/MATLAB remain deferred.


### Private portal completion and Output Return — 2026-10-02

[private-wayland-followup-20261002.md](Qt6ParityValidation/private-wayland-followup-20261002.md)
records current app3ba673f1…/basecf147e23… actual private portal Open acceptance
(Response0/owned fixture URI) and Save As cancellation (Response1/no new file,
unchanged fixture), using the repaired request-ID helper. One GUI F5 completes
0.595s; pending Orbit Earth close Cancel/Discard and readback retain the resource.
A single Output leaf Return reopens the retained Earth/trajectory/camera at shared
48.6%. Earlier successful portal cases, main minimize/restore and numerical
matrices were not repeated. Seven unchanged PNGs and 209 indexed raw files retain
unsuccessful intermediate actions. Helper quit was acknowledged; parent sandbox
teardown was -15, with a transient owned compositor in its immediate inventory.
Later owned-PID absence and unchanged user GNOME PID/start ticks are recorded;
no graceful GUI Quit, host/physical-GPU/crash or full replacement acceptance is
claimed. Remaining Help walkthroughs continue separately. Windows/macOS/MATLAB
remain deferred.


### Optimal lunar flyby Help walkthrough passed — 2026-10-02

[Tutorial 5](Qt6ParityValidation/help-tutorial-05-20261002.md) is Passed for bounded
private actual-input construction through its expressly taught code editor. All
five stages are covered: deliberate configuration Stop, patch-only/full/alternate
solves, and the 5000-km exercise. Actual own reopen/build precedes the
19:20:39.894341 UTC freeze of the 18701-byte source, SHA256
db65522abd06f9f4be8a52fb7c1b835db9e937751ab06887871595f9aa28f86c.
The own exercise converges in 11.291 s/four nominal passes; radius is
5000.202164763518 km and all eighteen equalities are retained.

Post-freeze original sample Step 5 fails in 162.538 s/44 passes/43 report iterations;
radius 4860.44617331 violates 5000. Its whole failure report equals the own first
failure only after 44 inequality-owner labels normalize. The reviewed repair
appends 22 prior own nominal-stage4 controls in all three Step 5 copies, preserving
physics/constraints/solver and archived originals. One actual updated-sample
CtrlO/Convert views/F5/readback passes 10.977 s/four passes/three report iterations.
Its whole 9854-byte report SHA256 72531ef34b947aa49bb04ceb826a626aa7143d8bb4eef36a6ccd969b5238a588
equals the frozen own 9850-byte report only after four printed backward-to-forward
Luna.RadPer labels normalize. No reference conversion Save or Steps 1–4 rerun.

Help copies are synchronized but not separately executed; original sample Step 3
Earth gravity differs, Help epoch-qualification equivalence is untested, and final
MOI perturbations/pass counts remain explicit. This is complete-report equality,
not every-state/visual/scientific parity. Ten unchanged PNGs and the frozen own
source accompany the record; full raw/source/result histories remain indexed in
ignored Tutorial 5 evidence. Runtime updated app 3ba673f1…/controlled base cf147e23…
is bounded private X11/softwareGL evidence; host/hardware/full replacement gates
remain open. Ledger 1–3 and 5 Passed, 4 and 6 In progress, 7–12 Pending.
Windows/macOS/MATLAB remain deferred; no old matrix/corpus was repeated.


### Mars B-plane Help walkthrough completed — 2026-10-02

[Tutorial 4](Qt6ParityValidation/help-tutorial-04-20261002.md) passed independent
Help-driven resource and command construction. The first B-plane target meets
both 1e-5 km tolerances in six iterations; Apply Corrections reduces the next run
to one. Mars capture reaches 12000.01965272428 km within its 0.1 km tolerance in
11 iterations; its corrections also reduce the next run to one. Four exported
summaries and three native views show fuel consumption, polar arrival and capture.
Actual own save/reopen precedes the frozen 8628-byte source at 19:57:35.160009 UTC
(SHA e889a806…). One later reference GUI run satisfies the same targets; the
original sample remains unchanged. Source/default and drawing differences remain
explicit, without claiming complete-report, every-state or pixel equality.

All eight private sessions and intermediate failures are retained and indexed.
The final authored and reference sessions exit through the GUI with owned process
statuses 0/0/0. App 3ba673f1…, controlled base cf147e23… and startup 5f80be1f… are
unchanged; no old matrix or corpus was repeated. Other tutorials and full Linux,
host, hardware and crash gates remain open. Windows/macOS/MATLAB remain deferred.


### Electric propulsion Help walkthrough completed — 2026-10-02

[Tutorial 8](Qt6ParityValidation/help-tutorial-08-20261002.md) passed independent
Help-driven electric hardware and finite-burn form construction, one two-day run,
spiral/fuel inspection and actual own save/reopen before freezing the authored
5750-byte source at 20:08:14.975711 UTC (SHA 9e80e251…). The own run completes in
0.850 s, reaches SMA 8362.8537296235 km and consumes 20.96985583229 kg of fuel.

Post-freeze comparison found the original sample used a polynomial thruster and
Earth shadowing instead of the Help's constant 5 N model and empty shadow list.
Only those two settings are repaired. One changed-reference GUI run completes in
0.900 s and its full 3283-byte propagation summary matches the frozen own summary
byte for byte. Original results, all three private sessions and hashes are
retained; nine unchanged PNGs and the authored source accompany the record.
Execution uses app 3ba673f1…; a later function-output fix rebuilds ef933221… without
repeating this unchanged chapter. Full Linux, host, hardware and crash gates and
other tutorials remain open; Windows/macOS/MATLAB remain deferred.

### Eclipse and station contact Help walkthrough passed — 2026-10-02

[Tutorial 7](Qt6ParityValidation/help-tutorial-07-20261002.md) passed bounded
private actual-input construction of the Help chapter's core scenario from the
independently authored Tutorial 2 prerequisite. Earth shape, EclipseLocator,
Hyderabad station and ContactLocator were entered through resource forms; actual
Output reports show one 2105.5299296-s eclipse with three portions and two station
contacts. The combined run completes in 1.645 s. Own save/CtrlO reopen/Build and
resource readback precede the 20:12:02.032395 UTC freeze of the 6498-byte source,
SHA256 c7964540d1e47ff970989e9b8bfbe880ab308fb5c01554a4d69c27a8b1d709d5.

The blank-target initial attempt is retained as an operator correction. Help
names only the Simple Orbit Transfer prerequisite; no final shipped Tutorial7
script exists. Post-freeze additions match Help settings and qualitative report
expectations; prior compatible Tutorial2 reference evidence is reused without a
passing rerun. Default/Target/view differences remain explicit, as do optional
station-network/burn-coverage exercises and the missing GUI LSK field. Nine
unchanged PNGs and frozen own source accompany all raw actions/reports/identities
in the ignored indexed Tutorial7 evidence. App 3ba673f1..., controlled base
cf147e23... and startup 5f80be1f... are unchanged across both private X11/softwareGL
sessions. Bounded/acknowledged-helper cleanup is 0/0/-15, not graceful GUI Quit or
host/Wayland/portal/hardware/crash/full replacement acceptance. Windows/macOS/MATLAB
remain deferred; no old matrix, corpus or prerequisite-reference run was repeated.


### DSN simulation Help walkthrough passed — 2026-10-02

[Tutorial 9](Qt6ParityValidation/help-tutorial-09-20261002.md) passes bounded
private actual-input construction through the Help-prescribed empty script
editor. Staged resource Save/Build steps produce the short four-observation
simulation in 0.081 s. A separate editor-authored ramp file and three stations
produce the full noisy 21-day scenario in 22.963 s: 1348 paired range/TCP
observations, 505 hourly epochs, CAN/GDS/MAD counts 210/233/231 per type.
All 674 range frequencies match Help ramp metadata; IDs/modulo/interval and
finite values are checked. Full observations are retained for Tutorial10.

Own SaveAs/Ctrl+O reopen/Build and normal 0/0/0 cleanup precede the
20:21:58.590173 UTC freeze. Source SHA492402ba.../ramp d91497ac.../observations
a66ac094... remain unchanged. Post-freeze explicit numerical settings match
both samples, with attachment-order and owned-path differences recorded.
Earlier source-matched corpus successes support comparison; no needless noisy
reference rerun occurred. Seven unchanged PNGs, both authored sources, ramp and
full own observations accompany indexed raw actions/logs/failures. Short app
3ba673f1... and realistic ef933221... retain controlled base cf147e23... and
startup5f80be1f... . This covers the prescribed script-editor workflow; full
scientific/host/Wayland/hardware/crash and remaining tutorial gates stay separate.
Windows/macOS/MATLAB remain deferred; no old matrix or corpus was repeated.


### Event-locator creation defaults repaired — 2026-10-02

[Focused creation evidence](Qt6ParityValidation/event-locator-creation-20261002.md)
records the empty-target issue found in actual Help tutorial 7 construction.
New drafts select the first existing spacecraft and creation persists it even
when untouched. Explicit spacecraft/Contact regions are retained; invalid
choices reject before mutation; merely opening/Cancel creates no spacecraft.
One new offscreen widget regression passes in 0.37 s; independent source review
found no substantive regression. The first test compile error and identity
recorder correction remain recorded; no old suite or corpus was repeated.

The application is rebuilt as e5337acd... with controlled core/startup unchanged.
Subsequent independent Help 10–12 runs exposed a separate shared estimator
residual-plot issue: legends appear but samples are dropped, although numerical
reports complete. That plotting fix and the remaining tutorial/host/hardware
acceptance continue; no full replacement pass is claimed. Windows/macOS/MATLAB
remain deferred.


### GMAT functions Help walkthrough passed — 2026-10-02

[Tutorial 6](Qt6ParityValidation/help-tutorial-06-20261002.md) passes bounded
independent private-X11 resource/mission controls and literal Help function
editor construction. The repaired named no-output function call and Global
sharing support targets inside/outside the function: 6+11 iterations. Full
mission 4.748 s and one justified required-observation run 4.915 s satisfy
B-plane goals 1e-5 km and RMAG 12000.017391244 km within 0.1 km. Actual Mars
MOI/Call/Achieve summaries, mass change -1075.9520123944 kg, shared three-view
0→47.6→88.6% animation and MarsView Output-tree/Enter reopen are recorded.

Own mission/function SHA75b78de1.../79745474... survive actual Ctrl+O/Build and
function editor readback, then final 20:43:38.795064 UTC freeze before source
comparison. Nine unchanged PNGs and exact sources/summaries/report/solver
accompany 269 indexed raw files, including all failures and both freezes.
The named-call Outputs-clearing defect/fix and harness mistakes are preserved.
Compatible earlier 4522 sample evidence (5.588 s) is reused without a new run;
scientific equality and Current-only full trial history are unclaimed.

All nine owned sessions closed with Xvfb/WM exit0 and application exit-15 from
controlled SIGTERM; graceful application exit/crash-proof behavior is unclaimed.
Construction app 3ba/repaired ef933/final e533 retain base cf147/startup 5f80.
Host/Wayland/portal/hardware/scientific and remaining tutorial gates stay
separate; Windows/macOS/MATLAB are deferred. No old matrix/corpus repeated.


### Estimator-owned residual plot data loss repaired — 2026-10-02

[Focused residual-plot evidence](Qt6ParityValidation/owned-residual-plot-20261002.md)
records the independent Help10–12 failure: bulk Deactivate discarded samples.
Separate Qt redraw suspension now retains data and forces Activate refresh;
ordinary Toggle admission remains independent. New actual OwnedPlot callback
regression passes 0.08 s after a disclosed test-only temporary-reference fix.
Rebuilt app b8074b02 retains unchanged base cf147/startup 5f80. One affected
actual GPS filter retry shows populated curves/export and identical CSV, with
report differences limited to Run Date. Chapter completion and host/physical
GPU gates remain separate; no old corpus or successful matrix repeated.


### DSN/GN orbit estimation Help walkthrough passed — 2026-10-02

[Tutorial 10](Qt6ParityValidation/help-tutorial-10-20261002.md) passes bounded
private actual-input construction from an empty Help-prescribed script editor,
using immutable own Tutorial9 observations/ramp. DSN estimation converges in
two iterations,1344/1348 used,WRMS1459.977978→.963600. Six blank residual plots
exposed shared bulk replacement data loss; after the focused receiver fix,
one affected34.357-s retry renders iterations0/1 and actual CAN range export
reconciles209 accepted rows. Text/PNG export and grid/legend toggles pass.

Mandatory GN appendix actual SaveAs/Edit→Replace construction simulates1348
paired Range/RangeRate observations in24.271 s. Temporary-storage exhaustion
truncates the first estimator report; preserved as environmental failure,
not accepted complete evidence. One justified same-source/input retry53.322 s
produces a complete three-iteration report,1342/1348 used,WRMS431.749062→.959887,
relative2.19625e-7<.0001 and six populated curves. CAN RangeRate export209 points
reconciles accepted rows, rounding and known A1 epoch offset. GN final rounded
position truth error27.585935 km exceeds7.549834 km prior; convergence does
not imply improved truth accuracy. Misnamed ASCII export is not a PNG claim.

All three own missions Save/reopen/Build and normal0/0/0 cleanup precede
21:17:24.535580 UTC freeze. Post-freeze86 shared explicit properties match the
sample; known input/hardware/output differences and omitted defaults remain
explicit. Earlier source-matched55.659-s sample corpus execution is reused
with earlier-app/offscreen limits; no noisy reference rerun. Current patched
appb8074b02... retains basecf147e23.../startup5f80be1f... . Full indexed raw
evidence/failures,three authored sources,GN observations,exports and11 unchanged
PNGs are retained. MATLAB analysis and full scientific/host/Wayland/hardware/
crash gates stay separate; Windows/macOS remain deferred. No old matrix/corpus
was repeated.


## GPS filter/smoother Help walkthrough passed — 2026-10-02

[Tutorial 11](Qt6ParityValidation/help-tutorial-11-20261002.md) completes bounded
private actual-input construction of simulation, cold filter/smoother, warm
start, residual/covariance inspection, two ephemerides, one-day prediction and
Linux console Appendix B. Own source/readback and 201-file freeze at
21:20:41.811444 UTC precede any related sample access. The affected plot retry
retains the original CSV byte for byte. All 49 warm a priori covariance values
match report rounding; 145 cold and 36 warm observations are accepted.

The prediction writes 217 filter and 937 smoother state/covariance rows; the
one Linux GmatConsole run exits0 in1.343 s and reproduces both ephemerides byte
for byte. Reports match after Run Date and three equivalent path prefixes
normalize. Original storage-exhaustion zero-byte files, harness mistakes,
all stages/actions and controlled final app−15 cleanup are retained. Related
2010 GPS examples differ deliberately; source-matched earlier corpus results
are reused without noisy reference reruns. Appb807/corecf147/startup5f80 remain
bounded private evidence; new consolef80e and unused corrected Bulirsch0472
identities are recorded. MATLAB analysis/conditioning, host/hardware/crash and
full replacement gates remain open. No old suite/corpus was repeated.


### Bulirsch positive custom controls passed — 2026-10-02

[Custom-control evidence](Qt6ParityValidation/bulirsch-controls-20261002.md)
retains an actual Apply/readback failure and the narrow two-field copy fix.
One new focused test0.27s checks GUI/source and actual runtime clone retention;
one private GUI retry applies0.6/.0001, saves/reopens, runs60s Completed0.336s,
and opens/report-compares the finite endpoint with the saved command summary.
MinimumTolerance is deprecated/read-only/no-effect and correctly remains hidden.
Original numerical algorithms/core/startup are unchanged. Rebuilt plugin
SHA04726589... is required; appb8074b02... stays unchanged. Actual File Exit0 is
recorded separately from first helper termination and final helper's expected
already-exited-app diagnostic. Broad native/plugin/scientific gates remain open.

### Inter-spacecraft tracking Help walkthrough passed — 2026-10-02

[Tutorial 12](Qt6ParityValidation/help-tutorial-12-20261002.md) passed bounded
private actual-input construction through the Help-taught script editor. Ten
literal original Help blocks were entered into an empty new mission, saved,
run and reopened with simulator/estimator/command readback. Initial 12.392-s
execution converged in three iterations but both residual plots were blank;
that first failure and zero-sample export remain preserved. After the shared
OwnedPlot redraw/data-admission repair, one affected unchanged own 4.425-s retry
renders both plots and exports 806 Range/804 RangeRate points, reconciled to
final accepted report rows. Actual CtrlO/Build and final freeze 21:01:07.402213 UTC
precede reference access. All 126 operational statements match the original
sample; no original-reference GUI run or seeded authoring was needed.

The literal original tracking state is scientifically unsuitable: inertial
perigee 161.081 km lies inside Earth. A reviewed source-backed frame inference
changes only SimTrackSat/EstTrackSat CoordinateSystem to EarthFixed, retaining
state numbers and physics/solver settings. One changed-sample actual GUI run
completes in 3.406 s / two iterations, 1646/1652 observations, WRMS 0.990936275466, and
824/822 exported points reconciled to final report rows. GUI frame/Keplerian
readback agrees with independent polar-motion/LOD invariants, SMA 42166.24166491408 km,
ECC 4.033172844e-7. Preview changes were Discarded; no reference Save. The orbit
is near-geosynchronous, not an asserted historical recovery or general
scientific validation. Help illustrative GMD/report-frame excerpts were
refreshed only from that verified run. Original own 5478-byte source SHA 43e4aa2e...
stays unchanged; corrected sample SHA dcb5b405... and final Help 07777b2d... are
retained with indexed raw evidence and ten unchanged PNGs.

Construction app e5337acd...; affected/corrected sessions app b8074b02...;
base cf147e23... and startup 5f80be1f... are unchanged. Actual AltF4 closes all
three with 0/0/0; helper CLI exit 1 is its owned-exit handling, not a mission timeout.
Initialization-only EOF and transient /tmp exhaustion/recovery remain explicit.
No host GNOME/Wayland/portal/hardware/crash or full replacement acceptance is
claimed. Windows/macOS/MATLAB remain deferred. No old passing matrix or resource
construction was repeated.

The single build-qt-help build exits 0 at 21:31:25.812008 UTC; installed-content
checks at 21:32:17.193642 UTC confirm EarthFixed inputs and refreshed verified
GMD/report-frame excerpts. Supplemental build/check metadata retains the initial
checker path correction; no repeated build or mission.


### Positive FixedGrid report passed — 2026-10-02

[Valid owned frame evidence](Qt6ParityValidation/fixed-grid-20261002.md) records
actual selector/file/Apply/save-reopen and Output Enter. First insufficient-SPK
60-s fixture stays unqualified; GUI MaxStep 10/300s supplies adequate coverage.
One corrected F5 Completed0.052s gives 31 finite entries/one Sun event from
UTC 2015 Jan 1 noon to 12:05, matching explicit epoch/UseEntireInterval. No engine or
production code changed. Raw disk artifacts preserve both outcomes. Actual File
Exit app/WM/Xvfb 0; helper already-exited-app handling 1 is separate. Appb807/core/
startup unchanged; broader locator/native/scientific acceptance remains open.


### Ordered Python module paths and fresh restart readback — 2026-10-02

[The bounded path record](Qt6ParityValidation/python-module-paths-20261002.md)
qualifies Python Module Add/Replace/Remove/Up/Down/Browse in Set paths, pending
startup import/export that retains other source, Cancel/invalid rollback and
other-path Apply that preserves the live Python list. Imported pending rows remain
independent of the engine's stale list. Export and restarting with that file
are required for Python changes; no live reload/cache reset is promised.

One affected Paths check passes in 1.20 s; the new PythonPaths check passes in 1.17 s
after its preserved raw-absolute getter fixture failure is corrected, with no
production change or diagnostic rerun. Two fresh children select independent
13.25/23.5 constants. One private actual-input workflow adds/reorders/exports/
imports and reopens its GUI-authored source. A fresh exported-startup GUI run
completes in 0.031 s, opens report 23.5 and records the exact modules-second origin.
Both owned sessions close app/WM/Xvfb 0/0/0; helpers' already exited app diagnostic 1 is
separate. Full original 47 files / 5,064,944 bytes, startup/source/module hashes,
typing-protocol mistakes and five unchanged PNGs are retained and indexed.

Source a922195f... and actual app 787cd09b... / 5,876,904 B retain base cf147...,
util 1e4e... and production startup 5f80... . First-failure fixtures/logs and passing
stdout/source identities remain in the owned controls tree. Custom-path
ExternalForce execution, package/environment/cache behavior, host/Wayland/
portal/GPU/crash and full replacement gates remain separate. No old successful
matrix or corpus stage was repeated; Windows/macOS/MATLAB stay deferred.


### Earth Region overflight passed through the GUI — 2026-10-02

[The bounded Region record](Qt6ParityValidation/region-contact-20261002.md)
qualifies Regions category Add with paired four-row vertices, Contact one-SC
pending target/observer/output, Apply/Save As/CtrlO/readback and Output Enter.
One 300 s actual-input run Completed 0.052 s gives one 81.477054418 s contact on
UTC 2015 Jan 1 noon; 31 finite 10 s state rows bracket the +5° exit at 80–90 s.
Interpolation differs 0.000173537 s and is approximate, not exact SPICE/frame
identity. Own 1186 B source SHA 2825e9fd... remains unchanged; normal File Exit
app/WM/Xvfb 0/0/0 differs from helper's already-exited diagnostic 1. Unsatisfied
initial Save/Enter timing caused no commit/run. Source 2f9810c0/new Region 0.36 s and
EventLocators 4.65 s once retain production route/metadata failures and detached
context fixture correction. App 244dc41a... / 5,926,536 B/core/util/startup controlled;
no old matrix repeated. Broader locator/host/portal/GPU/scientific replacement
gates stay open, Windows/macOS/MATLAB deferred.


### Local Spacecraft/String function arguments passed — 2026-10-02

[The local-arguments record](Qt6ParityValidation/local-function-arguments-20261002.md)
adds one independently authored ordinary positional call through the actual
ordered pickers: whole ProbeSC/String LabelIn inputs and Variable ReturnedMass/
String Echoed outputs. After Apply/Save/CtrlO/Build, sole F5 Completed 0.021 s;
the exact 123.5/text report also retains the declared caller mass/text and actual
Output readback. Mission/function hashes remain unchanged. Full actions/screens,
incomplete initial chooser Save/Enter timing and normal owned exits 0/0/0 are
retained; helper already-exited diagnostic 1 is separate. App 244dc41a... /
5,926,536 B, compiled source 2f9810c0 and core/util/startup remain controlled.
Existing Array/global/zero-input passes were reused without repetition. Other
resource/signature/object-output, physical desktop/GPU/crash and full replacement
gates stay separate; Windows/macOS/MATLAB remain deferred.


### Two independent thrust histories through the GUI — 2026-10-02

[The bounded thrust record](Qt6ParityValidation/multi-thrust-live-20261002.md)
qualifies distinct two-segment histories, segment/tank readback, retained History
selection, separate one-spacecraft Begin/End controls, cardinality rejection,
Save/CtrlO/Build and actual Output Enter. Independent setup was typed in the
actual Script editor; only owned documented thrust data was supplied, no sample
mission seed. Original Synchronized F5 failed 0.351 s; its exact failure epoch
is unknown and the source-backed step-boundary explanation remains an inference.
The first sequential revision Completed 0.349 s but failed intended epochs/fuels
because ElapsedSecs goals are per-command durations. Only eight later literals
changed to 5; first 10 values and all physical settings/inputs were retained.

One corrected saved-source F5 Completed 0.330 s gives six finite boundary rows at
0/10/15/20/25/30 s. Max time/fuel residuals 2.165325e-7 s / 4.121148e-13 kg pass
independent 1e-6 bounds; final own source 5205 B SHA 317e151e... and 2502 B report
SHA c2a06348... remain exact. Actual AltF4 exits app/WM/Xvfb 0/0/0, distinct from
helper already-exited diagnostic 1. Original sources/reports, unsaved attempt,
600 s controlled -15 expiry after corrected Save, chooser/operator corrections,
full runtime identities/actions and eight unchanged PNGs remain retained/indexed.
App 244dc41a... / 5926536 B, core cf147..., util 1e4e..., startup 5f80..., plugin
e978... stayed fixed; prior cardinality 0.25 s was reused, no old tests repeated.

Synchronized boundary operation and combined asynchronous OrbitView remain
unqualified: completed final scene/log confirms absent SatA receives zero
placeholders in the shared callback and latest display; source appends earlier
history rather than clears it, actual point counts were not exported. No
trajectory truth/backward/time-varying-profile/solve-for, host/Wayland/portal/GPU/
crash or full replacement claim; Windows/macOS/MATLAB remain deferred.


### Custom-path Python ExternalForce passed after GUI restart — 2026-10-02

[The constant-force record](Qt6ParityValidation/custom-external-force-20261002.md)
qualifies Set paths Add/export followed by a fresh startup, actual force-panel
module/function/exclusion Apply, Save/CtrlO/readback and one 0.038 s run. Engine
and callback origin identify the owned module outside bundled paths. Two finite
rows match the independently known 60 s polynomial within predeclared component
bounds; all reported/nominal-time differences are retained. Source 1344 B SHA
8318112e... stays exact. Startup export normalizes the live FileManager snapshot,
with no original-comment-byte/live-cache claim; terminal-slash verifier correction
is preserved. Both File Exits app/WM/Xvfb 0/0/0 are separate from helper diagnostics.
App 244dc41a... / 5,926,536 B/core/util/startup fixed; no engine/build/test or old
matrix/corpus repeat. Package/environment/cache, host/hardware and full Linux
acceptance stay open; Windows/macOS/MATLAB remain deferred.


## Missing-spacecraft viewer checkpoint — 2026-10-02

The independently authored two-history case exposed a completed OrbitView that
placed SatA at Earth's origin when a subsequent SatB publication omitted SatA.
Commit `08cdda3c` carries explicit presentation availability metadata through
ordinary callbacks, Current replay and named-camera preparation. Qt retains the
last real pose and breaks the resumed trajectory; valid origin coordinates and
existing numerical arrays/commands remain unchanged. Legacy absent-data warnings
remain visible. The wx capability guard is source-reviewed, with no wx build claim.

[The repair/build record](Qt6ParityValidation/viewer-data-availability-20261002.md)
binds the rebuilt 5,935,320-byte application, SHA `989d3499...`, and core SHA
`2082a341...` to the recorded source. The new availability regression passes
0.31 s after its preserved test-only structural-notification assertion failure;
only the failed test target was rebuilt/retried. Affected InvalidPlotData and
SolverPlots pass once in 1.49/4.07 s and were reused.

[One actual-input affected retry](Qt6ParityValidation/multi-thrust-viewer-retry-20261002.md)
opens the frozen authored source, completes one effective F5 in 0.335 s, displays
both spacecraft off Earth's origin and opens the Output report. The entire
2,502-byte report is byte-identical to the frozen pre-fix report. Normal owned
application/WM/Xvfb exits are 0/0/0. No passing tutorial, corpus or old matrix was
repeated. This private X11/software GL evidence does not qualify the host desktop,
physical GPU or compositor-crash cause; synchronized thrust and other explicitly
recorded external/domain limits remain separate.


## Saved compositor fault mechanism identified — 2026-10-02

[Read-only saved-core evidence](Qt6ParityValidation/compositor-popup-null-resource-20261002.md)
resolves the prior PID 15518 fault to Mutter popup setup sending an event through
a null Wayland resource. Exact Ubuntu 50.1-0ubuntu2.4 symbols/source match the
recorded module build ID. Register/instruction evidence establishes the immediate
fault; optimized-out popup fields leave its owner and precise interleaving
unproven. The inspected current upstream branch retains the unguarded send;
earlier related repairs already exist in 50.1 and do not cover this branch.

No live reproduction, application test, host change or speculative GMAT patch
was made. The original crash record remains unchanged, and runtime SHA 989d3499…
/base SHA 2082a341… remains the previously rebuilt binary. Host/GPU acceptance stays
open pending an established safe host condition and its outstanding operations.
Completed tutorial/corpus/regression work was not repeated.


## Resource creation Help and ordinary controls — 2026-10-02

[The bounded control record](Qt6ParityValidation/remaining-gui-controls-20261002.md)
adds visible selected-resource Help/F1 to creation and Clone, repairs active report
search routing and stored OF camera aliases, and implements paired CustomFOV
creation/editing. The four focused checks pass; directly affected shared checks
are recorded without repeating completed audits/tutorials/corpus. One private
actual-input X11 case passes Help/F1/Cancel, paired FOV creation, Save/reopen,
Build and four-row readback. App/WM/Xvfb exits are 0/0/0.

Current GmatQt is 6,008,024 bytes / SHA167efe0f…, with base/util/startup unchanged;
compiled source snapshots and raw failures/corrections remain bound in the record.
The unchanged CustomFOV engine rejects the diagnostic's exact boresight; known
interior/exterior checks pass, without a numerical repair or broad scientific
acceptance claim. Live host testing stays stopped. Mutter's immediate NULL
Wayland-resource fault is established, while GMAT client attribution, repaired
host condition and physical GPU acceptance remain unproven. All nine gates,
external/domain limits and deferred platforms remain in the current map. The
full replacement goal stays active and full qualification is not claimed.


## CustomFOV fallback path lifecycle — 2026-10-02

[The focused path record](Qt6ParityValidation/custom-fov-paths-20261002.md)
repairs pending/detached CustomFOV construction and all five Qt interpretation
routes so configured FOV fallback expressions and aliases survive Help/Cancel,
relative-file creation/Apply, rejection, failed Build/rebuild and Unicode reopen.
The new path check passes 0.31 s; directly affected controls and folder checks
pass 0.35/2.60 s. The 15-failure baseline and corrected compile signature remain
recorded. Current GmatQt is 6,027,456 bytes / SHA02d7711e…; base/startup are
byte-identical and util changes for an additive read-only path-definition getter.
No numerical algorithm, new native scene or host test changed. Exact boresight,
other recorded domain/external limits and host/GPU/client-attribution gates remain
unqualified. Live host testing stays stopped; full qualification is not claimed.


## Separate Default camera reset and current evidence — 2026-10-02

[The camera reset record](Qt6ParityValidation/camera-reset-20261002.md)
implements the actual missing legacy Reset-to-Default operation; existing Script
view already restores Current. Separate primary/named Default metadata, history
and retained reset state survive translation, replay and source transactions.
Toolbar Reset and scoped Space pass the new 0.23 s check and one actual-input
private X11 case. Three affected passing camera checks are reused. Its full
420-byte report remains identical across camera actions, the saved calculation
section is byte-identical and normal owned app/WM/Xvfb exits are 0/0/0.
Build/accessor and test fixture corrections remain recorded.

Current app is 6,075,296 bytes / SHA8d791a3d…; base/util/startup are unchanged from
the FOV path repair. [Native relative-file FOV](Qt6ParityValidation/custom-fov-file-native-20261002.md)
adds actual creation/Imager/Save/reopen/run/report evidence at its recorded
SHA02d7711e… runtime; its helper-induced SIGTERM is explicit. The earlier shared
resource creation/Clone Help and F1 evidence stays accepted without a repeat.
[Rebuilt event Help](Qt6ParityValidation/event-help-startup-kernels-20261002.md)
corrects the obsolete panel kernel instruction and validates the installed page
and startup link without rerunning a completed tutorial.

All nine acceptance gates remain in the current map. Live host testing stays
stopped. Mutter's NULL Wayland event-resource fault is established; GMAT client
attribution, safe/repaired host condition and physical GPU acceptance remain
unproven. Broader recorded scientific/external limits and deferred platforms
remain visible. Full qualification is not claimed.
