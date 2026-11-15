Callable Function resources and invocation retirement, 2026-11-06
=================================================================

This turn made source progress. The previous v42 turn was also progress: fourteen
final hashes and all four terminal build receipts were current at turn start.
No full implementation or acceptance audit is claimed.

Native providers and ownership
--------------------------------

Function now has explicit provider capability/preparation/owned-object/pair
contracts. Fourteen classes located in FunctionFactory and MatlabFunction add
class-specific passive hooks and guarded completed Clone/Copy. Derived types must
supply their own complete clone/copy/provider rather than silently inheriting a
native leaf's field profile. This is an implementation inventory, not proof of
an entire runtime/factory universe or all function classes.

Function environment binding assigns actual solar-system/internal-frame refs
without native setter side effects, keeps native engine force sharing and copies
already owned force tables. ObjectManagedFunction preserves shared native global
stores; already owned maps or local/global aliases copy through the shared scope
bag. Declaration/signature/source/format/bracket data and native leaf values copy;
active FOS/input/output wrapper bindings do not checkpoint. Destination-owned
table storage survives assignment until its actual borrowed aliases can bind.
Readiness/changing-input flags return to cold native preparation.

GetEphemStates exposes its owned file frame before references bind. Frame and axes
identities register before spacecraft/output/internal aliases, using the complete
frame binder. Capturing a callable also marks its manager-owned FOS entries as
private dependencies according to native ownership flags, without owning/resuming
the FOS container. Recursive function-definition backlinks are metadata refs;
function execution is skipped by retained-value initialization. Numeric output
caches are copied as data, never evaluated as replacement constant functions.

UserDefinedFunction discloses all actual owned/retained/queued prototypes and
structural commands. Recursive owner discovery sees undisclosed-by-parent-level
axes/hardware descendants. Interpreted functions still advertise no passive
provider. Unsupported ownership/reference paths fail; no all-function clause is
waived or qualified by the native leaves implemented here.

Cache and callee lifetimes
----------------------------

Source inspection found GetEphemStates kept FOS spacecraft/frame pointers after
manager retirement. Cleanup now invokes each actual function's Finalize while
invocation data lives, and GetEphemStates cleanup releases those runtime refs and
its file-frame cache. Retiring the actual owned FOS clears the function's wrapper,
global/system/frame/force views and readiness without mutating shared tables.
A store belonging to another manager is not detached by a same-name guess.

FunctionManager destructor cleanup handles actual invocation state, with callback
errors logged without replacing an exception already unwinding. Cold definition
managers do not invoke a shared callee's cleanup. Managers record a callee lifetime
token on every binding/copy/rebind; entry/setter/getter/cleanup paths validate it,
and address reuse triggers new-definition retirement. Expired cached expression
callees detach before their objects can be inspected; native Initialize can bind
a new current-scope callee by name. This is not a claim of arbitrary global/table/
service retirement safety or concurrent destruction support.

The legacy SetFunction API now uses the same invocation transition logic and
retains its explicit naming behavior. Expression/command binding still avoids
renaming shared definitions. SystemTime/ConvertTime/Sign scalar/string/cache copies
and RotationMatrix's converter copy are complete; uninitialized pause/time/sign
cache scalars now have defaults. Clock/RNG/pause behavior is still performed only
by native execution, which was not run in this implementation phase.

Open work, including a new owning-source finding
--------------------------------------------------

RHSEquation::Finalize/MathTree::Finalize leave borrowed scope maps and wrapper
references after a function FOS ends. Their v42 deep UDF body-copy path must not
read those retired bindings. Definition binding retirement and full interpreted
providers are the next implementation work; no interpreted provider was enabled.
The direct GetEphemStates source-borrows-destination-owned-file-frame Copy case is
explicitly rejected before deletion; full retained ownership/alias support for it
remains required. Global/scope-table/service retirement, public/interior/caller
aliases, all command/provider families, scientific behavior and external ABI/
platforms remain open. No narrower completed function profile is declared.

Compilation and accounting
----------------------------

Stage: ``build/nasa-coverage/coverage95-20261106-implementation-callable-function-values``.

Thirty-nine production preimages, before/build/final hashes, indexed patches,
source map, logs/terminal receipts and planning preimages retained. New stage
source whitespace0; prior dirty whitespace remains untouched. Four integrated
builds pass:

* v1: 181/181 steps, 216.97 seconds, return 0.
* v2: 89/89 steps, 160.97 seconds, return 0.
* v3: 181/181 steps, 174.9 seconds, return 0.
* v4: 66/66 steps, 6.94 seconds, return 0.

Final source matches v4. Matching Qt/core/console/enabled plugins and Python
bindings were rebuilt, including test targets; no test binary was executed.
Four Function virtual slots and three fields (Function/ObjectManagedFunction/
FunctionManager) require matching consumers. External ABI/source/platforms and
optional debug/runtime behavior are unqualified.

No tests, GUI/mission/function/model/solver execution, numerical comparisons,
acceptance readers/seals or qualification campaigns ran. No installed/shared
runtime, package, commit, push or external messages changed. Original assertions,
fixtures, texts, tolerances, fingerprints, denominator, ledger and gates remain.
Tracking-v43 stays1093/1158=94.39%; recorded source-versioned qualified baseline
1053/1158=90.9%, all105 original acceptance rows open. No broad engineering or
verification row credit. Full implementation is not complete.

Next
------

First retire command/RHSEquation/MathTree wrapper and scope bindings when an invocation FOS ends: Finalize currently leaves borrowed maps/wrappers, and v42 deep UDF definition copying must not inspect retired FOS data. Then implement passive interpreted Function/UDF/GmatFunction compiled-body/prototype/reference providers and full command-family ownership before enabling that provider. Finish owned-frame borrow-of-destination retirement, global/scope-map/service/caller/interior/later lifetime cases, direct RVECTOR math results, and every original resource/command/body/kernel/NAIF/recursive/exceptional/scientific requirement. Preserve full scope; verification stays deferred until implementation is finished. Research POV independently; do not ask the user for a profile.
