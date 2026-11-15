Force borrower retirement — implementation, 2026-11-06
=======================================================

A command-owned finite-thrust force can be deleted during BeginFiniteBurn
reinitialization while an older cached ODE model still holds its raw pointer.
PhysicalModel now notifies admitted ODE borrowers when the force retires. The
ODE callback removes the exact reference without accessing the destroyed derived
force. This contributes to lifetime/scoping implementation; it does not complete
the original broad function/object requirements.

Lifetime contract
------------------

AddForce records the receiving model's weak GmatBase identity and the force's
transient classification in a process-lived side registry. Registration precedes
vector insertion and rolls back if insertion fails. The force destructor extracts
subscriptions and releases the registry mutex before callbacks. Retirement does
not allocate a registry on the destructor path. Expired/address-reused recipients
fail the existing current-identity check. Engine object lifecycle remains serialized;
this is not a synchronization lease for concurrent destruction.

The ODE callback only uses the dying force pointer as an identity. It removes the
entry, updates force/owned/transient counts, clears force-type data and invalidates
normal/parameter initialization. Normal ReleaseForce unregisters before every
native clearing/replacement/deletion/destructor path, preserving borrowed ownership.
Native value copies own their cloned forces and do not subscribe to their own
partially constructed owner; their RAII copy ownership remains unchanged.
Subscriptions carry no force/model ownership and add no native fields or virtual
slots. Illegal external deletion of a model-owned cloned component is not an API
contract; arbitrary other raw borrowers and direct provider force-list mutation
are not universally subscribed.

Propagation cleanup now removes the exact transient pointer it inspected rather
than deleting by name, which could select an equal-name static force. The native
index adjustment and active Sandbox maneuver-vector policy remain. The
UpdateSpacecraftParameters action now invokes its whole all-force dynamic update
once instead of repeating that same helper. No acceleration/mass-flow formula,
expected value or tolerance changed.

Remaining implementation
-------------------------

The mechanism retires an ODE force reference, not dependencies within a still-live
force. Complete explicit sampling-copy aliases from the owning retained graph and
actual invocation identities. Preserve explicit Global shared identity and design
its maneuver persistence deliberately; do not blanket-remove all forces on function
return. Current command initialization runs on entry and caller-frame restoration.
The active Sandbox vector, standalone burn roots, PropSetup/PSM and all other
original resource/body/frame/command/scoping/provider obligations remain.

Source review found a further Burn assignment defect: its previous owned local
coordinate-system pointer is overwritten with NULL without deletion. Fix ownership
and add passive retained Burn frame/body/spacecraft binding; maneuver-start setters
can Fire and must not be used merely to reconnect a value. Unbound local Burn
configuration also needs its native dormant initialization policy. See
next-function-global-burn-source-research.json. E452/E460 entire-object and E466
input-preservation requirements retain full scope; no broad parent row is closed.

Compilation and evidence retained
----------------------------------

The private integrated v1 build passes 327 steps / 174.99 s.
After exact cleanup/single dynamic update, v2 passes 67 steps / 5.05 s.
Final sources match v2; sessions 67711 and 62554 are closed, no live build. Five
exact preimages (propagation cleanup added before editing), before/at-build/final
hashes, focused/per-file patches, notes/research, logs/receipts/summary and planning
preimages are in
``build/nasa-coverage/coverage95-20261106-implementation-force-borrower-retirement``.
The earlier OrbitRenderer QImage::mirrored deprecation warning, if emitted, is
retained verbatim in summary.json. External binary/cross-platform behavior remains
unqualified.

No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals ran. Original assertions, fixtures, tolerances,
requirement texts/fingerprints, denominator, ledger and qualification gates remain.
Tracking-v27 stays 1093/1158 (94.39%), 65 other original rows; recorded source-
versioned verified baseline stays 1053/1158 (90.9%), all 105 acceptance rows open.
No installed/shared runtime, package, commit or push changed. Goal active;
implementation incomplete.
