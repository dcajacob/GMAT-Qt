Math-node main/private/Global use and guarded Copy — 2026-11-03
================================================================

Coverage remains **1,019/1,158 (88.0%)**, 139 open and **82 more needed**.
Historical operations remain 1,816/2,367 (76.7%). No exact requirement is newly
admitted. Original authorities, fingerprints, denominators, assertions,
whole-row conjunctions, 59 holds and separate qualification gates remain intact.

All 44 math spellings retained
-----------------------------

All 44 advertised math-node spellings (42 concrete classes) now pass bare
main/private/explicit-Global creation and use in both GUI modes. Aliases remain
separate entries. Each mission executes Script-style object Write, including
the actual native type/name: four Shared, three function-private PrivateOnly
and one Shadow output. A different-type, same-name Spacecraft PrivateOnly in
the caller must retain DryMass=987 and its exact configured prototype. This
detects an accidental caller lookup or a leaked private math object.

Each window checks actual function-file keyboard Cancel/Save, direct/direct/
nested calls, independent scalar outputs and caller continuation, default
active math state, actual shared-store pointer identity, exact caller/source/
history/prototype/file invariants, Unicode reopen, repeat without explicit
Build and actual report viewer. Whole-object function output is not imposed.
The new driver retains the established resource driver's assertions and
structure for these mechanisms; the original driver is unchanged.

Selected scope evidence has **88 new GUI passes**, 264 missions, 1,056 calls,
2,112 actual object Write constructor lines, 1,320 raw marker scalars and
792 actual global-store identity checks. All 264 missions preserve the
different-type caller shadow. Five unchanged original regressions pass in
both modes: Array function frames, direct recursion, mutual recursion,
QuaternionProduct built-in and NumberString built-in. The built-in catalog is
explicitly bound in the receipts. Final selection is **98 GUI passes**, with
ten affected original regression passes and unchanged assertion-source hashes.

These are default bare-node resource/scope operations and selected active-state
copy contracts. No operand graph is evaluated by the new resource cases.
Configured wrapper/child graphs, initialized FunctionRunner/manager ownership,
all-state copying, mathematical domains/formulas and science remain unqualified.
The original regressions keep their own bounded algebraic/numeric assertions.

Demonstrated seven-file repair
-----------------------------

The pilot reaches private function reset and fails for MathElement, Add, Strcat
and FunctionRunner because their inherited Copy is unimplemented. Separate
native support records the same missing Copy across 88 scalar/matrix controls,
two per advertised spelling. Exact sources and all failures remain.

MathNode.cpp/.hpp now override the inherited Copy slot with exact concrete-type
dispatch over the 42 MathFactory classes. Each path calls that class's typed
assignment; null/incompatible sources are rejected before assignment, self-copy
is unchanged and unsupported derived types must provide their own Copy instead
of being sliced. The dispatcher/alias class set is independently checked against
the complete 44-entry catalog. No factory entry is added or removed.

Dispatch alone passes two controls but fails 86: 84 lose active state/flags,
and two FunctionRunner controls encounter an unsized cached result matrix.
MathFunction and BuiltinFunctionNode assignments now call MathNode assignment;
NumericFunctionNode and StringFunctionNode call BuiltinFunctionNode assignment.
This restores inherited active state while retaining their existing own-field
and graph conventions. FunctionManager initializes the transient-force pointer
to null and copies its matrix result cache only when sized, resizing for a new
valid shape. Unused result storage is not an active-state equality assertion.

All 88 native controls then pass with independent 17.25 and 2x3 matrix literals,
copied active types/flags, destination name/global identity, source/clone
independence, self-copy, **264 atomic null/Variable/different-math rejections**
and two unsupported-derived rejections. The controls are constructed/uninitialized
nodes; they do not claim complete initialized argument/manager graph copying.

The exact seven-file preimages, dispatch-only patch and final core-v4.patch
remain. There is no new object member, virtual slot, mathematical formula or
table error-policy change. The inherited Copy behavior and parent assignments
change and need technical review. Existing wrapper/graph ownership conventions
remain a separate review obligation.

Failures and execution identity
------------------------------

Six failed CTest attempts remain: four GUI pilot failures and two native support
failures. Support v2 retains all 88 missing-Copy diagnostics; v3 retains 84 state
failures, two cache failures and its two successful MathElement subcontrols.
Those subcontrols are diagnostic results, not separate passing CTest cases.
Final support v4 passes all controls. No compile/setup-only failure or superseded
passing CTest occurs in this stage. An existing unrelated OrbitRenderer QImage
deprecation warning remains in the build log.

Headless v4 passes all 44 new scope cases and five affected regressions.
Visible v1 passes the same 49 cases with 49 actual mapped processes and unchanged
before/after source/runtime hashes. Windows are initially minimized, resizable
and nonfullscreen; zero screenshots are retained. No runtime ELF or kernel copies
are in the bundle, and process identity does not claim physical GPU/human
observation. Every build/native handle is terminal. Exact executed source
capsules preserve each version; no earlier source-limited pass is promoted.

Five corrupt-proof controls reject a wrong private type, missing repeat, wrong
global pointer, wrong Copy value and omitted Copy spelling. All prior 566 rows
remain unchanged, including their original failed observations; each math row
gains two private/Global contexts and no whole-scope qualification flag changes.

Custody, review and remaining work
---------------------------------

Authoritative stage: build/nasa-coverage/coverage95-20261103-function-math-resources/.
Use evidence-audit-v1.json, evidence-inputs-v1.json, stage-receipt-v1.json,
seal-verification-v1.json and the executed/final source capsules.
math-scope-matrix-v1.json retains the complete role table.

The current schema-2 core review inventory is
build/nasa-coverage/core-change-review/20261103-function-math-resources/:
**233 pending paths**, 220 tracked differences and 13 untracked files. Of the
previous 228 paths, 226 hashes are unchanged. MathNode.cpp and FunctionManager.cpp
have explicit increments; five other paths are newly pending. All seven stage
preimages match the preceding executed sources. The inventory is a worktree/
HEAD review aid, not exclusive authorship, release comparison or approval.

Next conjoin the complete scoping requirement with retained evidence for the
28 nominal automatic-global candidates, 321 owner-bound system parameters and
propagator/axes owner routes. Reuse these 44 math routes, the earlier 81 resource
type witnesses, user-variable/Array/String and navigation proofs. Nominal roles
do not authorize exclusions. Whole E491/E493/E497 remains open, with no duplicate
credit for already-counted children. E477/SaveMission, E406/terminal output,
earlier source/science conflicts and all 59 holds remain unresolved. Canonical
coverage is still the default-orientation coverage-snapshot-v1.json; the full
goal remains active and incomplete.

::

  cmake --build build/linux-gui --target GmatQtNasaFunctionMathResourceTests GmatMathResourceCopyTests -j4
  python3 src/qtgui/tests/RunFunctionMathResources.py --filter '^QtGui.NasaFunctionMathResourceAdd$'
  python3 src/qtgui/tests/RunFunctionMathResources.py --visible --filter '^QtGui.NasaFunctionMathResourceAdd$'
  ctest --test-dir build/linux-gui --no-tests=error -R '^QtAcceptance.MathResourceCopy$' --output-on-failure
  python3 src/qtgui/tests/AuditFunctionMathResources.py --stage build/nasa-coverage/coverage95-20261103-function-math-resources --output /tmp/function-math-resources-audit.json
  python3 src/qtgui/tests/TrackCoreChanges.py --verify build/nasa-coverage/core-change-review/20261103-function-math-resources

The runner defaults to all 44 new scope cases and accepts --filter/--output.
Both acceptance build targets include the new executables; native X, engine
locks and 180-second timeouts remain. X-socket launches require sandbox escalation.
Reuse completed versions and replay only new or affected work.
