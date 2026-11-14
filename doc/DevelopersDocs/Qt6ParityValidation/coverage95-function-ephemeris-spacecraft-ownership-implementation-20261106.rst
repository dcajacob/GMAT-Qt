Returned ephemeris private spacecraft ownership — implementation, 2026-11-06
============================================================================

A detached returned EphemerisFile can now retain a private Spacecraft together
with its frames, axes, calculated points and native hardware/estimation
references. The existing exact formal caller mappings still take precedence for
external resources. The shared EphemerisFile dependency owner introduced by the
frame stage retains this expanded graph and its inherited contexts.

Spacecraft binding
------------------

A new nonvirtual RebindClonedReferences path registers the actual native owned
attitude, power system, tanks, thrusters, hardware and plates before following
references. Antenna/tank cross-links and owned FOV/error/model children retain
their cloned identities. Native owners continue to delete attached objects; the
graph deletes only top-level snapshots. It does not attach a back-owning graph
token to its owned spacecraft.

The hook directly rebinds internal/display frames, origin/J2000 pointers, frame
maps, process-noise and estimated-parameter references, external STM sources,
attached-component covariance owners and the precision origin-relative cache.
It does not call setters that convert initial state, clone hardware again or
append estimated parameters. State, epoch and STM values remain copied values.
Power-system spacecraft/origin pointers refresh after binding. Plates discard
the source spacecraft's cached inertial-frame pointer; the existing spacecraft
reflectance paths provide their own frame before using it.

Binding finishes before initialization. Calculated points and coordinate systems
initialize before the remaining resources. A frame/spacecraft/frame reference
path can depend only on Cartesian position, so such pointer backlinks are
permitted; pure private frame/point cycles still fail explicitly. The new
InitializeClonedResource uses native spacecraft initialization while skipping
ConfigManager hardware reassociation: the snapshot already has the correct
attached antenna/error-model bindings. Ordinary Initialize keeps its existing
association path. These source routes are compiled but behaviorally unverified.

Copy/reference defects repaired
-------------------------------

Thruster now exposes its actual typed frame, body, spacecraft and tank references;
tank names/types and typed renaming are consistent with those accessors.
Spacecraft default/copy/assignment now initialize or copy origin flattening and
equatorial radius alongside mu. Its lookup-table interpolators are independent
owned copies and are deleted by their spacecraft, rather than sharing mutable
buffers. ProcessNoiseModel and EstimatedParameter initialize copied model
pointers, preserve configured names, stage independent clones before replacing
old owners and reject self/null/wrong-native-type clones. ProcessNoiseModel's
getter also accepts the ProcessNoise type advertised by its reference interface,
while retaining its existing ProcessNoiseModel accessor.

No field or virtual slot is added in this stage; new lifecycle methods are
nonvirtual and new getters override inherited slots. The earlier EphemerisFile
shared-owner member still requires rebuilt native clients/subclasses.

Remaining scope
---------------

Direct private acceleration models are still borrowed when no caller counterpart
exists. ODE owned forces, their frame/body/origin references, transient models and
writer transient-vector lifetime are the next dependency scope. Standalone
returned spacecraft/other resource roots still need their own dependency owner;
this stage captures private spacecraft inside a returned ephemeris graph.
Explicit caller-bound spacecraft outputs keep the existing policy and are not
universally rewired here. Global ephemeris identity roots referring to locals,
input graph isolation, other SpacePoint/resource families, general Clone/scoping,
SPICE context and third-party reference contracts remain open. No broad function
I/O, all-frame, final-pass or universal cloning row is closed.

Compilation and campaign status
-------------------------------

The private integrated build passes v1: 392 steps / 353.21 s; v2: 68 / 7.17 s;
v3: 66 / 6.18 s; v4: 66 / 7.83 s. The refinements add native-type clone checks,
frame/spacecraft backlink handling, phased initialization and attached covariance
owner rebinding. Core, Qt, console, plugins and Python bindings compile/link. V1
has the existing OrbitRenderer QImage::mirrored deprecation; v2-v4 have no
warnings. Final source matches v4 hashes. No sessions remain live.

No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals ran. Engineering tracking stays 1092/1158 (94.30%),
with 66 other original rows. Recorded verified baseline stays 1053/1158 (90.9%);
all 105 acceptance rows remain open. Original assertions, fixtures, tolerances,
texts, fingerprints, denominator, ledger and gates are unchanged. Implementation
continues before verification.

Artifacts
---------

The stage began under the existing 20261105 prefix:
``build/nasa-coverage/coverage95-20261105-implementation-function-ephemeris-spacecraft-ownership``.
It contains ten exact preimages, before/at-build-v1-v4/final hashes, a focused
patch, source notes, four build logs/receipts and summary. No installed/shared
runtime, package, commit or push was changed.
