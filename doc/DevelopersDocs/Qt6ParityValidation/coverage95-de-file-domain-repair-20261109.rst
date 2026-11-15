DE405 file-domain and clock consistency repair
===============================================

Valid requests shortly after the first DE405 coefficient epoch were rejected
because A1 time was compared directly with a coefficient-time boundary. The
stored coefficient start also subtracted the reference epoch twice. State,
lunar orientation and position-delta reads now validate finite, converted
coefficient epochs against the declared interval before interpolation or seeking.
Both delta endpoints and Earth zero-state requests receive the same checks.
Failed seeks are rejected explicitly. Unsupported times remain errors.

The exact first epoch exposed a second defect: both forward TDB conversions
computed TT but evaluated the documented correction using TAI. MathSpec
TimeSystems.tex specifies TT Julian centuries. Both forward paths now use TT,
and both inverse paths solve that same existing two-sine model to machine
convergence. Model coefficients and reference epochs are unchanged; no clock
nudge, domain tolerance, class-layout or virtual-order change was introduced.

The added QtAcceptance.DeFileDomain regression passes 619 checks using direct
binary DE405 decoding and an independent long-double Clenshaw recurrence, plus
the independently evaluated MathSpec time model. It covers both file endpoints,
selected interiors, scalar and split epochs, TDB and TT override, lunar state
and libration, invalid/recovery controls and local one-second position deltas.
Maximum position difference is 5.821e-11 km; maximum precise clock-model
difference is 5.428e-12 seconds. These are finite model-consistency checks,
not a claim about modern physical accuracy or all DE providers.

All 55 affected original controls pass in both headless and visible profiles,
plus the added native support case: 112 case executions. Both original lunar
models retain every original assertion and numerical bound. General deltas
crossing coefficient granules/records, other platforms/providers and full
coherent regression remain open. The supplied SPK has no reference coverage
at the first file epoch and was not used as an endpoint oracle.

Review 062/checkpoint 049 bind evidence under
build/nasa-coverage/coverage95-20261109-de-file-domain-current-review.
All 1,334 original inputs, the accepted prefix and canonical ledger are intact.
Acceptance remains 1,078/1,158 (93.1%); no new requirement credit was taken.
The coherent aggregate rebuild is underway, followed by a fresh broad run.
