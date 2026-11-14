Parent workspace preservation for nested MATLAB optimizers
=========================================================

The existing helper saved seven globals, while the legacy fmincon driver also reads and writes ordinary base-workspace variables. The parent script assigns X, fVal, exitFlag and output metadata before its final CallGMATfminconSolver(X). That callback can execute a child optimizer. The child could replace those variables before the parent C++ Optimize reads exitFlag. Preparation also writes diagnostics before the prior solve-only guard begins. This packet implements named workspace preservation at both preparation and solve boundaries.

Capture and restoration
-----------------------

The new GmatQtOptimizerWorkspace helper captures 28 fixed driver/global/diagnostic/startup names and existing private evaluation-status names. It records base existence, base local/global binding, base value, independent global-store existence and global value. A global may exist without a base binding, and a same-name base local/global-store pair is preserved separately. Capture does not declare missing globals. Native MATLAB values remain in the helper; no value conversion through the restricted JSON numeric/structure codec is needed. Function handles and option structures retain ordinary MATLAB assignment semantics.

Restoration first applies global-store values without clearing compatible existing globals, then restores or removes base bindings. It distinguishes a missing binding from an empty value and removes a child's newly introduced private status suffixes. A base name is cleared only if its variable actually exists, avoiding accidental clearing of an unrelated function. Unrelated user variables remain shared. These are known internal driver names, not a snapshot of the entire MATLAB process or arbitrary user-variable namespace.

Callback ownership and failure cleanup
--------------------------------------

Each Qt callback enters a private checkpoint with session identity, non-reused callback ID and starting workspace depth. Push/pop scopes are owned by that callback and checked in LIFO order. Normal completion explicitly leaves the checkpoint before publishing the callback result. An onCleanup path repeats leave idempotently and unwinds remaining child captures after failed or unacknowledged native scope operations. The callback cleanup is independent of socket deletion order.

A capture remains in the helper until restoration succeeds. Partial restoration failure can therefore be retried as the callback unwinds. Native cleanup still does not replay an ambiguous external pop acknowledgment. A new session cannot silently discard active owners. Cleanup failures on an exceptional path are reported without replacing the original exception; successful paths report restoration failure directly. This is source-level ownership/recovery implementation, not proof of transport failure atomicity or process-death recovery.

Native preparation and results
------------------------------

One internal noncopyable C++ scope replaces the solve-local global guard and is used by OpenConnection as well as Optimize. Preparation scratch/startup values are restored before returning. Initialization failure leaves readiness false. A solve clears stale result state before capture, reads its own exit flag into native storage before restoring the parent and publishes convergence only after restoration succeeds. Non-nested/legacy requests retain their original path because the bridge guard is inactive there.

GmatQtMatlabJob delegates push/pop to the workspace helper and no longer declares unrelated globals for every job. CMake tracks and copies the client, job helper, new workspace helper and unchanged driver together. The legacy optimization driver and gmat_startup scripts remain byte-identical to their reference preimages; no script/function interface conversion or numerical formula change is introduced.

Compilation and compatibility
-----------------------------

Configured C++ build v1: return 0, 4.36 seconds, final printed progress 5/6.

Terminal exit status establishes build completion; the final Ninja progress counter is retained verbatim. There are no warning/error diagnostic lines. Seven participating existing source/help preimages, one new helper, two unchanged reference preimages and four planning snapshots preceded edits. Indexed patches, before/build/final hashes, source map, compiler log/terminal receipt and four matching build-tree helper/driver hashes remain in the stage directory. Final sources match the successful build capture; delta whitespace is clean and three help XML files parse.

The MATLAB files were not parsed or executed. C++ compilation and helper-copy identity do not qualify nested round trips, binding/absence semantics, optimizer outcomes, recursive cleanup, Unicode/large values or cancellation. Existing instance layouts and virtual slots are unchanged. Matching configured consumers built; external binary and other-platform qualification remain undone. The workspace helper/client set must match the backend's nested protocol support.

Ordinary MATLAB handle assignment retains identity rather than deep-cloning arbitrary handle objects. Deliberate deletion of globals can destroy aliases in other live function workspaces; arbitrary user-code side effects, custom startup behavior, MATLAB path/current-directory/format state, private helper replacement, process termination and full native solver/configuration state remain outside this named-variable provider's completion claim. All broader original clauses remain open.

Primary documentation
---------------------

* https://www.mathworks.com/help/matlab/ref/whos.html — Workspace/global metadata distinguishes variable existence and global base bindings.
* https://www.mathworks.com/help/matlab/ref/who.html — Global workspace inventory is separate from names visible in the base workspace.
* https://www.mathworks.com/help/matlab/ref/global.html — Global declaration shares a store across function workspaces and can change a same-name local binding.
* https://www.mathworks.com/help/matlab/ref/clear.html — Ordinary clear removes a workspace binding while a global value can remain; clear global removes the shared global.

Remaining implementation
------------------------

Finish complete nested solver/session lifecycle and transport interruption/acknowledgment semantics, including output/calling-name scopes, active endpoint/resource lifetime, changed solver configuration and supported-platform behavior. Preserve arbitrary MATLAB handle and global alias semantics within their actual contracts; do not infer full-session rollback from known driver-variable captures. Complete original function I/O/scoping/resource/property/output staging, native all-command optimizer/target behavior, external caller provenance, shared globals/dependency retention and supported client protocols. Finish coherent changed-component/membership/topology/storage preparation, state/result/posterior and estimator/smoother semantics, public restoration-buffer contracts, shared external values/lifetimes and source/stream/participant/error/noise/hardware providers. Continue private renderer, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool, ephemeris B-1/format/final-iterative/scientific and all specification/external/historical dependencies, plus independent POV/rotation research. Defer runtime verification until the full implementation pass is complete.

No runtime/GUI/MATLAB/helper/model/mission/numerical tests, new tests, acceptance auditors/readers/seals or evidence admission occurred. Assertions, fixtures, tolerances, denominator, gates, fingerprints, original source text and dispositions are unchanged. No commits, pushes, application packages, installed/shared runtime updates or external messages occurred.

Tracking-v105 remains 1093/1158 (94.39%), with 65 unresolved rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. No row or verification credit is claimed. Full implementation audit incomplete; goal active.
