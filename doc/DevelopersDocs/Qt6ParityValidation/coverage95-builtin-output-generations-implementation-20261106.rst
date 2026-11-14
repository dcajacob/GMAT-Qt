Builtin output refresh and managed store lifetimes, 2026-11-06
==============================================================

The prior v51 turn made source progress. Its four final hashes and successful
76-step build receipt were current at turn start. This batch changes four
production paths and makes concrete progress. Full goal active and incomplete.

Typed output replacement
-------------------------

Matching an old output name no longer hides changed native types or dimensions.
Refresh compares scalar/Array/vector/String carriers with current formal output
metadata, preserves compatible object identity and stages every incompatible
replacement plus the complete dictionary before publishing slots. Metadata names,
lengths and shapes validate before mutation. Replaced slots retain their actual
old input references and aliases; no artificial script names or name-based
substitutes are introduced. Output bindings retire after publication.

Refresh runs during initial argument construction and before/after native
initialization on subsequent invocations. The ordinary initialization/execution
pipeline is retained. No model or function was executed to verify this behavior.
Custom initializers that mutate declarations and cache typed output pointers
still require a complete notification/rebinding contract; that original scope
is not declared closed by post-initialization refresh.

Store authority and retirement
--------------------------------

A process-lifetime registry holds weak identities for actual manager-owned map
addresses. A replacing manager retains displaced owned objects in the actual
store owner's retirement pool, even when another manager borrows the map. Global
borrows retain their ownership policy. Unregistered externally supplied maps and
their objects remain caller-owned; arbitrary external owner/lifetime recovery
still remains required. Registry records retire with managed maps and stale weak
records do not grant ownership to a new manager or reused object address.

Managers track weak identity for their current managed store. When its owner
retires, a borrower detaches that view and rebuilds instead of reusing a retired
map. Rebinding preserves previous owned stores for cleanup, and owner selection
uses the actual registered store rather than a stale ownership flag. Recursive
restoration binds the corresponding current owner. Retirement pools carry actual
object lifetime tokens, deduplicate identities and participate in the existing
whole-scope cleanup collection. Normal cleanup preserves subscriber retirement
ordering and actual callee store detachment.

Argument retirement
---------------------

ObjectManagedFunction can detach precisely the wrappers owned by a retiring
manager, including output and deletion-queue aliases of borrowed arguments.
Another manager's installed wrappers are untouched. FunctionManager clears every
input/output alias before deleting a wrapper once, and retires wrappers while
FOS dependencies remain alive. Cleanup retains its existing first-error policy.
Cold manager copies do not share invocation owners. Direct initialization marks
partially constructed invocation state for cleanup before later steps can fail.

Full original scope
--------------------

No universal function or ownership requirement is closed. Native custom output
initialization/declaration and cached-pointer protocols, legacy discovery,
externally supplied maps, concurrent/engine-reset/interior/ownership mutations,
inout aliases, all remaining solver/resource/command/plugin and interpreted
providers, scientific behavior and every original scope clause remain required.
No managed-map or fixed-native-type acceptance profile replaces the original.

Compilation and accounting
---------------------------

Stage: ``build/nasa-coverage/coverage95-20261106-implementation-builtin-output-generations``. Four production preimages, before/build/
final hashes, indexed patches, source map, logs/terminal receipts and four
planning preimages are retained. Added delta whitespace: zero.

* v1: 114/114 steps, 257.79 seconds, return 0.
* v2: 66/66 steps, 10.28 seconds, return 0.

Final source matches the final successful build. Matching configured core/Qt/
console, enabled plugins and Python consumers rebuilt. Three manager fields
(owner pool, weak current owner and managed-view flag) and new nonvirtual
retirement APIs require matching consumers; no new virtual slots. External
source/ABI/platform/runtime and optional debug behavior remain unqualified.

No tests, GUI, missions, functions, models, solvers, numerical comparisons,
acceptance readers, seals, packages, installed/shared runtime, commits, pushes
or external messages ran. Original assertions/fixtures/tolerances/texts/
fingerprints/denominator/ledger/gates remain intact. Tracking-v52 remains
1093/1158 = 94.39%; recorded source-versioned qualified baseline remains
1053/1158 = 90.9%. All 105 original acceptance rows remain open; 65 rows remain
outside the engineering tally. No new row or verification credit. Full
implementation remains incomplete.

Next
----

Complete custom builtin output-declaration/cache rebinding protocols, legacy output discovery and every custom vector metadata/arithmetic case, including all inout aliases and externally supplied map lifetimes. Complete every remaining solver/estimator/simulator and resource provider, owned plot/subscriber/interface/measurement resources, all command/plugin ownership/reference/retirement paths and passive UserDefinedFunction/GmatFunction body/prototype admission. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings, arbitrary ownership/interior/scope-table/service/engine-reset/later lifetimes, GetEphemStates destination-owned-frame alias Copy and every original body/kernel/NAIF/scientific/resource requirement. Preserve complete original scope and verification deferral. Research POV independently without asking the user for a dialect.
