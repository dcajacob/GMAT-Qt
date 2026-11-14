Subscriber event identities and native ground-track metadata
============================================================

Plot and ephemeris callbacks previously identified affected objects only by names. A function can rename a copied spacecraft to a formal argument, and independent retained objects can share the same name. Correct native references and identity-bound Cartesian samples were therefore insufficient: color changes and maneuver/property notifications could miss their actual target or affect another scope.

Identity-bearing event dispatch
--------------------------------

Publisher gains nonvirtual overloads for orbit/target colors, segment colors, spacecraft-property changes and scalar/vector maneuver notifications. Existing exported signatures and Subscriber virtual callback signatures remain in place. Overloads stage a synchronous subject snapshot with actual weak generation identities, then dispatch through the existing entry points. Repeated actual subjects are deduplicated; null subjects are rejected. No event owns or clones the subject resources.

Each legacy entry point installs an event-kind/originator-specific dispatch scope and consumes the request before invoking subscribers. Nested legacy notifications cannot inherit an outer event's subjects, even when the kind and originator match. Nested typed notifications restore the enclosing event on return or exception. Identity queries distinguish an authoritative empty or expired subject set from a legacy event; a retired/reused address cannot select a new object or trigger a name fallback. Origin lifetime checks also preserve authoritative absence after retirement. Existing global maneuver flags retain their native semantics.

Assignment supplies the actual object/property owner. Vary supplies the actual changed spacecraft, including Parameter owners. Propagate supplies its actual spacecraft roots and bound formation members for all three segment-color call sites. Impulsive burns, finite burns and thrust-file begin/end commands supply the actual maneuvered spacecraft. Estimator/smoother property notifications supply each actual spacecraft. External name-only publishers remain source and binary entry-point compatible and may opt into the new overloads.

Plot and ephemeris consumers
----------------------------

OrbitPlot matches typed subjects against its bound SpacePoints and translates matches back to logical curve/color roles. Both default/current orbit and target palettes and segment override/reset maps use those roles. Legacy callers retain name-based behavior, including configuration callbacks before references are bound. OrbitView's existing unrelated three-argument segment overload is unchanged; Publisher continues using the inherited four-argument virtual route.

GroundTrack keeps stable satellite and station declaration names parallel to its actual reference arrays. SetRefObject updates a role rather than searching for an object's current instance name, and stages both arrays together when appending. Native value copies and reference rebinding preserve roles; binding frames capture/restore them with the corresponding references, Clear clears them, and RenameRefObject updates them. Native readiness checks role/object shapes and declared membership. Curve labels, availability masks, palettes and segment actions use logical roles, while Cartesian samples continue selecting actual object columns. Repeated references under distinct declared roles can remain separate curve slots.

EphemerisFile maneuver and spacecraft-property handlers use actual subject identity when present, retaining the legacy name path otherwise. An unrelated property event now returns before changing the writer's event epoch. PropagationEnabledCommand scopes both finite-burn segment-discovery/cut loops to the ephemeris resource's actual spacecraft, including formation members, instead of cutting every enabled ephemeris subscriber. This does not alter force/integration calculations or ephemeris format rules. Subscriber's scalar maneuver adapter now uses a local StringArray, removing a shared static buffer that could be overwritten by nested notifications.

Ground-track native metadata
-----------------------------

Function-private physical plot keys have no configured object in Moderator's main object store. GroundTrack now supplies its actual line width, retained point count, redraw count and footprint setting to Qt after resetting a window. It resolves the actual body's texture path before publication. Grid/update-frequency behavior keeps its existing native route.

Native satellite palettes are published after initial curve naming, label updates and ordinary collected samples. Qt applies those role-keyed palettes, solver colors and segment overrides directly. Native metadata no longer depends on finding a configured spacecraft with the same role name; legacy providers without palette metadata keep their configured-object fallback.

Native ground stations publish a quoted logical role, longitude, latitude and color from the actual bound BodyFixedPoint. The location convention matches the former receiver's GetSphericalLocation(21545.0), with the same degree conversion and longitude reduction. Qt validates the metadata and upserts the station by role, without looking up a main-script peer. The old AddStation action remains as a receiver-compatibility fallback. This is display metadata only; it does not change trajectory, station frame or latitude/longitude computation used by the engine. Other receiver implementations have not been qualified for function-private station metadata.

Compilation and remaining scope
--------------------------------

Configured compilation v1: 235/235 steps, 214.5 seconds, return 0.
Configured compilation v2: 71/71 steps, 12.6 seconds, return 0.

All changed C++ translation units are in the configured core/Qt/plugin build. Twenty source paths have preimages, before/build/final hashes, indexed deltas, a source map and compiler logs/terminal receipts. Eighteen preimages precede v1; Subscriber.cpp and QtPlotReceiver.cpp were added after v1 and before their first edits/v2. All compiler sessions are terminal, final source hashes match the final successful build, and added-line whitespace is clean. No extra standalone compilation was needed.

Two StringArray fields were added to GroundTrack, changing that native object's layout; matching configured consumers were rebuilt. New Publisher overloads/query and GroundTrack helpers are nonvirtual. Existing Publisher event signatures and Subscriber virtual slots are unchanged. External prebuilt binary compatibility for the changed GroundTrack layout is unqualified.

No runtime tests, GUI checks, missions, numerical matrices, acceptance readers/audits, evidence admission, fixture/assertion/tolerance/denominator changes, packaging, installed/shared runtime changes, commits, pushes or external messages were performed. Compiler success establishes compilation only.

This batch advances the original complete function I/O, local/global resource and cloning requirements. It does not complete every parent/any-resource clause. Public declaration/property changes, component replacement, shared-global dependency retention, remaining callable/command/resource providers and ephemeris non-Cartesian/provider selection remain implementation work. Other original scientific, specification, external-dependency and historical-platform rows remain intact.

Tracking-v66 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
