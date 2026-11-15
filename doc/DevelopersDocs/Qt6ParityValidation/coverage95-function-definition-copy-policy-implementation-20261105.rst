Function definition copy policy, 2026-11-05
============================================

GmatFunction assignment previously called Function assignment directly. It could
copy new file/policy metadata into a destination while retaining its old compiled
command sequence and invocation state. It also discarded unusedGlobalObjectList
without deleting it. UserDefinedFunction assignment instead detached fcs and
copied readiness flags, without a complete prototype/runtime transition.

Existing contract and repair
-----------------------------

The current owning source already requires rebuilding before changing an
interpreted FunctionPath or SourceFormat. Function inherits the default blocked
command-mode property/whole-object assignment rules. The shipped architectural
Functions.tex describes FCS parsing during Sandbox setup and idempotent reuse
within a run. This repair applies those existing boundaries to copy entry points;
it does not introduce an in-run function hot-swap or runtime compiler.

UserDefinedFunction assignment now checks an existing compiled FCS before any
mutation. Equal native type, path, function name, source format and output-bracket
policy preserve the destination's graph, compiled interface, wrappers, argument
stores and current invocation. Derived input/output interface caches are not
copied over that compiled interface. A different definition raises a rebuild
error before changing those fields or retiring an active graph.

An uncompiled destination releases owned outputs and old automatic/template
objects, clears borrowed reference maps, and then receives definition metadata
with fresh execution/readiness flags and null runtime context. Source-owned
prototypes and argument stores are not borrowed. Existing ownership checks retain
shared Sandbox/SolarSystem objects. GmatFunction now delegates through this layer
and releases obsolete unused-global cache storage on the fresh path. Its Copy
entry point rejects null/non-GmatFunction values before assignment.

SetFunctionControlSequence accepts initial ownership or the same pointer again.
A different/null replacement of an existing interpreted sequence fails without
discarding its owned graph. Initial installation resets readiness flags. Native
Sandbox setup only installs missing FCS, and Qt's detached validation uses fresh
resources/path-visit guards. The owning-source RunMission trace still clears its
Sandbox and adds fresh clones between runs; this preserves the existing source
rebuild path rather than relabeling a cached graph.

Direct C++ clients that previously replaced a compiled definition through Copy
or a raw sequence setter now receive the same rebuild boundary as the source
setters. This is a behavior change. General private/transitive returned resource
dependencies, shared kernel context and complete function I/O/scoping/resource
clone parents remain open; no full parent closure is implied.

Compilation and records
-------------------------

Private integrated build v1 passes 76 steps / 12.86 seconds (session 6849
completed); the sequence-installation follow-up v2 passes 76 steps / 12.34 seconds
(session 87769 completed). No live sessions. Final source hashes match v2. XML
syntax was checked during editing. No tests, GUI, mission/function/model/solver
execution, numerical comparisons, memory checks, acceptance readers or seals ran.
Linked test/probe clients were not run.

Stage: build/nasa-coverage/coverage95-20261105-implementation-function-definition-copy-policy.
Four preimages, before/at-build/final hashes, exact patches, owning-source notes,
logs/receipts and summary-v1 are retained. No fields or virtual slots changed.
Tracking-v13 updates notes without adding rows: 1092/1158 (94.30%), 66 other
original rows. Recorded verified acceptance stays 1053/1158 (90.9%); all 105
acceptance rows remain open. No original requirement/text/fingerprint, assertion,
fixture, tolerance, denominator, ledger or gate changed. Verification stays
deferred until the full remaining implementation pass is complete.
