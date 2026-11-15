STM solver-loop restoration and nested-function recovery
========================================================

This targeted batch verifies selected-frame STM calculations across Target and
Yukon Optimize trials, then fixes a demonstrated propagation-state association
defect exposed by a nested-function rejection/recovery case. One production
source file changes. Public APIs and native object layouts are unchanged.

Coverage remains 1,106/1,158, with 5 of the agreed 18 complete. FRR!E525 remains
open pending its complete frame/parameter/body/domain conjunction. Original
assertions, numerical bounds, protected tests and failure receipts are retained.
No full regression was run.

The demonstrated defect
-----------------------

A function's spacecraft input aliases can differ from their retained full
names. In the failing nested call the two inputs are named craft/guide, while
their full names remain Sat/Ref. PropagationStateManager keyed its association
map by full name, but supplied the alias as the self-association lookup key.
The missing lookup silently inserted index zero. Therefore the guide's STM
force derivatives used the first spacecraft's Cartesian state.

The guide's incoming 60-second STM is independently correct, and its trajectory
state remains correct. Only its additional variational propagation is wrong.
Native state-map diagnostics show the guide's Cartesian state starts at index 6,
but its STM uses index 0. An independent RK4 integration of the variational
equation using the first spacecraft's known circle reproduces the wrong guide
STM to 8.526512829121202e-14. The error against the correct guide STM is
0.0238466432815585 seconds in its B block, far above the unchanged 2e-6 bound.
Both original independent failures remain failed, with their original matrices.

The repair uses GetFullName for self-associations, consistently with the
existing association-map key. Explicit associated-object behavior is unchanged.
The corrected native diagnostics assign the guide's STM to index 6. The repair
changes only PropagationStateManager.cpp; it does not alter force equations,
integrator equations, expectations or tolerances.

Targeted validation
-------------------

The Target case varies the initial phase of a circular point-mass orbit and
achieves an independently calculated final X coordinate. The Yukon case uses
a known quadratic phase objective and a declared constraint. Every executed
trial must restore the initial state, epoch and identity STM, then calculate
and report the full selected-frame STM and its four partitions in a guide VNB
frame. Independent circular variational flow and two-end R/Rdot Jacobians supply
the expected values, including each trial's actual control.

Target exercises 10 trials and Yukon exercises 13. Both pass before repair and
again after the shared state-manager change. These are the two directly affected
passing cases selected for dependent checks; older unrelated passes are reused
at their recorded source versions. The actual native branch solver clones
converge and reach the independently specified phase/trajectory goals.

The additional nested-function case requests all five STM forms relative to the
spacecraft's own origin. That map is singular. Direct native getters reject it
with diagnostics and leave owner state, guide state and cached values unchanged.
The actual Report command then raises the same native exception; RunMission
returns the frozen expected error status -5. Two nested function frames unwind
without changing the caller's state, STM, selected-frame values or output
variable. No partial report values or post-call continuation are emitted.

Recovery changes only CheckFrame.Origin from craft to Earth, rebuilds the same
mission, and reruns it. Native status is 1, all five function STM/partition reports
match independent 90-second results, the caller remains exactly at 60 seconds,
and the scalar return and continuation sentinel agree with their expectations.
The originally wrong guide STM now satisfies its own independent oracle.

After repair the targeted evidence comprises 23 solver trials, 145 native
Parameter reads, 1,944 reported matrix entries, five atomic native rejections,
and the nested error/unwind/recovery route. All 1,334 protected original inputs
remain unchanged. Eleven negative evidence controls reject missing trials,
stale clocks, missing identity resets, wrong frame/readonly contracts, the wrong
guide STM, a relabelled error, lost rejection atomicity, caller leakage, partial
reports and the wrong native association index. Frozen replay launches zero
native tests; it does not replay the unchanged whole ledger.

Source and failure provenance
-----------------------------

The stage is build/nasa-coverage/complete18-20261112-stm-solver-frames-v1.
Executed source capsules, actual loaded worker/library/plugin hashes, fixtures,
independent readers, state-map diagnostics and raw reports/messages are retained.
The solver-loop and recovery expectations were frozen before their executions.
The native function plugin and Yukon plugin sources are included in the relevant
capsules. Only affected targets were rebuilt.

A supplementary worker compilation error (a missing bool return value) remains
retained and executed no native cases. Two subsequent native procedures finish
but their independent guide-STM audits fail; neither is relabelled as passing.
A diagnostic-only worker extension captures the initial association map without
changing expectations. After the production repair the same frozen numerical
expectations pass. The intentional native error status -5 remains visible in
both failing and passing procedure receipts; it is distinct from the unintended
numerical failure.

After successful execution, a misleading unmeasured producer metadata key was
renamed from production_changes to native_worker_source_writes. It records that
the native worker edits no production sources while running. The stage's actual
production patch affects one file. Original receipts remain unchanged, and the
reader explicitly accepts their legacy key. This metadata-only cleanup was
rebuilt, not rerun natively; passing scientific evidence remains bound to its
actual after-v1 source/image capsule. It is not a claim of whole-current-worker
or whole-current-build qualification.

This engineering stage extends receipt
``654a3b61691fa1d8825b79746e03200df8d45374e3dd3cf92ca0f4d0603adf35``.
The canonical requirement prefix remains
``de77757b896aa68c65df2462c026b58bf11ed684b502148fa5a14c0593111574``.
No new requirement row is admitted by this bounded batch.

Remaining FRR!E525 obligations include reconciliation of the complete allowed
frame/parameter/body inventory against saved evidence, defined owner-dependent
frame Jacobians, broader rejection/atomicity and extended-STM cases. Unsupported
rejection is not counted as proof of mathematical singularity.
