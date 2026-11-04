Discriminating fmincon properties and complete parent
=====================================================

The independent whole-row audit admits FRR!E272 and E275-E279: five remaining
property children and their complete seven-property parent. Coverage is
**1,045/1,158 (90.2%)**, with 113 open and **56 more needed** for the unchanged
1,101 target. Historical operations remain **1,816/2,367**. Denominators, original
fingerprints and every unrelated row and qualification gate remain unchanged.

Requirement-by-requirement evidence
-----------------------------------

* E275, TolCon: actual editor value 0.1 accepts physical constraint violation
  0.05; value 1e-10 drives it below the strict bound. Restoring 0.1 restores the
  loose result. Native/MATLAB settings, actual violation, controls and costs agree.
* E276, TolFun: actual editor 0.1 accepts the nonstationary initial point; 1e-10
  reaches the independently known minimum. The current active-set implementation
  uses this legacy option in its optimality stopping criterion. No claim that it
  directly bounds the absolute objective value is substituted for that behavior.
* E277, TolX: 0.1 stops at the nonstationary initial point with actual exit flag4;
  1e-9 reaches the known minimum. Nonidentity physical/optimizer scaling is checked.
* E278, DiffMaxChange: 0.1 versus 1e-8 changes the first measured optimizer-space
  finite-difference probe from sqrt(eps)*5 to exactly the configured upper clamp.
* E279, DiffMinChange: 1e-12 versus 0.02 changes that probe to the configured lower
  clamp. The active physical upper-bound optimum remains independently verified.
* E272: the complete creation/configuration parent joins those five children with
  the already qualified MaxIter and MaxFunEvals children. The exact prior actual
  Resource Tree creator Cancel/accept, all editable fields, native option IDs,
  49 atomic-invalid checks per mode, history and Unicode reopen are reused. The
  prior actual limit cases are re-audited at their original sources; no child is
  counted twice and the parent is admitted only after all seven are complete.

New execution and reused components
-----------------------------------

Twelve new GUI contexts pass: six profiles in both modes. They execute60 positive
real fmincon missions across baseline, edited, reopened, no-explicit-build repeat
and restored phases. Three actual GUI edits per ordinary profile preserve all
unrelated options, isolate drafts, Apply, Undo/Redo and save a Unicode caller.
Native, actual MATLAB options/X0/bounds/X, 270 real OutputFcn notifications,
990 raw GMAT branch evaluations and actual ReportViewer agree with independent
physical formulas. Perturbations are measured from the raw evaluation trace,
not inferred from successful configuration readback or final bounds alone.

The additional range profile configures equal and inverted positive minimum/
maximum perturbation limits. Real MATLAB rejects both; the Engine evaluates13
and the corrected missions then succeed in the same window. Four such expected
failed mission attempts remain with full diagnostics and source/history/disk/
settings isolation. These are passing negative controls, not hidden test failures.

Six prior GUI contexts and18 positive missions are reused: actual creator/fields
in both modes and both discriminating limits in both modes. Their raw receipts,
SDK/project mappings, exact source capsules and original98 aggregate invalid
controls are independently checked again without rerunning established natives.
Twenty-one corrupt-proof controls are rejected. All59 original hold identities
and139 reviewed source records survive:26 resolved,33 unresolved,including11
unresolved MATLAB rows. All23 rows in the tracked fmincon/selection/Vary slice now have
source-versioned proofs; broader MATLAB qualification remains false.

Source fidelity and remaining gates
-----------------------------------

All259 pending core paths and all11 production paths from the preceding iteration
integration are byte-identical. This increment adds acceptance code, registration,
runner and offline audit only. All11 older test sources are byte-identical. All43
initial new-test require expressions survive in source2, which adds only the
range/recovery profile and three checks (46 total). No numerical bounds, solver
formula, algorithm, API, class layout or generated startup changes in this stage.
Earlier exact core/ABI increments and failed runs remain sealed and pending review.

Source1 binds the five ordinary headless cases; source2 binds range headless and
all visible cases. Actual executables and SDK mappings, terminal JUnit, raw CSV/
JSON/messages and observer traces are retained. Six visible contexts start
minimized, resizable and nonfullscreen and record12 native exposures. No
screenshots, runtime/kernel copies or terminated licensing services are used.
Three build logs and a pre-execution source cleanup preimage remain; all builds
and new case selections pass. Reuse remains source-versioned capability evidence,
not qualification of the entire current build.

All321 Parameter whole obligations, E477/E491/E406, the old Object/Literals
I/O probes, PrepareMissionSequence full-menu failure, plot/source conflicts and
scientific, core, current-build, full-NASA and MATLAB bundle gates remain open.
The full95% objective stays active. Next inspect the11 remaining original MATLAB
rows and their exact I/O, process-mode and reverse-interface obligations; preserve
failed probes and whole-row conjunctions rather than promoting a bounded witness.

Replay
------

From the project root, build the reusable target and run the new selections::

  cmake --build build/linux-gui --target GmatQtNasaFminconOptionTests --parallel 2
  python3 src/qtgui/tests/RunFminconOptionAcceptance.py --output /tmp/fmincon-options-headless
  python3 src/qtgui/tests/RunFminconOptionAcceptance.py --visible --output /tmp/fmincon-options-visible

The runner also accepts --filter. Use a fresh source capsule/output name for any
new map-watched replay; old watchers reject source/binary drift. Offline admission::

  python3 src/qtgui/tests/AuditFminconOptionAcceptance.py --qualification build/nasa-coverage/coverage95-20261103-fmincon-options/qualification-v1.json --output /tmp/fmincon-option-audit.json

The exact cumulative reconstruction command is in ledger-replay-inputs-v1.json.
The prior sealed1039 union is reused through sealed-prior-prefix-v1.json, preserving
all original fingerprints and source-bound evidence. Primary behavior references:
https://www.mathworks.com/help/optim/ug/fmincon.html and
https://www.mathworks.com/help/optim/ug/tolerances-and-stopping-criteria.html .
