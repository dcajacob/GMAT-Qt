STM base alignment and selected-frame GUI follow-up
===================================================

This targeted engineering batch repairs FK5/ICRF STM frame transport and fills
five selected-frame GUI transaction routes. It adds no whole-requirement credit:
coverage remains 1,106/1,158, with 5 of the agreed 18 complete. FRR!E525 remains
open. The original denominator, numerical expectations, tolerances, protected
tests and failure receipts are unchanged. No full regression was run.

The demonstrated defect and repair
---------------------------------

The previous STM helper used CoordinateConverter's cached rotation and rotation
rate matrices. Those describe the input/output axes but omit the intervening
FK5/ICRF base alignment applied by the actual Cartesian converter. Consequently
the reported STM was wrong for selected ICRF frames, even with both endpoint
epochs retained correctly.

Only the private OrbitStmFrame.hpp helper changes in production. For differing
base systems, it now obtains the exact linear native Cartesian map by converting
six unit vectors with translation omitted. This includes the base alignment in
both rotation-rate terms, without numerical differencing or changes to native
alignment/velocity conventions. The same-base branch, public APIs, instance
layouts and propagator equations are unchanged.

Five frozen contexts cover native ICRF forward/backward reads, spinning axes in
both base directions, and a same-base control. Each exercises five native STM
parameters on original, cloned and assigned spacecraft. The inner matrices are
literal source-versioned saved results, not newly qualified propagation results.
Independent quaternion reconstruction at two literal ICRF table nodes and
two-end Cartesian Jacobian algebra define the expectations. The 1e-9 entry bound
was frozen before execution. The baseline retained 27 passes and 48 failures;
after repair all 75 reads pass, checking 1,080 matrix entries with maximum error
1.1368683772161603e-13. The native base-alignment rate convention remains zero;
this batch does not qualify a different physical alignment-rate model.

Missing GUI behavior
--------------------

One supplementary native widget case covers all five STM parameters in an ICRF
coordinate system. It uses actual owner, property and coordinate combo popups,
checks Cancel and pending drafts, exact Apply/Undo/Redo with rebuilds, Unicode
save/reopen, and five read-only report viewers. Each profile executes one
report-only mission with a fresh identity STM and four independently checked
partitions: 72 entries at the frozen 1e-12 bound, no propagation.

The case passes once in private Xvfb and once on a real nested Xephyr desktop.
The visible desktop starts minimized; its actual server, host window and separate
test display are recorded. These are new missing GUI witnesses, not reruns of the
older propagation cases. Source/image guards bind both profiles to the same
production libraries as the passing native read checks.

A supplementary worker compilation failure (missing QDialog include) and the
first GUI launcher's offscreen-platform rejection remain retained failures. The
fixture and expectations were not relaxed. The registration now uses the native
launcher and explicitly selects xcb. The failed launch did not exercise GUI
transactions. The two subsequent profiles pass.

Correction to the previous engineering report
---------------------------------------------

The sealed 20261111 STM frame-reference report described its two original STM GUI
checks as headless native X11. Their archived registration uses RunTest.py, whose
headless environment selects Qt offscreen. That platform description was wrong.
Their numerical results remain source-versioned passes, but those runs establish
neither native X11 input/exposure nor visible behavior. The prior files and
statuses remain immutable; previous-platform-correction-v1.json binds the exact
archived sources and logs establishing this correction. No requirement coverage
changes because of this correction. The new narrowly scoped native and visible
GUI witnesses supply only their explicitly listed behavior.

Admission and replay
--------------------

The stage is build/nasa-coverage/complete18-20261112-stm-base-alignment-v1.
Executed source capsules, loaded native image hashes, frozen fixtures/oracles,
raw reports, logs, failures, and corrected platform scope are retained. All 1,334
protected original inputs remain unchanged. Eight negative evidence controls
reject missing reads, an unconverted matrix, lost owner isolation, changed
expectations, missing selector input/history, offscreen-as-native and a missing
visible desktop. Frozen replay launches zero native tests and does not rerun the
unchanged whole ledger.

The previous canonical prefix remains the OEM file-invariants seal, SHA-256
``de77757b896aa68c65df2462c026b58bf11ed684b502148fa5a14c0593111574``.
The engineering receipt extends the preceding STM frame-reference receipt with
zero row credit. Historical passes retain their original executed source versions.
No full-current-build or full scientific qualification is claimed.

Remaining FRR!E525 obligations include the complete frame/parameter/body
conjunction, selected-frame function input/rebinding/restoration and repeated
calls, solver loops, domain/atomicity and extended-STM cases. Owner-dependent
axes require full state-dependent Jacobians where maps are defined; rejecting an
unproven singularity does not fulfill that obligation.
