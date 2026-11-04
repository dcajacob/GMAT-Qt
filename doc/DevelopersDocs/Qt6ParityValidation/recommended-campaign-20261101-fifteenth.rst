Recommended campaign: input-unit repairs and documentation routes
=================================================================

This increment fixes two reproduced defects and a targeted rendering regression,
then advances the remaining all-input documentation review. It does not qualify
All Others!E12. Coverage remains 923/1,158 requirements (79.7%) and 1,816/2,367
historical scopes (76.7%). All 217 recovery decisions remain unchanged:158
accepted and59 held. Local contracts remain36 verified and one missing.
The full-current-build, full-NASA and technical-core-approval gates remain false.

Repairs and their actual evidence
---------------------------------

Hardware sensor origins are stored in metres, while plot positions and lengths
are in kilometres. The Qt receiver now tags native hardware offsets and the
geometry boundary converts them before body rotation and view translation.
Direct programmatic plot geometry retains its existing kilometre convention.
A new native test first failed on the unconverted offset, then passed with the
same assertions. It checks actual receiver freeze, unchanged configured hardware,
a sentinel mission value, an independent identity-pose result and a prescribed
90-degree body rotation. The repaired test passes without retaining screenshots.

Only the two explicitly affected existing sensor-mask tests were replayed.
Backend passed; Rendering initially crashed while constructing a standalone
canvas before the engine's leap-second reader was initialized. A debugger stack
identified the path through UTC caption formatting. The Qt caption helper now
returns its existing unavailable text until the engine is ready. Core time-scale
algorithms are unchanged. A new lifecycle test checks the pre-initialization
fallback and the initialized Orbit/Ground J2000 UTC literal and XY exclusion.
Both affected existing tests subsequently pass, with their source assertions
unchanged. These are offscreen checks; they do not establish host-GPU behavior.

The metadata review also found an out-of-bounds MessageWindow name lookup and an
incorrect type offset. A new direct test failed before the repair and passes
with the identical test source afterward. The production repair changes exactly
two lines in src/base/subscriber/MessageWindow.cpp. It preserves field IDs,
integer precision type, default10, positive17 and nonpositive-value behavior.
One affected-resource metadata probe confirms Precision/Integer and its actual
resource_Precision control. It replaces no old census record and runs no mission.

Core scrutiny
-------------

MessageWindow.cpp is the only new core file changed in this increment. Its
separate before/after source and two-line patch are preserved. The total HEAD
versus worktree core-file inventory is now99, versus98 previously. All previously
changed core sources retain their pre-increment hashes, including
TimeSystemConverter.cpp and MinQP.cpp. Passing metadata tests grant no general
scientific or technical approval to those99 files. The Qt receiver, geometry,
model unit tag and caption readiness guard are separately identified frontend
changes. No previous GUI assertion or case recipe was weakened.

Documentation progress
----------------------

The structured resolver now separates individual DocBook labels and removes
index terms before matching. Eight new parser checks cover multiple labels,
case variants, index-term contamination, foreign owners/prefix collisions,
missing Units entries, explicit inherited routes, missing targets, duplicates
and preservation of unmatched records. Routes remain candidates for semantic
review and always confer no qualification keys.

Explicit owner/field rules bind inherited FunctionPath, radio geometry,
SpacePoint identifiers, ReportFile formatting and Estimator/Solver fields, plus
reviewed label aliases. All987 original measured type/field/unit identities
remain present. Unit-bearing documentation routes increase from865 to983; the
four remaining records each have a substantive disposition:

* String.Object and Variable.Object are metadata, while their actual scalar
  editors render parameterValue instead. Both original records remain visible.
* MessageWindow.Dec is the preserved corrupt pre-repair lookup. The separate
  current probe reports Precision as an integer and binds it to digit-count
  documentation; no historical record is silently renamed.
* Transmitter.FrequencyBiasSigma has no reviewed guide unit entry. The source
  polynomial suggests MHz/s^k coefficient/sigma units. An indexed getter also
  appears to return the coefficient vector after checking sigma-vector bounds,
  whereas the whole-vector getter used by Qt returns sigmas. This is a
  source-inspected candidate defect, not a runtime-verified failure or a repair.
  Transmitter core code remains unchanged.

Nineteen existing unit descriptions were corrected: metre origins and
nondimensional direction-vector components in Imager/Antenna, and ReportFile's
formatting-digit count. A new shipped Qt Input Conventions topic documents nine
inherited-control conventions, including counts, filenames, resource references,
representation selectors and MessageWindow precision. The DocBook schema and
HTML build pass. All19 corrections and nine new table rows were verified in
their own generated rows. Ten HTML files, including navigation and the single-page
guide, were installed; replaced originals are compressed. No image was copied.

Limits and next work
--------------------

983/987 is a field-documentation routing count, not NASA acceptance coverage or
proof of correct units. Dependent-unit narratives and ambiguous/native unit
strings still require semantic review. The296 compiled input-construction
sites remain templates requiring individual bindings. Nested axes, attitude,
propagator, force/atmosphere/provider modes, configured built-ins, commands and
expressions, conditional combinations and standard dialogs remain explicit
completeness obligations. The14 native selector rejections remain in the original
census. None is silently reclassified as a GUI acceptance pass.

The new resource-only metadata receipt binds the executable, selected source and
application-bin libraries. It is not a full loaded-plugin/source-to-binary seal.
The targeted checks retain exact per-run source/library identities, including
their different before/after core versions. Earlier passes are not relabelled as
newest-binary qualification, and no unrelated existing suite is replayed.

Reviewable artifacts
--------------------

Under build/nasa-coverage/recommended-campaign-20261101/:

* fifteenth-increment-audit.json and fifteenth-increment-sources.tar.gz preserve
  this increment's audit and frozen text sources.
* fifteenth-core-change-review.json and fifteenth-core-MessageWindow.patch
  isolate the new core repair and retain the full99-path inventory.
* sensor-mask-units/ contains the actual before-fix failure, final unit proof,
  lifecycle pass and affected rendering/backend results, including the resolved
  crash stack. Early fixture/build setup failures remain separate from defects.
* message-window-before/after.json and their logs/JUnit files bind the exact
  unchanged test to the reproduced failure and repair.
* resolved-input-documentation.json and remaining-input-field-dispositions.json
  preserve every field and route or explicit unresolved disposition.
* inherited-input-units-documentation-review.json and
  inherited-input-units-render-review.json describe the actual shipped doc edits.
* fifteenth-documentation-progress.json retains the remaining E12 obligations.

The original campaign, handoff, gaps, recovery decisions and sealed evidence
remain unchanged. Goal completion remains unproven until the all-input contract
and the final requirement-by-requirement handoff audit are satisfied.
