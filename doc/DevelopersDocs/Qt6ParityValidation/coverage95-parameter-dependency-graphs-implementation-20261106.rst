Parameter dependency graphs — implementation, 2026-11-06
==========================================================

Retained function scenes now admit Parameter dependencies and bind their actual
configured references and typed cached pointers. This is the dependency layer for
stopping conditions and other resources that retain dynamic parameters. Existing
primitive function argument/output value semantics remain. No expression is
replaced by an evaluated constant.

RefData and typed caches
-------------------------

Parameter dispatches through the passive provider hook to RefData. The binder maps
the actual ordered declaration list, preserves unresolved declarations and lexical
role names, reconnects the Parameter metadata back-reference and invokes typed
cache binding. Native FindObject uses declaration role names, which can differ
from mapped caller resource names. Copying an alias does not replace those keys.
No native parameter initialization or evaluation occurs during binding.

Typed binding is implemented for eight core RefData families: OrbitData, TimeData,
AttitudeData, SpacecraftData, BurnData, EnvData, PlanetData and BplaneData. It also
covers the four plugin families found in the source: SolverData, OdeData,
ThrustSegmentData and GravData. Frame/body/owner/model caches are mapped by actual
identity. Time baselines/history and other stored numeric state retain their native
copy policy. Attitude's mapped epoch parameter ID is refreshed without evaluation.
SolverData explicitly retains its ordered solver candidates/status history.

OdeData owns an independently mapped transient table for function values. Native
runtime tables keep their borrowed policy; SetTransientForces retires the snapshot
container on a new external binding. Ephemeris reporting explicitly reconnects the
caller-live runtime force table. Force object ownership still follows the graph;
this does not qualify arbitrary global/late force lifetimes or numerical behavior.

RefData adds one appended virtual cache-binding contract; unknown providers must
implement it. OdeData adds one shared transient-container member. A consistent
core/provider rebuild is required; external ABI/source/platform behavior is not
qualified. This is not proof of opaque provider closure.

Completed native Clone and Copy
---------------------------------

All 299 simple native GmatBase Clone definitions found in the inspected Parameter
source directories route through completion after concrete construction. There are
296 new typed Copy overrides; the existing Variable/Array/StringVar Copy methods
use the same typed path. Native class assignment copies complete concrete fields,
then reconnects retained references while holding the old destination context.
Mismatched runtime types fail explicitly. These counts describe source bodies and
declarations, not a runtime factory census or complete feature qualification.

Completion also repairs RefData.mParameter when it pointed to its source object,
including native copies without a function context. Other metadata aliases keep
their existing policy. This avoids relying on a destroyed source Parameter for the
copy's own metadata. Scalar/array/String parameter properties, formulas and
scientific tolerances are unchanged.

Original scope and remaining implementation
--------------------------------------------

FRR!E452/E460 entire objects and E466 input preservation retain their original
scope. Their prior source-cell extraction/workbook hash is copied as a reference.
No resource-type exclusion, new denominator or broad parent closure is introduced.

StopCondition still borrows command-owned lhs/rhs wrappers and stop/goal/epoch
Parameters. Its retained ownership/binder is the next implementation step.
ArrayElementWrapper owns row/column wrappers and needs exact nested bindings;
its generic GetRefObject does not expose index objects by name. Dynamic expressions
and current index dependencies must remain live expressions.

Solver/ThrustSegment/body/solar-system/custom resource graphs and aliases still
need implementation. A local unsupported owner can still be rejected by the scene
gate; the cache hooks do not silently discard it. Private/global/kernel/NAIF/later
replacement scope, interior owned-root aliases, optional Variable parser/database
paths, unknown factories and every original recursive/exceptional/scientific
obligation remain open. This stage does not complete the whole-resource universe.

Compilation and status
-------------------------

All four private integrated builds passed: v1 447 steps/193.9 s, v2 447 steps/195.48 s, v3 447 steps/196.11 s, v4 77 steps/3.49 s.
V1 adds passive dependency binding; v2 completes concrete native Clone/Copy; v3
repairs completed-copy self-metadata and typed guards; v4 restores destructor
comment placement around the new binding methods. Final 98 source hashes match v4.
Sessions 9985/91408/70289/30674 are closed. V1–v3 report the existing Qt
QImage.mirrored deprecation; v4 has no warnings. No new trailing whitespace occurs
in this stage; full dirty diff checks report preexisting whitespace outside it.

Exact preimages, source hashes, indexed patches, method/data-family maps, source
notes/clause reference, logs/receipts/summary and planning preimages are under
``build/nasa-coverage/coverage95-20261106-implementation-parameter-dependency-graphs``.

No tests, GUI, mission/function/model/solver execution, numerical comparison,
acceptance reader or seal execution. Original assertions, fixtures, texts,
tolerances, fingerprints, denominator, ledger and gates remain. No installed/shared
runtime, package, commit or push changed. Tracking-v35 remains 1093/1158 (94.39%),
65 other rows; recorded source-versioned verification remains 1053/1158 (90.9%),
all 105 acceptance rows open. Goal active; finish implementation before verification.
