Current admission
=================

The fixed-offset component now has a complete original FRR!E52 evidence join.
The adjacent ``coverage95-20261104-attitude-fixed-offset-row.rst`` documents the
1053/1158 source-versioned admission, immutable evidence, limits and replay.
The earlier partial checkpoint below is retained as history.

Optional fixed offset: partial implementation and evidence
========================================================

The current worktree adds an optional scalar-last ``OffsetQuaternion`` property
to ``CoordinateSystemFixed``. Its default is identity, preserving the original
aligned model and default script. The original initial-attitude and initial-rate
controls remain inactive. The fixed reference-to-body rotation premultiplies the
reference frame DCM and transforms its body angular velocity consistently.

This is progress toward original FRR!E52, fingerprint
``3041f4f0af0564670cc6227dfdb1a5d7d45c70b07832be99b9cf6e1f12143333``.
It is not a whole-row admission. Coverage remains 1052/1158, with 106 open rows
and 49 additional closures needed for 1101. All 59 prerequisite hold identities
and separate historical-operation coverage 1816/2367 remain unchanged.

Retained evidence
-----------------

The stage is ``build/nasa-coverage/coverage95-20261104-attitude-fixed-offset``.
``component-progress-review-v1.json`` aggregates 28 source3 GUI passes: seven
nonzero-offset witnesses and seven original aligned controls in each unattended
and visible profile. The visible display receipt records an initially iconic,
resizable window. Every test retains four epochs, all nine DCM components and
three body-rate components, plus the existing orbital witness. Original matrix
and rate bounds remain 2e-10 and 5e-9 degrees/second. Original driver and catalog
hashes are recorded in ``row-scope-review-v1.json`` and remain unchanged.

The new fixture uses an independent passive Rodrigues rotation of 36 degrees
about normalized [1,2,3]. Source3 archive SHA256 is
``7a40ae08893b82efb5806b0ae39be7197e6a6ece2012b489dd7270c1a930ea7b``.
Source/binary guards and actual process mappings are retained per run. This
bounded frame oracle does not establish every original FRR-35 frame/origin.

The isolated backend registration
``QtAcceptance.AttitudeFixedOffsetNativeSupport`` passed 163 assertions for
metadata, normalization, finite/size/index guards, rejected-input atomicity,
clone/copy/assignment and native serialization. ``native-support-v1`` binds its
backend source closure, executable and two loaded core libraries to the parent
source4 build. ``execution-v2.json`` records one actual host process and three
mapped project images; no GUI context or runtime binary copy is claimed.

The first backend launch failed to locate libGmatBase. ``execution-v1.json`` and
``native-v1.log`` retain that loader failure; no assertions ran. The second
launcher supplies the existing application/bin library path. The integrated
GUI-build support registration was not executed: source4 preflight refused after
concurrent optional MCP bridge edits. Those changes are preserved and the earlier
source3 results are not relabeled as current GUI-build qualification.

Other retained failures are the missing-property baseline, source2 compilation
failure, and source4 execution preflight refusal. Their receipts remain separate.

Core and scope limits
---------------------

``CSFixed.cpp/.hpp`` add one Rvector member and appended parameter ID while
overriding existing virtual slots. Actual Linux GCC layout measurement is
1984 to 2008 bytes, with 278 vtable entries before and after. This is an ABI
layout change requiring technical review; external ABI/platform approval is
false. Exact preimages, source versions and the 286-path core inventory remain
in the stage. Inventory v2 corrects the obsolete CSFixed formula/layout
descriptions while all 286 source hashes remain identical to inventory v1.
``incremental-core-v1.patch`` retains the exact two-file increment from its
preimages. No change from this component is pushed to the frozen private
preview branch.

Original FRR-35 requires five origin classes and sixteen axis families: 80
combinations before the original explicit body-only and station-only
restrictions. Seven initial witnesses do not close that conjunction. Remaining
work includes every source-permitted origin/axis combination, identity and
sign-equivalent/normalized/half-turn rotations, actual GUI invalid-input
atomicity, repeated Run/lifetime guards, and a complete independent scope reader.
The user guide now describes the optional quaternion, normalization and unchanged
inactive original fields; its XML parses. The new fixture's guide metadata was
updated after source3 execution, without relabeling those retained passes.
Wider scientific,
technical, full-current-build and NASA qualification gates remain false.

``full-frame-origin-scope-v1.json`` enumerates all 80 original combinations
and retains the exact authority for its 12 explicit exclusions. The other 68
cells remain mandatory and unverified. Native axis names were checked against
AxisSystemFactory; implementation support does not itself qualify a cell.

Replay and further execution
----------------------------

Read the source-versioned build/run receipts and
``component-progress-review-v1.json`` without rebuilding or relabeling results.
The canonical prior prefix replay remains ``prior-prefix-replay-v1.json``;
the sealed E380 row stage remains the admission authority.

``verify-composition-v1.py --output <fresh-output.json>`` replays the seven
offset/aligned pairs in both profiles, checks all 3457 source3 archive members
against the executed source map, and verifies actual build/mapping receipts.
It derives the Rodrigues offset without a GMAT helper and checks both expected
and actual orientation/rate composition for 56 paired epochs. The retained
``composition-replay-v1.json`` passes and rejects damaged orientation, rate and
missing-epoch controls. This bounded replay reruns no native tests and grants
no FRR-35 completeness, broader physical-oracle or whole-row credit.

For a new backend execution after checking the recorded source/library hashes,
use the standalone CTest registration with ``LD_LIBRARY_PATH`` pointing at the
existing application/bin. Give any new evidence a fresh versioned output path;
the existing run-v1/v2 launchers intentionally refuse to overwrite executions.
For further GUI execution, capture a new complete source capsule and build it
before running only the new/affected registrations. Preserve concurrent MCP
work and retain any source-preflight refusal.
