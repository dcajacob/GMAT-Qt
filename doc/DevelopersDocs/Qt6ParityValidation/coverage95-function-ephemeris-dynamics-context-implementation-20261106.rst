Returned ephemeris dynamics context — implementation, 2026-11-06
================================================================

Detached EphemerisFile results now retain a private static AccelerationModel
with its native force components and actual body/frame dependencies. The
existing shared dependency graph owns private model frames and preserves
source-identity caller mappings, true globals and native component ownership.
All owned force identities register before following spacecraft cross-links.
Harmonic frame bindings preserve declared role labels even when the caller's
actual resource has a different name.

DragForce has an additional typed binding path because its generic getter does
not expose its internal coordinate system. Its copy constructor borrows the
fixed frame, and assignment borrows both frame fields. The owned atmosphere
clone also has borrowed frame bindings. The new nonvirtual helper maps both
actual force fields and refreshes those atmosphere bindings before initialization.

Live burn context
------------------

Sandbox owns the live transient-force vector; BeginFiniteBurn owns its force
objects. FunctionManager and function commands already pass this same context.
Function-created ephemerides now receive it in both object initialization paths,
as main-sequence ephemerides do. Returned ephemerides rebind to their caller's
live context. Their private reporting model drops copied transient forces,
retaining static configuration and avoiding stale or duplicate burn attachments.
Ordinary ODEModel Clone retains its existing force-copy behavior.

This corrects the preceding ownership report's proposed frozen transient-vector
snapshot: that snapshot would miss subsequent BeginFiniteBurn/EndFiniteBurn
changes. FiniteBurn::SetSpacecraftToManeuver calls Fire and is not used as a
passive copy-binding operation. Native burn derivative mathematics is unchanged.
The EphemerisFile user guide documents this calling-context behavior.

Parameter initialization
-------------------------

ODEModel full initialization requires a propagation state manager. Reporting
copies instead use InitializeForParameters, shared with the existing single-
spacecraft derivative and force-lookup paths. It resolves missing J2000/origin
defaults, supplies harmonic and drag fixed frames and checks component failure.
It preserves the existing all-force initialization policy and does not claim
full propagation readiness. Dependency binding finishes before initialization;
points, frames, spacecraft/auxiliaries and physical models initialize in phases.

Scope and compilation
----------------------

Standalone returned roots, input dependency isolation, globals with private
dependencies, private celestial-body roots, opaque provider references and full
burn/function scoping remain open. Output copies replacing previously bound
subobjects and moving-frame acceleration contexts also need source work. E452
and E460 still include entire objects; E466 still preserves non-global inputs
unless also returned. No broad obligation is reduced to this ephemeris repair.

Integrated compilation passes 327 steps / 174.95 s (v1), 66 / 4.96 s (v2), and
76 / 139.64 s (v3). Core, Qt, console, plugins and Python bindings compile/link.
Eleven exact source preimages, before/at-build/final hashes, patch, source notes,
logs, receipts and summary are under
``build/nasa-coverage/coverage95-20261106-implementation-function-ephemeris-dynamics-context``.
Final source matches v3; no build remains live. V2's terminal receipt was
recovered after tool output was lost; its original handle is unavailable.

This stage adds nonvirtual helpers, with no new fields or virtual slots. Earlier
ODEModel/EphemerisFile layout changes still require native clients/subclasses to
rebuild; binary-provider compatibility remains unqualified.

No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals ran. Engineering tracking remains 1092/1158 (94.30%),
with 66 other original rows; recorded source-versioned verification remains
1053/1158 (90.9%) and all 105 acceptance rows remain open. Original assertions,
fixtures, tolerances, requirement texts/fingerprints, denominator, ledger and
qualification gates are unchanged. No installed/shared runtime, package,
commit or push changed. Full implementation continues before verification.
