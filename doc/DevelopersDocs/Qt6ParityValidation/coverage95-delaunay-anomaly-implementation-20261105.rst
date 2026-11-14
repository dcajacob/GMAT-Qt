Delaunay anomaly convention repair, 2026-11-05
=============================================

The FRR!E13 owning-source pass found an angle-convention defect in
StateConversionUtil. KeplerianToDelaunay consumes true anomaly and derives
Delaunay's mean anomaly from it; DelaunayToKeplerian returns true anomaly.
Several dispatch routes instead supplied or interpreted that angle using the
caller's selected Keplerian anomaly type. A retained MA or EA selector could
therefore alter the physical state when switching to or from Delaunay.

Implemented correction
----------------------

* Cartesian, both spherical, all three equinoctial and Planetodetic inputs now
  supply TA to the Delaunay helper. The asymptote and Brouwer routes that reuse
  the Cartesian dispatcher inherit the correction.
* Keplerian and ModifiedKeplerian inputs convert their selected sixth element
  to TA before entering the Delaunay helper.
* Delaunay inputs use the helper's returned TA for the intermediate Cartesian
  state. Only Keplerian or ModifiedKeplerian output changes that angle to the
  requested anomaly type.

The public low-level helpers keep their existing TA contract. TA input/output
avoids an added radians/degrees round trip. No formula, tolerance, singularity
threshold, representation inventory or exported signature is changed. The
header and spacecraft orbit-state guide now state which angle the selector
controls. The existing elliptic-only Delaunay domain remains in force.

Build and remaining work
------------------------

Stage: ``build/nasa-coverage/coverage95-20261105-implementation-delaunay-anomaly``.
The configured private build passed all 88 steps in 149.97 seconds; session
61956 is terminal. ``build-v1.json`` and its log retain the result. Base, Qt,
console, plugin and Python binding targets were rebuilt. Test/probe clients
were only linked; compilation is the only execution authorized in this phase.
No model, numerical matrix, GUI, test, acceptance reader or seal was run.

This fixes a concrete conversion defect without claiming the complete FRR!E13
obligation. The representation/frame preconditions and remaining native/Qt
entry paths still require the implementation review. The conservative tally
stays 1082/1158 (93.44%); recorded verification stays 1053/1158 (90.9%). Original
fixtures, assertions, denominator, fingerprints, acceptance ledger and gates
remain unchanged.

After the implementation pass, verification must cover non-TA Keplerian and
ModifiedKeplerian input/output, all non-Keplerian routes to/from Delaunay under
retained anomaly selectors, TA regression behavior, invalid anomaly/domain
errors and the spacecraft/parameter/editor routes. No such behavior is proven
by compilation.
