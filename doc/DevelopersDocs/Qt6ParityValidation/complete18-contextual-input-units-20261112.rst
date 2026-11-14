Context-dependent input units
=============================

Residual, objective, gradient and integrator-error inputs do not all have one
fixed physical unit. Their GUI labels now describe the applicable numerical
scale instead of showing a blank or calling them unconditionally dimensionless.
Weighted-residual inputs retain their nonzero-covariance versus unit-weight
fallback distinction. Yukon labels distinguish constraint, objective-change,
Lagrangian-gradient and elastic-penalty scales. VF13ad retains its documented
objective/constraint meanings. Propagator intervals display seconds, while
error thresholds identify the selected error metric's scale.

AcceptFilter.ThinningFrequency now displays records in Frequency mode and
seconds in Time mode. Changing the pending mode refreshes the label using a
detached preview; configured values, source, files and undo history are not
changed. Native calculations and public interfaces are unchanged.

The focused batch passes 112 observations across headless and visible profiles.
It covers residual controls, optimizer controls, all thirteen propagator types,
and Frequency-to-Time-to-Frequency unit updates. Fifty-six original label
failures are preserved. An initial supplementary fixture incorrectly requested
StepSize on SPICESGP4, which exposes InitialStepSize; its missing-field failure
and 38 earlier observations remain separate. The corrected field retains the
same seconds expectation. No mission or full regression was run.

The frozen reader checks the guide definitions and native weighting, objective-
change, covariance and thinning equations; archived source/build/runtime
identities; raw labels and native exposure; state isolation; and the exact
metadata/preview-only changes. Negative controls reject an unconditional
dimensionless label for a conditional residual scale and a stale thinning unit.

Twenty-two saved entries are complete, reducing the numeric-unit worklist from
33 to 11. That queue is not a complete current-input inventory or a defect
count. All Others E13 remains open. Canonical acceptance stays at 1,111/1,158,
ten of eighteen agreed requirements.

Evidence is sealed under
``build/nasa-coverage/complete18-20261112-contextual-input-units-v1``.
