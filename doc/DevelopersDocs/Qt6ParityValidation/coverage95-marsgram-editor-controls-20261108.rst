Mars-GRAM editor controls, 2026-11-08
====================================

The original FRR!E120-E125 group names Mars-GRAM, its Low/Mean/High density
choices and its input file. Current native DragForce exposes DensityModel and
InputFile, but the dedicated Qt atmosphere editor had no density selector and
enabled/serialized InputFile only for Exponential. This was a concrete GUI
implementation gap independent of the unavailable Mars-GRAM provider.

The editor now restores and enables the MarsGRAM2005 input-file draft and a
Mean/High/Low density selector. Acceptance includes these fields only when
applicable; other models retain their existing settings. The existing available-
factory check remains mandatory. A readable explicit Mars input path and a
documented density choice are validated before candidate application; no namelist
parser or substitute density calculation was added. Help now distinguishes the
Qt controls from the legacy GUI's script-only fields. Script examples are unchanged.

Provider capability evidence
------------------------------

Stage: ``build/nasa-coverage/coverage95-20261108-dynamics-provider-current-review``.
The actual display-free GmatConsole was run with isolated copies of the configured
startup. A one-second Exponential Mars control completes and reports literal 41.
The same MarsGRAM2005 case fails model creation and produces no report. The first
new control fixture omitted the required primary gravity body; its failure is
retained. Corrected fixtures use primary Mars with degree/order zero and canonical
Drag.AtmosphereModel syntax. No original fixture changed. These capability checks
establish provider availability, not density or trajectory accuracy.

Editor diagnostic and regressions
----------------------------------

New ``QtGui.MarsGramControls`` uses actual native ODEModel/DragForce metadata and
the real editor. It restores a pending Mars configuration, edits drafts, checks
Cancel/native isolation and verifies that the missing actual provider still
rejects application. It also accepts applicable metadata through the real
Exponential factory and checks inactive fields for Exponential and None. It does
not register a fake factory, supply an SDK, execute a Mars-GRAM model or calculate
density. Its temporary readable text file is an editor-path fixture only.

The pre-repair worker completes eleven checks with five failures: three Mars
field checks and two checks for the new density control being present but inactive
for other models. The exact unchanged witness passes all eleven after repair.
It and unchanged ``QtGui.Atmosphere`` pass headless and visible modes: four final
registered executions. The original regression retains its existing Earth weather,
file validation, pending edits, source history and native-run comparisons.

No positive real Mars-GRAM application, Low/Mean/High density result, namelist
parsing or scientific trajectory was verified. Those require the real compatible
plugin and data. The new widget-level checks do not stand in for them, primary
resource-editor persistence, arbitrary provider compatibility or native OS input.

Validation and campaign disposition
------------------------------------

Only AtmosphereDialog.cpp and the force-model help changed for product behavior.
CMake registers the new worker and acceptance-target dependencies. Native density
formulas, model identifiers, SDK interfaces and existing tests are unchanged.
Help XML parses, script examples match, source/binary guards match executed
versions, and 2,358 source versions are archived and verified. All 1,334 protected
original inputs/assertions/tolerances remain intact. Initial capability and GUI
failures, before-source copies and exact patches are retained.

The GUI controls implementation is complete at this bounded scope. All six
original inherited/provider requirements remain open. The full dynamics/drag
parent conjunction and implementation audit also remain incomplete. No original
fingerprint, denominator, hold status, scientific/NASA/platform/ABI/current-build
gate or requirement credit changed: 1,078/1,158 (93.1%), 80 open, 23 more needed
for the unchanged 95% target. Installed runtimes and release packages were not
updated; work continues in the private campaign build.
