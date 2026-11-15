Native body ephemeris cache coherence
====================================

A raw-state cache previously reused public last-ephemeris epochs without an explicit valid flag. A same-body relative query could publish a zero vector into that cache, initial or copied epochs could imply validity, and configuration changes could leave same-epoch states stale. The DE caller-buffer overloads also reassigned their local pointer rather than copying the provider result into the caller's array, and their cache-hit branch did not return.

State queries and time identity
-------------------------------

CelestialBody now keeps an explicit raw-state validity flag and GmatTime cache epoch. Canonical A1Mjd and GmatTime queries require a successful cached calculation, preserve the configured EphemerisUpdateInterval cadence and existing derivative bypass, and refuse SPICE cache hits while setup is pending. Fresh computation invalidates first and marks valid only after native success. Both public time forms synchronize on success; cache hits restore the public state/time views from the cache's actual calculation epoch rather than changing the cache key to the later requested epoch.

Both caller-buffer overloads validate the output pointer, call the canonical state query and copy six values. They share DE, SPICE, analytic, cache, precise-time and error behavior. A failed query does not fill the caller's buffer from a partial local state. Raw self-origin zero queries explicitly invalidate and use actual pointer identity; state/acceleration zero optimization no longer treats distinct same-named bodies as identical. The acceleration implementation retains its existing analytic formula and shared finite-difference cache bypass, stencil, boundary handling and center restoration. There is no new separate acceleration cache.

Native configuration and dependencies
-------------------------------------

CelestialBody invalidates on native source/file descriptor, time override, update interval, mu/potential settings, analytic elements/epoch, central/reference identity, NAIF and kernel changes, as well as initialization/copy/rebinding. A parent mu or provider change can affect analytic descendants even when the parent's own source is DE or SPICE. SolarSystem therefore invalidates actual body/special-point members together. A non-null notifying body must be a member by actual pointer before broadcasting; a passive clone borrowing the same SolarSystem pointer only invalidates itself. The own-body invalidator does not recursively broadcast or allocate.

SpacePoint's existing nonvirtual J2000 name/object setters notify actual CelestialBody instances without adding a virtual slot. Existing SetSolarSystem and kernel-removal virtual routes are overridden. SPICE resets clear setup and NAIF/observer readiness while retaining the user's explicit target ID; switching SolarSystem clears the borrowed kernel-reader pointer. SpecialCelestialPoint source changes participate. Validated global SPK/PCK/LSK filename changes invalidate member caches and SPICE readiness.

This resets reviewed native cache dependencies. It does not implement arbitrary external mutation detection, external reader ownership, a full private solar-system clone provider, coordinate-system rotation cache replacement, global CSPICE kernel-pool transactions, or complete kernel precedence/unload semantics. Those scientific/lifetime clauses remain open.

Rollback state
--------------

SolarSystemEphemerisTransaction now captures every actual body and special point, including user-defined bodies affected through dependency invalidation. Cache validity/precise epoch, state/last-state/J2000/previous-state values, public Real/GmatTime epochs, time override/interval and native NAIF readiness/IDs/name join its rollback record. SPICE builds also retain the borrowed reader and main-kernel descriptors. Global PCK/LSK names and time/interval settings join existing source metadata snapshots. Native time-override and interval edits share this update boundary so a rejected receiver does not leave earlier members partially changed. Native snapshots restore before abandoned-reader retirement, retaining the previous nested-retirement design.

The snapshot describes the reviewed native fields. It is not a rollback guarantee for arbitrary virtual side effects, external callbacks or the process-global CSPICE pool.

Compilation and retained status
-------------------------------

Configured compilation v1: 601/601 steps, 140.97 seconds, return 0.
Configured compilation v2: 67/67 steps, 4.11 seconds, return 0.
Configured compilation v3: 66/66 steps, 4.02 seconds, return 0.
Conditional non-SPICE syntax, src/base/solarsys/SolarSystem.cpp: 0.48 seconds, return 0.
Conditional non-SPICE syntax, src/base/solarsys/CelestialBody.cpp: 0.46 seconds, return 0.
Conditional non-SPICE syntax, src/base/foundation/SpacePoint.cpp: 0.38 seconds, return 0.
Conditional non-SPICE syntax, src/base/solarsys/SpecialCelestialPoint.cpp: 0.34 seconds, return 0.

Conditional compilation uses the configured Linux compiler with __USE_SPICE__ omitted. It does not link or qualify a full non-SPICE application, other platforms or runtime behavior. Seven source/help paths retain preimages before first edits, before/build/final hashes, indexed deltas, source map, logs and authoritative terminal compiler results. Final sources match configured v3 and the conditional syntax snapshot. Added-line whitespace is clean and the help XML parses.

Source review caught and corrected an early-return placement introduced while editing the assignment path; final v3 retains the self-assignment guard and invalidates before and after ordinary assignment. The intermediate builds and source snapshots remain retained.

Two CelestialBody instance fields change its layout. Existing public signatures and virtual slots remain intact; existing virtual routes have overrides and the cache helpers are nonvirtual. Matching configured consumers rebuilt. External binary/plugin compatibility remains unqualified.

No runtime/GUI/mission/numerical tests, acceptance audits/readers/seals, evidence admission, assertion/fixture/tolerance/denominator edits, packages, installed/shared runtime changes, commits, pushes or external messages were performed. No original all/any parent earns partial credit. Tracking-v74 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
