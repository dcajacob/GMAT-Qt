Shared subscriber invocation binding frames
==========================================

Nested function initialization can replace subscriber wrappers and parameter-pointer bindings shared with an enclosing caller. Reinitializing the enclosing command sequence had incidentally repaired some of those bindings while also resetting loop/solver execution. This change adds a scoped binding protocol for the common wrapper layer and the native ReportFile/XYPlot parameter-pointer lists.

Subscriber stages typed cold wrapper clones with actual source-to-copy alias tracking, validates concrete Clone identity/type, and allocates the full state before detaching live fields. The frame retains the actual enclosing wrappers, deletion queue and wrapper-generation leases. Restore swaps those original objects back, retires inner wrapper storage while enclosing environments remain owned, and validates internal-frame/solar-system references. Wrapper replacement validates slot shapes and stages both axes before mutation; unmatched names return false. Retired owners move outside live containers before destructor callbacks. ClearWrappers detaches requested arrays before releasing owners, preserves aliases still used by another axis, and the native subscriber destructor clears both axes. GmatFunction keeps each newly created wrapper in a private owner until acceptance and throws the previously unthrown initialization error on rejection. Frame destruction also retires detached owned wrappers when the subscriber itself was deleted; actual owner identity prevents restoration into a replacement object. Optional legacy borrowed-wrapper behavior remains unqualified.

ReportFile and XYPlot capture native parameter-pointer slots and validate their actual lifetime tokens. Their native parameter lists restore before common wrappers retire, so wrapper-deletion callbacks see the enclosing pointer slots. No report stream, native plot history, filename, formatting or solver-iteration configuration is copied or rolled back. Parameter-name definition changes remain present but invalidate old bindings and require the ordinary rebuild lifecycle; dynamic reconfiguration semantics still need full implementation. Missing/retired saved bindings also mark readiness false and report failure. These checks do not qualify arbitrary interior or concurrent destruction lifetimes.

FunctionManager captures shared subscribers from actual caller/global maps, deduplicates identities, and restores in reverse order after callee state restoration. The same scoped state unwinds after failed initialization/execution/result handling. Inactive cached-manager cleanup captures its own shared bindings around late native finalization; executing cleanup uses the already enclosing invocation frame. Configured native numerical algorithms, streams and plot histories remain responsible for their ordinary behavior. No alternate solver or frozen model is used.

Remaining scope
---------------

Other subscriber native reference/cache fields are not made complete by this common wrapper layer. Shared Parameter objects can retain owner/RefData caches independently of a subscriber pointer list; those binding frames remain required. Component-changing output Copy, legacy/shared/external-map ownership, direct entry/finalization/custom callbacks, arbitrary interior/service/engine-reset/later lifetimes, complete command/plugin copies and interpreted function value providers all remain open. No complete function I/O/core/scoping, any-command or resource-tree parent is credited.

Complete binding frames for the full native subscriber catalog, including OrbitPlot/OrbitView/GroundTrackPlot, EphemerisFile and enabled plugin reference/cache fields, plus shared Parameter owner/RefData bindings without rolling back explicit Global values or histories. Complete passive caller resource/command-cache rebinding when outputs replace owned components while preserving live solver/loop state. Finish legacy/shared/external-map ownership, direct/finalization/custom-manager reentry, all command/plugin cold-copy/ownership/reference and interpreted function value-provider routes. Continue every remaining original worklist requirement, including all body/kernel/NAIF/scientific/resource clauses and independent POV/rotation research. Keep full original scopes and verification deferral.

Compilation
-----------

v1: 294/294 steps, 268.15 seconds, return 0.
v2: 67/67 steps, 9.76 seconds, return 0.
v3: 294/294 steps, 254.4 seconds, return 0.
v4: 68/68 steps, 4.87 seconds, return 0.
v5: 67/67 steps, 5.26 seconds, return 0.

Final configured core/Qt/console/enabled-plugin/Python-consumer compilation passed. Subscriber adds one virtual binding-suspension route; ReportFile/XYPlot override it, and matching configured consumers rebuilt. One subscriber lease-vector field keeps restored wrapper generations alive until native owner retirement; definition copies keep it cold and assignment clears prior invocation leases. Lease pruning publishes the retained set before releasing orphan owners, permitting deletion callbacks to reenter. Source preimages, before/build/final hashes, indexed deltas, terminal receipts and four planning preimages are retained in ``build/nasa-coverage/coverage95-20261106-implementation-subscriber-binding-frames``. The first seven source preimages precede v1; the GmatFunction preimage was added after v4 and before its first edit. Final compilation covers all eight paths. Added-line whitespace is clean.

The previous handoff/worklist entries mislabeled their unchanged tally as tracking-v58; its actual worklist/tracking source was v59. This update corrects that narrative label only. No assertions, fixtures, tolerances, original requirement texts/fingerprints, denominator, ledger or gates changed. No runtime tests, GUI, missions, numerical comparisons, acceptance readers or seals ran. No packages, installed/shared runtime, commits or pushes.

Tracking-v60 remains 1093/1158 (94.39%) implementation present and 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal remains active and incomplete.
