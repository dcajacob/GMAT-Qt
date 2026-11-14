Implemented-feature verification, 2026-11-07
===========================================

Latest completed checkpoint
----------------------------

The clean final build is v14. The resumed original function run has 527 passing
cases, 35 failing cases and 213 cases not run after repeated common failures.
Seventy-nine earlier failures were explicitly held out of that resumed run.
These four categories account for the finite 854-case original function plan;
they are not 854 successful tests. All source/runtime hashes bound to the
resumed run still match after removal of temporary diagnostics.

An additional clean private native X11 replay passes all 35 selected cases:
the nine new focused cases, ten original function I/O cases, three repaired RF
returns, eight original propagation-stop cases, both named-call integrations
and three representative former crash cases. Two original pure C API cases,
basic and STM, pass again on this final runtime. There is no live campaign
build/test process left from this checkpoint.

The function run exposed two further bounded regressions. Propagate installed
its replacement LHS wrapper after SetStopParameter, which now rebinds that
wrapper. Revalidation could therefore dereference the already retired wrapper.
The replacement is now installed first. Original cases which crashed inside
mission execution now pass without changing their assertions. The earlier
post-run inspection failures are separate and remain unresolved.

Cold standalone Transmitter/Transponder/Receiver outputs also failed because
the retained-value binder demanded an antenna edge and operational initialization
before one was configured. Such values now retain their cold configuration;
the ordinary operational hardware validation code is unchanged. All three
original return/edit/repeat cases pass. The separate Antenna/FOV circular-value
failure remains open. These additions bring the bounded production patch to
nine source files; they do not introduce a new ownership or language design.

Current open failures include local thruster associations in nested functions,
cached system mass-property values, solver-parameter owner identity, three tank
rotation write probes, the Antenna/FOV value cycle, and post-invocation scope
inspection. Three conic metadata cases still expect MA/EA/HA to be read-only;
their original assertions remain unchanged. The independent near-critical
BrouwerMeanLong reconstruction failure below also remains open. No failed or
unexecuted case has been assigned passing credit.

The local-spacecraft diagnostic ruled out retained-value context as the cause
of the tested missing-thruster/system-center-of-mass failures: source and target
context flags were all false. Its three temporary log statements were removed;
the file is byte-identical to its pre-diagnostic version. Do not add a speculative
local-spacecraft copy guard based on that rejected hypothesis.

Continuation control
---------------------

The saved campaign Goal still has status blocked and retains the old
implementation-before-verification wording. The later human authorization to
verify implemented features remains controlling. The user has been told that
``/goal resume`` restores automatic continuation; model tools cannot resume the
saved Goal. Do not claim that automatic between-turn continuation is active.
Official lifecycle reference:
https://developers.openai.com/cookbook/examples/codex/using_goals_in_codex.

For continuation, inspect the current failure manifest and resume narrowly from
the first common functional failure. Do not rerun all 854 cases or redesign the
function/resource language. The command/Mission Tree/interpolation follow-up
plan contains 233 existing cases across 23 targets; it has not been executed.
The literal campaign and all qualification gates remain incomplete.

Earlier verification checkpoint
--------------------------------

The user asked why each batch required another instruction to continue. The
campaign is authorized to proceed through implemented-feature verification.
Continue the finite original worklist without asking the user to restart each
batch. Keep unresolved definitions, external providers and historical platform
contracts open. This report records a checkpoint, not completion of the campaign.

Evidence is under
``build/nasa-coverage/coverage95-20261107-implemented-feature-verification``.
``plan-v1.json`` preserves the 40 implementation-present original rows in twenty
groups, including their full texts and fingerprints. ``checkpoint-v1.json``
records the current sources, results, retained failures and remaining work.
All 1,334 pre-existing test inputs and the original requirements, worklist and
tracking-v148 are unchanged. Original assertions and tolerances remain intact.

Repairs verified in this checkpoint
-----------------------------------

* Native function filenames such as Encoded.gmf.txt again resolve to the declared
  Encoded function. LegacyASCII retains the exact final .gmf suffix requirement.
  Four original file-policy cases pass, and twelve new source-policy contexts
  check both modes, UTF-8 literals and included-file suffixes.
* Cartesian conversion now converts eccentric and hyperbolic anomaly inputs to
  true anomaly. Hyperbolic anomaly conversion also preserves its direction.
  The optional angle wrapping in both conversions terminates correctly.
  Six signed anomaly assignments match an independent conic-geometry oracle.
* Bulirsch MinimumTolerance help and units use the actual plugin type name;
  its native hierarchy does not include that concrete name. Requested Accuracy,
  the configured floor, owned clones and execution are checked separately.
* SPK and CK writer constructors now pass name/type in the base constructor's
  declared order. CK starts with its supported Quaternion representation before
  Initialize. The earlier checked initialization exposed the reversed arguments.
* Stopping an unstarted, partially initialized or already stopped ephemeris
  manager is safe. Previously, failed event-writer initialization was followed
  by a null-writer dereference during Spacecraft cleanup. Both original crash
  attempts and their debugger traces are retained.

Current successful verification
--------------------------------

Forty-nine original unattended Qt cases and nineteen private native X11 cases
pass on the final sources. They include all original extended/hyperbolic GUI
conversion cases, fifteen SPK grouping cases, nine original eclipse cases,
the original OEM ShortArc, the two repaired named-call integration cases,
eight FK5 impulse/thrust cases, Earth orientation and actual plot cadence.
Private X11 is not user-visible/native-input or cross-platform qualification.

The nine new focused cases cover two-body propagation against independent
Cartesian RK4 integration, seven body orbit fields with Apply/Undo/Redo/reopen,
the Bulirsch floor, signed anomaly writes, worksheet/current compatibility,
sixteen numerical groups under each count policy, SPK count rules, scalar Brent
roots and safe recording cleanup, source policy, three executed SPK origins,
and actual Brent eclipse intervals at two tolerances. SPK origins are Earth,
Mars and an analytic user-defined body. Eclipse intervals use the existing
independent ray/ellipsoid fixture and retain its original tolerances.

Two pure C API cases, basic and STM, also pass. Runtime-path adapters select the
matching integrated libraries while preserving the original clients, assertions
and runner evidence checks. Their exact paths-only patch is retained. The native
direct-conversion client passes all 238 contexts; independent forward mean
reconstruction passes 25 of 26 contexts.

Retained holds and test conflicts
----------------------------------

The independent Elliptic_SphericalAZFPA to BrouwerMeanLong reconstruction fails:
maximum Cartesian position-component error is 135.34652689226186 km. The case
has inclination 64 degrees. The existing inverse warns in the 58.80--65.78 degree
range and exits a nonconvergent fixed-point iteration. The shipped NASA help,
Resource_SpacecraftOrbitState.xml, documents critical-inclination instability.
Native agreement with the old expected mean vector does not resolve this
independent numerical failure. Retain the scientific hold; no replacement
averaging model, fixture change or qualification credit is recorded here.

Eight original anomaly GUI cases explicitly require anomaly destinations to be
read-only; an original Bulirsch case requires MinimumTolerance to stay hidden.
Those assertions conflict with the subsequently implemented worksheet behavior.
Their failures and inputs are retained unchanged. The new behavior is checked
by separate tests; the old failures must not be relabeled as passing.

Next finite verification work
------------------------------

Continue the existing function family: 854 registered cases across 45 targets,
86 unattended and 768 private native X11. The exact list is in
``function-family-plan-v1.json``. This runs existing tests, without expanding the
function/resource language. Then cover remaining Mission Tree operations,
Achieve families, interpolation boundaries and contact/intrusion Brent paths.
Reconcile implemented clauses in the original 24 partial/source-review rows.

The recorded verified baseline remains 1053/1158 (90.9%); 1093/1158 (94.39%) is
the carried engineering estimate, not a fresh acceptance result. All 105
original open acceptance rows and all qualification gates remain open.
No commit, push, public package or installed runtime update is recorded here.
