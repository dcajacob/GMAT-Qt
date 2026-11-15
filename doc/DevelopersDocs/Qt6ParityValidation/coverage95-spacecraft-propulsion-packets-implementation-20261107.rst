Spacecraft propulsion packets and MATLAB object updates
=======================================================

The generic typed object-list route could not replace spacecraft propulsion hardware without changing names separately from owned clones and thruster tank pointers. Spacecraft now overrides the existing SetField(Integer, StringArray) entry point for Tanks and Thrusters, and exposes a nonvirtual ReplacePropulsionHardware operation accepting complete desired names and actual sources for both families. The typed field route supports removal/reordering/clearing of existing attachments; adding a new attachment requires its actual source through the packet API.

Preparing the native owned graph
--------------------------------

A packet checks source counts, native types, identity syntax, duplicate names/sources and collisions with other attached hardware, plates or the power system. Unchanged actual owned components retain their identities. New components are cloned and named off to the side with RAII cleanup; Clone must return a distinct exact-type object. New tanks initialize when the spacecraft was already initialized. Each retained or new thruster resolves its complete tank-name list against the proposed tank set without changing existing links. Native chemical/electric thrusters require their corresponding tank family.

New thrusters receive the actual owner, solar system, available configured frame and proposed tank links before native initialization. Existing firing state follows the native same-name rule, but replacing/removing an already firing thruster rejects until the finite burn ends. Retained thrusters keep their existing coordinate systems and runtime identities. A rejected source, clone, tank link, compatibility check, initialization or mass-property preparation cannot publish a partial hardware packet.

Known covariance and spacecraft external STM/estimated-parameter owner pointers are prepared for same-name exact-type replacements. Removing or changing the type of a component still referenced by those internal structures raises an error. Covariance provides a nonvirtual HasOwner query for that check; existing noexcept RebindOwner performs the final pointer update. Arbitrary external borrowers and cross-scope dependency retirement are not solved by these internal updates.

Native total mass, analytic center of mass and inertia routines now accept an explicit proposed tank array; their existing no-argument callers delegate with the current tank array. Lookup and analytic mass-property selection still use the existing native computations. These values are prepared before publishing lists. Static source-diff comparison confirms the three analytic calculation bodies are identical after parameter-name/call normalization. No science formula, lookup policy or numerical acceptance tolerance was replaced.

Publication swaps prepared name/owned-object containers and retained thruster tank links, remaps known internal owners, publishes cached mass properties and then retires displaced owned components. Staging holds the previous function-value dependency context through this interval. This is a propulsion packet operation, not a universal rollback for global services or arbitrary user/plugin side effects.

MATLAB result grouping
----------------------

The MATLAB scalar-structure importer recognizes writable spacecraft Tanks and Thrusters fields as one packet. Both lists are parsed/planned before either is assigned, independently of their structure field order; omitted lists retain the existing attachments. Duplicate aliases for one list reject rather than produce order-dependent partial replacements. The assignment is queued at the first returned propulsion field.

Unchanged names preserve the actual bound owned object, including private objects whose labels also occur in a public map. New names resolve through the active command scope. Weak lifetime identity and current-map identity checks run again before native packet preparation, with captured contexts held through assignment. The native operation clones new sources; the public prototypes are not installed as owned children. Unknown/read-only field rules and native type checks remain. Earlier or later non-propulsion assignments and other result objects are outside this packet and may still be applied separately.

Compilation and remaining implementation
----------------------------------------

Configured compilation v1: last logged step 385/750, 72.41 seconds, return 1.
Configured compilation v2: last logged step 366/366, 106.71 seconds, return 0.
Configured compilation v3: last logged step 66/66, 8.2 seconds, return 0.
Conditional syntax without SPICE, src/base/spacecraft/Spacecraft.cpp: 0.72 seconds, return 1.
Conditional syntax without SPICE, src/base/hardware/Thruster.cpp: 0.46 seconds, return 0.
Conditional syntax without SPICE, src/base/foundation/Covariance.cpp: 0.39 seconds, return 0.
Conditional syntax without SPICE, plugins/MatlabInterfacePlugin/src/base/command/CallMatlabFunction.cpp: 0.76 seconds, return 0.

The first compiler attempt caught the hidden label overload for Thruster::SetBooleanParameter; the call now uses the native parameter ID. Its failed log/receipt is retained. Conditional syntax uses the configured Linux compiler/SDK with __USE_SPICE__ omitted. Thruster, Covariance and CallMatlabFunction pass. Spacecraft fails because unchanged EphemManager.hpp declares GetTargetState with undeclared SpiceInt/ConstSpiceChar/SpiceDouble types. Compiling the captured pre-change Spacecraft source produces the exact same error diagnostics; the unchanged dependency header and its hash are retained. This pre-existing dependency/portability issue remains open. No complete non-SPICE application or other platform is qualified. Configured v2 also reports existing wstring_convert deprecation warnings in MatlabInterface.cpp and a QImage::mirrored warning in OrbitRenderer.cpp, both outside this batch.

Eight source/help paths have preimages before first edits, before/build/final hashes, indexed deltas, source map, logs, terminal receipts and four planning preimages. Final sources match the last successful configured build and conditional snapshot. All compiler sessions are terminal; added-line whitespace is clean and help XML parses. Existing instance layouts and virtual slots are unchanged; Spacecraft overrides an existing method, and the new helper/packet APIs are nonvirtual. Matching configured consumers rebuilt; external binary/plugin and platform/runtime compatibility remain unqualified.

Legacy incremental SetRefObject/SetHardware attachment routes retain their separate mutation/retirement behavior. The EphemManager non-SPICE header/implementation boundary remains open. Power systems, plates, other hardware, other resource-owned lists, explicit reference clearing and long-term external borrower/context ownership remain open. All original broader function/resource/command/scientific requirements, including FRR-36 and specification/external/historical dependencies, remain in scope. No partial all/any parent earns credit.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed.

Tracking-v78 remains 1093/1158 (94.39%) with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
