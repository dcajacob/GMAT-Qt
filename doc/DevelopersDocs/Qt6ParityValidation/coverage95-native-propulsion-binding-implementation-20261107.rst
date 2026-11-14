Native propulsion binding and power-system replacement
======================================================

Spacecraft's native SetRefObject tank/thruster routes previously erased an existing clone before preparing its replacement, suppressed AttachTanksToThrusters failures and deleted retired objects even when surviving thrusters could still hold their pointers. Native attachment now dispatches through the staged propulsion operation introduced in the preceding batch. This connects the implementation to ordinary native binding as well as MATLAB list results.

Incremental declarations and initialized updates
------------------------------------------------

An initialized spacecraft requires actual sources for its complete declared tank/thruster lists. A native matching source replaces the corresponding binding, while every other name preserves its actual owned component. Resources whose names are not declared retain the native ignored-source behavior. Replacement failures propagate before publication; the old clone is not queued for unconditional deletion after a suppressed error.

Before initialization, native declarations can arrive in any order. A private overload accepts unresolved source slots for that phase only; owned arrays never contain null placeholders. A thruster with unresolved tanks gets an empty pointer cache rather than retaining source/displaced pointers. Duplicate configured tank names still reject, and a firing thruster cannot be left pending. The public typed packet remains strict. The native initialization boundary checks every declared attachment, requires all thruster tank links and supplies the current solar system before native thruster initialization. Initialization readiness remains false until successful completion.

SetupHardware/AttachTanksToThrusters now prepares every thruster link vector before swapping any of them. This avoids publishing earlier link changes if a later thruster cannot bind. It continues to use native configured names and existing chemistry/electric model initialization; force, flow, mass and direction formulas are unchanged.

Removal actions
---------------

RemoveTank/RemoveThruster/RemoveHardware prepare their desired propulsion lists before retiring components. Named tank removal rejects when a surviving thruster still declares that tank. Initialized removal retains complete-graph, firing-thruster and known covariance/STM owner checks. Before initialization, clearing tank definitions preserves retained thruster objects and clears their unresolved caches; the old implementation silently deleted those thrusters while retaining their names. Their eventual binding still must satisfy strict initialization.

Power-system removal is preflighted before a combined propulsion update and occurs only after that update succeeds. Thus a rejected tank/thruster removal cannot first delete the power system. Known owner references and firing electric-thruster dependencies reject unsafe power-system retirement. These are explicit component operations, not a transaction for arbitrary global or external borrowers.

Power-system provider
---------------------

Spacecraft exposes nonvirtual ReplacePowerSystem(name, actualSource). It validates names/source types and attachment collisions, prepares an exact-type distinct clone with its full owned name, current Sun/owner and native initialization when required, and validates old-owner retirement before publishing. The name and owned pointer publish together; known covariance/STM/estimated owner pointers remap to compatible replacements. The previous target/source contexts remain leased through preparation and retirement. Existing matching-source SetRefObject attachment delegates here.

An empty name or None with a null source is an explicit clear operation. Native empty/None string assignment and MATLAB spacecraft PowerSystem results use that provider. MATLAB uses the existing actual-reference plan and identity checks for a nonempty selection, preserving private same-name bindings. A rejected clone, initialization, collision or retirement leaves the previous name and component intact for this operation. Other output properties/results retain their separate assignment semantics.

Spacecraft solar-system assignment validates its attached power-system binding before publishing the root pointer. This bounded ordering improvement does not implement a universal transaction across all spacecraft bodies/frames/services or shared kernel state.

Compilation and retained implementation scope
---------------------------------------------

Configured compilation v1: 400/400 steps, 122.91 seconds, return 0.
Configured compilation v2: 399/399 steps, 126.56 seconds, return 0.
Configured compilation v3: 66/66 steps, 9.31 seconds, return 0.

Six source/help paths have preimages captured before their first edit (five before v1, MATLAB command source before v2), before/build/final hashes, indexed deltas, source map, logs, authoritative terminal receipts and four planning preimages. Final sources match the last successful configured build. All compiler sessions are terminal; added-line whitespace is clean and help XML parses. Existing instance layouts and virtual slots are unchanged; helper/packet APIs are nonvirtual and matching configured consumers rebuilt. External binary/plugin and platform/runtime compatibility remain unqualified. Existing deprecation diagnostics outside this batch, when emitted, are retained in build summaries.

The prior batch's non-SPICE Spacecraft compiler failure belongs to unchanged EphemManager.hpp SPICE-only type declarations and was reproduced on its captured source preimage. The header hash remains unchanged. This batch does not repeat the known failing conditional compile or claim complete non-SPICE portability.

Other hardware/plate attachment families, radio/antenna/error-model interconnections and existing UpdateClonedObject/parameter/copy/whole-spacecraft assignment paths still need their own complete native providers and actual dependency handling. Generic incremental AddHardware retains its old separate retirement route. External borrowers, changed cross-scope dependencies, public declarations/components, shared globals, remaining function/resource/command/scientific requirements and specification/external/historical dependencies remain open. No original all/any parent receives partial credit.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed.

Tracking-v79 remains 1093/1158 (94.39%) with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
