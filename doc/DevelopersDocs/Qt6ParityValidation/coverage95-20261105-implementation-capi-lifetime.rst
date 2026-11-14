C-interface model lifetime and derivative storage
=================================================

This implementation-first increment repairs demonstrated defects in
``CInterfacePluginFunctions.cpp``. The public C header adds a comment clarifying
run-scoped model/state lifetime; signatures and object layouts are unchanged.
No new tests, fixtures, assertions, numerical tolerances or registrations change.

Runtime selection is cleared before engine initialization, script loading and
mission reruns. These operations can replace sandbox objects on both success
and failure. Selected pointers and lookup tables must not continue to refer to
the previous sandbox. Model lookup requires an initialized run, and every search
restarts its local propagator traversal. Legacy command preparation is tracked
per run so subsequent searches do not repeatedly prepare an earlier command and
overwrite assigned propagation state. Null/stale results and the exception type
in the legacy indexed helper are repaired. IDs retain existing run-local numeric
conventions; clients must rediscover after session mutation.

Derivative storage formerly allocated once at the first returned dimension. A
later larger model could overrun that allocation. Each accessor now owns a
separate vector sized for its current result. Native failure returns null and a
zero dimension. Input/dimension pointers, sizes, derivative order and finite
numeric inputs are checked at the affected entry points. State error diagnostics
do not read six input elements merely to report an oversized input. Status text
uses dynamically sized strings/streams. Native/standard/unknown exceptions in
the affected loader, run, lookup, state and derivative paths become C status or
null returns. This does not add atomic rollback of every native state mutation.

Build and reused checks
-----------------------

Review artifacts are under
``build/nasa-coverage/coverage95-20261105-implementation-capi-lifetime``.
Final compile/link records are in ``build-v3``. One affected translation unit
compiles and the private library links without diagnostics. Older intermediate
compilations and candidates remain. Exact preimages, final file hashes and the
incremental patch isolate the change from the larger dirty checkout. The patch
passes reverse-application and added-whitespace checks.

The unchanged external pure-C client ``GmatNasaCapiAcceptanceTests`` passes its
existing ``basic`` and ``stm`` modes: 133 and 481 assertions, respectively. Each
uses two distinct native ODE models and its original independent orbital/force
expectations. Dimensions are six and 42 in separate processes. State assignment,
selectors and derivative comparisons pass. The client's own loaded-image output
identifies the private candidate and excludes the older installed CInterface.
No GUI execution or whole-row admission is claimed.

Replay uses each smoke directory's ``command.json`` from its ``session`` working
directory, with ``build-v3/plugins`` prepended to ``LD_LIBRARY_PATH``. The saved
``gmat_startup_file.txt`` uses the private plugin's extensionless PLUGIN value.
The saved scripts come from the existing ``RunCapiAcceptance.fixture`` builder;
no new scientific input was invented. Retain old result directories when
replaying. The private link reuses existing support objects/base libraries and
does not prove a full build of all worktree changes. Installed binaries, public
main and released packages remain unchanged. All processes finished.

Deferred qualification
----------------------

Backfill successful/failed reload and rerun invalidation, query-order changes
after user state assignment, mixed-size model switching within one process,
invalid arguments and exceptions. The existing smoke processes cannot verify
the mixed-size allocation repair or all new guards by themselves.

Legacy lookup still calls native command preparation. Full branch/function
traversal, stopped-run behavior, ephemeris/numerical mixtures, every dynamics
family, ABI platforms and full current-build/scientific qualification remain.
The existing not-ready LoadAndRunScript contract is unchanged. These finite
checks do not discharge All Others!E249/E251/E252. Verified coverage remains
1053/1158, with 48 additional qualified rows needed for 1101; all original hold
identities, assertions and historical coverage remain.
