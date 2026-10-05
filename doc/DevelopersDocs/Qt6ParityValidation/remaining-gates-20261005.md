# Remaining Linux Qt qualification gates — 2026-10-05

This is a requirement-level review of the current acceptance map at commit
053e28d4, not a new workflow audit, numerical run or full regression pass.
All nine [original gates](../Qt6ReplacementQualification.md#acceptance-gates) remain unchecked.
The [current acceptance map](current-acceptance-20261002.md) remains authoritative; historical pending statements
superseded by later repairs are not a new implementation queue.

## Current artifact and evidence

Read-only inspection confirms codex/qt6-gui and the installed GmatQt target at
6,740,680 bytes, SHA256
78e763656adfc1956508a7a6527f623e864283423debe1747e65b8894a874a72.
Base, util and startup match the actual OF script-ordering receipt. The adjacent
JSON records those comparisons and the exact inspected source/evidence hashes.
GmatQt is a symlink; its executable target, rather than the 13-byte link, is
measured. Earlier test results retain their recorded runtime identities.
No application change, rebuild, new test or host operation occurred in this review.
The original /home/dan/GIT/GMAT/GMAT checkout was not modified.

## Nine required gates

| Gate | Evidence inspected and disposition | Remaining requirement |
| --- | --- | --- |
| wx workflows | Current map records all 108 source/caller audits, actual controls and bounded executable evidence; subsequent creation Help, Rename and specialized omissions have separate repair records. | No further ordinary missing operation established by this review. Broad shared desktop/input acceptance remains open; do not repeat the completed audits. |
| Array editing | Current map records resizing, expressions, validation, pending Cancel, rollback, Undo/Redo and calculation-preserving reopen. | Preserve this bounded evidence; no affected Array change or new failed operation was identified. This review does not independently rerun the Array gate. |
| Specialized resources/references | Current map records spacecraft, frames, force models, propagation, hardware, outputs, Region, CustomFOV, Receiver ErrorModels and Transmitter SolveFors controls. | Wider scientific/FOV regimes remain explicitly unqualified; the exact-boresight core diagnostic is not proof of a GUI defect. Host file-picker/input scope remains separate. |
| Mission/branch controls | Current map records typed/nested transactions, local functions, file-thrust cardinality and independent multi-history execution. | Synchronized thrust execution is unqualified. The recorded leading-propagator failure and candidate core step-boundary explanation are not a demonstrated frontend repair; changing numerical step/algorithm behavior is outside this continuation. |
| Plots/native/fallback lifecycle | Current map and current renderer retain implemented histories, two playback modes, Slower/Faster, cameras, labels, models, masks and close/reopen evidence. Private software desktops are bounded evidence. | User host GNOME/physical GPU, portal/focus/window acceptance and crash attribution remain unverified. Native model mesh geometry has no QPainter mesh equivalent; fallback is explicitly a sphere/marker approximation. LastN decorative-listener and compact-history bounds remain visible. |
| Required OF translation | Current source and latest receipt implement the listed static perspective/FOV, multiple/body/trajectory views and ordered drawing settings. Unsupported settings preserve source and diagnose limitations. | Device VR/MSAA remain separate. MSAA is deliberately disabled after recorded Intel resets. Dynamic/compound settings and unsupported camera/font placements remain diagnosed limits; diagnostics are not implemented parity. Inactive legacy AxesLength is not evidence for inventing a new scale operation. |
| Selected plugins | Current map records representative configuration/execution/result/recovery for all 20 selected plugins, including promoted Python paths, FixedGrid, Region and custom-path force evidence. | This is representative GUI coverage, not unlimited scientific parameter/regime qualification. Synchronized thrust and the explicit external/proprietary dependencies remain unresolved at their recorded scopes. |
| Script/file transactions | Current map and script-ordering receipt record expressions, unknown settings, comments/references, atomic recovery, Save/Save As/reopen, Undo/Redo, includes/assets and configured FOV paths. | Retain diagnosed unsupported syntax and dependency limits. Exported Python startup requires a fresh process. No newly established ordinary transaction blocker or affected regression was identified here. |
| Actual application/relevant regressions | Installed executable/base/util/startup hashes match the latest receipt; its six relevant passing checks remain bound to recorded builds, including preserved first failures. | This is current artifact identity, not a fresh full matrix or full replacement acceptance. Rebuild and run relevant checks after the next actual application change. |

## Additional explicit user requirements

- All twelve original published Help walkthroughs are already independently
  constructed, run, reopened and compared after freezing authored scenarios.
  Six additional chapters are published, for eighteen installed chapters. No
  completed walkthrough was repeated.
- The seventh additional chapter, Lunar Transfer, remains HOLD. Its original
  and one perturbation-only variant miss ECC0 +/- 1e-5; about88.07 degrees
  Moon-inertial inclination does not prove an exact90-degree polar capture.
  Its full independent-construction disclosure includes incidental early ECC
  sample access. A validated, teachable corrected scenario is still required;
  accepting looser goals or a finite trajectory would not close it.
- L2 remains held: the valid opposite-sign energy bracket does not prevent an
  intermediate trial from missing the entry box and exhausting DE405. A bounded
  diagnostic is not an end-to-end lesson pass. Two legacy optimizer chapters
  are unresolved and two MATLAB chapters remain deferred.
- Shipped-example attempts are complete at their recorded 177-source /
  164-standalone / 311-child-stage scope. Historical December2019 TLE inputs,
  compatible legacy MarsGRAM integration/data and proprietary dependencies
  remain recorded inputs, not GUI operations supplied by this review. Stale/
  decayed current-time cases, numerical event failures and time bounds remain
  separate from successful execution. No unchanged attempt was repeated.

## What can change the next action

Full qualification cannot be proved while the host gate remains stopped.
Closing it needs an established safe host condition and resumed host testing;
private Xvfb/software rendering cannot substitute. The saved crash proves
Mutter sent an event through a null Wayland resource, but neither the GMAT
client owner nor a compositor repair was proved. No Mutter development,
system upgrade or speculative GMAT popup patch follows from that evidence.

Scientific holds need a demonstrably valid scenario preserving the declared
objective; external cases need compatible inputs/components. These are not
closed by another unchanged run or by changing acceptance targets. A newly
reproduced ordinary frontend omission can still justify an autonomous repair,
followed by an actual rebuild and only affected checks. This review establishes
no such omission and schedules no speculative implementation.

The dynamic OF importer restriction was examined as a possible lead. The
common OrbitView visibility callback inspected in Initialize configures drawing
flags there; its BooleanArray setter alone does not establish a live viewer
update. Converted drawing/clock settings also have static metadata semantics.
Dropping the guard without runtime semantics would therefore not prove the
operation. This is a bounded source observation, not a claim that all possible
dynamic fields are impossible or a new audit of the shipped corpus.

The goal remains incomplete and active after this first consolidated blocker
review. Windows, macOS and MATLAB remain deferred; all original acceptance
requirements and explicit limitations are retained.

## Evidence navigation

- [Actual OF script-ordering receipt](of-script-ordering-20261005.receipt.json)
- [Help walkthrough ledger](../Qt6TutorialWalkthrough.md)
- [Lunar held construction and results](help-lunar-transfer-20261003.md)
- [L2 event-domain diagnostic](l2-tutorial-bracket-2026-10-02.md)
- [Shipped-example attempts and missing dependencies](shipped-examples-20261002.md)
- [Independent thrust result and synchronized failure](multi-thrust-live-20261002.md)
- [Saved Mutter fault and attribution limits](compositor-popup-null-resource-20261002.md)
- [Deliberate zero-MSAA renderer](../../../src/qtgui/OrbitRenderer.cpp)
- [Static conversion guard](../../../src/qtgui/ScriptCompatibility.cpp)
- [Core OrbitView initialization](../../../src/base/subscriber/OrbitView.cpp)
