Save default restoration and source-aware DE files
=================================================

Appended snapshots previously omitted some native default-valued properties. An earlier nondefault assignment could then survive reload even after a later Save recorded a resource that had returned to its defaults. Native SolarSystem, CelestialBody/Planet and SpacePoint policies expose this distinction through IsParameterEqualToDefault; Formation also supplies a native default comparison. The implementation uses these existing policies rather than treating all hidden fields as persistent data.

Default-valued native properties
-------------------------------

Under ObjectScriptExportScope and script export modes, the common serializer includes a default-cloaked root if it has a writable property with a native default match. Parameter writing retains the read-only gate and permits a cloaked property only when the native comparison identifies its current default value. Unrelated hidden fields retain their policy. Native cloak/default metadata and per-object source/force-generation flags are unchanged.

Supported empty default strings and string/object lists are emitted too. Existing nonempty serialization, specialized writers, String snapshots and array-zero resets retain their behavior. Ordinary script saving outside the explicit export scope remains unchanged. The rule depends on each native class's declared default semantics; it does not invent missing default comparisons or prove that every specialized serializer reconstructs all state.

Interpreter routes an explicit empty DSNMediaFileDirectories list to CelestialBody's existing complete-clear setter. Existing SpacePoint kernel-list clears remain intact. Read-only fields, including non-Earth Earth-only settings, are not made writable.

Earth override clearing
-----------------------

Planet setters now restore the startup EOP file or clear the global ionosphere override when a previously nonempty Earth property is explicitly cleared. The ionosphere getter then uses the configured startup IONOSPHERE_PATH. The EOP reset follows the existing Moderator startup-file convention. A field already empty does not reset an unrelated host setting. Ordinary initialization still applies authored nonempty overrides, with a null EOP-provider guard for cold hosts.

This fixes the backend selection accompanying a clear rather than only changing the visible stored string. Numerical algorithms are unchanged; selected data can change as intended when an explicit override is removed. Complete shared-service ownership, nested caller/global lifetimes and other mission-boundary restoration remain open.

Consistent DEFilename lookup
---------------------------

A single native helper selects the active DE405/DE421/DE424 family, or the most recent configured DE family while SPICE is active. Getter, setter and native default comparison now agree. The former getter/default comparison assumed list element 1, while the setter used the active SPICE index as a DE file type.

Changing the remembered DE filename keeps the active SPICE source unchanged and uses the existing body SetSourceFile flow. No selected DE family produces an empty getter and no default filename export; an assignment requires a DE family selection rather than guessing its type. Existing supported DE families and ephemeris/force formulas are unchanged.

DE reader replacement and relative paths
---------------------------------------

CreateDeFile validates the DE type before touching the previous reader. A same-file fast path also checks the reader's existing DE type. It probes the original path and the existing GMAT-working-directory fallback with owned file handles. Successful fallback now reaches construction using that resolved path; previously that branch could leave a probe open and never construct a reader.

The replacement DeFile is allocated and parsed before retiring the previous owned reader. A failed probe or constructor therefore preserves the usable previous pointer. Successful creation retains the existing caller-driven body rebinding sequence. This does not make all ephemeris-source activation atomic: the broader source setter can still change flags before another operation fails, and complete borrower/shared-file/callback lifetime behavior remains open.

Compilation and boundaries
---------------------------
Configured compilation v1: 69/69 steps, 11.71 seconds, return 0.
Configured compilation v2: 67/67 steps, 8.61 seconds, return 0.
Configured compilation v3: 66/66 steps, 5.79 seconds, return 0.

The first compilation reported a function-pointer attribute warning for the fclose deleter. A stateless callable removes that warning; the final compilation has no warning/error diagnostics. v2 also narrows backend resets to explicit previous overrides, preserving the native nonempty-only initialization policy.

Five source/help paths have preimages before first edits, before/build/final hashes, indexed deltas, source map, logs and terminal compiler results. Final hashes match v3; all compiler sessions are terminal. Added-line whitespace is clean and the help XML parses. No existing class instance layout, public method signature or virtual slot changes. Configured consumers rebuilt/relinked as required; external binaries and Windows/macOS runtime/toolchain behavior remain unqualified.

No runtime/GUI/mission/numerical tests, acceptance audits/readers, evidence admission, assertion/fixture/tolerance/denominator changes, packages, installed/shared runtime changes, commits, pushes or external messages were performed. The original full any-Resource, function/resource/reference/host-service and scientific requirements remain open. Partial implementations receive no parent or verification credit.

Tracking-v72 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
