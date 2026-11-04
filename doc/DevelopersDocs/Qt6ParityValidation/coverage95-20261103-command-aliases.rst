Assignment aliases and ScriptEvent Mission Tree operations
=========================================================

Outcome and scope
-----------------

Eight native GUI passes cover four new cases in unattended and visible modes:
AssignmentAliases and ScriptEvent, each at the mission root and within If.
Both executions in every case retain independent numerical checks: 16 twelve-field
result rows and 96 command-summary Cartesian components. No production file changed.

The exact All Others!E196 parent remains open. Coverage is unchanged at
1,019/1,158 (88.0%), with 139 open and 82 needed. Historical coverage remains
1,816/2,367 (76.7%). All 59 prerequisite holds and the separate scientific,
technical-core, current-build and full-NASA gates remain unchanged.

Direct evidence, not an alias assumption
---------------------------------------

The new driver explicitly calls CommandFactory::CreateCommand for GMAT,
Assignment and Equation and requires native type GMAT for each. It separately
constructs BeginScript and requires that ScriptEvent has no distinct factory
implementation. It activates the real editor's Assignment or Script event
combo-box entry, verifies the complete emitted source, and proves that the draft
does not change the mission. ScriptEvent is a menu-level name for a BeginScript /
EndScript group; it is not treated as a separate executable script keyword.

The selected actual tree node then undergoes context Open and owner reuse,
Close, double-click Open, insertion before/after with exact source-byte retention,
parent/order checks, confirmed and cancelled Delete, Undo/Redo, context Summary,
and Unicode save/reopen. The ScriptEvent case also requires its closing boundary,
absence of interior AssignmentProbe as a separate tree entry, and deletion of the
entire event while retaining the following Report. Summary selects the event's
matching EndScript state. The original control-operation driver is unchanged.

Each restored mission independently checks a two-body circle with mu=398600.4415,
radius=7000 and total duration=20 seconds; Keep=41, Count=2*U+1=3, U=1,
Cost=Quantity=0. Time, position, velocity and scalar bounds remain respectively
1e-4 s, 2e-5 km, 2e-8 km/s and 1e-12. Summary bounds remain 2e-5.
No new general numerical or scientific qualification follows.

Evidence and reusable replay
----------------------------

The stage directory is ``build/nasa-coverage/coverage95-20261103-command-aliases``.
Source-v1 review and archives bind the original native assertions and references;
the executed-source archive includes all receipt sources. Before/after runtime
identities agree. All four visible processes have mapped native image evidence.
The desktop was initially minimized, resizable and nonfullscreen, with zero
screenshots. Both selections terminated successfully. No older family was rerun.

``evidence-inputs-v1.json`` binds 199 regular artifacts, excluding runtime copies
and symlinks. ``evidence-audit-v1.json`` checks receipt/JUnit/stdout/raw-proof
agreement, native factory census, aliases/templates, literal fixture derivation,
independent numerical values, source edits, exact sources and mapped libraries.
Six corrupt-proof controls must fail: wrong state, missing Close, missing reopen,
wrong parent, wrong alias and wrong template. ``audit-preflight-no-manifest.log``
retains an initial audit invocation made before its manifest existed; correcting
the staging script's integer census-field handling required no native replay or
assertion change. All four new native cases passed on their first execution.

Build and replay from the repository root::

   cmake --build build/linux-gui --target GmatQtNasaCommandAliasOperationTests -j 4
   python3 src/qtgui/tests/RunCommandAliasOperations.py
   python3 src/qtgui/tests/RunCommandAliasOperations.py --visible
   python3 src/qtgui/tests/AuditCommandAliasOperations.py \
     --stage build/nasa-coverage/coverage95-20261103-command-aliases \
     --output /tmp/command-alias-recheck.json

The four registrations are
``QtGui.NasaCommandAliasOperations{AssignmentAliases,ScriptEvent}{Original,OuterIf}``.
The target is a dependency of the existing acceptance-build targets. A fresh replay
captures current sources and has its own output directory; it does not overwrite
or replace this source-versioned evidence.

Remaining full-parent work
--------------------------

``parent-operation-status-v1.json`` retains every one of the original 53 factory
entries. Forty-one now have bounded six-operation evidence in selected supported
contexts, including directly verified alias mapping; twelve remain:
BeginMissionSequence, PrepareMissionSequence, CallFunction, Create, NoOp, Else,
EndFor, EndIf, EndOptimize, EndTarget, EndWhile and EndScript.

``structural-source-notes-v1.json`` records original wx Open/Delete exclusions,
hidden commands and Qt structural protections. Those observations are explicitly
not applicability waivers. Next exercise the actual structural/starter menus,
source insertion parent/order and summaries, with explicit source/native evidence
for non-tree entries. Preserve any exact-requirement conflict rather than turning
a missing menu operation into an assumed pass. The five rejected direct-If solver
placements and prior failures remain in their original stages and registrations.
