Propagation buffer identities — implementation, 2026-11-06
==========================================================

Propagation buffers now restore the exact objects they captured, including
retained private Formation members outside the script store. Preparation detects
changed root/member bindings before reading cached models. A separate dynamics
fix advances the complete flattened member count when a Formation is followed
by another object. These source changes compile; full propagation/function
isolation and scientific qualification remain incomplete.

Member and buffer identities
-----------------------------

The common Formation resolver selects actual bound spacecraft in Add order,
checks type/ambiguity/repetition, and retains the native script-store fallback
for unresolved configuration names. Buffer creation records exact source pointers,
deduplicates captures of one source and guards incompatible or known borrowed
Clone results. Failed adoption rolls back buffers. Fill and restore use that
source record, with no replacement by an equal-name script-store object.

Spacecraft buffer assignment uses typed native Copy, including the existing
post-copy graph rebind, to avoid backlinks to a temporary buffer root. Formation
buffers preserve their native dormant/base-assignment policy. Propagate rejects
duplicate spacecraft identities, while retaining the publication name list.
Independent private values do not become the same object merely because they
share a display name. User-scalar stop clocks select the first actual Formation
member; the inactive FillFormation helper uses the same resolver.

Refresh and ownership
----------------------

Buffered root snapshots hold context leases, member identities/order, state
sizes and context tokens. Changes cause preparation to rebuild before activating
or reading the old acceleration/PSM context. Old leases remain through model
reinitialization. After RunComplete retires snapshots, the next preparation
captures new sources. The native base cold-preparation Initialize remains a
single call. Buffers/mappings/leases clear on reinitialization, command assignment,
RunComplete and destruction; copies begin empty. This is not a universal native
configuration-version or external-parent lifetime guarantee.

Dynamics member slots
----------------------

UpdateDynamicSpacecraftData previously recursed over a Formation inside each
force loop, discarded the returned next index and advanced only one slot. The
next object could overwrite a preceding member's data. It now flattens the actual
members before the force loop and carries the returned slot forward. Native
parameter values/checks/formulas remain. UpdateInitialData clears all relevant
forces before one setup pass, or invokes one dynamic update; those helpers already
visit every force, so the prior outer loop repeated complete updates.

Remaining scope
----------------

Force caches that retain owned hardware/plate/frame pointers need complete review
after spacecraft Copy replaces those components. Name-based covariance/history,
publication and transient-target consumers also need an ownership/key inventory.
The common ODE spacecraft setup already uses actual member arrays and indexed
SetSpaceObject/SetSatelliteParameter; do not infer every remaining name use is a
confirmed collision. Read-only findings are retained in next-dynamics-source-research.json.
No universal same-name independent-value or private Formation dynamics claim is made.

PropSetup/PSM, GroundStation transitive dependencies, burns/events/other subscribers/
providers, globals with private dependencies, private bodies/kernels, deferred
member configuration, Parameter/type/cache/recursive/exceptional scoping and other
primitive/selector/native-copy paths remain. Later owner replacement, context-free
native mutations, cross-Sandbox lifetimes and complete body/frame/command/scientific
domains remain. E452/E460 entire objects and E466 input preservation are unchanged.
No broad parent row is credited; every other original scope stays in the worklist.

Compilation and retained work
------------------------------

Private integrated builds pass 84 steps / 140.11 s (v1) and 67 / 5.04 s (v2),
without warnings. Core, Qt, console, plugins and Python bindings compile/link.
Final sources match v2. Sessions 86360 and 93564 are closed; no live build.
Four exact preimages (ODEModel added before its first edit), before/at-build/final
hashes, focused patch, source notes/research, logs/receipts and planning preimages
are under ``build/nasa-coverage/coverage95-20261106-implementation-propagation-buffer-identities``.
PropagationEnabledCommand storage changes require a consistent rebuild; external
binary/cross-platform behavior remains unqualified.

No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals ran. Original assertions, fixtures, tolerances,
requirement texts/fingerprints, denominator, ledger and qualification gates remain.
Tracking stays 1093/1158 (94.39%), with 65 other original rows; the recorded
source-versioned verified baseline stays 1053/1158 (90.9%), with all 105 original
acceptance rows open. No installed/shared runtime, package, commit or push changed.
The goal remains active and the complete implementation is unfinished.
