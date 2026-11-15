Native covariance copy and resource owner rebinding
===================================================

Implementation-first repair in Covariance.cpp/.hpp and GmatBase.cpp. The retained
Save inventory's unavailable generic Covariance getter does not itself prove
persistent data loss. Owning-source review instead found concrete copy defects:
an unsized source left a destination's old matrix allocated with mismatched
dimension/metadata, assignment retained the previous submatrix cache, and copied
resource covariance elements retained pointers to the source resource.

Covariance copy construction now allocates its matrix after copying metadata,
using a default Rmatrix for an unsized source because the Rmatrix copy constructor
rejects unsized matrices. Assignment builds a complete temporary and swaps matrix,
cache and metadata together; the old destination/cache is released by the temporary.
Allocation failure before the swap leaves the covariance destination intact.
This is a covariance assignment guarantee, not atomic whole-resource assignment.

New nonvirtual RebindOwner(previous,replacement) updates only matching owner
addresses. GmatBase copy construction/assignment use it for source-to-destination
self references. References to other resources retain their existing borrowing
policy. A null previous owner is ignored. No member/layout/virtual-slot changes,
numerical formula changes or getter/Save serializer changes. Direct/native API
and full scientific qualification remain separate.

Build and existing checks
--------------------------

Stage: build/nasa-coverage/coverage95-20261105-implementation-covariance-copy.
Three exact preimages and incremental-v1/v2.patch remain; v1 was an unbuilt
authoring draft. Final v2 patch SHA256:
a400d9012da91c24ce830dbb256fff190a508a596199bd395e65d3f90bad2056.
Reverse patch check passes. The private integrated-current build rebuilds735
affected targets and passes151.08s. One existing QImage::mirrored deprecation
warning appears in unchanged OrbitRenderer.cpp; no build errors. summary.json
records final source/image hashes. Seven monitored installed images unchanged.

CovarianceTests and PropagationCovarianceTests compile unchanged against the
private frontend/core and both pass. They exercise the covariance editor's
validation/Cancel/Apply/Undo/Redo/Unicode paths, byte-identical direct-script
versus GUI GPS filter state/covariance outputs, and propagation covariance/STM/
A-matrix enablement and reports. These are bounded consumer regressions, not
independent universal covariance/science proofs or direct owner-address tests.

Legacy executables emit their original PASS lines, not NASA_CHECK records. Their
QTemporaryDir payloads are not all retained. Saved commands/logs/receipts and
build-existing.py/run-existing.py provide replay using fresh output directories.
Native xcb, resizable initially minimized desktop, zero screenshots, private core
mapped and installed core absent; all handles terminal. No new tests, fixtures,
registrations, matrices, readers or seals. Direct empty-assignment/cache/owner
identity and allocation-failure backfill remain deferred.

Generic Covariance getter/alias persistence and complete FRC!E132 remain open.
No original row admitted. Verified1053/1158,105open,48neededfor1101;59holds,
31resolved/28unresolved including6MATLAB,37MATLAB identities,historical1816/2367
unchanged. Goal active/incomplete. Public code/packages unchanged by this increment.
