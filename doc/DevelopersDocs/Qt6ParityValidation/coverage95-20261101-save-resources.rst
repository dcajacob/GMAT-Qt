95 percent campaign: native Save resource checks
===============================================

Result and coverage boundary
----------------------------

All 45 new Save fixtures pass unattended and visibly: 90 contexts and 1,696
independently repeated writable-property comparisons. The fixtures exercise the
actual Mission Tree Save editor and object picker, preserve selection order and
unrelated source, execute Save and the following statement, and rebuild its raw
export. Native parameter IDs, typed getters and owned-object inventories supply
the expected state; neither the serializer nor the GUI property list supplies
that oracle. Thirty distinct native root types appear in the fixtures, including
a complete configured Simulator reference graph and all nine numerical
integrator types. This is bounded serialization evidence, not science validation.

Original NASA coverage remains **998/1,158 (86.2%)**, with **103 more complete
requirements** needed for 95%. Historical coverage remains **1,816/2,367
(76.7%)**. FRC!E132, fingerprint
``5546e993341e14217244951300a7744969787f96fb0af50bf78a3aa60037dfe3``,
remains open: "any Resource" requires more families and graph shapes than these
fixtures establish. No denominator, requirement text, fingerprint, assertion,
qualification gate or earlier coverage receipt was changed.

All 59 prerequisite holds remain open. Closing every other requirement would
reach at most 1,099/1,158 (94.9%); at least two holds need substantive resolution
for the 1,101 target. This upper bound is not a forecast that the other gaps are
straightforward. The requested prerequisite-path decision remains unanswered.

Core repairs requiring review
-----------------------------

Seven core paths changed during this work. The exact before/after identities,
before-source files and narrow diffs are retained in
``build/nasa-coverage/coverage95-20261101/save-resource-final-audit.json`` and
``save-resource-core-diffs/``. The first two paths were already in the original
104-path scrutiny list; the other five are explicitly added. The remaining 102
original core paths are unchanged from the sixth-increment baseline. All 181
Qt production C++/header paths present at the beginning of this work are
unchanged.

* ``plugins/SaveCommandPlugin/src/base/command/Save.cpp``: built-in coordinate
  exports now declare their reserved identity without assignments the
  interpreter rejects. Numerical propagators export their owned runtime force
  model once, including when that model is selected explicitly in either order.
  A detached propagator clone suppresses its embedded duplicate; its live
  serialization flag is preserved. Single-propagator and explicit-model cases
  check the complete captured writable/owned state after rebuilding.
* ``src/base/hardware/PowerSystem.cpp``: InitialEpoch's getter returns its string
  value without script quote delimiters. Default Gregorian, explicit Gregorian
  and ModJulian cases cover solar and nuclear systems. Internal epoch storage
  and conversion calculations are unchanged.
* ``plugins/EstimationPlugin/src/base/estimator/Estimator.cpp``: assigning the
  FromParticipants format preserves that symbolic epoch string instead of
  clearing it. The numerical epoch reset remains unchanged. The native Save
  case checks it after export/reload.
* ``src/base/subscriber/OrbitPlot.cpp`` and ``OrbitPlot.hpp``: omit the empty
  default DrawObject array when an orbit plot has no objects. The parser rejects
  the previous empty boolean assignment. Empty and populated OrbitView fixtures
  check reconstruction; nonempty flags still use the native writer.
* ``src/base/propagator/Integrator.cpp``: remove a duplicate parameter-ID offset
  in GetParameterTypeString. Accuracy, MaxStepAttempts and
  StopIfAccuracyIsViolated now report their existing Real/Integer/Boolean kinds.
  All nine integrator types undergo native getter/export/reload checks.
* ``plugins/EstimationPlugin/src/base/estimator/Simulator.cpp``: implement the
  existing ObjectArray descriptor's propagator-name getter. A configured
  Simulator, tracking set, spacecraft, station, propagator and force model
  export/reload together; 126 writable descriptors are checked in each mode.

These patches repair serialization and metadata access. They do not modify
force equations, numerical integration algorithms or observation calculations.
They are not a blanket approval of scientific behavior or the earlier pending
core changes. Built-in-frame exports depend on that runtime's immutable default
definitions; cross-version portability is unqualified.

Failure history and assertion preservation
------------------------------------------

Twenty-seven failed native attempts and one debugger diagnosis are retained.
Every failed registered case has passing replacements. Failures include the
seven product/API defects above, a test reading unattached power-system computed
outputs, an ambiguous Engine/Engine2 definition token, incomplete Simulator
prerequisites, an incorrect GroundTrack alias/property assumption, and assigning
step size before selecting a new integrator type. The independent 120-second
witness was retained by correcting assignment order.

All 33 original ``require(...)`` expressions in the new driver are byte-identical
to the first pilot. The earlier Save-attachment source and fixture are also
unchanged and were not replayed. Passing new unattended evidence is reused at
its recorded source/runtime versions. Eight successful affected checks were
repeated only because the early evidence did not bind newly repaired core
sources. The directly affected ResourceRoundTrips regression passed; its
repeated source-bound result is retained separately.

The independent audit initially rejected ElectricThruster's duplicate Isp label:
one native ID is a read-only computed output, another is a writable input.
The audit now retains both IDs and rejects duplicate writable labels. All 20
checker contracts pass, including this distinction; all ten corrupted evidence
capsules are rejected. Native assertions and comparison bounds were preserved.

Retained evidence and replay
----------------------------

The evidence index is ``save-resource-evidence.json``. Its audit verifies frozen
source archives, before/after source/runtime identities, exact JUnit results,
actual native proofs, raw exports, native snapshots and visible-window metadata.
The final audit and ``save-resource-final-sources.tar.gz`` bind the repairs and
checker version. Source archives contain no runtime ELF images.

The 45 first visible passes used a nested window with automatic scaling,
initially minimized, resizable and non-fullscreen. No passing screenshots or
runtime binary copies were retained. Lossless compression of 309 duplicate
reports/traces saved 102,985,728 allocated bytes (98.2 MiB). Essential raw native
proofs, failures, source versions, logs and JUnit remain protected. The new run
folders, including negative capsules and review diffs, occupy about 75.3 MiB.

From the repository root, use a fresh output directory for a watchable replay::

   python src/qtgui/tests/RunAcceptance.py \
     --build-dir build/linux-gui --filter '^QtGui.NasaSaveResource' \
     --visible --display-route nested --display-scale auto --start-minimized \
     --pause-ms 350 --max-checkpoints 200 --max-screenshots 0 --report-every 45 \
     --output build/gui-acceptance/save-resource-watch

The same registrations run unattended without the visible options. To check
retained evidence without launching GUI cases::

   python src/qtgui/tests/AuditSaveResourceEvidence.py \
     --evidence build/nasa-coverage/coverage95-20261101/save-resource-evidence.json
   ctest --test-dir build/linux-gui \
     -R '^QtAcceptance.SaveResourceEvidenceContract$' --output-on-failure

Remaining Save scope
--------------------

The retained creation catalogue has 83 offered names; 53 do not directly match a
native type name in these new fixtures. That is an inventory difference, not a
coverage percentage: some names are aliases, some are built-in functions, and
older Save evidence covers additional parameter/chemical-hardware cases with
its original narrower property contract. Alias equivalence requires proof.

Next work should cover missing configured resource families and their native
persistent/reference contracts: functions, remaining hardware/FOV resources,
estimation support resources, event locators, filters, scientific-file outputs,
user-body categories, default celestial bodies and SolarSystem. Formation,
multi-propagator satellite mappings, runtime-updated reference graphs and other
attachment combinations also remain open. Read-only/computed runtime outputs
are recorded, not asserted as persistent configuration. Unsupported getter
kinds remain explicit failures; they are not skipped into passing evidence.

FRC!E135's historical ``ObjectName.ObjectType`` default-filename wording remains
separate from the current ``.data`` convention. Neither its assertion nor naming
behavior was changed. Other original requirements and historical gaps, private
NASA corpus provenance, cross-platform behavior and full current-build/science
qualification remain open. The 95% goal is active.
