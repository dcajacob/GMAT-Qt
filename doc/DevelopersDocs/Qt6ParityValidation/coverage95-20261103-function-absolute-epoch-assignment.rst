Function equation destination registration — 2026-11-03
======================================================

Coverage remains **1,019/1,158 (88.0%)**: 139 original requirements open,
**82 more needed** for 1,101. Historical coverage remains 1,816/2,367. Original
fingerprints, assertions, whole-row conjunctions and all 59 prerequisite holds
remain. No requirement credit or new native-bound Parameter type is added.
All technical, scientific, current-build and full NASA gates remain false.

Failure, cause and repair
-------------------------

The preceding power stage retained three native failures on unchanged absolute
epoch equations in a GmatFunction. At first function initialization,
``GmatCommand::SetWrapperReferences`` could not find ``PrivateOnly.A1ModJulian``
for ``PrivateOnly.A1ModJulian = 21910.25 + Calls * 365.25;``. Main-scope power
reads had completed; no private-function callback had occurred.

``Assignment::GetWrapperObjectNameArray`` included a one-dot destination for a
literal RHS, but required more than one dot for an equation RHS. The interpreter
eagerly creates automatic Parameters from this inventory while parsing the
function. Function initialization then clones the automatic store before
validating/initializing command wrappers. An LHS-only time Parameter was omitted
from that inventory; placing the same name on the RHS happened to create it.

Assignment.cpp now uses the same ``dot count > 0`` condition in both branches.
The new comment explains why function destination registration must precede
scope cloning. No equation formula, setter, owner policy, writable eligibility,
member, virtual slot or factory classification changes. This is one newly
pending production path, bringing the core inventory to **242**: all prior
241 hashes remain unchanged. Exact clean-HEAD working preimage, incremental
patch, current working source and complete core inventory are preserved.
No general ABI or engine approval is inferred.

Unchanged original probes and affected regressions
--------------------------------------------------

All twelve selected existing registrations pass unattended and visibly:

* Three original power probes, with their absolute private/Global epoch
  statements, strict controlled epochs, 1e-12 power bounds and independent
  decay/constant-bus/margin/clamp formulas unchanged.
* Three separately named self-referenced power controls from the preceding
  stage, retaining their own statements and expected epochs.
* Existing A1ModJulian, CurrA1MJD, UTCGregorian and DryMass function bindings.
* Existing SettableParameter and ResourceExpression Assignment editor cases.

The **24 GUI passes** include six original-probe and eighteen regression passes.
Function cases check 60 missions, 240 repeated/nested calls and synchronous
callbacks, 480 Parameter owner/Write/Report observations and 300 markers.
Power cases account for 288 owner/power observations; 144 belong to the repaired
original absolute probes. Four scalar Assignment missions retain literal results
4.5 and 215.75 and twelve invalid Apply/OK rejections. Existing function-file
keyboard Cancel/Save, source/history/prototype isolation, Unicode/no-Build repeat
and report viewer assertions remain in the unchanged native test sources.
Epoch tests retain their original scope/clock consistency limits; this stage
does not qualify general time-conversion science.

Twelve actual visible process maps and **174 exposure events** are recorded.
Inputs are identical before/after replay, with initially minimized, resizable,
nonfullscreen software-rendered windows. Screenshots, runtime-binary copies and
kernel copies are zero. Eight corrupt proof controls must be rejected by the
offline auditor. All build/native handles are terminal. There are no new native
failures, compile failures or interruptions; the three earlier failures remain
bound at their original source versions and are paired with current passes.

The three native C++ assertion sources and the power catalog match prior
receipt hashes. The older Parameter catalog matches the prior final source
archive; the Assignment catalog is statically bound and identical through the
new executed capsules. No nonexistent earlier runtime hash is claimed. An
offline artifact helper initially required a missing older-catalog key in the
power receipt; its exact failed source and captured error record remain. The
corrected helper states these custody boundaries explicitly. A second offline
seal attempt caught unstable list order from set iteration of the three retained
failures. Its exact source/log remain; the seal compares that list in stable
invocation/test order, preserving every identity and multiplicity, and compares
all other audit fields exactly. Native assertions and the production patch
remain unchanged. No native receipt or source capsule was rewritten.

Evidence and replay
-------------------

``src/qtgui/tests/RunFunctionAbsoluteEpochAssignments.py`` selects all twelve
existing cases. ``AuditFunctionAbsoluteEpochAssignments.py`` independently checks
raw emitted/stored proofs, native JUnit, function/Assignment files, actual output
bytes, owner references, image maps, source identities and negative controls.
The shared power value/owner contract is reused only after checking each raw
absolute/self-referenced mode and argument; raw evidence is never rewritten.

From the repository root, use fresh output directories::

   cmake --build build/linux-gui --target GmatQtNasaFunctionPowerParameterBindingTests GmatQtNasaFunctionParameterBindingTests GmatQtNasaAssignmentTests -j 6
   python3 src/qtgui/tests/RunFunctionAbsoluteEpochAssignments.py --output build/gui-acceptance/absolute-epoch-replay
   python3 src/qtgui/tests/RunFunctionAbsoluteEpochAssignments.py --visible --output build/gui-acceptance/absolute-epoch-visible-replay

Authoritative artifacts are in
``build/nasa-coverage/coverage95-20261103-function-absolute-epoch-assignment/``:
``evidence-audit-v1.json``, ``absolute-epoch-assignment-matrix-v1.json``,
``remaining-parameter-binding-worklist-v1.json``, ``evidence-inputs-v1.json``,
``stage-receipt-v1.json`` and ``seal-verification-v1.json``. All 566 matrix rows
and previous fields remain; current absolute-probe witnesses are additive.

Native binding evidence stays **125/321**; **196 types remain** (182 Spacecraft,
14 SpacePoint), including 41 older bounded names. All 321 whole obligations
remain open. Continue rotation, derived thruster, plate, attitude, frame, time,
orbit, vector/matrix/string and provider routes. Keep earlier strict Solver
clone, unmodeled System, strict time/custom Star probes, E491/E477/E406 and
science/source conflicts. SolarPowerSystem, nonconstant distance bus, internal
PowerSystem spacecraft-pointer identity, unit-label and electric-provider gaps
remain. The MATLAB installation-path question is pending; no hold is waived.
The full 95% goal remains active and incomplete.
