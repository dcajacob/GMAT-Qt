# Separate Default camera reset — 2026-10-02

The Qt plot already had **Script view**, which restores the selected initial
Current pose. The remaining active legacy operation was **Reset view / Space**
to a separately authored Default pose. Translation selected Current correctly
but discarded the separate Default destination. **Fit** also retained user
rotation and did not implement that operation. The unchanged b704092b observation
is preserved in [baseline readback](camera-reset-20261002.baseline.txt), with full
compile/link argv and runtime hashes under `build/example-qualification/20261002/camera-reset`.
It exits zero after confirming the observed gap; it is not a failing regression.

## Implementation and focused checks

Conversion now retains validated separate primary/named reset metadata, including
stored poses and existing automatic body/trajectory/regular-arc modes. Rendering
uses the existing camera resolver and its recorded callbacks for Default history.
Reset view and Space scoped to the focused canvas select Default; Script view
selects Current. Reopening a retained output keeps the selected reset state.
Rename preserves both destinations; an explicit primary pose edit clears that
primary legacy default, with Undo restoring it. No numerical algorithm changes.

GmatQt rebuilt successfully. The first build's test-only nonexistent accessor
was corrected before running checks. The first new test then failed because its
provider fixture named the arc `owner` but requested `Scout.Arc`. Only that fixture
was corrected to `SetOrbitProvider:99 0 1 Arc`; its expected Default eye
`[31,-2,53]` stayed unchanged. The registered **QtGui.CameraReset passes in 0.23 s**.
The already passing directly affected **OpenFramesCameraAliases / SegmentArc /
SegmentCameras (0.04 / 0.63 / 0.54 s)** are reused. No production edit followed
those checks. [Corrected registered result](camera-reset-20261002.checks.txt)
and [receipt](camera-reset-20261002.receipt.json) retain all failures, corrections,
argv, source hashes and runtime identities.

The new check covers Default/Current and aliases, source/calculation/comment and
Unicode roundtrip, primary/named stored and automatic reset, recorded regular-arc
history/replay/trim/solver cleanup, action scope, projection retention, retained
reopening and Rename/Edit/Delete/Undo. Published values are synthetic; the check
does not propagate a mission or claim native rendering acceptance.

## One actual-input owned presentation case

[Independently authored source](camera-reset-20261002.owned.script) specifies
distinct Default/Current poses for Primary and EarthView. It was opened through
the automatic conversion offer on an authenticated private Xvfb/Openbox, xcb and
software GL. One F5 completed its 30-second point-mass presentation propagation
in **0.151 s**, producing the two-state [420-byte report](camera-reset-20261002.report.txt).
This report is a mutation guard; it does not independently qualify orbital physics.

Actual toolbar Reset restored Primary Default. Script view restored its different
Current. A settled mouse drag changed the scene and actual Space restored exactly
the same Default scene pixels. The named selector chose EarthView, whose Script
view and Space produced its distinct Current and Default. The whole report was
byte-identical before and after all camera actions. Save As produced the
[converted source](camera-reset-20261002.converted.script) with both reset eyes
retained and the entire mission calculation section byte-identical. This native
case saved the converted file; Unicode reopen is covered by the focused check.

[Readback](camera-reset-20261002.readback.json) binds exact canvas ROI comparisons
to the preserved screenshots and XTest input ledger. All 17 screenshots remain
in the raw session. The first rapid drag had no settled scene change; the later
drag with delivered intermediate events supplies the rotation proof. The first
Save click did not close the file chooser before input settled; the subsequent
settled Save succeeded. Raw attempts remain; neither observation is attributed
to a product defect. File Exit ended application / WM / Xvfb normally **0/0/0**.
The helper exits 1 because it detects the normal app exit 0 as the end of its
interactive session; its authoritative record says closed and all owned process
exits are zero. No processes or host display were retained.

Current app: **6,075,296 bytes / SHA8d791a3d…**. Base SHA2082a341…, util
SHAf9844517… and startup SHA5f80be1f… remain unchanged from the path repair.
Earlier FOV native evidence was captured at SHA02d7711e… and earlier Help/FOV form
evidence at SHA167efe0f…; those scopes are retained without repeating them.

## Limits

This closes the identified Default reset operation and its bounded private input
case. Live host testing remains stopped. The saved Mutter NULL Wayland event
resource fault is established; GMAT client attribution, a repaired/safe host and
physical GPU acceptance remain unproven. Other recorded scientific/external
limits, all nine gates and Windows/macOS/MATLAB deferrals remain in the current
acceptance map. No full-matrix rerun or full replacement qualification is claimed.
