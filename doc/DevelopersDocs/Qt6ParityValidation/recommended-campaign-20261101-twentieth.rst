Recommended campaign: configured spacecraft input inventory
===========================================================

The five spacecraft selector families missing from the original census now
have actual specialist-editor construction evidence. Eleven new inventory
processes recorded315 configured states, with zero rejected states, process
failures or native-metadata errors. They expose72 distinct field/unit pairs,
of which48 were absent from the retained987-record census. All72 now have
reviewed active-guide unit descriptions. These are new inventory observations,
not315 GUI acceptance passes or a claim of complete E12 coverage.

Coverage remains923/1,158 requirements (79.7%) and1,816/2,367 historical scopes
(76.7%). Recovery remains158 accepted/59 held; local contracts remain36 verified
and one open. The original requirement/historical denominators and gaps remain
unchanged. All Others!E12 still needs completeness across the other dynamic
editor families before the final local contract can be credited.

What was measured
-----------------

The probe uses the actual ResourceEditor and AttitudeDialog constructors, not
an imitation form. It creates unregistered spacecraft drafts and verifies that
no configured spacecraft was added. It never runs a mission or shows a window.
Qt settings are isolated with the existing TestSettings helper. Application
libraries and data are used in place; only startup text fixtures and compact,
compressed census records are retained.

The runtime factory list supplies all eight attitude models. Spinner,
ThreeAxisKinematic and CommandableNadirPointing each permit initial-state edits
and are inventoried over four orientation representations, two rate
representations and all twelve Euler sequences:96 states each. The five other
models each expose one applicable model-settings form. A nonsingular initial
orientation permits all Euler sequences without creating a singularity merely
as an inventory fixture. The source read-only logic and actual control sets
confirm the selected orientation and rate components; no alternative components
are silently substituted. This produces293 attitude configurations.

Twelve additional forms cover Keplerian and ModifiedKeplerian representations
for elliptic TA/MA/EA and hyperbolic TA/MA/HA cases. The actual selector values,
sixth-element labels, offered anomalies and measured eccentricity regime are
checked. Ten more forms cover every runtime-offered spacecraft date format,
including the actual epoch entry and date-format selector. This inventories
controls and units; it does not rerun orbital-conversion or time-system truth
qualification.

Each process records110-113 actually loaded filesystem images and their hashes,
including24 project images. The execution receipt also binds235 selected source
and runtime identities before/after execution. This improves upon the older
exploratory census's available-library-only seal; it does not retroactively
upgrade that old receipt or establish whole-build source-to-binary qualification.
No loaded library, application asset or runtime directory was copied.

Documentation repairs
---------------------

The first route audit found three missing direct field rows: EA, MA and HA.
New rows document their selected sixth-element role and degree-scaled numerical
convention, including the distinction between hyperbolic anomaly and a geometric
angle. They describe the measured GUI interface only. A draft GUI/script claim
was narrowed before installation because native setter review did not establish
that broader interface claim.

Twenty-one attitude Units entries now explicitly describe dimensionless body
alignment/constraint/spin/nutation-reference vector components and direction
cosines. Existing quaternion/MRP, angle, angular-rate, orbit-length/velocity,
selector and epoch conventions are checked separately. A blank GUI label alone
never establishes dimensionless input.

Every original source byte outside these21 Units edits and the three added
rows is preserved, including existing descriptions, examples, defaults, allowed
values and identifiers. No core or production Qt C++ changed. All existing
assertions and case recipes are unchanged. The existing100-file core inventory
remains separately unapproved for technical scrutiny, with protected
TimeSystemConverter.cpp and MinQP.cpp hashes preserved.

Verification and reuse
----------------------

The new GmatQtConfiguredInputCatalog target builds successfully. The permanent
AuditConfiguredInputDocumentation tool checks raw receipt/log/startup/library
integrity, the complete applicable model/representation/rate/sequence products,
actual control sets and all72 source/guide unit meanings. The permanent runner's
reuse command passes without reconstructing any editor. Its startup generator
also exactly reproduces all eleven executed fixtures when given their template.
The default application template differs only in the legacy personalization-file
setting; the probe uses isolated Qt settings. The executed recording recipe and
its frozen source remain available alongside the reusable command.

The initial verifier rejected capitalized Days because its keyword check was
case-sensitive; that diagnostic check was corrected without changing the epoch
unit requirement. The first new-row HTML comparison also rejected equivalent
whitespace between source and generated blocks. The final check compares each
field description and every metadata term/value structurally, including Units
and the GUI-only interface. Neither was a product/scientific test failure, and
no existing GUI assertion was weakened.

The final DocBook schema and HTML build pass after the draft interface
correction. All186 checked field-unit rows pass, including162 retained rows,
21 clarified attitude rows and the three complete new anomaly rows. All5,643
local page/fragment links across275 reachable help pages pass. The121 changed
HTML files were installed together, with compressed originals. No images,
screenshots or binaries were copied. The343 construction-template bindings
retain their source keys and policy text; four reviewed reference hashes were
refreshed.

To reuse this evidence without replaying editor constructions, run from the
repository root::

  python src/qtgui/tests/RunConfiguredInputInventory.py \
    --reuse \
    --census build/nasa-coverage/recommended-campaign-20261101/configured-input-census \
    --review-output /tmp/gmat-configured-input-review.json

For a new recording, build GmatQtConfiguredInputCatalog and run the same script
without --reuse, using a new --census directory. The optional --startup and
--build-dir arguments select the runtime template and build. Recording refuses
an existing directory and retains rejected configurations rather than silently
dropping them. This utility is an unshown input inventory, not a replacement for
the existing visible GUI acceptance runner.

Remaining work and artifacts
----------------------------

The original987 identities remain intact, with the additional48 field/unit
pairs explicitly distinguished. That1035-record union is source-versioned
inventory evidence, not an assertion that every current GUI input is counted.
Other specialist/dynamic constructions, configured axes/propagators,
force/atmosphere/provider branches and built-in-object differences still need
concrete field/unit completeness review. Then complete the whole E12 and final
campaign handoff audit. Broader scientific, authority-dependent, platform/GPU
and whole-current-build qualification remain outside this bounded campaign and
explicitly unqualified.

Artifacts under build/nasa-coverage/recommended-campaign-20261101/ include:

* configured-input-census/ retains the eleven compressed raw outputs, logs,
  startup text fixtures and source/runtime/loaded-image receipt.
* twentieth-configured-input-documentation.json and
  configured-input-unit-meaning-review.json retain72 actual-control routes and
  source/guide meaning checks.
* configured-spacecraft-unit-doc-review.json and twentieth-doc-preservation.json
  retain the exact documentation changes and restoration proof.
* configured-input-reuse-audit.json and configured-inventory-runner-review.json
  retain the no-replay reuse result and eleven exact startup-fixture checks.
* twentieth-unit-render-review.json records186 rows, whole-guide links and
  installed HTML identities.
* twentieth-input-completeness-progress.json distinguishes the completed
  spacecraft families from the remaining dynamic-input obligations.
* twentieth-increment-audit.json and twentieth-increment-sources.tar.gz retain
  the final audit and frozen text sources.

The goal remains active and incomplete; no new NASA numerator is claimed.
