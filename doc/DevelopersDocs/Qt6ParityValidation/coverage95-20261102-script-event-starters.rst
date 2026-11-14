Complete Script Event command-support conjunction
================================================

Original requirement **FRC!E138** now qualifies: "The Script Event shall support
all commands that are supported in the main mission sequence." Coverage is
**1,003/1,158 (86.6%)**, with **98** further whole requirements needed for 95%.
Historical operation coverage remains **1,816/2,367 (76.7%)**. The original
denominator, requirement text, fingerprint and qualification gates are unchanged.

This closes the finite supported-command inventory, not every scientific model
configuration. The fresh 53-entry runtime census conjoins 47 bounded contained
command witnesses, four explicit alias/restriction decisions and the two newly
verified unique starters. Existing passes are reused at their recorded source
and runtime versions. MATLAB/unloaded plugins, newer private NASA corpus,
complete current-build, platform and physical-input qualification are not claimed.

Three production repairs
------------------------

``src/qtgui/MissionModel.cpp``: when the unique BeginMissionSequence lived inside
the first Script Event, source alignment started at that inner boundary and
dropped the opening BeginScript from the editable range. Alignment now starts
at the containing root event's opening statement, including a named header.
The entire remaining source suffix must still match. Changed labels, arithmetic
and extra boundaries must remain noneditable; three explicit unsafe variants
are checked in each new context.

``src/base/interpreter/Interpreter.cpp``: registered sequence starters now enter
command mode consistently with the existing native uniqueness validator.
Previously only BeginMissionSequence did so; arithmetic following
PrepareMissionSequence failed as if it preceded the mission boundary.

``src/base/executive/Moderator.cpp``: preparation-mode selection now agrees for
filename and GUI stream loading. Opening BeginScript markers are transparent
when identifying the first command and when executing preparation. Assignments,
branches and other commands are never skipped. A new stream load clears stale
preparation mode before selecting the new mission's mode.

The local C-interface documentation explicitly defines PrepareMissionSequence
as initializing the sandbox without executing the following control sequence.
The repaired GUI path is checked against real native filename loading.
No public API, class layout, numerical formula or force model changed.
Technical core approval remains false; source review is not user approval.

Exact preimages and increment-only diffs are retained in
``build/nasa-coverage/coverage95-20261101/``:

* ``event-starter-production-before/``;
* ``event-starter-MissionModel.diff``;
* ``event-starter-Interpreter.diff``;
* ``event-starter-Moderator.diff``;
* ``event-starter-core-changes.json``.

Results and failures
--------------------

Unique BeginMissionSequence passes the real body dialog and grouped Mission tree,
Cancel/pending/idempotent Apply, Undo/Redo, Unicode Save/Save As and reopening.
Plain and grouped execution agree with the independent 20-second circular orbit
and Count=21. Native containment requires exactly one starter inside event
markers. Three unsafe source-mapping variants reject.

Unique PrepareMissionSequence passes the same GUI lifecycle. Plain GUI stream,
native filename control and grouped GUI execution all leave epoch=21545,
the six initial Cartesian values, Count=0 and no following report. In the same
process, a subsequent ordinary GUI stream mission must produce Count=21,
the independent 20-second circle and a report, proving preparation mode clears.

Both cases pass unattended and visibly, with 16 new native exposure events.
The directly affected MissionNavigation regression also passes visibly with
18 exposures. A separate real native C-API regression passes 190 assertions,
21 epoch readings and nine runs, including preparation/reload transitions.
RunAcceptance did not select that support check; its separately executed wrapper
provides the native log, JUnit, actual loaded-image identities, unchanged input
tables and exact source archive. No broad old GUI suite was replayed.

All initial failures remain in ``event-starter-failures.json`` and raw receipts:

* Headless-v1 exposes the dropped event header and Prepare arithmetic rejection.
* Headless-v2 preserves both failures: the first mapping repair missed a named
  BeginScript header. Queued diagnostics identify the exact parser rejection.
* Headless-v3 passes Begin but the Prepare filename control rejects Unicode
  full-comment lines under the native filename parser's existing ASCII policy.
* Headless-v4 captures that precise ASCII diagnostic.
* Headless-v5 passes Prepare after its native control omits only full comment
  lines. Parsed executable statements must remain identical and ASCII. The GUI
  Unicode lifecycle and native file policy remain unchanged.

Selected unattended evidence uses Begin-v3 and Prepare-v5; visible-v1 passes
both with the final repairs. No existing acceptance assertion was relaxed.
All five target builds succeeded and their exact dependency bytes are retained;
no stale executable was launched. The six original branch tests and fixtures
remain byte-identical.

Qualification and reuse
-----------------------

``event-starter-evidence-audit-v1.json`` checks four starter selections and
127 consumed identities, including full process/JUnit proofs, native commands,
raw control results, exact executed source capsules and 149 dependency files
per successful build. Recorded runtime-image identity is not a GUI loaded-library
trace. The native C-API regression separately records actual loaded images.

``event-starter-qualification.json`` conjoins 2,494 bound artifacts with the
preceding sealed supported-command evidence. The admission checker verifies
the complete factory census, every witnessed entry's passing GUI receipt,
source-backed dispositions, both unique starters, independent whole-row review
and raw affected-regression provenance. Initial missing regression-artifact
bindings were corrected before admission, without native replay.

``event-starter-coverage-snapshot.json`` records the rebuilt union. Only FRC!E138
was added; no historical credit or unrelated requirement was inferred.
``event-starter-final-audit.json`` seals the final handoff sources and failures.
Thirty redundant summaries were losslessly compressed, saving about 51.6 MiB.
Before the final handoff seal this stage occupied about 29 MiB, with no passing
screenshots or copied runtime/kernel binaries.

Replay only these new contexts::

   cmake --build build/linux-gui --target GmatQtNasaScriptEventCommandTests -j 4
   python3 src/qtgui/tests/RunScriptEventCommandAcceptance.py --visible \
     --filter '^QtGui.NasaScriptEventStarter'

Restore the initially minimized nested desktop to watch; it is resizable,
automatically scaled and not fullscreen. Omit ``--visible`` for automation.
The helper's default filter now includes 37 additional contexts; the original
six branch cases remain separately registered.

Verify retained new evidence without running GMAT::

   python3 src/qtgui/tests/AuditScriptEventCommandAcceptance.py \
     --verify-receipt build/nasa-coverage/coverage95-20261101/event-starter-evidence-audit-v1.json

Why progress had stalled
------------------------

Many passing cases supplied partial evidence for this one broad obligation;
they could not honestly increase a whole-requirement numerator. Completing it
earns one row, not a point per test. This was a low-yield campaign priority.
The next concrete repair target is the existing GroundTrack skip boundary,
which holds four original rows open. Larger all-command matrices inside Target,
Optimize and functions are not the immediate next priority. Other original
gaps and external/historical prerequisite holds remain explicit; 95% is not
achieved or promised by this increment.
