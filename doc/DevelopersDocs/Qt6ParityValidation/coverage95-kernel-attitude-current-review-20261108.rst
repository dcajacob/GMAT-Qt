Kernel-attitude nomenclature review, 2026-11-08
===============================================

The original FRR!E53 text says "SPK attitude kernel". The shipped NASA help uses
the same SPK label but configures its SpiceAttitude example with .BC pointing,
.TSC clock and .TF frame files, separately from its SPK orbit ephemeris. Current
SpiceAttitude requires CK/SCLK inputs and warns that an FK may be needed. Its
reader converts spacecraft clock time and calls ckgpav_c for pointing and rates.

NAIF's `CK required reading
<https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/req/ck.html>`_ defines the
spacecraft/instrument attitude system and its pointing/rate readers. Its
`SPK required reading
<https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/req/spk.html>`_ defines the
ephemeris-state system. Together with the actual NASA example and native reader,
these sources reconcile the historical label with the implemented SPICE attitude
path. This is a source-based nomenclature interpretation, not a rewrite or waiver
of the original requirement.

Documentation repair
----------------------

Resource_SpacecraftAttitude.xml now identifies CK pointing, FK frame and SCLK
clock files by their appropriate types. It retains the historical "SPK attitude
kernel" term in an explanatory paragraph, distinguishes the existing attitude
and orbit script fields, and describes FK input as conditional on needed frame
definitions, matching the native model's existing behavior.

The XML is well formed. All eighteen script-example blocks and six XML IDs are
unchanged. The eight captured native/GUI owners and all 1,334 protected original
inputs/assertions are unchanged. No native calculation, script name, example
kernel path, class layout, fixture or tolerance changed. No application build
or runtime replay is needed for this wording-only repair. The validation used
Python's standard XML parser after finding optional lxml unavailable; no new
dependency was installed.

Stage: ``build/nasa-coverage/coverage95-20261108-kernel-attitude-current-review``.
It retains the complete E50-E53 source context, preimages, indexed patch, current
hashes and validation. The original worksheet text, fingerprints and denominators
remain unchanged. No new parser, angular-velocity convention or sub-Real time
accuracy obligation was inferred from the naming discrepancy.

Remaining obligation
----------------------

Source inspection and corrected labels do not qualify actual CK/clock/frame GUI
configuration or independent pointing/rate results. FRR!E53 remains open for that
evidence. FRR!E50 also retains the complete model-family obligation, including
the fixed-offset model's any-coordinate-system condition and the recorded
velocity-frame angular-rate failure. Those obligations were not replaced by a
single kernel example or by this terminology review.

No acceptance credit or qualification gate changed: 1,078/1,158 (93.1%), 80 open,
23 more needed for 95%. The campaign continues through the remaining original
implementation groups.
