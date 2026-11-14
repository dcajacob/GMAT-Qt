Optional cached A1ModJulian summaries: two complete original requirements
=========================================================================

Coverage is **1,002/1,158 original requirements (86.5%)**, up from1,000.
The95% target requires1,101, leaving99. Historical operation coverage stays
**1,816/2,367 (76.7%)**. The original workbook/RTTM, rows, fingerprints,
denominators and existing native assertions are preserved.

``All Others!E211`` has exactly one listed child: ``All Others!E212``,
A1ModJulian availability in state summaries. A reachable **Include A1ModJulian**
checkbox now adds each command's cached spacecraft epoch. It is off by default,
preserving the original four UTC/TAI/TT/TDB rows byte-for-byte. The old native
test's explicit A1-absence assertion remains unchanged and passes separately.
Its ``a1_gap:true`` record describes default output, rather than the availability
of the new optional display.

Implementation and core changes
-------------------------------

``GmatCommand::GetSummaryWithA1Epochs`` scopes a thread-local formatting request
around the existing command/mission renderer. A guard restores the previous
request after success or exception. Mission, branch and specialized inherited
summaries use the central renderer, which reads cached scalar ``epochData[i]``.
No epoch is reconstructed from another rounded time row or substituted from
the latest spacecraft state. The requested value is GMAT A1 modified Julian
days (JD minus2430000.0), printed in the existing modified-Julian column.

The six changed production paths are explicitly tracked:

* ``src/base/command/GmatCommand.cpp`` and ``GmatCommand.hpp``: additive
  nonvirtual summary-rendering method and optional cached epoch row.
* ``src/qtgui/InspectionDialog.cpp`` and ``InspectionDialog.hpp``: checkbox,
  refresh and error rollback of the complete coordinate/physics/A1 selection.
* ``src/qtgui/MainWindow.cpp`` and ``MainWindow.hpp``: optional argument routed
  through the existing current-mission/generation guards and modal dialog.

Exact pre-increment copies, SHA-256 identities and incremental diffs are in
``build/nasa-coverage/coverage95-20261101/summary-a1-core-before`` and
``summary-a1-core-changes.json``. This changes core summary presentation and Qt
production code. It adds no simulation-state field or vtable entry and changes
no propagation, forces or time conversion. The preceding C-interface source
changes remain untouched. Independent implementation review supplies no blanket
scientific/core approval or full-current-build qualification.

Runtime evidence and retained failures
-------------------------------------

The new test passes in unattended and visible modes, each with:

* Three spacecraft with distinct fractional initial epochs, including one
  unpropagated spacecraft.
* Fifteen report checks against independent literal epochs and elapsed-day
  arithmetic; forward20+5seconds, backward10seconds and two late assignments.
* Ninety displayed epoch checks across12 views, including earlier-cache revisits,
  frame changes, named full-mission and physics-only command checkpoints.
* Nine export/display equality checks, exact opt-out/default restoration,
  source/history/report isolation and an unexecuted branch with no cached epoch.
* An exception forced inside native opt-in rendering, followed by a default
  read proving that the presentation request was restored.

Full-mission values cover eight named command checkpoints times three spacecraft;
physics-only values cover three propagations times three spacecraft. Counting
rows alone does not provide the numeric evidence.

Only two directly affected existing regressions are replayed: the unchanged
``QtGui.NasaCommandSummaryFields`` and ``QtGui.Inspection``. Both pass visibly.
The former retains all176 original field comparisons, including default A1
absence. The native xcb observer records37 exposure events for the new case,
13 for the old fields test and11 for Inspection. The nested desktop is resizable,
initially Iconic and has no fullscreen flag. Its recorded host window metadata
also includes maximized flags; no person-watching or physical compositor/GPU
qualification is claimed. No screenshots or runtime binary copies are retained.

Two failed new-test attempts remain recorded. A reused substring selector picked
the enclosing If instead of the named unexecuted Propagate. The raw displayed
``If Command: BranchChoice`` exposed that mistake. Exact parsed command identity
fixes the new driver; the unexecuted-cache assertion is unchanged. A separate
visible-launch attempt failed to access the host X socket from the sandbox,
before any native case. Authorized host access resolved that setup issue; the
failed launcher contributes no GUI evidence.

Nine audit controls reject wrong live/mission epochs, wrong physics identities,
missing spacecraft or mode/evidence seals, wrong Julian units, extra source
credit and unearned core approval. They run no additional missions. All old
passing receipts are reused at their original source versions. No broad GUI
suite or previous Save cases are replayed.

Reusable automated and visible replay
-------------------------------------

Build and run the new automated registration::

  cmake --build build/linux-gui --target GmatQtNasaSummaryA1Tests -j4
  ctest --test-dir build/linux-gui --output-on-failure -R '^QtGui.NasaSummaryA1$'

To watch the new case in a resizable, non-fullscreen, initially minimized desktop::

  python3 src/qtgui/tests/RunSummaryA1Acceptance.py --visible

Restore the minimized test desktop to watch. The default350ms pauses pace the
checkpoints; ``--pause-ms`` adjusts pacing. ``--include-affected`` adds only the
two affected regressions; ``--list`` checks the plan without executing a case.
The helper creates a fresh runtime-only manifest and changes no historical NASA
receipt. Its run-local coverage report is not the cumulative audited union.
The test also belongs to ``check-qt`` and ``build-qt-acceptance``.

Evidence and remaining scope
---------------------------

Artifacts under ``build/nasa-coverage/coverage95-20261101`` include
``summary-a1-qualification.json``, ``summary-a1-coverage.json``,
``summary-a1-independent-review.json``, ``summary-a1-negative-controls.json``,
frozen auditors/source archives and ``summary-a1-final-audit.json``. Original
failures remain distinct from selected passing headless-v3/visible-v2 evidence.
Duplicate run reports/traces are losslessly compressed after auditing.

All156 remaining original requirements and551 historical cases remain explicit.
The95% goal remains active. The59 original prerequisite holds are not retired;
the optimistic all-non-held ceiling remains1,099/1,158 (94.9%), requiring at least
two substantive prerequisite resolutions to reach1,101. The GroundTrack sampling
boundary remains a real enabled failure. Complete resource persistence/scoping,
state/frame conversion families, iterative ephemeris output, optional missing
models, MATLAB, historical platforms and unspecified hardware remain unqualified.
