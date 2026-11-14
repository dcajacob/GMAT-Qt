95 percent campaign: owned attitude configuration persistence
============================================================

Nine new Save cases pass unattended and visibly: four Spinner representations
(Euler321, quaternion, direction cosine matrix and modified Rodrigues
parameters), ThreeAxisKinematic quaternion, CoordinateSystemFixed in
EarthMJ2000Ec, PrecessingSpinner, NadirPointing and CommandableNadirPointing.
All 119 preceding native state/source cases are reused at their recorded
versions. The combined evidence has **128 cases, 256 contexts and 6,832
writable-property comparisons**. Explicit owned-field input literals and native
attitude class identities are checked before export and after independent rebuild.

Coverage remains **998/1,158 requirements (86.2%)**, with 103 complete requirements
needed for 95%. Historical coverage remains **1,816/2,367 (76.7%)**. This increment
awards no complete requirement: FRC!E132 still needs complete persistent state,
mode and reference-graph obligations. Original authorities, denominators,
fingerprints and qualification gates remain unchanged.

Checks and retained failure
----------------------------

Existing native owned-object getters provide state truth. The checker requires
that the Spacecraft Attitude selector agree with its single owned native model.
Literal witnesses traverse unique native type paths. Representation inputs and
rates, precession/spin configuration and pointing vectors are compared with
explicit fixtures; every captured writable property is compared across Save and
rebuild. Generated script is never the independent state oracle.

One of nine first unattended attempts failed: CommandableNadirPointing had an
empty native type name. Its root selector still named the model, so the existing
owned-path assertion correctly rejected it. Source review also found that the
base attitude metadata marked its supported pointing inputs read-only. The
failed receipt and source versions remain retained. After two narrow core
repairs, only that failed unattended case was rerun. All eight other unattended
passes were reused; all nine first visible checks then passed with the repairs.
There are 19 attempts: one retained failure and 18 selected passing contexts.
No earlier 119-case native replay occurred.

Two core metadata repairs for scrutiny
---------------------------------------

* ``src/base/attitude/CommandableNadirPointing.cpp`` now passes the explicit
  CommandableNadirPointing type and instance name to Kinematic. Previously its
  single argument occupied the type slot, producing an empty type for this
  unnamed owned object.
* ``src/base/attitude/Attitude.cpp`` includes CommandableNadirPointing alongside
  NadirPointing in the existing writable reference-body, constraint and vector
  metadata branch. The commandable implementation already consumes those fields;
  they now participate in serialization and the native writable-state comparison.

Attitude equations, steering calculations, numeric tolerances, constants and
setters remain unchanged. These are configuration persistence checks and do not
constitute independent scientific attitude propagation qualification. Exact
before/after patches and hashes are retained in ``save-attitudes-core-diffs/``
and ``save-attitudes-final-audit.json``. The explicit Save core review list now
contains **21 paths**; the preceding 19 paths and Qt production sources stay
unchanged in this increment. Technical/scientific and complete current-build
approval remain false.

The shared native driver is unchanged. All 47 preceding balanced require
expressions, including the helper declaration, remain identical. No assertion,
expected value, field scope or tolerance was weakened. The 53 shared checker
contract passes are reused with exact unchanged checker/test source identities.
Ten corrupt attitude capsules are rejected after a valid baseline, including a
failed old receipt, wrong model, missing visible case, substituted owned path,
changed input literal, omitted native capture and false complete-row/goal credit.

Storage, audit and replay
--------------------------

Visible checks used automatic scaling in initially minimized, resizable,
non-fullscreen nested windows. No passing screenshots or runtime binary copies
are retained. Lossless compaction of 53 duplicate reports/traces saved
22,622,208 allocated bytes (21.6 MiB); all admission artifacts remain unchanged.
The expanded descriptor inventory is saved separately as
``save-attitudes-descriptor-inventory.*`` without modifying older sealed files.

Audit the retained conjunction without native replay::

   python src/qtgui/tests/AuditSaveAttitudeEvidence.py \
     --evidence build/nasa-coverage/coverage95-20261101/save-attitudes-evidence.json

To watch the nine new cases, choose a fresh output directory and restore the
initially minimized test window::

   python src/qtgui/tests/RunAcceptance.py \
     --build-dir build/linux-gui --filter '^QtGui.NasaSaveAttitude' \
     --visible --display-route nested --display-scale auto --start-minimized \
     --pause-ms 350 --max-checkpoints 200 --max-screenshots 0 --report-every 9 \
     --output build/gui-acceptance/save-attitudes-watch

Omit visible/display flags for unattended operation. Frozen receipts qualify
only their recorded source/runtime versions, not the entire latest build.

Remaining gaps
---------------

Remaining attitude modes and representations, SPICE/AEM file inputs, active
local/named frame/reference graphs, spacecraft state representations and hidden
persistent fields still need review. Default equality, type catalogue coverage
and read-only flags alone do not prove complete persistence. Parent force-order,
computed-getter and retired-editor-clean-observation limitations remain explicit.
These checks do not qualify attitude dynamics, model file portability or kernels.

All 59 original prerequisite holds remain unresolved. Even if every other row
closed, the original scope would reach only 1,099/1,158 (94.9%); at least two held
obligations need substantive resolution for 1,101. Further source conflicts may
lower achievable coverage. No unanswered prerequisite decision is assumed.
Private NASA, historical platforms, scientific and complete current-build gates
remain open; this report does not declare the 95% goal complete.
