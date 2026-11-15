Native restart continuity repair, 2026-11-08
==========================================

A rounded non-precision command restart at a moving origin was falsely treated
as a new plot arc. The original TranslatedOrigin case expected 0/20/37 seconds
and collected 0/10/30. Accepted precise endpoints and native Real-clock restart
states differed by about 2.5e-6 km solely from origin clock conversion.

Precise primary endpoint samples remain unchanged. Optional native restart
snapshots now support exact boundary continuity comparison. Real same-epoch
assignments still start new arcs; solver rollback restores the owned cursor.
Publisher validates optional restart object sets, epochs and finite values
before dispatch. No GmatBase/Solver/Optimizer/VF13ad layout or virtual ordering
changed. The optional IntegrationStep payload extends, requiring consumers to
rebuild before further current evidence is claimed.

Fifteen selected original internal plot cases pass headless and visible,
including TranslatedOrigin, all seven analytical burn cases, real state resets
and spacecraft-origin cases. Three original native step support cases also
pass in both profiles: eight registered executions, thirty internal plot
executions and six support executions. A native supplementary contract passes
24 checks including seven malformed-payload rejections and frozen-state,
assignment and rollback behavior. Original tests and tolerances are untouched.

The complete combined PlotIntegrationSteps parent remains open. Retained full
runs include intermittent segmentation faults; the current gdb run passes
95 internal cases then fails a message-history prefix assertion in
FrameFollowerForward. The product caps that buffer at 10,000 blocks; exact
history diagnosis and the crash backtrace remain outstanding. Focused passes
do not replace these full-run failures. An earlier primary-clock trial failed
an analytical burn check and is retained as a superseded trial.

The separate original CURRENT replay unit measured zero histories because it
creates logical public windows for cold native subscribers with private output
identities. An explicit-public-policy supplement passes all original replay
assertions for both Orbit/Ground. Private output isolation remains intact and
the original failure is retained.

Evidence: build/nasa-coverage/coverage95-20261108-plot-replay-current-review.
All 1,334 protected original inputs and canonical accepted prefix remain intact.
No new NASA requirement credit: 1,078/1,158 (93.1%); campaign and final coherent
full rebuild/regression remain open.
