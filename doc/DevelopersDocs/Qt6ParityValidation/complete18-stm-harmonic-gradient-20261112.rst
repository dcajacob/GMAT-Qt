Harmonic-force gradients for owner-dependent STM frames
======================================================

This batch fixes the missing native GravityField derivative provider for
owner-dependent STM frames. A second demonstrated defect is repaired when the
user chooses a lower native StmLimit than the force degree. The private frame
Jacobian now differentiates the complete configured acceleration while native
variational propagation preserves its existing selected approximation.

Two production files change, with one private request header. There are no new
public APIs, virtual slots or native object layout changes. Original tests,
assertions, bounds, failure receipts and the 1,158-row denominator are retained.
No full regression is run. The canonical coverage stays 1,106/1,158, with 5 of
the agreed 18 complete; FRR!E525 remains open.

The provider repair
-------------------

The existing historical acceleration path supported GravityField, but the new
owner-dependent differential retained gradients only for PointMassForce. A
synthetic degree-zero GravityField mission reproduces 45 defined matrix reads
rejected for missing captured acceleration gradients; its 15 mathematically
singular chart reads already reject correctly. Independent states and native
STMs pass, so this is a missing frame capability rather than a trajectory error.

A private synchronous request binds the gradient to the exact force instance.
GravityField retains its direct Cartesian acceleration gradient before its
independent-origin acceleration bookkeeping. ODEModel adds that contribution
to the existing snapshot gradient. Unsupported force types remain explicit
unfinished providers. The request is absent during ordinary force propagation.

After repair the same frozen degree-zero mission passes all 60 original, Clone
and Copy reads. One 60-read point-mass owner-frame fixture is the directly
affected dependent check for the changed ODE provider loop; it also passes.

A nonzero synthetic J2 mission supplies an additional independent numerical
check. Its force comes from the closed J2 potential, its spatial differential
uses complex-step arithmetic, and converged RK4 integration supplies both
spacecraft trajectories and native six-state variational flows. The frame map
uses 80-digit Decimal kinematics and five-point Cartesian differences. Native
field poses are shared geometric inputs, sampled through detached frames; this
does not independently qualify Earth orientation. All 60 frame reads pass and
the maximum native STM error is 1.9895196601282805e-13. An offline zero-J2 limit
also agrees with the independent analytic circle/STM without native execution.

Reduced native STM degree
-------------------------

The new reduced-degree fixture initially assumed StmLimit=0 removed every J2
contribution from the native variational matrix. That expectation fails and
remains failed. Native Pines equation 37 retains the radial-direction term
a4/r on the diagonal while truncating higher scalar partials. A separate frozen
fixture explicitly models that existing approximation. No tolerance changes,
production change or reclassification are used for that oracle correction.

With the correct native approximation accounted for, the frame calculation
still fails: it inherits the truncated force gradient even though its axes-rate
acceleration includes the full degree-two field. This is the second actual
production defect. Only an explicit private frame-gradient request now asks for
the complete configured force degree. The ordinary StmLimit argument, resource
setting and native propagation behavior remain unchanged.

The corrected fixture passes all 60 reads. Its independent native STM residual
is 6.394884621840902e-14 for the declared truncated approximation. Both spacecraft
states, their native STMs and the integrator clock are exactly equal before and
after the frame repair. No physical full-variational accuracy is claimed for a
native STM the user deliberately truncates. The independently differentiated
frame map still uses the complete J2 force.

Validation and retained scope
-----------------------------

The passing source-versioned evidence comprises four native missions and 240
Parameter reads: 180 defined reads and 60 proven singularity rejections. The
final library change affects only the private full-degree request when the
configured native limit is lower; the passing full-degree and point-mass
missions are reused at their actual executed versions. The final test
registration build changes no native image and runs no native case.

All 1,334 protected original inputs remain unchanged. Eight corrupted evidence
controls reject wrong providers/settings, nonfinite field poses, missing state,
wrong native/selected matrices, lost frame binding and rejection cache mutation.
Frozen replay launches zero native tests. Generated CTest registration is
archived as a stage-local copy, rather than binding this stage to a mutable
build-tree file.

Two earlier harness executions stop at native script parsing because the
test-generated potential filename contains Unicode. The console interpreter's
default mode rejects it. Both failures remain unchanged; the generated basename
is corrected to ASCII before executing the force-path checks. The supplementary
worker adds test-local native message capture to retain the actual diagnostic.
No application Unicode policy is changed.

The obligation index records the 22 named property children, the worksheet's
16 explicit axes types and five origin categories, together with eight saved
source-versioned evidence stages. The literal FR-xxx reference is still an
admission question. Inventory alone earns no coverage. Higher harmonic degrees,
tides, other required gradient providers, complete permitted frame/parameter/
body/domain composition and broader rejection/history scope remain open.

The stage is build/nasa-coverage/complete18-20261112-stm-harmonic-gradient-v1.
It extends engineering receipt
``d03c8f0e0375910ec4abf4807d734e8117cffa829af2d65627031b0f4ece7fc4``.
The canonical prefix remains
``de77757b896aa68c65df2462c026b58bf11ed684b502148fa5a14c0593111574``.
No whole-row, whole-current-build or full NASA qualification is inferred.
