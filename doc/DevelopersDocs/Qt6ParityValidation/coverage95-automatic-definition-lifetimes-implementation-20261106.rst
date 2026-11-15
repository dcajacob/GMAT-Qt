Automatic definition ownership and reference lifetimes, 2026-11-06
================================================================

The preceding v45 turn made source progress. Its thirty final production hashes
and two successful build receipts were current at turn start. This turn continues
implementation; the full goal remains active and incomplete, without a blocker.

Actual ownership and borrows
----------------------------

AddAutomaticObject no longer treats a temporary validator lookup hit as lifetime
ownership. New native automatic prototypes and unmanaged variants have explicit
unique owners in the existing opaque definition pool. Finding that same prototype
again does not mark it as borrowed. Current FOS/global/external variants retain
borrow semantics rather than acquiring a second deleting owner. The installed
persistent prototype is kept when validation produces another runtime instance.

Borrowed declaration slots record the actual object and its weak lifetime token.
Tokens are per slot, so reusing an address in a later registration does not make
an earlier declaration silently refer to the new object. Lookup variants have
separate weak records. Expired records are pruned before registration, lookup,
native function initialization and global-reference restoration. The legacy
sandbox/global restoration view is rebuilt from live non-invocation borrows.
No global table entries or shared global resources are deleted by this mechanism.

Before an owned FOS is deleted, manager cleanup attempts automatic-reference
retirement even if command retirement reports an earlier error. Borrowed local
slots keep their lexical keys with null values, and native GmatFunction startup
skips those placeholders; ordinary command/subscriber validation reconstructs
needed automatic Parameters in the new scope. Cold definition copies omit live
invocation-local automatic bindings too, and retain current valid global borrows.
Owned prototypes and their older retained pools keep one identity/owner across
native declaration aliases.

Passive Parameter prototype retirement
--------------------------------------

Parameter and RefData have a passive prototype-reference retirement path. It
preserves declaration type/name roles, scalar/vector/matrix/string and history
data while detaching actual owners, declared references and native family caches.
The existing native RefData cache providers participate. ODE prototypes drop the
transient-force table view without walking that borrowed table during retirement.
The Parameter and RefData entry points have distinct names to avoid ambiguous
lookup in concrete multiple-inheritance Parameter classes. Cold copies resolve
both installed-slot and live lookup-variant records without assuming every borrow
has an installed slot.

The data-family operation accepts null references only for the exact RefData
currently being retired, scoped by a restored thread-local identity. Ordinary
retained binding still rejects missing required references.

Copied owned automatic prototypes are made cold before publication. Definition
owner destruction retires their views before local declaration resources are
released. Returned value copies retain separate owners and are not replaced by
constant evaluated results. No expressions/functions/models/solvers were executed
in this implementation phase.

Full scope still open
---------------------

The interpreted function provider remains disabled. This implements the native
automatic Parameter declaration path and associated ownership/lifetime plumbing;
it does not close an all-resource or whole-function requirement. Custom non-Parameter
prototype binders, mutable external declaration tables, arbitrary cross-owner and
destination-owned aliases, solver ownership, all remaining commands/plugins and
full retained/private/global/caller/interior/recursive/exceptional/kernel/service/
engine-reset/scientific behavior remain required and unqualified. No smaller
function profile is declared complete and no original clause is waived.

Compilation and accounting
---------------------------

Stage: ``build/nasa-coverage/coverage95-20261106-implementation-automatic-definition-lifetimes``. Nine production preimages, before/
build/final source hashes, indexed patches, source map, logs/terminal receipts
and four planning preimages are retained. New source delta whitespace: zero.

* v1: 450/450 steps, 352.24 seconds, return 0.
* v2: 220/220 steps, 205.66 seconds, return 0.

Final source matches v2. Matching core, Qt, console, enabled plugins and
Python bindings were rebuilt. New nonvirtual methods and private opaque-pool metadata introduce no resource
instance fields or virtual slots. RefData helper/template consumers and enabled
plugins require the matching rebuild. External source/API compatibility remains
unqualified. External ABI/platforms,
optional debug configurations and runtime behavior remain unqualified.

No tests, GUI/mission/function/model/solver runs, numerical comparisons, acceptance
readers, seals or qualification campaigns ran. No installed/shared runtime,
package, commit, push or external messages changed. Original assertions, fixtures,
texts, tolerances, fingerprints, denominator, ledger and gates remain intact.
Tracking-v46 remains 1093/1158 = 94.39%; the recorded source-versioned qualified
baseline remains 1053/1158 = 90.9%. All 105 original acceptance rows remain open;
65 rows remain outside the implementation-present tally. No additional row or
verification credit. Full implementation is not complete.

Next
----

Complete full solver ownership and all remaining command/plugin ownership/reference/retirement providers, then passive UserDefinedFunction/GmatFunction body/prototype admission. Finish retained/interior/global/caller/scope-table/service/engine-reset/later/exceptional lifetimes, GetEphemStates destination-owned-frame alias Copy, direct RVECTOR results and every original body/kernel/NAIF/scientific/resource requirement. Automatic custom non-Parameter prototype providers, declaration-map mutations and arbitrary ownership aliases remain within original scope. Keep verification deferred until implementation is finished and research POV independently without asking the user for a dialect.
