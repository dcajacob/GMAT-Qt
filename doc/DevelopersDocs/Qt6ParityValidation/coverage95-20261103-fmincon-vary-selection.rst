Optimizer selection and fmincon Vary settings
============================================

The independent audit admits exactly eight original requirements. Coverage is
**1,031/1,158 (89.0%)**, with 127 open and **70 more needed** for the unchanged
1,101 target. Historical-operation coverage remains 1,816/2,367. Every denominator,
original fingerprint, copied bundle gate and unrelated open row is preserved.

Admitted original clauses
-------------------------

* FRC!E14: the complete conditional optimizer-selection parent. Actual Optimize
  pickers contain both fmincon and VF13ad resources, exclude the targeter, and
  exercise Cancel/accept, draft isolation, Apply, exact history and Unicode reopen.
  Both selected backends execute complete sequences. FRC!E15 was already verified
  and receives no duplicate credit.
* FRC!E16: actual fmincon picker selection, native solver binding and real execution.
* FRC!E35: the complete five-setting Vary parent, including all original children
  below and their combined physical/scaled behavior.
* FRC!E36: unequal GUI initial guesses, actual native and MATLAB initial vectors.
* FRC!E37: a GUI lower bound changes the physical optimum from 2 to 3, with cost 1.
* FRC!E38: a GUI upper bound changes the physical optimum from 2 to 1.5, cost 0.25.
* FRC!E39: nonzero additive scaling, including negative combined scales.
* FRC!E40: nonunit multiplicative scaling, including unequal two-control factors.

All five actual Vary inputs are enabled. Unsupported fmincon perturbation/MaxStep
inputs retain their disabled state and explanation. Nine profiles pass in both
modes: Initial, Lower, Upper, Additive, Multiplicative, Combined, References,
SelectFmincon and SelectVF13ad. Nonselection cases each run baseline, GUI, reopen
and independent script phases; selection cases each run GUI-selected, reopen and
script phases. Cancel, pending isolation, atomic invalid correction, idempotent
Apply, exact Undo/Redo, native command state and Unicode save/reopen remain checked.

The independent scale oracle is xn = m * (xd + a), as stated in the local Vary
guide. Actual MATLAB X0, Lower, Upper and final X agree with native physical
controls and the independently computed transformation. The two-control fixture
reaches (1.5,-1.5), cost 0.5 and equality residual zero with factors (0.25,2) and
additive terms (-3,4). The reference profile uses a Variable initial value, Array
element lower bound, spacecraft parameter upper bound and Variable scale factors;
its real results agree with the equivalent literal script. Raw evaluations,
native results, solver identities and actual readonly ReportViewer contents agree.

Evidence and repair
-------------------

There are 24 passing GUI contexts: 18 new and six affected original regressions.
Positive contexts execute 96 solver missions: 62 real fmincon solves, six newly
selected VF13ad solves and 28 legacy VF13ad missions. There are 64 new atomic
invalid-input checks, 20 retained legacy property-picker rejections, eight actual
optimizer picker visits and 14 corrupt-proof controls. Twelve visible contexts
start minimized, resizable and nonfullscreen and record 58 native exposures.
The 1,010 new-profile GMAT branch evaluations are not claimed as MATLAB iterations.

The original unattended case accepted an infinite initial value through Apply.
Its failure, complete source, raw baseline run and maps remain. The production
repair changes only CommandForm.cpp: edited numeric initial/bound/scale literals
must be finite; edited numeric multiplicative factors must be positive. Expressions
and references remain delegated to native validation, and untouched settings stay
intact. The repaired initial case uses byte-identical assertions. The later test
additions strengthen VF13ad convergence evidence and add the reference profile.
All original behavioral/numerical expressions survive; only the profile catalogue
guard is extended. One compiler failure and its exact preimage remain as well.

All six prior test sources remain byte-identical, including affected VF13ad Vary
and property cases. The 257 pending core paths are all unchanged; no public API,
class member, virtual slot or numerical formula changes. Exact frontend and test
registration patches, four executed source capsules, actual loaded SDK/plugin
maps, raw reports and separate admission-reader sources are retained. Generated
startup is unchanged. No screenshot, runtime/kernel copy or licensing-service kill.

Remaining original scope
------------------------

All 139 reviewed source records and all 59 original hold identities survive:
12 holds are resolved, 47 unresolved, including 25 MATLAB rows. The other 14
optimizer rows are FRR!E272 through FRR!E285: discriminating option/limit/tolerance/
finite-difference behavior and truthful actual MATLAB iteration/message/report
conjunctions still need complete proof. Prior resource option editing/readback
evidence is reused, but is not counted as those runtime semantics.

All 321 Parameter whole obligations, E477/E491/E406, earlier Object/Literals probes,
PrepareMissionSequence menu failures, plot/source conflicts and independent
scientific/core/current-build/full-NASA/MATLAB bundle gates remain open. The full
95% objective remains active and incomplete.

Replay
------

Build the registered targets::

  cmake --build build/linux-gui --target GmatQtNasaFminconVaryTests GmatQtNasaVf13adVaryTests GmatQtNasaVaryOptimizePropertyTests --parallel 2

Always select fresh output directories::

  python3 src/qtgui/tests/RunFminconVaryAcceptance.py --output /tmp/fmincon-vary-new-headless
  python3 src/qtgui/tests/RunFminconVaryAcceptance.py --visible --output /tmp/fmincon-vary-new-visible
  python3 src/qtgui/tests/RunFminconVaryAcceptance.py --filter '^QtGui.NativeNasa(VF13ad(Additive|Multiplicative)|VaryOptimizeProperty)$' --output /tmp/vary-legacy-new-headless
  python3 src/qtgui/tests/RunFminconVaryAcceptance.py --visible --filter '^QtGui.NativeNasa(VF13ad(Additive|Multiplicative)|VaryOptimizeProperty)$' --output /tmp/vary-legacy-new-visible

The stage's run-with-maps wrappers additionally bind actual process descendants
and loaded images to exact source capsules. Existing source4 wrappers refuse
changed inputs; changed-source work requires a fresh capsule, rather than editing
an old receipt. The independent audit can be replayed into a fresh output::

  python3 src/qtgui/tests/AuditFminconVaryAcceptance.py --qualification build/nasa-coverage/coverage95-20261103-fmincon-vary-selection/qualification-v1.json --output /tmp/fmincon-vary-audit-new.json

The exact cumulative builder command and inherited qualifications are recorded in
ledger-replay-inputs-v1.json. Change its output path for a fresh ledger replay.
The row-scope review, source QA, qualification, closure overlay and 23-row optimizer
audit preserve all exact original source text, fingerprints and remaining gaps.
