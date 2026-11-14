=================================================
CCSDS UTC creation time and approved ShortArc fix
=================================================

Coverage remains **1,051/1,158 (90.8%)**: 107 original requirements are open,
and 50 more are needed for 1,101. Historical operations remain 1,816/2,367.
All 59 original hold identities survive. This component adds no requirement
credit; full FRR!E375/E374 and broader qualification gates remain open.

Approved original test change
-----------------------------

The user explicitly approved the reviewed ShortArc patch after clarification.
Only expected_degree changes from 2 to 1 in NasaEphemerisSamplingCases.json.
The file still exports two records, and every physical state, epoch, count,
GUI assertion and numerical tolerance is unchanged. Degree 2 requires three
records; degree 1 is supported by the two actual exported records.

assertion-exception-approval-v1.json and approved-ShortArc-expectation-v1.patch
retain the approval, exact preimage and before/after hashes. The original
C++ driver is byte identical. The audit compares the complete original catalog
against an independent deep copy with exactly that one value changed. Both
headless and visible ShortArc contexts now pass. Earlier degree-2 expectations,
invalid metadata outputs and failed regression receipts remain in frozen stages.
This is one explicit assertion exception, not a general waiver.

Localized UTC repair
--------------------

CCSDS 502.0-B-1 section 4.3.7(b) requires creation time in UTC. The former
CCSDSEMWriter::WriteHeader used the generic local-time formatter. Actual Denver
outputs were seven hours behind the UTC run interval. The new support test also
reproduces the half-hour-offset error in Kolkata, while UTC is a passing control.

WriteHeader now converts the current time to a local UTC calendar structure,
using gmtime_r on Linux and the MSVC gmtime_s branch, and retains the existing
whole-second ISO layout. Conversion/formatting errors are explicit exceptions.
The original closed-stream behavior remains intact. TimeTypes.cpp and the public
header are unchanged. Generic local-time reporting is checked independently and
continues to use each process's local timezone.

This adds one newly pending core path, CCSDSEMWriter.cpp. All 283 previously
pending paths are unchanged; the inventory now has 284. No member, virtual slot
or public API is added. Exact preimages, core-increment-v2.patch and the full
core-change-inventory-v1 retain all pending HEAD/current sources and diffs.
Linux is executed; the MSVC branch has source coverage only.

Actual verification
-------------------

The new native executable creates both OEM versions 1.0 and 2.0 under each of
UTC, America/Denver and Asia/Kolkata. The independent Python receiver brackets
the actual process with UTC times, checks the serialized creation field, and
checks the unchanged generic local clock using independently obtained offsets.
Native fields have whole-second precision, so the lower bound is quantized to
a whole second. This does not alter any orbit-state tolerance.

Before the repair, UTC passes and Denver/Kolkata fail. Afterward, all three
zones pass for both versions, including Kolkata's different local calendar day. The
version-1 file also passes the B-1 syntax/count oracle under an explicit synthetic
sender/receiver identifier agreement. Version 2 is checked for its creation header;
its complete format is outside this historical B-1 component.

The affected GUI selection includes the approved original ShortArc and the
controlled aligned OneRecord case. All six native GUI contexts pass their original
orbit/GUI assertions. Both pre-repair creation fields fail the independent UTC
check; both cases pass that check after repair in headless and visible profiles.
The visible desktop is initially minimized, resizable and nonfullscreen.

ShortArc's retained SatId still has no recorded ICD agreement, so the independent
format audit remains incomplete for exactly that identifier reason. No agreement
is invented. The controlled OneRecord file passes syntax/count and UTC checks.
Neither observation is promoted to full NASA or universal science qualification.

There are six native support contexts and six GUI contexts. The two failed
non-UTC support checks and two failed pre-repair GUI UTC checks are retained.
All 12 actual native identities and the display server were observed terminal
in the same host namespace used for execution. No runtime binary or kernel copies
are made. Mutable runtime links are recorded as temporal metadata rather than
bound target bytes.

Reusable evidence and remaining scope
-------------------------------------

Three reusable support registrations are QtAcceptance.OemCreationClockUTC,
QtAcceptance.OemCreationClockDenver and QtAcceptance.OemCreationClockKolkata.
Existing GUI registrations and original numerical limits remain in place.

The frozen component reader verifies both source capsules, matching successful
builds, actual loaded images, independent clock results, preserved local reporting,
the exact approved exception, all 1,158 original row records/statuses, all 107 gap
records and all 59 hold identities. The prior window component and 1,051 prefix
also replay after the new builds without native reruns::

   python3 build/nasa-coverage/coverage95-20261104-oem-creation-utc/verify-component-v1.py

Full requirement and remaining-gap audits are in this stage. Their original
rows/statuses are preserved, with progress annotations only. All technical,
complete-current-build, full-NASA and goal-completion flags remain false.

Next, review the complete original E375 clause/field conjunction using the actual
B-1 standard and source-specific evidence. The earlier three-sample circular-state
failure remains red at its unchanged bounds; its diagnostic explains quadratic
truncation and does not make it a pass. E404's mandatory-error versus order-fallback
conflict and E381's incomplete frame wording remain visible. The approved ShortArc
metadata correction resolves neither separate obligation.
