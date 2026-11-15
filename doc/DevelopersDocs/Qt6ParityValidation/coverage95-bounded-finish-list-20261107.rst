Campaign scope reset and bounded finish list
===========================================

Verification is now authorized by the user's subsequent human reply. The first
batch and its two unresolved integration timeouts are recorded in
coverage95-verification-restart-20261107.rst. The stopping-point and pending-question
text below describes the earlier implementation checkpoint; it no longer forbids
verification. Undefined/external requirements and acceptance gates remain open.

Current stopping point
----------------------

The source review has selected no further concrete implementation change with
a defined stopping condition. The campaign is incomplete: unresolved definitions,
external components and platform contracts prevent a literal finish of every
original clause. Runtime verification remains deferred by the user's phase order;
the pending phase-order question has received no human answer. Do not keep the
campaign running by inventing new language, solver or renderer requirements.

The finite remaining decisions are:

* Supply the actual Mars-GRAM backend/interface and required data.
* Obtain a compatible VF13AD provider wrapper/build interface and its required
  derivative controls. A licensed binary already exists under
  application/plugins/thinksys/vf13ad; its README offers wrapper source on
  request. The binary's absence from the integrated build plugin directory
  must not be described as absence from the checkout. Current ABI compatibility
  and backward-difference support have not been established.
* Define the worksheet's unresolved behavior: rotation drift, SPK attitude
  wording, literal Vary controls, unit-field scope and the OEM frame contract.
  The pinned CCSDS 502.0-B-1 section 4.2.2 explicitly has no definitive authorized
  REF_FRAME list; Table 4-3 supplies examples, so copying it would not recover
  the worksheet author's intended exhaustive list.
* Define the processor/RAM placeholders and the one-word Validation requirement.
* Resolve the literal historical OS and MATLAB platform/API contracts. The
  original explicitly names engOpenSingleUse; MathWorks documents it as Windows
  only, while UNIX engOpen starts a new process. Neither API names nor a silent
  substitution establish the requested cross-process behavior on every host.
* Establish the intended POV import scope beyond the implemented static model
  geometry. Do not infer an unlimited ray-tracing renderer.

The seven source-review areas below retain their limits. Existing source paths
need later behavioral verification; they are not an instruction to rewrite whole
subsystems. All 105 original acceptance rows remain open. No new percentage,
acceptance credit, test, feature-code change or runtime execution is recorded by
this checkpoint. Four recent compiled-source snapshots still match current files.

The bounded record is
build/nasa-coverage/coverage95-20261107-implementation-stop-checkpoint/checkpoint-v1.json.
It preserves all 105 row texts, fingerprints and dispositions. This stopping
point supersedes older continuation instructions below and in the worklist.

The user asked why the work had become unrecognizable. Implementation had
expanded into resource-copy, include and native ownership edge cases without a
clear stopping rule tied to the original worksheet. Stop that expansion.
The proposed embedded-include language extension was never implemented and is
abandoned. Existing changes are retained pending the requested later verification.

Status that can be reported
---------------------------

The recorded verified baseline is 1053/1158 (90.9%). Tracking-v148 carries an
engineering estimate of 1093/1158 (94.39%); it is not a fresh completeness audit
or acceptance result. All 105 original open acceptance rows remain open.
No new percentage or requirement credit is assigned here.

The 105-row worklist contains 40 rows already marked implementation present,
24 marked partial implementation or requiring source-scope review, 17 with
specification dependencies, nine with external dependencies, eight historical
platform targets and seven dependent parents. Thus the 65 unresolved engineering
rows are not a count of 65 demonstrated missing features.

Bounded source reconciliation
------------------------------

This is a source-only triage overlay, not a completed implementation audit.
The following seven areas cover all 24 partial/source-review rows. Their current
dispositions remain unchanged. The order below is the implementation-decision
order; it does not establish that an area needs new code.

1. Ephemeris: FRR!E374, E375, E406. Compare the original OEM B-1 and final-solution
   clauses with the existing ephemeris writers and solver/estimator publication
   paths. Select a change only for a specific unsatisfied clause. Broad format
   or scientific coverage being unverified is insufficient evidence of missing
   implementation.

2. Functions and MATLAB: FRR!E450, E477, E479, E491 and All Others!E241-E244.
   Reconcile the original I/O, command and scoping clauses with the existing
   function and callback paths. FunctionManager.cpp:274 already contains retained
   resource handling. MatlabInterface.cpp:1190 and :1466 implement the two engine
   opening routes; :1003 and :1449 explicitly restrict SingleUse to Windows.
   Preserve that platform limitation as a concrete fact. Do not infer actual
   cross-process sharing or complete I/O coverage from the API names alone.

3. Solver commands: FRC!E7, E55. Map the existing command inventory against the
   original all-command requirement, including the current registration path
   in SolverBranchCommand.cpp:1327. A concrete unsupported command/context can
   justify a repair; a generic demand to redesign all lifetimes cannot.

4. Resource Tree, Clone and Save: FRC!E132 and All Others!E120, E156, E157.
   Use the retained 33-category original inventory to identify an unsupported
   resource operation. Save.cpp:605 and :916 already execute native resource
   serialization. MainWindow.cpp:573 and :584 explicitly reject cloning across
   mixed configuration/mission includes and included array initializers.
   These are known source-layout restrictions; their necessity for the original
   any-object requirement must be established before expanding the script
   language. Do not resume the abandoned include extension.

5. Body/frame properties: FRR!E501, E502, E525, E526. Map the properties and
   referenced body/frame lists in the original worksheet against current native
   parameter and coordinate-conversion routes. Keep unsupported combinations
   distinct from combinations whose implementation has not yet been verified.

6. Spacecraft models: FRR!E44, E46. Reconcile the current POV importer before
   selecting further work. PovModel.cpp:622 already implements macro definition;
   the current parser also has conditional, loop and switch paths. Earlier
   findings listing those features as entirely missing are stale. The worksheet
   gives no dialect/profile; do not turn the row into an unlimited POV renderer.

7. Console parity: All Others!E20. Console driver.cpp:188 and :219 already invoke
   the shared Moderator interpreter and mission runner. Preserve the original
   GUI/plot exceptions. Identify a specific missing console capability before
   claiming the console needs implementation work.

Specification and dependency work
----------------------------------

The 17 specification rows include undefined rotation drift, literal placeholders
and contradictory wording. Do not invent spin acceleration or silently replace
worksheet text. The nine external rows concern Mars-GRAM and VF13AD. The eight
historical targets include XP, Vista, Windows 7, Leopard and Fedora Core; modern
builds cannot establish those literal targets. The seven parent rows depend on
their children and do not create seven independent implementation tasks.

Execution boundary
-------------------

Every subsequent code change must name an original row, the precise missing
behavior, the owning source and a stopping condition. Reconcile an existing
implementation before extending it. Complete necessary implementation first;
runtime verification remains deferred at the user's request. Compilation is
permitted, but compilation is not acceptance evidence.

This document changes no source, fixture, assertion, tolerance, fingerprint,
denominator, disposition or qualification gate. It triggers no runtime execution,
commit, push, package or deployment. The campaign remains incomplete.

Sources: implementation-progress/worklist-v1.json, tracking-v148.json, the
original nasa-requirements.json extract and the owning source locations above.
All paths are relative to the GMAT-Qt repository or its build/nasa-coverage area.

Bounded work completed after reset
-----------------------------------

The OEM source review identified one concrete decimal-field gap: nonfinite
epochs/states could reach record formatting. The shared OEM 1.0 validation now
rejects them before buffering and rejects short direct-writer vectors before
indexing. The integrated build completed 71/71 steps without warnings; no runtime
verification or row credit was added. Existing final-solution suppression paths
were identified without redesigning them. See
coverage95-oem-numeric-fields-implementation-20261107.rst for scope and limits.
Continue with area 2, the original function I/O/command/scoping clauses. The full
implementation audit remains incomplete.

The area 2 review identified a run-control gap in solver contexts: a function
enabled its Pause/Stop poll only in RUNNING. Its existing polling gate now also
includes active solver publication states, preserving the cadence and handler.
The integrated build completed 4/4 steps without warnings. Existing I/O, local/
global scope, callback restrictions and engine-mode paths were identified; full
resource/command coverage is still unproven. See
coverage95-function-run-controls-implementation-20261107.rst. Next, reconcile the
original all-command solver inventory in area 3. No runtime verification or row
credit was added.

Source review checkpoint
-------------------------

The seven-area review is recorded in
build/nasa-coverage/coverage95-20261107-bounded-source-review/review-v1.json.
It preserves all 105 original row texts, fingerprints and dispositions, maps all
24 partial/source-review rows, and records current owning-source/factory hashes.
It is not a completed implementation audit or proof of runtime correctness.

The solver command dispatcher and structural registration paths already exist.
The old nested-optimizer comment does not establish a current blanket rejection.
The Resource Tree inventory retains all 33 original categories and conditional
library clauses, with the existing native/Qt copy routes. Save uses native resource
serialization. Body/frame properties have origin/coordinate-conversion routes;
console missions use the shared Moderator. None of these source observations
creates acceptance credit or justifies another general ownership redesign.

The model importer remains explicitly partial. Its object grammar does not handle
all POV geometry, including CSG difference/intersection. The legacy loader disables
POV and supplies no implemented compatibility profile to copy. Next, resolve this
remaining geometry/profile scope from original and primary format sources before
selecting a bounded model-import change. Do not ask the user to choose unfamiliar
POV jargon, repeat already implemented macros/arrays/loops, claim full POV support,
or expand into an entire ray-tracing renderer by assumption. The specification,
external and historical dependencies remain explicit and unchanged.

The primary POV-Ray `CSG grammar
<https://www.povray.org/documentation/view/3.60/533/>`_ and `inside/outside rules
<https://www.povray.org/documentation/view/3.7.2/303/>`_ establish subtraction,
intersection and merge as ordinary model geometry. The current parser rejects
these kinds. The next bounded increment is static CSG over supported finite
geometry, with transforms, surface data and valid solid operands preserved.
Closed-mesh operands require the format's inside_vector semantics. This is an
increment toward the full retained POV requirement, not a replacement profile
or a claim of complete format support. Research is recorded in
build/nasa-coverage/coverage95-20261107-bounded-source-review/pov-geometry-research-v1.json.

The bounded static-CSG increment is now implemented and compiled: difference,
intersection, merge, inverse, closed-mesh membership, transformed surface data
and explicit handling of completely removed models. See
coverage95-pov-csg-implementation-20261107.rst. Final compilation completed 6/6
steps, with only the existing Qt image deprecation warning; no runtime verification
or row credit was added. Remaining POV geometry/modifier/texture scope stays
explicit. Return to the original remaining clauses and inventory before selecting
more code; do not automatically expand this increment into a ray tracer or repeat
the earlier resource/include redesign.

Compatible cost syntax and remaining decisions
------------------------------------------------

FRC!E103 now has a compatible implementation: NonlinearConstraint optimizer(cost)
uses the existing objective path; comparison forms retain constraint behavior.
The selected spelling survives native copying/serialization and Qt source-span
editing. Both compilation passes succeeded without warnings. The original
disposition remains unchanged, with no runtime verification or row credit.

See coverage95-remaining-implementation-decisions-20261107.rst and its ledger for
all 34 originally specification/external/historical rows plus seven dependent
parents. GravitationalAccel already exists on both Thruster and ImpulsiveBurn;
that entry must not keep driving duplicate implementation. Undefined rotation
drift, literal placeholders and authentic external backends remain explicit.

The phase-order question is pending with the user: verify implemented features
while keeping unresolved requirements/gates open, or keep verification stopped
until those holds are resolved. No reply means no phase change. The remaining
original scope, including broad resource/command and POV requirements, is retained.

One further concrete model gap is implemented: torus radius syntax and ring/horn/
spindle tessellation, including spindle modes, signed-radius inversion and empty
results. Visible internal sheets and solid membership use distinct paths. The
integrated build completed 5/5 steps without warnings; runtime verification and
row credit remain unchanged. See coverage95-pov-torus-implementation-20261107.rst.
This does not complete or redefine full POV scope. No further renderer expansion
is selected by that packet; return to the original remaining-clause/decision
ledger. The phase-order question is still pending, so verification stays deferred.
