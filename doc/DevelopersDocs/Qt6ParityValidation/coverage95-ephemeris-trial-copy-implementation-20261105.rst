Ephemeris trial value copies and function bindings, 2026-11-05
=============================================================

This batch repairs another concrete FRR!E406 bypass. It does not declare that
broad row complete or add implementation/acceptance credit. Tracking remains
1086/1158 (93.78%), recorded verification remains 1053/1158 (90.9%), and all
105 original acceptance rows remain open. The full implementation goal remains
active; no runtime verification was performed.

Reachable source routes
-----------------------

Assignment::Execute delegates whole-object assignment to ElementWrapper::SetValue
and ObjectWrapper::SetObject. Matching object types call virtual Copy, which
reaches EphemerisFile::operator=. FunctionManager also calls Copy when preparing
or refreshing formal objects and when binding nested object results. The old
EphemerisFile assignment retired the destination writer before copying values,
even when the destination was a live output inside an enclosing solver trial.
It also cleared the preceding partial trial snapshot and reset publication state.
Thus packet/notification filters alone could not protect accepted output.

Trial configuration and output ownership
---------------------------------------

The prior flag/name snapshot is replaced by a detached EphemerisFile value
snapshot of the configuration at trial entry. It owns no writer or live
snapshot of its own. Copies never acquire a source's trial snapshot. Leaving
effective SOLVING or receiving terminal cleanup restores entry configuration;
the existing enclosing-solver classifier still treats an inner final pass as
a trial while an outer solver is iterating.

The public assignment route now distinguishes live output from detached values.
An unfinished writer or current publisher subscription identifies live output.
In a trial, its configuration is copied while publication/output state is
preserved. Source values remain visible to commands in the trial. Ordinary
accepted/nontrial assignment retains the native retirement/reset flow.

The preserving operation uses a detached temporary and swaps the live output
into it before ordinary resource-state copying, then swaps the output back on
success or exception. It retains the writer, stream, registration/link/provider
state, publisher coordinate/solar-system bindings, run and receive flags,
readiness/finalization flags, provider labels, precision/callback state, epoch
cursors, direction/segment/maneuver/gap bookkeeping, transient-force binding
and the trial snapshot's ownership. State/covariance/quaternion/acceleration
arrays and published precise epoch/sample views are not written by the resource
assignment and remain in place. In particular, uninitialized pre-packet arrays
are not read just to preserve them.

Restoring entry configuration uses the private resource-state routine directly,
so it cannot recursively enter trial-copy classification or retire the live
writer. If restoration throws, snapshot ownership is retained for the caller's
cleanup/recovery. This guarantees neither allocation-free value copying nor an
atomic configuration transaction: an exception can leave partially assigned
configuration until entry restoration is completed. The live output is returned
to its owner before the copy exception is forwarded.

Function binding and scope
--------------------------

Formal inputs are refreshed before their local subscribers are reinitialized
and resubscribed. Their cached run state can still say SOLVING from the preceding
trial invocation. Treating every such cached state as a live-output copy would
retain stale formal settings and restore them over final input binding.
Detached/completed values therefore take ordinary fresh-copy initialization;
currently subscribed or unfinished outputs take the preserving path.

Assignment also preserves the destination's IsLocal creation role. GmatBase
already preserves destination name/global identity, but its generic assignment
copies IsLocal. Importing that flag from a local value into a global output
could cause function cleanup to classify the destination as locally owned.
Ephemeris value copying now retains the destination role; construction/cloning
still uses the normal explicit ownership setup performed by the caller.

This repairs the direct ordinary-assignment/formal-copy routes for the native
EphemerisFile class. It is not proof of every recursive subscriber-instance or
returned-resource dependency graph. In particular, caller/global outputs whose
assigned spacecraft/frame references originate in a function still need the
remaining binding-lifetime review. Do not replace this missing review with an
assumption that every copied pointer is caller-owned.

ABI and build
-------------

The class now owns one private unique configuration snapshot instead of the
previous flag/name snapshot fields. Private copy/state helpers are added;
public signatures, parameter IDs, script grammar, numerical formulas,
interpolation policies and tolerances are unchanged. All dependents must be
rebuilt together, as done in the private configured build.

Stage: ``build/nasa-coverage/coverage95-20261105-implementation-ephemeris-trial-copy``.
Build v1 passes 77 steps in 186.21 seconds; build v2 passes 66 steps in 3.38
seconds. Sessions 37879 and 90420 are terminal. Base, Qt, console, plugins and
Python bindings built. Test/probe clients were linked only. Exact preimages,
source-at-build hashes/patches, build receipts and final source/summary artifacts
are retained in the stage.

No test, GUI, script/function/model/solver execution, numerical comparison,
acceptance reader/seal or qualification run. No original fixture, assertion,
tolerance, fingerprint, denominator, acceptance ledger or qualification gate
changed. No installed/shared runtime, package, commit or push update.

Remaining source work and later verification
-------------------------------------------

Continue the recursive/caller reference-lifetime inventory and final-pass policy
review before declaring FRR!E406 implemented. Preserve native RunInitialGuess
and terminal-last-iterate replay; SOLVEDPASS is not itself a convergence
certificate. Continue all other remaining implementation scopes before returning
to verification.

Later verification must include whole-object assignments and function input/
output refresh, live versus detached/completed subscribers, no-packet outputs,
copy exceptions, ordinary and nested trial/final passes, differing source and
destination settings, global/local roles, provider/precision/segment continuity,
all formats, entry restoration and repeated/recursive call bindings. New state
preservation is source-derived and numerically unverified.
