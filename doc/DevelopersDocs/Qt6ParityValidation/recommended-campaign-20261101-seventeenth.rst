Recommended campaign: numeric unit meanings and solar labels
============================================================

This increment reviews the original 157 numeric N/A/None records and repairs a
five-label native metadata defect. It does not qualify the full All Others!E12
all-input documentation requirement. Coverage remains 923/1,158 requirements
(79.7%) and 1,816/2,367 historical scopes (76.7%); recovery dispositions remain
158 accepted/59 held, and local contracts remain 36 verified/one missing.

Numeric documentation
---------------------

The 157 records map to105 unique guide rows. Ninety-five Units entries across 24
pages now explicitly describe146 measured records. These distinguish integer
counts/orders and identifiers, dimensionless ratios/exponents/directions,
parameter-dependent uncertainty, model-coordinate conventions and physical
quantities. Tank hardware origins are metres, RectangularFOV angle width is
degrees and MaxStep is seconds. Defaults, allowed values, formulas, examples,
identifiers and all other original source bytes remain unchanged for those24
pages; the byte-restoration verifier proves that only the declared Units text
changed. No existing GUI assertion or case recipe was weakened.

Ten guide rows representing 11 measured records remain held: BatchEstimator and
BatchEstimatorInv OLSEAdditiveConstant; Propagator Accuracy, LowerError and
TargetError; VF13ad FeasibilityTolerance/Tolerance; and Yukon
FeasibilityTolerance, FunctionTolerance, MaximumElasticWeight and
OptimalityTolerance. Their selected error/cost/constraint scaling still needs
review. A N/A/None entry is not automatically declared dimensionless.

Solar metadata repair
---------------------

GetSunToSCDistance computes the numerical distance ratio D/(one AU), and
GetPowerGenerated uses it in a scale factor multiplying separate base power.
The five SolarCoeff metadata labels previously returned the unresolved kW?
string. They now report dimensionless, with an explicit AU-normalized convention
and formula in the user guide. Coefficients must be fitted with this distance
normalization; no numerical value is converted or rescaled by the label change.
This is a source-derived input convention, not independent solar-power physics
qualification or NASA technical approval.

A new native metadata/data test failed before the fix and passes afterward
with identical test source. It checks all five labels and parameter identities,
unchanged defaults, read/write and clone values, inherited InitialMaxPower kW
units and the default coefficient data's normalization at r=1. It does not run
a physical power mission. One affected-resource metadata/editor-construction
probe confirms all five current dimensionless GUI-property strings and identical
other default field metadata. The old raw kW? records remain preserved, with
explicit current replacement evidence. The receipt binds 33 runtime/source
identities, including 26 available project libraries; it does not measure the
complete loaded-library set or qualify native user exposure.

SolarPowerSystem.cpp is the only new core file changed. Its unit-label
method changes and a terminal newline is added. Every other byte, including all
numerical power calculations, remains unchanged. The total HEAD-versus-worktree core inventory is now100
files, versus99 previously. The new narrow patch and all prior core changes
retain their separate technical-scrutiny obligation. TimeSystemConverter.cpp,
MinQP.cpp and all production Qt C++ retain their pre-increment hashes.

Editing failure and verification
--------------------------------

The first batch editor applied original text offsets in insertion order. A
Thruster row became malformed, and schema validation rejected the source before
installation. The preserved originals were restored, the edits were reapplied
with original-offset ordering, and every changed page was checked for exact
restoration outside Units text. The failed edit and diagnostic logs remain
separate; they are not treated as passing evidence.

A reusable text-edit helper now handles unordered length-changing edits and
rejects overlaps, duplicates and invalid spans. Five regression checks pass
through the registered support CTest case. The solar metadata/data CTest passes
separately. Only these new support checks and one affected resource probe ran;
no old GUI scenario or mission was replayed. Existing passing evidence is reused.

The DocBook schema and HTML build pass after the editing repair. All 100 changed
field-unit descriptions were verified inside their own generated table rows.
All 5,653 local page/anchor links across 275 reachable guide pages pass. Changed
HTML was installed together, with originals compressed and unchanged HTML
reused. No screenshot, image asset or runtime binary copy was retained.

What remains
------------

All 987 original measured identities remain visible. Active-guide routes remain
984, with the previous three corrupt/nonrendered metadata dispositions retained.
The 146 numeric conventions are now source/guide reviewed, and five stale solar
unit strings have explicit current replacements. This is not full-field or
NASA coverage credit. The 11 numeric records above, other dependent descriptions,
native/guide unit agreement, nonnumeric field meaning and concrete nested-mode
and dynamic-template expansion still need the complete E12 audit. Independent
scientific, platform/GPU, whole-current-build and broader authority-dependent
qualification remain separately unqualified.

Under build/nasa-coverage/recommended-campaign-20261101/:

* numeric-unit-row-review.json and numeric-unit-text-preservation.json retain
  the 95 exact edits,146 reviewed records,11 held records and source preservation.
* numeric-unit-semantic-dispositions.json reconciles the complete157-record
  numeric worklist and the five explicit current solar-label replacements.
* solar-units-before/after.json, logs and JUnit files retain the real failure
  and repair with unchanged test source; solar-unit-metadata/ retains the new
  affected resource metadata and bound runtime/source identities.
* seventeenth-core-change-review.json and seventeenth-core-SolarPowerSystem.patch
  isolate the metadata-only core edit and preserve the 100-file inventory.
* seventeenth-unit-render-review.json records100 field rows, whole-guide links
  and the changed/unchanged HTML deployment identities.
* seventeenth-documentation-progress.json preserves the remaining obligations.
* seventeenth-increment-audit.json and seventeenth-increment-sources.tar.gz
  preserve this increment's audit and frozen text sources.

The original campaign, handoff, gaps, raw census and sealed source-versioned
receipts remain preserved. The goal remains active and incomplete.
