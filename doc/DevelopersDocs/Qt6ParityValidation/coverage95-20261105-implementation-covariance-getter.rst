Inherited covariance matrix view
================================

Implementation-first repair in Covariance.hpp/.cpp and GmatBase.cpp. The inherited
descriptor0 is declared Rmatrix and already supports indexed covariance reads,
but GmatBase's whole-matrix getter always threw. The corrected native probe
confirmed stored,read-only spacecraft covariance with working indexed reads and
a failed descriptor0 matrix read. This owning-source/native contract supports a
specific repair; missing getter observations alone are not treated as defects.

GmatBase now returns the stored covariance matrix for descriptor0. A new
nonvirtual const Covariance::GetCovariance overload provides that view without
casting away constness or copying data. Other IDs retain their existing path.
Unsized covariance retains its actual unsized matrix; no data is fabricated.
No field IDs,metadata/read-only/script eligibility,layout,members,virtual slots,
formulas or serialization policy change. Derived getter/id/name aliases and
empty-state persistence remain separate qualification obligations.

The initial manual probe selected Spacecraft's existing Covariance label alias,
which maps to its6x6 OrbitErrorCovariance and already works. Its native process
completed but the author's missing-getter expectation failed. Source/binary/log/
receipt remain. The corrected probe explicitly selects GmatBase descriptor0;
it is retained separately under before-v2. The public Spacecraft alias is not
changed or relabeled as a prior defect.

Stage: build/nasa-coverage/coverage95-20261105-implementation-covariance-getter.
Three exact production preimages and incremental-v1.patch retained. Patch SHA256:
7074681691a66647b946a929653f38fc311728d5393345863ac35773299677c3.
Reverse patch check passes. Private735-step rebuild passes145.08s. One existing
QImage::mirrored deprecation warning appears in unchanged OrbitRenderer.cpp;no
build errors. summary.json records final source/native image hashes. Seven
monitored installed images and public code/packages unchanged by this increment.

The corrected small manual C++ diagnostic is built before production edits and
reused afterward. It observes descriptor0,read-only flags,stored versus indexed
values,same-storage identity and a live indexed update for a spacecraft. An
empty ForceModel covariance retains an unsized same-storage view. Private Base
loader identity and source hashes are recorded. This adds one engineering
diagnostic,not a new suite/catalog or whole native-ABI qualification.

CovarianceTests and NasaSaveResourceTests compile unchanged and both pass existing
GUI workflows: covariance editor/filter and Save Cartesian. The Save case now
records a9x9 inherited covariance for Subject without a getter error; runtime and
reload values are exactly equal. Earth records an empty matrix representation.
Empty JSON representation alone does not prove sized/unsized-state persistence
or justify a transient exclusion. Legacy covariance PASS and Save NASA_CHECK
schemas remain distinct;temporary legacy raw payloads are not all retained.

Replay uses build-existing.py,run-existing.py and saved compiler/native commands
with fresh isolated startups/output directories. Native GUI uses xcb in a
resizable initially minimized desktop,zero screenshots;private Base mapped and
installed Base absent. Manual diagnostic has display variables removed. All
handles terminal. No new suite tests,fixture definitions,registrations,matrices,
qualification readers or seals. Original assertions/tolerances unchanged.

No blanket recomputation or retirement of238missing getter identities is made.
Complete generic-alias/override/persistence domains and FRC!E132 remain open.
No original row admitted. Verified1053/1158,105open,48neededfor1101;59holds,
31resolved/28unresolved including6MATLAB,37MATLAB identities,historical1816/2367
unchanged. Goal active/incomplete;new formal backfill remains deferred.
