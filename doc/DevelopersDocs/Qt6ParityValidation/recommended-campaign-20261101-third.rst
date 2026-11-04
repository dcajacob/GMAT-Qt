Recommended acceptance campaign: comparators and retained GUI evidence
======================================================================

The audited union now contains **867/1,158 requirements (74.9%)** and
**1,804/2,367 historical operations (76.2%)**. This increment adds six newly
verified test-harness capabilities and recovers 26 requirement rows from
complete retained evidence. No GMAT GUI case was replayed and no production
GUI or core source changed. This remains a source-versioned evidence union,
not a single current-binary regression or formal NASA qualification.

The authoritative campaign index is
``build/nasa-coverage/recommended-campaign-20261101/campaign.json``. Earlier
increment reports and sealed receipts remain unchanged.

Progress and remaining obligations
----------------------------------

* Recovery: 117 of 217 candidates reviewed; 99 recovered, 18 held for missing
  obligations, and 100 still awaiting review. The recovered total includes
  98 requirements and one historical scope.
* Local contracts: 27 of 37 verified. Six VF13ad progress/report and four
  units/documentation rows remain pending.
* Original denominators, deferred projects, scientific/core review and all
  wider qualification gates remain visible. No rows were retired to raise
  the percentage.

New reusable file comparisons
------------------------------

``src/qtgui/tests/AcceptanceCompare.py`` provides explicit comparator dispatch,
JSON evidence and standalone HTML. It preserves existing per-fixture assertions
and tolerances. It does not change the Qt GUI's existing comparison commands.

* ``file`` compares complete byte contents, including BOM, newline and binary
  differences. Input hashes identify both files. A text diff preview is bounded
  to 200 lines and explicitly marked if truncated; comparison itself is complete.
* ``numeric`` compares all numeric whitespace or CSV fields and reports signed
  actual-minus-truth differences. Nonnumeric identities, missing/extra fields
  and records remain differences; nonfinite data is an error. Numeric CSV
  columns are numbered from one, including preceding identity columns.
* ``position-velocity`` compares matched, strictly increasing numeric epochs
  and all six state components. Both files must declare identical frame,
  time-system, time-unit, position-unit and velocity-unit metadata. There is no
  implicit resampling or coordinate/time/unit conversion.
* ``oem`` handles the GMAT KVN OEM1.0/2.0 writer subset: segment identities,
  calendar epochs, all six state fields, optional OEM2.0 acceleration, and every
  entry of each six-by-six lower-triangle covariance matrix. Segment, record,
  component, covariance-frame and epoch mismatches fail. Invalid structure,
  unsupported versions/XML, nonfinite data and out-of-span records are errors.
  Calendar clock labels are compared without an inferred time-scale conversion
  or leap-table decision. Optional UTC ``Z`` and fractional-zero formatting are
  equivalent. CREATION_DATE/ORIGINATOR are explicitly listed as ignored unless
  ``--strict-header`` is supplied. This is not general CCSDS certification.
* Custom comparators register through an explicitly selected Python module.
  Duplicate builtins, unknown dispatch, empty results, inconsistent tolerance
  flags, malformed/nonserializable records and callback exceptions cannot pass.

For each numeric field, the inclusive acceptance boundary is
``abs(actual-truth) <= absolute + relative*abs(truth)``. Decimal arithmetic
preserves small differences at large origins and widely separated tolerance
terms. Per-field policies override only the named absolute/relative values.
These policies belong to the explicitly requested comparison; metadata does
not override the existing GMAT acceptance fixtures.

HTML export includes input hashes, policy, every compared numeric field and
difference, identity/structure differences and errors. Content is escaped,
assets are embedded, and no external script/network resource is required.
Recorded catalogue HTML preserves actual failure/run/reuse counts and unknown
metadata; exporting it performs no new test execution. Both output paths are
checked against all input paths, including symlink/hardlink aliases, before
writing either report. Each output uses atomic replacement.

Qualification and practical use
--------------------------------

``QtAcceptance.ComparatorContracts`` passes 24 independent tests, including
literal expected deltas, exact tolerance boundaries, long precision, Unicode,
actual CLI exit codes/JSON/HTML, input protection, plugin dispatch and errors,
OEM acceleration/covariance/multiple segments, and malformed-file rejection.
The exact emitted proofs qualify only All Others!E303/E304/E305/E307/E309/E330
in ``NasaComparatorTraceability.json``. They are original test-system rows,
not newly exposed GMAT widgets or independently certified orbital truth.

The source-bound receipt, log/JUnit, source archive and six-row qualification
are under ``comparator-contract-qualified`` in the campaign directory. An
independent headless Chrome render of ``example-comparison.html`` was visually
inspected at 1440 by 1100. No desktop window opened; its temporary profile and
passing screenshot were removed after inspection. The retained example is
explicit literal comparator input, not a GMAT mission result.

Execute just the new registered backend contract::

   ctest --test-dir build/linux-gui -C Release \
     -R '^QtAcceptance[.]ComparatorContracts$' --output-on-failure

Compare files without running GMAT::

   python3 src/qtgui/tests/AcceptanceCompare.py \
     --actual actual.txt --truth truth.txt --kind numeric \
     --absolute 1e-8 --relative 1e-10 --json comparison.json --html comparison.html

The exit status is zero for pass, one for differences and two for errors.
For per-field policies, supply ``--policy policy.json`` with this structure::

   {"absolute":"1e-8","relative":"1e-10",
    "fields":{"X":{"absolute":"1e-6"},"VX":{"absolute":"1e-9"}}}

Position/velocity files use this explicit header and numeric record layout::

   # FRAME=EarthMJ2000Eq
   # TIME_SYSTEM=A1
   # TIME_UNIT=day
   # POSITION_UNIT=km
   # VELOCITY_UNIT=km/s
   epoch x y z vx vy vz
   21545 7000 0 0 0 7.5 0

A custom module defines ``register_comparators(registry)`` and calls
``registry.register(name, callback)``. The callback receives actual bytes,
truth bytes, a report object and the chosen tolerance policy. It appends
complete numeric records, for example using ``numeric_cell``, or structural
differences using ``mismatch``. Select it with ``--custom-module module.py``
and ``--kind name``. The module is explicit trusted user code.

Export a recorded catalogue without replay::

   python3 src/qtgui/tests/AcceptanceCompare.py \
     --catalogue recorded-catalogue.json --html recorded-catalogue.html

Retained GUI rows recovered this increment
-------------------------------------------

Each accepted row has a separate semantic decision and an automated check of
the complete original source/reference hashes, registered command, executable
identity, exact passing log/JUnit, snapshot and mandatory execution cohorts.
No assertion, receipt or source mapping was weakened to reuse it.

* FRR!E376/E379/E392/E394/E398/E405: actual SPK output, Cartesian states,
  EarthMJ2000Eq, user odd Hermite order, user-provided intervals and optional
  ephemeris writing. Native circular-state readback and explicit enabled/disabled
  results remain mandatory, together with configuration/history/persistence.
* All Others!E114/E115/E116: About build date, exact shipped NOSA license access
  and contributor credits, including missing/malformed license correction.
* FRR!E84/E91/E92/E95/E96/E97/E98/E99/E100/E101: thruster mass switch, independent
  VNB/LVLH velocity results, Cartesian vectors, impulsive mass switch and the
  complete gravity/Isp/tank depletion-option conjunction. Original literal
  constant-flow and rocket-equation checks remain unchanged.
* All Others!E195/E197/E199/E200/E201/E203: every named branch kind for expand/
  collapse and descendant deletion, actual tree Open/double-click and insertion
  before/after, with exact literal source, pending/history and snapshot checks.
* FRC!E92: actual indexed Achieve tolerance selection to1e-7, persisted command,
  independently known goal4, full original report and invalid-index recovery.

One additional held row
------------------------

FRR!E375 expressly requires CCSDS502.0-B-1 output. Its retained GUI case proves
configuration, recognizable header, accessible output and analytic native
readback, but not every required file-standard metadata/structure/time/units
obligation. Raw temporary OEM output was not retained. It remains open pending
a writer-to-independent-validator check with small retained output. The new
generic OEM comparator does not silently close this production-writer row.
The preceding seventeen held rows remain open as previously documented.

Failures, preservation and storage
-----------------------------------

There were no new GMAT production test failures because no GMAT test executed.
During development, one newly written fixture mutation accidentally removed
part of a covariance row while trying to remove acceleration; it correctly
produced an input error. The fixture was corrected to remove state acceleration
only, preserving its intended failed-comparison assertion. Initial review-file
generation also selected four other-sheet rows by numeric suffix. Qualification
refused the incomplete ten-row claim; the manifest now selects the six exact
All Others source keys and qualifies all six. The failed attempt is excluded
from campaign credit. Chrome's first sandboxed render could not create its
Crashpad socket; the successful headless render used an approved sandbox
exception. These were tooling/development issues, not weakened GMAT assertions.

All 97 previously changed core files still match the core-first audit index.
The original GUI assertions/recipes and production core/plugin/util sources
remain unchanged from the recorded pre-campaign baseline. The only CMake change
in this increment is one backend test registration. New reusable Python tools,
tests, metadata and review manifest are separately inspectable. Existing broad
worktree differences predate this increment and are not claimed as these changes.

This increment creates no runtime/binary snapshot or retained passing screenshot.
The complete campaign directory is about14MiB, including the earlier increments,
source archives, original-sized coverage reports and the excluded development
attempt. The active 217+37 campaign remains unfinished; the next work is the
remaining recovery audits and ten local contracts, with explicit dispositions
for every rejected obligation.
