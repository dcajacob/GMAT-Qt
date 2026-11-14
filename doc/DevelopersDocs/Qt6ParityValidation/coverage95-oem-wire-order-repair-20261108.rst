OEM record chronology and optional metadata repair, 2026-11-08
============================================================

The native OEM 1.0 writer now rejects decreasing record epochs and repeated
epochs within one metadata block, including repetitions across buffer writes.
It permits a shared boundary epoch between successive blocks. Optional usable
times and interpolation fields are omitted after metadata clearing when unset;
a selected interpolation method requires its degree. Previously these paths
could emit invalid ordering or the internal UNSET marker.

The relevant requirements are in `CCSDS 502.0-B-1
<https://spsweb.fltops.jpl.nasa.gov/portaldataops/mpg/MPG_Docs/Source%20Docs/CCSDS-OrbitDataMessages-502x0b1.pdf>`_,
Table 4-3 and section 4.3.5(c). B-1 also distinguishes separate interpolation
blocks and requires enough exported records for the recommended interpolation.
The current review retains that original issue of the standard.

Implementation
--------------

CCSDSOEMWriter validates the records remaining after its existing adjacent
duplicate suppression before appending them. A private registry retains the
last serialized epoch and metadata position for each writer. Metadata rewrites
at the same position preserve that history; opening another file and destroying
the writer clear it. A copied writer starts its own file history. The public
writer headers, inheritance, instance layouts and virtual method order are unchanged.
CCSDSOEMSegment recognizes cleared optional fields rather than serializing their
UNSET marker. The new checks apply to OEM 1.0.

The changed production owners are CCSDSEMWriter.cpp, CCSDSOEMWriter.cpp and
CCSDSOEMSegment.cpp, plus the internal CcsdsOemB1Chronology.hpp helper.
Interpolation inputs and equations are unchanged. CMake registers the new
QtAcceptance.OemWireOrder core support test and its build dependencies.

Verification
------------

The final frozen native witness has 13 checks. With unchanged production writers,
six pass and seven fail; the independent original B-1 reader identifies the
corresponding invalid ordering, optional timestamps and missing degree. After
the repair, the same witness passes all 13. It covers disjoint/shared-boundary
blocks, descending/overlapping epochs, buffer joins, metadata rewrites, cleared
optional fields, missing degree, reopen and copy. Eight positive output files
pass the independent checker. A ninth, with optional interpolation omitted,
retains its explicit unreviewed interpolation-profile gap.

The original native text and numeric support commands pass with their original
assertions: 36 and 57 observations. Only their startup argument is redirected
using the existing isolation helper. The numeric checks include the existing
locale and OEM 2.0 acceleration/covariance controls.

Twelve original GUI executions pass: MultiBuffer, ShortArc, OneRecord,
WindowSplit, SolverEphemerisTarget and SolverEphemerisOptimize, each on private
headless and visible nested desktops. The Target/Optimize outputs are OEM 1.0;
their final-iteration behavior survives the new chronology checks. Their unchanged
SATID convention remains an explicit independent-reader ICD gap. No identifier
agreement is inferred from the emitted files.

The unchanged NarrowAtLast case still fails its scientific comparison before
and after this repair. Its raw three native samples are byte-identical. Maximum
position error remains 2.6942092880233304e-5 km against 2e-5 km; velocity error
remains 2.9043781227255927e-8 km/s against 2e-8 km/s. The original order-reduction
warning remains. Its B-1 text check passes; that does not qualify its trajectory
accuracy. All 1334 protected original inputs remain byte-identical.

Original scope and continuation
-------------------------------

FRR!E374's original format list names OEM and SPK. Current source dispatches
both; the SPK route uses native Type 13/Hermite output with odd-degree validation.
This review adds no new SPK numerical qualification. The B-1 format and option
headings remain open, including the scientific failure and broader conventions.

For FRR!E381, `B-1 section 4.2.2
<https://spsweb.fltops.jpl.nasa.gov/portaldataops/mpg/MPG_Docs/Source%20Docs/CCSDS-OrbitDataMessages-502x0b1.pdf#page=28>`_
supplies reference-frame examples rather than
an exhaustive authorized list. It does not recover the worksheet's unfinished
coordinate-system specification. Existing frame mappings remain present;
the missing historical scope and all-frame qualification remain open.

Canonical acceptance is unchanged at 1078/1158 (93.1%), with 23 further rows
needed for 95%. No original assertion, tolerance, denominator, formal hold or
qualification gate changes. The finite audit continues with FRR!E406: estimator
and nested iterative-output paths still require review beyond the bounded
differential-correction/optimization checks above.

Artifacts are in ``build/nasa-coverage/coverage95-20261108-oem-current-review``:
the final before/after witness, raw files, original support/GUI receipts,
independent checks, source preimages and ``implementation-v1.patch``.
``publication-v1.json`` binds the completed component. The initial prototype
witness is diagnostic only; the frozen v3 witness supplies the before/after
comparison. Installed runtimes, releases and Git remotes were not updated.
