GetEphemStates destination-owned frame aliases, 2026-11-06
=========================================================

The prior v53 batch finalized 17 production paths and two successful builds.
This batch changes two production paths and makes concrete source progress.
The full implementation goal remains active and incomplete.

Copy and ownership
------------------

The explicit rejection of a source Function borrowing this destination's owned
file frame is replaced by shared lifetime records for actual frame identities.
A weak registry deduplicates owners; it holds no strong frame references. Owner
allocation and registry insertion finish before raw ownership is adopted, and
actual GmatBase lifetime tokens distinguish retired/reused objects at release.
Native owned frame/axes clones remain independent and typed. Two shared lifetime
fields and nonvirtual helpers were added; no new virtual slots.

Assignment stages the new frame and lease vectors before copying values. A
source borrowing the old destination frame through its direct output/internal
cache or declared retained context retains that actual frame independently of
this destination, including assignment failure and later target destruction.
A source sharing the primary component uses its actual registered owner. The
copied destination remaps its source-owned primary aliases onto the new frame.
Copy construction also preserves an internal alias to its own frame while other
ordinary invocation services remain cold. Active callbacks are not copied.

All four native SPK/Code500/STK/CCSDS frame replacements construct and adopt the
new frame before publishing it. Existing direct internal/output aliases keep
their actual old frame through leases. Finalization clears internal aliases to
released owned/retained frames; finalization and destruction release each actual
owner once. The existing native component inventory and retained graph binder
still distinguish primary ownership from borrowed lifetime leases.

This closes the located direct-alias rejection, not every lifetime or Copy
requirement. Arbitrary other scope-table/interior borrows, external deletion,
concurrent destruction, engine reset, source/ABI/platform compatibility and all
broad original function/resource/command/scientific cases remain unqualified.
No scoped/direct native case substitutes for the original universal clauses.

Compilation and accounting
--------------------------

Stage: ``build/nasa-coverage/coverage95-20261106-implementation-getephem-frame-alias-copy``.

* v1: 67/67 build steps, 4.75 seconds, return 0.
* v2: 67/67 build steps, 4.95 seconds, return 0.

Final production hashes match build v2. Production preimages, before/build/final hashes, indexed patches, source map, logs/terminal receipts, summary and four planning preimages are retained. Both build processes terminated. Added delta whitespace: zero. Matching configured core, Qt, console, enabled plugins and Python consumers rebuilt.

No tests, GUI, missions, functions, models, solvers, numerical comparisons, acceptance readers or seals ran. No packages, installed/shared runtime, commits, pushes or external messages. Original assertions, fixtures, tolerances, source texts, fingerprints, denominator, ledger and gates remain intact.

Tracking-v54 retains 1093/1158 = 94.39% implementation present, with 65 other rows. Recorded source-versioned qualified baseline remains 1053/1158 = 90.9%; all 105 original acceptance rows remain open. No new row or verification credit; full implementation incomplete.

Next
----

Complete whole nested function invocation state and native output-name/cache contracts, legacy discovery, inout aliases, external-map lifetimes and custom vector arithmetic/type inference. Complete every remaining solver/estimator/simulator/resource provider, owned plot/subscriber/interface/measurement resources, command/plugin ownership/reference/retirement and passive UserDefinedFunction/GmatFunction body/prototype admission. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings and all arbitrary ownership/interior/scope-table/service/engine-reset/later lifetime cases, including GetEphemStates aliases outside its direct cached frame fields and declared retained contexts. Finish all original body/kernel/NAIF/scientific/resource requirements. Preserve full original scope and verification deferral; research POV and unresolved rotation semantics independently without inventing specifications or asking again for a POV dialect.
