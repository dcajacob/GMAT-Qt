Plot sampling through Toggle and repeated Run
=============================================

Three new cases passed unattended and visibly with both plot controls set to
skip one step. Each case executed the same saved script twice in the same GUI
without rebuilding or reloading the mission between runs. The two executions
produced identical retained coordinates, epochs, segment connections, viewer
completion flags and final mission reports.

* LifetimeToggleGap collected 0 and 20 seconds, disabled both viewers until
  30 seconds, then collected 30 and 50 seconds. Neither viewer connected the
  line across the disabled interval. Propagation finished at 57 seconds.
* LifetimeInitiallyOff disabled both viewers before propagation started and
  enabled them at 10 seconds. Both collected 10 and 30 seconds. The final
  spacecraft report reached 37 seconds.
* LifetimeFinishOff collected 0 and 20 seconds, then disabled both viewers.
  Both finished with those two points retained. GroundTrack's final active
  flag was false. The spacecraft continued to 30 seconds.

The native oracle checked both plots against literal epochs and connectivity,
and all seven reported time and Cartesian values against a circular solution.
The offline audit checked 84 report values, 96 OrbitView position components,
and 64 pairs of sample epoch and connection flag. It recorded two native
exposure events on the visible nested desktop. The check for the final active
flag covers GroundTrack. GroundTrack coordinates were checked for finiteness.
The fixtures configure Toggle in the script and set sampling through the native
plot forms. Earlier Toggle editor evidence retains its own source versions.

This stage changed the acceptance fixture and runner only. FRR!E330/E634 gained
bounded evidence for the three activation transitions and repeated Run.
FRR!E329/E633 still inherit their open children. Same-epoch state assignments,
function and provider lifetimes, general sampled frames, other ephemeris
providers, solver redraw and rendering, and the original enabled legacy boundary
gate remain open. The full goal, 59 holds and qualification gates continue.
Coverage is **1,009/1,158 (87.1%)**, with 149 open and **92 more needed**.
Historical operations have a separate total of 1,816/2,367. Zero rows were added.

Evidence and replay
-------------------

``build/nasa-coverage/coverage95-20261102-plot-step-lifetime`` holds the stage
receipt, 101-artifact manifest, source-v1 archives, two invocation receipts and
the offline audit. It binds the preceding SPK stage. Three altered proof copies
were rejected for a line across the disabled interval, an extra hidden sample
and a wrong final state. Lossless compression saved 19,628,018 bytes. Mutable
launcher links are inventoried separately. Use a fresh output directory for
replay, or omit ``--visible`` for the unattended path.

.. code-block:: bash

   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --cases LifetimeToggleGap,LifetimeInitiallyOff,LifetimeFinishOff --visible --pause-ms 150 --output build/gui-acceptance/plot-step-lifetime-local-replay
   python3 src/qtgui/tests/AuditPlotStepLifetimeEvidence.py --manifest build/nasa-coverage/coverage95-20261102-plot-step-lifetime/evidence-manifest.json --output /tmp/plot-step-lifetime-audit.json
