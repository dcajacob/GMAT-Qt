C API ephemeris discovery, 2026-11-08
====================================

All13 external C-client cases pass:8 legal configurations and5 expected
rejections. Numerical model discovery skips preceding SPK propagation, finds
the requested models in prepared/stopped/nested command graphs, preserves
assigned numerical states, and leaves spacecraft epochs unchanged. No production
change was needed. Acceptance remains1076/1158 (92.9%), with25 more needed for95%.

Actual interface and policy checks
----------------------------------

The client uses the public C loader, runner, named/unnamed model lookup, state,
derivative and spacecraft-epoch interfaces against the private integrated runtime.
The unnamed query's zero is checked as a success status; subsequent named queries
provide model IDs. The SPK setup's unused InternalODEModel is excluded from lookup.
Each case recovers through a known-good ephemeris-before-numerical script.

Native accepts distinct SPK setups. SingleSetup accepts repeated groups using
one setup and rejects distinct setups. SingleGroup accepts one group and rejects
both repeated and distinct multiple groups. Quoted enum values and a policy-only
backward/synchronized command also pass. Invalid/duplicate policy fields fail
loading; invalid counts fail initialization. Recovery succeeds after all5
rejections. These are actual loaded scripts and native initialization outcomes,
not direct calls to the count-policy helper.

The13 cases execute2758 C assertions,252 configured Cartesian scalar comparisons,
252 native Newton derivative checks and84 unchanged-epoch observations. All4
spacecraft epochs are sampled after discovery, including the2 SPK spacecraft.
No SPK stepping or independent SPK output/physical-frame result is claimed.
Positions/velocities retain2e-5km/3e-8km/s bounds; epochs retain2e-6s and native
derivative comparisons retain1e-12. These bounds were not changed.

Inputs, reuse and limits
------------------------

The tiny affine SPK fixture is copied byte-for-byte from the already qualified
origin component: body-199990, center399, J2000, TDB coverage0--900s, SHA256
0708f58440f7b463484540e92bce6de3f544a1dd0cb0109b570e7d2e1db8c6d7.
It is added as CapiEphemerisDiscovery.bsp for reproducible CTest execution. The
previous pure-C numeric helpers are reused without editing that source.

The passed pilot and remaining12 cases have identical1746-source/32-image
inventories before and after execution. All generated script/startup/expectation
inputs also match before/after. Loaded-image records identify the integrated
CInterface. The shared runtime identities match the preceding completed C API
component; no core or plugin was rebuilt for this batch. All1334 protected
original inputs remain unchanged. There were no failed native predecessors in
this component, no GUI executions, screenshots or runtime-image copies.

These optional count policies preserve the implemented native/worksheet choices;
they do not resolve the original unconditional FRC!E114 wording. Unlimited
numerical capacity FRC!E113, complete E252 configuration scope, E249/E251 dynamics
dependencies and every broader scientific/NASA/current-build/platform/ABI gate
remain open. The component adds no original requirement credit and changes no
sealed ledger or hold identity/status. No package, installed runtime or public
code was updated.

Evidence: ``build/nasa-coverage/coverage95-20261108-capi-propagate-ephemeris``.
Read ``component-audit-v1.json``, its frozen-source replay, selected logs/receipts,
source capsules, fixture provenance, input guards and publication. CMake registers
QtAcceptance.CapiEphemerisDiscovery with the new client and preserved SPK input.

Continue the previously implemented scalar-stop clock/rebinding/rejection
boundaries. Existing original stop fixtures already cover goal Variables/Arrays/
other-spacecraft epochs and first-step root behavior; reuse them. Add only the
uncovered measured-scalar, formation-clock and wrapper evaluation checks.
