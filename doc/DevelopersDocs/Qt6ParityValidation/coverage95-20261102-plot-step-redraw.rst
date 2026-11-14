Rendered segment limits in both native plots
===========================================

Three new regular-propagation cases pass unattended and visibly. They set the
sampling interval and NumPointsToRedraw through each plot's native editor. A
circular Earth trajectory advances on ten-second integration steps. Skipping
nine steps selects the initial state and each hundred-second point through
1,000 seconds. The test captures the actual OrbitView and GroundTrack canvases
at a live Report breakpoint, then again after mission completion.

* RedrawLastTwo draws two live segments and all ten at completion.
* RedrawAll sets the limit to zero and draws all ten in both phases.
* RedrawRetained also sets MaxPlotPoints to four. It draws two live segments
  and all three retained segments at completion.

The reference canvas uses a copy of the live model with redraw limiting off.
It keeps all vertices and camera settings, then enables line connections for a
literal number of final segments. Its segment count is independent of the
production firstVisibleFrame calculation. The reference and actual canvas use
the same renderer, which keeps the comparison focused on segment selection. At
each phase, the exact requested line list matched every production pixel.
Shorter and longer alternatives, when applicable, differed by at least five
pixels. For the visible RedrawLastTwo live case, showing three segments instead
of two changed 72 OrbitView pixels and 138 GroundTrack pixels.

The fixture compares pixels in memory and retains only framebuffer dimensions,
SHA-256 hashes, and difference counts. The offline audit checks those records
against literal segment lists and independently verifies retained epoch grids
and OrbitView positions. It checks 24 matching frames, 60 distinct alternative
frames, and 104 plotted epochs. The visible run recorded 11 native X11 exposure
events in the initially iconic, resizable, non-fullscreen nested desktop.
Three altered copies of the records were rejected for a wrong live segment
limit, changed framebuffer hash and an indistinguishable shorter line list.

Evidence and replay
-------------------

``build/nasa-coverage/coverage95-20261102-plot-step-redraw`` holds the
96-artifact manifest, source-v1 archive, stage receipt and audit. It binds the
preceding same-epoch assignment evidence. Lossless compression saved
19,639,634 bytes. Use a fresh replay directory.

.. code-block:: bash

   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --cases RedrawLastTwo,RedrawAll,RedrawRetained --visible --pause-ms 150 --output build/gui-acceptance/plot-step-redraw-local-replay
   python3 src/qtgui/tests/AuditPlotStepRedrawEvidence.py --manifest build/nasa-coverage/coverage95-20261102-plot-step-redraw/evidence-manifest.json --output /tmp/plot-step-redraw-audit.json

FRR!E329/E633 gained regular-trajectory pixel evidence for live and completed
redraw with bounded retention. Their sampling children E330/E634, the original
legacy boundary test, and solver redraw/rendering conjunctions remain open.
General sampled frames, function/provider lifetime and the 59 prerequisite
holds also remain open. Coverage is **1,009/1,158 (87.1%)**, 149 open and **92
more needed**. Historical coverage is 1,816/2,367. Zero whole rows were added.
