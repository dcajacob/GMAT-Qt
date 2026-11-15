MATLAB call arguments and function cross-reference review
========================================================

The current MATLAB command now accepts numeric/string literals and whole native
objects through its existing typed transport. Signed literal calls previously
entered the native ObjectManagedFunction expression path. Literal names also
required nonexistent native objects during MATLAB initialization. The interpreter
now recognizes a complete MATLAB call, and the command uses private per-call
workspace bindings for constants. String literals are decoded and transferred as
values. Input/result temporary cleanup precedes native output setter execution.
Trailing arithmetic still takes the expression route. No root instance layout,
virtual ordering, equations, tolerances or original acceptance inputs changed.

The existing restriction on whole-object formal input/output aliases remains;
its diagnostic now includes the actual native type. Direct native object fields
outside system Parameters remain separate scope. These are not universal object
or property support claims.

Current evidence
----------------

The unchanged original 16-case MATLAB I/O baseline was 11 PASS / 5 FAIL. After
repair it is 13 PASS / 3 FAIL. The retained failures are the original Object
Report of ChemicalTank fields, which the script/Report language rejects, and
LiteralRecovery/LiteralClose, whose assertions require the obsolete null-runner
rejection even though literal calls now succeed. None is counted as a pass.

A supplementary generated driver retains the original native object, input
isolation, output, report, repeat, Unicode reopen, source/history and external
file assertions. Its object report uses a separate MATLAB field reader because
native script expressions cannot read ChemicalTank.FuelMass. The first attempted
observer used native scalar assignments and failed for the same reason; both
witness versions and baselines are retained. The corrected baseline has no such
observer error and fails MATLAB argument admission, as expected.

The supplementary StringLiteral uses an encoded apostrophe/Unicode/punctuation
constant in main, local and nested calls. Its recovery/close cases induce a real
MATLAB error, require unchanged caller output/sentinel/counter, and assert no
reserved transport workspace variables remain. Successful runs check cleanup
as well. Every substitution is recorded; reversing them exactly recovers the
protected driver. Original checks, assertions and tolerances remain unchanged.

The final offscreen selection has 23 PASS / 3 retained original FAIL: all original
16 I/O checks, four supplementary cases and six original destination regressions.
The final visible selection has 12 PASS: all four supplementary cases, seven
original typed/literal/type-rejection cases and the original dynamic-array alias
regression. These are 35 passing executions and three retained original failures,
not 35 newly qualified requirements. All builds and test handles are terminal.
All 1,334 protected original inputs match their baseline. Forty-four current
owning/reference sources and the generated-driver receipt are archived.

Requirements and remaining work
-------------------------------

FRC E160 names MATLAB but points to FRR37 (GMAT Functions). FRC E161 names GMAT
but points to FRR38 (Parameters and Calculations). All Others E241 references
FRR36, absent from the current workbook inventory. Exact texts, fingerprints,
all 51 positional FRR37 children and all 88 positional FRR38 children remain in
the immutable plan. The native function/callback/scope and parameter-family
components remain evidence routes, not a complete current conjunction proof.

Canonical acceptance remains 1,078/1,158 (93.1%): 80 open, 23 more required for
95%. This component adds zero requirement credit. Full resource/property graphs,
all command/global/recursive/service lifetimes, any-body/frame/domain science,
MATLAB formal process-mode/platform gates and cross-reference definitions remain
open. The six formal MATLAB holds are unchanged. No admission reader reran.

Artifacts are under build/nasa-coverage/coverage95-20261108-function-cross-reference-current-review.
Next: the already-created Units wording review. The literal non-dimensional
wording is preserved. Current guide/source review confirms missing dimensionless
thruster direction/duty/scale/efficiency labels; implement and verify that finite
GUI correction, then continue automatically through the remaining groups.
