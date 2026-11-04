Qt GUI acceptance runner
========================

Run from the Qt checkout. The same CTest scenarios and result assertions serve
both unattended and visible runs. This is a comprehensive runner for the
registered regression checks, with an explicit inventory of acceptance gaps;
it does not certify every widget or every NASA requirement.

The final bounded failure-repair handoff is
``Qt6ParityValidation/remaining-failures-20261031.rst``. It records the later
21 effective failure repairs, core scrutiny, selected visible/native receipts
and coverage gaps. It does not claim a fresh full-suite replay or NASA
qualification.

The production fixes for the initial eleven failing checks are recorded in
``Qt6GuiAcceptanceFixes.rst``; their original assertions remain unchanged.

``Qt6NasaCoverage.rst`` describes the separate source-bound requirements
inventory and reviewed execution proofs being added toward NASA coverage.

``Qt6GuiAcceptanceCloseout.rst`` describes the bounded Linux handoff,
current execution reports, unresolved scientific findings and deferred backlog.

Build and watch
---------------

From a terminal in the logged-in Linux desktop::

  cd /home/dan/GIT/GMAT-Qt
  python3 src/qtgui/tests/RunAcceptance.py --build --snapshot \
    --visible --start-minimized --scale auto \
    --pause-ms 100 --max-checkpoints 5000 --max-screenshots 0 \
    --output build/gui-acceptance/my-new-visible-run

This builds ``build-qt-acceptance`` and runs every ``qt-gui`` CTest check in
sequence. On Linux, a normal, resizable Xephyr window titled "GMAT GUI acceptance
- live test desktop" shows an authenticated test desktop on the existing
X11/XWayland DISPLAY. Its starting size and UI scale use the same host-aware
configuration as the tutorial runner. It does not enter fullscreen. You can
resize or maximize its outer window yourself. Xephyr, xauth and xrandr must
be installed. Its TCP listener is disabled, its authorization is private to the
run, and only the owned test server is stopped at completion/interruption.
The host compositor and settings are not changed. Windows uses native Qt
windows; that route has not been validated by this task.

``--display-route native`` puts test windows directly on the existing desktop.
It can expose additional window-manager constraints, occlusion and DPI issues;
these failures are retained. Native rendering uses software OpenGL. Tests with explicit CPU
fallback/offscreen guards retain that route and are listed as visibility gaps.
Model/parser/helper checks are identified as noninteractive support checks.
macOS visible operation has not been implemented or qualified.

Visible runs default to the host's Qt/XWayland scale. ``--scale auto`` selects
this policy; ``--scale 1.5`` selects 150 percent explicitly. Starting dimensions
can be set with ``--width 2560 --height 1600`` (physical test-desktop pixels).
The default size leaves room for host decorations. The shared tutorial resize
fix refreshes RandR so Qt receives updated inner-screen bounds when you resize
the outer window. Geometry changes are recorded in the receipt. With
``--start-minimized``, restore the outer test desktop when you want to watch;
it remains resizable. Use a new output directory for each run. ``--snapshot``
currently supports Linux; omit it on other platforms, whose qualification
remains separate.

``--display-scale test --fixed-size`` retains the original controlled 1x,
1600x1200 baseline. ``--display-scale system`` is an alias for the host-aware
default. The dedicated 2x/1.5x CTest cases still apply their own factors and
offscreen-only cases retain their 1x baseline. Larger-scale failures remain
failures; assertions are unchanged. Resume checks include scale, starting size
and resize mode, so different display profiles cannot silently share passing
results. No desktop settings are changed. Physical/system-DPI qualification
remains separate.

``--pause-ms 1000`` slows visible checkpoints. ``--max-checkpoints`` bounds
the pacing overhead per process (default 24, maximum 5000), independently of
``--max-screenshots`` (default 12, maximum 1000). The example disables automatic
observer screenshots while retaining traces, assertions and result files.
Separate images required by visual assertions remain part of those tests.
A zero pause still records traces and any enabled captures. Pauses occur when
a native window is exposed and use no nested event
loops; the existing tests retain ownership
of their modal timers and assertions. Automatic checkpoints capture window
showing and button/action activation; this does not guarantee that every
intermediate programmatic edit remains on screen for a viewing interval.
OpenGL snapshots read the exposed native surface without calling QWidget grab
or repaint on the renderer. Ordinary widgets can also have a constructed-state
snapshot before exposure; that image is not treated as native exposure evidence.

For a shorter demonstration::

  python3 src/qtgui/tests/RunAcceptance.py --snapshot --visible --start-minimized \
    --filter '^QtGui\.(Desktop|Workflow|MissionInsertion|NativeWindows|NativePlots)$' \
    --pause-ms 700 --max-screenshots 0

Automatic observer screenshots from verified passing cases are discarded after
the run by default. Failure/interruption images and separate images used by
visual assertions remain, along with logs, traces, JUnit, numerical outputs and
original receipts. Add ``--keep-passed-screenshots`` to retain passing observer
frames. A separate retention record stores removed frame hashes and dimensions;
it does not replace the pixels for later visual inspection.

Snapshots preserve executables only for the selected filter, while retaining
the complete catalogue and coverage denominator. Resuming a snapshot rejects a
selection whose executables were not preserved. Identical large inputs share
read-only cached copies under ``build/linux-gui/acceptance-input-cache``; live
worktree/build files are copied into that cache, never hard-linked to snapshots.
Do not edit frozen inputs. Existing snapshots remain valid if unused cache names
are removed, because their hard links retain the data.

Filters leave the unexecuted cases visible in the coverage report. They do not
shrink the coverage denominator. Ctrl+C stops the owned CTest/process group,
retains the interrupted case and writes the final report. The runner never
targets an already-running GMAT session.

Coverage estimates print at completed-case boundaries, at most once every
60 seconds, plus the final summary. Set ``--coverage-interval-seconds 120`` to
change the interval or ``0`` to keep only the final estimate. A long-running
case delays the next estimate until it finishes. ``coverage-estimate.json``
and ``report.md`` retain the current figures: reviewed requirement scopes and
historical operation scopes have separate denominators. Passing more regression
checks, observing widgets or saving screenshots does not itself increase these
figures. Full widget/input-domain coverage remains unquantified.

The preserved installation's default ``application/output`` path is an explicit
link to a separate writable output directory. This allows the shipped launcher
to discover its normal startup file without writing into preserved inputs.
Snapshot verification checks the link target; changing it to an input directory
fails verification. Pre-existing user output files are not copied into the test.

Unattended automation
---------------------

Use the same suite without ``--visible``::

  python3 src/qtgui/tests/RunAcceptance.py --build --snapshot --max-screenshots 0 \
    --output build/gui-acceptance/my-new-run

The result directory must not already exist. Omitting it creates a fresh
temporary directory and prints its absolute path. These CMake targets are also
available::

  cmake --build build/linux-gui --target build-qt-acceptance --parallel 4
  cmake --build build/linux-gui --target check-qt-acceptance
  cmake --build build/linux-gui --target watch-qt-acceptance

``check-qt-acceptance`` and ``watch-qt-acceptance`` each build and execute the
complete registered suite, including the separate support checks exactly once.
The watch target starts the Linux test desktop minimized and resizable. Both
targets disable automatic observer screenshots. Restore the test desktop to
watch it. Each invocation creates and prints a fresh result directory; use the
Python runner below when choosing an output directory or resuming prior work.

The Python runner selects ``qt-gui`` checks. To include the separate acceptance
and tutorial support checks exactly once, use this sequence on Linux::

  cmake --build build/linux-gui --config Release --target build-qt-acceptance --parallel 2
  ctest --test-dir build/linux-gui -C Release \
    -L '^qt-(acceptance|tutorial)-support$' -LE '^qt-gui$' --output-on-failure
  python3 src/qtgui/tests/RunAcceptance.py --snapshot --max-screenshots 0 \
    --output build/gui-acceptance/my-new-complete-run

Check each command's exit status before proceeding. Add the visible options
from the first example to the final command to watch the same GUI scenarios.
The 2026-10-31 catalogue has 1732 ``qt-gui`` checks and 11 separate support
checks. The explicit label exclusion is necessary: 12 acceptance support
checks also carry ``qt-gui``. Both CMake acceptance targets use this same
disjoint selection. The previous name-based prepass repeated 13 GUI cases and
omitted the two tutorial support checks; that scheduling defect is corrected.
Do not launch it concurrently with another GUI test run.

``--build-dir`` and ``--config`` select another configured build. Existing
``check-qt`` remains available. When the acceptance environment is absent, its
offscreen/private-X11 behavior is unchanged and the observer is dormant.
The observer is linked only into tests, not the shipped application or frontend
library. Both native and offscreen acceptance runs keep scenario assertions;
this is not a separate demonstration with weaker checks.

CTest owns test timeouts, environments, expected failures and resource locks.
Visible cases import their original CTest registrations into a per-case driver.
Their default timeout is three times the original timeout to allow real native
rendering, plus the maximum configured viewing-pause budget. Set
``--visible-timeout-factor 1`` to retain the original execution allowance plus
viewing time. Their
assertions, environment, expected failures and original working directory stay
the same. The receipt records both timeout and viewing budget. This supports
slower viewing without silently weakening result checks.
The runner invokes one case at a time, preserving full verbose output and JUnit
results. Avoid running a separate GUI test process concurrently: CTest resource
locks do not coordinate different CTest invocations. Startup arguments supplied
by CMake are remapped to per-case copies with private output/log/personalization
paths and absolute installation references. Scenario-authored startup fixtures
and their negative tests are not remapped. Existing ``TestSettings`` isolation
is preserved. Tests can still intentionally inspect or temporarily change
fixtures as described in their source.
Visible widget tests explicitly use Qt QFileDialog widgets, matching their
offscreen automation contract. Native portal dialogs require a separate OS-input
driver and remain a coverage gap.

Exit 0 means the selected regression checks passed (or an inventory-only
``--list`` completed); it does not mean full GUI acceptance. Exit 1 reports
failing/timed-out checks, exit 2 reports remaining gaps when
``--strict-coverage`` is used, and exit 130 records interruption. Use
``--strict-coverage`` to prevent an incomplete suite from being accepted as
complete coverage. It intentionally fails on the current known gaps.

Evidence and coverage
---------------------

The output contains:

* ``report.md``: readable mechanism/case results and explicit coverage gaps.
* ``results.json``: run settings, test/source/binary hashes, test results, all
  registered mechanisms, checked-result messages and coverage limits.
* ``junit.xml``: aggregate selected-test results for CI.
* ``mechanisms.csv``: local mechanism IDs, intended behavior, NASA documentation,
  linked tests, failures and missing execution evidence.
* ``control-coverage.json``: literal named-control source locations, candidate
  test references and observed/changed/activated evidence.
* ``nasa-historical-matrix.json``: every row of the 2012 NASA matrix, with its
  historical status retained separately from current applicability reviews,
  scoped operation proofs and unresolved rows.
* ``nasa-requirements.json`` and ``nasa-traceability.md``: requirement instances
  from the shipped workbook, reviewed applicability, exact proof linkage and
  explicit source conflicts. Workbook requirements and historical operation
  rows are separate denominators.
* ``plan.json``: exact CTest registrations, sources and visibility routes.
* Per-case verbose logs, JUnit XML, startup copies, result receipts, JSONL traces
  and bounded visible PNG snapshots.

The coverage contract is ``src/qtgui/tests/AcceptanceContract.json``. Its
26 ``UI-*`` IDs are local mechanism groupings, not invented NASA requirement
numbers. They link to the repository's NASA user-guide XML. The NASA test plan
at https://gmat.atlassian.net/wiki/spaces/GW/pages/380273283 supplies the widget,
requirement, valid/invalid/boundary, system, stress and graphical-test goals.
The historical requirements-to-test CSV contains 2,367 cases across 62 object
groups. An old Passed field is never treated as evidence that the current Qt
implementation passed that requirement.

Source-bound NASA expansion checkpoints are recorded in ``Qt6NasaCoverage.rst``
and the versioned ledger at ``build/nasa-coverage/coverage-ledger.json``.
The 2026-10-31 stocktake records cumulative demonstrated coverage of 925/1158
requirement scopes and 1833/2367 historical operation scopes. Those receipts
span preserved builds; they do not establish a full pass on the current build.
The earlier 210/210 visible run, with 30 reviewed requirement-instance proofs
and 200 historical-operation proofs, is a historical checkpoint. Remaining
workbook entries, historical cases and assurance gaps stay explicitly open.

The runner deliberately distinguishes:

* An executable test passing its existing assertions.
* A source reference suggesting a control-to-test mapping.
* A widget being created/observed, shown, changed or activated.
* An actual native window exposure and a native-surface/widget snapshot.
* A reviewed requirement/control-level outcome oracle, which is not established
  merely by any of the preceding observations.

Qt ``setText``/``setCurrentIndex`` and button/signal calls appear as programmatic
changes/activations. Input-event records include Qt's ``spontaneous`` flag, but
are not claimed as an OS input-driver qualification. The current static census
includes literal ``setObjectName`` declarations only. Dynamic property rows,
unnamed widgets, delegates, context menus and native desktop controls need
additional reviewed inventory. It is invalid to calculate a whole-GUI coverage
percentage from the named-control census.
Runtime evidence is matched to that census by object name; reused names are
ambiguous, and attribution to an individual source declaration is not established.

Existing test oracles include exact source/Undo/reopen checks, invalid-edit
recovery, known/analytic numerical outputs, independent authored-script
comparisons and geometric/pixel checks. Same-engine script equality proves a
bounded GUI equivalence, not independent scientific truth. Full tutorial GUI
replay, scientific/dependency holds, physical Intel/Wayland lifecycle, native
portals/input, full stress/platform matrices, per-control input boundaries and
case-level NASA traceability remain listed gaps. The current acceptance ledger
is ``doc/DevelopersDocs/Qt6ParityValidation/current-acceptance-20261002.md``.

Maintenance and validation
--------------------------

New Qt tests are discovered automatically from CTest. Give every new registered
case an explicit mechanism mapping in the contract. The runner reports unmapped
cases; ``QtAcceptance.RunnerContract`` also checks the current CMake declarations
against the contract. Update the intended behavior/source and gap descriptions
when adding a new oracle, rather than upgrading coverage from a test name alone.

Only adapt an offscreen guard for a widget transaction test after validating
the native route. ``AcceptancePlatform.hpp`` provides the explicit opt-in
platform check; dedicated fallback/pixel/focus tests retain their constraints.
Optional observation must never alter an assertion or convert a failure into
a pass. Use the support checks independently::

  ctest --test-dir build/linux-gui -R '^QtAcceptance\.' --output-on-failure

The contract tests include a real Qt observer probe, report/status distinctions,
startup isolation, visibility routing, source-mapping limits and trace recovery.

Validation checkpoint: 2026-10-26
---------------------------------

The full registered suite completed with 172 passes and 11 failures. All eleven
failures also reproduced through ordinary CTest. The receipt/report is in
``build/gui-acceptance/20261026-qualified-run/``. It retains 488 snapshots and
records native exposure for 125 cases; offscreen exposure flags are not promoted
to native visibility. Six eligible cases have no recorded native exposure.

The final observation-hook update is checked separately by all six affected
visible renderer/window/focus/desktop cases in ``20261026-pacing-final``. All 15
runner contracts pass, and ``20261026-display-auth-check`` verifies the final
private-display authorization transport. ``validation-summary.json`` records
this bounded sequence and source hashes; a new full matrix after those final
observation/reporting changes is not claimed. The actual host-visible test
desktop was captured and inspected.

Existing failures cover object-drawing removal, Contact resource creation,
external-force preservation, plot/solver toggles and disabled ephemeris
intervals, plus plugin resource creation. They remain failures with nonzero
runner status. No result assertion was relaxed or converted to an expected pass.
Full per-widget/NASA-case coverage, explicit visual steps for transient dialogs,
native portal/OS input and physical host/platform qualification remain gaps.

NASA expansion checkpoints
--------------------------

The eleven initial failures were fixed without relaxing their assertions;
see ``Qt6GuiAcceptanceFixes.rst``. The later resource-family checkpoint passed
210/210 checks visibly and is retained in
``build/gui-acceptance/20261026-nasa-expanded-final/``. Subsequent command-family
coverage, proof scopes and source conflicts are documented in
``Qt6NasaCoverage.rst``. Consult the newest receipt for current results; the
historical 172/183 checkpoint above is not the current failure count.

Preserved builds and resumable batches
--------------------------------------

Use ``--snapshot`` when other work may rebuild the shared checkout. It copies
project executables, project runtime libraries/data, test helpers/fixtures,
reviewed sources and CTest registrations into the report directory. The runner
checks the complete capture for changes before publishing its manifest. CTest
then executes the copied registrations and binaries, with the copied project
libraries taking precedence in the loader path. Per-case startup/output/settings
isolation remains enabled. Relative project paths and registered path arguments
are relocated; external tools/libraries and external paths remain outside the
snapshot guarantee. This is an artifact/input identity guarantee, not compiler
provenance or host/GPU qualification.

Preserved/resumable runtime support is currently Linux-only. Ordinary CTest and
the existing non-preserved runner paths retain their platform behavior; this
feature does not qualify Windows or macOS. Failed CTest setup (including a
missing executable) is a failure, not a successful skip.

Run a watchable batch of up to 50 pending cases::

  python3 src/qtgui/tests/RunAcceptance.py --build --snapshot --visible \
    --max-cases 50 --output build/gui-acceptance/preserved-1

Continue the same preserved build without rebuilding it::

  python3 src/qtgui/tests/RunAcceptance.py --resume build/gui-acceptance/preserved-1 \
    --visible --max-cases 50 --output build/gui-acceptance/preserved-2

For further batches, resume the most recent report directory. Keep config,
visibility, display route and display scale unchanged. Viewing pause and image
budgets may change. Exit 3 means the requested batch finished but more selected
cases remain; exit 1 still means a failed check or damaged snapshot. Omit
``--max-cases`` to run all pending selected cases. ``--strict-coverage`` retains
its separate exit 2 for unresolved coverage gaps.

A resumed result is reused only when its snapshot identity, case command and
properties, execution settings, successful JUnit result and log/JUnit hashes
match. Skipped, failed, missing, altered and incompatible results run again.
A completed per-case result can be recovered after an interruption before the
aggregate receipt was written. Reports label reused evidence and link its
original log; they do not imply that the case executed again. Snapshots are
verified again at the end of each batch. Added, changed or deleted snapshot
inputs invalidate reuse. Do not edit the saved bundle.

Runs made without ``--snapshot`` cannot be resumed safely. If controller code
changes, use the archived ``snapshot/repo/src/qtgui/tests/RunAcceptance.py`` to
resume that older bundle, or create a new snapshot for the newer suite. A new
snapshot is also required to include newly registered tests or a new build.

Generated test products stay outside the preserved input tree. The registered
build-tree capture prefixes used by solver/segment tests are redirected to
``<case>/outputs/`` for each attempt; the executed command is recorded in its
result. The runner rewrites only that generated CTest command line and retains
its other registrations and properties. Unexpected files or mutations inside
the input snapshot still fail integrity validation. A run that recorded an
integrity failure cannot lend reusable passing results even if its files are
later restored.

On Linux, the ``check-qt-acceptance`` and ``watch-qt-acceptance`` build targets
now select ``--snapshot`` automatically. Direct Python invocation keeps it
explicit. Existing ordinary CTest and non-Linux build-target behavior remain
available; snapshot runtime support is not being presented as cross-platform
qualification.

Explicit exposure in short widget tests
---------------------------------------

``tests/AcceptanceView.hpp`` provides ``acceptanceView(widget)`` for short
widget tests that finish before a native window can expose. Call it at a known
safe boundary, after creating the fixture and before scheduling modal timers;
do not place it in a generic event filter or pass an editor invalidated by an
Apply refresh. In visible mode it waits for actual native exposure and rejects
offscreen/minimal platform substitutes. In unattended mode it is a no-op.
Keep all result assertions in place. Exposure only proves that a window was
presented; it is not a per-control result oracle or OS-input qualification.

Frontend transaction-only checks can retain the ``qt-gui`` inclusion label
while adding ``qt-acceptance-transaction``. The runner reports those checks as
support rather than authored GUI replay; it still executes them.

Start the test desktop minimized
--------------------------------

Use ``--visible --start-minimized`` to launch the outer Linux Xephyr desktop
minimized. Restore its ordinary framed window whenever you want to watch.
The inner test windows keep the states required by their assertions; the
option does not minimize each application window or request fullscreen.
It requires the nested display route and verifies the host window reached
its iconic/hidden state before starting test clients. The setting is part of
the preserved execution profile and must also be supplied when resuming.
