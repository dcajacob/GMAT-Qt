ODE copied-force ownership — implementation, 2026-11-06
========================================================

ODEModel copies now own the force objects they clone, including transient forces.
Previously every transient was treated as borrowed during cleanup, so independent
transient copies were not deleted. A unique_ptr owner map distinguishes copied
components from borrowed Sandbox transients. Ordinary borrowed-force policy
remains; ownership no longer depends solely on a copied AllowODEDelete flag.

All clones are staged and validated before assignment retires the destination.
Null, source/peer, duplicate-clone and incompatible native results fail without
adopting source objects. Staged copies clean up if a later clone fails. Completed
copies reset readiness and use actual force/transient counts; assignment also
resets force-member initialization. No strong rollback claim is made for every
later metadata/base-class assignment exception.

Unique replacement, both DeleteForce overloads, ClearForceList, ClearDefaultForce
and ClearForcesOfType release explicitly owned copies. All current native
forceList erasure sites were inspected. A pointer already in the model is now an
idempotent AddForce operation, before unique replacement can delete that same
object. The existing null-force diagnostic is unchanged. Counts are maintained
after clearing/removing/replacing components. ODEModel gains a private-owner map;
its native clients/subclasses must rebuild. No GmatBase field or virtual slot is
added, and proprietary binary compatibility remains unqualified.

Ephemeris acceleration sampling
-------------------------------

Temporary transient attachments now have scoped removal on success or exception,
including AddForce throwing after insertion. Forces already present by identity
are retained and are not temporarily removed. A unique transient that would
replace a persistent component fails before that destructive replacement. Null
or non-transient entries in the supplied transient vector fail explicitly.
Ordinary native transient-applicability/skip rules and force-warning behavior
remain; this is not a redesign of the derivative mathematics or all provider
policies.

Remaining implementation
-------------------------

The returned EphemerisFile dependency builder still does not capture a private
ODEModel/AccelerationModel without a caller counterpart. Force frame/body/origin
bindings, finite-burn references, source-private internal frames and a retained
transient-vector snapshot still need transfer. HarmonicField and DragForce copy
constructors reset frame references. FiniteThrust copies clear burn pointers;
its single-spacecraft derivative method already exists and should be retained.
Burn lacks named frame/body/spacecraft getters, and FiniteBurn's typed thruster
name metadata needs reconciliation for snapshot binding.

Standalone returned resource roots, input graph isolation and global roots
pointing to local objects remain. The original inventory was reread: E452/E460
explicitly include entire objects, and E466 requires preserving non-global inputs
unless also returned. The broad function I/O/scoping/Clone rows remain open;
these obligations are not reduced to only this force-copy repair. All other
remaining OEM/body/frame/command/provider/specification scopes also remain.

Compilation and campaign status
-------------------------------

The private integrated build passes 225 steps / 163.31 s (v1), then 66 / 5.05 s
(v2) after preserving the original null-force diagnostic. Core, Qt, console,
plugins and Python bindings compile/link. V1 retains the existing OrbitRenderer
QImage::mirrored deprecation; v2 has no warnings. Final source matches v2 hashes.
Both sessions are closed.

No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals ran. Engineering tracking stays 1092/1158 (94.30%),
with 66 other original rows. Recorded source-versioned verification remains
1053/1158 (90.9%); all 105 acceptance rows remain open. Original assertions,
fixtures, tolerances, texts, fingerprints, denominator, ledger and gates remain
unchanged. Complete implementation continues before verification.

Artifacts are retained under
``build/nasa-coverage/coverage95-20261106-implementation-ode-copied-force-ownership``:
three exact preimages, before/at-build-v1/v2/final hashes, focused patch, source
notes, logs, receipts and summary. No installed/shared runtime, package, commit
or push was changed.
