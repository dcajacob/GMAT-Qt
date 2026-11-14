MATLAB formal object alias repair, 2026-11-08
==============================================

A valid ChemicalTank call could receive a MATLAB structure at top level, but
CallMatlabFunction rejected the same operation when its destination was a GMAT
function's formal input. The new strict ObjectInOut witness reproduced that
rejection using a licensed engine before the product edit.

Initialize now accepts that destination through the existing supported-output
check. Execute retains independent result slots, typed structure/property
validation, lifetime guards, workspace cleanup and the existing native setters.
No header, layout, virtual slot, state equation or tolerance changes.

The same positive witness passes after repair in three phases: initial,
no-build repeat and Unicode save/reopen. Main/local/nested calls produce native
input masses15,30,45,8 and output masses12,33,48,11, with exact CSV/viewer,
caller isolation, source/history/file and reserved-workspace checks.
WrongAliasPayload induces a real MATLAB error. The caller tank remains12,
Main remains2.5 and Keep41; the already executed counter is1, the following
report is absent, and reserved temporary variables are cleared.

The new cases extend the retained strict I/O driver through recorded reversible
substitutions. The protected original driver and all1,334 original test inputs
are unchanged. There are36 passing current executions:22 in the explicit
headless composite and14 in the focused visible selection. Five original
failure executions remain visible, plus the one valid pre-repair defect run.
The old failures are Object's unsupported Report field route, LiteralRecovery
and LiteralClose's old null-runner diagnostic, and WrongAliasType's old formal-
object rejection diagnostic (the latter also runs visibly). They are not erased
or reclassified as passes. Strict supplementary real-error cases pass.

The source review also retains literal E241--E248 and confirms the matching
native/global mode values30/31/32. MathWorks documents engOpenSingleUse as a
Windows-only API, and the C Engine cannot attach to an existing Linux/macOS
session. The existing UI and native guards reflect that distinction. Current
single-process engine/workspace/lifecycle evidence does not qualify Windows
shared/private two-process behavior. Original FRR-36 is not defined by a
matching workbook requirement. These and all broader original gates remain open.

Primary references:
https://www.mathworks.com/help/matlab/apiref/engopensingleuse.html
https://www.mathworks.com/help/matlab/matlab_external/attaching-to-an-existing-matlab-session.html

Stage: build/nasa-coverage/coverage95-20261108-matlab-interface-modes-current-review
No new acceptance credit; canonical1078/1158 remains unchanged. Continue with
current C API configuration/stopping evidence and the remaining original scopes.
