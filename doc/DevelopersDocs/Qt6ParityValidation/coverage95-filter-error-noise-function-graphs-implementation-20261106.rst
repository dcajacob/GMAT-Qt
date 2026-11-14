Filter, error and noise function graphs — implementation, 2026-11-06
====================================================================

Filters, ErrorModel, ProcessNoiseModel/ProcessNoise and EstimatedParameter/model
now enter retained function input, saved-caller and output graphs. Completed native
Clone/typed Copy hooks reconnect their contexts and keep the previous destination
lease alive during replacement. The two estimation plugins expose these factories
through the generic managed Moderator creation path. LinearTime is enumerated only
in testing modes; the source map does not claim normal-mode or runtime availability.
DataFilter's concrete base also has typed Copy/context support.

The original entire-object/input-preservation clauses (FRR!E452/E460/E466) and full
scoping/clone obligations remain unchanged. The clause extraction is copied from
the prior stage as a source reference, with its workbook hash. No resource-type
exclusions, parent closure or engineering row credit is introduced.

References and value state
----------------------------

One appended GmatBase virtual contract lets providers passively bind actual
references or snapshot borrowed non-object state that generic names/setters cannot
represent. False uses the existing generic reference path. The root/dependency
predicate is shared. This hook is not model execution or proof of opaque-provider
completeness. It requires a consistent core/provider rebuild; external ABI/source/
platform behavior is unqualified. ErrorModel adds one owned epoch-pool member.

DataFilter maps the actual ordered observer/tracker arrays and aliases declared
names to mapped resources, retaining All and unresolved declarations. AcceptFilter
preserves its thinning counters/window history in explicit function values; native
CleanUp still resets them. Both concrete filter Initialize methods formerly returned
false after a successful base initialization; they now return that result. Native
participant setters replace same-role pointers and handle spacecraft configured in
both roles. Spacecraft trackers are exposed through the appropriate GetRefObject
query. Scientific filtering/thinning rules remain unchanged and unverified.

ErrorModel owns its default epoch and independent function-value epoch snapshots.
Duplicates within one model retain pointer aliasing. Native copies duplicate owned
epochs but keep externally supplied manager pointers borrowed, preserving that live
binding policy. The setter retains any of its own existing epochs returned to it,
and releases unused owned values. MeasurementManager allocates/supplies the external
raw epochs; this patch does not qualify or change that manager's allocation/cleanup
policy. Cross-model non-object epoch aliasing and external source lifetime remain
open. This is not a blanket deep-freeze of ordinary live error-model bindings.

ProcessNoise maps the actual frame, retires the private inertial cache and defers
reconstruction to native evaluation. Cold definitions do not build frames during
value binding. Frame/system changes invalidate readiness; assignment releases the
previous owned frame. Native initialization stages its replacement before retiring
the old frame and checks missing frame/system/J2000-body inputs. Existing matrix
conversion/noise formulas remain. LinearProcessNoise receives typed Copy support.

ProcessNoiseModel/EstimatedParameter stage and type-check owned component copies
before replacement. The completed-parent guard defers child context reconnection.
Source or prior destination component pointers returned by Clone are rejected
without deleting those existing objects. These are definition/value copies, not
qualified estimator/integrator checkpoints. Interior owned-root/owner aliases,
custom model internals and later arbitrary dependency replacement remain open.

Compilation and remaining scope
---------------------------------

Private integrated compilation passed twice: v1 747 steps /225.75 s and v2 9 steps
/1.78 s. V2 adds filter replacement/dual-role/access fixes and prior-component Clone
guards. Final eighteen source hashes match v2. Sessions 11102/78593 are closed.
The first build reports the existing Qt QImage.mirrored deprecation; v2 has no
warnings. Exact preimages, hashes, indexed patches, source family map/notes/clauses,
logs/receipts/summary and planning preimages are in
``build/nasa-coverage/coverage95-20261106-implementation-filter-error-noise-function-graphs``.

Continue solver/subscriber/event/interface/data/measurement/stop-condition/function/
body/solar-system/custom families, dynamic Parameter/wrapper graphs, interior owned
aliases, private/global/body/kernel/NAIF and every original recursive/exceptional/
scientific obligation. StopCondition still borrows command wrappers/parameters;
dynamic expressions must retain their dependencies, not become evaluated constants.

No tests, GUI, mission/function/model/solver execution, numerical comparison,
acceptance readers or seals ran. Original assertions, fixtures, texts, tolerances,
fingerprints, denominator, ledger and gates remain. No installed/shared runtime,
package, commit or push changed. Tracking-v34 stays 1093/1158 (94.39%), 65 other rows;
recorded source-versioned verification stays 1053/1158 (90.9%), all 105 acceptance
rows open. Goal active; finish implementation before verification.
