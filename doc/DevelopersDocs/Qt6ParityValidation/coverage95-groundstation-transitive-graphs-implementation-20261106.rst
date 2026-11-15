GroundStation transitive function graphs — implementation, 2026-11-06
=====================================================================

GroundStation is now a retained function resource root and dependency. Earlier
work copied its owned native components but left stations outside the retained
scene's type gate, owner inventory and Clone/Copy rebinding. This batch connects
those routes; original whole-object/scoping parents remain incomplete.

Actual owned and body references
---------------------------------

GroundstationInterface exposes actual ordered owned components, typed passive
rebinding and retained initialization. GroundStation returns its hardware,
configured error models and per-signal-path models. The binder validates shape,
path keys and native types, registers all copied identities before recursing,
and maps nested owned or borrowed dependencies through the existing scene.
FunctionManager's native owner and caller-component inventories include stations.
Completed Clone/typed Copy hooks reconnect contexts; Copy holds its previous
lease across replacement. Native destination naming remains unchanged.

BodyFixedPoint adds a nonvirtual passive central/J2000-body binder. It stages
mapped pointers and consistent names, retires owned frames/generated geometry,
and attaches the current SolarSystem without name-based body replacement.
A scene prepass initializes station body geometry before calculated points and
coordinate systems can query a station-origin state. Station hardware initializes
after frame binding. Noncomputational owner/frame reference loops can reuse a
station's stable identity; pure computational point/frame cycles still reject.
ObjectInitializer preserves a retained station's actual J2000 binding before
looking for a same-named global body. Its current-reference scan now also checks
retained hardware/error/FOV identities before name/template lookup.

Mask and native component lifetime
-----------------------------------

Retained station initialization bypasses the native second name-based hardware
association. It also keeps an existing generated Imager and FOV instead of
assigning a new FOV after registering the old child. Their copied internal names
remain part of the retained value. A missing generated mask uses the extracted
shared native construction path. Ordinary native initialization keeps its prior
rename/reload behavior. Explicit mask filename changes/clear retire only private
generated imagers and invalidate readiness; configured AddHardware entries stay
ordinary owned resources. Native CustomFOV interpolation/file policy remains;
this is not an external-file-content snapshot.

StationObjectCopies now inventories source-owned identities across every native
collection, rejects repeated ownership, and rejects Clone results equal to any
source peer, an already adopted clone or an incompatible native type. Staged
cleanup does not delete those borrowed results. This strengthens the demonstrated
native component-copy boundary, not every third-party allocation contract.

Compilation and remaining scope
---------------------------------

Private integrated builds pass: v1 173 steps /153.14 s; v2 67 steps /5.86 s.
Final seven source hashes match v2; sessions 35274/93521 closed. Core, Station,
configured providers, estimation, Qt and Python wrappers compile together.
Three appended pure virtual GroundstationInterface contracts require matching
provider implementations and a consistent rebuild; no native fields added.
External ABI/source compatibility and other platforms remain unqualified.
Warnings, exact preimages, hashes, indexed patches, notes, logs, receipts, summary
and planning preimages are retained under
``build/nasa-coverage/coverage95-20261106-implementation-groundstation-transitive-graphs``.

No tests, GUI, missions/functions/models/solvers, numerical comparisons, acceptance
readers or seals ran. Original assertions, fixtures, tolerances, texts, fingerprints,
denominator, ledger and gates remain unchanged. No installed/shared runtime,
package, commit or push changed. Tracking-v31 stays 1093/1158 (94.39%), 65 other
rows; recorded source-versioned verification stays 1053/1158 (90.9%), all 105
acceptance rows open. No broad parent credit.

Continue actual named-resource family creation/ownership mapping, standalone
hardware/attitude/FOV and subscriber/event/provider roots, explicit copy aliases,
global activity lifetimes, private bodies/kernel pools, unresolved configuration,
direct/later owner replacement and every recursive/exceptional/body/frame/
scientific obligation. Runtime NAIF/kernel identity policies remain unqualified.
Implementation goal stays active; verification remains deferred.
