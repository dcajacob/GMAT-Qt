Finite-thrust state derivatives and propagator ownership
=======================================================

Evidence is stored in
``build/nasa-coverage/complete18-20261112-stm-finite-thrust-v1``.

A constant-mass chemical-thrust mission exposed missing native STM and
A-matrix contributions. The spacecraft uses NadirPointing attitude and a
body-frame thrust direction. Its Cartesian trajectory agreed with the
independent propagation, while the original STM exceeded the frozen bounds
and the force did not provide a captured Cartesian gradient.

FiniteThrust now registers both matrix derivative types and samples its six
Cartesian acceleration partials through private spacecraft and burn values.
Native matrix assembly supplies the actual integrator-stage state and epoch.
The per-spacecraft query supplies the same gradient to owner-dependent
coordinate transformations. Existing public interfaces are retained.

The synchronized reference propagator exposed a second defect. Propagate
passed its cumulative spacecraft list into each model's coefficient setup.
That registered the reference-only model as an acceleration provider for the
thrusting spacecraft. Setup now uses each model's own state-manager objects
in their native order.

The repaired mission passes 45 live native full and partitioned STM reads.
Maximum Cartesian STM error is 1.07e-13. A separate A-matrix mission passes
all 36 thrust derivative entries, with maximum error 5.97e-19. The existing
finite-thrust function compatibility check passes three missions and twelve
function calls. Original numerical bounds remain unchanged.

The initial powered frame reference is checked independently. An intermediate
oracle assumed a pre-burn reference after observing the ownership defect.
That assumption is retained as a failed intermediate interpretation. The
corrected ownership restores the powered reference and the final check uses
the original powered-frame expectation.

Early collector attempts exposed native setup constraints and a traversal
loop in the test observer. The observer loop was corrected and its timed-out
process was confirmed terminal before retrying. These receipts and the
original numerical failure are retained. Offline replay launches no tests.

This stage covers live propagation with constant mass. Post-propagation
reports after transient-force removal, mass depletion and the wider force,
frame and augmented-state conjunction remain open. Canonical coverage stays
at 1,113/1,158, with 12/18 agreed rows complete.
