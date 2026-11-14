Subscriber value commits and shared global continuity
====================================================

A function binding snapshot must distinguish temporary lookup changes from an explicit whole-object value assignment. Restoring the old bindings after a successful subscriber Copy could overwrite the requested resource value. The new commit signal runs after a successful ObjectWrapper Copy of a distinct source and actual subscriber target. Identity operations and failed Copy do not signal a commit.

SharedSubscriberBindings now belongs to the subscriber module and registers actual captured subscriber identities in a thread-local scope chain. A commit dismisses that target through every active enclosing scope; unrelated entries still restore. Dismissal suppresses all layered binding actions while retaining captured owners until normal scope retirement. State destruction retires leaf capture storage before enclosing dependencies. Scope restoration unregisters entries before capture destruction, so nested cleanup cannot inspect a vector being retired. Native direct Copy paths outside ObjectWrapper, reentry during a self-mutating subscriber evaluation, concurrent ownership and later callbacks remain unfinished.

The shipped function guide, ``doc/help/src/Resource_GmatFunction.xml`` (Global Spacecraft, Subscribers and Other Objects), specifies continuous output for globally shared spacecraft/subscribers across function calls. The shipped ``Ex_GMATFunction_UsingGlobalObjects.script`` and ``Ex_GMATFunction_TargetInsideFunction.script`` declare those shared resources. Source inspection found only a step-mode plot exception in ObjectInitializer; ordinary initialized global subscribers could still be rebound against function locals and have their native output reset. No sample or mission was executed.

Function initialization now reuses an initialized subscriber whose actual pointer occurs in the global dictionary, including an alias also present in the local dictionary. It preserves the current native stream, plot history, sample cursor and bindings rather than invoking their initialization again. Registered subscribers match publisher entries by actual identity, preventing same-name local objects from entering the wrong initialization path. Unregistered/new/local or uninitialized subscribers retain the ordinary build/initialize/subscribe path. GmatFunction also reuses initialized shared wrapper bindings and leaves their existing ephemeris force context installed. Main mission initialization remains native. These are implementation changes, not runtime qualification of continuity or numerical output.

Remaining scope
---------------

All original function/resource/scientific clauses remain in scope. Cold/new/local subscriber graphs, native writer/renderer refresh after copied values, every direct API/native Copy hook, Parameter owner/RefData and other global-resource interiors, component-changing outputs, subscriber self-mutation, owned subscriber cleanup, external-map and arbitrary interior/service/engine-reset/later lifetimes are not proved complete by these changes. No complete function I/O/core/scoping, any-command or resource-tree parent is credited.

Complete cold/new/local subscriber value and reference providers across the full native catalog, including native writer/renderer rebinding, self-mutating subscriber callbacks and all direct API/native Copy paths. Finish shared Parameter owner/RefData and global-resource interior bindings; complete passive caller resource/command-cache repair when outputs replace owned components while preserving native values, histories, solver/loop state and explicit Global changes. Finish legacy/shared/external-map ownership, direct/finalization/custom-manager reentry, all command/plugin and interpreted function value-provider routes and arbitrary scope/interior/service/engine-reset/later lifetimes. Continue every remaining original worklist requirement and independent POV/rotation research. Preserve full original scope and verification deferral.

Compilation
-----------

v1: 295/295 steps, 223.55 seconds, return 0.
v2: 67/67 steps, 3.62 seconds, return 0.

The final configured core/Qt/console/enabled-plugin/Python-consumer build passed. SharedSubscriberBindings has an exported pimpl interface; FunctionInvocationState adds a private dismissal route for that scope. No existing instance fields or virtual slots were added. Matching configured consumers rebuilt. Six production preimages precede v1; ObjectInitializer/GmatFunction preimages were added after v1 and before their first edits. Final compilation covers all eight paths. Preimages, before/build/final hashes, indexed deltas, source map, logs, terminal receipts and four planning preimages are retained in ``build/nasa-coverage/coverage95-20261106-implementation-subscriber-value-commits``. Added-line whitespace is clean.

No runtime tests, GUI, missions, function/model/solver execution, numerical comparisons, acceptance readers or seals ran. No packages, installed/shared runtime, commits or pushes. Original assertions, fixtures, tolerances, requirement texts/fingerprints, denominator, ledger and gates remain unchanged.

Tracking-v61 remains 1093/1158 (94.39%) implementation present and 65 other rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal remains active and incomplete.
