Finite-thrust value graphs — implementation, 2026-11-06
========================================================

Copied finite-thrust components now reconnect live-source burn and configured
spacecraft dependencies through the retained function scene. Whole-model values
retain their force collection; detached ephemeris scenes use caller-live transient
context. The previous function binder incorrectly applied the reporting policy to
every retained ODE model. Broad object/scoping requirements remain incomplete.

Retained force dependencies
----------------------------

The passive FiniteThrust binder validates source burn lifetimes and explicit
spacecraft tokens before resolving them. It stages actual mapped roots, new weak
identities and declaration-role bindings before replacing the copy. Expired
explicit targets cannot turn into legacy name fallback. A burn role map preserves
formal/caller spelling separately from object GetName, replaces changed bindings
and deduplicates one burn under multiple roles. Clear/rename/getters maintain those
roles; default unnamed lookup keeps the first-selected-burn policy.

Copies retire propagated spacecraft/index lists, PSM/state pointers, derivative
fill flags and owned evaluation/Jacobian buffers. Later native PSM setup rebuilds
integration state; parameter sampling uses retained configured targets and the
earlier private Burn evaluation path. Native Clone/typed Copy hooks reconnect a
completed value, holding the previous destination context through replacement.
No maneuver starts merely because a value is copied or bound.

Whole-model and reporting policy
---------------------------------

ODE binding now has separate whole-value and reporting entry points. The retained
scene carries its public-root policy through cloning, caller rebinding and native
owner inventories. Ordinary model values retain/rebind every native force component,
including transients. An ephemeris scene alone discards frozen transient components
and continues observing the caller's live vector. The new private copy is never
inserted into that active vector merely by being cloned.

When AddForce attaches an overlapping finite-thrust force, it replaces an explicitly
owned copied component after successful insertion, avoiding duplicate engine
application when a current live force supersedes a snapshot. Overlap comparisons
are staged before insertion. Ordinary borrowed-force admission and native Formation/
multiple-spacecraft restrictions remain. This does not establish arbitrary sampling-
copy aliases or a universal global-maneuver persistence policy.

Owned model-state storage
--------------------------

PhysicalModel assignment now stages source model-state storage and releases the
destination raw/model aliases exactly once. A null source leaves both destination
aliases null. Previously a null source could leave modelState pointing at a freed
raw buffer. The new ownership paths preserve DEBUG_MEMORY allocation accounting.
This is scoped storage repair, not proof of transactional copy or every PSM/cache
contract. Native Initialize already frees/reallocates derivative/Jacobian storage;
SetDimension marks it uninitialized. Do not infer a universal derivative-buffer
fault merely from cached pointers before that initialization.

Remaining family scope
------------------------

A source-only inventory covers 81 factory files, 584 distinct literal registrations
and five nonliteral sites. It includes commands, Parameters, internal types,
preprocessor alternatives and optional/unavailable providers; these are not runtime
registration counts or completeness evidence. See factory-type-source-inventory-v1.json
and next-resource-family-scope-source-research.json. Map actual named resource roots
and their ownership contracts to input/saved-frame/output/Clone/Copy routes before
claiming the original entire-object clauses.

Next implement PropSetup/owned propagator/model/PSM retained binding while preserving
its destination-name and dormant-definition policies. Continue GroundStation,
standalone hardware, subscribers/events/providers, explicit sampling-copy aliases,
global activity lifetime, private bodies/kernels, unresolved configuration, direct
assignment/later owner changes and all recursive/exceptional/scientific scopes.
E452/E460 entire objects and E466 input preservation retain full scope.

Compilation and retained work
-------------------------------

Private integrated builds pass: v1 231 steps / 161.61 s, v2 67 steps / 3.03 s.
DEBUG_MEMORY syntax-only compilation also passes for PhysicalModel and FiniteThrust;
no linking or execution was part of that check. Final sources match v2 and both
syntax receipts; 69701/28551 closed, no live build. Six exact preimages, before/
at-build/final hashes, combined/indexed per-source patches, source notes/inventory,
logs/receipts/summary and planning preimages are under
``build/nasa-coverage/coverage95-20261106-implementation-finite-thrust-value-graphs``.
One FiniteThrust role map changes native layout; core/providers rebuilt together.
No new virtual slot was added. External ABI/cross-platform behavior remains
unqualified. Compiler warning lines, if any, are retained in summary.json.

No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals ran. Formulas, tolerances, original assertions/fixtures,
requirement texts/fingerprints, denominator, ledger and qualification gates remain.
Tracking-v29 stays 1093/1158 (94.39%), 65 other original rows; recorded source-
versioned verified baseline stays 1053/1158 (90.9%), all 105 acceptance rows open.
No installed/shared runtime, package, commit or push changed. Goal active;
implementation incomplete. No broad parent row credited.
