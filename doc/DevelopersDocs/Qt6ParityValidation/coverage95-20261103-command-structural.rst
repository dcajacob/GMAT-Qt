Structural Mission Tree operations and retained starter-summary conflict
=======================================================================

Outcome
-------

Twelve new structural contexts pass unattended and visibly: 24 GUI passes,
48 twelve-field result rows, and 288 independently checked summary components.
EndFor, EndIf, EndWhile, EndTarget, EndOptimize and Else are each exercised at
original depth and with their complete enclosing branch inside another If.
The BeginMissionSequence initial-state summary probe fails in both modes and
remains an ordinary failing registration. Its two pre-failure numerical result
rows are retained separately; they are not counted as passing operation cases.

Coverage remains 1,019/1,158 (88.0%), with 139 open and 82 needed. Historical
coverage remains 1,816/2,367 (76.7%). No original requirement or denominator
changed. All 59 prerequisite holds and separate scientific, technical-core,
current-build and full-NASA gates remain. No production source changed.

What the structural tests actually prove
----------------------------------------

The actual context menu must disable Open and Delete, enable Close and Summary,
and enable insertion on both sides of the six structural branch markers.
Direct source-model Replace/Remove attempts must reject the structural node;
double-click must not open an editor. Close without an owner must be harmless.
These are explicit restrictions, not successful Open/Delete operations.

Insertion must preserve every unrelated source byte and the exact parent and
relative order. Insert Before remains in the enclosing branch. Insert After
EndFor/EndIf/EndWhile/EndTarget/EndOptimize moves to that branch's parent;
Insert After Else remains inside its If. Nested EndIf cases check the parent's
label as well as its type, distinguishing the inner If from the outer guard.
Undo/Redo and restoration must succeed after each insertion.

Each restored mission executes before and after Unicode save/reopen. Command
summaries must display the selected spacecraft and frame with six components
from the independent 20-second circular-orbit reference. Literal scalar results
are Count=3 for For/While, Count=7 for If/Else, U=4 and Quantity=9 for Target,
and U=2 and Cost=0 for Optimize. Keep remains 41. Time, position and velocity
bounds remain 1e-4 s, 2e-5 km and 2e-8 km/s. Scalar bounds are 1e-12 except the
predeclared solver bound 2e-5; summary bounds remain 2e-5. The Else fixture changes
only the retained condition from false to true so the Else command executes.

The original wx tree excludes branch-end markers from Open/Delete via its
BEGIN_NO_PANEL condition. Current Qt protections agree with those exclusions.
This source observation does not waive the exact All Others!E196 wording.

BeginMissionSequence conflict
-----------------------------

The new probe reaches the initial-state Summary assertion after its menu,
protection, insertion and first execution checks, then fails because the native
summary is exactly::

   No summary data for BeginMissionSequence command BeginMissionSequence1

BeginMissionSequence::Execute captures state, but its constructor deliberately
sets includeInSummary=false. GmatCommand::BuildCommandSummaryString honors that
suppression. The existing independent C-API acceptance client also explicitly
requires BeginMissionSequence's no-data response after RunScript. Its exact
assertion source is retained in the stage; it was inspected, not rerun.
The original wx tree hides this boundary entirely, whereas Qt exposes it as a
protected node. No production behavior or existing assertion was changed to
resolve these incompatible expectations. No skip, WILL_FAIL or inverse assertion
was introduced. Both failed native attempts remain bound to their source version.

Evidence and replay
-------------------

The stage is ``build/nasa-coverage/coverage95-20261103-command-structural``.
The v1 source review/archive and full executed-source archive preserve the exact
native assertions, references and production inputs. Before/after identities
match, and all 13 visible processes have mapped core/test image evidence.
Both selections terminated with 12 passes and one failure. Visible windows were
initially minimized, resizable and nonfullscreen; zero screenshots were taken.
No previously completed family was rerun.

The 465 regular-file bindings in ``evidence-inputs-v1.json`` exclude runtime
copies and symlinks. Audit v1 checks source/runtime identities, actual factory
census, exact JUnit/receipt/stdout/proof outcomes, numerical outputs, summaries,
fixture derivations and source edits. It retains the two failed starter outcomes
separately. Five corrupt-proof controls must be rejected: wrong state, enabled
Delete, missing reopen, wrong insertion parent and missing structural protection.
No requirement key is assigned by this bounded audit.

Build and replay from the repository root::

   cmake --build build/linux-gui --target GmatQtNasaCommandStructuralOperationTests -j 4
   python3 src/qtgui/tests/RunCommandStructuralOperations.py
   python3 src/qtgui/tests/RunCommandStructuralOperations.py --visible
   python3 src/qtgui/tests/RunCommandStructuralOperations.py --include-starter-conflict
   python3 src/qtgui/tests/AuditCommandStructuralOperations.py \
     --stage build/nasa-coverage/coverage95-20261103-command-structural \
     --output /tmp/command-structural-recheck.json

Default replay selects the twelve supported structural cases. Adding
``--include-starter-conflict`` includes the original failing summary probe and
currently returns failure. All thirteen ordinary registrations remain in CTest;
there is no claim that the complete suite is green. The new target is included
in the acceptance-build dependencies.

Full-census continuation
------------------------

``parent-operation-status-v1.json`` preserves all 53 entries. Forty-one have
bounded six-operation evidence in selected supported contexts. Six more now have
verified structural restrictions and available operations; BeginMissionSequence
has a retained summary conflict. PrepareMissionSequence, CallFunction, Create,
NoOp and EndScript still need explicit native operation/applicability evidence.
All twelve entries remain outside the full-six-operation count. The exact parent
stays open; none of these distinctions reduces its wording or denominator.
