Help tutorial GUI replay
========================

The default tutorial driver uses QtTest mouse/keyboard events addressed to live
Qt widgets. Selectors identify object names, button captions, labelled fields,
tree items, menu entries and tabs. It rejects coordinate-only clicks. It waits
for controls to exist and be enabled, scopes duplicate controls to the active
dialog or editor, and asserts typed text, selections and checkbox states.
``TutorialGuiProbe`` links the same frontend as GmatQt, hosts this test driver,
and records source, completed-run endpoints, summaries and output files. It
never sets editor text, resource parameters or mission commands through setters.
Its explicit RNG fixture sets the same seed immediately before each GUI and
reference run; this makes noise-enabled navigation examples reproducible.
An explicit SetSeed instruction taught by a chapter can subsequently override
that fixture; the corrected GPS chapter uses the example's SetSeed(1).

Watch a tutorial
---------------

On this Linux checkout, from a terminal in the logged-in desktop::

  python3 src/qtgui/tests/RunTutorials.py --build --visible --tutorial 1 --pause-ms 1000

Omit ``--tutorial 1`` to select all eighteen published Help chapters. Repeat
``--tutorial`` to select several. ``--list`` shows the inventory without starting
an application. A requested visible run opens a Xephyr window containing the
private replay desktop. By default it starts with the host display's Qt scale
and sizes the window to fit the enlarged controls on the host screen. Override
the scale explicitly if desired::

  python3 src/qtgui/tests/RunTutorials.py --visible --scale 1.5 --pause-ms 1000

``--scale auto`` is the default; ``1`` means 100%, ``1.5`` means 150%, and ``2``
means 200%. Factors from 0.5 to 4 are accepted. The effective host Qt/X11 factor
can differ numerically from the desktop's fractional setting because XWayland
may render at a higher scale before the compositor scales the result.
Unattended runs default to scale 1 for reproducibility; an explicit factor
also works there. Scaling affects only the private replay UI.

Choose an explicit starting desktop size in pixels with::

  python3 src/qtgui/tests/RunTutorials.py --visible --width 2560 --height 1440 --pause-ms 1000

The visible QtTest window is resizable: drag its outer border or maximize it,
and the inner desktop and maximized GMAT window follow. ``--width`` and
``--height`` select its starting size. Use ``--fixed-size`` to keep that size.

``--width`` accepts 640 to 4096 pixels and ``--height`` accepts 480 to 3072.
These options also set the size of unattended Xvfb runs. Custom sizes use the
default semantic QtTest driver; historical coordinate recordings retain their
original size. Choose dimensions that fit your monitor.

The replay does not inject input into other host
windows. Without ``--visible``
the same actions and comparisons run on private Xvfb. Both use software GL.
QtTest delivers GUI events to identified widgets; it does not require the host
mouse or keyboard focus. Maximizing the test window and scrolling identified
controls into view are layout fixtures. The simulation is still authored via
normal frontend event handlers.

All eighteen published chapters have semantic QtTest recipes. Their simulation
comparisons can report differences; completed construction is distinct from an
identical supplied example. An action without a selector fails explicitly. The
``--driver legacy-x11`` option retains the old XTest/coordinate fallback route
only for diagnosis; its results are identified separately and do not qualify
the new semantic QtTest suite.

``--pause-ms`` (0 to 60000) controls pauses at named checkpoints. The independent
``--input-delay-ms`` (0 to 1000, default 100) allows additional settling between
inputs. The stage timeout includes these requested delays. An event-loop
acknowledgement also waits for Qt to consume input.
Ctrl+C cleans up only owned application/WM/server processes and retains evidence.
Closing the visible desktop terminates that case as a failure.

``--seed`` selects the recorded unsigned 32-bit seed (default 424242).
``--unseeded`` keeps the engine's random-device initialization. Raw measurement
equality is then generally inappropriate for chapters that enable noise. The
seed fixture is confined to the test executable; no seed statement or other
configuration is inserted into an authored or supplied mission script.

Prerequisites are Xvfb, Xephyr and xrandr for resizable visible mode, xauth, ImageMagick ``import``,
Python with Pillow/NumPy/SciPy, and Openbox. On Debian/Ubuntu x86-64, prepare a
private Openbox without installing system packages::

  python3 src/qtgui/tests/PrepareTutorialDesktop.py

The runner discovers that copy under ``build/tools/tutorial-desktop`` or a system
``openbox``. Other explicit copies can be selected with ``--wm``,
``--wm-library-path`` and ``--wm-data-dir``. Existing desktops and configuration
are not changed. Unsupported or missing prerequisites fail before qualification.

Construction and comparison
---------------------------

Every selected tutorial starts from a fresh default mission. Continuation
stages may open only milestones generated by that case's preceding GUI input.
The event-location tutorial rebuilds its Tutorial 2 prerequisite; DSN estimation
rebuilds its Tutorial 9 prerequisite. Newly generated observations remain data
dependencies. They are not imported mission configurations. Guide-taught script
or function editing remains keyboard editing through the corresponding GUI.

Ported recipes express the reviewed Help operations through current GUI
controls. ``BuildGuideGuiRecipes.py`` generates reviewed form-based recipes;
``BuildGuideScriptRecipes.py`` transcribes explicitly taught code listings.
The QtTest driver reads the actual QObject/widget tree. It uses input events
for typing and choices, including modal dialogs and context menus. The recipes
contain no recorded coordinates or image anchors. The separate legacy driver
remains available for diagnosis of old external recordings. Recipes require
maintenance when controls or defaults change; an unsuccessful replay is
a harness failure until its cause is established, not automatically a GMAT bug
or a typo in the user guide. Guide hashes force a review after chapter changes.

The runner snapshots the effective recipe, including prerequisite stages, before
input begins. It freezes the newly constructed source, its hash, run receipts and
artifacts before reading a supplied reference. It then opens and runs the
reference in a separate offscreen frontend process, using a byte-identical copy
in a private directory with the required relative data paths. Original
references remain unchanged. It retains these distinct results:

* Exact script bytes and a complete unified diff.
* Executable statements after removing comments and the optional GMAT prefix,
  and normalizing whitespace between grammar tokens. Quoted string contents
  and boundaries between adjacent vector numbers are preserved.
  Defaults, resource order, labels, commands and viewer settings remain visible
  differences; they are not silently normalized away.
* Final sandbox spacecraft epochs, J2000 origins, six Cartesian components and
  total masses. Absolute comparison tolerances are recorded: 1e-9 days, 1e-5 km,
  1e-8 km/s and 1e-7 kg respectively. These are endpoint comparison tolerances,
  not an independent scientific accuracy claim or every chapter's goal test.
* Complete output-file bytes, with snapshots taken at completion. Diagnostic
  GmatLog is excluded. The additional content comparison replaces only the
  estimator report's ``Run Date`` wall-clock header; every observation epoch,
  measurement, residual and other numerical value remains exact.
  A separate whitespace comparison identifies layout-only differences while
  preserving every non-whitespace token. It does not turn a mismatch into a
  pass. Files unchanged since the start of a run are recorded as preexisting
  inputs/artifacts rather than outputs of that run. Explicit report paths
  outside the default output directory are captured too.

For chapters that continue beyond their supplied example, explicitly declared
``reference_milestone`` stages have an additional comparison. The final source
and extension results remain separately reported. The GPS chapter declares
SimSat from its simulation stage and EstSat from its smoother stage, combining
only those observed results and their output snapshots. This reflects the
guide's separate files versus the combined example. It cannot turn a failed
stage or an undeclared later prediction into a baseline match.

``Qt6TutorialMismatchReview.md`` classifies all sixteen original mismatches,
documents the GUI/guide fixes and distinguishes filename, alias, whitespace,
viewer/default and expected correction-history differences from numerical
simulation differences. Strict comparisons are retained unchanged.

The old OpenFrames examples and the Qt-created plots often have different
executable viewer definitions. A numerical match does not make those scripts
identical. ``matched`` requires statements, endpoint comparisons and output
contents to match. Exact output-byte equality remains separate. ``mismatch``
preserves a completed construction with differences.
``failed`` records unsuccessful construction or reference execution.
``no-reference`` records the absence of a comparison oracle.
No standalone completed reference exists for the event
chapter; its comparison cannot be claimed from the Tutorial 2 prerequisite alone.
Filter/smoother and other staged chapters require separate interpretation of
their related examples and intermediate outcomes.

Evidence and scope
------------------

The dated ``Qt6TutorialReplayResults.md`` records the completed eighteen-chapter
reconstruction and its retained comparison findings.

``--output`` selects a new directory; otherwise a fresh temporary directory is
printed. It contains ``report.md``, ``results.json``, per-tutorial results, the
construction freeze, source diffs, numerical/output comparisons and per-stage
action logs, anchor locations, original/isolated startup files, runtime hashes,
GUI snapshots and completed-run receipts. Intermediate process cleanup is not
a graceful-exit qualification. Runtime identities are retained per stage.

Exit 0 requires all selected comparisons to match (or a successful inventory
listing). Exit 1 retains failures or differences; exit 130 means interrupted.
Historical manual passes do not qualify a new automated replay. The existing
manual ledger is ``Qt6TutorialWalkthrough.md``; current replay evidence must be
consulted separately. This test does not close physical GPU/Wayland acceptance,
native portals, every optional exercise, proprietary dependencies, or Windows
and macOS qualification.

Check the runner's independence and result contracts separately::

  python3 src/qtgui/tests/TutorialRunnerTests.py

These support checks do not substitute for a successful GUI reconstruction.
The CTest ``QtTutorials.SemanticDriver`` check exercises moved windows, duplicate
button captions, active-editor scoping, nested modal dialogs, context menus,
combo choices, wide checkboxes, coordinate rejection and read-only protection.
