Vary unsupported-setting warnings and Yukon bound controls
=========================================================

This continuation proves the exact original requirement ``FRC!E52`` without
changing its literal ``DEFERRED`` wording, fingerprint or denominator. The
requirement concerns warning dispatch for unsupported Vary settings; it does
not qualify the numerics of every third-party solver. The independent audit is
``build/nasa-coverage/coverage95-20261102-vary-warnings/evidence-audit-v2.json``.
The cumulative audit advances the campaign to **1,010/1,158 (87.2%)**,
148 open and **91 more needed**. Historical operations remain
**1,816/2,367 (76.7%)**. The full 95-percent objective remains incomplete.

Complete obligation
-------------------

The original requirement fingerprint is
``d3d36805e08962704136c1f7c3adc2f05bd7c49d54241739dc03bf22b7e37f14``.
``requirement-review.json`` retains its exact text and clause-by-clause evidence.
Only this one row receives new credit. No Vary parent, other solver row or
administrative NASA disposition is inferred.

* All 16 combinations of the existing four solver capability groups are tested
  with each of six optional settings and an omitted-option control: 112 native
  initializations per execution mode.
* Explicit default values remain explicit. Unsupported VF13ad Lower and Upper
  settings each produce one warning per Run; omitted settings remain quiet.
* Eight lifecycle checks cover all-options, clone after serialization,
  assignment, omission, assignment clearing provenance, reparse clearing
  provenance, string setter and numeric setter.
* The actual factory census identifies exactly three Vary-compatible loaded
  solvers: DifferentialCorrector, VF13ad and Yukon. Their capabilities match
  the independently read table in ``doc/help/src/Command_Vary.xml``.
* Six actual GUI fixtures run twice each: VF13ad explicit, omitted and explicit
  default bounds; DifferentialCorrector supported settings; Yukon upper-only
  and lower-only bound optima. Actual MDI controls, messages, Unicode
  Save/reload, source/history preservation and native convergence are checked.
  Yukon bound fixtures contain no NonlinearConstraint substitute.

Both unattended and visible selections pass all eight CTest registrations:
new Gui and Contract tests plus the five original VF13ad Vary controls and
original NasaVaryParameter regression. All original regression assertions are
retained. Visible execution starts the resizable nested desktop Iconic, never
fullscreen. Thirty native exposure events are recorded; zero screenshots and
zero runtime binary copies are retained. This does not establish physical GPU,
host compositor or native OS input delivery qualification.

The independent auditor checks raw CSVs against ``Cost=(X-3)^2``, sentinel 41,
initial values and analytic optima X=2 or X=4; it matches displayed warnings,
native stdout, proofs, JUnit, exact source archives and loaded engine/plugin
images. Six negative controls reject missing/spurious warnings, false interior
optima, lost clone provenance, incorrect capability dispatch and incomplete
factory coverage. Absent licensed fmincon/SNOPT executions are not claimed;
the capability matrix checks the backend-independent warning contract.

Demonstrated production changes
-------------------------------

``Vary.cpp/.hpp`` now retain explicitly authored option identities across
cloning/assignment and clear them on reparse/destruction. A mutex-protected
out-of-object map preserves the command's existing object layout and virtual
interface. Initialization warns for unsupported explicit settings; successful
runtime numeric setters also warn. Omitted defaults do not warn. The warning
names the varied variable, field, selected solver and solver type. Numerical
values and solver algorithms are not changed by this warning implementation.

``Yukonad.cpp`` advertised no range support despite its documented capability
and adapter passing variable bounds to the NLP. Its declaration now enables
bounds. The new bound-only witnesses then exposed a separate false-convergence
defect: with MaxStep=0.5, Yukon stopped at X=1.5 or X=4.5 with cost 2.25 instead
of the bound optima X=2 or X=4 with cost 1. Exact analytic callbacks independently
reproduced both failures without GUI wrappers or finite differences.

``Yukon.cpp`` now requires complementary inequality multipliers before its
Lagrangian-gradient convergence criterion can succeed. QP multipliers describe
the full proposed step and can spuriously cancel the objective gradient after
MaxStep accepts an interior point. The check rejects that false stationarity;
the existing user function-change criterion and tolerances remain unchanged.
This is a scientific/core change requiring the campaign's separate review.
No general optimizer or ABI qualification follows from these bounded passes.

Both new analytic bound controls and 16 existing Yukon support cases pass.
The existing cases include parallel/reversed/scaled constraints, nonlinear
elastic recovery, QP controls and tight/wide infeasibility tolerances.

Versions and failures
---------------------

The stage preserves production/test preimages, patches, source-v1 through
source-v11, build logs and ``failure-register.json``. Final native GUI and
analytic proofs use source-v9. Source-v10 tried to add a Yukon build dependency and conditional registration,
but validation found that testing TARGET before plugin configuration removed
the registrations. Source-v11 uses the existing VF13ad guard and an explicit
Yukon dependency. A no-op build and exact command/image comparison pass;
executable sources and native registered commands match source-v9. Earlier passing receipts are retained at their
own versions and are not mislabeled as current-build qualification.

The first GUI run fails before the missing-warning fix. A later Contract
fixture initially read Qt messages before queued delivery; adding event
processing fixes that fixture. One new-test compile failed on a hidden getter
overload. Further GUI failures expose Yukon's false capability declaration and
then false interior convergence. Both analytic pre-fix failures remain failed.
The first support invocation passes 14 cases but cannot locate four unbuilt
executables; its parent remains incomplete. Those four pass after their targets
are built, in ``yukon-support-v9.xml``. No original assertion is weakened.

A separate bounded investigation of the earlier plot native segfault makes
five traced GDB invocations. All exit normally; the cause remains unresolved,
and no crash fix is claimed. See
``build/nasa-coverage/coverage95-20261102-native-recovery/diagnostic-conclusion.json``.

Replay and independent audit
----------------------------

Reuse the recorded passes unless new changes affect them. For an intentional
replay from the repository root::

  cmake --build build/linux-gui --target GmatQtNasaVaryWarningTests -j4
  python3 src/qtgui/tests/RunVaryWarningAcceptance.py --visible

Omit ``--visible`` for unattended execution. Add ``--regressions`` to include
six affected original GUI cases after building ``GmatQtNasaVf13adVaryTests`` and
``GmatQtNasaVaryParameterTests``. The visible launcher always requests an
initially minimized, resizable desktop and zero screenshots. Native X sockets
require the usual execution sandbox escalation.

The direct new registrations are
``QtAcceptance.NasaYukonParallelUpperBoundSteps`` and
``QtAcceptance.NasaYukonParallelLowerBoundSteps`` in
``GmatQtNasaYukonParallelTests``. The original analytic cases remain registered.

Offline admission::

  python3 src/qtgui/tests/AuditVaryWarningAcceptance.py \
    --qualification build/nasa-coverage/coverage95-20261102-vary-warnings/qualification-v2.json \
    --output /tmp/vary-warning-audit.json

The original immutable qualification binds 1,058 artifacts; qualification-v2
preserves it and adds the corrected registration follow-up evidence. BuildNasaCoverage95 now
routes this contract to that auditor; native replay alone awards no credit.
The 59 prerequisite holds and scientific/core/full-current-build/full-NASA
qualification gates remain open.

Cumulative ledger recovery
--------------------------

The cumulative auditor discovered a missing older console receipt under the
build tree and two changed live source references in a previous central-body
qualification. The exact missing console run was not recoverable; an alternate
older run was not hash-identical and was not substituted. The unchanged
ConsolePlotExceptions CTest registration was rebuilt and rerun: all eight
console cases pass, including three invalid-configuration controls. The original
console and GUI-interface auditors then pass with new replacement
qualifications. The old qualification and failed cumulative attempt remain.

PointMassForce.cpp and ODEModel.cpp were recovered byte-for-byte from the
rotating-stage and step-publication preimages. The existing source-relocation
contract revalidates the original central-body proof without native replay.
No original assertions, expected results or row membership changed.

Recovery records are under
``build/nasa-coverage/coverage95-20261102-ledger-recovery``. The new console run
is physically retained there; its recorded build path is a symlink documented
in ``console-path-recovery.json``. If the build tree is removed, restore only
that missing link after verifying the retained hashes. Do not overwrite an
existing path or fabricate the missing historical receipt.

The authoritative new ledger is
``build/nasa-coverage/coverage95-20261102-vary-warnings/coverage-snapshot-v2.json``.
It contains all 148 remaining original instances and preserves the 59 holds.
The old 1,009 ledger remains immutable historical evidence. The four plot
step/parent obligations remain open, including other sampled force/attitude
inputs, remaining frame/provider semantics and solver lifecycle/render
conjunctions. The unresolved native plot segfault remains a separate issue.
