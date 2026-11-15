Concrete owned clone updates and typed property propagation
===========================================================

Spacecraft::UpdateClonedObject assigned through FuelTank, Thruster, PowerSystem or Hardware base operators. This omitted concrete model fields such as chemical-thruster coefficients, electric-thruster data and solar-power coefficients. Its plate branch was also nested inside a Hardware check although Plate derives from GmatBase. The property route called CopyParameter, which silently did nothing outside real/integer/string/filename/Boolean types. This batch connects both notification routes to prepared concrete native values.

Owned update preparation and publication
----------------------------------------

A matching owned component is selected within its native tank/thruster/power/plate/other-hardware family, with ambiguity and concrete RTTI/type-name checks. Whole updates clone the incoming definition; property updates clone the current owned value and change only the notified property. This preserves other live values when one property changes. Temporary copies and publication use FunctionValueCopyGuard so a native owned leaf does not invoke an unrelated completed public-root context hook.

Thruster preparation retains actual current frame/body bindings when their declared roles are unchanged, resolves changed frame/body roles through available actual bindings/native owner context, resolves tank names against the owner's actual tanks and preserves firing state except for an explicit IsFiring property notification. Its native local coordinate system is prepared against the actual owner. A firing thruster cannot remain pending. Chemical/electric compatibility and initialized model constraints retain their native initializer checks.

Power preparation binds the actual owner/origin and current Sun, preserving an existing private Sun under the same solar-system binding. Solar shadow bodies preserve existing actual same-name bindings, then use available incoming/native owner bindings; native default/empty-list policy remains. A solar shadow evaluator is prepared through InitializeClonedResource. RF hardware uses the actual attached antenna by its native declared name, with type/ambiguity/missing checks; ordinary Transmitter FromSpacecraft epoch handling follows the existing spacecraft initialization rule. Missing references can remain pending before owner initialization, and initialized updates require valid native initialization.

For tanks, the existing native mass/center/inertia routines evaluate a proposed tank array before publication. No science formula is replaced. Publication invokes the concrete virtual Copy into the same owned object. The actual tank/thruster/power/radio/plate identity therefore survives, including known covariance/STM owners and borrowers of that component root. Prepared thruster local coordinate systems and solar shadow evaluators transfer to the existing object, with owner bindings restored after native copy. Native readiness reflects successful preparation. Previous/current function-value dependency contexts remain leased through publication, with selected previous private references retained when necessary.

The concrete native Copy may still allocate and can fail during publication; such a failure marks the owner uninitialized and propagates. This is not a universal allocation-failure rollback or arbitrary external-plugin transaction. Nested owned error-model/FOV identities, arbitrary external borrowers and private-source notification provenance remain separate open work. Component matching still follows the legacy name-based notification contract within its family; it is not proof that every same-name private value receives only notifications from its actual declaration.

Typed single-property transfer
-------------------------------

GmatBase::CopyParameter now requires matching concrete RTTI/type names, resolves the destination through the source property label and checks its native type. It uses native setters for real/time, precise GmatTime, integer/unsigned/Boolean values, strings/filenames/enums/object selectors/colors/equations, On/Off, Boolean arrays, fixed-size signed/unsigned arrays, native typed string/reference lists, vectors, matrices and Generic values. Setter failures propagate. String/reference names retain native provider semantics; owner rebinding is performed by the concrete owner/provider rather than guessed in the generic method.

Typed Thruster Tank replacement validates the whole proposed name/link set and preserves native ordinal mix-ratio growth/default rules. SolarPowerSystem ShadowBodies replacement validates the whole list, retains actual existing bodies, supports explicit empty/no-body state and invalidates readiness. Other native list/reference resize providers remain required where generic names alone would leave stale ownership. Types without a native whole-value getter, including unspecified RealArray/RealElement cases, now report the missing provider rather than silently succeeding. No element coordinates or array sizing policy is invented.

FuelTank base assignment now calls its base initializer explicitly. Previously its virtual Initialize call could validate new base fuel data against old ChemicalTank leaf volume/model fields before those fields were copied. Concrete ChemicalTank/ElectricTank assignment retains its complete leaf initialization afterward; model formulas and physical limits remain native.

Sandbox clone-owner reinitialization now runs after either a whole-object or a property notification when an already initialized owner reports invalidation. ObjectInitializer is scoped with RAII, and a false owner initialization result raises an error. The previous code skipped this path entirely after property notifications and ignored a false initialization result after whole notifications. This uses the existing Sandbox object maps; it does not redefine all function/private-scope owner maps.

Compilation and scope retained
------------------------------

Configured compilation v1: 426/426 steps, 125.82 seconds, return 0.
Configured compilation v2: 66/66 steps, 8.94 seconds, return 0.

Twelve source/help paths have preimages before first edits/build v1, before/build/final hashes, indexed deltas, source map, build logs, terminal compiler receipts and four planning preimages. Final sources match the last successful configured build; all compiler sessions are terminal. Added-line whitespace is clean and help XML parses. Existing instance layouts and virtual slots remain unchanged: concrete classes override the existing typed SetField slot, new owner helpers are nonvirtual, and friend declarations add no instance data. Matching configured consumers rebuilt. External binary/plugin, platform and runtime compatibility remain unqualified. The existing OrbitRenderer.cpp QImage::mirrored warning is outside this batch.

The unchanged EphemManager.hpp non-SPICE declaration issue remains open with its prior captured-preimage failure and unchanged dependency hash. No known failing conditional compilation or acceptance audit was repeated.

Remaining work includes actual-source clone notification provenance, plate/other-hardware replacement, full antenna/error-model/owned-FOV and other resource graphs, whole-spacecraft assignment, other clone owners, typed resize/reference providers, cross-scope ownership/public edits/shared globals and every original function/resource/command/scientific clause. FRR-36, other specification dependencies, external dependencies and historical targets remain explicit. No all/any parent receives partial credit.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed.

Tracking-v80 remains 1093/1158 (94.39%) with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
