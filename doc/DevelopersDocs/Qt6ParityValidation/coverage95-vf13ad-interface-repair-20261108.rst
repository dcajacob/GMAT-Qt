VF13ad interface repair and bounded verification, 2026-11-08
===========================================================

The private campaign runtime can again use its existing VF13ad library.
Three unchanged optimizer checks pass on private X11 and in the visible
nested desktop: ProgressForward, ProgressCentral and ResourceReportTruth.
The registered native value-provider support check also passes. This is
component evidence; canonical acceptance remains 1078/1158 (93.1%).

Concrete failure and repair
---------------------------

The private startup named the existing VF13ad component, but its private
runtime directory link was absent. Restoring that link exposed a crash:
the progress fixture could not build, and configuration cleanup called a
null virtual function. The existing library was not changed or substituted.

Recent function-value/restoration work had added three virtual methods to
GmatBase and two to Solver. These changed interfaces consumed by the
prebuilt optimizer. The repair keeps the added capabilities behind
nonvirtual base entry points and typed, scoped provider registrations.
Native inheritance and instance data remain unchanged. Providers register
their existing implementations outside the objects' virtual tables;
registration retirement removes the callbacks. Native superclass calls
retain their base implementation semantics.

The compiler comparison against tracked headers at commit
215be630654f66f3953528f58dea4cc2de639af9 finds identical GmatBase, Solver and
Optimizer virtual tables and class layouts under this build's Linux compiler
settings. Their sizes are respectively 1128, 2168 and 2744 bytes. This is a
specific baseline comparison, not universal binary compatibility proof.
After removing registration scaffolding and undoing method qualification
renames, all remaining non-whitespace source in 47 provider translation
units matches its preimage. No force, derivative or optimizer equation was
changed by this repair.

Execution and remaining failures
--------------------------------

* ProgressForward and ProgressCentral each check enabled and disabled
  reporting. Enabled reporting delivers five iteration frames; disabled
  reporting delivers none. Both reach the independently checked constrained
  optimum X=2, Y=-1, cost=2, including desired/achieved constraint values.
* ResourceReportTruth checks the GUI-applied and reopened mission. Its
  reported final X=2 agrees with the native result and CSV; cost is 1 for
  that separate scalar fixture. Its original assertion was retained.
* QtAcceptance.NativeValueProviders checks actual spacecraft-owned attitude,
  dormant DifferentialCorrector/Yukon value transfer, and Sign reference
  rebinding through the base entry points. Separate fixture checks exercise
  scoped callback selection, retirement, reentrancy and prepared restoration.
  The core support test is intentionally run through CTest, because the GUI
  runner selects only the qt-gui label.
* Seven original function resource cases fail after completed missions when
  inspecting the retained global-object store: Spacecraft, ODEModel, DragForce,
  GroundStation, DifferentialCorrector, BatchEstimator and Yukon. Rebuilding
  the exact pre-repair sources reproduces all seven at the same assertions.
  The repaired sources, provider link and selected workers were restored and
  rebuilt afterward. These failures remain open; their assertions and the
  function-store lifetime behavior were not changed.

All 1334 protected original inputs remain byte-identical. The new support
test adds no original NASA row, changes no tolerance, and supplies no
initialized-model or complete function/global-lifetime qualification.
Backward derivative selection remains unqualified, so FRR!E286, E289 and
E292 remain open. Formal holds, original denominators, historical evidence,
the accepted prefix and platform/scientific gates remain unchanged.

Artifacts and continuation
--------------------------

Stage: ``build/nasa-coverage/coverage95-20261108-vf13ad-current-review``.
It retains initial failure logs, debugger traces, source preimages,
``abi-repair/abi-layout-v2/result-v2.json``, the exact-source attribution
control, execution manifests, raw optimizer reports, native proof and
visible receipts. ``results-summary-v1.json`` and ``publication-v1.json``
bind the completed component. ``implementation-v1.patch`` is relative to
this component's starting workspace, preserving earlier work.

The finite implementation audit continues with the original ephemeris-format
and OEM B-1 group. Reproduce the unchanged NarrowAtLast case on current
sources before treating its recorded scientific failure as current. Keep
the original B-1 scope, unresolved coordinate-system wording and all other
acceptance requirements intact. No campaign completion, installed runtime
update, package, commit or push is claimed.
