Formation function graphs — implementation, 2026-11-06
=======================================================

Formation values now participate in retained function dependency graphs. Bound
spacecraft members and their covered dependencies can be captured before formal
mutation, retained across caller-frame restoration and copied as return values.
The member binder preserves the native copied packed-state policy. The full
function I/O/scoping requirements and private propagation consumers remain open.

Member references and copied values
-----------------------------------

An appended FormationInterface virtual carries native binding across the core/
plugin boundary; FunctionManager does not depend on concrete plugin classes.
The default interface operation rejects implementations that do not supply the
binding contract. Formation stages member names, resolved spacecraft pointers
and J2000 binding, checks duplicate/ambiguous names and compatible member state
sizes, then replaces its member/name packet. Alias permutations do not use
sequential renames. The binder preserves the already-copied packed state,
dimension, epoch, precision, STM and AllowSharedMembers flag. It does not call
Clear/Add/BuildState or push values into members. Native shared-member Clone
still refreshes its current snapshot according to its existing constructor.

The graph builder covers Formation roots and private dependencies using the
existing spacecraft/frame/force ownership paths. Two-argument GetRefObject now
exposes the actual bound member, allowing ObjectInitializer to preserve retained
identity. The metadata getter no longer prints unconditionally. Empty/partially
unbound declared configurations retain their native deferred references; complete
materialization and isolation of those configurations are not claimed.

Native buffers and context lifetime
-----------------------------------

Native Formation Clone remains a dormant member buffer. Explicit FunctionManager
value snapshots instead preserve complete member graphs, using a selected-source
scope that restores its prior value during nested calls and exceptions. The
source selection applies only to the intended root. Completed Clone/whole Copy
hooks reconnect retained values; Copy checks native type and holds the old lease.

Native operator assignment retains destination member bindings, including when
BufferSatelliteStates dispatches statically through FormationInterface. The base
operator now preserves contexts owning those actual member pointers while it
copies source data/body. A GmatBase nonvirtual helper flattens and deduplicates
constituent leases and prunes old contexts owning none of the active references,
while keeping one current metadata delegate. Repeated assignments do not build
an unbounded chain of old context wrappers. A subsequent retained graph copy
imports the actual source context's complete private-object union.

Remaining implementation
-------------------------

Active propagation still resolves Formation members and buffer originals by
store name. It needs bound-member/source identities, same-name independent-value
handling and complete stop/PSM/force/transient consumer work. Spacecraft buffer
restoration uses direct native operator assignment and can retain source-root
backlinks without post-copy rebinding. These are source-confirmed next work;
private Formation propagation is not declared complete or qualified. Findings
are retained in next-propagation-source-research.json in this stage. The largely
commented FillFormation helper is not the only relevant consumer.

PropSetup/PSM, GroundStation transitive graphs, burns/events/other subscribers/
providers, globals with private dependencies, private-body/shared-kernel contexts,
Parameter owners/caches across type changes, recursive cleanup/exceptional scoping,
other primitives/selectors and direct native copy paths remain. Later owner
replacement, cross-Sandbox lifetimes and moving-frame/PSM scientific contexts
also remain. Entire-object E452/E460 and input-preservation E466 are unchanged.
No parent engineering or acceptance row is awarded and no original scope waived.

Compilation and retained sources
--------------------------------

The private integrated builds pass 94 steps / 139.20 s (v1), 747 / 222.95 s (v2)
and 95 / 141.63 s (v3). V2 reports the existing OrbitRenderer QImage::mirrored
Qt deprecation; v1/v3 have no warnings. Core, Qt, console, plugins and Python
bindings compile/link. Final sources match v3. Sessions 20977, 32062 and 2051
are closed; no build remains live. Seven exact preimages (the last two captured
when scope expanded before their edits), before/at-build/final hashes, focused
patch, source notes/research, logs/receipts and planning preimages are retained
under ``build/nasa-coverage/coverage95-20261106-implementation-formation-function-graphs``.
The new FormationInterface virtual is appended after existing methods; a
consistent rebuild is required. External binary/cross-platform behavior remains
unqualified. GmatBase adds a nonvirtual helper and no field/virtual-slot change.

No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals ran. Original assertions/fixtures/tolerances/texts/
fingerprints/denominator/ledger/gates remain. Tracking is 1093/1158 (94.39%), with
65 other original rows; recorded source-versioned verified coverage remains
1053/1158 (90.9%), and all 105 acceptance rows stay open. No installed/shared
runtime, package, commit or push changed. Goal active; implementation incomplete.
