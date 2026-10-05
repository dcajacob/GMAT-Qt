# Propagation segment labels — 2026-10-04

The legacy OF configuration panel has an active **Label Segments** operation,
`DrawUsePropLabel`, which Qt previously retained only as a comment. OrbitView /
Advanced / Object drawing / **Label style / Segment labels** now offers
Default, On and Off for space points. Default is Off. Object-name labels,
trajectory lines and models remain independent; Show labels and the existing
font size/placement apply to both kinds of label.

Conversion preserves boolean prefixes, omitted false defaults and source-ordered
Add resets. Settings survive pending edits, Apply, Undo/Redo, Unicode Save/reopen,
Rename and object removal. A spacecraft labels each accepted regular propagation
arc; solver trials have no regular labels. Bodies retain the first provider's
label over their merged trajectory, matching the inspected legacy backend.
A body fixed at the body-frame origin has no separate moving segment label.

Each label follows its segment at the display epoch and clamps to its endpoint
outside that segment's time range. Recorded step and interpolated replay use the
same label helper in native and fallback renderers. Merged arc identities retain
the association of collected points. Compact endpoints survive trimming;
interior geometry uses the retained collected samples. After decimation or
trimming, interpolation is a display approximation, not a reconstruction of the
legacy backend's unlimited raw trajectory history. No numerical engine files
or numerical states are changed.

## Focused verification

| Check | Result |
| --- | --- |
| NativeSegmentLabels | 0.71 s, private Xvfb / software OpenGL |
| SegmentLabels | 0.40 s, final offscreen control/engine check |
| ObjectGuides | 0.35 s |
| ConvertedFonts | 0.81 s |
| SegmentArc | 0.60 s |
| SegmentCameras | 0.52 s |
| FontRendering | 0.19 s, corrected standalone initialization guard |
| ObjectDrawingLayout | 0.18 s, corrected standalone initialization guard |

Synthetic checks exercise separate segment-only text pixels, replay epoch
following, backward arc lookup, old/future endpoint clamping, merged identities,
trial exclusion, zero font size, master/curve visibility, immutable retained
samples and trimming. [Native capture](segment-native-labels-20261004.png) and
[fallback capture](segment-fallback-labels-20261004.png) preserve rendered output.

A new short two-segment fixture runs with and without the display. Both complete
210-byte numerical reports are identical, SHA256
`614fffa0db7e1c87566a5e4b743217ff3c4ba02f32845b2b72552f6416be87f8`.
The labels-only callback captures both named spacecraft arcs and one merged
Earth arc. Arc endpoints match the independent report. Actual resource controls
verify Cancel, pending edits, Apply, exact source Undo/Redo, Unicode Save/reopen,
Rename, pruning and retained labels after engine objects are discarded.
No completed example, tutorial or corpus audit was repeated.

The initial new test compile needed its QDialog include. The first eight-check
run passed six; the two standalone drawing/font checks crashed because the
dialog queried ConfigManager before Moderator initialization. A bounded debugger
backtrace identified GetConfiguredObject from OrbitObjectDrawingDialog. Both
lookups now require IsInitialized. Only the failed checks and the directly
affected new dialog check were retried; all pass. Original failures and build
corrections remain in [build logs](segment-labels-build-20261004.txt) and
[check logs](segment-labels-checks-20261004.txt). Committed copies trim trailing
whitespace; exact raw logs, scripts, reports and captures are under
`build/linux-gui/segment-labels-20261004/raw/`.

## Runtime and limits

GmatQt is rebuilt at **6,230,920 bytes**, SHA256
`f2a52efae004c20287fae85cca18bfb9f35e1934f4c0ef2cd2c573909ef84aba`.
The [receipt](segment-labels-receipt-20261004.json) binds app, base, util, Qt startup
and both reports. Base/util/Qt startup remain byte-identical. All edits stay in
`/home/dan/GIT/GMAT-Qt`; the original checkout is untouched.

These focused checks do not close all nine acceptance gates. Live host testing
remains stopped; Mutter's null Wayland resource fault is established, but GMAT
client attribution, host repair and physical-GPU acceptance remain unproven.
Held scientific cases and external dependencies retain their dispositions;
Windows, macOS and MATLAB remain deferred. Full qualification is not claimed.
