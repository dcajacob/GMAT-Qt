Contact, intrusion and region Brent implementation, 2026-11-05
================================================================

This batch completes the owning-source implementation routes for FRR!E605/E606.
The source review covers all three registered event locators: EclipseLocator,
ContactLocator and IntrusionLocator. The two rows are now tracked as
implementation present, not verified acceptance. They were previously partial
because contact intervals, intrusion constraints and region contacts had not
been connected. Recorded acceptance, the 105 original open acceptance rows and
all qualification gates remain unchanged.

Continuous constraints
----------------------

The shared EventRootRefinement backend now includes apparent latitude,
latitude-extremum, instrument-FOV and phase-angle residuals, in addition to the
previous triaxial occultation residuals. Position/state calls use the same
observer frame and aberration correction as their GF searches. The extremum
residual is the analytic numerator of the apparent latitude derivative.
Phase refinement uses the public phaseq API with the original phase threshold;
MinimumPhase's existing interpretation is preserved. Its setter now rejects
nonfinite values.

Point-FOV geometry uses angular distance for circles, the projected quadratic
for ellipses, and signed angular distance to the actual spherical polygon
boundary for simple polygons. Winding orientation excludes the antipodal
region. Concave polygons remain supported; a convex halfspace intersection
would have changed the SPICE geometry contract.

For extended targets the apparent triaxial limb retains the body's light-time
orientation, then rotates into the instrument frame at the observer epoch.
Circle/ellipse constraints are quadratic cones with forward-ray restrictions.
Simple polygon FOVs are triangulated by spherical ear clipping. Each triangle's
continuous overlap margin is the maximum, along the target limb, of its minimum
linear edge constraint. The existing polynomial extrema routine finds all
stationary/intersection candidates without an angular sampling grid. Polygon
vertices inside the target cone cover complete polygon containment. The union
of triangles supplies the extended-target margin. Point targets use real
polygon edges so artificial triangulation diagonals cannot create false roots.

Instrument shape/frame/vertices come from public bods2c/getfov calls, with
SPICE_GF_MAXVRT capacity on the heap. No private NAIF ABI functions are called.
The derivation is new, numerically unverified code.

Contact and intrusion integration
--------------------------------

EphemManager retains its original exported contact/intrusion signatures as
SPICE-default wrappers, and adds overloads accepting root method/tolerance.
Contact elevation, imager FOV and each occultation window are refined before
intersection/difference. Contact's ABSMAX report search now owns a separate
confinement window for each contact; it previously aliased result.data and
reused the first contact's window. Empty maxima fail before extraction, and
Brent refines interior peak epochs using the latitude-derivative residual.

Intrusion refines FOV, phase, occultation and reverse-transit windows before
their window differences. RootFindingMethod=SPICE(default)/Brent and
RootTolerance=1e-6 positive finite TDB seconds are appended metadata fields.
Existing IDs are preserved. Typed access, enum choices, copy/assignment,
default serialization cloaking, configuration-only restrictions and Qt controls
are connected. The class gains two fields; dependent images must be rebuilt
together. Contact root setters invalidate readiness. Its previous station-only
restriction is removed now that the other contact paths are connected.
Additional-target workers copy the same settings.

Planetographic regions
----------------------

Region crossing discovery retains its membership-transition search. Brent
refines a transition against the existing signed boundary distance, using time
offsets in TDB seconds. Both A1 and TDB epoch representation limits are checked;
conversion back to the native A1 epoch remains necessary for the region state
lookup. The SPICE selection retains the old region binary interpolation.

The region distance helper now computes distance to the actual finite segment,
including endpoints and zero-length segments. Its old infinite-line distance
could have zero residual away from a polygon edge. The region membership
intersection predicates are unchanged. Sampling uses a finite positive step
and a checked ceiling interval count with at least one interval, avoiding the
old division by zero for a search shorter than one step. A SpacePoint pointer
replaces an invalid BodyFixedPoint downcast of the region's central body.

Source route review and numerical limits
---------------------------------------

The factory registers EclipseLocator, ContactLocator and IntrusionLocator.
Eclipse passes root settings for umbra/penumbra/antumbra. Contact passes them for
station rise/set, spacecraft contact intervals and additional-target workers;
region transitions call the exported shared scalar solver. Intrusion passes
them for each sensor/target search. Native metadata and Qt editor routes expose
the method/tolerance; unknown methods and nonpositive/nonfinite tolerances fail.
Root settings are configuration-only in EventLocator's command-mode policy.

SPICE remains responsible for interval discovery, occultation ordering/type and
coverage. A clipped confinement endpoint is not independently moved. Interior
window boundaries need a continuous sign-changing bracket around the GF
candidate, bounded by coverage and neighboring result boundaries. Missing
brackets, including unresolved isolated grazing/degenerate boundaries, fail
explicitly rather than receiving an unrefined result. Station rise/set retains
its existing crossing/tangency classification. StepSize can still miss events;
root refinement cannot repair discovery omissions. Requests below epoch
resolution fail. These limits and all residual behavior need later numerical
and GUI qualification; source presence is not evidence of their correctness.

Primary references
------------------

* `NAIF getfov <https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/getfov_c.html>`_.
* `NAIF gftfov <https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/gftfov_c.html>`_.
* `NAIF phaseq <https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/phaseq_c.html>`_.
* `NAIF gfposc <https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/gfposc_c.html>`_.

The installed CSPICE source, especially zzgffvu and zzelvupy, was read to
establish frame/epoch conventions and non-convex polygon support. It is located
at ``/home/dan/GIT/gmat-build/gmat-git/depends/cspice/linux/cspice64/src/cspice``.
The implementation calls only public CSPICE APIs.

Build and deferred verification
-------------------------------

Stage: ``build/nasa-coverage/coverage95-20261105-implementation-contact-intrusion-brent``.
Private configured builds pass: v1, 78 steps/160.98 seconds; v2, 71 steps/5.70
seconds; v3, 4 steps/4.38 seconds. Sessions 22180/42330/51284 are terminal.
Base, event plugin, Qt, console, dependent plugins and Python bindings built.
Test/probe clients were linked only. No test, GUI, event/model execution,
numerical comparison, acceptance reader/seal or qualification run was launched.
No original assertion, fixture, tolerance, fingerprint, denominator, ledger or
gate changed. No installed/shared runtime, package, commit or push update.

The provisional tally is 1086/1158 (93.78%): 33 additional original rows,
28 new/compatibility and 5 existing. The other 72 original open rows remain
partial/unclassified/dependent. Recorded verification remains 1053/1158 (90.9%).
The current source map is worklist-v1.json and the latest tally is tracking-v5.json
in ``build/nasa-coverage/coverage95-20261105-implementation-progress``.

Later verification must cover all FOV shapes and winding orders, concave and
near-degenerate polygons, point and triaxial bodies, full containment, crossing
limbs, stellar/light-time corrections, rotating instrument/body frames, phase
thresholds, all occultation types, contact-window maxima, region short windows
and segment endpoints, clipped and neighboring roots, grazing/error paths,
epoch precision, tolerance values, native/Qt persistence and default-SPICE
regressions. That work is deliberately deferred under the implementation-first
instruction.
