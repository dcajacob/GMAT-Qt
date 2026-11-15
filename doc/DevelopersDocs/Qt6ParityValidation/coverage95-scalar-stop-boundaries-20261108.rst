Scalar stop boundaries, 2026-11-08
=================================

Two demonstrated implementation defects are repaired. Seven new native cases
and ten unchanged GUI regressions pass, including sixteen expected scalar/index
rejections and recovery. Acceptance remains1076/1158 (92.9%), with25 more original
requirements needed for95%. This component adds no whole-row credit.

Corrections and executed behavior
---------------------------------

Propagate initialized the stopping Parameter objects but left nested Array index
wrappers referencing configured Variables. Changing the run's Index from1 to2
still read Grid(1,2). Initialization now binds the complete measured/goal wrappers
through the existing SetWrapperReferences helper. The actual bound condition now
reads3,9,3 as its run index changes1,2,1. An additional mission changes the index
before propagation and stops at the selected9s goal rather than the first row's3s.

StopCondition retained its prior time conversion kind when reinitialized with a
non-time scalar. A copy rebound from ElapsedDays reported86400 while an equivalent
fresh scalar reported1, despite neither being a time condition. Validate now
resets the kind before classifying the current Parameter. Rebound/fresh scalar
conditions agree, and angular reinitialization also removes cyclic wrapping.
The original bound conditions retain their own metadata. No signature, layout,
virtual slot, interpolation formula or numerical limit changed.

Actual point-mass missions check mixed user/system conditions, an unpropagated
goal owner one day away, valid matching-epoch Formation members, and the changed
goal index. Eleven condition/base-clock observations and twelve nominal Cartesian
vectors (72 components) pass independent circular equations using a pinned Earth
mu. Bounds remain2e-5km,3e-8km/s and2e-6s. Coupled-member epochs must match under
the existing native rule; this does not qualify arbitrary mismatched formations.

The actual bound Variable/Array conditions additionally check changed cells,
separate wrapper rebinding, NaN/infinities, invalid/noninteger/nonfinite indices,
whole-Array/missing values, invalid goals and finite subtraction overflow.
Sixteen native exceptions are required, with subsequent valid evaluation after
each recoverable error. This is bounded native evaluation, not every scalar
parameter/domain or general function-value ownership qualification.

Unchanged regressions and retained failures
-------------------------------------------

The original GoalVariable, GoalArray, GoalProperty, FirstStepRepeat and
FirstStepBeforeRoot tests pass in both headless and visible profiles. All1334
protected original inputs, assertions, fixtures and tolerances are unchanged.
The GUI profiles have identical1181-source-entry/107-binary-entry inventories.
Visible nested execution is resizable, requests an initially minimized desktop,
records49 native exposures and takes no screenshots. Headless tracing is disabled;
its xcb/Xvfb registration and passing results are retained separately. These
observations do not establish native OS input or cross-platform qualification.

The seven new native cases bind1745 sources and32 runtime images per run. The
earlier six cases are reused at their recorded generator version; the final
generator produces byte-identical inputs/oracles for those cases. The added
goal-index case uses the same production/C++/binary identities. No completed
unrelated SPK/Save/ephemeris/function matrices were repeated.

Both initial new-case failures remain, including the86400-versus1 raw record.
An initial compile elevated existing native-header warnings; only the new target
permits the existing overloaded-virtual/unused-parameter categories as warnings.
Other witness warnings remain fatal. Static review corrected an unexecuted test
block to use independent wrappers: ordinary StopCondition copies intentionally
retain borrowed wrappers. Production copy semantics were preserved. A new reader
also initially expected visible observer records in the headless profile; that
metadata assumption was corrected without native reruns or numerical changes.

Evidence and scope
------------------

Stage: ``build/nasa-coverage/coverage95-20261108-scalar-stop-boundaries``.
Read its component audit/replay, exact product preimages/patches, builds, raw
proofs/logs, recorded fixtures, source capsules and publication. The GUI capsule
contains1180 text/source entries; the ELF observer is represented only by its
identity. No runtime binary was copied.

All original scopes, fingerprints, denominators and hold identities/statuses
remain. Full current-build, NASA, scientific/core, physical-frame, platform,
external-ABI and native-OS-input gates remain open. The private integrated runtime
was rebuilt; installed applications, published packages and public code were not
updated. The campaign remains active.
