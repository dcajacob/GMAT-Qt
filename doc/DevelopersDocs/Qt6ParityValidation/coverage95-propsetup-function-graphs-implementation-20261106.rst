PropSetup function graphs — implementation, 2026-11-06
======================================================

PropSetup values now retain their mapped propagation declarations, owned
propagator/model pair and inline PSM through function input, saved-caller and
output scenes. Native command clones keep the historical empty registration
policy. This implements a configured value copy, not an integrator checkpoint.
The original entire-object/scoping clauses remain incomplete.

Owned state-manager data
-------------------------

StateManager formerly shallow-copied owned ListItem pointers, allowing both
copies to delete the same entries. Assignment also cleared owned property arrays
without freeing them. Staged RAII copies now own independent metadata; assignment
and destruction retire owned lists. Native registered-object copy policy remains
empty. PSM and estimation SortVector retire previous owned maps and stage each
new item until insertion succeeds. PSM removal frees the removed property list
and retires prepared mapping/completion/STM/covariance-history data.

A passive retained PSM binder stages actual object identities, property lists,
epoch IDs and current selection, preserving numeric state and labels. It rejects
missing/type-incompatible/collapsed declarations. Prepared state maps and runtime
associations are rebuilt later by native preparation, without SetPropItem,
BuildState, solver or model execution merely to bind a value.

Propagation setup ownership and copy purpose
---------------------------------------------

Completed native Clone/typed Copy hooks reconnect PropSetup contexts after
concrete construction. Copy holds the old lease and preserves the destination
name, including a native assignment failure. Both actual owned children are
registered before recursive binding. The setup reconnects its model, engine and
inline PSM; missing required components receive a preparation diagnostic.
Component clone helpers reject borrowed source/peer identities and changed types.
Add/remove propagation declarations invalidate readiness.

Explicit function snapshots and retained value Copy restore mapped declarations.
Ordinary mission-command Clone retains an empty registered-object list. The
passive engine binder also leaves copied cached body/origin references dormant
for that command case. Numerical engines reconnect their actual owned model;
analytic EphemerisPropagator/TLE SPICEPropagator override the passive hook to map
actual runtime objects for full values and retire their borrowed PSM/state caches.
They do not open kernels or initialize a stepper during binding. Concrete native
analytic name-copy policy is preserved for dormant clones.

FunctionManager admits PropSetup roots and owned propagator dependencies, finds
their native owners, maps owned ODE forces before generic PhysicalModel handling,
seeds model reference context and clears child context leases to avoid cycles.
Initialization orders setup references after frames, spacecraft and models.
InitializeClonedResource prepares model references for parameter reporting;
PrepareInternals later rebuilds state and initializes the engine normally.

Compilation and boundaries
----------------------------

Private integrated builds: v1 failed after an include-order error; v2 passed
190 steps / 151.48 s; v3 passed 233 steps / 164.39 s. Final fourteen source hashes
match v3. The v3 terminal receipt was recovered from its completed runner; no
second build was started to replace a missing observation. The separate bounded
receipt watcher (10176) is closed. Core, configured analytic providers, estimation,
Qt and Python wrappers compile together. One appended Propagator virtual contract
requires a consistent provider rebuild; no new native fields. External ABI and
other platforms remain unqualified. Warnings and the failed receipt are retained.

Exact preimages, before/at-build/final hashes, combined/per-source patches, index,
notes, logs, receipts, summary and planning preimages are in
``build/nasa-coverage/coverage95-20261106-implementation-propsetup-function-graphs``.

No tests, GUI, missions/functions/models/solvers, numerical comparisons, acceptance
readers or seals ran. Original assertions, fixtures, tolerances, requirement texts,
fingerprints, denominator, ledger and gates remain unchanged. No installed/shared
runtime, package, commit or push changed. Tracking-v30 remains 1093/1158 (94.39%),
65 other original rows; recorded source-versioned verification remains 1053/1158
(90.9%), all 105 acceptance rows open. No broad parent credit.

Continue the source-backed named-resource family map, GroundStation transitive
contexts, standalone hardware, subscribers/events/providers, explicit copy aliases,
global activity lifetime, private bodies/kernels, deferred configuration, direct/
later owner replacement and every recursive/exceptional/body/frame/scientific
obligation. Other solver state-manager ownership internals remain unqualified.
Implementation goal remains active; verification stays deferred.
