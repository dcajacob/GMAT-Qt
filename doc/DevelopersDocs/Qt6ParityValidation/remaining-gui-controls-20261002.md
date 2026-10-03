# Resource Help and remaining ordinary GUI controls — 2026-10-02

Work is in `/home/dan/GIT/GMAT-Qt`, `codex/qt6-gui`, starting at
`ad343e74d9540001b5307e3153249b453a5ce85b`. The original checkout is untouched.
This increment implements ordinary missing controls; the
[current nine-gate map](current-acceptance-20261002.md) retains full scope.

## Changed operations and evidence

| Operation | Implementation and bounded evidence |
| --- | --- |
| Resource creation and Clone Help | Visible outer Help buttons open the selected resource's installed offline topic. F1 also works from the name field and nested draft editors, and the topic follows type changes. Pending names, values, array cells and function paths survive Help/F1; Cancel creates no resource or source change. Category-specific creation uses the same controls. Celestial-body factory types resolve to CelestialBody; Custom/Conical/RectangularFOV resolve to FieldOfView. Variable, Array and String retain their separate pages. The new ResourceCreationHelp test covers 16 types, GroundStation category Add, nested Array Help and Variable Clone. It passes 2.17 s before the FOV form addition and 2.11 s after that affected form is rebuilt. Buttons apply to all types offered by the shared creator; a published page is not guaranteed for every third-party plugin. Existing missing-page diagnostics/Contents remain available. |
| Report keyboard/menu search | MainWindow dispatches Find/Replace/Next/Previous to the active report, including reports embedded in comparison results. Removing ReportViewer's duplicate Ctrl+F resolves its shortcut ambiguity. The report Find button remains; read-only Replace opens local Find. Go to line remains script-specific. New SearchRouting passes 0.20 s with configured keyboard events, menu/button routes, wrapping and inactive script routing; source, dirty state, report file and active mission are unchanged. |
| Stored OF camera aliases | OpenFramesView conversion accepts SetCameraLocation/CameraEye/CameraCenter/CameraUp as the legacy stored Default aliases, preserving last-write source order across alias and canonical spellings. Original statements/comments remain in conversion comments. New OpenFramesCameraAliases passes 0.04 s for primary and body-relative secondary metadata, ordering/Off, Current precedence, invalid-source protection and Unicode roundtrip. This is source/metadata evidence; no new rendered scene is claimed. |
| CustomFOV creation and editing | The inline form exposes paired cone/clock or azimuth/elevation vertices, Add/Remove, file Browse/mode and interpolation. Grouped validation rejects incomplete/mismatched/nonfinite pairs and mutually exclusive inputs atomically. Indexed engine setters avoid its fixed-size vector Elevation setter. Initialization occurs on a separate clone so authored pairs/file are retained instead of expanded derived geometry. Source-backed reopening retains the authored representation. Detached replacement construction restores FileManager state before file resolution; valid/invalid edits and relative-file helper validation preserve the configured search path. Appearance-only edits keep geometry source exact. New CustomFovControls passes 0.32 s for creation/Cancel, failed-edit recovery, paired controls, representation/interpolation, file transitions, source/comments/Undo/Redo/Unicode reopen and Clone. Known interior/exterior visibility is checked through the unchanged engine. |

## Actual input on the rebuilt application

One bounded authenticated private Xvfb/Openbox session uses XTest input and
software GL, with a private settings directory and only OUTPUT_PATH changed in
its startup clone. It opens CustomFOV creation, types OwnedFov, clicks Help and
presses F1 from the name field; both display the installed FieldOfView page.
After closing Help, the pending name and paired controls remain. Cancel leaves
no OwnedFov resource. A fresh creator adds a fourth row and enters cone
`20 20 20 20`, clock `0 90 180 270` through actual table editing. Create registers
it and displays all four paired rows.

The final settled Save dialog saves `/tmp/ownedfov20261002.script`. Actual Open,
Build and resource double-click reopen the saved mask with the same four rows.
The preserved file is **5,156 bytes**, SHA256
`a3c6e5643d372f134029bb26ef8470d5c95952155b260f51dce0af2e7916a2be`.
No mission is executed. Main-window Close exits the application **0**; owned
WM/Xvfb cleanup is **0/0**. The helper returns 1 because its liveness check sees
the normally exited application before reading the queued Quit; session.json
records all three zero process exit codes and closed=true.

Raw actions, process/runtime identities and 26 displayed-pixel screenshots are
retained at:
`build/example-qualification/20261002/remaining-gui-controls/native-help-fov/isolated-x11-l9z012yv/`.
Screenshots 007/008 show Help/F1, 009 pending controls, 010 Cancel, 012 typed pairs,
013 creation, 023 actual Open, 025 reopened Build, and 026 four-row readback.
The saved source copy is `native-help-fov/owned-fov.script`.

Earlier driver attempts remain visible: an Enter sent before inspecting the
creator accepted a default Spacecraft; it was closed, undone and the source
rebuilt before the recorded FOV case. HOME was rejected by the helper's keysym
spelling. Two early Save attempts displayed “No such file or directory”; their
rejected filenames were not instrumented, so no application/harness attribution
is asserted. The final inspected filename and settled Save/Open button sequence
succeeds. Transient blank screenshots and earlier dialog images are retained,
not presented as successful reopen evidence.

## Numerical limitation exposed by the new fixture

The first FOV check failed **0.26 s** at an assumed visible exact boresight for
the cardinal cone-20 polygon. A standalone diagnostic directly constructing the
unchanged CustomFOV class, without the Qt form/helper, returns boresight 0/0
false, interior cone5/clock45 true, exterior cone40/clock0 false. A rotated
polygon and interpolated Az/El fixture agree. External rays through polygon
vertices are a source-based explanation consistent with this result, not a
proven/repaired numerical mechanism. The new GUI assertion uses the independently
confirmed interior/exterior directions. Exact boresight and broader scientific
CustomFOV regimes remain unqualified. No numerical-engine rewrite was made.
The frozen [direct diagnostic source](remaining-gui-controls-20261002.custom-fov-visibility-diagnostic.cpp) and [output](remaining-gui-controls-20261002.custom-fov-visibility-diagnostic.txt) are committed; findings and receipts are retained locally.
Detached-helper path preservation does not qualify all engine/interpreter
construction or startup-path regimes.

The standalone compile/run both exit 0. To reproduce from the committed diagnostic
with the existing configured build (output is owned under `/tmp`):

```python
from pathlib import Path
import shlex, subprocess, tempfile
root = Path('/home/dan/GIT/GMAT-Qt')
ninja = (root/'build/linux-gui/build.ninja').read_text()
block = ninja[ninja.index('build src/qtgui/CMakeFiles/GmatQtCustomFovControlsTests.dir/tests/CustomFovControlsTests.cpp.o:'):].split('\n\n')[0]
flags = dict(line.strip().split(' = ', 1) for line in block.splitlines()[1:] if ' = ' in line)
source = root/'doc/DevelopersDocs/Qt6ParityValidation/remaining-gui-controls-20261002.custom-fov-visibility-diagnostic.cpp'
with tempfile.TemporaryDirectory(prefix='gmat-fov-diagnostic-') as output:
    app = str(Path(output)/'diagnostic')
    subprocess.run(['/usr/bin/c++', *shlex.split(flags['DEFINES']), *shlex.split(flags['INCLUDES']), '-std=c++17', '-fPIC', str(source), '-o', app, str(root/'application/bin/libGmatBase.so.R2026a'), str(root/'application/bin/libGmatUtil.so.R2026a'), '-Wl,-rpath,'+str(root/'application/bin')], check=True)
    subprocess.run([app], check=True, timeout=10)
```

## Builds, focused checks and retained failures

Receipts are under `build/example-qualification/20261002/remaining-gui-controls/`.
SearchRouting initially reproduces the real duplicate Ctrl+F failure **0.19 s**;
CameraAliases initially fails **0.03 s** because the stored eye is lost.
`search-alias-fixed-check.txt` records their **0.20/0.04 s** passes.
Creation Help first fails **0.40 s** because its fixture asks for Propagator,
while the offered factory type is PropSetup and the Help page is Propagator.
Only that fixture is corrected; its retry passes **2.17 s**. The later affected
CustomFOV-form Help check passes **2.11 s**.

The first affected old-check invocation started before its build completed and
found three missing executables; OpenFramesSyntax passed **0.15 s**. After the
build finishes, the rebuilt ReportSearchOptions/OpenFramesSyntax/ScriptEditing/
Help checks pass **1.88/0.13/0.81/0.48 s**, total **3.30 s**. This operator error
and the subsequent receipts are retained. CustomFOV's initial failed assumption,
state-helper fix, direct-core diagnostic, test-only correction and **0.32 s**
pass are separately recorded. A duplicate staging hunk rejection is retained as
a transfer error, not an executable failure.

GmatQt was rebuilt after each application change. The final application is
**6,008,024 bytes**, SHA256
`167efe0f180609e29772cbad0e0a20e609113007fa182ffd59764b2a1859fe69`.
Base SHA256 `2082a341f5f123e81bad362972cd2b28a0a4753047ae51b5c09609e246be430c`,
util `1e4e7fe6ba83701266ef780588003c3ae607b6ebc56ecc040f9cfac399777fcc`, and startup
`5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a` are unchanged.
The committed [receipt](remaining-gui-controls-20261002.receipt.json) binds runtime/source hashes and focused results; local `final-runtime.json` and `compiled-source/` preserve the compiled sources. The [actual-input saved FOV source](remaining-gui-controls-20261002.owned-fov.script) is also committed.
Earlier passing search/alias and shared regressions are reused at their affected
source scope; they are not relabeled as tests of later unrelated code. Only the
Help check is relinked/repeated for the concrete CustomFOV form addition.

## Acceptance boundaries

All 108 wx audits, representative selected-plugin coverage, shipped-example
attempts with outcomes/dependencies, and all twelve independently constructed
published Help tutorials retain their completed bounded evidence. They are not
repeated. The four new checks and directly affected old checks do not establish
a fresh full-matrix pass.

Live host testing remains stopped. The saved desktop fault establishes a null
Wayland event resource in Mutter popup setup; GMAT client attribution, host
repair/safe condition and physical-GPU acceptance remain unproven. This private
X11/software GL result does not close those gates. No Mutter development or
system upgrade is performed. Historical TLE inputs, compatible MarsGRAM/data,
proprietary optimizers, retained example failures/timeouts, synchronized thrust
and recorded event/optimization/ephemeris domain limits stay explicit.
Windows, macOS and MATLAB remain deferred. Full qualification remains unclaimed.
