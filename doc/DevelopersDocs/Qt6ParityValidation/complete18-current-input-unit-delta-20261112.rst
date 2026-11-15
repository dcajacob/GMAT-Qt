Current input-unit domain delta
===============================

The refreshed inventory measured 85 factory types, 1,047 distinct type/field
pairs and 357 Qt construction templates. It found 39 numeric field identities
absent from the older inventory. MessageWindow.Precision was already covered
by the terminal-input stage. This batch adds GUI witnesses for the other 38.
Inventory measurements themselves receive no GUI or requirement credit.

Seven unit descriptions were missing from the generic property metadata:
RootTolerance on three event locators, IntegrationStepsToSkip on three plot
types, and TextEphemFile.Interval. The event and plot primary captions already
described seconds or step counts; the generic metadata now agrees. The text
ephemeris interval now explicitly displays seconds.

The initial MinimumTolerance expectation followed an existing incorrect
dimensionless description. Inspection of BulirschStoer and ODEModel showed
that MinimumTolerance floors Accuracy and is compared to the same normalized
or absolute error estimate. Its GUI description and reference source now say
error-metric units. The earlier passing label check remains archived and is
explicitly excluded from valid unit evidence. Numerical tolerances and native
algorithms are unchanged. Generated/installed delivery of the corrected help
paragraph remains tracked separately.

Passing headless body-orbit, quaternion and optional solver witnesses were
reused. Only the eight corrected descriptions were rerun headlessly; the
visible profile covered all 38 newly inventoried fields. The batch records
76 valid passing GUI observations, eight retained failures and one excluded
initial-oracle pass. It executes no missions or full regression suite.

The immutable evidence stage is
``build/nasa-coverage/complete18-20261112-current-input-unit-delta-v1``.
Its replay checks source archives, image identities, exact field inventories,
raw observations, isolation invariants and negative controls. The canonical
count remains 1,111/1,158 and 10/18 agreed rows. All Others!E13 still requires
the full current input/evidence conjunction before whole-row admission.
