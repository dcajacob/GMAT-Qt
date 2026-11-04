85 percent campaign: complete propulsion and C status contracts
===============================================================

Requirements coverage is **972/1,158 (83.9%)**, with **13 more needed** for
985. This increment adds ``FRR!E85``, ``FRR!E90`` and ``All Others!E258``.
There are48 additions since the sealed924-row baseline. Historical coverage
remains **1,816/2,367 (76.7%)**. All **186 remaining requirements and551
historical gaps** retain their original scope, fingerprints and closure gates.
The85percent goal remains active.

Propulsion evidence
---------------------

Only the missing unattended executions of six original integrated-thruster
cases and two original custom-body burn cases were run. The retained visible
passes and original legacy clauses in both modes are reused. All stronger
seven-clause integration and three-clause local-origin obligations remain
mandatory. A new user-body Local MJ2000Eq case supplies the third listed local
axis route in both modes. The full composed proof contains22 clause/mode
obligations; no complete parent or any-FRR35 coordinate-family credit is added.

The six integration cases cover regulated pressure, blowdown, pressure-dependent
Isp, mass-off, duty switching and thrust-scale switching. Their unchanged
numeric assertions check all300 reported values per case: mass, pressure,
temperature, decoy tank, total mass, position, velocity, thrust, Isp, flow,
acceleration and sentinel. Closed-form pressure/mass/rocket equations and
trajectory quadrature are recalculated independently by the evidence auditor.
Coast, shutoff and restart remain explicit. This is a synthetic negligible-
gravity propulsion fixture, not empirical propulsion or arbitrary-polynomial
science qualification.

For Local VNB, LVLH and MJ2000Eq with a user-provided Ceres-backed body,
three prescribed states and three spanning impulses check all252 reported
values per execution. Independent vector bases, unchanged inertial positions,
translated-origin states, rocket mass depletion and backward restoration are
mandatory. The inertial free vector must be unchanged by origin translation;
VNB/LVLH must differ from Earth-origin controls. The actual configured burn
must retain Local, MJ2000Eq and Probe, even while Origin is inactive in the
inertial GUI route. Shared SPICE ephemeris is explicitly an origin input, not
independent ephemeris truth.

Ten passing numerical GUI executions provide2808 newly checked numeric values.
The new visible case passed in2.326seconds in an initially minimized, resizable,
non-fullscreen desktop. No unrelated known passing case was repeated.

C status evidence and failures
--------------------------------

The existing133-assertion pure-C client pass already directly calls
``getLastMessage``. A separate source-versioned audit now credits its complete
status contract: initialization, failed nonexistent-script load with a last
diagnostic, successful load, exact latest mission-success message and state
update messages. The entire original C-client qualification is rechecked.
No C API, GUI or engine execution was repeated for this credit. Command-summary,
linked configuration-family, MATLAB and deferred epoch-getter scopes stay open.

One new test attempt crashed: a native-configuration assertion held a resource
pointer across Undo/Redo and reload, which replace that object. The assertion
now fetches the current configured object at its point of use. All native
identity and original numeric assertions remain intact. Only this failed case
was rerun unattended, followed by its new visible companion; both pass.
One omitted explicit execution-only review also failed preflight before any
GUI/engine execution. Both are recorded with exact artifacts and dispositions.

**No production GUI or engine source changed.** The original impulsive C++ test
gained one isolated case; the existing numerical formulas and assertions are
preserved. CMake gained only its registration. New auditors and coverage-union
dispatch compose actual evidence. All101 existing changed core files remain
byte-identical and pending independent review.

Eleven coherently rehashed corrupt propulsion/status proof copies are rejected,
including coherent actual/expected fuel corruption, wrong custom origin,
missing visible evidence, deleted legacy clause and wrong C-client mode.
These checks launch neither GUI nor engine. Original reference paths remain;
seven documentation-version pairs only clarify units, while old normative
illustrations are reused at their exact identities. Receipts are never rewritten.

Passing screenshots and runtime copies remain zero. Source-only deltas retain
exact test bytes; run-local planning/coverage summaries are losslessly compressed,
saving22,962,491bytes while preserving receipts, logs, JUnit and numeric reports.
Source-versioned coverage does not establish complete current-build, physical
input, arbitrary-domain, host/GPU/platform, scientific or NASA qualification.

Reproduce without replaying passing GUI cases
-----------------------------------------------

From the checkout root::

   python src/qtgui/tests/AuditHardwareConjunctionAcceptance.py \
     --qualification build/nasa-coverage/coverage85-20261101/sixth-qualification.json
   python src/qtgui/tests/AuditCapiStatusAcceptance.py \
     --qualification build/nasa-coverage/coverage85-20261101/status-qualification.json
   python build/nasa-coverage/coverage85-20261101/negative_sixth_qualification.py
   python build/nasa-coverage/coverage85-20261101/audit_sixth.py

Exact selected execution commands remain in their run receipts. The cumulative
``coverage.json`` lists all source-row additions and every original remaining
gap. Earlier increment audits and archives retain their own source versions.
