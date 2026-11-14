Station rise/set selection acceptance, 2026-11-02
================================================

The audited requirements union advances from 1,003/1,158 (86.6%) to
**1,006/1,158 (86.9%)**. Exactly FRR!E600, E601 and E602 are added.
E600 explicitly conjoins E601, E602 and the previously qualified E603
combined rise/set child. Historical coverage stays 1,816/2,367 (76.7%).
There are 152 original requirements remaining; 95 more are needed for the
95-percent target. No original row, fingerprint, assertion or denominator
was removed. This is source-versioned acceptance evidence, not blanket
NASA, scientific, current-build, physical GPU or host-desktop qualification.

Implemented behavior
--------------------

ContactLocator now exposes an EventTypes checklist in its actual resource
editor. StationRise, StationSet and both selections locate discrete apparent
elevation crossings; ContactIntervals preserves the existing default and
can be combined with either station event. Reports include selected observer,
event type, raw A1 epoch, UTC epoch and event count. Empty selections reject.
Station selections require a spacecraft target and ground-station observers.

The native search uses positive elevation windows and an analytic derivative
of the same apparent state/frame/correction. Interior boundaries are root
candidates; confinement endpoints require independent equality or a bracket
within GF's temporal resolution. Thus clipping an already-visible interval
does not invent a rise or set. Two-sided checks reject tangencies and
plateaus while retaining flat-slope crossings. Occultation is checked at the
root; a background body transited by the spacecraft does not hide it.

Validation and evidence reuse
-----------------------------

* All ten new GUI cases pass unattended and visibly: rise-only, set-only,
  combined, three mixed interval/event subsets, two bounded selections,
  no-event station selection and alternate-target identity isolation.
* The affected default MultiStation case passes in both modes. Both complete
  reports match its preserved 546-byte passing report byte for byte.
* All 25 native analytic-SPK cases pass, including query and actual ephemeris
  endpoints, clipped intervals, tangencies, plateaus, cubic crossings, shallow
  and shifted-cubic outside/inside roots, a latitude pole, singleton search,
  and foreground/background occultation ordering.
* A separate new native gate checks explicit empty-list parsing, initialization
  rejection, native choices, clone isolation and 12 invalid-input rejections.
* Existing independent circular-orbit/ellipsoid station roots supply the GUI
  oracle. Bounded expectations filter the original **unclipped** roots. Raw A1
  and UTC results, full reports/viewers, pending edits, Cancel/Discard,
  Apply/history, Undo/Redo, Unicode save/reopen and final orbital truth retain
  assertions. No broad prior passing suite was replayed.
* Visible receipts record 232 native exposure observations on xcb. The nested
  display is resizable, starts minimized, uses automatic scaling, and is not
  fullscreen. No passing screenshots or runtime/kernel copies were retained.

The three-row admission seals all actual receipts, logs, JUnit, raw reports,
input identities and frozen source partitions. It validates compiled-source
intersections and binds ten executing audit helper modules to reviewed source
bytes. Negative controls reject altered raw epochs, wrong selected event types
and missing visible evidence. Independent implementation review passes.

An older body proof had recorded ResourceProperties.cpp as a mutable artifact.
Its exact prior source bytes are preserved, and a narrowly audited relocation
delegates the full original validator against an otherwise identical
qualification. The original qualification, receipts and assertions remain
unchanged. This preserves old credit without claiming a new-build replay.

Production changes for scrutiny
-------------------------------

Seven production files changed in this batch. Exact preimages, diffs and
hashes are in ``build/nasa-coverage/coverage95-20261101/`` under
``station-selection-core-changes.json``:

* ``src/base/subscriber/EphemManager.cpp/.hpp``: additive native station
  crossing search and its public nonvirtual entry point. The original
  GetContactIntervals body is verified byte-identical.
* ``plugins/EventLocatorPlugin/src/base/locator/ContactLocator.cpp/.hpp``:
  event selection, clone/validation state, native search dispatch and report
  section. The default interval path bypasses the new search.
* ``src/base/interpreter/Interpreter.cpp``: explicit empty EventTypes input
  clears this new list, allowing native validation to reject it.
* ``src/qtgui/ResourceProperties.cpp`` and ``EventLocatorDialog.cpp``:
  actual checklist metadata, control placement and selection validation.

New numerical selection logic is core work and remains listed for user
scrutiny. This batch does not modify orbit integration, force evaluation,
state-conversion formulas or the existing time-conversion algorithms.

Failures encountered and retained
---------------------------------

The first native build exposed an incorrect integer-property overload. Native
iterations then exposed a missed exact confinement root and an invalid empty
point-body frame for occultation. Review added shallow and shifted-cubic
counterexamples that invalidated angular-epsilon/Newton-only recovery; actual
time-bounded bracketing replaces those proposals. A short synthetic kernel
needed wider support for its direct GF diagnostic.

The first actual GUI mission exposed equality-search evaluation outside the
propagated spacecraft ephemeris. The final inequality search plus independently
verified boundaries resolves this without trimming or extrapolating the orbit.
An equality-search alternative was rejected by source review before execution.

A stale review hash rejected one launch before native execution. A replay-list
path error and direct-auditor module-name handling were also corrected. The
first ledger merge exposed the older mutable source binding described above.
All failed runs, diagnostic sources, source versions and logs remain recorded.
No final selected test fails; this does not close older enabled failures.

Remaining coverage gaps
-----------------------

FRR!E597 and E598, including a user-defined spacecraft list, remain open.
Brent's method with user-defined tolerance (E605/E606) is not qualified.
Arbitrary trajectory/body/frame, light-time/stellar/FOV scientific domains
are not inferred from these geometric GUI cases and bounded native controls.
The broader state-conversion row still includes its enabled Brouwer failure;
GroundTrack integration-step sampling remains incomplete. Software/platform
prerequisites, historical/current contract conflicts and all other original
gaps remain in the exact 152-row requirements ledger.

Replay and inspect
------------------

From the repository root::

  cmake --build build/linux-gui --target GmatQtNasaContactResultTests \
    GmatQtStationCrossingKernelTests GmatQtStationEventParameterTests -j4
  python3 src/qtgui/tests/RunStationEventAcceptance.py --visible

The focused runner defaults to the ten new GUI cases. Add ``--include-native``
for the two native gates and ``--include-default-report`` for the affected
default report. ``--list`` lists the selected registrations without executing
tests or generating large inventory reports. Omitting ``--visible`` gives
unattended replay. New cases are also registered in CTest/check-qt.

Authoritative artifacts under ``build/nasa-coverage/coverage95-20261101/``:

* ``station-selection-coverage-snapshot.json``: complete cumulative union and
  every remaining original requirement/historical gap.
* ``station-selection-qualification.json`` and
  ``station-selection-independent-review.json``: source-bound three-row proof.
* ``station-selection-headless-v3`` / ``station-selection-visible-v1``:
  the two terminal GUI runs.
* ``station-crossing-native-headless-v7`` and
  ``station-selection-parameters-headless-v1``: native gates.
* ``station-selection-core-changes.json``: all seven production changes.
* ``station-selection-storage.json``: lossless compression saved about
  103 MiB of repeated summaries; raw case evidence and receipts remain intact.

The native GF contracts were checked against NAIF's primary documentation:
``https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/gfuds_c.html`` and
``https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/gfposc_c.html``.
