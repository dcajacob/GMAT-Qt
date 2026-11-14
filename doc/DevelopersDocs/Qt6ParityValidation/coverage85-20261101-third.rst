85 percent campaign: external C interface and precise epoch repair
==================================================================

Requirements coverage is **946/1,158 (81.7%)**. This increment adds sixteen
exact C interface rows to the previous 930. The campaign has added 22 rows
since its sealed 924-row baseline. **39 more requirements are needed** to
reach the target of at least 985; the goal remains active. Historical coverage
is unchanged at **1,816/2,367 (76.7%)**. All original denominators and remaining
source rows are retained, including deferred and contradictory requirements.

What actually ran
-------------------

``GmatNasaCapiAcceptanceTests`` is compiled as C11 with warnings treated as
errors. A compile-time guard rejects C++ compilation. It includes the shipped
pure-C header and directly links the real ``CInterface`` library; it invokes
the engine in a private startup/output directory. The receipt identifies the
actual loaded project libraries, executable and source inputs before and after
execution. Runtime images are hashed, never copied into evidence archives.

The basic case passes **133 assertions**. It loads two spacecraft and distinct
force/propagation models, checks the complete thirteen-object configuration
inventory, runs two circular trajectories with different radii, planes and
durations, and checks every Cartesian result against analytic equations.
Both model selectors, full state assignment/readback, description, dimension,
status and per-model isolation are checked. Acceleration is checked against
Newton's equation at propagated and caller-assigned three-dimensional states.

The STM case passes **481 assertions**. All 36 post-run STM entries for each
model are compared with central finite differences of a separate C math-only
RK4 point-mass integrator. At identity, STM derivatives must equal the analytic
Newton Jacobian. At a nonidentity matrix, all entries must equal ``A*Phi``.
The evidence auditor independently recalculates these results in Python.

The epoch case passes **620 assertions**, including the same Cartesian,
Jacobian and STM tests. A private read-only test module observes actual scalar
and precise propagation-state/model/force epochs after public C API calls.
Distinct literal A1ModJulian epochs must reach all five observed values;
switching models must preserve their separate states and epochs. A rejected
native epoch must return failure and preserve the previous propagation state.
All dynamics actions occur without reloading the successful configuration.

These are real engine/API acceptance cases, **not visible GUI passes**. No GUI
case was replayed for this increment. The earlier visible plotting proofs keep
their minimized, resizable, non-fullscreen window policy and original receipts.

Exact new source rows
----------------------

* ``All Others!E246``: actual C-language ABI.
* ``All Others!E253/E254/E255``: configuration data parent, count and all names.
* ``All Others!E259/E260/E261/E262``: complete tested dynamics-action parent,
  Cartesian assignment and native-observed A1ModJulian epoch assignment.
* ``All Others!E263/E264/E265``: acceleration, Jacobian and STM derivative.
* ``All Others!E266/E267/E268``: model state, full descriptions and dimensions.
* ``All Others!E270/E271``: post-run Cartesian state and complete STM.

No credit is assigned to MATLAB wrappers, linked full FRR/FRC configuration
families, all-command summaries, or the deferred public spacecraft epoch getter.
The private epoch observer is not a new public C ABI function and cannot close
the missing getter requirement.

One new core change to scrutinize
----------------------------------

The first epoch test found that ``SetState`` reported success with scalar
epoch21545.25 while the precise propagation epoch remained21545.000367678607.
``plugins/CInterfacePlugin/src/plugin/CInterfacePluginFunctions.cpp`` now
synchronizes scalar and precise state/model/force epochs and elapsed-time
bases. A rejected native model epoch returns failure before changing the
propagation vector. The public ABI, force equations and integrator algorithms
are unchanged.

The repaired epoch case and the affected six-state basic case both pass with
unchanged source/runtime hashes. The latter is an intentional affected-case
regression, rather than unrelated GUI replay. The earlier 100 changed core
files remain byte-identical; this adds **one**, making **101** changed core
files relative to the recorded HEAD. All remain pending independent technical
review. The epoch patch, before/after hashes and review limits are retained in
``build/nasa-coverage/coverage85-20261101/capi-epoch-core-review.json`` and
``capi-epoch-core.patch``.

Failures and evidence guards
------------------------------

Five ineligible acceptance attempts are retained: two incorrect fixture text
expectations, incomplete nested-library capture, the actual epoch defect, and
a source-change-during-recording rejection. Each has a passing replacement;
none earned coverage. Two build failures and one pre-execution setup failure
are also recorded. The last metadata failure occurred because the audit helper
changed while the case was recording inputs; the guard correctly rejected it,
and the affected case was repeated with frozen inputs.

Twelve coherently rehashed corrupt proofs are rejected without executing the
engine or GUI: extra/duplicate credit, full-NASA overclaim, fake GUI execution,
missing C library, incorrect Cartesian state, acceleration, STM, STM rate,
precise epoch, skipped JUnit and a removed native assertion.

Failed source archives are retained as verified source deltas against a sealed
passing archive. The affected basic regression reuses the identical epoch
source archive. All original member bytes remain recoverable; this saves
24,301,779 bytes. No screenshots or runtime binary copies are retained.

Reusable execution
--------------------

From the checkout root::

   cmake -S . -B build/linux-gui -DPLUGIN_CINTERFACE=ON
   cmake --build build/linux-gui --target GmatNasaCapiAcceptanceTests GmatNasaCapiEpochProbe -j4
   ctest --test-dir build/linux-gui -R '^QtAcceptance.NasaCInterface' --output-on-failure

Each invocation writes a unique private directory under
``build/linux-gui/capi-acceptance``. The local plugin option is enabled; the
installed GUI startup and existing GUI/base/util executable images were not
changed by building these test targets. A future full GUI startup regeneration
can include the enabled existing C plugin; no complete-current-build pass is
inferred here.

For a fresh retained run, the reusable evidence auditor can seal its exact
requirement review, for example::

   python src/qtgui/tests/AuditCapiAcceptance.py --run PATH_TO_EPOCH_RUN --case epoch --output epoch-qualification.json

Use ``--case basic`` or ``--case stm`` for the other cases. The coverage union
accepts these qualifications through ``BuildNasaCoverageExtension.py`` and
rejects duplicate or unsupported keys. Existing qualifications are reused;
auditing a receipt does not execute the engine again.

Remaining scope and next coverage work
---------------------------------------

**212 requirements and 551 historical cases remain unqualified.** The complete
original rows, fingerprints, reasons and closure gates are exported in
``build/nasa-coverage/coverage85-20261101/coverage.json``. The next substantive
GUI block is live solver-iteration publication/replacement: all/current/solution
behavior for OrbitView, XYPlot and GroundTrack. Existing final histories and
mode controls are retained; they cannot substitute for the missing transient
membership oracle.

Other open work includes finite-maneuver polynomial integration, coordinate/
state/body families, missing unattended conjunctions, console capability, and
legacy or absent dependencies. API derivative-buffer resizing across mixed
state dimensions and nested command-summary traversal remain unverified
candidates, explicitly recorded for review. None receives credit here.

This is source-versioned capability evidence. Old passes retain their original
source identities. Full current-build qualification, scientific/core approval,
physical input/host-GPU/cross-platform qualification and full NASA qualification
remain separate and unachieved.
