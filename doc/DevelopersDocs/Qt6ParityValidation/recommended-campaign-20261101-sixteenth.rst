Recommended campaign: active-guide input-unit coverage
======================================================

The original All Others!E12 requirement is: "The system shall have
user-documentation that describes the units of all input fields." This
increment advances that documentation obligation. It grants no new NASA scope:
coverage remains 923/1,158 requirements (79.7%) and 1,816/2,367 historical scopes
(76.7%). All 217 recovery dispositions remain 158 accepted/59 held. Local
contracts remain 36 verified and one missing all-input documentation contract.

What changed
------------

A new source inventory follows the explicit frontend source list and local
includes, covering 81 C++ files and 100 headers. It recognizes locally derived
editors, list/tree inputs and direct/stack/static Qt dialogs. It finds 343
construction templates in 56 files, including all 296 earlier sites and 47
additional sites. A template is not an individual exposed field: tables and
generic editors instantiate many fields, while some candidates are read-only.
Unknown Qt allocation types and nonliteral/additional frontend source lists
require review instead of being silently skipped.

The shipped Qt Control Units reference adds 34 unit-convention sections. These
cover specialist editors, command/condition operands, resource tables and
matrices, plot style, sensor masks, star display, model coordinates, playback,
comparisons, directories, scripts and output/navigation controls. Each of the 343 source-template
identities has an explicit convention route, bound to unchanged source,
section text and referenced field-document identities. These routes still need
concrete native/conditional field expansion and semantic review; they confer
no qualification keys.

The active-user-guide audit now follows the actual ug-profile include graph.
It excludes inactive pages and training-only content. The first active check
found980 of 987 original measured records after the transmitter addition; four
other matches existed only through inactive pages or spelling differences.
Two new native-label aliases and two BulirschStoer step-ratio descriptions close
those four routes. All 984 measured records with unit-documentation routes now
have active-guide routes. The other three preserved records are the old corrupt
MessageWindow.Dec lookup and the String.Object/Variable.Object metadata fields
that their scalar editors do not render. The separate repaired Precision field
and actual scalar-input conventions remain documented. Original records are
not renamed or removed, and original NASA denominators remain intact.

The transmitter reference previously described frequency-bias coefficients in
Hz powers. The implementation returns MHz and uses elapsed seconds in its
polynomial. The guide now states MHz, MHz/s, MHz/s^2, and so on, and explicitly
documents FrequencyBiasSigma with the corresponding coefficient units. No
transmitter value, covariance equation or numerical algorithm changed. The
previously identified indexed sigma-getter candidate defect remains separately
open; the whole-vector getter used by Qt is a different path. This documentation
repair is not a runtime validation or scientific approval of that getter.

The HTML link audit found one existing broken link: Python Interface Setup's
ID was attached to its title, where the stylesheet did not emit the requested
anchor. Moving the same ID to the containing refsection preserves the title,
text and existing link target. Initial schema failures from three references
to inactive pages led to the stricter active-guide audit; those references were
corrected. No failing assertion was removed to obtain a pass.

Verification and preservation
-----------------------------

Twenty-two new source/binding checks pass. They cover comments and raw strings,
derived controls, direct/static/stack dialogs, distinct constructor identities,
compiled-source boundaries, unknown Qt types, dynamic source-list changes,
missing/duplicate/stale bindings, changed live inputs, changed unit text,
missing sections, invented completion, inactive pages/rows, profile selection,
root versus unsupported fragments and include cycles. The new support test is
registered for reuse and passes through CTest. Existing GUI assertions and case recipes remain intact.
No GUI test, mission or native metadata probe was replayed in this increment.

The DocBook schema/HTML build and explicit rendering checks cover all 34 new
convention sections, the two transmitter fields and two step-ratio fields.
Every reachable generated HTML page and local fragment is checked before
deployment. Generated anonymous anchor IDs change with preceding content;
changed reachable HTML files are therefore installed together. Unchanged HTML
is reused, originals are compressed, and no image or runtime binary is copied.
The machine-readable render receipt records exact page/link/update counts.

All production Qt C++ and core sources retain their pre-increment hashes.
The existing99 changed core files retain their separate scrutiny obligation;
no technical or scientific approval is granted. The original campaign,
handoff, remaining-scope report, raw metadata and sealed source archives remain
preserved. Source-versioned passes are not relabelled as newest-binary
qualification. Full-current-build and full-NASA gates remain false.

Remaining work
--------------

The new complete measured-field worklist retains all 987 identities and makes
semantic review explicit:157 numeric records with N/A/None unit entries,
31 dependent/narrative descriptions,156 native-versus-guide unit strings,
five ambiguous native SolarCoeff strings,635 other field-meaning reviews and
the three preserved metadata dispositions. These are review buckets, not157
confirmed defects or635 new tests to rerun. N/A and blank native strings are
never automatically interpreted as dimensionless.

Prioritize actual physical-unit ambiguities, including tank-origin units,
RectangularFOV angle width, integrator MaxStep and the five native kW? solar
coefficient strings. Review ratios, counts, identifiers and dependent solver
criteria by their source/guide meaning. Then finish nested-mode and configured
field expansion, together with command/expression conventions. Generic source
routes and one-selector metadata sweeps do not by themselves prove every
conditional input has a correct unit description. Separate full-build,
platform/GPU, scientific and authority-dependent qualifications remain outside
this documentation claim and explicitly unqualified.

Reviewable artifacts
--------------------

Under build/nasa-coverage/recommended-campaign-20261101/:

* input-construction-inventory.json records all 343 templates and 181 source files.
* input-construction-bindings-review.json binds every template to a convention.
* shipped-input-documentation-review.json records every original field and its
  active-guide candidates, including the three explicit metadata dispositions.
* input-unit-semantics-worklist.json retains the complete remaining semantic
  worklist without granting credit.
* transmitter-input-units-review.json and active-unit-route-repairs.json record
  the actual field-documentation changes and source evidence.
* guide-link-review-initial.json and python-interface-anchor-repair.json preserve
  the actual broken-link diagnosis and narrow repair.
* qt-control-units-render-review.json records field/section rendering, all
  reachable HTML link checks, changed/unchanged deployment and source identities.
* sixteenth-increment-audit.json and sixteenth-increment-sources.tar.gz retain
  this increment's evidence audit and frozen text sources.

All Others!E12 and the goal remain incomplete until the remaining all-input
obligation and final requirement-by-requirement handoff audit are proven.
