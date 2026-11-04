Recommended campaign: dynamic input domains and final documentation inventory
=============================================================================

This increment records11 new specialist scenes and177 actual controls, with
zero rejected scenes or process failures. Their unit audit passes, including33
native field rows. It also records concrete or parametric field/unit rules for
all34 compiled control families and checks their343 construction keys against56
source files. These are input-documentation observations and source reviews,
not177 new GUI acceptance passes or a blanket E12 qualification decision.

Coverage remains923/1,158 requirements (79.7%) and1,816/2,367 historical scopes
(76.7%). Recovery dispositions remain158 accepted/59 held; local contracts remain
36 verified and one open. The remaining gate is the consolidated All Others!E12
evidence audit followed by the bounded campaign handoff audit. The full original
scope, rejected/held candidates and broader qualification gaps remain visible.

New concrete evidence
---------------------

A single new unshown inventory mode constructs Earth, Luna, Mars and Sun body
panels; inactive/active atmosphere forms; Earth gravity; external-force settings;
and ordinary, numeric-literal and function-input parameter pickers. It does not
Apply settings, run a mission or show a window. No configured spacecraft is
added. Application libraries/data are reused in place, and actual loaded file
images are hash-bound. Startup text, compressed records and logs are retained;
no screenshot or runtime binary/data copy is created.

The body source declares the same field families with explicit built-in/kernel
and read-only branches. Earth alone adds NutationUpdateInterval, whose existing
guide convention correctly states seconds. Pole rates, body/model quantities,
A1 orientation epochs, identifiers and file/reference fields retain their
separate conventions. Thirty native body-field identities and three atmosphere
numeric fields receive exact active-guide checks.

The atmosphere form always constructs the same13 controls; model/source changes
toggle availability. F107/F107A labels are sfu, matching10^-22 W/m^2/Hz.
MagneticIndex is the dimensionless Kp input in0..9; model-specific Kp-to-Ap
conversion is internal. The inactive and active cases check actual enabled
states and labels. Gravity's nine controls distinguish harmonic counts from
angle values and distinguish model/body names from files. External settings
include a checkable QGroupBox input that the lexical constructor census treats
as a container; the new inventory explicitly measures that Boolean input.

The parameter picker has seven actual controls in the three measured modes.
Its editable numeric entry accepts the literal1. This confirms that its unit
description cannot describe every entry as merely a reference name. Other
picker-mode predicates change eligibility or visibility; the source-domain
rule distinguishes references, array indices, literal values and function
arguments without assigning them a universal physical unit.

Documentation and source-domain review
--------------------------------------

Three shipped Qt control-unit paragraphs were clarified: literal/expression
entries inherit the receiving field/function convention; MagneticIndex uses
Kp; and both external-force toggles are Boolean. The ForceModel field table also
states the Kp convention explicitly. Every other source byte is preserved.
Sixteen reviewed policy-description hashes and nineteen reference hashes were
updated, preserving all construction keys and source identities.

The34-family source review now gives named input sets or quantified row/cell
rules, rather than only a widget-family route. It distinguishes command operands
and options, native-property editors, matrices/covariances/coefficient/mixture
grids, model and geographic vertices, camera/style/clock controls, dynamic-data
bounds, file comparisons, report selection, source editors and nonquantity
controls. Repeated object/curve/row/cell instances inherit explicit column or
parameter rules. Source declarations/excerpts and complete source hashes remain
inspectable. The review also keeps logical actions and the checkable external
QGroupBox explicit. It does not turn a source/hash match into numerical truth
or user-exposure qualification.

The current all-source guide inventory contains1,383 documented field-table
entries, including772 GUI-tagged entries. Of those,770 have explicit Units
metadata. The two exceptions are FieldName and Start in Command_Template.xml,
which is absent from the active167-file user-guide include graph. Both template
rows are preserved with explicit dispositions. Consequently no active-source
GUI-tagged field row lacks Units metadata. This table result still needs to be
combined with the measured non-table/dynamic input and source-domain evidence
for the complete E12 decision; it is not used as that decision by itself.

Validation and preserved boundaries
-----------------------------------

The extended GmatQtNasaInputCatalogTests utility builds. Its old modes and all
original source/assertions are byte-preserved after removing the one new include
and inventory-mode branch. A separate header implements the new mode. No core,
production Qt C++, CMake or scientific algorithm changed. The existing100-file
core-change inventory stays separately unapproved, and protected
TimeSystemConverter.cpp and MinQP.cpp hashes remain intact.

The nested-input unit auditor passes for all11 scenes,177 controls and33 native
field rows. The source-domain auditor passes for34 families,343 construction
keys and56 source files. All315 earlier configured states and72 field/unit pairs
were re-audited from retained receipts with no editor reconstruction. The old
census, sealed sources and original coverage records remain intact. Source
versions are kept distinct: extending the older inventory executable does not
retroactively change its earlier execution receipts.

The DocBook schema and HTML build pass. All187 checked field-unit rows, the
three clarified unit-policy paragraphs and all5,643 local page/fragment links
across275 reachable guide pages pass. Three changed HTML files were installed,
with compressed originals. The new render helper initially joined adjacent
field labels without spaces and rejected the already documented Brouwer field
pair; it was corrected to use the same label normalization as the preceding
verifier before installation. Existing GUI assertions and their expected values
were not changed.

Reusable checks, from the repository root::

  python src/qtgui/tests/AuditNestedInputDocumentation.py \
    --census build/nasa-coverage/recommended-campaign-20261101/nested-input-census \
    --output /tmp/gmat-nested-unit-review.json

  python src/qtgui/tests/AuditControlFieldDomains.py \
    --review build/nasa-coverage/recommended-campaign-20261101/control-field-domain-review.json \
    --output /tmp/gmat-control-domain-review.json

These audit retained source/runtime/documentation evidence without replaying
passing GUI scenarios. The new recording mode is --nested-inputs on the existing
inventory executable, using startup/output arguments as with its earlier modes.
The frozen recording recipe retains the exact executed command and inputs.

Final consolidation still required
----------------------------------

Combine all987 original identities and their explicit corrupt/nonrendered
metadata dispositions, the48 additional configured field/unit pairs, the new
built-in/nested evidence, all34 field-domain rules, active guide reachability and
unit meanings, and rendered/installed documentation into a requirement-by-
requirement E12 audit. Check every original campaign obligation and preserved
failure/core/gap record before assigning the final contract and updating the
handoff. No additional independent scientific or broader NASA qualification is
being added as an E12 gate.

Artifacts under build/nasa-coverage/recommended-campaign-20261101/ include:

* nested-input-census/ and nested-input-unit-review.json retain the new actual
  scenes, raw records, loaded images and unit meanings.
* dynamic-input-unit-doc-review.json retains the exact four documentation edits.
* control-field-domain-review.json and control-field-domain-integrity.json retain
  all34 concrete/parametric source-domain rules and current binding checks.
* twenty-first-guide-field-inventory.json and guide-field-metadata-dispositions.json
  retain all table rows and the two inactive-template dispositions.
* twenty-first-configured-input-reuse.json retains the315-state no-replay audit.
* twenty-first-unit-render-review.json records187 field rows, policy paragraphs,
  whole-guide links and installed HTML identities.
* twenty-first-increment-audit.json and twenty-first-increment-sources.tar.gz retain
  the final audit and frozen text sources.

The goal remains active pending final consolidation. Whole-current-build,
independent numerical, platform/GPU and broader NASA qualification remain false.
