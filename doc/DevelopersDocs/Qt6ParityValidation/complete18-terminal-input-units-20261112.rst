Terminal input unit display reconciliation
=========================================

The remaining eleven records in the saved numeric input worklist are
reconciled without granting whole-row credit to All Others!E13. The original
requirement denominator remains 1,158 and the sealed count remains 1,111;
10 of the agreed 18 rows are complete.

NAIF identifiers and tracking identifier/type-code controls now state their
meaning explicitly. They are not physical quantities. MessageWindow.Precision
is labeled in digits. This uses the concrete type name because the legacy
MessageWindow constructor does not add its name to the IsOfType hierarchy.
No native parameter values, numerical algorithms or public APIs changed.

The existing A1 modified Julian epoch captions were verified for MOEEq and
MOEEc. A supplementary covariance display check inspected every row and column
header for Cartesian and Keplerian solve-fors, the product-of-units explanation,
and the warning that changing solve-fors does not convert pending values.
The covariance matrix, native object, source, saved file and undo history
remained unchanged. No filter or propagation mission was executed.

Validation retained the original nine missing-description observations, the
first precision lookup failure and the initial covariance test interaction
failure. That test was corrected to expose the All Properties section before
clicking its control. The independent expectations did not change. Passing
headless epoch, covariance and identifier witnesses were reused; only the
failed labels were rerun. The visible profile covered all terminal controls.
Together these runs provide 74 passing GUI observations, with every failure
retained in its original receipt.

Evidence is in
``build/nasa-coverage/complete18-20261112-terminal-input-units-v1``.
The immutable stage verifies archived source and runtime image identities,
original protected test inputs, and negative controls for identifier units,
epoch scale and the covariance mean-anomaly basis. Its replay launches no
native tests. The full current input inventory and its GUI evidence still
need reconciliation before E13 can receive whole-row credit.
