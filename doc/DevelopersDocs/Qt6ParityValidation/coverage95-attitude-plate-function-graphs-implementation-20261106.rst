Attitude and Plate function graphs — implementation, 2026-11-06
================================================================

Eight concrete attitude types and Plate now enter retained function input,
saved-caller, output and native Clone/Copy contexts. The actual creation routes
support named attitude prototypes as well as spacecraft-owned attitudes, and
Plate has a managed creation route. Factory literals do not prove runtime
availability or whole-resource completeness; SpiceAttitude is conditional.

Original scope retained
------------------------

The original workbook FRR!E452/E460 specifies entire objects and E466 preserves
non-global inputs on return unless they are outputs. The exact D/E cells for
rows 450–470 and workbook hash are retained in original-function-io-source-clauses.json.
This is requirement-source reading, not an acceptance audit. No type exclusions,
changed denominator or alternate success criterion is introduced. Full I/O and
scoping parents remain open.

Attitude references and intrinsic state
----------------------------------------

The passive typed binder maps actual owner/frame/body references and declaration
names. Ownerless prototypes retain their definition. Bound SPICE attitudes update
owner identity metadata. Concrete Clone/typed Copy reconnect completed contexts;
Copy holds the previous destination lease. Null owner setting and ownerless frame
association no longer dereference a missing spacecraft.

Retained binding preserves source attitude epoch, cache time and readiness after
native owner construction. An already prepared value bypasses subclass reinitialization:
CSFixed/Spinner initialization can overwrite an existing intrinsic basis. Native
spacecraft initialization keeps its definition path; retained spacecraft initialization
uses the typed helper and honors failure. This is scoped value-state preservation,
not scientific proof of arbitrary attitude/dynamics continuation or a new drift-rate
model. Existing orientation/rate formulas and tolerances remain.

AEM/SPICE reader constructors and assignment independently stage/check owned copies
before replacing destination storage. Same-file CCSDSEM SetFile retains parsed
segments; its Initialize and the NPlate Initialize methods return void and report
errors by exception. Native parser/cache/kernel policies remain. No reader/model
execution was performed as part of this implementation phase.

Plate history and frame scope
-------------------------------

GmatBase.GetConfiguredObject actually calls Moderator.GetInternalObject in default
Sandbox 1. Plate's old history-normal queries therefore bypassed function-private
maps. ObjectInitializer now binds the parsed header's frame in its actual local/
global maps. Retained binding preserves that exact reference before name lookup,
and keeps the header role separate from the mapped resource name. Native history
and inertial refs carry weak lifetime identities and are validated before use/copy.
Missing/expired bindings fail explicitly. Scoped private children retain their
binding-required policy after their strong context lease clears, rather than
falling back silently to Sandbox 1.

Plate's owned reader is staged through native copy/initialization. File/type changes
retire its obsolete reader. Copy preparation captures owned history/header metadata
without normalizing numeric inputs or mutating copied readiness. Unset-file prototypes
stay copyable; normal evaluation performs numeric/covariance initialization. Existing
prepared reader data is retained. Normal query entry points prepare an invalidated
definition as needed. Retained spacecraft binding keeps the mapped plate frame,
replacing the old post-binding cache clear.

This does not complete dynamic newly declared private frame roles after a later
file change. They require current-scope rebinding; a missing scoped binding fails
instead of selecting an unrelated Sandbox object. Cold name-only preparation can
still use the legacy default-Sandbox path if no private source match exists.
Cross-Sandbox, file-role export aliases, opaque providers and late owner changes
remain open. Weak identity checks are not a concurrent deletion ownership lease.

Compilation and remaining work
--------------------------------

Private integrated compilation: v1 179 steps /31.58 s fail; v2 226 steps /152.21 s pass; v3 144 steps /150.76 s pass; v4 144 steps /152.24 s pass; v5 144 steps /150.46 s pass; v6 67 steps /7.28 s pass.
V1's failed void-return guard is retained; v2 corrected it. V3 added frame lifetime
checks, v4 scoped fallback policy, v5 definition-preserving reader preparation,
and v6 prepared attitude-state preservation. Final sixteen source hashes match v6;
58400/19654/68552/51091/79969/6632 closed. One new Attitude virtual contract and five
Plate data members require a consistent provider rebuild. External ABI/source/
platform behavior is unqualified. Exact preimages, hashes, indexed patches, source
notes/map/clauses, logs/receipts/summary and planning preimages are under
``build/nasa-coverage/coverage95-20261106-implementation-attitude-plate-function-graphs``.

Continue full named-resource creation/ownership mapping, solver/subscriber/event/
interface/data/filter/measurement/stop-condition/function/body/solar-system/custom
providers, interior owned-root/owner aliases, globals/private bodies/kernel pools/
NAIF policies, unresolved configuration, direct/later replacement and every original
recursive/exceptional/body/frame/scientific requirement. No parent closure claimed.

No tests, GUI, mission/function/model/solver execution, numerical comparison,
acceptance readers or seals ran. Original assertions, fixtures, texts, fingerprints,
denominator, ledger and gates remain. No installed/shared runtime, package, commit
or push changed. Tracking-v33 stays 1093/1158 (94.39%), 65 other original rows;
recorded source-versioned verification stays 1053/1158 (90.9%), all 105 acceptance
rows open. No broad parent credit. Goal active; implementation incomplete and
verification remains deferred.
