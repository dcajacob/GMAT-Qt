===========================================
OEM exported-record interpolation support
===========================================

Coverage remains **1,051/1,158 (90.8%)**. There are 107 exact requirements open
and 50 needed for 1,101. Historical operations remain 1,816/2,367. All 59 original
hold identities,including 28 unresolved holds,remain unchanged. This component
repair earns **zero new requirement credit**;FRR!E375 and parentE374 remain open.

What the remaining work means
-----------------------------

The source-based planning review in
``build/nasa-coverage/coverage95-20261104-next-closure-queue/feasibility-review-v1.json``
partitions the 107 open exact rows into 31 engineering/validation tracks,six
MATLAB integration/specification tracks,42 specification/dependent-conjunction
tracks,and 28 legacy/missing/undefined-prerequisite tracks. There are no confirmed
quick wins. These are planning categories,not promises of independent closure.

The 28 prerequisite tracks comprise 11 removed two-body rows,eight legacy-OS
rows,six MarsGRAM rows,and three undefined hardware rows. At least four literal
incomplete requirements cannot be verified as written:AllOthersE294-E296 contain
unspecified minimum hardware values,andFRRE381 says to fill out the CCSDS frame
list. No row is declared permanently impossible. Even closing all 37 technical
and MATLAB candidates would require 13 additional closures from other categories.
The unchanged 95 percent target therefore remains conditional on source decisions
and prerequisites. Installing MATLAB removed an important dependency;the remaining
six MATLAB rows involve integration or specification obligations beyond installation.

Actual defect and repair
------------------------

Before the repair,new ShortArc and SparseArc exports each contained two records,
but advertised Lagrange degrees 2 and 7. The independent September2004 CCSDS502.0-B-1
oracle rejects both under section4.3.5(f):the file recipient needs degree + 1 records.
The exact standard remains ``coverage95-20261101/ccsds-502x0b1.pdf``,SHA256
``fab306a994e5bce9cad8c64d77af172a5819af38faf459faab1b7e2f6163223c``.

``EphemWriterCCSDS.cpp`` now caps the recommended degree using the complete
pending OEM segment's serialized-record count,instead of the internal interpolation
buffer. ``CCSDSOEMWriter.cpp/.hpp`` add one public nonvirtual getter matching
adjacent UTC-millisecond duplicate suppression. The existing segment store supplies
buffer accumulation and reset semantics. One record supports degree 0. No member,
class-layout or virtual-slot change is introduced;one exported C++ method is added.
The existing internal interpolation algorithm remains separate.

All 280 previously pending core paths are unchanged. Three newly pending paths
bring the inventory to 283. Exact preimages,``core-increment-v3.patch``,
``core-increment-v5.json`` and the full ``core-change-inventory-v1`` retain every
HEAD/current source and patch. Technical approval and full-build qualification
remain false.

New and affected results
------------------------

Five new GUI cases pass both headless and visible profiles:

* ShortArc:two records,degree 1.
* SparseArc:two exported records despite a richer internal sampling history,degree 1.
* ThreeRecords:three records,degree 2.
* MultiBuffer:1002 records across the native1000-record buffer boundary,degree 7.
* LongThenShort:1001 records then two after a disabled interval,degrees 7 and 1.

All retain the original GUI lifecycle,independent circular-state bounds and
terminal-state checks. Only the new controlled sender and receiver explicitly
agree on the synthetic test-object identifier. This is not a retroactive ICD
agreement for retained ``SatId`` files or an operational mission agreement.

The sixth new case,OneRecord,sets equal explicit export bounds at10seconds.
It produces only an OEM header and fails in both profiles. This failure is retained;
no tolerance,expected count or acceptance gate is relaxed to make it pass.

The separate native writer support check passes with 1,007 independently parsed
states/epochs. It covers formatted-epoch duplicate suppression,last-record retention,
1,000 + 3 accumulated records,reset and a single-record/degree 0 next segment. Its
first support attempt retains a fixture assumption failure and missing live maps;
``support-review-v4.json`` explains the corrections. The final run uses an optional
ready/ack handshake to retain actual live maps. Existing writer-copy behavior is
preserved;copies do not copy pending segment data.

Affected original FixedAligned and ToggleGap pass both profiles. Original ShortArc
fails both because its unchanged catalog still expects degree 2 with two records.
The repaired raw file contains degree 1 and has only the existing unresolved ICD
identifier limitation. ``proposed-original-ShortArc-expectation-v1.patch`` contains
exactly one proposed value change,2to1. It is **not applied or approved**. The active
campaign instruction to preserve assertions requires an explicit exception before
changing that original expectation. All original driver/catalog bytes remain intact.

There are 20 actual GUI contexts: 14 passes and six retained failures. The final
visible selection ran nine contexts in an initially minimized,resizable,nonfullscreen
1600x1200 nested display.21 native identities and one display-server identity are
retained with terminal observations. No runtime binary or kernel copies were made.

Replay and limits
-----------------

``component-audit-v2.json`` replays all five source capsules,successful source/build
guards,actual loaded-image observations,independent B-1 checks,numerical proof bounds,
unchanged original assertions,remaining 107 rows and 59 hold identities. Source1 is
an unexecuted draft;source2 retains the before-fix witnesses;source3 retains the new
GUI results;source4 retains the failed support attempt;source5 owns the final support,
original regression and visible results. Native tests are not rerun by the audit::

   python3 build/nasa-coverage/coverage95-20261104-oem-record-support/verify-component-v2.py

New reusable registrations are ``QtGui.NasaOemRecordSupport{ShortArc,SparseArc,
ThreeRecords,MultiBuffer,OneRecord,LongThenShort}`` and
``QtAcceptance.OemWriterRecordSupport``. The latter is a non-GUI support test;the
ordinary RunAcceptance inventory deliberately selects only the qt-gui label,so
``run-support-with-maps-v1.py`` runs it separately. The source3 run name containing
"and-support" does not imply that support registration executed there.

Reuse the 1,051 prefix in ``coverage95-20261104-next-closure-queue/sealed-prior-prefix-v1.json``;
``prior-prefix-replay-v1.json`` confirms it still replays without native execution.
This repair does not establish every OEM frame,time system,physical unit,creation
clock or reader behavior. The equal-window failure,original assertion conflict,
E404 mandatory-stop/fallback conflict and complete E375/E374 obligations remain open.
All broader scientific,NASA,technical and current-build gates remain false.

Final packaging correction
--------------------------

Use ``evidence-manifest-v2.json``, ``stage-receipt-v2.json`` and
``seal-verification-v2.json``. The first manifest followed 300 runtime symlinks
and bound mutable target hashes. Its preflight failure and original seal are
retained as superseded evidence. Version2 excludes every symlink from artifact
bindings and retains link text separately as temporal runtime observations.
The component contains no regular native binary or kernel files. All actual
source/build/map/test observations remain unchanged;no native rerun was needed.
