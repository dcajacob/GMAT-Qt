Function command filename repairs and policy review, 2026-11-08
==============================================================

Two real filename defects are repaired. Twenty-four final current-source GUI
executions pass: twelve headless and twelve on the visible nested X11 desktop.
The 1334 protected original tests/inputs remain unchanged. No original assertion,
fixture or tolerance changed. No new NASA requirement credit is claimed;
canonical acceptance stays 1078/1158 (93.1%), 80 open, 23 needed for 95%.

Product repairs
---------------

Interpreter::ParseAndSetCommandName counted quote characters to distinguish a
command label from a filename. An escaped apostrophe creates four quote bytes
in one filename, so SaveMission interpreted a path ending in Host''s Delta as a
label plus a second operand. The actual native Filename became only s Delta;
the wrong host-export payload is retained as baseline-misdirected-host.script.
The parser now walks the first quoted operand, skips doubled apostrophes, and
recognizes a label only when a separate second quoted filename follows. The
existing non-file command label path is unchanged; Save's optional filename
remains in its options and retains its existing label classification.

SaveMission checked the entire path for an extension. A dot in a parent
directory therefore prevented .script from being appended to an extensionless
filename. The second diagnostic reproduced the correct parsed path but an
export without its required suffix under dot.parent. SaveMission now checks
the basename. Generic FileUtil extension rules remain unchanged.

Only production files Interpreter.cpp and SaveMission.cpp changed. No public
header, class layout, virtual ordering, solver/provider interface, interpolation
or physical equation changed. The configured private app/plugins, selected
workers and console were rebuilt. Installed runtimes/releases are untouched.

Final checks
------------

The new FunctionCommandPolicySaveMission witness retains twelve policy/context
combinations: LegacyTerminal and Immediate, each in a terminal main position,
before a report, inside a function, inside a function's loop/branch, with an
existing native label, and under a dotted parent directory. It checks Unicode
and apostrophe paths, actual host contents without private function locals,
independent function result 8/caller continuation 42, and native reopen/Build/Run
of all nine exported host scripts. Three legacy contexts suppress export as
before. A real write failure stops the mission before the next assignment,
delivers its native error, preserves the directory and permits the next mission
to Build/Run successfully. Thirteen observation records pass in each profile.

The new MatlabGuard witness tests actual GmatInterface guards with a callback
probe: ten denied callback/script-control operations while a MATLAB evaluation
scope is active, nested exceptional scope restoration, cross-thread rejection,
receipt-time denial and normal callback resumption. Eleven observation records
pass in each profile. This is a native guard unit test; no licensed MATLAB
function, external socket route or whole E479 qualification is claimed.

Ten unchanged original checks also pass in each profile: six legacy SaveMission
Script Event contexts, MissionLabelsWrite, GmatCallSyntax, ImplementationSourcePolicy
and ImplementationFunctionSyntax. SourcePolicy retains twelve native/strict
function/include filename and encoding observations. FunctionSyntax retains
four bracket/compatibility observations. All original legacy effects, source,
result, history and rejection assertions remain intact.

The new policy worker uses private native X11 headlessly; generic original
widget tests retain their registered headless routes. All selected tests pass
on the visible native desktop. No other operating system is qualified here.

Prototype history and scope limits
----------------------------------

Early new-witness compilation needed two existing pure callback methods.
Its first generic offscreen registration stopped before behavior execution;
registration was corrected to the existing native launcher. The subsequent
filename and dotted-directory failures are real raw behavior diagnoses. After
both production fixes all twelve export contexts passed, but the new error
observer read the Message Window before native queued messages were delivered.
The new witness now drains normal Qt events before observing that error; its
failure and continuation assertions are unchanged. These earlier prototypes
remain diagnostic artifacts, not passing current-source or NASA evidence.
The final complete batches pass 12/12 in both profiles.

The old protected ReviewFunctionCommandScope.py assumes an obsolete SaveMission
source spelling and unconditional function restriction. Current source already
supports explicit ExecutionPolicy=Immediate in functions. That historical reader
was neither edited nor used for current admission. Its dated blocker is corrected
by this source review and actual policy execution. Full all-command E477 scope
still requires the remaining command/type/dependency conjunction.

LegacyASCII and Native behavior remain available together; literal historical
only-ASCII/only-.gmf wording E480 remains unqualified. E479 full native-to-MATLAB
callback and instance/platform conjunctions remain open. E491 full scoping,
any-resource/any-eligible-global clauses and seven retained post-call global-store
failures remain open. Earlier archived component proofs are not a claim of a
full current-source regression after these repairs. Formal holds, source row
fingerprints, canonical ledgers and all broader gates remain unchanged.

Evidence and continuation
-------------------------

Stage: build/nasa-coverage/coverage95-20261108-function-core-current-review.
Plans/manifests preserve exact original E477/E479/E480/E491 rows. Exact production
preimages, a bounded patch, failed diagnostics/raw export, original and new raw
CTest/GUI receipts, build logs, current owner hashes and a verified source archive
are retained. Delta whitespace is clean. All process handles are terminal.

Continue autonomously with full original E491 scoping. The next plan is
build/nasa-coverage/coverage95-20261108-function-scope-lifetime-current-review/plan-v1.json.
Rebuild the actual two resource workers and reproduce the seven original failures;
trace active/post-call views and real store lifetimes before changing behavior.
Preserve actual caller identities, cold invocation/wrapper retirement, all
assertions and the complete resource/scoping matrix. No user decision is pending.
