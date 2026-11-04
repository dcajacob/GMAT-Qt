Remaining native command dispositions and preparation operations
==============================================================

Outcome and coverage
--------------------

Four new native GUI passes cover preparation restrictions/recovery and hidden
command dispositions in unattended and visible modes. They retain eight normal
numerical result rows, four prepared-state rows and 24 initial-summary components.
All 53 factory entries now have an explicit operation or native disposition.
Forty-one retain bounded full-six-operation evidence; twelve remain outside that
count. The exact All Others!E196 requirement remains open.

Coverage remains 1,019/1,158 (88.0%), with 139 open and 82 needed. Historical
coverage remains 1,816/2,367 (76.7%). All 59 prerequisite holds and separate
scientific, technical-core, current-build and full-NASA gates remain. No
production source changed and no original requirement receives new credit.

Five remaining entries
----------------------

PrepareMissionSequence has actual context Open/reuse, Close, double-click Open,
Insert After with exact source preservation and Undo/Redo, Summary and Unicode
save/reopen evidence. Two phases each require epoch=21545, Count=0, no following
report, and Cartesian state [7000,0,0,0,sqrt(398600.4415/7000),0]. Moderator
explicitly executes only PrepareMissionSequence, which captures that initial
state. Every summary component must agree with the independent initial state.
Changing only the starter to BeginMissionSequence must execute the normal
20-second circle and produce Keep=41, Count=21, U=1, Cost=Quantity=0. The original
script is restored before the second preparation/reopen phase.

Insert Before of the retained labeled OperationProbe assignment fails in
object-setup mode. Removing the starter also fails because Count=Count*7 would
then be interpreted as a setup expression before the mission boundary. Both
positive assertions remain ordinary failing registrations and fail in both
modes. A separate restriction case verifies the actual rejection status, exact
native diagnostic, unchanged source, cancelled Delete and confirmed-but-rejected
Delete. It does not substitute these rejections for successful operations.

NoOp is constructed by the native factory. The retained mission contains two
native NoOp nodes, including the engine's initial node; none appears in the
ordinary or expanded Qt snapshot or actual tree. Its unchanged mission executes
with Count=21 and the independent 20-second orbital result.

EndScript is constructed and occurs once in the native ScriptEvent graph. It is
absent from the ordinary snapshot and actual tree, but occurs once as a protected
node in the expanded diagnostic snapshot. The ordinary event group remains the
editable unit. The unchanged event mission executes with Count=3 and the same
independent orbital result. Whole-group operations already have source-versioned
evidence in the alias stage and were not replayed here.

Create is constructible, but its retained mission-local definition must fail for
the exact native rule that objects be created before the mission sequence.
CallFunction is explicitly nonconstructible in CommandFactory: it is an internal
parent token used to resolve actual function command implementations. The test
requires a null result, while requiring actual instances for NoOp/Create/EndScript.
The full factory census remains exactly the inherited 53 names in native order.
These are applicability observations, not waivers of literal six-operation scope.

Retained failures and test development
--------------------------------------

Seven failed native attempts are preserved with source versions, logs and JUnit:

* v1 full preparation operations fail Insert Before; the same unchanged driver
  and positive assertion fail visibly.
* v1 hidden dispositions read the message pane before queued Qt diagnostics were
  delivered. Adding test.events() after Build fixes that timing; the parser,
  graph, tree and numerical assertions remain unchanged. v2 and visible pass.
* v2 preparation protection initially expected the detailed parser error in the
  generic panel status. v3 checks exact generic status and the message pane's
  detailed native error separately.
* v3 then reaches and fails the unchanged positive Delete assertion. That driver
  remains registered and fails visibly at the same assertion.
* v4's new restriction case incorrectly expected an unavailable preparation
  summary. The native result and Moderator source show that Prepare itself runs.
  v5 corrects this new-test expectation to require all six independently known
  initial-state summary components. The failed v4 source and result remain.

The original full-operation driver is byte-identical from v1 to the visible run.
No production change or scientific reference/tolerance adjustment resolves these
failures. Two independent positive-operation failures remain in ordinary CTest;
there are no skip or WILL_FAIL conversions and no claim of a fully green suite.
Old failed-development test ELF files were rebuilt; their receipt hashes, exact
source versions and build/output records remain. Qualified current cases bind
unchanged executable hashes across their headless and visible receipts and all
four visible processes have mapped native core/test image evidence.

Evidence, replay and next work
------------------------------

The stage is ``build/nasa-coverage/coverage95-20261103-command-dispositions``.
``evidence-inputs-v1.json`` binds 291 regular artifacts, excluding runtime copies
and symlinks. Source-v1 through v5 reviews/archives preserve the exact development
states. Executed-source-v5 and matching before/after tables bind the visible run.
The desktop was initially minimized, resizable and nonfullscreen, with zero
screenshots. Every run is terminal.

Audit v1 checks version-specific source identities, fixture derivations, native
census, graph/tree counts, parser errors, exact source/history behavior, numerical
and summary references, and JUnit/receipt/stdout/proof agreement. Six negative
controls reject executed preparation body, incorrect transition state, missing
reopen, visible NoOp, constructible CallFunction and accepted mission-local Create.
Its qualified_keys list is empty.

Build and replay from the repository root::

   cmake --build build/linux-gui --target GmatQtNasaCommandPrepareRestrictionTests \
     GmatQtNasaCommandHiddenDispositionTests GmatQtNasaCommandPrepareOperationTests \
     GmatQtNasaCommandPrepareProtectedOperationTests -j 4
   python3 src/qtgui/tests/RunCommandDispositions.py
   python3 src/qtgui/tests/RunCommandDispositions.py --visible
   python3 src/qtgui/tests/RunCommandDispositions.py --include-positive-conflicts
   python3 src/qtgui/tests/AuditCommandDispositions.py \
     --stage build/nasa-coverage/coverage95-20261103-command-dispositions \
     --output /tmp/command-dispositions-recheck.json

Default replay selects the two current supported cases. Including positive
conflicts selects both original failing probes as well and currently exits
failing. All four registrations and acceptance-build dependencies remain.

The continuation census is ``parent-operation-status-v1.json``. It has no entries
left without a disposition, but retains all twelve entries without full-six-
operation evidence: six protected branch markers, BeginMissionSequence's summary
conflict, PrepareMissionSequence's parser restrictions, and four hidden/internal
entries. No policy exception is inferred from the census. Continue another exact
remaining scope rather than replaying completed operation families. FRR!E406's
final-solution ephemeris evidence is a candidate for a requirement-specific scope
review and new failed/stopped/nested/recovery cases; reuse existing format and
solver proofs at their recorded source versions.
