Continuous event roots and eclipse Brent path, 2026-11-05
========================================================

This batch extends the implementation toward the complete FRR!E605/E606
obligation. It does not complete that obligation: contact intervals and intrusion
FOV/phase/occultation boundaries still need the shared backend. No requirement
credit or verification result is assigned.

Shared backend
---------------

EventRootRefinement.cpp/.hpp provide a shared Brent-Dekker scalar solver,
confinement-aware window refinement and continuous triaxial-ellipsoid limb
residuals. The existing station elevation path now calls the shared scalar
solver, retaining its existing candidate/bracket/tolerance selection.

For an observer vector o in unit-ellipsoid coordinates and transformed ray d,
the tangent-cone quadratic is proportional to
``dot(d,d) - dot(cross(o,d),cross(o,d))``. The cross-product form avoids
subtracting two large nearly equal observer-distance terms. A forward-ray
constraint excludes the cone's opposite nappe. The signed margin is the minimum
of the normalized quadratic and forward linear constraint.

The other body's apparent limb is an ellipse ``c + u*cos(t) + v*sin(t)``.
Substitution gives a second-order trigonometric quadratic and a first-order
forward constraint. Extrema of their minimum occur at stationary points or at
their intersections. The implementation isolates the resulting polynomial
roots on two bounded tangent-half-angle charts covering the whole circle;
it does not use an angular sample grid. Overlap, full containment and reverse
containment produce the ANY, FULL and ANNULAR boundary residuals. PARTIAL
boundaries can arise from any of those three residuals.

SPICE still discovers intervals and establishes their occultation type and
front/back ordering. The new code refines those selected boundaries using real
geometry functions. It does not use the integer occultation classification or
a Boolean visibility result as a numerical root function. A clipped confinement
endpoint stays unchanged; an interior boundary without a continuous sign bracket
fails explicitly. Neighboring boundaries constrain the candidate bracket, and
tolerances below the representable SPICE epoch resolution are rejected.

Geometry and references
-----------------------

Only public CSPICE APIs are called: spkpos, bodvrd, pxform, edlimb, el2cgv and
the window/error functions. Radii remain triaxial. Body orientations use each
body's reception/transmission light-time epoch. Stellar aberration is omitted
for occultation geometry, matching the documented gfoclt contract.

Primary references are NAIF's
`gfoclt <https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/gfoclt_c.html>`_,
`edlimb <https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/edlimb_c.html>`_
and `gftfov <https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/gftfov_c.html>`_.
The installed CSPICE source at
``/home/dan/GIT/gmat-build/gmat-git/depends/cspice/linux/cspice64/src/cspice``
was also read to establish geometry/epoch conventions. Private NAIF procedures
are not called or made an ABI dependency. The residual derivation and root
isolation implementation are new code and remain numerically unverified.

Eclipse integration
--------------------

EclipseLocator appends RootFindingMethod=SPICE/Brent and RootTolerance (positive
TDB seconds, default 1e-6). Metadata, typed access, copy/assignment, configuration
restrictions, default cloaking, native serialization and the Qt editor are
connected. Existing EclipseTypes parameter IDs are preserved. The class gains
two members; all dependent images must be rebuilt together.

An EphemManager overload accepts the root settings; the old signature remains
as a SPICE-default wrapper. Umbra, penumbra and antumbra searches now pass their
settings through this overload. The GUI uses the common Root refinement label.
ContactLocator's station-only selection restriction is retained until its
interval/FOV/occultation path is extended.

Build and remaining implementation
----------------------------------

Stage: ``build/nasa-coverage/coverage95-20261105-implementation-eclipse-brent``.
Build-v1 passes in 5.01 seconds for the shared backend/station connection;
build-v2 passes 77 steps in 154.68 seconds including eclipse metadata/callers
and all dependent Python bindings. Sessions 28004 and 34533 are terminal.
Use their JSON/log receipts for results. No test, GUI, model, eclipse/contact/intrusion execution, numerical
comparison, acceptance reader or seal was run. No fixture, assertion, tolerance,
fingerprint, denominator, acceptance ledger or qualification gate changed.
No installed/shared runtime, package, commit or push update.

Implementation tracking stays 1084/1158 (93.61%), with recorded verification
1053/1158 (90.9%). All 105 original acceptance rows remain open. Next implement
real-valued elevation, point/ellipsoid FOV, phase and occultation refinement for
contact intervals and intrusion constraints, applying each refinement before
window intersection/difference. Then complete the cross-locator policy wiring.
The later verification pass must include shape/epoch conventions, all eclipse
types, near-degenerate limbs, polynomial extrema, clipped/neighboring boundaries,
precision limits, user tolerances, station regressions and native/Qt persistence.
