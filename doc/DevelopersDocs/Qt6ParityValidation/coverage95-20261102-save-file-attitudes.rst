95 percent campaign: file-backed attitude persistence
=====================================================

Three new Save cases pass unattended and visibly: CCSDS-AEM quaternion,
CCSDS-AEM Euler-angle and SpiceAttitude with the existing MarsExpress CK/SCLK
inputs. All 128 preceding native state/source cases are reused at their recorded
versions. Combined evidence has **131 cases, 262 contexts and 7,208
writable-property comparisons**, with native owned-class and explicit input
literal checks before and after independent export/rebuild.

Coverage remains **998/1,158 requirements (86.2%)**, with 103 complete requirements
needed for 95%. Historical coverage remains **1,816/2,367 (76.7%)**. This increment
awards no complete NASA row. FRC!E132 still needs complete persistent state,
mode and reference-graph obligations. Authorities, denominators, fingerprints
and qualification gates remain unchanged.

File and native checks
-----------------------

The two AEM fixtures contain explicit, constant identity attitudes in native
quaternion and Euler input formats, over two UTC epochs bracketing the configured
epoch. Their exact bytes, paths and hashes are checked against the fixture asset
manifest. They are small native-reader fixtures, not full CCSDS standard or
scientific attitude-conformance evidence. Both complete a mission with the file
backed model initialized before Save.

The SPICE case reuses the supplied MarsExpress CK pointing and SCLK clock files,
with the sample's spacecraft/frame IDs and epoch. File identities are bound
before and after execution without copying those existing data files. The
optional frame-kernel list is empty; the native warning remains retained.
This case does not qualify an FK reference graph, SPK-as-attitude semantics,
kernel pointing accuracy or embedded dependency portability.

All cases check the actual Save editor and ordered object picker, empty-selection
rejection, exact Apply/source retention, mission continuation, the standalone
export's independent rebuild, all writable native fields and the owned model.
AEM file paths and both SPICE kernel lists have explicit native getter witnesses.
Reloaded exports are configured and compared; a second mission initialization
from those exports is not asserted in this increment.

Together with the preceding attitude cases, all eight current AttitudeFactory
model names are accounted for. CSFixed is recorded as the factory alias for
CoordinateSystemFixed. This name accounting is not every state/rate/sequence,
frame, input or hidden-state partition and earns no whole-row credit.

Failures, unchanged core and reused checks
-------------------------------------------

Two first unattended attempts failed because the new fixtures expected Filename
metadata for AttitudeFileName. The native source explicitly declares String;
the mission had completed before the descriptor assertion rejected that fixture
error. Only the two failed AEM cases were rerun after correcting the fixture's
expected descriptor. The passing SPICE case was reused unattended, and all three
first visible cases passed. There are eight attempts: two retained failures and
six selected passing contexts. No previous 128-case native replay occurred.

No additional core or Qt production source changes are made. All 21 preceding
Save core review paths remain unchanged. The shared native driver is unchanged;
all 47 balanced require expressions, including the helper declaration, remain
identical. No assertion, expected value, field scope or tolerance is removed.
The 53 shared checker contract passes are reused with exact unchanged checker
and test-source identities. Twelve corrupt evidence capsules are rejected after
a valid baseline, including failed old receipts, missing/changed kernel identity,
wrong native model/path/literal, omitted native captures and false credit.

Storage, audit and visible replay
----------------------------------

The visible cases use automatic scaling in initially minimized, resizable,
non-fullscreen nested windows. No passing screenshots, runtime binary copies or
existing kernel copies are retained. Lossless compaction of 31 duplicate reports
and traces saved 19,869,696 allocated bytes (18.9 MiB), preserving every admission
artifact. The expanded descriptors and model-name accounting are recorded in
``save-file-attitudes-descriptor-inventory.*`` and
``save-file-attitudes-model-inventory.json`` without modifying earlier seals.

Audit the retained conjunction without native replay::

   python src/qtgui/tests/AuditSaveFileAttitudeEvidence.py \
     --evidence build/nasa-coverage/coverage95-20261101/save-file-attitudes-evidence.json

To watch the three new cases, choose a fresh output directory and restore the
initially minimized test window::

   python src/qtgui/tests/RunAcceptance.py \
     --build-dir build/linux-gui --filter '^QtGui.NasaSaveFileAttitude' \
     --visible --display-route nested --display-scale auto --start-minimized \
     --pause-ms 350 --max-checkpoints 200 --max-screenshots 0 --report-every 3 \
     --output build/gui-acceptance/save-file-attitudes-watch

Omit visible/display flags for unattended operation. Frozen receipts qualify
only their recorded source/runtime versions, not the complete latest build.

Remaining scope
-----------------

Active local/named frame and reference graphs, spacecraft representations and
other file inputs, remaining state/rate/sequence partitions and hidden persistent
fields still require review. Optional FK input and post-rebuild initialization
are explicit gaps. Computed read-only getter, parent force-order and earlier
retired-editor-clean-observation limitations remain inherited and unqualified.
No new attitude dynamics, kernel pointing or complete format claim is made.

All 59 original prerequisite holds remain unresolved. Even if every other row
closed, that scope reaches only 1,099/1,158 (94.9%); at least two held obligations
need substantive resolution for the 1,101 target. Source conflicts may lower
achievable coverage further. No unanswered decision is assumed. Private NASA,
historical platforms, scientific, technical core approval and complete
current-build gates remain open; the 95% goal is not complete.
