# Qt 6 desktop GUI

The `codex/qt6-gui` branch starts at integration commit `0ef9a8a`, retaining
the existing PR fixes and their regression infrastructure. The Qt frontend
links the same GmatBase and GmatUtil libraries. wx-specific behavior fixes
must also be implemented and tested in their Qt equivalents; ancestry alone
does not establish that equivalence.

## Design

Use Qt 6 Widgets with the recognizable GMAT desktop arrangement: Resources,
Mission, and Output navigation tabs on the left, an MDI workspace in the
center, messages below, menus and toolbar above. Retain platform palette and
font defaults. Start with familiar workflows before introducing new ones.

The script editor displays line numbers and colors GMAT commands/resource
types, numbers, quoted strings and percent comments. Keyword names are refreshed
from the loaded engine factories, including plugin types. Highlighting follows
the light/dark palette and does not change saved text or undo history. The
line-number margin grows with the document and follows scrolling/font changes.

The Window menu lists open workspace views, marks the active view, and restores
minimized windows when selected. Tile, Cascade, Next and Previous remain
available. Reopening a resource focuses its existing panel and preserves pending
edits; a clean panel from an older model is replaced with current values.

The old Qt experiment was inspected as reference. Only its message adapter
was reused initially, with changes to preserve UTF-8/local encoding fallback
and avoid indefinitely accumulating already delivered messages. The new
window and build integration do not depend on the old Qt 5 fallback or
placeholder plotting implementation.

## Build and first launch

Qt 6.4 or newer with Widgets and OpenGLWidgets development files, plus
OpenSceneGraph 3.6 development libraries, are required. The native orbit view
uses OSG rendering within Qt, without the OpenFrames viewer or its controls.
Texture decoding uses Qt image readers; OSG JPEG plugins are not required.
Spacecraft model files require the corresponding OSG reader plugins (3DS for
the bundled Aura model, OBJ for the synthetic native test). The renderer searches
the configured OSG library plugin directory and an `osgPlugins-<version>`
directory beside the executable, in addition to OSG's usual search paths.
In an existing configured GMAT build:

```
cmake -S . -B build/linux-gui -DGMAT_INCLUDE_QT_GUI=ON
cmake --build build/linux-gui --target GmatQt --parallel 6
application/bin/GmatQt
```

For Windows, use an x64 MSVC build and the matching Qt MSVC development package.
Add `-DCMAKE_PREFIX_PATH=C:/path/to/Qt/6.x/msvc2022_64` to the GMAT configuration,
along with the existing CSPICE and Xerces dependency options. The Qt frontend
can be built with `GMAT_INCLUDE_GUI=OFF`; it does not require wxWidgets.
After configuring, build and deploy the Qt runtime beside the executable:

```
cmake --build build/windows-qt --target deploy-qt --config Release --parallel 6
application\bin\GmatQt.exe
```

The Windows-only `deploy-qt` target uses Qt's `windeployqt` tool to copy its
libraries and platform/image plugins. It does not install the MSVC
redistributable or provide GMAT's other third-party runtime dependencies.
Those must already be available. This target prepares the build directory;
it is not a standalone installer.

The build generates `gmat_startup_qt.txt` beside the executable, including the
enabled native plugin targets with their actual filenames. Building `GmatQt`
also builds those plugins. The wx startup file is untouched. The Qt startup
uses relative paths and is installed in `bin`; Debug build paths account for
the separate debug directory. Use `--startup /absolute/path/to/file` to override
this configuration explicitly. The wx OpenFrames and OVtoOFI adapters are excluded.
The currently available OpenFrames plugin uses wx and must not be used as
a Qt widget provider. No Qt OpenFrames support is claimed yet.
Custom startup files are checked before engine initialization: active
OpenFramesInterface or OVtoOFI plugin entries produce an error in the Message
Window identifying the startup line. Commented entries and unrelated native
plugins are unaffected. This detects these known wx providers; it is not a
general compatibility check for arbitrary third-party plugins. Failed startup
also prevents the command-line mission from being loaded or built.

The `--screenshot /absolute/path.png` option captures the initialized window
and exits with failure if initialization, optional script interpretation,
or image writing failed. For headless layout inspection set
`QT_QPA_PLATFORM=offscreen`.
Add `--run` to execute the loaded mission before capturing it.

Linux installation has been staged with CMake, relocated to a path containing
spaces, and exercised from an unrelated directory without a startup override.
The installed copy ran the plot
fixture, loaded packaged maps, and resolved GmatBase/GmatUtil from the installed
`bin`. Qt itself is currently a system runtime dependency on Linux; this is not
a self-contained Qt distribution. Windows build-directory runtime deployment
has been tested; standalone Windows packaging and macOS deployment remain unverified.
The repeatable launcher check is:

```
python3 src/qtgui/tests/LaunchTests.py /path/to/installed/bin/GmatQt \
  src/qtgui/tests/propagate.script
```

It checks default startup, paths containing spaces, execution and capture from
an unrelated directory, and failure exit codes for missing or invalid inputs.

## Current evidence and remaining work

[The current acceptance map](Qt6ParityValidation/current-acceptance-20261002.md)
reconciles the workflow/viewer/plugin requirements, completed example and Help
phases, and remaining host/external limits. Historical milestones below retain
their recorded scope and do not constitute a new verification queue.

Mission-tree context menus provide transient **Breakpoint before command**
markers. Run (`F5`) honors those markers; **Debug mission** (`Ctrl+F5`) stops
before the first command when none are set. The paused inspector shows read-only
runtime objects with Spacecraft/All objects filters and current parameter values.
**Step command** (`F10`) runs the current command and pauses before the next;
function calls run as one step. **Resume** (`F5`) continues to the next breakpoint,
and **End mission** (`Shift+F5`) stops execution. Close/Escape resumes. Pause during
debugging takes effect at the next command boundary, rather than inside an
integrator step. The engine's normal dispatcher executes all commands.

Breakpoints do not modify or serialize script source. A rebuild of unchanged
source retains them; an accepted source change clears placements to avoid stale
command identity. **Clear breakpoints** removes all markers. Pending panel edits
must be applied/discarded before running or debugging, and runtime inspection
does not allow configuration editing. The debugger is limited to the main mission
scope; stepping into function-local scopes is not provided.

The Mission menu includes **Save and build script** (`Ctrl+Shift+F7`) and
**Save, build and run mission** (`Ctrl+Shift+F5`). Saving must succeed before
either action builds or runs. Edit > **Go to line** (`Ctrl+L`) navigates without
changing the script. Relative input/include lookup follows the open script's
folder; startup assets continue to use the startup directory. When using Save
As in another folder, relative asset references retain their script spelling
and may need to be selected again for the new location.

The force-model editor's **Python external force…** controls add the first
external contributor, edit its module/function and exclusion settings, or remove
it by unchecking **Use external Python force**. These changes stay pending until
the parent force model's Apply; Cancel discards the dialog changes. The module
selector lists configured Python search paths. Enter a module name without `.py`;
the engine does not import an absolute file path in this field. Restart GMAT
after modifying an imported Python module, because Python caches its code.

Mission Python calls provide a module selector, function name and ordered input
and output argument browsers. The module selector lists configured Python paths
and also accepts a runtime module name. These controls use the simple
`Python.Module.Function(inputs)` syntax, with optional outputs and a leading
command label with bracketed outputs. Empty-input calls may retain their original
parentheses or bare spelling, and a single unlabeled output may remain
unbracketed. Apply validates the script; missing modules or functions are
reported at execution and can be corrected in the retained mission editor.
Help opens the CallPythonFunction reference. Restart GMAT after changing imported
module code. More complex Python syntax remains a script editing workflow.

**Polyhedron gravity…** in the force-model editor manages one contributor per
celestial body. Add a row, select its body and closed triangle mesh, and enter
density in kg/m³; mesh coordinates are in kilometres. Remove selected rows to
remove their contributors. OK keeps changes pending until the parent Apply,
which validates the mesh and refreshes the retained panel. Invalid settings
leave the previous mission intact so the pending values can be corrected.

The force-model **SRP** and **RelativisticCorrection** On/Off choices add or remove
their physical forces on Apply. Enabling SRP refreshes the retained panel with
its radiation-pressure settings. Changes remain pending together with other
force-model fields; invalid mixed settings keep the previous mission and the
pending panel for correction. Disabling removes that force's configuration while
retaining unrelated gravity, drag and other source settings.

PrimaryBodies and PointMasses can change together with other force settings in
one Apply. A body cannot appear in both lists. Retained primary bodies keep their
gravity settings, and removing one removes its owned configuration. The
atmosphere dialog previews pending body selections, so selecting a new primary
and enabling its drag can be one transaction. Removing the primary for an
existing drag force requires disabling that drag or selecting a valid body;
rejected changes keep the previous mission and pending panel for correction.

File / **Open another script window…** and **New script window** keep additional
scripts in the MDI workspace. Window titles identify the active mission, inactive
scripts and unsaved changes; equal filenames show their full paths. Opening an
already open file focuses its existing editor, preserving its edits and Undo.
Resources, Mission, Output and ordinary Build/Run belong to the active mission.
Reading, editing, saving or searching an inactive script leaves that mission
loaded. Save, Save As, Find/Replace and Go to line use the selected script window.

Mission / **Make selected script active** validates and builds its editor buffer
before switching the active mission. Pending configuration panels offer Apply,
Discard or Cancel; rejected Apply keeps their values and the existing mission.
An invalid target script retains both buffers and restores the previous built
mission. **Save and build script** and **Save, build and run mission** first save
the selected script and then make it active. Each document keeps its own Undo,
modified state, filename and relative include/asset directory. Saving over a file
open in another script window is rejected to protect that independent buffer.

**Reload selected script…** asks before replacing the buffer and clearing its
Undo history. Read/encoding failures leave the buffer intact. Reloading the active
script invalidates the previous model until Build or Run; reloading an inactive
script does not change the active model. **Close selected script** offers
Save/Discard/Cancel for unsaved changes. Closing the active editor keeps its mission
loaded; View / **Active script** or reopening its path restores the editor. Run
protects every document from editing, closing or switching. Application close
also checks unsaved inactive scripts.

Spacecraft **OrbitErrorCovariance / Edit cells…** opens a fixed 6×6 initial
covariance grid. Labels and units follow the pending Cartesian or Keplerian
solve-for choice; Keplerian covariance uses mean anomaly (MA). The dialog explains
EarthMJ2000Eq estimation input, propagation axes, warm-start input precedence and
the batch estimator's UseInitialCovariance setting. Changing solve-fors changes
labels without converting the numbers. **Copy upper triangle to lower triangle**
is an explicit operation. OK checks finite values, symmetry and positive
definiteness using GMAT's existing filter factorization, then keeps the matrix
pending until the spacecraft panel's Apply. Cancel preserves the original values.
The display abbreviates numbers while editing/tooltips retain full precision;
initial columns fit the six state components and remain adjustable.

The initial shell compiles with Qt 6.10.2 on Linux, initializes the engine,
loads the default mission and renders the familiar layout. File loading
checks reads before replacing the editor; saving uses QSaveFile and only
changes document identity and modified state after successful commit.
Script files use UTF-8. Malformed or truncated UTF-8 is rejected before replacing
the current document, instead of silently substituting or dropping bytes.
GUI messages retain the UTF-8-first, locale-fallback behavior of the earlier fix,
including incomplete sequences at the end of a message.

Mission execution now services Qt events through GMAT's existing interruption
checkpoints. Engine and UI access remain on one thread. Run, Pause, Resume,
and Stop update the toolbar; active execution prevents script/model edits
and nested runs. Closing an active mission requests Stop and retains the
window until execution has unwound. `Moderator::SetUiInterpreter(nullptr)`
now safely detaches the Qt adapter during shutdown.

Command panels provide fields for Maneuver, finite burns, Vary, Achieve,
Minimize, NonlinearConstraint, Report, FindEvents and Target/Optimize headers.
These edit only matched source spans: command labels, comments, option order,
unrecognized options and nested branch bodies are preserved. Unsupported
syntax remains available in the command source editor. Apply still validates
the complete mission; the form itself does not replace engine validation.
Templates include solver branches, variables/goals, optimization constraints
and finite-burn pairs, using available configured resources where possible.

With `ShowProgressWindow = true`, solver branches create Qt MDI tables showing
variables, objectives, goals, desired values and residuals through GMAT's
listener interface. Convergence and failure are explicit. Closed windows do
not invalidate engine listener pointers; subsequent runs recreate them.
Mission stop/failure ends an otherwise unfinished progress indication.
This is current-value feedback, not yet iteration-history plotting.

Double-clicking a resource opens its main form, with extra supported properties
under **Advanced**. Ephemeris output, burns, thrusters, ground stations, plot
settings, event locators, dynamic data and tracking configuration use their
specialized forms directly in the panel. Spacecraft provide orbit/hardware,
ballistics/mass, attitude and visual-model pages. Other types use typed form
fields. Common fields appear once; the Advanced table holds remaining settings.
Subscriber window position, size, stacking order and minimized/maximized state
are hidden from both Setup and Advanced, as specified by the engine's property
visibility metadata. Existing script assignments remain supported and preserved.
Empty Advanced property grids are hidden; legitimate actions such as Object
drawing remain available.
Scalar numbers,
booleans, strings, enumerations and references are supported. Applying edits
changes a clone, validates it, compares its serialized settings with the original
resource, and patches changed configuration assignments into the current script
before interpreting the candidate. Unchanged settings retain their original
syntax and implicit defaults, including power-system epochs. Owned force models
are written together in engine order because their selectors create dependent
forces. Other resources, comments and mission commands retain their source. A rejected candidate restores the previous model
without changing editor contents. Successful changes update the script as
one undoable edit. Deletion also preserves surrounding source and grouped
declarations, and protects referenced resources, built-ins and celestial bodies.
Stale panels reject changes, and unapplied panel edits
prevent Build/Run from silently using older values. Plot object lists (`Add`),
XY Y-parameter lists (`YVariables`), and report parameter lists (`Add`) accept
comma-separated resource/parameter names. Apply replaces the complete list;
orbit visibility follows object names when reordering, with new objects shown.
Empty report lists can be populated and cleared. References use canonical
resource names (for example, `Luna`). Spacecraft hardware lists, thruster tanks,
finite-burn thrusters, and force-model primary/point-mass body lists are also
editable. Tank reordering preserves mixture ratios by tank name; newly added
tanks receive a ratio of 1, editable after Apply. Other compound lists and
indexed expressions still require the script editor.

Arrays and writable numeric vector/matrix properties have an **Edit cells…**
grid. It preserves dimensions, rejects nonfinite or nonnumeric values, and
keeps changes local until Apply. Cancel leaves the property unchanged. Array
values serialize as indexed initial assignments and participate in the same
validation, rollback and undo flow as scalar properties. Array grids offer row and column controls, preserve retained cells and fill
new cells with zero. Apply validates the resized mission; Undo restores the
prior dimensions and values. Arrays support 1–1000 rows and columns. The
Row/Column selectors jump directly to a cell; enter a finite number and click
**Set cell** (or press Enter). This edits the pending grid, then **OK** returns
it to the resource panel for **Apply**. Use **Expressions…** for formulas evaluated at mission start; see the expression
workflow below.

Spacecraft panels open on **Orbit and hardware** and use the engine's current
state-element labels. **Setup** contains the ballistics/mass, attitude and visual
model forms. **Advanced** provides the remaining fields, grouped by section and
searchable. Pending values survive page changes; one Apply validates all changes.
Cell matrices, file choosers and compound selectors may still open focused dialogs.

Variable and String panels provide a focused **Initial value** field. Use a
finite number for a Variable and literal text without enclosing quotes for a
String. Apply updates its configuration initializer while preserving later
mission assignments and comments. The change is undoable and checked against
the interpreted value before it is committed. Strings must fit on one line;
GMAT cannot represent every combination of apostrophes and percent signs.
An unsupported value leaves the prior source and model intact. **Show script…**
displays the applied configuration, excluding pending edits.

Use **Edit > Set paths…** to manage ordered GMAT function and Python module
search directories and the default output directory. Add puts a directory
first; Replace, Remove, Up and Down edit the pending search order. Browse picks
a directory. Equivalent directories are shown once, retaining their first
position. For GMAT functions, the mission directory takes precedence when a
mission is built. Python's standard and environment paths precede GMAT's
configured module paths. Output must exist and be writable; Apply relocates
the default reports and log. Explicit mission filenames keep their own
destinations. Resolve pending resource/command edits before opening this
dialog; it is unavailable during execution.

**Read another startup file…** validates and previews full startup settings as
pending state without changing the current session. Apply uses the pending
function/output settings and invalidates the built mission; build it again
before running. Python module additions, removals and ordering require
**Export startup settings…**, followed by restarting GMAT with that exported
file: `GmatQt --startup /absolute/path/to/exported-startup.txt`. Apply does not
change Python's search paths or reload imported modules. Close discards
unapplied changes.

Export writes a startup file separately from Apply, retaining other imported
startup source and protecting the current mission filename and its symlinks.
The Startup settings tab previews the exported content. Cached data and plugin
changes also require a new process. Relative startup entries resolve from the
executable location even when that file is exported elsewhere. MATLAB path
editing remains outside this Linux/no-MATLAB qualification.

Use **Edit > New resource** or the Resources context menu to add a resource.
Right-clicking a category offers its available types directly, such as
**Add GroundStation…** under Ground Stations or **Add Array…** under Variables,
Arrays, Strings. Each action opens the creator for that type. Resource children
inherit their category's creation options. Right-clicking the Resources root or
empty space retains the general creator; **Edit > New resource** also lists all
types. The popup closes before opening a creation dialog.
The larger creator contains the name and configuration together; there is no
separate name-only step. Leave **Name** blank to use the displayed type-based
suggestion, such as `GroundStation1`; the engine chooses an unused suffix.
Explicit names remain unchanged when switching types.
The dialog lists the engine's viewable resource types, including plugin formations,
ground stations, regions, functions, estimators, smoothers, process-noise models and
estimated parameters, plus Variable, String and Array. Variable and String creation includes an initial-value field;
values remain pending when switching between types. Arrays have row/column
controls (1–1000 each) synchronized with pending cell values. Unregistered drafts
use the same forms as existing resources; Cancel discards them without changing
the mission. Creation validates a complete candidate mission and is one undoable
script edit. Duplicate names, invalid identifiers, stale script snapshots and
pending panel changes are rejected. The resource appears in the tree and its
main form opens after successful creation. Some resource types require
further configuration before they can execute. Force models have their own
Resources category.

EphemerisFile creation includes a Spacecraft selector and writes that required
reference together with its declaration before validating the candidate mission.
If there is no spacecraft, create one first; the error leaves the script/model
unchanged. The existing Sun remains editable, but new Star creation is excluded:
the selected engine fails before its missing central-body setting can be assigned.

Under Regions, **Add PlanetographicRegion…** opens a form with paired latitude
and longitude rows. Choose Vertices, enter at least three finite pairs, and use
Add/Remove to change the boundary. Alternatively, choose Area file and browse to
a readable file in GMAT's PlanetographicRegion format. The inactive input mode
is retained while editing; the saved resource uses only the selected mode.
This engine supports Earth regions. Create/Apply validates the geometry and
keeps invalid entries available for correction without changing the mission.
For a ContactLocator with a Region target, choose exactly one spacecraft in
Observers; the choices follow the pending target before Apply. See the
[Region contact qualification](Qt6ParityValidation/region-contact-20261002.md).

The Resources context menu can delete unused resources. GMAT's dependency
checks protect resources referenced by other resources or mission commands;
built-in resources and generated parameters are protected separately. Pending
changes in the target's own panel must be resolved first. Unrelated open panels
do not block deletion. The serialized result is interpreted before committing
one undoable script change; a failure restores the prior engine model.
Unrelated panels retain their existing stale-snapshot protection after a change.

## Functional validation

The complete Qt suite is registered with CTest when `GMAT_QT_BUILD_TESTS=ON`.
From an already configured GMAT build, run:

```
cmake -S . -B build/linux-gui -DGMAT_INCLUDE_QT_GUI=ON -DGMAT_QT_BUILD_TESTS=ON
cmake --build build/linux-gui --target check-qt --config Release --parallel 6
```

`check-qt` builds the frontend, enabled native plugins and test executables, then
runs six checks: files, workflow, mission editing, plots, high-DPI plots and the
launcher. To rerun without rebuilding, use
`ctest --test-dir build/linux-gui -C Release -L qt-gui --output-on-failure`.
CTest locks serialize access to shared engine outputs even under parallel CTest.
Each GUI test uses temporary INI settings rather than normal platform settings;
the launcher uses the new `--settings-dir` option for the same isolation.
A small Python launcher supplies runtime DLL search directories on Windows.
The six-test suite passes on Linux and native Windows with Qt 6.10.2.
The Windows run uses MSVC 2022 with the wx GUI disabled. Its initial build
exposed Windows min/max macro collisions; the Qt frontend and its consumers
now define NOMINMAX. See [Windows evidence](Qt6WindowsValidation/README.md).
macOS has not been built or tested.

Enable and run the real-engine workflow executable with a host startup file:

```
cmake -S . -B build/linux-gui -DGMAT_INCLUDE_QT_GUI=ON -DGMAT_QT_BUILD_TESTS=ON
cmake --build build/linux-gui --target GmatQt GmatQtWorkflowTests --parallel 6
QT_QPA_PLATFORM=offscreen build/linux-gui/src/qtgui/GmatQtWorkflowTests \
  /absolute/path/to/host-startup.txt src/qtgui/tests/propagate.script \
  /tmp/gmat-qt-resource-editor.png
```

The screenshot argument is optional. The test has passed on Linux with Qt
6.10.2 and verifies actual 600-second propagation, repeat execution,
pause/resume/stop, exclusion of nested execution and editing, close-during-run,
invalid-script recovery, resource validation/rollback, stale panel rejection,
script undo, and resource-tree-to-Apply-button interaction. It does not prove
plotting, platform parity, or all property types. The separate wx/console
exit regression checks shared-engine behavior.

The workflow suite also creates a spacecraft through the New resource dialog,
checks name/type validation and undo, inserts a propagation command using the
created object, and verifies that it advances by the requested 60 seconds.
It also exercises deletion through the context menu and confirmation, rejects
resource/command dependencies, protects pending target edits, permits unrelated
open panels, restores deletion with Undo, checks independent variable declarations,
and runs the mission after deleting unused resources.

The Edit menu routes text actions to the focused editor, including resource
table cells and command panels. It retains the originating editor while a menu
is open, and does not fall back to the script when a navigation tree has focus.
Workflow and Mission tests exercise Paste/Undo/Redo in those panels and verify
that the mission script stays unchanged until Apply.
They also check repeated resource opening and refreshing stale clean panels.
The plotting suite checks Window-menu activation, minimized-window restoration,
closed-window removal, and safe handling of an entry whose window was closed.

The requested initial Linux Qt desktop is implemented and verified; see the
[acceptance audit](Qt6LinuxValidation/README.md). It is not a complete wx feature
replacement. Replacing wx entirely would additionally require specialized
resource and command forms, compound-property controls, broader plugin
compatibility, advanced graphics/plot style parity, wider functional coverage,
and platform packaging. Those limitations remain described below.
Linux and Windows have been built and exercised. Windows also passed a native
desktop launch with the deployed Qt runtime and no Qt directory on PATH.
macOS and a standalone Windows installer remain unverified.

### PR-fix audit

The integration baseline `0ef9a8a` is an ancestor of the Qt branch. Existing
non-Qt fixes remain present; Qt equivalents have separate runtime checks:

| Earlier fix | Qt evidence |
| --- | --- |
| Failed reload retains edits and undo (`bfe0c35`) | `GmatQtFileTests`: missing/malformed files leave text, dirty state and undo/redo intact |
| Failed save retains identity (`d8f14d4`) | `GmatQtFileTests`: failed new destination leaves title/dirty state/original bytes intact; later save succeeds |
| UTF-8 messages with locale fallback (`916d796`) | `GmatQtFileTests`: valid Unicode and incomplete trailing sequence |
| Reversible zoom and retained replay (`19dd83b`, `cd2eaca`, `34140f6`) | `GmatQtPlotTests`: inverse zoom and replay without history mutation |
| GroundTrack sampling, sparse data, station validation | `GmatQtPlotTests`: numerical report comparison, collection/retention settings, absent satellites, invalid station |
| Close/reopen and mixed plot safety | `GmatQtPlotTests`: mixed subscribers, close during execution, reopen and repeat run |

Build `GmatQtFileTests` with the other Qt tests and run it without a startup file:

```
QT_QPA_PLATFORM=offscreen build/linux-gui/src/qtgui/GmatQtFileTests
```

These tests exercise the real editor and atomic save implementation in temporary
directories. They do not initialize a mission engine or alter user scripts.
The existing wx/console exit regression remains a separate shared-engine check.

## Mission sequence editing

The Mission tree reflects the engine's nested command graph, including branch
ends and Else blocks. Double-click opens a command statement editor. The
context menu supports insertion before/after, append, and deletion of complete
commands or branches. Structural end commands cannot be removed independently.
Script events are edited as complete blocks. Templates help insert common
commands. A Propagation form supports one spacecraft, one propagator, and a
single parameter/value stop condition, including elapsed seconds or days. Resource
selectors use the current mission; changing the spacecraft also changes an elapsed-
time stop condition. The form and script text stay synchronized, preserving labels,
comments and an existing BackProp modifier. Multiple stops are edited through
Stopping conditions; propagator assignments use Propagators and spacecraft. Periapsis and apoapsis are supported by both stop editors.

Changes use an immutable snapshot of the complete mission with source ranges
matched within each branch. This distinguishes identical statements in different
branches. Edits patch the original script, preserving unrelated resource source
and implicit defaults. Engine formatting differences and generated optional
command dictionaries are matched without writing them into the source. Leading
comments, simple commands and For/solver headers without semicolons remain available in the controls. An omitted For step remains implicit.
A command that cannot be aligned safely remains a text edit; unrelated resource
definitions are never regenerated as a fallback. Applying validates the complete candidate before updating the script
as one undoable edit. Invalid candidates restore the previous engine model;
stale panels and unapplied changes receive the same protection as resource panels.

Build `GmatQtMissionTests` with `GMAT_QT_BUILD_TESTS=ON`, then run:

```
QT_QPA_PLATFORM=offscreen build/linux-gui/src/qtgui/GmatQtMissionTests \
  /absolute/path/to/host-startup.txt src/qtgui/tests/mission.script \
  /tmp/gmat-qt-mission.png
```

This real-engine test verifies nested branches, repeated-command identity,
replacement/insertion/deletion/append through numeric execution results,
invalid-edit rollback, retry, stale panels, undo, and Mission-tree-to-Apply
interaction. It has passed on Linux with Qt 6.10.2.

The workflow suite additionally opens a real Propagate command, verifies form
population and spacecraft selection, preserves an advanced statement, rejects
invalid duration input without changing the mission, and executes a form-selected
0.01-day propagation (864 seconds). Its optional screenshot captures this panel.

## Plotting and output

The Qt receiver now records and renders native OrbitView, GroundTrack,
GroundTrackPlot, XYPlot and DynamicDataDisplay publications. Plot windows use
the central MDI workspace and can be reopened from Output without losing
their recorded samples. The receiver retains numeric data independently of
window lifetime; a rebuild starts a fresh recording. Body and coordinate
system pointers are used only while receiving engine publications, never
during drawing or replay.

New and reopened plots receive focus after being shown, so activating the main
window does not unexpectedly raise the script over them. The default-mission
regression checks both retained samples and visible/frontmost plots after event
processing. A native Linux X11 launch from an unrelated working directory also
ran the default mission and captured its ground-track window in front.

OrbitView uses a native OpenGL surface with textured spherical bodies, shaded
lighting and depth-tested trajectories and spacecraft markers. It retains the
existing orthographic rotation, pan, reversible wheel zoom, Fit, image export,
and replay controls. Texture paths come from the configured celestial bodies;
missing images fall back to the body color. Body-fixed orientations are captured
at each recorded epoch and replayed without consulting live engine objects.
XY and ground-track chart headings shorten to fit narrow windows. Legends wrap
into rows and reserve space above the chart; excess entries use a count instead
of drawing over the data. Hover over the chart for complete title, axis labels
and visible curve names. Wider windows restore full labels automatically.

Orbit and ground-track playback has Start, Play/Pause, Latest, a mode selector
and Slower/Faster buttons. Speeds step through 0.25×, 0.5×, 1×, 2× and 4×;
the button disables at each limit. Pause/resume retains the selected position;
dragging the timeline pauses playback. Latest follows incoming data, and Play
at the end restarts retained history. Separate timeline and mode/speed rows keep
controls accessible in narrow tiled windows. Speeds describe a nominal
three-second sweep at 1×.

Interpolated steps is the default: equal playback ticks cover equal amounts of
connected forward simulation time. Display positions, orientation, cameras,
vectors and trajectory tips interpolate between recorded samples. Propagation
steps visits retained samples in original order and exposes adaptive numerical
step spacing. Both skip callback-ID gaps from discarded solver trials. Resets
and disconnected edges jump immediately without inventing connecting arcs;
histories without connected forward time fall back to recorded steps. XY uses
recorded samples because its history has no simulation epoch. Raw numerical
samples and exports remain unchanged. See
[two modes and speed buttons](Qt6ParityValidation/interpolated-playback-20261003.md)
and [the segment-pause repair](Qt6ParityValidation/playback-segment-gaps-20261003.md).

The Standard toolbar's Animation controls share one playback clock across Orbit,
Ground Track and XY displays. Start pauses at the beginning; Play/Pause resumes
from the selected position, Latest follows incoming data, and mode, slider and
Slower/Faster speed controls apply to all retained plot histories. A plot reopened
from Output joins the shared mode and position. Position is the same percentage
of each plot's retained history, not a common simulation epoch. Tables contain
current values and do not replay. Using an individual plot's transport releases
shared playback and the toolbar reads Local; a shared control takes ownership
again. Rebuild and new-run clearing stop animation and reset the transport.
Debug mission uses a bug pictogram, distinct from the Run triangle; Ctrl+F5
remains its shortcut.

Configured spacecraft models retain their texture materials, display scale,
offset and rotation. Like wx, normalized spacecraft display size is exaggerated
(1000 km at ModelScale=1), so it is visible on orbital scales. Fit includes that
size. Missing models fall back to a marker and report a diagnostic. Spacecraft
attitude is recorded with its inertial-to-body convention correctly inverted.

The Sunlight button selects illumination from the recorded Sun ephemeris;
turning it off uses light from the camera for inspection. Sunlight direction
and spacecraft/body orientation follow the replay frame. This is visual diffuse
lighting, including ambient fill, not an eclipse or radiometric simulation.
The orientation regression compares a body-fixed surface point with an independent
coordinate conversion at the first and last recorded epochs.

OrbitView star fields honor `EnableStars` and `StarCount`. The startup file's
`STAR_FILE` catalog supplies equatorial directions and magnitudes. Valid entries
are sorted by brightness; the requested count selects the brightest entries,
capped by the available catalog. Missing/empty catalogs and rejected malformed
rows are reported without preventing a mission from running.

Stars use a fixed 50-degree vertical celestial field of view, independent of the
orthographic scene's pan and zoom. Camera rotation and the recorded inertial-to-
plot-frame rotation determine their directions, including Earth-fixed replay.
They draw behind bodies and spacecraft, without writing depth. Five magnitude
bands control display brightness and point size; this is not a photometric
simulation. The native tests verify count/disable behavior, magnitude, rear-sky
rejection, occlusion, pan/zoom invariance, camera rotation and replay; the real
mission tests independently check Earth-fixed frame conversion.

The offscreen/minimal Qt platforms retain the earlier CPU body-disk renderer;
they do not exercise the native renderer. When Xvfb is available on Linux,
`check-qt` also runs native OpenGL tests at scale 1 and 2 using software Mesa.
Those checks cover texture colors, foreground/background occlusion, body
rotation, portrait resize, model materials, Sun/camera lighting, replay selection,
missing-texture/model fallback, text overlays and repeated viewer lifetime.
`QtGui.NativePlots` additionally runs the real-engine plot workflow with the
native renderer, including close during run, reopening and repeat missions.

The view uses positions already converted by OrbitPlot; non-spacecraft bodies
are converted from the internal frame separately. Scripted cameras support
object or vector references, viewpoints and directions, viewpoint scale, and
signed up axes in the configured view-up coordinate system. Camera states are
captured numerically with the mission frames; replay and reopened plots need no
live engine pointers. Drag/pan/zoom remain offsets from the tracked camera.
**Fit** frames the mission; **Script view** restores the scripted framing.
Native and fallback rendering share the camera basis. Orthographic is the
default, with perspective and vertical field of view available through the
Qt camera controls and imported OpenFrames camera settings. Degenerate eye/target settings
are reported; a parallel up vector gets a stable fallback roll.

Native rendering also honors constellation outlines from `CONSTELLATION_FILE`,
XY/ecliptic reference grids, body wireframe and Sun-direction lines. Catalog
directions follow the recorded plot frame and remain fixed under pan/zoom;
rear-hemisphere segments are clipped. Celestial-body `3DModelFile` assets retain
their physical dimensions and configured scale/offset/rotation, with a sphere
fallback when loading fails. Constellation borders/names and eclipse shadows
remain outside this milestone; constellation outlines are implemented.

XY plots support the ten standard marker shapes, indexed marker changes and
point highlights, distinct dotted/long-dash/short-dash/dash-dot/transparent line
styles, and the error-bar visibility setting. Current-iteration clearing retains
its break anchor for subsequent iterations and bounds old anchors with history
trimming. Tests exercise repeated clears, indexed changes and distinct rendered
styles at normal and high display scales.

GroundTrack uses the engine's geodetic longitude/latitude, resolved body map,
collection/update frequencies, configured point limit and line width. Missing
satellites preserve their curve slots and break the line. Dateline crossings
split at the map boundary. The engine's SubscriberFactory maps the script type
`GroundTrackPlot` to the newer `GroundTrack`; the real-engine plot fixture now
verifies that alias against geodetic report coordinates, map loading, and its
retention limit. The older Cartesian callback is checked separately with a
known spherical projection; the current script factory cannot exercise it.
Ground station
markers reject objects that are not body-fixed points. Replay does not mutate
the retained histories. QPainter handles physical display scaling.

The plot resource's **SolverIterations** setting controls trial history: All
keeps all passes, Current replaces earlier iteration histories, and None keeps
accepted passes. In nested targeters, an inner solution remains a trial while
its enclosing targeter is solving. Plot filtering and colors respect that
enclosing state, including inner initialization. Stop and failed execution
release the display context before a corrected run.
`ShowFootPrints = All` draws the legacy five-degree reference circles, with
spherical geometry and dateline splitting even near a pole. These are not
computed sensor coverage or visibility-horizon footprints.

Dynamic tables preserve displayed values and cell colors when closed and
reopened. Output also lists report files and opens a read-only paged viewer.
Large reports can be inspected without loading the entire file.

Build `GmatQtPlotTests` with `GMAT_QT_BUILD_TESTS=ON`, then run:

```
QT_QPA_PLATFORM=offscreen build/linux-gui/src/qtgui/GmatQtPlotTests \
  /absolute/path/to/host-startup.txt src/qtgui/tests/plots.script \
  /tmp/gmat-qt-plots.png
```

This real-engine test compares final orbit XYZ, XY samples, and geodetic
longitude/latitude with a 16-digit GMAT report. It also verifies map loading,
sampling and retention settings, dateline interpolation, missing samples,
invalid station handling, close-during-run/reopen/rerun, reversible zoom,
replay retention, dynamic-table values/colors, and Output report opening.
The test has passed at display scale 1 and 2. The existing Qt workflow suite
continues to pass with the plot receiver installed. These checks do not
establish complete graphics or cross-platform parity.

The plot suite also exercises subscriber-list edits through the resource panel,
verifies that replacing XY parameters publishes the selected Z values, checks
added orbit-body data and reordered visibility, and rejects malformed,
duplicate, unknown, or noncanonical object names. Report list population and
clearing are checked against the rebuilt engine model.


### Resource, function and plugin workflows

Force-model editors expose owned gravity, atmosphere and radiation-pressure
properties under their canonical names, such as `GravityField.Earth.Degree`.
Spacecraft attitude controls follow the selected attitude model and display
representation; Apply refreshes the fields available in that representation.
Matrix/vector values use a cell editor. Hardware, force-body and supported
event-locator lists validate through reconstruction of the whole mission.

Function calls have argument/output controls beside their script source.
Event locators and functions appear in the resource tree and creation menus.
Solver and event-locator reports are accessible from Output. The Help menu's
**Available engine types…** lists the types registered by this runtime; this
separates actual plugin availability from dedicated Qt form coverage.

Opening an OFI-based example automatically offers **Convert views** before
building it. Build and Run offer the same choice for scripts pasted or edited
in the editor. Accepting validates the conversion and continues; **Keep
original** cancels the build without changing the script. The prompt explains
visual differences, with conversion notes under Details. Unsupported OpenFrames
features get a specific manual-conversion explanation instead of an unknown-type
build failure.

You can also use **Edit > Convert OpenFrames views for Qt**, or explicitly
approve conversion at launch with:

```sh
./application/bin/GmatQt application/samples/Ex_HohmannTransfer.script --convert-views
```

Conversion is explicit, validated, undoable and unsaved. Calculations remain
unchanged; viewer differences are documented in comments and the message
window. It imports common plot flags and named camera views into Qt OrbitView,
including perspective projection and vertical FOV. A camera selector switches
views during playback. Body-relative cameras follow object orientation;
two-frame look-at orientation is retained in both rotation modes. Named
trajectory-relative views use recorded regular arcs and copied endpoint poses;
see [bounded camera evidence](Qt6ParityValidation/named-arc-navigation-20261002.md).
Unsupported OpenFrames object kinds, Relative Velocity and Thrust Vector
modes, and dynamic viewer assignments require manual editing.
Keep the original file if it will also be used with the OFI application.

The Linux compatibility tests execute a GMAT function after editing its
arguments, verify the Yukon algebraic sample against its analytic optimum,
and locate eclipse intervals with editable event settings and an Output report.
See [parity acceptance evidence](Qt6ParityValidation/README.md) for the complete
scope and retained limits.


Table columns start with font-aware, content-based widths bounded for long
labels and values. Resource values receive spare initial space; empty numeric
editor button columns are hidden. All visible columns remain manually
resizable: drag a header border, or double-click it to fit the contents. Solver
and dynamic-data updates preserve adjusted widths for the open table. Widths
are not persisted after closing the table or restarting the application.


### Native window lifetime and desktop input

A hidden OpenGL composition anchor is created before showing the main window
on native platforms. Without it, opening the first OrbitView during Run made
Qt destroy and recreate the already-visible native surface. This is documented
[Qt behavior when adding the first QOpenGLWidget dynamically](https://doc.qt.io/qt-6/qopenglwidget.html).
Avoiding that replacement protects native input/focus continuity, especially
when Run was triggered by an input event on Wayland. The anchor lives until
the main window is destroyed; offscreen/minimal platforms retain the CPU path.

`GmatQtWindowTests` checks native-surface stability and exercises title-bar
minimize, Output activation, restore and repeat minimize in the real Qt event
loop. `QtGui.NativeWindows` runs it under Xvfb. To check the actual desktop
graphics path and saved settings without platform/rendering overrides:

```sh
build/linux-gui/src/qtgui/GmatQtWindowTests application/bin/gmat_startup_qt.txt --desktop-settings
```

The test reads saved layout settings but does not save changes. A sleeping
main thread in `QCoreApplication::exec()` alone does not establish a deadlock;
inspect input delivery/native window lifetime as well as rendering.


### Intel graphics stalls and missing OrbitView content

OrbitView uses a single-sample framebuffer. The previous four-sample MSAA
request reproduced Intel Iris Xe (ADL GT2) GPU hangs and context resets on the
GNOME Wayland desktop. The mission had 123 trajectory samples, but the captured
view was black and opening it stalled for several seconds. Disabling MSAA
rendered the textured Earth and orbit on the same hardware without a reset;
hardware acceleration remains enabled. Edges may be less smooth without MSAA.

The native window test now checks Earth-texture and orbit pixels after opening
OrbitView, in addition to window activation and minimize/restore behavior.
An optional fourth argument saves that captured image:

```sh
build/linux-gui/src/qtgui/GmatQtWindowTests application/bin/gmat_startup_qt.txt --desktop-settings /tmp/orbit.png
```

See [desktop GPU validation](Qt6ParityValidation/README.md#desktop-gpu-regression)
for the reproduction and validation evidence. Software-rendered Xvfb tests
alone did not expose this hardware failure.


### Replacement qualification in progress

The [workflow/viewer/plugin checklist](Qt6ReplacementQualification.md) tracks
remaining wx replacement work. Command settings now include For loop bounds
and step, If/While conditions, assignment destinations/expressions, Toggle
subscribers/state, and Global/Save object lists. Editing branch headers leaves
nested commands unchanged; Apply validates the complete mission.


Resource editors offer **Select…** for references with known engine types and
**Browse…** for filenames. Multi-resource selection preserves existing order;
drag rows to reorder them. Cancel keeps the old selection, and accepted choices
remain pending until Apply. Filename browsing permits new output paths without
creating files. Formation membership is editable and restricted to existing
spacecraft. Text entry remains available for expressions and plugin-defined
references whose choices cannot be enumerated.


XY plots have a **Style…** toolbar action with per-curve visibility, line/marker
controls, width, size, shape, color and error bars, plus plot grid and legend
switches. Choices remain pending while switching between curves. OK applies
them to the displayed plot, including existing samples; Cancel leaves it
unchanged. These are display settings for the current plot, not saved script
properties, and rebuilding the mission resets them.


Orbit plots offer **Camera…**, with **Orthographic / Perspective** and a vertical field-of-view
control (1–150 degrees, before wheel zoom). Orthographic remains the default.
Perspective uses depth-dependent sizing and clips objects behind the camera;
native stars retain translation invariance and follow perspective zoom. These
interactive controls currently affect the displayed plot only. Conversion imports
the selected OpenFrames view’s perspective/FOV into a `% GMAT-Qt-Camera` JSON
comment. Qt validates and restores it when running a saved script; the base
engine treats it as a comment. Resource/mission edits retain these settings.
Use **Keep projection** to write the displayed projection/FOV into the script
as one undoable edit, then save the script normally. It preserves mission
calculations and keeps the current viewer open. Unbuilt script edits and pending
resource-panel changes must be resolved first. Orbit angles, pan and zoom are
not included in this action.


### Array expression cells

Array resource panels offer **Expressions…**, separate from numeric initial
values. Nonempty cells produce ordinary GMAT assignments immediately after
BeginMissionSequence, evaluated in row order before existing commands. Blank
cells have no generated assignment and keep the numeric initial value. Formula
text is retained in a marked script block and shown when reopening the grid.
Apply validates the candidate mission without executing it; Save keeps the
formulas in the script, and Undo restores the previous block. Unknown references
or extra statements are rejected without changing the previous mission.

Apply numeric/dimension edits separately from expression edits. Existing
assignments elsewhere in the mission are left untouched and can still override
these initializations later; edit those assignments through the Mission tree.
This grid does not reinterpret arbitrary existing assignment code as a formula
block. Resource edits preserve the existing mission section, including comments
and formula commands, rather than replacing it with engine-formatted output.


Propagator resource panels expose writable settings from the selected integrator
or ephemeris propagator, including InitialStepSize. Apply preserves the separate
force-model section for numerical propagators. TLE, BulirschStoer and
PrinceDormand853 configuration, save/reopen, execution and recovery scenarios
are covered by the compatibility suite; the qualification checklist records
remaining plugin and specialized-editor gaps.


OpenFrames conversion preserves arbitrary camera up vectors in the optional `up`
array of the `% GMAT-Qt-Camera` comment. Qt applies that vector in the configured
ViewUpCoordinateSystem, preserving camera roll; the standard ViewUpAxis remains
a nearest-axis fallback for the base viewer. Keep projection retains this vector.
An explicit ViewUpAxis edit in the resource panel removes the override, and Undo
restores it. When editing raw script, remove the `up` member to use ViewUpAxis.
Zero, malformed or nonfinite imported vectors are rejected with an explanation.
Trajectory-relative orientation remains open.


### Named camera views

Converted plots with multiple OpenFrames views have a camera selector beside
projection. Each camera retains its own projection/FOV and recorded tracking
history. Switching restores that view's scripted pose, resetting interactive
rotation, pan and zoom, and follows the same replay time without rerunning the
mission. Closing and reopening the plot retains the selected camera; a fresh
mission run starts with the first view. Keep projection updates only the selected
camera, and normal Save preserves all named definitions.

Additional views preserve stored Current/Default eye, center and up vectors.
Object references track positions, including center offsets. With InertialFrame
Off, body-relative views also follow object orientation; InertialFrame On retains
plot-frame axes. Named whole-trajectory views support stored and automatic
framing. Named propagation-segment views preserve stored and automatic cameras
as described below. Body views without a stored location use the rendered
body/model bounds. Duplicate/unknown view names, invalid poses
and missing reference objects are rejected before execution. The base viewer
uses the first camera and ignores the additional Qt metadata.


Body-relative camera settings are retained in Qt metadata for the primary and
additional views. Eye, center offset and up rotate from body axes into the plot
coordinate system at each recorded sample. This applies to both celestial bodies
and spacecraft, with their different attitude-matrix conventions. Playback uses
these recorded poses without consulting live mission objects. Stored Current
location takes precedence over Default location; omitted stored components use
OF defaults, and malformed vectors are rejected. An explicit ViewDirection edit
clears an imported primary center offset. A body-relative primary view requires
an object reference and a vector eye offset; incompatible edits are rejected and
the previous mission is restored.


Two-frame LookAtFrame conversion rotates the entire stored eye/center/up pose
so its local +Y direction follows the reference-to-target vector. ShortestAngle
On uses the direct rotation; Off uses azimuth then elevation. The reference frame
can follow body attitude or retain plot-frame axes. This preserves the OF roll
behavior and differs from simply moving the center to the target object.
Coincident origins use the unaligned reference frame. Keep projection and
save/reopen preserve alignment modes; explicitly editing ViewDirection restores
standard OrbitView direction semantics, and Undo restores the imported alignment.

`tests/OpenFramesCameraReference.cpp` is an optional developer qualification probe
against an installed OpenFrames library, not a dependency of GmatQt. It compares
the shared alignment helper with the actual FollowingTrackball transforms in
absolute/body-relative modes, both rotation modes and singular directions.


### Save command

When the Save plugin is loaded, the mission editor offers a Save command template
and an Objects field. This command exports the current values of named resources
into the configured output directory; it does not save the mission script.
The shipped plugin writes one file named from the selected objects (for example,
`SavedSat_SavedNumber.data`). Repeated execution of the same Save command within
a loop appends snapshots; starting a new mission run replaces its previous file.
Exported resource definitions can be opened through File > Open.

The plugin now reports output-open, write and close failures, including the file
path, instead of reporting a successful mission with missing data. Rebuilding and
running after correcting the output path recovers normally. Copying commands and
reinitializing streams are safe, and export preserves each object's comment flags.


### Coordinate-system axes

Coordinate-system resource panels now offer **Axes…**. Choose an axis type and
edit its dependent properties together, then Apply to validate the complete
coordinate system. Changing the type resets pending axis edits; Cancel leaves
the mission unchanged. Apply pending edits in the outer resource panel first.
Primary/secondary objects use reference pickers, ObjectReferenced directions use
R/V/N selectors, and epoch fields are labeled A1ModJulian. Type-only changes also
apply correctly; previously the engine's generic Axes setter silently did nothing.

The main resource table exposes the selected axis model's writable properties.
ObjectReferenced settings require distinct primary/secondary objects and exactly
two distinct directions, leaving the third blank. Built-in coordinate systems
remain protected. Successful edits are undoable, and reopening reads the current
axis model. LocalAlignedConstrained edits reject zero or nonfinite vectors,
parallel alignment/constraint vectors, an alignment reference equal to the origin,
and a constraint coordinate system that references itself. Runtime geometry can
still become singular as objects move.

Workflow checks now cover MOEEq epoch edits and a Sun-aligned
LocalAlignedConstrained frame, including numerical transforms after save/reopen.
Other axis modes, dependency combinations and time-varying singularities still
need broader qualification.

### Report-file settings

ReportFile's Delimiter control names Space, Tab, Comma, Semicolon and Pipe; a
custom single ASCII delimiter can also be entered. Apply rejects empty,
multicharacter, quote and control-character delimiters other than tab instead of
silently accepting the engine's first-character truncation.

The Add list accepts whole parameters and array elements with positive numeric
indices, for example `Results(1,2), Sat.X`. Commas inside the array indices stay
part of the reference, including when reopening the selection list. Expressions
as array indices and the full wx parameter-selection workflow remain pending.


Report formatting checks cover fixed-width headers, ColumnWidth, left/right
justification and zero-filled significant digits. With AppendToExistingFile true,
each run adds its output (including headers when enabled) to the existing file;
with it false, a run replaces the file. Output report windows show their full
file path in the title and display read-only, unwrapped text. Large reports use
bounded pages with navigation to the complete file.


### Choosing report parameters

Report command settings include **Select…** beside Report file and Parameters.
ReportFile's Add property uses the same parameter selector. Choose an existing
reportable parameter or enter a reference, then add it to the ordered list. For
arrays, choose a row and column and use **Add element**, or use **Add parameter**
for the whole array. Remove, Up/Down and dragging control the selected order.
Cancel preserves the previous selection; applying the command or resource validates
references. New property references can also be constructed with the browser below
or entered directly.


The report selector now also offers **Browse object properties**. Select an object,
a reportable property and, when needed, its coordinate system, central body or
force model. **Use reference** fills the parameter entry; **Add parameter** adds it
to the selected order. Body-fixed properties filter incompatible coordinate
systems. Owned attitude properties and attached hardware are also available. Hardware
choices include only direct attachments of the selected object; typed references
and already-configured parameters remain available.


Hardware browsing checks cover fuel mass and a chemical-thruster coefficient,
including exclusion of unattached resources and a spacecraft with no attachments.
Owned attitude parameters use `Spacecraft.Property`; attached hardware uses
`Spacecraft.Hardware.Property`. The selector disables **Use reference** when a
required hardware dependency has no valid choice.


Force-model property browsing includes **SMADot** and **TLONGDot**. Their selected
force-model references are checked through save/reopen and numerical reports.
The shared engine also fixes TLONGDot's zero-perturbation case: an unperturbed
orbit retains its nonzero instantaneous angular rate instead of reporting zero.


### Mission command resource selection

Mission command forms now offer **Select…** beside burn, Maneuver spacecraft,
solver, event-locator and function references. Pickers show compatible configured
resources, and Cancel preserves the current field. Labels, comments and surrounding
command text remain intact. Vary accepts targeting and optimization solvers;
Target/Achieve and optimization commands filter to their respective solver types.
BeginFiniteBurn and EndFiniteBurn also provide a spacecraft picker; the engine
supports one spacecraft per finite-burn command. Maneuver includes a Backprop
checkbox for applying its burn backwards in time.


Toggle commands offer a **Select…** checklist for outputs and an **On/Off** dropdown.
Choose one or more subscribers; Cancel leaves the command unchanged. The checklist
supports dragging to retain your preferred order. Changes still go through command
Apply before updating the mission. Orbit View, Ground Track and XY plots retain
existing samples while switched off, suppress the disabled interval and start a
separate trajectory when switched on again. Ground Track uses any spacecraft
color assigned during the disabled interval when sampling resumes. Closing and
reopening a plot keeps its retained history. When a mission completes with a plot
switched off, Latest still displays all retained samples rather than only the
recent segments used during a live run. Completion does not switch the plot on;
a later run starts with fresh history.


Global and Save command forms have **Select…** object checklists. Choose
configured resources or user variables, arrays and strings. Global omits resources
that are automatically global from new choices; Save retains those choices.
Computed parameter values are not objects in this selector. Cancel preserves
the command. Global is also available in the insertion templates when registered
by the runtime. Apply validates structure; Global resolves deferred object names
during execution. Clear is not an executable command in the selected runtime;
the legacy text-form selector does not provide Clear runtime support.

### Script search and replacement

Edit → Find/Replace opens a reusable, nonmodal script search dialog. Find next
and previous use the standard shortcuts and wrap at document boundaries. Search
and replacement histories retain the last 12 entries for the session. Match case
and Whole words are optional; searches are literal text. Replace affects the
selected matching occurrence, while Replace All is one Undo operation and never
searches inside its newly inserted replacement text. Empty replacement deletes
matches. Read-only scripts cannot be changed. Search actions bring the Script
window forward, and failed searches report a visible message.

### Resource file selection

Browse uses an existing-file picker for known function, spacecraft model/SPAD,
ground-map, data-interface, thrust-history, custom-FOV and file-ephemeris inputs.
Report, ephemeris, event and estimator output paths use a Save-style picker and
may name files that do not yet exist. Selecting a destination does not write or
overwrite it; normal Apply/run handling still controls output creation. Cancel
preserves the current property, and accepted selections remain pending until
Apply. Unclassified plugin filename fields retain the general path picker.

### If/While condition builder

The Condition field includes **Edit conditions…** for comparisons of numeric
values, parameters and positive numeric array indices. Rows provide left/right
operands, all six GMAT comparison operators and AND/OR joins. Add and remove rows
as needed; Choose operand opens the shared parameter browser for the selected
operand cell. Incomplete rows disable OK. Cancel leaves the header untouched.
Accepting updates only the condition span; the command editor's Apply still
validates the mission and supplies Undo. Unsupported grouped or expression syntax
remains in the editable text field and is never flattened into rows.

### Single-parameter mission selection

Condition operands, Achieve goals/values, Minimize objectives and nonlinear
constraint operands now use a single-choice parameter browser. Select a configured
parameter, browse an object's property and reference frame, or choose an array
element. OK uses that one entry directly; there is no report list to assemble.
Empty entries and bare arrays disable OK. Report and ReportFile parameter lists
retain their ordered multi-selection workflow. Command Apply still validates the
chosen reference in its specific context.

Vary's variable picker filters for writable numeric parameters and array elements.
The assignment destination picker also offers user strings and whole arrays.
User variables remain selectable even though their system-parameter metadata does
not mark them as settable. Known read-only properties are omitted from property
browsing and cannot be accepted by typing them into these writable pickers.
Literal numbers cannot be selected as writable destinations. The original command
text fields still support direct editing and engine validation on Apply.

### Spacecraft SPICE kernel lists

The spacecraft SPICE section now includes orbit, attitude, spacecraft-clock and
frame kernel lists. Browse opens an ordered list: Add files accepts existing files,
Remove selected deletes entries, and dragging changes order. Duplicate paths are
not added twice. Cancel discards dialog edits; OK leaves them pending in the
resource editor until Apply. Applying replaces the complete list, so removal and
clearing work as expected. Paths with spaces or commas are retained; paths with
apostrophes, semicolons or embedded line breaks are not accepted by this editor.

SPICE orbit/attitude qualification now includes a shortened Mars Express example
configured through the kernel lists, Qt view conversion, save/reopen and report
comparison. Restoring an unavailable clock kernel permits rerunning without
restarting GMAT. The bundled example references a frame kernel absent from this
checkout, so the qualification fixture omits that file; the unmodified example
still requires its missing input.

### Spacecraft colors

OrbitColor and TargetColor now appear in the spacecraft Visualization section.
Choose color opens a visual picker; its button previews the current color. You
can also enter a GMAT color name or an RGB triple such as `[23 145 210]`. Selection
is pending until Apply, and Cancel leaves the value unchanged. Colors persist in
the mission script. Kernel-list properties are grouped together in the SPICE tab,
including attitude and spacecraft-clock kernel fields.

### Solver command options

Target/Optimize forms use dropdowns for SolveMode and ExitMode and a checkbox for
ShowProgressWindow. **Add default options** inserts only omitted settings using
the engine defaults, preserving existing settings and pending solver edits. The
action changes the command source for review; Apply validates it normally.
FindEvents' explicit Append option also uses a checkbox. Unrecognized values
remain represented rather than silently becoming a different setting.

### Thruster coefficients

Chemical and electric thrusters have a **Coefficients…** editor with separate
thrust and impulse/mass-flow tabs. Each row shows the coefficient name, editable
value and engine-provided unit. Columns start sized to their contents and can be
resized. The dialog includes pending property edits; Cancel leaves them unchanged.
OK checks every value is a finite number and returns the complete set to the
resource panel. Apply validates and saves the thruster changes.

FindEvents also offers **Add default options** when Append is omitted. Its Append
checkbox controls whether the event report keeps existing output or replaces it.
Adding the option preserves the locator selection, command label and comments.

### Camera controls in tiled viewers

**Camera…** opens a separate panel for the imported camera selector, projection,
field of view and **Keep projection**. These controls remain accessible when the
plot is narrow or tiled. Changes preview immediately; Close dismisses the panel
without reverting the preview. Reopening retains the current settings. Script
view, Fit and Sunlight remain available on the viewer toolbar.

### Live orbit display controls

**Display…** opens controls for axes, grid, object labels, legend, XY/ecliptic
planes, wireframe bodies, the origin–Sun line, stars and constellations. Stars and
constellations can be switched independently; enabling a previously disabled sky
loads its catalog from the selected startup paths. Turning them off retains the
catalog and StarCount for re-enabling. These choices survive closing/reopening
the retained viewer; a new build/run uses the script's settings.
Changes update the current view
without rerunning the mission or clearing replay history. Close retains the live
settings. These controls do not save script properties; use the plot resource
editor for persistent settings. Reopening the panel reflects the current view.

### Thruster tanks and mixtures

**Tanks and mixtures…** edits the ordered tank list and its ratios together. Add
selects a configured tank and starts its ratio at 1; Remove drops the selected row.
Up/Down move the tank and ratio as a pair. Ratios must be finite and positive but
do not need to sum to one. Columns are adjustable. Cancel leaves pending values
unchanged; OK returns both fields to the resource panel, and Apply updates the
mission. The separate property controls remain available.

Perspective **Fit** now uses the narrower of the horizontal and vertical viewing
angles. Tall, narrow tiled windows therefore keep the fitted scene inside both
edges. Stored camera distances remain unchanged unless Fit is selected.

### Stored OpenFrames trajectory views

Conversion now preserves whole-trajectory views with a stored Current or Default
camera location in the plot frame. They no longer track the spacecraft position.
The named trajectory object must be in the plot's Add list (or use
CoordinateSystem). Primary and additional named cameras are supported.
Automatic named whole-trajectory framing is also supported as described below.
Named propagation-segment views are supported separately as moving endpoint
frames, as described below.

### Large report navigation

Report windows provide First, Previous, Next, Last and a page-number control.
Each page loads roughly 1 MiB; rows may continue across pages, but UTF-8 characters
and Windows line endings remain intact. **Find…** searches the displayed page.
**Reload** reads the current file again, including replacements or appended data;
if the file shrank, the page is adjusted automatically. Read failures retain the
previous display and show an error. Viewing a report does not modify its file.

Report windows also provide **Search file**, which starts at the beginning and
searches the complete file for the entered text. **Match case** is enabled by
default; turn it off for case-insensitive Unicode text. **Whole words** excludes
matches within longer names, including names containing Unicode letters or
combining marks. **Next match** continues after the current result; **Stop**
cancels a scan. Changing an option cancels the scan and resets Next match.
Searches run in bounded chunks, navigate to the matching page, and highlight the
visible original text. A message identifies matches continuing on the next page.
The existing **Find…** control independently searches the displayed page.

### Propagate stop-condition selectors

The Propagation form has **Choose…** controls for the stop parameter and stop
value. They use the parameter browser with its object and coordinate-system
choices. Seconds/Days remain quick selections; another parameter is shown as
Other parameter. Labels, comments and existing backward-propagation modifiers
are retained when fields change. Apply validates the complete command.

The stop-parameter picker includes **Periapsis** and **Apoapsis**, with a central
body selector. Choosing either disables the stop-value field and removes the
numeric goal from the command. Existing labels and comments remain intact. Choose
a scalar stop parameter again to return to a parameter/value condition.

Propagation controls also include **Propagate backwards** and **Stop tolerance**.
The tolerance field shows GMAT's current default as its placeholder; leaving it
blank omits the option. Enter a positive tolerance to override it. Existing option
formatting, labels and comments are preserved when editing represented commands.

### Multiple propagation stops

**Stopping conditions…** opens an editable parameter/goal table in the command
editor. Add or remove conditions, move rows with their goals, or use the parameter
pickers. Periapsis/apoapsis rows have no editable goal. Columns are adjustable.
Propagation stops when any condition is satisfied. OK updates the source as one
undoable edit; Cancel leaves it unchanged, and Apply validates the mission.
Existing StopTolerance and OrbitColor options remain in the command.

**Segment color…** provides the wx propagation color override. Choose a color
or enter a GMAT color name or RGB triplet. Turn the override off to use each
spacecraft's orbit color. Cancel leaves the command unchanged; OK updates its
pending source, and Apply validates the mission. Existing stops, tolerance,
labels, flags and comments stay in place. Orbit View and Ground Track retain
the colors of each segment in their trajectory histories; solver trial samples
keep the target-color palette.

Mission assignments to spacecraft OrbitColor and TargetColor also reach Ground
Track. Each displayed sample uses the live mission's current colors, so removing
a segment override restores the updated spacecraft color. Earlier trajectory
samples keep their original colors, and a rerun starts from the saved configuration.

### Propagator assignments

**Propagators and spacecraft…** opens an adjustable table of propagators and their
spacecraft or formations. Add/remove rows, select a propagator, and choose one or
more objects for each row. The object picker also supports dragging to reorder.
Each object must appear only once, and empty assignments prevent acceptance.
Select Independent or Synchronized mode and optionally propagate backwards.
OK changes the command source as one undoable edit; Cancel leaves it unchanged.
Apply validates the complete mission. Existing stop conditions, options, labels
and comments remain intact. **Propagate STM**, **Compute A-matrix** and
**Propagate covariance** apply to all spacecraft in the command. Existing quoted
or unquoted flags load into these checkboxes. Covariance includes STM automatically,
so the STM checkbox is checked and disabled while covariance is enabled. Turning
covariance off restores the explicit STM selection. Opening and accepting unchanged
settings retains the exact original command, including the flag spelling and groups.
Covariance propagation requires Cartesian spacecraft in MJ2000Eq frames and
fixed-step integration with force-model ErrorControl = None. The existing engine
validates those requirements when initializing the run; use its reported error to
correct the spacecraft or propagator configuration.

An ElapsedSecs stop is a duration for that Propagate command. To report at mission
elapsed times 10, 15 and 20 seconds, use successive durations 10, 5 and 5 seconds.
The retained synchronized file-thrust attempt failed at a leader step and remains
unqualified. Separate independent commands preserve each spacecraft assignment;
their recorded result does not qualify synchronized thrust propagation.

### For-loop selectors

The For-loop command form includes a variable-only selector for Index and
parameter browsers for Start, Step and End. You can still type numeric bounds
directly. Cancel leaves the field unchanged. Header edits preserve the loop body;
Apply validates the command through GMAT.

### Applying array values and formulas together

Array numeric values and expression cells can now be changed in one resource
panel and submitted together with Apply. Validation succeeds for the complete
candidate or leaves both unchanged. Undo restores both as one edit. A resize that
would remove an existing expression cell is rejected; clear that formula first
or include its removal in the same Apply.

The expression grid uses the pending numeric grid dimensions, so formulas can be
entered in newly added cells before Apply. Existing formulas outside a pending
shrink remain visible in highlighted cells. Clear those formulas to accept the
expression edit, or Cancel and enlarge the numeric grid. Cancel preserves formulas.

### Power-system properties

Solar and nuclear power-system resources group general settings and bus
coefficients into tabs. Solar systems also have Solar coefficients and Shadow
tabs. ShadowBodies now has an ordered celestial-body selector; selections stay
pending until Apply. Invalid names are rejected without changing the script.
Changing EpochFormat converts the pending InitialEpoch while preserving the same
instant. Invalid dates leave both pending fields unchanged, with an error message.
Apply validates and stores the format and date together.

Solar ShadowModel offers None and DualCone in a dropdown, matching GMAT's supported
choices. Selecting None disables shadow attenuation when the mission runs.

Spacecraft DateFormat uses the same conversion behavior: choosing a new format
converts the pending Epoch instead of reinterpreting its text. Invalid dates
restore the previous format and stay available for correction. Apply stores the
converted epoch and format together.

### Spacecraft state representation

DisplayStateType offers the engine's representations compatible with the current
coordinate frame. Changing it converts the six pending element values and updates
their labels and units. Invalid values leave the selection and fields unchanged.
Apply coordinate-system, epoch or anomaly changes before converting the state
representation. Other pending properties remain available for the same Apply.

### Editing GMAT function files

Open a GmatFunction resource and choose **Edit function file…** to edit the file
at its FunctionPath inside Qt. The editor provides syntax highlighting, line
numbers, find/replace, Save and Cancel. Save updates the function file; Cancel
discards the dialog's pending edits. Run rebuilds the mission before executing it.

Saving preserves UTF-8 BOMs and uniform CRLF line endings. If another program
changes the file while it is open, Save leaves that file untouched and asks you to
reopen it. Unreadable or invalid UTF-8 files cannot be saved through this editor.
Save As writes a copy and updates FunctionPath as a pending resource change.
Apply that change to make the mission use the copy. Existing-file replacement
requires confirmation; failed saves leave the current path unchanged.

**New function file…** creates a file from a basic input/output template using the
chosen `.gmf` filename's basename. Choose an unused destination with a valid
GMAT identifier basename, edit the template, and Save. The
file is created only on Save; Cancel creates nothing. Apply the pending
FunctionPath to connect it to the mission. Existing files use Edit function file.

### Function-call argument selectors

The function-call form offers **Select arguments…** beside Inputs and Outputs.
Add, remove or reorder arguments; their order determines the function's input and
output positions. Whole supported objects, variables, strings, arrays and array
elements are available alongside the property browser. Output choices filter
writable parameters. The text fields remain editable, and Apply validates the
function signature. Quoted commas and array-index commas remain within one
argument when loading the selector. Cancel leaves the command unchanged.

Configured GMAT functions expose these controls for empty-input and bare calls
as well as calls with arguments. Changing a function retains the original call
spelling and comments. Adding inputs to a bare call inserts parentheses; clearing
those inputs restores the bare spelling. Single-output calls use the function's
context Help, and leading labels survive serialization and source-mapped edits.


Whole Spacecraft and String arguments can be selected as local function inputs;
Variable and String results can be selected as outputs. Declare the output
objects inside the function and use the input formal names from its header.
Avoid re-creating formal inputs inside the function: GMAT then ignores their
incoming settings. [One actual-input case](Qt6ParityValidation/local-function-arguments-20261002.md)
verifies the offered choices, ordered call, saved/reopened source and independently
known mass/text result.

### Omitted Vary and Achieve options

Use **Add default options** in a Vary or Achieve command form to expose settings
omitted from the script. Vary adds missing perturbation, bounds, maximum step and
scale-factor fields; Achieve adds tolerance. Values come from GMAT's command
defaults. Existing options, pending edits, labels and comments are retained.
Edit the added fields, then Apply to validate and update the mission.

Vary option fields follow the selected solver's capabilities. Unsupported
settings are disabled with an explanation, and their values are retained when
switching solvers. Select a configured solver to enable supported settings.

### DifferentialCorrector settings

The solver resource editor offers algorithm and derivative-method dropdowns in
Convergence. Output contains report style, report destination and progress
settings. Browse chooses a report output path; Apply stores the settings and
running the mission writes the report.

### Fit translated orbit scenes

Fit centers the visible objects and retained trajectories, so scenes far from the
coordinate origin use the window effectively. Hidden curves do not affect Fit.
Framing stays stable during replay. Script view restores the scripted camera.

### Automatic whole-trajectory views

When an OpenFrames view selects ViewTrajectory for a named plotted object and has
no stored camera location, conversion now creates an automatic trajectory camera.
It frames that object's retained path and updates while the mission runs. Named
views retain their own target paths. Replay keeps framing stable; Fit frames the
whole scene, and Script view restores the selected trajectory view. Automatic LookAt targets retain both ShortestAngle modes, including roll and
bounding-center orientation. Automatic CoordinateSystem views also retain the
OpenFrames default radius and viewport-aware distance, with or without LookAt.
Named propagation-segment cameras use the moving segment frame described below.

Editing the primary OrbitView camera's reference, position, scale, direction or
up settings in the resource editor turns off imported automatic trajectory
framing for that camera. Your explicit pose then takes effect. Additional named
views retain their own settings, and Undo restores automatic framing.

Automatic CoordinateSystem views without LookAt use OpenFrames' twelve-Earth-
radius framing. With LookAt they use its root-frame one-unit radius fallback.
Stored Current/Default locations still take precedence. Fit remains available
for framing the actual visible scene.

### Automatic body cameras

Converted views of a planet or spacecraft without a stored camera location now
frame that object's displayed size. Native model framing includes model scale,
rotation and offset, while retaining Qt's model display convention. Relative and
LookAt orientation continue to follow the selected view. Manual primary-camera
pose edits override automatic body framing; Undo restores it.


The spacecraft property panel offers **Ballistics and mass…** for the compact
Spherical and SPAD file controls from the wx interface. Enter mass, coefficients,
areas and SPAD scales; use Browse for the input files and the interpolation
choices for each file. OK keeps changes pending in the property panel. Apply
updates the mission, and Cancel leaves the pending values unchanged.


Spacecraft tank and thruster attachment pickers include **Select all** and
**Clear selection**. Individual checkboxes and row dragging control membership
and order. OK keeps those choices pending until Apply. Clearing every attachment
is supported. The spacecraft **PowerSystem** dropdown offers available nuclear
and solar systems and **No power system** for detachment. Remove dependent mission
references before detaching hardware that those commands or reports use.


**Visual model…** opens the spacecraft's model preview beside rotation,
translation, scale and color controls. Browse loads a new model and resets its
pose; Recenter zeros the offsets and Autoscale restores scale one. Show Earth
adds a wireframe size reference. Drag/wheel and Fit adjust the preview view.
Invalid inputs keep the last valid preview while you correct them. Numeric
values follow GMAT's supported ranges. OK keeps changes pending in the
spacecraft panel; Apply updates the mission. The splitter and scrolling controls
keep the editor usable on smaller displays.

**Attitude…** opens a focused spacecraft attitude editor. Model selection updates
the available fields and retains each model's pending settings during the dialog.
Orientation and rate selectors convert pending values before displaying the new
representation. Invalid values remain available for correction. Reference frame
controls respect the model's restrictions; reference bodies and AEM input files
have dedicated selectors. OK keeps the selected model's settings pending until
spacecraft Apply, together with other spacecraft changes.


XY plot resources now provide **XY plot setup…** with Show plot, Show grid,
Solver iterations, a single X parameter and an ordered Y list. Select opens a
numeric parameter browser with object properties, reference frames and indexed
array elements. Remove, Up/Down and drag reordering edit the Y list. Strings and
bare arrays are excluded; turn off Show plot to keep a plot with no Y selections.
OK retains edits in the resource panel, and Apply validates and saves them to
the mission. Cancel leaves the pending resource values unchanged.


Ground-track resources provide **Ground-track setup…** with central-body and
spacecraft/station selection, sampling/update frequencies, maximum retained
points, recent-segment redraw, visibility, solver settings and texture browsing.
Body changes choose the corresponding default texture and retain custom choices
when switching back. Default restores the body's map. Missing or undecodable
images produce corrective feedback. The controls scroll in smaller windows,
keeping OK/Cancel accessible, and changes remain pending until resource Apply.

Recent-segment redraw affects running plots and replay while retaining their
history. Zero draws all retained points; completing a run restores the full
retained trajectory. Maximum retained points can be one. These behaviors apply
to both orbit and ground-track rendering.


OrbitView resources provide **Orbit-view setup…** with ordered space-point
selection and per-object Draw flags, three object-or-vector camera selectors,
view/up coordinate systems, up axis and scale, drawing and star options, solver
iteration mode and collection/update/retention/redraw settings. The groups scroll
in smaller windows while OK/Cancel remain visible. OK keeps edits pending until
resource Apply. Draw flags hide the body/model while retaining its trajectory,
matching wx; removing an object removes both.

**Reset to script camera on each run** controls `UseInitialView`. When unchecked,
Qt retains zoom, rotation, pan, Fit, projection/FOV and the selected named camera
within this mission across rebuilds/reruns and viewer close/reopen. When checked,
a rerun restores the scripted camera. New/Open starts a fresh camera session.
Drawing-only edits through the grouped dialog preserve imported OF camera
metadata. Projection/FOV remain available through the live viewer Camera dialog
and Keep projection action; obsolete readonly wx FOV fields are not exposed as
resource settings.

Chemical and electric thrusters provide **Thruster setup…** with direction and
performance groups. Local axes/origin controls follow the coordinate-system
selection; origin is inactive for MJ2000Eq and SpacecraftBody axes. Electric
model selection enables its applicable efficiency, Isp and constant-thrust
fields while retaining other pending values. Power limits are checked together.
The controls scroll on smaller displays; OK keeps changes pending until Apply.

**Tanks and mixtures…** selects tanks matching the thruster type, with ordered
positive mixture ratios. Removing all tanks clears both lists and survives
save/reopen; an active burn still requires a tank. **Coefficients…** edits chemical
coefficients or electric polynomial coefficients, and is enabled for electric
thrusters when ThrustMassPolynomial is selected. Spacecraft **PowerSystem**
selection attaches the power source required by electric propulsion.

Impulsive burns provide **Impulsive burn setup…** with the delta-V elements in
km/s, Local or named coordinate systems, dependent Local axes/origin, and optional
fuel depletion. The fuel group selects one tank and enables Isp/gravity when
Decrement mass is checked, retaining pending values when it is toggled. Zero
delta-V is valid. The single fuel tank must be attached to the maneuvering
spacecraft for mass depletion. OK keeps edits pending until resource Apply.

Finite burns use the **Thrusters** selection checklist with individual checks,
Select all, Clear selection and drag reordering. Clearing all selections is
saved as an empty configuration. The base engine rejects executing an active
finite burn without thrusters; select an attached thruster and Apply to recover.


EphemerisFile resources provide **Ephemeris output…** with output spacecraft/frame,
format, filename browsing, write enable, sampling/interpolation and time interval.
Code-500 exposes byte order; STK exposes distance units and event boundaries; SPK
restricts frames and uses integrator samples. Step size and interval endpoints
accept typed values. Changing epoch format converts both explicit dates together
and retains spacecraft-epoch sentinels. OK keeps edits pending until resource
Apply.

**Output > Ephemeris files** opens OEM/STK text in the paged viewer. Binary files
show their format, path and size with **Copy path** and **Open folder** actions.
A spacecraft's **EphemerisName** field provides an existing-file chooser for
OEM/STK/Code500 propagation, including first selection when blank. SPK propagation
uses the spacecraft's ordered orbit-kernel list and matching NAIF ID.


### Dynamic-data display setup

Open a DynamicDataDisplay resource and choose **Dynamic data setup…**. Set Rows
and Columns, click **Resize grid**, then double-click a cell or use **Edit cell…**
to choose a parameter, text/background colors and warning/critical bounds.
**Clear selected** or Delete restores blank cells. Warning and critical colors
apply to numeric cells whose text color is black; another text color overrides
condition colors. Each parameter can appear once; arrays require an element such
as `A(1,2)`. Both grid and live display columns can be resized by dragging headers,
and live updates preserve those widths.

OK keeps settings pending in the resource panel. Apply validates the full mission
and updates the script as one undoable edit. Cancel leaves earlier settings
intact. UpdateDynamicData command settings can select a display and individual
parameters; leaving its checklist empty updates every cell. Unselected cells keep
their preceding published values, matching the base engine. Closed displays retain
published data for reopening, including immediate reopen before Qt finishes
deleting the old MDI child.


### Barycenters and libration points

A Barycenter resource exposes **BodyNames**. Use its Select button to add or remove
celestial bodies, clear the selection or drag them into a different order. Apply
replaces the complete list; at least one distinct celestial body is required.
A LibrationPoint resource offers primary/secondary selectors for celestial bodies
and custom barycenters, plus an L1–L5 dropdown. The two bodies must differ;
SolarSystemBarycenter, spacecraft and other libration points cannot be selected.

Both resources offer orbit and target color pickers. Built-in barycenters explain
that membership is fixed and permit only their colors to be changed. All edits
remain pending until Apply, validate the complete mission, support Undo/Redo and
persist through script save/reopen. Configured calculated points can be used as
coordinate-system origins and OrbitView objects through the existing selectors.


### File-interface imports

A FileInterface resource provides its format dropdown and input-file chooser.
Apply requires an existing, readable file and rejects directories. For the bundled
TVHF_ASCII format, the Set command settings offer spacecraft and data-interface
selectors plus **Select fields…**. Choose Epoch, CartesianState and/or Cr, or
check **Import all fields**. Fields omitted from a subset leave the target's
current values unchanged. CartesianState imports all six components together;
the reader's internal component/frame/body metadata is not offered as separate
assignments.

Use **Set (file import)** in the mission command templates to insert an import.
The field picker reads metadata from an initialized clone of the interface;
browsing does not import data into the spacecraft. OK changes pending command
text, Cancel preserves it, and Apply validates the complete mission. Labels and
comments, Undo/Redo and script save/reopen retain the normal command-editor
behavior. A missing file or malformed input produces an error; repairing the
input allows another run without reopening the script.


### Tracking data, simulation and batch estimation

Resources now includes Measurement Models, Error Models, Interfaces, Calculated
Points, Celestial Bodies, Data Filters and Fields of View. Supported resource
types also appear in the creation dialog. Open a TrackingFileSet and choose
**Tracking configurations…** to edit its ordered signal paths and measurement
types. Add/remove rows and use Up/Down to reorder them. **Signal path…** selects
spacecraft/stations and their attached hardware; repeat a participant for a
return path and drag entries to reorder the path. **Types…** selects one or more
measurement types. Both table columns have adjustable widths.

OK keeps tracking changes pending until the resource's Apply; Cancel preserves
the earlier definitions. FileName and RampTable use ordered file pickers. FileName
can select a future simulation output, which is not written until execution;
the estimator reads the same selected file. RampTable selects existing inputs.

Simulator AddData and estimator Measurements provide tracking-set checklists.
Their Propagator selector edits the default propagator and retains explicit
spacecraft mappings configured in the script. Spacecraft SolveFors and
ground-station ErrorModels/AddHardware are editable lists; clearing a station
list omits its assignment so the saved script retains the empty default.
ErrorModel Type provides measurement choices, and noise/bias units follow the
selected type. Noise and bias values use those units.

Use the RunSimulator and RunEstimator mission templates and typed command
selectors. Apply validates the complete script, retains command labels/comments
and supports Undo/Redo and save/reopen. An unset optional antenna field of view
stays unset during script reconstruction; explicitly configured references are
retained. Antenna FOV pickers support selecting, replacing and clearing an
optional reference. A bounded, noise-free range-skin simulation/batch workflow
is covered; broader measurement types, filter regimes and multiple propagator
mappings remain on the replacement checklist.

For simulators and data filters, **Time interval…** edits the epoch format and
both endpoints together. Changing format in the dialog or the property table
converts both dates. Failed conversion preserves the earlier format and values;
Cancel preserves pending properties. Apply rejects an interval whose initial
epoch follows its final epoch. Gregorian dates follow the engine's millisecond
format; numeric epochs retain its full time precision.

TrackingFileSet and estimator DataFilters use typed filter checklists. AcceptFilter
and RejectFilter provide observed-object, tracker and measurement-type lists,
ordered file selection and editable RecordNumbers such as `1, 4-20` or `All`.
File selection includes **All files** and, for an accept filter, **From tracking**.
AcceptFilter ThinMode offers Frequency or Time; ThinningFrequency remains
validated by the engine. The bounded batch-estimation test covers frequency
thinning and record rejection; broader filter modes remain to qualify.

### Script-event editing

Open a BeginScript event in the Mission tree and choose **Script event…**.
Comments appear above a numbered, highlighted script body, separated by a
resizable splitter. The outer BeginScript/EndScript labels and inline comments
stay fixed; nested events and branches are edited in the body. Comment text is
stored as GMAT comment lines. Clearing the body creates an empty event.

OK updates pending command text; Cancel preserves it. Command Apply validates
the complete mission, and an invalid command leaves the earlier mission intact
so the text can be corrected. The command source remains available for editing
complete blocks directly. Pending edits support Undo/Redo, and saved/reopened
events retain their commands, labels and comments.

### Comparing output files

Use **File > Compare files…**, or **Compare…** in an Output report viewer to
prefill that report as the baseline. Compare two files or a baseline directory
against up to three candidate directories. Directory matching removes the
baseline prefix and prepends each candidate prefix; missing matches also try
the same name with `.truth`. Supported directory extensions are `.txt`,
`.report`, `.data`, `.script`, `.eph`, `.oem`, `.e` and `.truth`; logs and backup
files are excluded. **Files: All** compares every match, or enter a positive
limit. The directory table columns can be resized manually.

Text comparison includes blank lines unless **Skip blank lines** is selected.
Numeric lines use the engine's token comparison, ignore strings/comments and
skip blank lines. Numeric columns skip initial headers, accept engine-supported
numeric/UTC fields and report each column's maximum absolute difference.
Numeric tolerance must be finite and nonnegative. Trailing rows and unequal
column counts are differences; malformed rows after data starts and non-finite
numbers are errors. Text files must use UTF-8; BOM and CRLF are accepted.
Individual lines larger than 16 MiB are diagnosed explicitly.

**Compare** runs without blocking the interface. **Stop** retains the previous
complete result; **Close** cancels active work. Results are read-only, paged and
searchable through the complete file. **Save results…** exports the complete
comparison, including differences and errors, and prevents overwriting its input
files. Comparing reports does not edit the mission or change its calculations.

For an existing polyhedron gravity contributor, its force-model properties offer
a celestial-body selector, shape-file Browse control and density in kg/m^3.
Apply checks a finite positive density and a closed triangle mesh with outward
face winding. Shape coordinates are in kilometres. The file contains a vertex
count, rows of `label x y z`, a face count, then rows of `label vertex1 vertex2
vertex3`; connectivity uses one-based vertex-row indices. Record labels are
decorative integers. Relative paths resolve from the mission's script directory.
Malformed inputs are reported before gravity evaluation; the calculation for
valid meshes remains the existing plugin calculation.

### Kalman filtering and smoothing

With the EKF plugin enabled, Resources includes **Process Noise Models** and
**Estimated Parameters**. Process-noise properties expose the coordinate system,
update interval and acceleration-noise vector; Gauss–Markov properties expose
the solve-for coefficient, steady-state value/sigma and half-life in seconds.
Spacecraft SolveFors can select configured estimated parameters, and its
ProcessNoiseModel selector lists available noise models. Smoother Filter selects
a sequential estimator. RunEstimator and RunSmoother offer separate typed
resource selectors in the mission editor. Their reports open from Output.

InputWarmStartFile browses existing state/covariance CSV files; leave it empty
for a cold start. OutputWarmStartFile selects a future output; leave it empty to
disable that output. These fields retain the engine's filename restrictions,
including its rejection of non-ASCII filenames. Unicode mission filenames are
supported independently of those plugin output restrictions.

WarmStartEpoch accepts an explicit date or the **FirstMeasurement** and
**LastWarmStartRecord** boundaries. Changing WarmStartEpochFormat converts an
explicit date and leaves a boundary selection intact. FirstMeasurement requires
a seed record strictly before the first observation. LastWarmStartRecord needs
observations after the selected last record; a seed reaching the end of the data
cannot continue estimation. File selections and dates remain pending until
Apply. Failed conversions retain the earlier format and entered date.


### Inspecting configuration and run results

Resource and command panels offer **Show script…** for the applied configuration.
The preview is read-only; pending panel edits stay in their editor and are not
included. **Find…** searches within the preview and standard Copy works.

After running a mission, use **Run > Mission summary…**, the Mission tree's
summary context menu, or **Summary…** in a command panel. Command summaries show
states captured at that command, including script-event end states. Choose a
coordinate system or filter an entire mission to physics commands. Frames that
depend on spacecraft are excluded. **Save as…** exports UTF-8 text and protects
the mission script from being overwritten. Rebuilding/loading invalidates old
summaries; editing the script requires a new run before inspecting results.
Failed/stopped runs show partial results and identify commands without data.

### Solar-system ephemeris settings

Double-click **SolarSystem** in Resources to select a planetary ephemeris source,
its DE file, planetary SPK/PCK files, update interval in seconds, and whether to
use TT for ephemeris. The source choices come from this runtime. Zero requests
an update every time. Browse selects an existing file; typed filenames remain
pending until Apply. Invalid settings retain the pending edits for correction.

DE selection updates its filename and retains pending filenames when switching
sources within the panel. With SPICE selected, the DE field shows its fallback
and is disabled. Apply saves that fallback before selecting SPICE, so the saved
script and subsequent resource/mission edits can rebuild it. Close offers
Discard or Cancel if changes are pending; successful resource Apply refreshes the controls in the same window.
Undo restores the exact previous script.

The selected DE file must contain complete binary records; the engine validates
the ephemeris version and SPICE kernel types. Filename apostrophes, percent
signs and control characters are rejected to prevent script-parser ambiguity.
The leap-second kernel remains a startup setting. These controls configure the
existing engine calculations.


### Celestial-body settings

Double-click a body in Resources to open **Properties**, **Orbit**,
**Orientation** and **Visualization**, following the wx layout. Properties
include gravitational parameter, equatorial radius, flattening and planetary
PCK files. Orbit exposes the runtime ephemeris choices, NAIF ID and orbit SPK
files. Built-in ephemerides use SolarSystem; central body and rotation source
are read-only, matching wx. User-defined bodies expose their pole and rotation
constants; Earth exposes the nutation update interval. Orientation also has
the SPICE frame ID and FK files.

Kernel lists offer Add, Replace, Remove and Up/Down. Apply writes the complete
selected membership and order, including removal of startup defaults. Kernel
types are validated by the engine. A body whose ID or SPK does not supply its
requested ephemeris can fail at execution; correct those fields and run again.
New resource includes the available celestial-body types, including Asteroid.

Visualization offers texture browsing with a pending preview, model browsing,
model offset/rotation/scale, and orbit/target colors. Clear the model filename
to restore the spherical body; clear the texture filename to use the engine's
default texture. Textures must decode, and models must be readable by the
viewer. Appearance changes leave propagation calculations unchanged.

Values remain pending until Apply; invalid edits retain them for correction.
Show script displays applied settings. Successful resource Apply refreshes the controls in the same window;
Close offers Discard or Cancel for pending edits. Undo restores the prior
script exactly. The four pages scroll and long rows wrap to fit desktop
scaling. These controls configure the existing engine.


The body selectors used for solar-power shadows and force-model gravity show
available celestial bodies, including configured user bodies. Check or uncheck
bodies, use Select all or Clear selection, and drag rows to reorder. Cancel
retains the original pending list. The Sun is excluded from shadow bodies.
Primary and point-mass choices exclude bodies in the other pending gravity list;
typed overlapping lists are rejected on Apply. Clearing shadows explicitly
removes the implicit Earth shadow, including after save/reopen and execution.
Body-list-only edits retain the surrounding script and implicit defaults.


Event reports opened through Output are checked against the current mission's
running locator. An old file is not shown as current output after a rebuild,
unbuilt script edit, disabled locator, WriteReport-off run or Manual run without
FindEvents. The read-only explanation identifies WriteReport and FindEvents;
a generated report opens with the normal paging/search/comparison controls.
Report and binary-ephemeris windows activate when opened.


## Desktop and viewer continuation (2026-09-30)

Unspecified plot positions/sizes now arrange the initial viewers separately, so
both default Orbit View and Ground Track are visible. Scripted geometry is
retained, and manually moved/resized views leave automatic placement. Output
activation continues to restore and raise a selected minimized viewer.

Ground Track distinguishes a propagation-block refresh from mission completion.
Its recent-segment setting remains active while the mission is running or paused;
actual completion shows all retained history at Latest.

Ground Track now honors solver None filtering and Current iteration removal;
All retains trial samples with target colors. SolverPlotTests compares real GUI
mode edits and display histories against independent accepted data and unchanged
numeric solver/geodetic reports, including native rendering and replay.

Help / About GMAT shows current engine/build/bitness and Qt/OSG versions, credits
and View license. The license comes from the configured root License.txt and is
read-only/searchable. File errors stay visible and retry rereads the file.

Resource Apply keeps its MDI window open and refreshes accepted controls from
the rebuilt model. Clean resource companions refresh their page/filter/columns;
unapplied companion edits are preserved and stale Apply remains protected.
Command Apply also refreshes its accepted controls in the same window. An accepted
insertion becomes an editor for the first inserted command, so the next Apply
changes that command rather than inserting a duplicate. Failed Apply retains the
pending text. Broader companion refresh/keyboard/portal cases remain under qualification.

Force-model body edits through the polyhedron fields or contributor dialog retain
the moved contributor's source settings and implicit density. Reopening pending
rows retains their original identity; newly added rows remain new. Changing an
existing Python external-force module updates its declaration in place. Both
paths preserve unrelated force settings and comments through mixed Apply,
Undo/Redo and save/reopen. Wider plugin and desktop qualification remains open.

SPICE diagnostic initialization retains previous GMATSpiceKernelError.txt files
and selects unused numbered siblings for new raw output. An unavailable output
directory or diagnostic path beyond SPICE's 255-byte limit uses console output
with a visible warning; underlying kernel errors still reach GMAT. Existing,
repeated, fresh, read-only and long-path cases plus corrected kernel load/unload
are covered by SpiceDiagnosticTests. Wider storage/race cases remain open.

See Qt6ReplacementQualification.md for current evidence and incomplete gates.


### Offline context Help

Help / GMAT Help, Using GMAT and Tutorials open the local user guide. Resource
and command editors have Help buttons and F1; subordinate modal editors inherit
the resource topic, with ballistics, attitude and visual-model topics selected
explicitly. Command topics follow the current pending command text. Help is
read-only, with Find, Back/Forward, Topic, Contents and Close/Escape. Open in
browser uses the desktop application for a requested page.

The default location is the startup HELP_PATH/html directory. QSettings entries
Help/<engine type> can override individual local or web topics. Missing or
malformed UTF-8 pages report the exact path; restore the file and select Retry.
Help does not apply pending configuration or edit the mission.

Build the offline R2026a documentation from this repository with:

```bash
cmake --build build/linux-gui --target build-qt-help
```

This explicit target needs Python, make and Java, and uses the bundled DocBook
tools already used by doc/help/Makefile. It copies help.html, html and files into
application/docs/help, which the existing install rules include. The generated
files are ignored by Git. GUI compilation itself does not require the help build;
QtGui.Help requires the offline pages to be built before qualification.


### Run scripts from a folder

Mission / Run scripts from folder provides a sorted range, two filename filters,
repeats, output and optional per-repeat directories, exact saved-script copies and
ReportFile numerical comparison with an optional export. Functions and backup
files are excluded. The range is selected before filters. Saved copies retain the
original asset/include base; copy associated assets separately for a portable
mission. OpenFrames scripts offer in-memory Qt conversion without rewriting the
source or copy.

Results classify each build/run failure and aggregate all selected repeats.
Stop interrupts an active mission or prevents further runs. Close/Escape during
execution requests Stop. The Results summary is written in the output directory;
comparison export cannot overwrite a mission, copy, baseline, report or summary.
Final batch viewers and solver progress remain in Plots, with Tile/Cascade controls.

Pending configuration panels must be applied or discarded before the batch.
Clean configuration windows close because their engine objects are replaced.
The open script and Undo/Redo are retained; accepted engine configuration, paths,
log and batch state are restored afterward. Ordinary viewer history is retained.
The application reports a restoration failure and requires rebuilding the open
mission if restoration fails.


### Mission tree views

The Mission toolbar provides All, command Filter, expansion levels and Undock.
Filter offers a sorted checklist with Include/Exclude, Check all/Uncheck all and
Apply. Pending checklist changes can be closed without changing the view.
Branches containing matches retain context and boundaries. Equation and ScriptEvent
are the assignment and script-event aliases. Filters affect the view and retain the
complete mission source and calculations.

Undock opens a mission-only floating window with the same tree, selection and
editing controls. Resources/Output stay in the main navigation. Dock or Close
restores the Mission tab. Detached placement is saved; filter and expansion state
remain for the current session and survive tree rebuilds.


### Welcome and recent missions

Help / Welcome provides New mission, recent missions, sample Browse and offline
Using GMAT, Reference guide and Tutorials. Project, issue, wiki and video links
use the configured runtime GMAT.ini entries. Normal interactive startup opens
Welcome unless Show Welcome on startup is cleared; the manual menu remains
available. Requested script/run/screenshot modes open directly to their work.

File / Recent missions shares the ten-entry history updated by successful opens
and Save/Save As. Equivalent paths are deduplicated; Clear recent list preserves
files. Recent/sample/New actions honor the current pending-panel and unsaved-script
Save/Discard/Cancel flow. Missing files report their full path, and canceled
operations retain the current source and identity. Main Welcome content scrolls
in compact windows while Close and the startup preference remain visible.


### Bulk parameter selection

The ordered report, XY Y-parameter and function-argument selectors provide
Add all configured and Remove all. Add all adds each compatible configured name
once; manual function input/output arguments may repeat a reference in different
positions. Report and plot additions avoid duplicates. Single-value selectors
retain their caller restrictions and hide these bulk actions.

Select multiple object properties opens a multi-selection object list and an
adjustable property/reference table. It shows the properties common to the
selected objects, with coordinate-system, central-body, force-model or directly
attached hardware references shared by those objects. Check all selects available
references; Uncheck all clears them. A manually checked property without a common
reference explains the conflict and keeps OK disabled. Select fewer objects to
add different attached hardware separately. Accepted additions use object/property
order; Cancel leaves the parent selection unchanged. The parent command/resource
still requires Apply before changing the mission.


The XY viewer's **Style…** dialog has Plot, X axis, Y axis and Curves tabs.
Edit plot/axis labels, choose independent fixed minimum and maximum values,
and set major tick intervals and label precision. Unchecked limits follow the
data; invalid values keep OK disabled until corrected. Cancel retains the
current view. Accepted display options survive viewer close/reopen and reset
when the mission rebuilds. Changing limits restores the plot's pan/zoom so the
new range is visible. Use the resource editor for settings saved in the script.


The XY viewer's **Export data…** action saves its title, axis labels and retained
curves as UTF-8 text with ordered X/Y pairs at full stored precision. Hidden curves
and points outside the displayed range are included. Export does not recover
history already removed by MaxPlotPoints. Choose a separate file from open
missions, startup files and mission outputs. Failed writes preserve the previous
file, and Cancel leaves the mission and plot unchanged.


Plot Save image writes PNG atomically and refuses current open missions, startup
files and configured mission outputs, including symlink aliases. A failed write
preserves the previous destination and reports the problem. XY Export data uses
the same protection; these display/export operations do not edit the mission.


For a new GmatFunction, choose its existing function file with Browse or use
New file to edit and save a template first. Create then adds the function and its
file path to the mission as one undoable edit. Missing files keep the creator
open for correction. Canceling the file chooser/editor writes nothing; files
you explicitly save remain after creator Cancel or mission Undo. An imported
function may have a resource name different from its file name.


New function templates use the chosen `.gmf` file's basename in their declaration,
even when the mission resource has a different name. Choose a filename beginning
with a letter and containing letters, digits or underscores; invalid new template
names produce feedback before any file is written. Directory names may contain
spaces or Unicode. Existing-file imports retain their original contents.


### Apply solver corrections

Target and Optimize command panels have an Apply Corrections button. Run the
current mission first, then use it to update numeric Vary initial guesses from
that block's last solver result. The result need not have converged. Scale
factors are converted back to the command's initial-value units; guesses that
refer to variables, arrays or object properties remain source references and
are listed in the Message Window.

Apply or discard pending panel edits first. Pending/stale source, an unexecuted
block or a rebuild without a subsequent run cannot use old results. Corrections
preserve unrelated source and can be undone/redone as one change. Successful
changes rebuild the mission and refresh clean retained command panels, including
open Vary controls. Nested solvers keep their own corrections; Vary commands
inside script events are also mapped. A new run is required before applying
another block's correction after the rebuild. Save the changed mission to keep
its updated guesses.

QtGui.SolverCorrections covers actual controls, scaled/reference/nested and
script-event cases, unexecuted/pending/stale guards, interruption/nonconvergence
recovery, known variable goals, exact source/Undo/Redo and Unicode reopen. The
native Wayland panel capture and its precise coverage limits are recorded in
Qt6ReplacementQualification.md. Broader solver mode and desktop acceptance
requirements remain under qualification.


### Intentional Stop and retained partial viewer history

The Stop command, a Target/Optimize ExitMode of Stop, and the Stop toolbar action
all report Mission stopped. Runtime failures continue to report Mission failed.
Partial command/mission summaries remain available for the stopped or failed
run; unexecuted commands do not acquire new result states.

When a mission stops or fails, Orbit and Ground Latest show the complete retained
history even if a preceding Toggle Off suppressed the subscriber's usual end
callback. Replay still uses the retained samples and preserves the configured
recent-segment behavior before Latest. This marks the end of incoming display
data; it does not make a partial run successful or invent disabled samples.
The same finalization applies to started folder-run items.

SaveAndContinue retains a solver solution as the initial guess when that same
block executes again in control flow. DiscardAndContinue uses the original Vary
guess for each invocation. RunInitialGuess propagates the configured guess
without solving. SolverModes covers all six combinations for both selected DC
and Yukon in a repeated spacecraft propagation fixture; the disabled Stop and
write-failure variants also have native Wayland viewer/recovery evidence. Exact
coverage and remaining qualification limits are recorded in
Qt6ReplacementQualification.md.


### Plot toggles inside optimization and nested targeting

The same subscriber checklist and On/Off controls apply inside Yukon optimization
and nested Target sequences. Disabled intervals suppress plot samples; resuming
begins a separate arc. Numerical propagation, solver values and ReportFile
output retain their script-configured behavior.

OptimizerToggle and NestedSolverToggle exercise the actual controls with Current
Orbit/Ground/XY plots, exact source/Undo/Redo/Unicode reopen, known solver goals
and complete independent state/geodetic reports. Ground/XY accepted histories
also match an independently filtered None-mode run. The nested case solves an
entire inner Target while those plots are disabled. Both cases have native
Wayland captures, camera/replay and immediate close/reopen evidence in
Qt6ReplacementQualification.md. Other mixed/deeper combinations remain under qualification; bounded solver
ephemeris evidence is recorded in the later qualification appendix.


NonlinearConstraint now retains typed controls for array-element operands,
provides <=/>=/= relation choices and a numeric operand browser, and rejects
out-of-range literal array indices before Apply. Five fixed-bound Yukon cases
have analytic optima and independent complete iteration reports after actual
MDI edits/Undo/Redo/Unicode Save/Save As/reopen, plus native Wayland inspection;
see the NonlinearConstraint qualification appendix. A separate varying-right
bound produces NaN in the script reference before any GUI edit and remains
unqualified; no numerical-engine rewrite was made. Other original qualification
gates remain open and Windows/macOS remain deferred.


Vary, Achieve and Minimize forms retain array-element operands and option values
without treating index commas as field separators. Achieve now includes a
Tolerance parameter browser. Goal/objective browsers offer numeric references;
value/tolerance browsers also accept finite real literals. Literal array indices
are checked before Apply, and edited Vary literal bounds/initial values get
inline range validation while reference expressions and untouched settings stay
intact. Actual DC/Yukon array workflows preserve source/Undo/Redo/Unicode file
round trips and match complete independently configured iteration reports; see
the solver operand qualification appendix. The actual application was rebuilt.


Clean retained command panels now skip Apply when their source has not changed,
so another Apply does not rebuild/invalidate the run or add an extra Undo step.
Global insertion/editing and shared Variable/Array/String/Spacecraft execution
across repeated function calls now have independent known reports and exact
source/Undo/Redo/Unicode file round trips, including deferred-reference runtime
failure and correction/rerun. See the Global scope qualification appendix for
native widget evidence and the original desktop acceptance limits.


New command, resource and report windows now open within the workspace instead
of cascading partly offscreen and hiding their controls. Existing window
positions stay intact. Mixed EphemerisFile/Orbit/Ground/XY Toggle workflows now
have OEM/STK two-arc output, initial activation, terminal Off, source/file round
trips, correction/rerun and native rendering/replay evidence. STK event-boundary
metadata now records the resumed arc's actual start after a Toggle gap; captured
state rows and reports are unchanged. See the ephemeris/window-placement
qualification appendix for the affected checks and remaining desktop limits.


Ground Track now names empty spacecraft curves during initialization, so a
solver used as the first mission branch can anchor Current history before its
first propagation. Target/OEM and Yukon/STK Toggle cases now retain only the
accepted Ground/XY arcs; their complete trial reports and accepted ephemeris
state rows remain byte identical across the fix. Native replay/close-reopen,
Current/None accepted-history agreement and exact source/file recovery are
recorded in the solver-contained ephemeris qualification appendix. The actual
application was rebuilt; broader solver/binary cases and desktop gates remain.


Binary ephemeris Toggle workflows now have bounded SPK two-arc and Code-500
continuous-output activation/terminal-Off evidence in both byte orders, with
complete decoded states, exact source/file recovery, binary Output details/Copy
path and native viewer/plugin readback. Internal Code-500 Toggle gaps and SPK
reader gap traversal exposed legacy engine limitations and remain unqualified;
the continuous Code-500 case keeps Export enabled while only viewers pause.
See the binary Toggle qualification appendix for raw failures and scope limits.
No numerical engine or product behavior was changed in that increment.


Unavailable SPK reader states now leave gaps in retained Orbit, Ground and XY
trajectories. Valid camera history remains usable, and valid samples after the
gap recover without a joining line. Raw numerical reports and the engine's
completion status remain unchanged; this display repair does not qualify
scientific propagation through an ephemeris hole. The actual application was
rebuilt. See the unavailable SPK states qualification appendix for the reproduced
failure, three affected checks, native rendering/recovery and unchanged reports.


### Named propagation-segment cameras

OpenFrames ViewFrame values such as `Sat.FirstArc` select the plotted spacecraft
and a named Propagate command. Both ViewTrajectory On and Off use the moving
segment frame, matching OpenFrames. The camera follows captured position and,
with InertialFrame Off, captured attitude. InertialFrame On retains plot-frame
axes. Once another propagation arc begins, the first retained matching arc's
endpoint remains the camera reference. A later segment's camera waits until its
samples arrive. Replay uses retained poses rather than the live spacecraft.

Stored Current/Default eye, center, up and FOV are preserved; ordinary LookAtFrame
targets retain alignment. Automatic segment cameras use OpenFrames' empty-frame
one-unit radius fallback, independently of the displayed spacecraft model or
whole trajectory. Fit remains available for viewing the actual scene. Segment
LookAtFrame targets are rejected because OpenFrames' target lookup does not
resolve dotted segment names.

Build validates that the named command propagates the plotted spacecraft;
formation members are recognized. Missing commands, wrong spacecraft and
unplotted references leave the source intact and can be corrected. Explicit
primary camera edits clear that camera's imported segment tracking; additional
named views retain their settings, and Undo restores the original metadata.
The bounded two-arc qualification covers nonzero attitude, source conversion,
Unicode Save/reopen, replay and failure recovery. Repeated same-name arcs in
loops, solver/backward segment regimes, history eviction and segment label
options remain unqualified.

Newly opened or reopened plot/table windows are sized and placed within the
workspace so cascade positions do not hide their controls. Activating an
existing viewer preserves its position. The normal desktop viewport is covered;
viewports smaller than widget minimum sizes remain unqualified.


### Converted per-object labels and trajectories

OpenFrames DrawLabel and DrawTrajectory arrays now retain each plotted object's
choice independently in Qt metadata, alongside camera settings. DrawObject still
controls the model/body through standard OrbitView settings. A hidden model can
retain a label or trajectory; a shown model can have neither. Empty or short OF
arrays leave omitted objects at their true defaults, and a later Add assignment
resets those defaults in source order, matching OF. Malformed arrays retain the
original script with an explanation. This also works without an explicit View
selection; that case retains the existing default-camera warning.

Qt stores these choices by object name, so Orbit setup reordering retains them.
Removing an object prunes its imported display metadata in the same undoable
Apply; Undo restores both source and choices. Deleting the converted viewer also
removes its own imported directive, and Undo restores the exact original source.
The viewer's Object labels option
is a live master switch. Imported label-only objects participate in Fit/legend;
ordinary OrbitView objects retain their existing hidden-object label behavior.
Native and fallback renderers use the latest recorded label position and suppress
labels behind the camera rather than reusing an older visible pose.

The OrbitView resource editor now provides Object drawing… with separate
Trajectory and Label choices for each selected object. Default inherits the
viewer callback (trajectory) or shown-object behavior (label); On and Off store
an explicit choice. Ordinary viewers keep implicit defaults until edited.
Body/model visibility remains in Orbit-view setup, and Object labels remains the
live master switch. OK keeps choices pending until Apply; Cancel discards the
dialog changes. Apply retains the panel and camera metadata. Reopening restores
choices, and removing an object prunes its flags in the same undoable Apply.
Columns fit the normal initial dialog and remain user adjustable.

The same Object drawing dialog now includes a Markers tab with independent
Center and Endpoints choices and a pixel size for each object. Blank size means
10 pixels; marker Default inherits ordinary Qt behavior (Off). Imported OF true
center/end defaults and 10-pixel sizes are retained explicitly, including short
arrays and Add resets. Unsigned size arrays reject out-of-range object indices.
Markers can render with models, paths and labels hidden. A ring marks the latest
position and four-petal glyphs mark retained arc endpoints. Replay uses only the
recorded points through its selected frame; disconnected or different-provider
arcs retain separate endpoints. Live curve visibility hides all its glyphs.
Native markers obey scene depth; fallback sphere occlusion and behind-camera
checks prevent stale or hidden center glyphs. Zero size disables Qt glyphs;
OF zero/very large/device-limit behavior remains unqualified.

The bounded qualification covers automatic Build conversion, Orbit setup
Cancel/pending/Apply, reorder/removal/Undo, Unicode Save/reopen, invalid metadata
correction, retained hidden-object histories and complete independent numerical
reports. Other OF-specific object axes/planes/grids, endpoint markers, velocity
vectors, font/model overrides, VR and time synchronization remain under the
broader OF capability gate.


### Persistent per-object trajectory widths

Object drawing / Paths and labels now has a Width (px) field for every object.
It accepts finite fractional widths at least 1; leave it empty to inherit the
ordinary Qt 1-pixel width. OF DrawLineWidth arrays retain their supplied prefix,
omitted 2-pixel defaults and source-ordered Add resets. Reordering retains named
widths; removal prunes them, and Undo restores exact source and choices. Width
edits remain pending until Apply and preserve other drawing/camera settings.

Imported widths above the native GL stroke limit use depth-tested screen-space
geometry, so a wide trajectory does not become thinner at the desktop's pixel
scale. Ordinary/default strokes keep the established rendering path. Near-plane
clipping retains the visible part of a segment crossing the camera; entirely
behind-camera paths remain hidden. Bounded native/fallback width, depth and
clipping checks passed without repeating the calculation reference. Very large
widths, extreme joins, zero-length projected paths and other GPU limits remain
unqualified.


### Per-object label font and placement

Object drawing / **Label style** offers a pixel font size and Top-Right,
Top-Left, Bottom-Right or Bottom-Left placement for each selected object.
An empty size and Default placement retain the ordinary viewer font/placement.
Size 0 suppresses that object's label. The viewer's Object labels master switch
and each object's Label choice still control visibility. Values remain pending
until Apply; Cancel, Undo and file reopening retain their normal behavior.

OF conversion preserves DrawFontSize and DrawFontPosition, including the stored
14-pixel/Top-Right defaults, short arrays, Add resets and scalar position ordering.
The inspected OF editor stores these fields but its rendering path does not
apply them; Qt renders the requested values. This is not a pixel-for-pixel OF
font comparison. Both Qt rendering paths share placement and logical-pixel font
sizing; the desktop scale is applied once. Supported metadata sizes are integers
from 0 to 10000; larger sizes or other placements retain the original source with
an explanation. The bounded rendered evidence covers 12/24/32 pixels and all
four placements. Very large sizes, font substitution and edge clipping remain
unqualified. See the font qualification appendix for controls, native captures
and byte-identical complete state reports.


### Per-object body axes

OrbitView / Advanced / Object drawing / Body guides provides Body axes for each
plotted object. On shows the object's X/Y/Z axes at its recorded position and
attitude, including during replay. Off hides them; Default leaves them hidden.
These choices are independent of the model/body and trajectory controls and stay
pending until the parent resource Apply. Show labels controls the axis captions.
Converted OpenFrames DrawAxes arrays retain the corresponding named choices.


The same Body guides page provides **Lat/lon grid** and **Local XY plane** for
each object. On draws the guide at its recorded position and attitude; Default
and Off hide it. The grid follows the object's sphere, while the radial plane
extends to fifteen times its radius. These controls are independent of global
Grid/XYPlane, object visibility and paths. Changes remain pending until Apply
and survive replay, Save/reopen, Rename and Undo/Redo. Converted OpenFrames
DrawGrid/DrawXYPlane arrays retain the corresponding named choices.


### Cloning configured resources

Select a resource in Resources and use **Clone resource…** from its context menu
or Edit menu (`Ctrl+Shift+C` while the tree has focus). Its current configuration
opens in one form with an optional name. Leave the name empty to use the suggested
available name, adjust settings, then choose Clone. Cancel leaves the mission
unchanged; invalid settings stay open for correction. Clone is one undoable edit
and opens the new resource afterward.

The copy retains explicit settings, array formulas, references and Qt viewer
camera/drawing settings. References continue pointing to the existing objects.
Built-in coordinate systems/points, celestial bodies, SolarSystem and PropSetup
are protected as in wx. Build unsaved script changes and apply/discard pending
panels before cloning. Resource Rename is described below.

### Renaming configured resources

Select a resource and use **Rename resource…** in Resources/Edit or press F2
while the tree has focus. Enter its new name and choose Rename. References in
the open mission update together; Cancel leaves the mission unchanged, and
invalid names/settings stay open for correction. The edit supports Undo/Redo.
Build script changes and apply/discard pending panels first.

Built-in coordinates/points, celestial bodies and SolarSystem are protected.
Propagators can be renamed; their associated automatically named force model
follows the wx naming rule. Filenames, literal strings/comments and camera
labels retain their text. Main-file resources can be renamed with unrelated
literal includes or GMAT functions after read-only dependency checks. Included
resources, affected or unresolved external dependencies and unsupported tracking
signal-path syntax require coordinated script-editor edits; the GUI explains
that boundary before changing the mission. See
[the safe local Rename check](Qt6ParityValidation/safe-local-rename-20261003.md).


### Undo after a GUI edit

Undo and Redo use the focused text field's history. With a tree, plot or button
focused, they restore the selected script's committed edits, including resource
and mission Apply. An independently selected inactive script keeps its own
history. Fields in owned dialogs retain local history; a dialog's non-text
controls cannot undo a script behind it. Use the shortcuts displayed in Edit
(Ctrl+Z / Ctrl+Y on the qualified Linux configuration).


### Velocity display for each object

OrbitView / Advanced / Object drawing / Paths and labels provides Velocity
Default/On/Off for each selected space point. On draws a line at every recorded
sample, ending at position plus 1000 seconds times its velocity in the display
coordinate system. Default and Off hide these lines. This fixed scale is a
visual guide, not a propagation prediction. Paths and object visibility are
independent controls. Changes remain pending until the parent Apply and survive
Save/reopen, Rename, Undo/Redo and recorded replay. Converted OpenFrames
DrawVelocity arrays retain their independent named choices.

### Naming mission commands and adding Write

Command settings include an optional Name. Leave it blank for the normal command
label. Edits remain pending until Apply; the Name and source fields stay in sync.
A name-only edit preserves the rest of the command and nested branch text.
Structural command boundaries keep their original labels. SaveMission's quoted
filename stays in its source editor.

When the selected runtime supports Write, the mission insertion menu includes
it. Its simple form selects an ordered list of parameters; full option syntax
remains available in the source field. Names and Write are covered by
Qt6ParityValidation/mission-label-write-20261002.md.


### Keeping resource settings reachable

New resource settings panels open within the workspace. Activate a retained
panel through Window or by double-clicking its resource to bring all its controls
back into view. Pending edits remain in that same panel. Other positioned
windows keep their placement.


### Separate spacecraft for file thrust

BeginFileThrust and EndFileThrust each select one spacecraft. Use separate
commands and distinct thrust histories/segments for independent spacecraft;
the form rejects several typed choices before Apply and explains the separate
command workflow. Each spacecraft needs its own Propagate mapping under thrust.
ElapsedSecs conditions give an additional duration for each command, so matching
reports at 10, 15, 20, 25 and 30 seconds use durations 10, 5, 5, 5 and 5.

[The bounded two-history record](Qt6ParityValidation/multi-thrust-live-20261002.md)
qualifies sequential independent propagation and boundary fuel reports. Its
synchronized boundary attempt failed and remains unqualified. The later
[viewer availability repair](Qt6ParityValidation/viewer-data-availability-20261002.md)
retains the last real pose when a spacecraft is absent from a publication. One
[unchanged-source affected GUI retry](Qt6ParityValidation/multi-thrust-viewer-retry-20261002.md)
shows both spacecraft at their orbital endpoints and produces the same complete
numerical report. Legitimate zero coordinates remain valid. Original attempts
and source limitations are retained.


### Resource creation Help, report search and custom FOVs

New resource and Clone resource have visible Help buttons. F1 opens the resource
topic from the name field and nested editors; changing the selected type changes
the Help topic. Help retains pending edits. Celestial-body types use CelestialBody
and FOV types use FieldOfView. Missing plugin pages retain the existing diagnostic
and Contents navigation.

Find/Next/Previous and Edit-menu search use the active report, including reports
in comparison results. Replace on a read-only report opens Find; Go to line stays
with the selected script. Stored OpenFrames camera aliases translate with the
same last-assignment precedence as their Default spellings.

CustomFOV Setup edits paired cone/clock or azimuth/elevation vertices, with
Add/Remove, interpolation and an alternative FOV-file Browse mode. At least three
finite paired vertices are required. Apply/Create commits one representation;
Cancel retains the mission, and unrelated appearance edits preserve authored
geometry. Save/reopen retains authored vertices rather than expanded interpolation
or file-derived arrays. [Focused and private actual-input evidence](Qt6ParityValidation/remaining-gui-controls-20261002.md)
records current coverage and the unchanged engine's exact-boresight limitation.
This evidence does not close host/GPU or broader scientific qualification.


Configured `FOV_DATA_PATH` aliases now survive pending creation Help/Cancel,
file-based creation/Apply, Build and Save/reopen. Bare relative mask names search
the mission directory and then the configured FOV fallback using existing engine
lookup rules. Accepted first-use creation retains the ordinary engine default
when no FOV path was configured. [Focused path evidence](Qt6ParityValidation/custom-fov-paths-20261002.md)
records this bounded lifecycle and its remaining limits.


### Resetting the selected plot camera

**Reset view** restores the selected camera's Default pose. Press **Space** while
the plot canvas has focus for the same operation. For converted OpenFrames views,
Default is retained separately from Current; **Script view** restores the initial
Current pose. Reset clears your mouse rotation, zoom and pan while retaining the
selected projection. Converted views without a stored Default use their legacy
automatic framing. Cameras without separate reset metadata restore their scripted
pose; ordinary unscripted plots reset their viewing angles and fit.
Closing and reopening the retained Output view keeps its reset state.
[Focused and private-input evidence](Qt6ParityValidation/camera-reset-20261002.md)
records the supported cases and host/GPU limits.

The [relative-file FOV workflow](Qt6ParityValidation/custom-fov-file-native-20261002.md)
also has actual creation, Imager selection, Save/reopen and report evidence.
The rebuilt event tutorial directs SPICE leap-second kernel selection to startup
`LSK_FILE`; `LEAP_SECS_FILE` separately selects the time table. SolarSystem's panel
has no leap-second kernel input. This wording correction retains the completed
tutorial ledger without claiming an additional settings-panel operation.


GUI-created coordinate systems can participate in local Rename even when their
owned axis settings are authored as `Frame.XAxis`, `Frame.Primary` and
`Frame.Secondary`. Axis direction literals are retained; configured object
references are renamed. New command/resource windows preserve their requested
normal size during initial maximized-sibling restoration and remain bounded to
the workspace. Later manual sizes remain intact.
[Focused checks and separate native evidence](Qt6ParityValidation/axis-command-usability-20261003.md)
record these additional walkthrough-discovered usability repairs.


### Additional offline Help walkthroughs

Tutorials now includes eighteen chapters: the original twelve plus Creating a Report, Force Models, Formation Rendezvous, LEO Station Keeping (both parts), ACE Station Keeping and Mars B-Plane Targeting (Legacy Walkthrough). The older Mars title distinguishes it from the modern chapter. All new chapter/subsection local links, illustration paths and script attachments have installed verification. The Propagate Help page describes Single step, accepting an empty stopping-condition list, multiple groups and scrolling fields.

[Publication evidence](Qt6ParityValidation/help-publication-20261003.md) and the [walkthrough ledger](Qt6TutorialWalkthrough.md) retain each bounded GUI pass and its scientific limitations. Lunar remains unpublished after two capture-target failures; L2 is held, and MATLAB/legacy optimizer chapters retain their dependency dispositions. This does not close the nine replacement acceptance gates or host/GPU qualification.

Local Help script/function attachments display as read-only literal text with line breaks and indentation. Back/Forward keeps their file URLs; Enter in Find searches the current page without activating another dialog button. [Focused and actual-input evidence](Qt6ParityValidation/help-attachments-20261003.md) records the repair and rebuilt runtime. Viewing an attachment does not import or run it.


### Propagation segment labels — 2026-10-04

OrbitView / Advanced / Object drawing / Label style now includes **Segment
labels** (Default/On/Off) for space points. Default hides them. This converts
legacy `DrawUsePropLabel`, independently of object-name Label, model and path
visibility. Show labels remains the master; font size and placement apply.
Each accepted regular spacecraft segment follows its own retained trajectory
at the replay epoch, clamping outside its range; bodies use the merged first
provider label. Solver trials do not create regular segment labels. Compact
endpoints survive trimming, while intermediate placement approximates the
available collected geometry. [Focused evidence and runtime limits](Qt6ParityValidation/segment-labels-20261004.md)
retain the unchanged numerical report and explicit host/GPU qualification stop.


### Transmitter solve-for selection — 2026-10-04

Transmitter creation and editing expose **SolveFors** with a FrequencyBias
selector. Clear the selection for an empty solve-for list. OK in the selector
keeps changes pending until parent Create/Apply; Cancel/discard preserves the
configured object and source. Unsupported names and duplicates stay pending for
correction. [Focused verification](Qt6ParityValidation/transmitter-solvefors-20261004.md)
records source recovery and the rebuilt runtime without repeating an estimation
mission or claiming host/GPU qualification.


### Receiver error-model selection — 2026-10-04

Receiver creation/editing now exposes **ErrorModels** with ordered selection of
existing ErrorModel resources. Clear selection removes the explicit list and
restores the empty default on Apply. The form rejects missing/wrong-type names,
duplicate names and multiple models for the same measurement type. Selection
remains pending until parent Create/Apply; Cancel/discard preserves configured
settings. [Focused evidence](Qt6ParityValidation/receiver-error-models-20261004.md)
records exact source/file recovery, Rename/Undo and the rebuilt runtime without
claiming new GPS measurement or host/GPU qualification.


### Coordinate-system labels — 2026-10-04

OrbitView / Advanced / Object drawing / **Frame labels** provides independent
**Axes labels** and **Reference frame** choices. Default shows X/Y/Z when Axes
is On, while the origin's coordinate-system name is hidden. Set Reference frame
On to show it even with axes hidden. These controls are independent of Show
labels for objects. Choices stay pending until Apply and survive Save/reopen;
Default removes the explicit override. The same settings convert from OF
AxesLabels/FrameLabel and appear in both renderers. [Focused evidence](Qt6ParityValidation/coordinate-labels-20261004.md)
retains source/report invariance and the explicit host/GPU qualification limit.


### LastN solver trajectory display — 2026-10-04

OrbitView's primary form and setup dialog offer **LastN** under Solver iterations,
with an enabled **LastN count (0 = all)** field. The default count is 1. Like the
legacy viewer, the limit is twice N finalized trajectories per plotted object;
0 retains all trials subject to Maximum retained points. It is not a count of
complete solver iterations. Accepted trajectories remain, and in-progress Qt
samples stay visible. Choose All, Current or None to remove the LastN override.
Count edits remain pending until Apply and preserve imported camera settings.

OF conversion preserves the source-order count rules. The numerical subscriber
uses All while the viewer handles LastN history. [Focused evidence and limits](Qt6ParityValidation/lastn-20261004.md)
record unchanged full reports/accepted states, cleanup semantics, the decorative
listener distribution limit and the continuing host/GPU qualification stop.


### Sensor masks — 2026-10-04

OrbitView / Advanced / **Sensor masks…** edits an ordered list of named FOV displays.
Add a row, choose a plotted source and hardware with a configured field of view,
then set its label and Auto/Manual length. Auto uses the source's native model
bound when available, its body radius or a 1 km point fallback. Manual length is
in kilometres along the boresight. OK stays pending until Apply; Cancel/discard,
Undo/Redo and Save/reopen preserve the expected source transaction.

Conical, rectangular and custom masks use the hardware's position and pointing,
FOV color/opacity, white boundaries and an independent label. Both renderers follow
recorded and interpolated source pose. Remove/reassign masks before removing their
source from Add. Imported global hardware references are supported. This draws FOV
geometry and does not calculate visibility or a surface footprint. [Focused evidence](Qt6ParityValidation/sensor-masks-20261004.md)
records exact source/report preservation and the continuing host/GPU limits.


### Star display options — 2026-10-04

OrbitView / Advanced / **Star options…** selects Qt default, Monitor,
Virtual Reality, Printer or Custom. Monitor and Virtual Reality use the bundled
HYGv3 catalog, magnitude range −2 to 6 and a 40,000-star limit; their pixel-size
and dimming presets differ. Virtual Reality is a display preset. Custom enables
catalog Browse, count, magnitude range, pixel sizes and minimum dimming ratio.
Preset changes retain custom values. Printer hides stars on a white background.
Qt default removes the override and restores ordinary Qt style/catalog behavior.

The primary form's **Stars** checkbox remains the enable control. Its count and
the custom count share pending edits: the last edited value is retained. OK stays
pending until Apply; Cancel/discard, Undo/Redo and Save/reopen preserve source.
An unchanged imported preset does not rewrite the source's ordinary count.
[Focused evidence](Qt6ParityValidation/star-options-20261004.md) includes catalog coordinates,
size/brightness/body occlusion, source recovery and the remaining qualification
limits. No host/GPU or VR-device acceptance is implied.
