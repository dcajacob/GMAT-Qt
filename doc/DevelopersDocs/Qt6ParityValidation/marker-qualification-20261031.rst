Bounded 2D marker qualification
================================

This increment adds 28 reusable Linux GUI acceptance registrations covering
all ten current Qt 2D marker identities across a finite rendering and control
matrix. It fixes a real plot-frame layering defect. Existing test assertions
and numerical-core sources are preserved. This is additional input-domain
coverage, not completion of NASA GUI qualification or exact wx raster parity.

The evidence root is ``build/release-check/20261031-marker-qualification/``.
``completion-audit.json`` records the final verified outcomes and counts;
``effective-outcomes.json`` retains each reused result's own source version.
``marker-matrix-audit.json`` records measured raster and clipping limitations.
Large marker-proof and raw trace receipts are losslessly compressed as ``.gz``
after execution; the audit reads these directly. No evidence is discarded.
``increment.patch`` is relative to the preserved, already-dirty starting files,
not a diff that absorbs earlier work into this increment.

Domain and independent oracle
------------------------------

The permanent ``MarkerRasterCases.json`` lists the ten NASA enum identities:
Cross, Circle, Plus, Star, Square, Diamond, Crossed square, Triangle up,
Triangle down and Circled plus. The matrix uses sizes 8 and 16 logical pixels;
scales 0.5, 0.75, 1, 1.5, 2, 3 and 4; four physical-pixel phases on each axis;
normal and highlighted glyphs; and both XY and ground-track contexts.
Ground-track uses an opaque white synthetic map and retained synthetic samples.

* Fourteen raster cases exercise 17,920 marker observations. Curve-default and
  per-point marker override policies are both checked. Each raster group batches
  sixteen spatially separated phase combinations into one captured frame.
* Seven capture cases check 107,520 edge/corner/outside observations, 1,120
  visibility/replay/highlight observations and 560 pixel-exact physical-resolution
  PNG exports, including a Unicode destination. Exported files are temporary.
* Seven actual-control cases open the Style action and marker popup, enter sizes
  through the spin editor, and toggle Markers through keyboard input. They check
  140 accepted edits and 280 Cancel/reopen paths, retained values and samples,
  and preservation of another curve. They use QtTest events, not an external
  OS-level mouse/keyboard driver. Existing export-picker coverage is reused;
  the new export matrix calls the actual export API directly.

``MarkerRasterOracle.hpp`` computes scalar segment/circle geometry independently
of QPainter and the plot transforms. It checks visible red stroke pixels against
that geometry and the continuous plot clip, and checks sampled defining
centerline features against observed pixels. Sampling is at most 0.75 physical
pixels apart. With stroke width s=max(1, scale), the geometry distance bound is
(s+1)/sqrt(2)+0.25 physical pixels; the witness bound is s/2+1.25. Original XY
full-color, placement, clipping and capture assertions remain byte-identical.

The visible-stroke mask requires red >180, red-green and red-blue >64, and
abs(green-blue)<=12. It deliberately excludes faint antialias fringes; original
full-color pixel counts are recorded separately. A direct Qt line diagnostic
showed that a faint fringe beyond the ideal stroke cap was native antialias
behavior, not the plot-frame defect. This mask is a finite red-on-white/gray
fixture oracle, not a universal perceptual or arbitrary-color test.

Fractional clipping may discard an entire boundary pixel. Positive defining-
feature witnesses are therefore restricted to the inward-rounded whole-pixel
clip; the number excluded is recorded. A zero-local-length corner contact has
no guaranteed visible feature and is also counted separately. A partially
clipped glyph may have no eligible defining-feature witnesses: it must still
have visible ink, and every detected ink pixel still undergoes geometry and
clip checks. No witness coordinates are shifted, and no distance bound is
expanded for these cases. Fully interior glyphs require positive witnesses.
Partially clipped shape identity at low resolution remains explicitly limited.
The final visible receipts count 28,376 fractional-clip witness exclusions and
48 isolated contacts. Six partial glyphs at 0.75x have no eligible positive
feature witness; they pass only the visible-ink, geometry and leakage checks.
Every fully interior matrix glyph has positive feature witnesses.

Product repair and fixture development
---------------------------------------

PlotCanvas previously painted its frame after curves and markers, erasing valid
boundary data. The shipped native TsPlotCanvas::DrawPlot paints axes/grid before
PlotData. The Qt 2D frame now follows that order, drawing the complete border
without the data clip and then restoring the existing data clip. Orbit rendering
is unchanged. No sample, solver, equation, propagation or numerical tolerance is
modified.

``frame-order-isolation.json`` applies the same final stroke mask and unchanged
1.75-pixel witness bound to retained before/after diagnostic images. The boundary
witness's nearest visible ink moves from 2.1213 pixels (fail) to 0.7071 pixels
(pass). That integer-aligned witness remains eligible under the final oracle.
The fix is therefore independently distinguishable from new-fixture corrections.

New-test development also corrected an empty-label checkbox hit target, the
antialias fringe/color mask, isolated clipping contacts and fractional clip
eligibility. Earlier failures remain in diagnostic logs. These are corrections
to newly introduced tests, not waivers or modifications of the existing suite.
Source versions are retained; eight earlier passing unattended cases are reused
with their original source identities while the twenty remaining cases use the
final oracle. The visible matrix runs all 28 against the final sources/binary.

Verification and reproduction
-------------------------------

All 28 new unattended cases pass. All 35 visible cases pass: the full new
matrix, five affected XY axis/scaling cases, Plots and PlotsHiDPI. All 41
traceability support tests pass. The audit confirms that 455 original test
source files, 1,706 core source files and seven checked production shared
libraries are unchanged, and verifies 1,606 current source/binary input hashes. Windows use the existing resizable nested desktop, start
minimized, and retain no passing screenshots. This increment makes no binary or
runtime-data snapshot. Diagnostic failure images and machine-readable receipts
are retained. The prior sealed solver handoff is verified read-only.

Build and run only the new unattended contracts::

   cmake --build build/linux-gui --target GmatQtMarkerRasterTests -j4
   python3 src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
      --max-screenshots 0 --filter '^QtGui\.Marker(RasterXY|RasterGround|Controls|Capture)(Half|ThreeQuarter|One|OneHalf|Two|Three|Four)$' \
      --output build/gui-acceptance/NEW-MARKER-AUTOMATED

Watch the complete affected scope in the resizable desktop, starting minimized::

   cmake --build build/linux-gui --target GmatQtMarkerRasterTests GmatQtXYAxisTests GmatQtPlotTests -j4
   python3 src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
      --visible --start-minimized --scale 1 --width 4000 --height 2500 \
      --pause-ms 150 --max-checkpoints 24 --max-screenshots 0 \
      --filter '^QtGui\.(Marker(RasterXY|RasterGround|Controls|Capture)(Half|ThreeQuarter|One|OneHalf|Two|Three|Four)|XYAxes(SubunitDPI|FractionalDPI|HiDPI|HighScale)?|Plots(HiDPI)?)$' \
      --output build/gui-acceptance/NEW-MARKER-VISIBLE

The explicit pixel dimensions fit this verified host's scaled work area and the
4x fixture. On another host use a sufficiently large resizable desktop or select
its fitting scale profiles; do not interpret host clipping as a qualified pass.
The desktop is a normal window, not fullscreen. Restore it from the taskbar to
watch, and adjust pacing/checkpoint limits for a slower demonstration.

Remaining coverage and review boundaries
-----------------------------------------

NASA row-level estimates retain the prior source-versioned 742/1,158 requirement
scopes (64.1%) and 1,803/2,367 historical operation scopes (76.2%). This increment
does not claim additional formal rows: no matching private historical marker
keyword procedures or current authoritative requirements corpus was established.
There is no defensible total widget/input-domain coverage percentage.

The shipped wx renderer uses radius-sized markers, an expanded thick-X highlight
and an open-sided crossed-square; current Qt uses diameter-sized markers, a ring
highlight and a closed crossed-square. The identities are source-linked, but
these three semantic differences remain explicit compatibility gaps. They were
not silently converted into new NASA acceptance requirements.

Other marker sizes in the 1..30 control domain, arbitrary scales and subpixel
phases, colors/background maps/themes/fonts, native OS input, other platforms,
GPU/compositor behavior, partially clipped glyph identity and current NASA
qualification remain outside this finite matrix. The twelve original positive
solver-nesting source conflicts remain disabled/unexecuted, and the existing
97-file core-change inventory still requires independent scientific review.
This GUI-only increment does not approve that prior core work.

The 1,798 current CTest registrations have source-versioned effective outcomes
of 1,786 passed and twelve skipped source conflicts, with zero active failures.
Effective suite accounting composes prior passing evidence with the new and
affected receipts. It is not one uninterrupted full-suite replay on the final
binary. Consult the audit for counts, source hashes, native exposure checks,
excluded clipping witnesses, reused versions and remaining skips.
