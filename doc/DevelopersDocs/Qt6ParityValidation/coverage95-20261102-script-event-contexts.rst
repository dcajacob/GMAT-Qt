Script Event aliases and restricted contexts
===========================================

One new execution context and three rejection contexts pass unattended and
visibly, with 14 native exposure events. NoOp matches the paired plain mission
and independent orbital/scalar references. Duplicate BeginMissionSequence,
duplicate PrepareMissionSequence and ordinary mission-body Create all reject
in both plain and grouped source for their specific native rule.

No production source changed: 293 recorded production files remain identical.
Existing successful tests were reused at their recorded versions. Original
coverage remains **1,002/1,158 (86.5%)**, with **99** whole requirements needed
for 95%; historical coverage remains **1,816/2,367 (76.7%)**. FRC!E138 remains
unqualified, with its original fingerprint and full obligation intact.

Execution and rejection evidence
-------------------------------

NoOp uses the actual event body dialog, grouped Mission-tree entry, Cancel,
pending/idempotent Apply, Undo/Redo, Unicode Save/Save As and reopening. Its
native command graph must contain exactly one NoOp inside event markers.
Both plain and grouped missions must execute two ten-second circular arcs,
retain Keep=41 and produce Count=21 from 3 multiplied by 7. Twelve report cells
in each mission are checked independently, and both reports must agree.

The two initial starter fixtures incorrectly expected duplicate mission starters
to execute. Their failures are retained in headless-v1. Native
``Interpreter::FinalPass`` explicitly rejects more than one sequence starter.
The new diagnostic tests therefore assert that exact rejection in both main and
grouped source. This corrects newly introduced invalid test expectations; it
does not disable or relax any preceding acceptance assertion. These are
rejection tests, not evidence that a legal unique starter works inside an event.

Create similarly requires the exact native diagnostic that objects must be
created before the mission sequence begins. The generic interpreter guard
excludes ordinary command mode and permits function mode. Existing grouped
GmatFunction execution retains the legal function-local ``Create Variable out``
and checks the resulting function output. Other resource/setup requirements
remain independently open where previously recorded.

Every rejection checks unchanged source/history and no following output. The
visible runs use automatically scaled, initially minimized, resizable nested
windows, without fullscreen or screenshots. Physical host input/compositor/GPU
qualification is not claimed.

Inventory decisions
-------------------

``event-marker-alias-dispositions.json`` records four explicit decisions and
binds source identities plus recorded concrete evidence:

* Equation is the GUI alias for GMAT/Assignment; factory construction returns
  Assignment. The NoOp pair's contained GMAT arithmetic has checked results.
* ScriptEvent is the GUI alias for BeginScript. Existing nested-event evidence
  checks both native marker pairs and the resulting Count=12.
* CallFunction is the abstract dispatch name. Direct factory construction is
  deliberately unavailable. Existing concrete builtin, GMAT and Python calls
  retain their independent grouped effect checks. MATLAB and unloaded plugins
  receive no qualification from this decision.
* Create is forbidden in ordinary main command mode; both new negative cases
  establish that boundary. Legal function-local creation is exercised through
  the existing GmatFunction evidence.

The recorded runtime inventory now has **47 bounded witnesses and four explicit
alias/restriction decisions among 53 entries**. These counts are not NASA
requirement percentages. Two concrete legal-context questions remain:
BeginMissionSequence as a unique starter, and PrepareMissionSequence as a unique
starter with its special sandbox-preparation behavior. The duplicate rejection
tests cannot answer either question. Moderator currently selects preparation
mode using the first physical command; wrapping a root starter may therefore
require a distinct parity check. No exclusion or core repair is inferred yet.

Provenance and retention
-----------------------

``event-marker-evidence-audit-v2.json`` binds eight selected outcomes and 151
consumed hashes. It checks full process proofs, passing JUnit, exact source
capsules, unchanged runtime identity maps and three successful builds with 149
recovered dependency files each. Build-v1 links at Ninja step [3/4] following a
CMake regeneration; its process exit is zero. No final-counter claim is made.

NoOp uses headless/visible-v1, duplicate-starter rejection uses v2, and Create
uses v3. No previously passing case was replayed for the added Create branch.
The final test driver/helper is archived separately from the earlier executed
versions. Recorded runtime identity is not loaded-library observation.

The initial posthoc audit used incorrect visible-display metadata field names;
the checker was corrected without native replay. Audit-v1 was then superseded
because its build2 receipt referenced the mutable current-build pointer. The
final audit instead consumes an immutable copy of the identical original guard.
Build3 uses an immutable original guard as well. The Create fixture was absent
from the aggregate manifest reference list; the final checker binds its actual
registered argument, complete unchanged pre/post source table, native-saved exact
case bytes and process-emitted fixture hash. This distinction remains explicit.

Thirty redundant generated summaries were losslessly compressed, saving about
51.5 MiB. Stage allocation before its final source seal was 21.1 MiB, with no
passing screenshots, runtime binaries or kernel copies. Initial failures and
all consumed evidence remain. The preceding backend seal remains unchanged.

Replay and next work
-------------------

Build and watch these four contexts only::

   cmake --build build/linux-gui --target GmatQtNasaScriptEventCommandTests -j 4
   python3 src/qtgui/tests/RunScriptEventCommandAcceptance.py --visible \
     --filter '^QtGui.NasaScriptEventMarker'

Restore the initially minimized test desktop to watch. Omit ``--visible`` for
automation. The helper's default filter now includes all 35 additional command
contexts; the six original branch tests remain separately registered. A passing
rejection context means the expected invalid input was rejected.

Verify evidence without running GMAT::

   python3 src/qtgui/tests/AuditScriptEventCommandAcceptance.py \
     --verify-receipt build/nasa-coverage/coverage95-20261101/event-marker-evidence-audit-v2.json

Next, test unique-starter placement and actual preparation semantics before any
whole-row admission. All other original requirement gaps and qualification
holds remain separate. The 95% goal is active.
