N-plate eclipse derivative evidence
===================================

The targeted stage is stored in
``build/nasa-coverage/complete18-20261112-nplate-shadow-v1``.

The propagated N-plate STM and A-matrix paths omitted the derivative of the
illumination fraction. Both now include the existing analytic shadow gradient
in the pressure coefficient derivative. The optical solve-for columns and
public APIs are retained.

The annular-eclipse case exposed a second defect. Sun-facing plates applied
the dependency between the incident direction and plate normal inside the
partial derivatives and again in the final chain rule. The body-frame and
inertial-frame public derivative routes now apply that dependency once.

Final targeted validation includes the following cases.

* A mixed-optical fixed-body plate under NadirPointing in penumbra.
* The separate A-matrix-only path for that penumbra configuration.
* A mixed-optical Sun-facing plate in an annular eclipse.
* A NadirPointing plate in umbra, with zero SRP force and gradient.

These cases provide 135 owner-frame parameter reads, 36 raw A-matrix entries,
144 direct plate-derivative entries across both public routes, and 162 direct
attitude-derivative entries. All original numerical bounds remain in force.
The independent eclipse calculation uses a cross/dot angle to avoid
cancellation in nearly aligned disk geometry. Complex-step derivatives were
checked against 80-digit arithmetic before using that reader revision.

Original failed receipts and intermediate passes are retained with their
executed source and binary versions. Saved full-sun N-plate and spherical
penumbra evidence is replayed offline. Canonical coverage remains
1,113/1,158, with 12/18 agreed rows complete. This stage contributes evidence
to FRR!E525. Other force, attitude, augmented-state and frame combinations
remain open, along with the pending formation-cloning and historical-function
acceptance decisions.
