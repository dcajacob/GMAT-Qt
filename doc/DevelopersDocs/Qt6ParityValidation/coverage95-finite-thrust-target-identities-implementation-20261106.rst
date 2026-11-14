Finite-thrust target identities — implementation, 2026-11-06
============================================================

EndFiniteBurn now removes the exact active force it matched. Previously it
searched by force/spacecraft names, then independently removed the first force
with the same name. Different function-local objects can share those labels.
Configured finite thrust now records actual spacecraft and burn identities.
This is a partial contribution to broad object/scoping requirements, not a
complete transient lifetime or function-value implementation.

Configured identity
--------------------

BeginFiniteBurn records its actual resolved spacecraft separately from the
FiniteThrust propagated-object array. FiniteThrust target selection uses the
configured pointer and a weak lifetime identity. Explicit expired bindings never
fall back to equal names. Default PhysicalModel transient providers and unbound
legacy FiniteThrust definitions retain name selection. Names remain available
for serialization and legacy APIs; Rename/Clear update the target registry.

New nonvirtual GmatBase lifetime helpers lazily register identities in the
existing side registry, without native GmatBase fields or virtual slots.
Validation compares the current registry identity, so pinning an old token or
reusing a deleted object's address cannot revive an old reference. Assignment
preserves object identity; new copies receive their own lazy identity. Tokens
are non-owning and do not provide concurrent destruction safety.

FiniteThrust guards burn dereferences with saved lifetime identities. Its overlap
check compares shared actual targets and selected thruster names when burns are
bound, preserving burn-name fallback for unbound legacy definitions. UsesBurn
matches the actual burn pointer, independent of formal or object spelling.
EndFiniteBurn combines that match with actual target identities and removes the
same matched pointer. Unrelated expired burns are not inspected by UsesBurn.

Propagation attachment, Formation-member diagnostic matching, ephemeris temporary
attachment and finite-thrust integration/sampling use the target query contract.
ODE transient admission compares configured FiniteThrust target intersections;
other provider paths retain guarded legacy names. Native Formation/multiple-
spacecraft finite-burn restrictions remain. Live ephemeris transient attachment
and the earlier private sampling isolation remain; no formulas or tolerances
were changed. Main integration retains its native setter/Fire evaluation path.

Remaining scope
----------------

Direct formal aliases of the same actual object no longer depend on equal labels.
Arbitrary copied spacecraft are independent values; explicit query-copy provenance
across retained ephemeris graphs still needs a defined binding path. Private graph
children can have their context cleared to avoid ownership cycles, so a child
context lookup alone cannot recover that provenance. No broad alias equivalence
or universal zero-acceleration correctness claim is made.

Weak root/burn tokens do not keep deleted roots alive or validate deleted force
objects in an old ODE cache. Audit Begin force ownership/reinitialization and all
force borrowers. Current GmatFunction initialization reinitializes native commands
on entry and frame restoration; do not infer stale command bindings solely from
fcsInitialized. Do not remove every transient on function return: explicit global
maneuvers need a deliberate persistence policy. Relevant source findings are in
next-transient-lifecycle-source-research.json.

Standalone burn/hardware, PropSetup/PSM, GroundStation/event/subscriber/provider
graphs, global/private bodies/kernels, deferred configuration, Parameter/type/cache/
recursive/exceptional scoping, later owner replacement and cross-Sandbox/scientific
body/frame/command obligations remain. E452/E460 entire-object and E466 input
preservation requirements retain their full original scope. No parent row is closed.

Compilation and retained work
-------------------------------

The private integrated v3 build passes 676 steps / 210.48 s.
Two failed compilation attempts are retained; they identified nested loop variable
shadowing in the new Formation diagnostic selection. The correction uses an
explicit transient pointer outside the nested loops. Final source hashes match v3;
sessions 57132, 26093 and 21608 are closed, no live build. Eleven exact preimages,
before/at-build/final hashes, focused/per-file patches, source notes/research,
logs/receipts/summary and planning preimages are under
``build/nasa-coverage/coverage95-20261106-implementation-finite-thrust-target-identities``.

PhysicalModel gains an appended virtual target query and FiniteThrust gains
binding data; all core/providers were rebuilt together. External binary ABI and
cross-platform behavior remain unqualified. The existing Qt OrbitRenderer
QImage::mirrored deprecation warning is retained in summary.json.

No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals ran. Original assertions, fixtures, tolerances,
requirement texts/fingerprints, denominator, ledger and qualification gates remain.
Tracking-v26 stays 1093/1158 (94.39%), 65 other original rows; recorded source-
versioned verified coverage stays 1053/1158 (90.9%), all 105 acceptance rows open.
No installed/shared runtime, package, commit or push changed. Goal active;
implementation incomplete.
