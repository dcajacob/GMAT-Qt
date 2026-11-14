Subscriber retained value graphs
================================

Function resources need their own object graph, not just a native object with shallow Parameter pointers. ReportFile, XYPlot, DynamicDataDisplay and MessageWindow now disclose exact-type reference providers. Unimplemented derived types do not inherit admission. This advances the whole-resource input/output path; no complete all-resource/function/command requirement is credited.

Owned graphs and native references
----------------------------------

Subscriber inventories wrapper-owned GmatBase objects and pairs their independently copied identities. It verifies both axis shapes, null positions, native wrapper types, repeated aliases and reverse ownership mappings. Split aliases, merged distinct wrappers/components and reused source objects are rejected. Optional borrowed-wrapper native builds promote retained values to independently owned wrapper graphs. Owned identities register before any reference provider runs; nested supported resources use the full native binder, and expression trees reconnect their structural nodes through their wrappers.

ReportFile maps its Parameter vector; XYPlot maps its X Parameter and all Y Parameters; DynamicDataDisplay maps each cell Parameter while keeping configured values and style and clearing its dormant legacy wrapper cache. MessageWindow has the common environment only. The common binder resolves internal/data frames, MJ2000 origin, solar system and wrapper references by actual object identity. A new copy starts with a fresh physical plot identity; assignment retains the destination output identity and native pending rename. Active publisher packets are not retained as values. Ephemeris remains on its specialized provider path, with common wrapper-owned objects/environment now registered as well.

Native completed Clone, Copy, supported assignment and retained copy-construction hooks reconnect after the most-derived native state exists. GmatBase exposes a nonvirtual native-copy guard query, so an enclosing owner can defer hooks while constructing components. Native copies without an existing context use the same retained graph builder, with external definitions borrowed and wrapper-owned resources independently mapped. Parameter/callable references to the already registered subscriber root can form a passive reference graph without evaluating the subscriber during construction.

Copy retirement
---------------

Before a supported assignment mutates fields, an identity-specific token captures source/destination wrapper leases and both contexts. Nested copies are guarded so base assignment cannot signal completion early. The target keeps this retirement chain if native assignment or graph binding fails; old fields cannot lose their old wrapper-owned dependencies merely because common assignment retired its arrays. A later successful completion detaches only its matching token, commits enclosing binding scopes and releases captures after publication. Enclosing subscriber binding frames independently retain their original graph context until their saved wrappers retire, including dismissed frames after a value commit. A nested operation's state is not cleared by an enclosing token. Native assignment is not a whole-resource rollback transaction; partial values after failure and arbitrary self-mutating/evaluation reentry still require further implementation and qualification.

Readiness and ordinary use
--------------------------

Subscriber assignment clears both initialization flags and finalization readiness. Valid retained wrapper roles are reused by GmatFunction and ObjectInitializer, preserving their copied identities instead of recreating references from another same-name scope. A changed declaration falls back to ordinary wrapper rebuilding; complete configuration-edit/provider refresh remains open. Existing initialized shared global subscribers keep their separate preservation path.

Retained subscribers stay dormant during graph preparation. Data reception and DynamicDataDisplay updates initialize a cold retained output on first use, after role checks; value capture itself opens no subscriber writer or window. ReportFile assignment closes an existing stream when its filename/path/append mode changes and reports close failure. Unchanged writer configuration retains the destination stream. Existing native filename, append, header and initialization policies remain in place; file collisions, output history and renderer/session behavior are not qualified by compilation.

Compilation
-----------

v1: 748/748 steps, 417.3 seconds, return 0.
v2: 170/748 steps, 37.27 seconds, return 1.
v3: 579/579 steps, 287.13 seconds, return 0.
v4: 295/295 steps, 306.93 seconds, return 0.
v5: 66/66 steps, 8.75 seconds, return 0.

The final configured core/Qt/console/enabled-plugin/MATLAB/Python 3.14 consumer build passed. v2 exposed a mistaken numeric assignment to the existing binding-epoch identity pointer; the assignment was removed before v3. Sixteen source paths have preimages, before/build/final hashes, indexed patches, source map and terminal receipts. Thirteen preimages precede v1; FunctionManager.hpp, ObjectInitializer.cpp and GmatFunction.cpp were added after v1 and before their first edits. All sessions closed before finalization; final source matches the final successful compiler snapshot. Added-line whitespace is clean.

Two Subscriber virtual routes and four cold runtime fields were added; new GmatBase/FunctionManager helpers are nonvirtual. Matching configured consumers rebuilt. Unconfigured/platform/older binary plugin compatibility remains unqualified. No runtime tests, GUI, missions, function/model/solver evaluation, numerical comparisons, acceptance readers or seals ran. Existing test executables may be linked but were not run. No packages, installed/shared runtime, commits or pushes. Assertions, fixtures, tolerances, original texts/fingerprints, denominator, ledger and gates remain intact.

Remaining scope
---------------

Complete the remaining OrbitPlot/OrbitView/GroundTrackPlot/GroundTrack, owned-plot, TextEphemFile, MATLAB workspace and callback value providers, including renderer/native histories, owned frames/interpolators/writers, engine/session/force-container lifetimes and opaque callback ownership. Finish self-mutating subscriber evaluation, direct/configuration-property edits and arbitrary callbacks/old-origin/scope/interior/reset/later lifetimes; qualify private Qt camera/vector/sensor/playback copies, file destinations, native output activation and exceptional native mixtures. Complete shared Parameter owner/RefData/global-resource interiors, component-changing output/cache repair, legacy/shared/external-map ownership, direct/finalization/custom-manager reentry, all command/plugin and interpreted-function providers. Continue every original body/kernel/NAIF/scientific/resource requirement and independent POV/rotation research; preserve full scope and verification deferral.

The four admitted native providers do not establish full caller/recursive/exceptional function I/O or every public configuration edit. Other subscriber types do not yet disclose retained reference providers, including owned plots and opaque external resources. Native histories, already-retired or arbitrary borrowed interiors, direct property edits, ownership through reset/later use and self-mutating callbacks remain open. Legacy OpenGlPlot retains its previously recorded unconfigured compilation issues; this batch does not qualify it.

Tracking-v64 remains 1093/1158 (94.39%), with 65 other rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
