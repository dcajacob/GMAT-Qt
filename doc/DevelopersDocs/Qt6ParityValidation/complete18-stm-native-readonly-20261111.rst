Native STM read-only setter contract, 2026-11-11
==============================================

The generic native matrix setter no longer replaces cached values for OrbitSTM,
OrbitSTMA, OrbitSTMB, OrbitSTMC or OrbitSTMD. Their metadata was already read-only,
and the GUI already rejected these destinations. The inherited value-variable
setter nevertheless accepted a replacement cache value without a diagnostic.
It did not change the spacecraft's propagated STM. The repair closes that native
API inconsistency; it does not implement selected-frame STM conversion.

Two production owners change: OrbitStmParameters.cpp/.hpp override the existing
generic setter slot for each of the five concrete parameter classes and throw a
read-only diagnostic. No instance data or virtual slots are added. Existing
getters, numerical code, typed-setter behavior, Clone and Copy implementations
remain unchanged. Native value Copy remains available for the existing
provider/cache transport contract. OrbitErrorCovariance and other matrix types
are unaffected.

The supplementary witness is frozen before execution. All15 generic cases fail
before repair: five parameter classes times original, clone and copied providers.
The metadata reports IsSettable=false, but the setter accepts all15 writes and
changes their caches; every underlying spacecraft matrix remains unchanged.
Those raw failures and source/build/image identities remain retained.

The same witness passes all15 cases after repair. It checks a thrown read-only
diagnostic, exact cache and native-owner preservation, successful native value
Copy, and unchanged subsequent evaluation. Five existing typed-setter rejection
controls pass before and after. The worker builds a small isolated script but
executes no mission. Only its affected target is rebuilt; no full regression or
GUI replay is needed for this native dispatch defect. Saved numeric and GUI
passes retain their original source versions.

All279 captured owning source files remain stable around each execution, apart
from the two deliberately changed production owners between versions. All1,334
protected original inputs remain unchanged. Five evidence negative controls
reject an omitted case, changed cache, lost owner isolation, missing diagnostic
and a forged baseline pass. The exact incremental patch and source capsules are
retained in ``build/nasa-coverage/complete18-20261111-stm-readonly-contract-v1``.
Its frozen evidence reader launches zero native tests.

Remaining full requirement
--------------------------

FRR!E525 is still open. OrbitData currently returns the internal STM unchanged.
The selected-frame result requires J(t) Phi(t,t0) J(t0)^-1, including frame rates.
Spacecraft's ResetSTM action and SetSTMToIdentityMatrix currently store no
reference state/epoch. Initial propagation assembly, copies, native restoration
and function reference rebinding must preserve that authority with the matrix.
Frames depending on another spacecraft need its actual state at the reference
epoch. A current state must not be evaluated as though it were historical.
Singular frame combinations require mathematical justification and explicit
rejection tests; missing implementation is not a singularity exemption.

The source-versioned ledger remains1,106/1,158 (95.5%), five of the18 agreed rows
complete,13 remaining. This repair awards zero new rows. The original scientific
failures, immutable prefix and review package archive are unchanged. No complete
current-build, arbitrary-frame or scientific qualification is claimed.
