Plot sampling across GMAT function calls
=======================================

Four new cases pass unattended and visibly. The bundled GMAT Function guide
describes declaring a shared spacecraft and its subscribers global in both the
caller and function for continuous data. The fixtures exercise that path and
also pass a spacecraft as a local formal argument. Both plot controls are set
through the native editors. Generated GMF bodies, caller scripts, raw reports,
published runtime identity traces and both plot histories are retained.

* FunctionShadow passes the caller's second spacecraft as a formal named One.
  The main spacecraft named One has runtime ID 1. The function publishes its
  local One with ID 2 at 0, 10, 20 and 27 seconds. Both main viewers exclude
  those points and retain their caller's 0, 20 and 37 second trajectory. The
  returned time, position and velocity match the second spacecraft's orbit.
* FunctionGlobalTwice propagates the same global spacecraft through two calls.
  Both viewers retain 0, 20, 37, 57 and 74 seconds and the mission finishes at
  77 seconds. All four Propagate command intervals publish the same object ID.
* FunctionNested traces the same object from the caller to OuterArc to StepArc
  and back. Both viewers select 0, 20, 40 and 57 seconds. The caller finishes
  at 60 seconds.
* FunctionLocalPlot creates a local OrbitView and GroundTrack inside StepArc.
  Both register only during the call and collect the formal spacecraft's 0 and
  20 second points. The observer sees neither subscription after return. The
  caller plots retain their separate 0, 20 and 37 second trajectory. The local
  plot evidence covers publication, retained points and unsubscription. The
  local GroundTrack coordinates are checked for finiteness.

Two original failures isolated a lifetime bug. With a local formal named One,
OrbitView retained only the initial caller point. With a shared global One, the
first function entry restarted sampling cadence. The diagnostic trace showed
distinct runtime spacecraft IDs in the shadowing case and confirmed that the
caller continued publishing its original object. ObjectInitializer was rebuilding
references and initializing an already active global plot against function
locals. It could rebind the caller's spacecraft and reset the sampling cursor.

Function initialization now preserves an initialized global-pool OrbitPlot or
GroundTrack **in explicit step mode** when it is the very subscriber already
registered with the publisher. Local and newly created subscribers take the
usual initialization path. The exact ObjectInitializer.cpp preimage, patch and
hashes are in the stage evidence root. The new diagnostic's arm Report also
added a one-column marker. The first report checker rejected it before and
after the core correction. Source-v4 requires the marker and checks every
following row. All four failed invocations remain recorded.

Evidence and replay
-------------------

The evidence root is ``build/nasa-coverage/coverage95-20261102-plot-step-function``.
Its 306-artifact manifest binds the preceding boundary stage and the canonical
ledger. The first three cases pass at source-v4 and the local plot at source-v5.
The unchanged GlobalScopes regression also passed visibly. The independent
audit checks 88 provider packets, 126 function/caller report values, 60 caller
plot epochs and eight local plot epochs. Visible receipts recorded 23 native
X11 exposure events. Six altered proof copies were rejected. Lossless compression
saved 88,286,642 bytes. Use a fresh output directory for replay.

.. code-block:: bash

   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --cases FunctionShadow,FunctionGlobalTwice,FunctionNested,FunctionLocalPlot --visible --pause-ms 150 --output build/gui-acceptance/plot-step-function-local-replay
   python3 src/qtgui/tests/AuditPlotStepFunctionEvidence.py --manifest build/nasa-coverage/coverage95-20261102-plot-step-function/evidence-manifest.json --output /tmp/plot-step-function-audit.json

FRR!E330/E634 gain bounded evidence for function-local shadowing, shared globals,
repeated and nested calls and a local subscriber lifetime. FRR!E329/E633 still
inherit those open children and remaining solver redraw/retention clauses.
General coeval frames, attitude and acceleration, additional provider and
function combinations, and solver rendering remain open. The original boundary
gate passed at its recorded sources. Coverage is **1,009/1,158 (87.1%)**, 149
open and **92 more needed**. Historical operations remain 1,816/2,367. Zero
whole rows were admitted, and all 59 prerequisite holds continue.
