Script Event support: eighteen more contexts and two repairs
==========================================================

All eighteen new contexts pass unattended and visibly at their recorded source
versions. Original requirements coverage remains **1,002/1,158 (86.5%)**,
with **99** more whole requirements needed for 95%. Historical operation coverage
remains **1,816/2,367 (76.7%)**. FRC!E138 remains open; no original scope,
denominator, fingerprint or qualification gate was changed.

This batch reuses the preceding nine command contexts and six original branch
passes without rerunning them. It adds six SaveMission boundary contexts,
nested events, Stop, four plot commands, Write, CommandEcho, builtin/GMAT/Python
calls and UpdateDynamicData. The supported-command inventory now has bounded
witnesses for 39 of its 53 entries. That count includes implementation aliases
and is not a NASA coverage percentage or complete current-build qualification.

Production changes to scrutinize
-------------------------------

Only two recorded production paths changed; 293 other recorded production
source files remain identical. No public API, class layout, numerical formula,
force model or conversion primitive changed.

``src/base/command/SaveMission.cpp``: a terminal SaveMission previously exported
the plain mission but produced no file inside a final Script Event. Its physical
last-command check encountered the closing EndScript marker. The repair examines
only the immediate successor suffix and skips consecutive EndScript markers.
It rejects every other successor and retains the function exclusion. It does
not change the generic command walker or skip branch terminators/owners.
The paired plain/grouped test compares all normalized native mission statements,
ignoring comments/formatting, grouping markers and the intentionally different
output paths.

``src/qtgui/MissionModel.cpp``: an accepted unbracketed builtin output serialized
with brackets, causing the complete grouped event to become noneditable after
rebuild/reopen. The existing configured-GmatFunction normalization now also
accepts configured BuiltinGmatFunction objects. It still preserves callee,
ordered arguments, output identity and command label. Six changed-call variants
must remain noneditable, including a nonfunction callee.

Exact preimages and increment-only diffs are retained under
``build/nasa-coverage/coverage95-20261101/``:

* ``event-extra-core-before/SaveMission.cpp`` and ``MissionModel.cpp``;
* ``event-extra-core-SaveMission.diff``;
* ``event-extra-frontend-MissionModel.diff``.

The independent review passes both repairs and the bounded evidence capsule.
The review does not qualify unrelated current-build behavior.

Assertions and observed results
-------------------------------

The original Script Event driver and six original fixtures remain byte-identical.
The expanded driver retains the preceding command assertions, adding explicit
contracts for the new contexts. Every new context uses the actual event body
dialog and grouped Mission-tree entry, Cancel/pending Apply, idempotent source
changes, Undo/Redo and Unicode Save/Save As/reopen, with native command containment
and source/history retention after execution.

Each mode checks 216 raw report cells against the independent circular-orbit,
literal/algebraic and command-effect references, plus runtime object values.
The paired main-sequence SaveMission control supplies another twelve report
cells. The selected visible run records 168 native exposure events in the
automatically scaled, resizable, initially minimized nested desktop. There is
no fullscreen state, retained screenshot or physical host/GPU observation claim.
These are programmatic Qt interactions with native exposure; physical/native OS
mouse and keyboard input remain a separate qualification layer.

The SaveMission cases require saving when terminal or followed only by nested
EndScript markers, and no saving when followed by meaningful commands inside
or outside the event, inside a terminal If branch, or from a function. The branch
case places its report before If so the branch-terminator boundary stands alone.
Saved native mission content is retained and the plain/grouped statement lists
must match. Explicit NAIF IDs make the paired fixture deterministic.

Stop requires Stopped, explicitly rejects watchdog expiry, and checks both the
unmodified following variable and absence of the following report. Plot checks
cover removal of old samples, pen connectivity intervals and a single marker
at the correct epoch; the unselected control retains its samples and no marker.
CommandEcho is observed on then off before its two reports. DynamicDataDisplay
must retain the captured value 7 while the native variable/report reaches 99.
Write emits a unique literal string. Function outputs differ from their initial
values, and the builtin call also exercises six unsafe mapping rejections.

Failures and source-version reuse
--------------------------------

All native failures remain recorded:

* Headless-v1 establishes the real SaveMission missing-file defect, with a
  successful plain control in the same test.
* Headless-v2 exports both missions after the repair, but the complete comparison
  detects two automatically assigned NAIF IDs changing between rebuilds.
  Pinning those inputs preserves the comparison assertion; v3 passes.
* Headless-v4 and v5 establish the builtin group-reopening failure. V5 records
  the exact source/canonical bracket difference. The narrow mapper repair and
  six rejection witnesses pass in v6.

The selected unattended evidence uses v3 for the terminal SaveMission pair,
v5 for the strengthened branch and MarkPoint cases, v6 for the builtin case,
and v4 for the other fourteen contexts. The visible-v1 run passes all eighteen
with the final two repairs. Existing passing cases were not broadly replayed.
Earlier source versions are not asserted to equal the final driver/build.

``event-extra-failures.json`` records failures and review corrections.
``event-extra-evidence-audit-v1.json`` verifies 643 consumed evidence hashes,
archived exact executed sources, pre/post source and runtime identity tables,
complete logged NASA_CHECK proofs, passing exact-case JUnit/prefixes and raw
independent results. The auditor is self-contained; its own source identity
covers its validation logic. Runtime tables describe recorded images, not
observed loaded-library traces. An immutable coverage snapshot supports later
reuse without pinning a mutable current coverage file.

The prior 663-artifact batch seal still verifies after these new production
repairs. Thirty-five redundant generated files were losslessly compressed,
saving 60.19 MiB. This stage occupied about 39.8 MiB before final documentation
and sealing. No binary/kernel copies or passing screenshots were created.

Remaining scope
---------------

``event-extra-supported-inventory.json`` retains every original factory entry.
Seven loaded executable types still need grouped witnesses: FindEvents, Set,
RunSmoother, RunEstimator, RunSimulator, BeginFileThrust and EndFileThrust.
Seven explicit alias/framework/main-body applicability dispositions remain:
BeginMissionSequence, CallFunction, Create, Equation, NoOp, ScriptEvent and
PrepareMissionSequence. Entries cannot be silently discarded because their
factory names differ from executable source syntax.

Nested-event witnesses are now present. Whole FRC!E138 can close only after the
remaining executable contexts and legal-context dispositions are conjoined with
the recorded evidence. The full set of 156 remaining original requirements,
external/historical prerequisite holds, critical Brouwer model gap and GroundTrack
skip-boundary gap remain separate and unresolved. The 95% goal remains active.

Reusable replay
---------------

From the checkout root, build and watch only this new batch::

   cmake --build build/linux-gui --target GmatQtNasaScriptEventCommandTests -j 4
   python3 src/qtgui/tests/RunScriptEventCommandAcceptance.py --visible \
     --filter '^QtGui.NasaScriptEventExtra'

Restore the initially minimized GMAT test-desktop window to watch it. Omit
``--visible`` for unattended execution; narrow ``--filter`` for an affected case.
The helper defaults to 350 ms viewing pauses and no screenshots. Its default
filter now selects all 27 additional command contexts; the original six branch
tests remain registered separately in the comprehensive acceptance suite.

Verify retained evidence without running GMAT again::

   python3 src/qtgui/tests/AuditScriptEventCommandAcceptance.py \
     --verify-receipt build/nasa-coverage/coverage95-20261101/event-extra-evidence-audit-v1.json

The final source archive preserves the exact handoff helpers and both production
repairs. This is a bounded support increment, not NASA qualification or completion
of the requested 95% threshold.
