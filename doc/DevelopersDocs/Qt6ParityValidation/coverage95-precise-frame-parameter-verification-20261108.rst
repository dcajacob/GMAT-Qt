Precise frame-parameter write repair, 2026-11-08
================================================

The current FRR!E13 implementation review found an epoch mismatch in OrbitData.
GetCartState preserves a precise spacecraft or publication-sample epoch when
reading a frame-dependent parameter. The inverse write instead rounded the
epoch through a Real MJD. A read followed by an unchanged write could therefore
change the internal spacecraft state in a time-dependent frame.

The repair selects the same precise epoch policy for inverse frame conversions,
retaining the ordinary A1Mjd overload for spacecraft without precision time.
Both the general parameter-write branch and the companion anomaly branch use
this policy. No public signature, class layout, conversion formula or tolerance
changed. Origin translation and covariance epoch paths are outside this repair.

Native diagnostic and verification
----------------------------------

Stage: ``build/nasa-coverage/coverage95-20261108-state-conversion-current-review``.
New ``OrbitParameterEpochTests.cpp`` and ``RunOrbitParameterEpoch.py`` register
``QtAcceptance.OrbitParameterEpoch``. They use the existing private startup and
native process-map handshake without editing its helper.

Five native cases exercise the public X/VX parameter APIs. A prescribed rotating
frame has a closed-form rotation and derivative; independent six-component
expectations check unchanged writes, position increments and velocity increments.
The synthetic 0.25 rad/s frame makes a 100 ns epoch loss observable; it does not
model a physical body's rotation. Limits were fixed before the product repair:
1e-9 km for position components and 1e-9 km/s for velocity components, with exact
precise-versus-ordinary epoch dispatch checks.

``native-diagnostic-v1`` retains two passing ordinary-epoch controls and three
precise-epoch failures, including a 0.000175 km position change on an unchanged
write. ``native-final-v2`` passes all five with identical inputs, expectations
and limits. All cases record their 139 loaded ELF identities. These checks
directly exercise the general Cartesian setter; the companion frame-anomaly
branch and sampled-publication writes have no new dedicated diagnostic here.

Existing GUI checks and retained failure
----------------------------------------

Unchanged ``QtGui.NasaOrbitConversionCartesian`` and the XFrame/VXFrame cases of
``QtGui.NasaFunctionCartesianFrameBinding`` have passing executions in headless
and visible modes. The conversion editor check covers five outgoing pairs,
pending Cancel/Apply, source history, Unicode reopen and independent reported
Cartesian results. The function cases retain their existing owner/frame graph,
mutation, report and sentinel assertions.

One visible VXFrame execution failed at report-viewer activation after its
numeric phases. A diagnostic rerun with six screenshot opportunities passed.
The intermittent activation failure remains OPEN; a later passing run does not
waive it. The original failed log is retained. The first GUI preflight executed
no tests because the default review referenced changed documentation; a new
execution-only manifest contains no acceptance claims. Two initial headless
function cases could not open their X display in the sandbox; the authorized
display-access retries passed. The already passing editor case was not repeated
in that headless retry.

All 1,334 protected original inputs/assertions match their baseline hashes.
Native and GUI source guards, GUI binary guards and a verified source archive
are recorded in ``results-summary-v1.json``. Existing tests, fixtures, assertions
and numerical bounds were not edited. Original denominators, fingerprints,
scientific references and acceptance gates remain unchanged.

Remaining original obligation
------------------------------

The source census finds all 182 nonidentity dispatch routes among 14 advertised
representations. This is source presence, not independent numerical qualification.
FRR!E13 remains open: its complete allowed representation/body/frame matrix,
domains, frame rebinding and rollback still require appropriate evidence.
Rounded origin-translation and covariance epoch calls need separate diagnostics;
their formulas and conventions were not changed speculatively.

No new requirement credit is claimed. Canonical acceptance remains
1,078/1,158 (93.1%), with 23 more requirements needed for the unchanged 95% target.
The campaign goal remains active.
