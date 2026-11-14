Native vector function boundaries and expression results, 2026-11-06
===================================================================

The prior v47 solver ownership batch made source progress; all thirteen final
hashes and both successful build receipts were current at this turn's start.
This batch adds implementation. The full goal remains active and incomplete.

Value and transfer behavior
---------------------------

FunctionManager now evaluates native RVECTOR inputs into independent column
Arrays, using the existing formal Array contract. Dynamic input replacement
still publishes only after successful evaluation and allocation. Matrix input
allocation failures now propagate explicitly.

Output capture preserves the actual vector type in a new owning RvectorWrapper,
so native row/column Array and vector destination rules survive function-local
cleanup and caller restoration. The wrapper has no resource references or child
owners. Native Clone, assignment across vector sizes and passive reference
binding preserve independent data and ordinary metadata.

Saved unnamed vector results retain their type and resize the owned matrix
storage before replacement. VectorEvaluate returns an independent native vector;
MatrixEvaluate can consume the corresponding column representation. Native
VECTOR_WT result metadata has a strict typed FunctionRunner dispatch and checks
its dimension metadata. Unsupported output metadata now throws before reading
an undefined type. Builtin VECTOR_WT formal-object carriers and output discovery
remain required; this does not declare all plugin/builtin producers supported.

MathNode has an explicit vector evaluation route; unsupported nodes throw.
MathElement reads native vector shape without evaluation and dispatches the
bound Parameter wrapper. Direct RHSEquation/EquationWrapper vector results own
their values and check reported dimensions. Scalar evaluation rejects vector
nodes. Fixed three/six-component generic bridges dispatch existing native typed
evaluators and setters, enforcing the existing native sizes. No scientific
formula, tolerance, unit or assertion changes.

ElementWrapper vector-to-matrix transfer now constructs data for matrix-valued
properties and Parameters as well as Arrays, honoring the destination row/column
shape and retaining the native setter's dimension checks. No arbitrary matrix
reshape or new scientific interpretation is introduced.

Full original scope remains
---------------------------

No whole-function, any-command, any-clone or universal scientific requirement is
closed by these paths. Native solver/resource and command/plugin providers,
interpreted body/prototype admission, private/global/caller/interior lifetimes,
recursive and exceptional restoration, custom vector metadata, builtin carriers
and discovery, full vector arithmetic and all original body/kernel/NAIF cases
remain open. No narrower profile is substituted for the original requirements.

Compilation and accounting
---------------------------

Stage: ``build/nasa-coverage/coverage95-20261106-implementation-function-vector-boundaries``. 17 production paths, including
two new wrapper files, have preimages/existence metadata, before/build/final
hashes, indexed patches, source map, build logs/receipts and planning preimages.
New delta whitespace: zero.

* v1: 59/210 logged steps, 16.64 seconds, return 1.
* v2: 153/153 logged steps, 179.42 seconds, return 0.
* v3: 88/88 logged steps, 6.69 seconds, return 0.

The initial compile caught use of a nonexistent generic Parameter GetRvector
accessor; passive shape lookup now uses the native VectorSize metadata and the
fixed three-component family. The final successful build matches all final
production hashes. Core/Qt/console/enabled plugins/Python consumers rebuild with
the new MathNode and FunctionManager virtual routes and the exported wrapper.
External binary/platform compatibility and runtime behavior remain unqualified.

Only compilation and essential source debugging ran. No tests, GUI, missions,
functions, models, solvers, numerical comparisons, acceptance readers, seals,
packages, installed/shared runtime, commits, pushes or external messages ran.
Original fixtures, assertions, tolerances, texts, fingerprints, denominator,
ledger and qualification gates remain intact.

Tracking-v48 remains 1093/1158 = 94.39%; the source-versioned qualified baseline
remains 1053/1158 = 90.9%. All 105 original acceptance rows remain open; 65 rows
remain outside the engineering implementation tally. No new row or verification
credit. The implementation pass is not complete.

Next
----

Complete full native solver value/reference providers, remaining command/plugin ownership/reference/retirement providers and passive UserDefinedFunction/GmatFunction body/prototype admission. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings, all interior/scope-table/service/engine-reset/later lifetimes, GetEphemStates destination-owned-frame alias Copy and every original body/kernel/NAIF/scientific/resource requirement. Finish native VECTOR_WT builtin formal carriers and discovery, custom vector shape providers and vector arithmetic/cross-scope/recursive/error cases without reducing original scope. Preserve verification deferral until the full implementation pass is complete. Research POV independently without asking the user for a dialect.
