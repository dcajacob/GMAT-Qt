Function output syntax provenance, 2026-11-05
==============================================

FRR!E470 requires output brackets even for a single result. Its trailing
``WHAT ABOUT ZERO OUTPUTS`` is an editorial question; FRR!E465 separately and
explicitly permits calls without outputs. The defined positive-output rule is
implemented through RequireOutputBrackets, while the default false setting
retains native shorthand under the user's direction to support both behaviors.
No additional empty-output-list spelling or rule is invented.

Parser repair
---------------

CreateAssignmentCommand converted a whole-RHS function call into a CallFunction
by adding brackets around its output before assembly. That could make authored
single-output shorthand appear bracketed to the strict setting. It also recorded
the synthesized syntax in the command's cached mOutputBracketSyntax flag.

The conversion now retains the authored output text for a declared GmatFunction.
Assembly therefore applies RequireOutputBrackets to the actual syntax, and the
cached command retains the same provenance for its initialization check. Existing
CallFunction copies/assignments retain that flag. A whole-RHS FunctionRunner that
remains an Assignment has its existing validation/initialization checks. Compound
expressions retain native expression syntax. Optional command labels remain
outside the output text.

Built-in, MATLAB and Python conversion retains its existing canonical brackets.
In particular, built-in conversion has its own bracket handling, so removing the
conversion brackets indiscriminately would move a labeled call's name into its
output list. The final repair is limited to declared GmatFunction calls. Normal
CallFunction serialization continues to write canonical output brackets.

Zero-input calls retain their existing bare/empty-parenthesis routes. Calls that
assign no outputs retain their existing syntax, such as MyFunction(input) or
MyFunction(), with native argument-count validation. A no-output call has no
caller output wrappers to assign. This is distinct from FunctionRunner's one
blank output slot used to capture an expression result.

Full original I/O scope
------------------------

source-inventory-v1.json records original FRR!E450-E471 texts/fingerprints and
owning routes for all 22 rows. Scalar, String, Array, numeric/string literal,
property, input/output count, formal-input creation and local/global-output
resolution routes are recorded. The I/O parent E450 remains partial:

* Whole resources receive root clones; private/transitive dependencies are not
  universally isolated or transferred with returned roots.
* GroundStation inputs remain borrowed in the current FunctionManager path.
* Returned ephemerides bind direct formal dependencies to caller outputs/inputs;
  this does not own arbitrary private dependency graphs.
* Function-definition assignment/rebuild and compiled-state lifetimes require
  additional work.

Subscriber availability and existing cleanup routes are recorded without
declaring the separate subscriber/scoping requirements complete. This source
inventory adds only E470 to engineering presence, not the broader I/O parent.

Compilation and tracking
--------------------------

The private integrated build v1 passes 66 steps / 10.97 seconds (session 18288
completed). After retaining the other provider conversion rules, v2 passes
66 steps / 10.40 seconds (session 35921 completed). No live sessions. Final
source hashes match at-build-v2; XML syntax was also checked during editing.
No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals were run. Linked probe/test clients were not run.

Stage: build/nasa-coverage/coverage95-20261105-implementation-function-bracket-provenance.
Two exact preimages, before/at-build/final hashes, patches, source inventory,
logs/receipts and summary-v1 are retained. The older function-output-brackets
stage already existed and remains intact. Artifact orchestration errors were
corrected before launching builds; both compilations passed.

Tracking-v10 is 1092/1158 (94.30%), with 66 other original rows partial,
unclassified or dependent. It includes 39 added engineering rows: 33 new/repair/
compatibility and six existing. Recorded verified acceptance stays 1053/1158
(90.9%); all 105 original acceptance rows remain open. No original assertion,
fixture, tolerance, text/fingerprint, denominator, ledger or gate changed.
Parser/cache/runtime and GUI qualification remain deferred until implementation
of the remaining worklist is complete.
