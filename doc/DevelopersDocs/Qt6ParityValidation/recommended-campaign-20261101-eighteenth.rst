Recommended campaign: remaining numeric and dependent unit conventions
======================================================================

Coverage remains 923/1,158 requirements (79.7%) and 1,816/2,367 historical
scopes (76.7%). All 217 recovery candidates have dispositions: 158 accepted,
59 held. Local contracts remain 36 verified and one open, All Others!E12.
This increment advances that documentation obligation without awarding partial
requirement credit or changing the original scope.

Corrections and evidence
------------------------

The eleven previously held numeric records now have source-derived unit
explanations in ten guide rows. OLSEAdditiveConstant distinguishes normalized
residuals from the zero-covariance unit-weight fallback. Propagator accuracy
settings follow the selected error metric, including absolute fallbacks.
Yukon tolerances distinguish constraint residuals, relative/near-zero absolute
objective change and gradients in solver coordinates. MaximumElasticWeight is
a penalty coefficient, not an iteration count. Four misleading Yukon field
descriptions were corrected alongside their Units entries.

The VF13ad Tolerance binding was inspected read-only in the installed wrapper:
the inherited tolerance member is supplied as HSL argument 11, ACC. The vendor
specification describes objective-value accuracy and recommends deliberate
function/variable scaling. FeasibilityTolerance follows constraint residuals.
The distributed wrapper source is unavailable; the vendor-binary qualification
gap remains explicit. No vendor routine or mission was executed for this review.
The primary reference is https://www.hsl.rl.ac.uk/archive-specs/vf13.pdf.

Thirty-one original dependent-unit records now have explicit guide conventions
in nineteen rows: filter thinning counts/seconds; electric-thruster polynomial
coefficients with kW input and mN/mg-per-second output; the ephemeris Boolean;
measurement-dependent bias/noise units; ground-station coordinate units; and
SNC process-noise sigma. Ground-station descriptions now correctly separate
StateType from HorizonReference. An additional efficiency row states that 0.7
means 70 percent; it does not add an implementation range restriction.

The 156 retained explicit native/guide unit records pass a declared-unit
consistency check. That review found and repaired three Isp-coefficient exponent
symbols, two kW spellings and an underspecified coordinate-system epoch unit.
The documentation resolver previously merged case-distinct Delaunay actions
and angles. It now prefers exact field spelling and permits a case-insensitive
fallback only when the spelling is unambiguous. All eight original resolver
tests and helpers retain identical syntax trees; three new boundary tests bring
the total to eleven. The registered support CTest passes. This is documentation
routing verification, not independent orbital-science qualification.

Validation and preserved boundaries
-----------------------------------

Thirty-six field-unit rows across fourteen source pages changed in this batch,
along with the declared mode/scaling descriptions and notes. The restoration
check proves every other source byte is preserved. The guide schema and HTML
build pass. Generated table rows include all 36 new unit descriptions and the
100 descriptions checked in the preceding increment. The complete reachable
help-page graph is checked for local page and fragment links before changed
HTML is installed together. Compressed originals are retained; no images,
screenshots or runtime binaries are copied.

All 987 original census identities remain preserved. Active-guide routes remain
984, with the same three corrupt/nonrendered metadata dispositions. All 343
source-template routes retain their original source keys and unit-policy text;
only the hashes of their reviewed field-reference documents are refreshed.
A source template is not a fully expanded, exposed field inventory.

No core, production Qt C++, CMake or mission algorithm changed this increment.
The existing 100-file core-change inventory remains unapproved for technical
scrutiny; protected TimeSystemConverter.cpp and MinQP.cpp are unchanged. No
previously passing GUI scenario was replayed and no mission was run. The
original campaign, handoff, gaps, census and sealed increment archives remain
unchanged. Current-build, broader NASA and independent scientific qualification
remain separate unqualified gates.

Remaining bounded work
----------------------

The review retains 634 field meanings still to check against their documented
unit conventions. These are records awaiting review, not 634 established bugs
or necessarily 634 new tests. Configured/nested modes and dynamic controls also
need a concrete completeness audit beyond the single-selector census. Complete
that inventory and the E12 documentation audit before crediting the final local
contract. Independent scientific qualification is not a prerequisite for this
documentation requirement and is not claimed by these unit descriptions.

Artifacts under build/nasa-coverage/recommended-campaign-20261101/ include:

* remaining-numeric-unit-review.json and vf13ad-unit-binding-review.json retain
  the eleven-record source and binary-binding evidence.
* dependent-unit-row-review.json and unit-agreement-row-repairs.json retain the
  exact field-unit and ground-station description edits.
* explicit-native-guide-unit-agreement.json retains all 156 consistency results.
* eighteenth-doc-preservation.json and eighteenth-resolver-assertion-preservation.json
  prove the documentation and existing assertion boundaries.
* eighteenth-unit-semantic-dispositions.json retains all 987 original identities
  with explicit reviewed and open dispositions.
* eighteenth-unit-render-review.json records field rows, links and installed hashes.
* eighteenth-increment-audit.json and eighteenth-increment-sources.tar.gz retain
  the final audit and frozen text sources for this increment.

The goal remains active; no complete NASA qualification is claimed.
