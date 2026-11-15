Function declaration, environment and native-cache invocation state, 2026-11-06
=============================================================================

The previous v54 batch made concrete source progress in two production paths;
its final hashes and two successful terminal build receipts were current at
this turn's start. This batch changes 20 production paths. Full goal active,
incomplete; no blocker or universal-function closure is claimed.

Layered invocation state
------------------------

Function tracks active invocation depth independently of an output callback.
Nested entry during preparation or execution captures its existing declaration
and environment before installing inner services. Copies start at zero depth;
assignment does not copy an active source depth. A polymorphic suspension route
allows native and plugin layers to supply additional state. Missing custom
suspension state fails explicitly. Saved fields stage before publication.

The state object restores once against the actual owner's lifetime token,
remembers a failed result on repeated restoration, and attempts enclosing
restoration after an inner-layer failure. Native field restoration uses a
statically checked nonthrowing swap helper; Rvector6 swaps its six stored
components without numerical evaluation or resizing. Inner native storage
retires while its inner environment still lives. Unexpected custom restoration
errors are reported; ordinary return turns restoration failure into a function
error, and exception unwinding preserves the original failure.

Declarations and services
-------------------------

Function captures call/path/name/source-format/bracket data, input/output names,
output type/shape arrays, solar system, internal frame, force service, readiness,
error and notification flags, and retained environment storage. Nested output
preparation gets an independent notification cycle. The existing scoped output
handler remains active through failure cleanup and restores before the saved
function fields. First-level calls retain their completed metadata normally.

ObjectManagedFunction captures its changing-input flag and retained table owner.
Its FOS/GOS views restore after the base readiness flag, with predicates for
actual registered manager owners. Unregistered maps keep their caller-owned
lifetime contract. An expired/replaced managed owner leaves the corresponding
view null and the function uninitialized, and reports restoration failure;
address reuse cannot grant the old view authority over a new registration.

Managed map registrations now also carry their own generation token. Views
validate both that registration and its actual owner, including reuse of a map
address while a shared retirement pool is still alive. FunctionManager stores
and clears its weak registration identity, detaches retired borrowed maps and
callee views, and checks store authority before builtin output refresh. A
retired output store during declaration reports an error.

The native DeleteObjectMap helper now retires command/callee bindings and
unregisters every deleted managed dictionary. Displaced owned entries remain
recorded in their actual owner's retirement pool, preserving aliases and the
existing subscriber-first cleanup/deduplication rather than losing their owner
when an old dictionary is removed. Unregistered external maps retain their
original ownership contract. Full exceptional/concurrent/interior and shared
resource graph lifetime coverage remains required.

Native cache fields
-------------------

Seven stateful leaves explicitly capture their fields: ConvertTime formats,
values and converter; GetEphemStates spacecraft/frame references, file metadata,
epoch/state values and frame owners; Pause values; RotationMatrix converter;
Sign value; SetSeed value; SystemTime values and formats. Saved ephemeris owners
keep the enclosing file frame alive across inner replacement and cleanup.
Other known stateless native leaves inherit the common layers. No clock, RNG,
sleep, model or engine is executed by suspension/restoration, and native global
side effects are not rolled back by copying their service pointers.

The unchanged source worksheet explicitly requires entire objects as function
inputs/outputs (FRR!E452/E460), nested/iterative/recursive calls and any supported
command (E474:E477). Those original scopes remain required. The common metadata,
service and cache work in this batch is incremental toward those requirements;
wrapper and full manager/interpreted resource frames still remain to implement.
The manager header's additional preimage was captured before its first edit,
after build v3; final build v4 covers the complete 20-path delta.

Remaining complete requirement scope
------------------------------------

This batch does not capture/detach argument or owned output wrapper maps, whole
FunctionManager input/result/stack resources, interpreted FCS/prototype/command
runtime frames, or arbitrary custom subclass state. Those are required for full
same-manager/shared-function/recursive/exceptional behavior. Native cache state
is one necessary part, not proof of whole function reentry. Unregistered external
maps, concurrent destruction, engine reset, arbitrary interior ownership and all
other original resource/scope/scientific requirements remain unqualified.

Compilation and accounting
--------------------------

Stage: ``build/nasa-coverage/coverage95-20261106-implementation-function-invocation-state``.

* v1: 76/181 build steps, 18.48 seconds, return 1.
* v2: 181/181 build steps, 170.91 seconds, return 0.
* v3: 181/181 build steps, 169.08 seconds, return 0.
* v4: 89/89 build steps, 145.2 seconds, return 0.

Build v1 exposed a Qt slots-macro collision in a helper identifier; the name was corrected before v2. Final production hashes match build v4. Production/planning preimages, before/build/final hashes, indexed patches, source map, logs/terminal receipts and summary are retained. Every build process terminated. Added delta whitespace: zero. Matching configured core/Qt/console, enabled plugins and Python consumers rebuilt. One Function depth field, one FunctionManager registration-identity field and one virtual suspension route require matching consumers; external ABI/platform/runtime behavior remains unqualified.

The preceding batch narrative tracking label is corrected to its already-existing v54 source; no count or requirement change. No tests, GUI, missions, functions, models, solvers, numerical comparisons, acceptance readers or seals ran. No packages, installed/shared runtime, commits, pushes or external messages. Original assertions/fixtures/tolerances/texts/fingerprints/denominator/ledger/gates remain intact.

Tracking-v55 retains 1093/1158 = 94.39% implementation present, 65 other rows; recorded source-versioned qualified baseline 1053/1158 = 90.9%, all 105 original acceptance rows open. No additional row or verification credit; full implementation incomplete.

Next
----

Complete function argument/output wrapper ownership and each manager input/result/call-stack frame across same-manager and shared-function reentry, including exceptional restoration, output-name/cache transactions, legacy discovery, inout aliases, external maps and custom vector type/arithmetic. Complete interpreted FCS/prototype/command runtime frames and passive UserDefinedFunction/GmatFunction providers, every remaining solver/estimator/simulator/resource provider and owned plot/subscriber/interface/measurement resources. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional binding and arbitrary ownership/interior/scope-table/service/engine-reset/later lifetimes, GetEphemStates aliases outside direct fields/retained contexts, all original body/kernel/NAIF/scientific/resource requirements and independent POV/rotation research. Preserve the full worklist and verification deferral.
