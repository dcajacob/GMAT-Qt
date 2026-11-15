Polynomial array input units
============================

FrequencyBias/FrequencyBiasSigma and the four thrust-angle coefficient arrays
had no unit summary or per-cell unit labels. Their main inputs now describe
the unit series. Cell-editor headers identify each coefficient and its unit:
MHz, MHz/s, MHz/s^2, ... for transmitter frequency, and deg, deg/s, deg/s^2, ...
for thrust angles and their uncertainties. The native equations use elapsed
seconds; no numerical conversion, setter or native unit metadata was changed.

Headers update when the existing thrust-vector editor adds or removes columns.
Transmitter vectors retain their existing fixed-size editor. Covariance and
other numeric grids retain their prior headers and behavior.

Seventy original missing-unit observations remain in their failed receipt.
The unchanged supplementary test passes 140 observations across headless and
visible profiles. It checks main-input and backing-table summaries, every
header of three-element vectors, and thrust-vector lengths 3, 5, 2, then 3.
Cancel preserves source, files, native objects and undo history throughout.
These are display checks; no mission or full regression was run.

The frozen reader checks guide definitions and native elapsed-time polynomial
equations, archived source and executable identities, raw GUI observations and
the narrow metadata/header-only patch. Negative controls reject a scale change,
an incorrect polynomial order and an omitted resized-column observation.

Six more entries leave the saved unit worklist, reducing it from 94 to 88.
The worklist is not a complete current-input inventory or a defect count.
All Others E13 remains open, and canonical coverage stays at 1,111/1,158 and
ten of eighteen agreed requirements.

Evidence is sealed under
``build/nasa-coverage/complete18-20261112-polynomial-array-units-v1``.
