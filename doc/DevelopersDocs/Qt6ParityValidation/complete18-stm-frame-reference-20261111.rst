STM frame transport and reference ownership, 2026-11-11
=====================================================

OrbitSTM and its four3x3 partitions now use the selected coordinate system.
The reader applies J(t) Phi(t,t0) J(t0)^-1, including rotation rates at both
endpoints. Native propagation equations and the unqualified MJ2000Eq route
are preserved. All five parameter types advertise their existing coordinate
dependency while retaining the default EarthMJ2000Eq spelling/behavior.

Reference state ownership
------------------------

An internal registry stores the matrix's initial epoch, precise-clock mode,
initial state, coeval reference-spacecraft states and available accelerations.
Actual STM resets and initial propagation assembly establish that reference.
Native construction, assignment, restoration and dependency rebinding carry it
with the matrix. Propagation advancement prevents later commands from replacing
the initial reference with the current state. Lifetime generations distinguish
retired objects from later objects at reused addresses. No public instance data
or virtual slots are added.

Historical frame evaluation uses a scoped read-only view of the original runtime
reference objects. It does not move live spacecraft back to an old epoch.
Reference accelerations are captured when available at the initial state; an
initially unready provider can fill them before propagation advances. A later
provider cannot manufacture a missing historical acceleration after advancement.
Detached frame/axes graphs are rebound and initialized before conversion, so
initial-epoch calculations do not overwrite live rotation caches.

The work also demonstrated an assignment clock-mode defect. Ordinary SpacePoint
assignment preserves GmatBase metadata, while SpaceObject copies its complete
GmatState. A copied spacecraft consequently retained the precise timestamp in
storage but advertised the receiver's old clock mode. Nine selected-frame reads
used a rounded Real epoch. SpaceObject assignment now synchronizes that metadata
with the copied GmatState. No timestamp values or numerical tolerances change.

Targeted verification
---------------------

The frozen native witness covers six prescribed frame/reference cases, each
through original, cloned and assigned spacecraft and all five matrix types:
90 reads and1,296 entry comparisons. Cases include a fixed tilted frame,
forward/backward rotation, later reference resets, identity and100-nanosecond
precise-epoch offsets. The independent Python oracle constructs both Cartesian
Jacobians and their inverse directly. Literal input matrices retain their saved
source versions; injection tests do not claim new propagation qualification.

All90 reads pass at the frozen1e-9 entry bound; maximum error is
2.2737367544323206e-13. The15 generic and five typed native read-only controls also
pass. Both original forward/backward STM GUI cases pass unchanged, including
their independent circular variational and two-scale perturbed-trajectory
assertions. They run only on isolated headless native X11. No full regression or
unrelated visible GUI replay is run.

Two new native missions exercise the missing other-spacecraft frame path. Sat
and Ref follow separate Earth point-mass circles, with full STM and four
partitions reported in Ref's velocity/normal axes. One mission has one Propagate;
the other reaches the same endpoint across two successive commands. Independent
circular-flow and two-end frame equations check every reported entry, both
trajectories and exact partition consistency. Each mission also verifies
unchanged-owner rejection of a self-origin state map and noncoeval references.
The largest entry difference is1.1874114136389835e-7 in the seconds-valued B
partition, below its unchanged2e-6 bound. A/D bounds remain2e-8 and C2e-11.
These are bounded point-mass missions, not arbitrary-force/frame qualification.

Original failures and corrections
---------------------------------

The original frozen matrix baseline retains75 failed reads and15 identity passes.
The first transport implementation crashed because detached native axes were
used before initialization. Its native result and debugger stack remain held.
Initializing those clones next exposed90 missing-origin errors: native coordinate
Clone deliberately clears borrowed references. The final helper rebinds the
borrowed origins, solar system, axes references and nested frames before
initialization. The subsequent81-pass/nine-failure clock diagnosis is retained,
followed by the90-pass clock repair. No earlier result is relabelled.

The first new split-command script ran20seconds then60seconds, totaling80seconds.
ElapsedSecs stopping values restart for each command. The fixture generator now
emits20then40 to realize the originally frozen endpoints20and60. Its expected
endpoint, matrices and bounds remain unchanged; the failed80-second receipt is
preserved. Two build/setup failures and a sandbox-blocked debugger attempt are
also retained separately from native assertions.

Evidence and remaining obligation
---------------------------------

Artifacts are in
``build/nasa-coverage/complete18-20261111-stm-frame-reference-v1``. Executed
source capsules distinguish every build/run. Production inputs are unchanged
between the final90-read, read-only, GUI and observer-mission runs; new mission
registrations do not trigger repetition of those passing cases. All1,334 protected
original inputs remain unchanged. Eight evidence controls reject lost reads,
wrong precise values, lost isolation, forged baseline passes, unconverted matrices,
current-epoch similarity, wrong reference identities and lost singularity checks.
The frozen engineering reader launches zero native tests and does not replay the
unchanged full coverage ledger.

FRR!E525 remains open. Remaining work includes the full allowed frame-family
conjunction, actual selected-frame GUI transactions, function/reference rebinding
and restoration witnesses, and complete domain/error coverage. Target-dependent
axes currently report that a full state-dependent Jacobian is required. This is
an implementation limit, not an admission-time singularity exemption; every
mathematically defined required combination must still be fulfilled. Only the
tested self-origin rejection is justified here as a singular state map.

This stage awards zero new requirements. Sealed source-versioned coverage remains
1,106/1,158 (95.5%), five of the18 agreed rows complete and13 remaining. The prior
prefix, original scientific failures and review package archive are unchanged.
No whole-current-build or full scientific qualification is claimed.
