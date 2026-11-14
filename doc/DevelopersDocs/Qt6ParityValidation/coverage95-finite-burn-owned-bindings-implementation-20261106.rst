Finite-burn owned bindings — implementation, 2026-11-06
========================================================

Finite burns now resolve selected thrusters and their electric/chemical type from
the current spacecraft before throttling and firing. Reporting resolves the
requested current owned thruster, avoiding an obsolete cache after spacecraft
Copy replaces its children. Tank association and sampling likewise use current
owner references. The changes compile; complete burn/function/force ownership
and scientific verification remain unfinished.

Selected components and passive lookup
--------------------------------------

RefreshThrusterBindings checks current selected identities, missing/ambiguous or
duplicate selections and chemical/electric consistency. A spacecraft address
alone no longer determines whether cached burn classification is valid. Typed
casts guard electric throttling and thruster access. GetRefObject resolves only
the requested configured thruster without firing, throttling, tank mutation or
validation of unrelated selections. Unbound definitions retain prototype storage.
SetRefObject validates type/name and selects the current owned component when
bound. Assignment/list clearing retires old reporting caches. SetSpacecraftToManeuver
keeps its native Fire side effect; this increment does not use it passively.

Tank and plate ownership
------------------------

Thruster::BindTankReferences stages configured tank references in declaration
order against a supplied current collection and swaps the validated pointer
vector. It never reads old borrowed tank objects. Names and mixture ratios stay
intact. Spacecraft association and finite-burn setup/firing use this path. The
previous burn setup called an unsupported ClearTankNames action and rebuilt
names; an empty tank collection could also evade its lookup failure. Missing/
ambiguous bindings now fail through the current owner association.

Owned-thruster single tank getters resolve against their current spacecraft;
FuelTank/Hardware array getters refresh pointer bindings. Standalone prototypes
keep their existing path. ChemicalThruster refreshes references before firing
pressure/temperature sampling, preserving the disabled zero-result path. Thrust,
ISP and mass-flow formulas and numerical tolerances remain. Burn mass-depletion
selection uses its configured thrusters rather than unrelated spacecraft hardware;
an unbound burn reports false. Spacecraft assignment also deletes owned plates
removed from its list, fixing a lost owning-pointer allocation leak.

Actual source inventory and remaining scope
--------------------------------------------

SRP NPlate acceleration/derivatives call current spacecraft reflectance methods,
and torque fetches the current plate array. Fire, IsFiring and electric throttle
logic already fetch current thrusters. A generic stale plate list or universal
old-thruster force path was not established; these formulas were not replaced
speculatively. The repaired paths are cached burn classification/reporting and
owner tank association/sampling. Native frame/provider and internal plate-cache
lifetimes still require complete review.

Live transient target identity/restoration across private spacecraft sampling,
same-name independent values and function/caller lifetime remains. FiniteThrust
sampling calls SetSpacecraftToManeuver, which itself fires; root target restoration
must account for that side effect. Current child refresh does not keep a deleted
spacecraft root alive. ThrustFile segments retain borrowed mass-source pointers;
actual ownership/consumers need inventory before any universal stale-cache claim.
Read-only findings are retained in next-force-context-source-research.json.

Standalone burn/hardware roots, PropSetup/PSM, GroundStation/event/subscriber/
provider graphs, globals/private bodies/kernels, parameter/type/cache/recursive/
exceptional scoping and complete command/body/frame/scientific domains remain.
Generic prototype setters, later owner replacement and cross-Sandbox lifetimes
are not universally repaired. E452/E460 entire objects and E466 input preservation
remain; no original scope is waived and no broad parent row is credited.

Compilation and retained work
------------------------------

Private integrated builds pass 390 steps / 186.11 s (v1), 67 / 2.88 s (v2) and
67 / 2.63 s (v3). Core, Qt, console, plugins and Python bindings compile/link.
Warnings are retained verbatim in the build summary; they are not acceptance evidence.
Final sources match v3. Sessions 44668, 21416 and 88200 are closed; no live build.
Six exact preimages (ChemicalThruster added before its first edit), before/at-build/
final hashes, focused patch, source notes/research, logs/receipts and planning
preimages are retained under
``build/nasa-coverage/coverage95-20261106-implementation-finite-burn-owned-bindings``.
New helpers are nonvirtual; existing native fields/vtable slots are unchanged.
External binary/cross-platform behavior remains unqualified.

No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals ran. Original assertions, fixtures, tolerances,
requirement texts/fingerprints, denominator, ledger and qualification gates remain.
Tracking stays 1093/1158 (94.39%), 65 other original rows; the recorded source-
versioned verified baseline stays 1053/1158 (90.9%), all 105 acceptance rows open.
No installed/shared runtime, package, commit or push changed. Goal active;
the complete implementation pass is not finished.
