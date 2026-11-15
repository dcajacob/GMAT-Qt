FK5 integration-frame implementation scope, 2026-11-05
======================================================

FRR!E144 requires propagation with respect to Earth's mean equator axes at
J2000 as defined by FK5 reduction. Current owning-source inspection identifies
an existing implementation. This adds one existing row to the provisional
engineering tally, now 1087/1158 (93.87%). It adds no acceptance credit: recorded
verification remains 1053/1158 (90.9%) and all 105 acceptance rows remain open.

Owning route
-------------

* Moderator::CreateInternalCoordSystem creates InternalEarthMJ2000Eq through
  CreateCoordinateSystem with default/internal flags. That factory sets Earth
  origin/J2000 body and constructs MJ2000Eq axes. The internal system is kept
  separate from user-selectable spacecraft/display coordinate systems.
* AxisSystem's base-system default is FK5. MJ2000EqAxes inherits that base,
  initializes the identity rotation and retains zero rotation derivative;
  CalculateRotationMatrix leaves those fixed inertial axes unchanged.
* Spacecraft::GetPropItem(CARTESIAN_STATE) returns its internal state buffer;
  its CartesianX through CartesianVZ parameters return that buffer's six
  components. PropagationStateManager maps those values into the state used by
  Propagate/ODEModel. Display state conversion is separate and does not change
  the axes in which the numerical state is integrated.
* ODEModel::MoveToOrigin/ReturnFromOrigin and precise-time counterparts change
  the state origin by translation. They do not rotate away from the common
  inertial axes. Choosing another central body does not change the meaning of
  the FK5 axes requirement.
* ODEModel::SetInternalCoordinateSystem creates MJ2000Eq input/target frames
  and body-fixed force frames separately. GravityField transforms into the
  force frame; DragForce supplies the atmosphere's central-body MJ2000Eq and
  fixed systems. Their local force calculations are distinct from the
  integrated state's axes.
* Spacecraft::GetStateInRepresentation converts from internal to selected
  coordinates. CoordinateConverter::ConvertFromBaseToBase explicitly handles
  ICRF/FK5 conversion rather than treating their bases as identical.

The row's implementation is present without a new selector or replacement
physics. Numerical FK5 reduction accuracy, ephemeris/frame data, epoch handling,
force transforms and input/output results still require the later independent
verification pass. This source finding does not qualify all-body/all-frame
Parameter clauses or every force model, and supplies no scientific accuracy
claim.

Retained artifacts
-------------------

Stage: ``build/nasa-coverage/coverage95-20261105-implementation-fk5-source-scope``.
The stage records preimages/current hashes of owning sources and exact metadata/
documentation patches. ``tracking-v6.json`` adds only FRR!E144 as existing
implementation and retains the original verification baseline. There are now
34 additional engineering rows: 28 new/compatibility and 6 existing, leaving
71 original rows partial/unclassified/dependent. The complete implementation
audit remains unfinished.

No executable source changes or new build are needed for this source-present
finding. The preceding ephemeris and Achieve source changes compiled in their
own stages. No test, GUI, script/function/model/solver execution, numerical
comparison, acceptance reader/seal or qualification run. No original assertion,
fixture, tolerance, row text/fingerprint, denominator, acceptance ledger or gate
changed. No installed/shared runtime, package, commit or push update.
