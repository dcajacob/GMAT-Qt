Burn function graphs — implementation, 2026-11-06
==================================================

Finite and impulsive burns now participate in retained function value graphs.
Their frame, origin, J2000 and target spacecraft dependencies reconnect through
the owning captured scene; copying/binding a value does not fire a maneuver.
Owned local-frame and selected-hardware cache defects were repaired. Full
function/object/scoping implementation remains incomplete; no broad row is closed.

Retained values and dormant definitions
----------------------------------------

Burn adds passive typed getters and virtual RebindClonedReferences and
InitializeClonedResource contracts. Binding stages actual mapped references and
preserves declared role labels when caller/formal aliases differ. It assigns the
SolarSystem directly, avoiding SetSolarSystem's eager virtual initialization and
maneuver-start setters. FunctionManager admits Burn roots/dependencies for input
capture, saved caller frames, named/unnamed outputs and context rebinding. Native
finite/impulsive Clone and typed Copy reconnect only after the concrete copy is
complete; previous destination leases span replacement.

Private initialization runs after frames and spacecraft. It regenerates the
local factory frame from resolved references without replacing a retained origin
with an equal-name solar-system lookup. A burn with no spacecraft remains a valid
dormant definition. Unbound thruster prototypes likewise remain dormant until
owner initialization can perform native validation. This preserves definitions
rather than pretending that missing target state is an executable maneuver.

FiniteBurn retains only configured unbound prototype thrusters or rebinds current
owned thrusters after the mapped spacecraft initializes. Native copy policy for
integrator-owned epoch/state/mass/origin overrides remains reset; no Fire or
throttle evaluation runs to reconnect it. ImpulsiveBurn similarly retains selected
unbound tank prototypes or resolves current mapped-owner tanks. Its passive tank
getter refreshes from the actual owner without firing.

Native frame and selection repairs
-----------------------------------

Burn assignment now releases its previous owned local frame. Destruction,
reinitialization, mode/origin/axis and target/reference changes use the same
retirement helper, preserving configured-frame aliases as borrowed. Passive
setters validate types/declared roles and invalidate old frames. The configured-
frame factory checks for a missing frame. MJ2000Eq and spacecraft-body shortcut
flags are recomputed for the current axes; rejected axis names leave the stored
selection intact. The legacy Inertial alias cannot retain an unrelated frame.
These repairs can change previously incorrect frame-selection results; numerical
qualification is deferred. Conversion equations and supported-axis policy remain.

Impulsive tank lookup now stages each selected tank independently, skipping null/
wrong-type entries and rejecting missing, ambiguous or duplicate selections. The
old loop reused an exhausted iterator for later tanks and could loop forever on
a null entry. Target-change detection precedes the base setter's assignment.
Indexed tank/thruster changes retire old prototype cache keys and readiness;
negative indices and the finite append/duplicate out-of-range read are guarded.
Only configured prototype keys enter retained graphs. Native mass-depletion
restrictions, thrust/ISP/mass-flow equations and tolerances remain unchanged.

Remaining implementation
-------------------------

FiniteThrust native copies still clear bound burns while retaining configured
and propagated target data. The retained PhysicalModel binder currently reconnects
body/frame roles, not the force's burn/spacecraft graph. Add explicit live-source
burn/target rebinding and retire copied evaluator/propagated caches. Preserve the
separate ephemeris reporting policy: strip frozen transient copies and observe the
live caller vector. Cloning a private value must not activate it in that vector.
See next-finite-thrust-value-source-research.json.

Explicit transient sampling-copy aliases/provenance, global maneuver persistence,
private celestial bodies/kernels, unresolved configured references, direct native
assignment/later owner changes, shared provider caches and full recursive/
exceptional scoping remain. Continue PropSetup/PSM, GroundStation/event/subscriber/
provider and every original body/frame/command/scientific obligation. E452/E460
entire objects and E466 input preservation retain their full scope.

Compilation and retained work
-------------------------------

The private integrated build passes all three versions: v1 390 steps / 182.41 s, v2 66 steps / 2.39 s, v3 68 steps / 2.72 s.
Final sources match v3; 28416/25772/71425 closed, no live build. Nine exact
preimages, before/at-build/final hashes, combined and indexed per-source patches,
notes/research, logs/receipts/summary and planning preimages are under
``build/nasa-coverage/coverage95-20261106-implementation-burn-function-graphs``.
Burn gains two virtual contracts; core/providers were rebuilt together. No native
fields were added. External ABI/cross-platform behavior remains unqualified. The
existing OrbitRenderer QImage::mirrored deprecation is retained in summary.json.

No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals ran. Original assertions, fixtures, tolerances,
requirement texts/fingerprints, denominator, ledger and qualification gates remain.
Tracking-v28 stays 1093/1158 (94.39%), 65 other original rows; recorded source-
versioned verified baseline stays 1053/1158 (90.9%), all 105 acceptance rows open.
No installed/shared runtime, package, commit or push changed. Goal active;
implementation incomplete.
