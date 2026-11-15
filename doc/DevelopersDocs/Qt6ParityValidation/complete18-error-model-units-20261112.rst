Measurement-error input units
=============================

The existing editor already updates Bias, BiasSigma and NoiseSigma units when
the pending measurement Type changes. It reads units from a detached native
ErrorModel clone and updates both the backing table and visible field labels.
This batch verifies that behavior; no product code changed.

The six targeted headless/visible runs pass 312 observations. Initial resource
views cover the five unit families: km, km/s, Hz, range units and degrees.
Pending switches cover all nineteen ordinary measurement types. Separate
testing-mode checks cover RightAscension and Declination without exposing them
as ordinary-mode choices. Values are not converted when the measurement type
changes. Discard preserves the original native object, script, saved file and
undo history. No mission or full regression was run.

The frozen reader compares the ordinary types with the tracking-data reference,
checks the native unit map and degree-based testing adapters, and verifies the
actual selector/label receipts, source/build/runtime identity, native exposure
and state isolation. Original product sources match the previous sealed stage;
only supplementary test registration was added. Negative controls reject a
wrong measurement-unit family or a missing observation.

This completes fifteen saved unit records, reducing the queue from 88 to 73.
The queue is not a defect count or a complete census of current conditional
inputs. All Others E13 remains open; canonical coverage stays at 1,111/1,158 and
ten of eighteen agreed requirements.

Evidence is sealed under
``build/nasa-coverage/complete18-20261112-error-model-units-v1``.
