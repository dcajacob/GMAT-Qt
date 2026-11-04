95 percent campaign: owned force configuration persistence
===========================================================

Four new cases pass unattended and visibly: gravity, radiation pressure,
Jacchia-Roberts drag and a combined gravity/radiation/NRLMSISE00/relativity
configuration. All 115 preceding native state/source cases are reused at their
recorded versions. Combined bounded Save evidence has **119 cases, 238 contexts
and 5,584 writable-property comparisons**, plus explicit native owned-field
literal checks before and after export/rebuild.

Coverage remains **998/1,158 requirements (86.2%)**, with 103 complete requirements
needed for 95%. Historical coverage remains **1,816/2,367 (76.7%)**. These owned
configurations add no complete NASA row. FRC!E132 still needs the complete state,
mode and reference-graph obligation; original authorities, denominators,
fingerprints and qualification gates remain intact.

Native owned-field checks
--------------------------

The driver resolves each declared owned type path through GetOwnedObjectCount
and GetOwnedObject, requiring a unique child at every step. It then uses that
child's typed native getter. The independent audit follows the captured native
ownership tree separately; generated script and GUI property mappings never
supply state truth. Missing/ambiguous paths, wrong values and root substitutions
are rejected. Root-field checks retain their existing behavior.

The fixtures check gravity Degree=4/Order=2, radiation Flux=1400 and
Nominal_Sun=150000000, and drag F107=160/F107A=155/MagneticIndex=4 with the
prescribed atmosphere model. Both original and rebuilt force component sets are
checked. The existing JGM2 coefficient input is hash-bound without a data copy.
No propagation occurs, so these results do not independently validate physical
gravity, radiation, atmospheric, relativistic or integration calculations.

Comparison correction and retained failures
--------------------------------------------

Two failed native attempts remain retained. The first drag fixture omitted the
required primary body; its native log explicitly rejects drag on Earth without
Earth as a primary body. The corrected fixture supplies that existing precondition.
The two initially passing gravity/radiation cases were reused unattended.

The combined case exposed a harness association error. Canonical Save output
rebuilt the same named force objects in a different internal enumeration order:
drag and relativity exchanged positions. The old ordinal pairing compared those
different objects and reported type/field gaps. Force parents now pair each
unique (native type, native name) identity exactly once. Every field and metadata
entry remains checked. Missing, duplicate or changed force identities fail;
ordered property arrays and every non-force owned collection retain their
original ordered checks. Raw captures retain both actual enumeration orders.
Exact trajectory/bitwise equivalence after internal force reordering is outside
this persistence increment.

All 46 preceding balanced native require expressions remain in order, including
the original value, metadata, source, reload and continuation assertions. One
additional guard rejects missing/ambiguous owned witness paths. No expected
value, numeric tolerance, property scope or ordered property-array check is
removed. The comparison now associates the same native objects across reload.

One narrow core metadata repair
--------------------------------

The audit found a native type-label defect in
``src/base/forcemodel/RelativisticCorrection.cpp``. GetParameterTypeString returned
the field labels Radius/Mu instead of their declared Real type. The repair returns
the existing parameter-type label. Actual native getters now observe Radius and
Mu as finite real values. Equations, constants, setters, force calculations and
mutability remain unchanged. Covariance/DerivativeID read-only getter limitations
remain visible in the raw captures and unqualified; they are not substituted for
writable settings.

The earlier combined settings passes lacked those valid type/getter observations
and are explicitly superseded. Only the combined case was rerun in each mode.
There are 12 new acceptance attempts: two failures, two superseded passes and
eight selected passing contexts. No preceding 115-case native replay occurred.
All failures have passing replacements at recorded versions.

Relativity is one additional core review path, bringing the explicit Save list
to **19 paths**. All preceding 18 paths stay unchanged in this increment. No Qt
production path changes. The exact one-line patch, source backup and identities
are in ``save-forces-core-diffs/`` and ``save-forces-final-audit.json`` under
``build/nasa-coverage/coverage95-20261101/``. Scientific/technical and complete
current-build approval remain false.

Independent audit, storage and commands
----------------------------------------

All 53 checker contracts pass: the previous 41 plus seven owned-path and five
force-identity cases. They include missing/ambiguous paths, changed values,
force reordering, duplicate/missing identities, changed reordered values and
continued ordered checks for non-force collections. Eleven corrupt conjunction
capsules are rejected after a valid baseline, including failed/invalid old
receipts, wrong owned-field literals, missing physical-input identity and an
older combined pass with invalid relativity type labels.

Visible cases used automatic scaling in initially minimized, resizable,
non-fullscreen nested windows. No passing screenshots or runtime binary copies
are retained. Lossless compaction of 49 duplicate reports/traces saved 32,575,488
allocated bytes (31.1 MiB), preserving all admission artifacts.

Audit the retained conjunction without native replay::

   python src/qtgui/tests/AuditSaveForceEvidence.py \
     --evidence build/nasa-coverage/coverage95-20261101/save-forces-evidence.json
   ctest --test-dir build/linux-gui \
     -R '^QtAcceptance.SaveResourceEvidenceContract$' --output-on-failure

To watch the four new cases, choose a fresh output directory::

   python src/qtgui/tests/RunAcceptance.py \
     --build-dir build/linux-gui --filter '^QtGui.NasaSaveForce' \
     --visible --display-route nested --display-scale auto --start-minimized \
     --pause-ms 350 --max-checkpoints 200 --max-screenshots 0 --report-every 4 \
     --output build/gui-acceptance/save-forces-watch

Omit visible/display flags for unattended operation. Frozen older receipts keep
their original source/runtime scope; they are not latest-build qualification.

Remaining scope and source reconciliation
------------------------------------------

Other owned-force/attitude modes, spacecraft representations/file inputs, hidden
persistent fields and valid reference graphs still need complete conjunction.
The new ``save-forces-descriptor-inventory.*`` derives the expanded native field
inventory without overwriting the preceding sealed 115-case inventory. Read-only
flags remain review evidence, not automatic proof of nonpersistent state.
Earlier retired-editor observations, scientific output, unsupported getter,
external dependency and FRC!E135 filename gaps remain explicit.

Two evidence-only recovery reviews avoided unnecessary native runs. RMAG is
currently settable in native metadata and has an actual setter, conflicting with
the original FRR!E519 read-only clause. The original user-body orientation list
asks separately for rotation rate at reference epoch and rotation drift rate;
the shipped guide defines RotationConstant as an angle, so treating it as the
fifth required rate would change the original obligation. FRR!E221/E227 remain
open without an authoritative reconciliation. Exact rows/source identities are
recorded in ``rmag-readonly-source-reconciliation.json`` and
``orientation-rate-source-reconciliation.json``. No existing setter/model is
removed and no new row credit is inferred from current-model replay.

All 59 original prerequisite holds remain unresolved. The optimistic
1,099/1,158 (94.9%) ceiling assumes all other clauses can close; source conflicts
may prevent that. At least two original holds need substantive resolution for
the 1,101 target. No unanswered decision is assumed. Full private NASA,
platform-wide, scientific and current-build qualification stay open.
