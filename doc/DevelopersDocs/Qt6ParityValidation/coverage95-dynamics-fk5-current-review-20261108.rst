Dynamics, FK5 and rotation requirement review, 2026-11-08
========================================================

The current-source review covers the complete dynamics heading FRR!E103,
including its children E104 through E140. The earlier Mars-only review did
not cover this whole heading. No additional product gap was demonstrated in
the remaining force routes, and no executable change is made by this review.

* Nonspherical gravity uses GravityField and the configured celestial-body
  list. ResourceProperties.cpp applies PrimaryBodies and PointMasses through
  native force objects, validates body references and rejects overlapping
  lists. GravityDialog.cpp exposes potential files, degree and order, including
  a file selection route for user-defined bodies.
* AtmosphereFactory.cpp creates JacchiaRoberts and MSISE90. The MSISE86 factory
  is implemented inline in Msise86Factory.hpp; absence of a matching .cpp does
  not indicate a missing provider. NRLMsise00Factory creates NRLMSISE00,
  corresponding to the worksheet's original spelling NRLMISISE-00. DragForce
  forwards F107, F107A and MagneticIndex to atmosphere objects. The Qt
  atmosphere dialog exposes those inputs and registered atmosphere choices.
* MarsGRAM2005 has conditional InputFile and Mean/Low/High controls after the
  separately recorded editor repair. The configured runtime still cannot
  create its actual atmosphere provider. All six Mars-GRAM child requirements
  and both dependent dynamics/drag headings remain open.
* The historical cannonball drag and SRP routes use the native Spherical
  models. Additional shape choices do not remove that route.
* PointMassForce and the shared body lists support the named planetary,
  Pluto and user-defined-body references. GravityField provides native Solid
  and SolidAndPole tide models, with available models determined from its
  gravity/tide data. The Qt gravity dialog uses that native capability list.
* PhysicalModelFactory creates RelativisticCorrection, and the native force
  implements initialization and derivatives. The resource editor's toggle
  creates or removes the actual owned force object.

These findings establish source routes, not independent numerical accuracy,
the availability of every provider/data file, every body combination, native
operating-system input, or complete GUI acceptance of the dynamics heading.

FRR!E144's existing FK5 integration route remains present. Moderator creates
InternalEarthMJ2000Eq separately from display coordinates; AxisSystem defaults
to FK5 and MJ2000EqAxes retains identity rotation with zero rotation derivative.
Spacecraft and PropagationStateManager supply the internal Cartesian state to
propagation. ODEModel's origin changes translate that state, while force-local
frames and CoordinateConverter's explicit ICRF/FK5 conversions remain separate.
No new selector or replacement frame physics is needed from this source
finding. Independent FK5/frame/epoch qualification remains open.

For FRR!E221/E227, current CelestialBody.cpp still implements the documented
six angle/rate inputs, including W = W0 + RotationRate*d. The prior historical
research does not recover a NASA definition of the separate phrase "rotation
drift rate". The possible naming error is an inference. No spin acceleration,
units or original requirement interpretation is invented; both rows remain
open while independent work proceeds.

The current owner hashes, exact original row metadata and original-input
integrity result are recorded in the dynamics stage's current-review-v1.json
and the finite implementation audit's reviews 014 through 016. This review
changes no original input, assertion, tolerance, denominator or acceptance gate.
Canonical acceptance remains 1078/1158 (93.1%); new requirement credit is zero.
