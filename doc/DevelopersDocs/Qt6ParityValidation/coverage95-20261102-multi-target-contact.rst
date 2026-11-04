Multiple-spacecraft contact acceptance
======================================

The audited source-versioned union advances to **1,009/1,158 (87.1%)**,
adding exactly ``FRR!E597`` and ``FRR!E598``. There are **149 unqualified
original requirements**, and **92 more** are needed for95%. Historical
coverage remains1,816/2,367 (76.7%). The goal remains active.

Actual behavior and original scope
----------------------------------

ContactLocator keeps its existing scalar ``Target`` and adds an optional
``AdditionalTargets`` spacecraft list. The real resource editor exposes an
"Additional spacecraft" checklist. Each selected spacecraft records its own
ephemeris and receives its own native contact search; the combined report
identifies every selected target, including targets with no selected events.
An empty additional list retains the existing single-target path.

E598 is supported by independent, distinct-phase circular trajectories for
one, two and three selected spacecraft, with a fourth propagated spacecraft
excluded from recording and reports. Eight cases cover rise-only, set-only,
both, intervals-only, mixed reports, an alternate primary, a bounded query,
and no-contact observers. The roots use explicit geodetic/elevation algebra
and bisection with hashed orientation/time inputs, without native GMAT/GF
truth. All24 final Cartesian values are independently checked per case.

E597 closes by conjunction of its exact children: new E598; E599/E603/E604
from the sealed85-percent prefix; and E600/E601/E602 from the independently
reaudited station-selection qualification. Inherited E604 includes actual
occultation suppression/splitting results. **No newly executed multi-target
occultation combination is claimed.** The original wording does not demand
every Cartesian product of its options. This does not qualify unrestricted
trajectories, all scientific models/platforms or the complete current build.

Verification and retained failures
-----------------------------------

All eight new GUI cases pass unattended and visibly. Visible execution starts
minimized, remains resizable and non-fullscreen, uses automatic host scaling,
and records206 native exposure observations across ten GUI cases. No PNGs
are retained. The original default and station-Both cases pass in both modes;
the546-byte default report is byte-for-byte identical to its prior receipt.

Every new case verifies saved/reopened selections, pending edits, Cancel and
Discard, Undo/Redo, selected target/observer identity, raw root/interval values,
final orbits, independent ephemeris-manager ownership and the Output viewer.
Runtime clone and assignment reports equal the original; append equals two
complete copies; clearing removes cached events for every selected target.

The new native contract passes13 rejection paths plus deep copy/assignment,
rename/reference binding, detailed report-cell copying, ownership destruction,
reference replacement and repeated-initialization checks. The unchanged
station-parameter contract's12 rejection paths also pass. The existing
25-case station root-kernel matrix is reused without replay.

Five failed GUI attempts are preserved in ``multi-target-failures.json``:
incorrect order/schema assumptions in new fixtures, two failures exposing
lost labels in ContactResult copies, and a stale pointer in the new driver
after save/reopen. The debugger did not reproduce the latter crash; source
inspection found the invalid retained pointer, and the corrected driver
reacquires the configured object. All eight affected new cases were checked
again. Superseded GUI-driver runs are excluded from final qualification.

Initial admission attempts exposed missing native entries in the frozen
source partition and an unexecuted inventory-probe classification error.
Those metadata defects were corrected without test replay. Five corruption
controls reject wrong target identity, an unselected event type, an omitted
spacecraft, an incorrect A1 root and an incorrect interval duration.

Changes requiring scrutiny
---------------------------

Seven production paths have exact preimages, hashes and incremental patches
in ``multi-target-core-changes.json``:

* ContactLocator.cpp/.hpp: target-list metadata/reference binding, per-target
  workers, combined reports, deep ownership and aggregate clear; changed
  references invalidate workers, and initialization rebuilds transient
  observer/FOV state. Destruction now releases owned contact results.
* ContactResult.cpp: copy observer/no-event labels and owned events; maintain
  valid event counts after copying and clearing.
* ContactEvent.cpp/.hpp: initialize scalar metadata and copy every report
  column, sampled array and formatting/value field.
* ResourceProperties.cpp and EventLocatorDialog.cpp: the actual checklist,
  spacecraft/type validation and target/observer exclusion.

No propagation, force, state-conversion, time-conversion or root-search
algorithm changes occur in this batch. Original contact-test source bytes
are checked against the inherited compiled source, preserving all assertions.
The CMake registrations/build dependencies and acceptance readers are test
infrastructure changes, recorded separately.

Replay and evidence
--------------------

Build the focused targets::

  cmake --build build/linux-gui --target GmatQtNasaMultiTargetContactTests GmatQtMultiTargetContactParameterTests GmatQtStationEventParameterTests GmatQtNasaContactResultTests -j4

Watch just the new eight-case GUI family::

  python3 src/qtgui/tests/RunMultiTargetContactAcceptance.py --visible

Omit ``--visible`` for unattended execution. ``--case NAME`` selects individual
new cases; ``--include-native`` adds the two parameter contracts;
``--include-default-report`` adds the two affected existing GUI cases.
``--list`` lists only the selected CTests. The standard acceptance-build target
also includes both new executables.

Artifacts under ``build/nasa-coverage/coverage95-20261101/`` include:

* ``multi-target-coverage-snapshot.json``: the exact cumulative ledger and
  every remaining original gap.
* ``multi-target-qualification.json`` and ``multi-target-admission-v5.log``:
  the exact two-row conjunction and successful admission.
* ``multi-target-headless-v4`` / ``multi-target-headless-v5`` and
  ``multi-target-visible-v1``: selected final GUI evidence; native contracts
  come from ``multi-target-headless-v3``.
* ``multi-target-build-v6.json`` and compiled source archive:1,201 compiled
  inputs across550 objects, successful builds and executable/image identities.
* ``multi-target-visible-images.json``:14 actual PID-bound observations,
  seven headless and seven visible. The early headless interval-only and
  visible PairRise processes were not observed; no maps are inferred for them.
* ``multi-target-negative-controls.json``, ``multi-target-independent-review.json``,
  preimages/patches, frozen reader sources and all failed receipts.

Admission rereads raw numerical outputs, binds all11 executing readers,
regenerates the independent fixture from frozen sources and verifies the
inherited obligations. Source/runtime versions are preserved without copying
ELF files or physical kernels. Lossless compression removes51.89MiB of
repeated summaries; all raw case results remain available.

Remaining gaps
---------------

The cumulative ledger retains149 unqualified original requirements. These
include GroundTrack integration-step sampling, the unresolved state-conversion
case, OEM/requirements conflicts, MATLAB prerequisites, Brent/user-tolerance
behavior and the previously documented function/model/authority gaps. This
increment does not change their disposition or credit them by association.
