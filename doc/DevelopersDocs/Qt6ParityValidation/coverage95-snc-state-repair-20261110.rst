SNC timing/getter/copy repair — 2026-11-10
========================================

The Save descriptor review exposed uninitialized ordinary timing fields and an
incorrect precise-time getter in SNCProcessNoise. The label getter forwarded to
the Real interface, and the ID getter tested ElapsedTime rather than CurrentEpochGT.
Copy construction/assignment also omitted timing and computed-noise state.

The constructor now initializes elapsed time to zero and ordinary epoch to21545,
matching the existing GmatTime default. Both precise getter overloads return the
stored split epoch through the correct typed interface. Native Clone, assignment
and Copy retain ordinary/split epochs, elapsed time and a computed noise matrix.
An uncomputed matrix remains unsized, including when copying a fresh source over
a previously computed destination. Public headers, object layout, virtual ordering
and both GetProcessNoise formula bodies are unchanged.

Controlled verification
-----------------------

The same final probe binary fails on the original aggregate plugin at the precise
getter and passes on the private repaired plugin. The baseline's executed source
identity is the aggregate manifest/preimage, not the newer source file present
in the working tree when the comparison ran. Actual loader traces identify both
plugin files. The final probe passes209assertions:100ns split-epoch readback,
deterministic defaults, four copy routes, fresh-cache reset and all36components
of independently specified white-acceleration covariance at2.5seconds. Its fixed
absolute matrix bound is1e-16.

The registered probe and the unchanged PropagationCovariance,
SaveLegacySupportProcessNoiseModel and ResourcePolymorphicCopy controls all pass
against the new plugin. Source/caller hashes and original timeouts are retained.
PropagationCovariance initially received an observer failure because LD_DEBUG
reported libEKF.so rather than its versioned symlink target. The actual CTest
passed; resolving the recorded path and comparing the already recorded digest
corrected that observation without rerunning the passing test.

The first copy implementation attempted an ordinary Rmatrix copy of an unsized
source, causing an actual sandbox clone regression. The final implementation
uses a conditional matrix snapshot and native storage swap. That initial source,
library and failures remain. Early probe setup revisions and the initial CMake
dependency-order error also remain; none is counted as a successful execution.

Scope and artifacts
-------------------

Stage: build/nasa-coverage/coverage95-20261110-snc-state-v1.
verification-summary-v1.json binds the source comparison, formula-body check,
controlled probe and four targeted registrations. The private runtime is under
runtime-v2/lib; installed binaries and packages were not updated. All593original
aggregate runtime inputs and1,334authorized original test inputs remain unchanged.
No full regression was run and no complete NASA requirement row is credited by
this repair. Sealed coverage remains1,081/1,158, with20more rows needed for95%.
The wider Save property/reference/consumer conjunction remains under review.
