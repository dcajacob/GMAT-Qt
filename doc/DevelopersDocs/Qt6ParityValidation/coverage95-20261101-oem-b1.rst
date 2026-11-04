CCSDS B-1 output conformance investigation
========================================

Requirement coverage remains **998/1,158 (86.2%)**: **103 complete original
requirements** are still needed for 95 percent. Historical coverage remains
**1,816/2,367 (76.7%)**. No requirement is added by this investigation.
FRR!E375 (OEM B-1) and E374 (its format parent) remain open. The goal is active.

New independent checks
----------------------

``CcsdsOemB1Oracle.py`` checks the original September2004
`CCSDS502.0-B-1 <https://ccsds.org/Pubs/502x0b1s.pdf>`_, Tables4-1 through4-3
and sections4.3.1 through4.3.7. The exact historical PDF is retained at
``build/nasa-coverage/coverage95-20261101/ccsds-502x0b1.pdf`` with SHA256
``fab306a994e5bce9cad8c64d77af172a5819af38faf459faab1b7e2f6163223c``.
This is version1.0; version2 covariance/acceleration extensions are separate.

The checker uses no GMAT reader, writer, time converter or integrator cache.
It checks ASCII/line limits, required and ordered keywords, numeric syntax and
precision, calendar/ordinal/Julian epoch ordering, metadata boundaries, usable
intervals, constant time system and object identity, comment placement and
exported-record support for Lagrange/linear interpolation. Exact decimal time
keys preserve submillisecond ordering and UTC leap-second day boundaries.
Unreviewed leap dates, custom clock conventions and unsupported interpolation
algorithms remain explicit gaps. Format checks do not establish physical
trajectory accuracy, frame orientation, units provenance or creation-clock
accuracy. Existing independent trajectory checks remain necessary.

The new CTest registration ``QtAcceptance.CcsdsOemB1OracleContract`` passes:
22 tests include positive boundaries and corrupt-file controls.
``AuditCcsdsOemB1Outputs.py`` additionally checks original receipt, snapshot,
registration, assertion source, executable, log/JUnit, raw OEM and numerical
record identities. Ten corrupt/missing evidence controls are rejected.

Actual retained-output findings
-------------------------------

All nine previously audited GUI sampling outputs were inspected at their
recorded source/runtime versions. **No native mission was replayed.**
All nine raw files retain their original audit hashes.

* **ShortArc fails section4.3.5(f).** Its file advertises degree2 Lagrange
  interpolation but supplies only two records, at0 and10seconds. A recipient
  needs three distinct records to perform the recommended degree2 interpolation.
  The existing fixture explicitly expects both degree2 and those two records.
  Both original assertions are unchanged. The internal three integrator points
  are not available to the file recipient.
* **Eight other files are incomplete for full conformance**, with no detected
  syntax/record-support violation. Their synthetic ``SatId`` is neither a
  COSPAR designator nor supported by a retained ICD agreement. The standard
  permits an agreed ICD identifier for unlisted assets. The audit does not
  invent that agreement or classify the synthetic identifier as categorically
  forbidden. ``--icd-object-id`` supports an explicitly documented profile for
  future callers; this retained-output audit deliberately supplies none.

The relevant metadata code in ``EphemWriterCCSDS.cpp`` derives its order from
``interpolator->GetPointCount()``. This investigation demonstrates the mismatch
on an actual retained output; it makes **no product or core change**.
All104 pending core paths, the previous interpreter/Qt repairs, existing
assertion sources, fixtures and numerical tolerances retain their sealed
identities. CMake only gains the new support-test registration.

A repair must count actual records in each complete exported segment, including
buffer accumulation and millisecond duplicate suppression. It must cover
sparse fixed sampling, segment resets, long/multiple-buffer arcs and one-record
degeneracy. Counting the last native buffer alone would be insufficient.
The existing ShortArc metadata expectation conflicts with such a repair; it
cannot be silently changed under the instruction to preserve assertions.
The separate historical E404 mandatory-stop versus current-guide order-fallback
conflict also remains open. Correcting exported metadata alone would not close
E404 or imply full E375 qualification.

Reusable commands and retained evidence
---------------------------------------

Run the independent checker on any generated OEM::

   python src/qtgui/tests/CcsdsOemB1Oracle.py path/to/output.oem

It returns nonzero for either a demonstrated violation or incomplete
conventions. Its JSON distinguishes ``failed`` from ``incomplete``.
Exercise its new automated contract::

   ctest --test-dir build/linux-gui \
     -R '^QtAcceptance.CcsdsOemB1OracleContract$' --output-on-failure

Repeat the added audit without rerunning GUI missions::

   python src/qtgui/tests/AuditCcsdsOemB1Outputs.py \
     --sampling-audit build/nasa-coverage/ephemeris-sampling-review/validation-summary.json \
     --standard-pdf build/nasa-coverage/coverage95-20261101/ccsds-502x0b1.pdf \
     --output /tmp/gmat-oem-b1-audit.json

That audit currently returns nonzero because the gaps above remain. Original
GUI receipts continue to prove their original bounded sampling assertions;
they are not retrospectively upgraded to full format conformance. No new
visible scenario was necessary to reproduce this serialized-output discrepancy.
Future new/affected GUI cases retain resizable, initially minimized,
non-fullscreen execution and zero passing screenshots/runtime binary copies.

The campaign directory retains ``oem-b1-retained-output-audit.json``,
``oem-b1-evidence-negative-audit.json``, ``oem-b1-change-and-failure-review.json``,
the CTest XML, contract/configuration logs, exact CMake delta and source-only
archive. The first change-audit attempt incorrectly compared a mutable workflow
report to an earlier pilot; the corrected audit checks last sealed product and
assertion identities. No source or expectation was changed to obtain its pass.
Temporary rendered standard pages were removed. No passing screenshots or
runtime executable copies were created. Scientific/core approval, complete
current-build qualification and full NASA qualification remain open.
