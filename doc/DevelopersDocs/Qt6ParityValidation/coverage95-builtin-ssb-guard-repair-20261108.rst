Built-in barycenter diagnostic compatibility
============================================

Barycenter's ephemeris-source guard had shadowed CalculatedPoint's existing
built-in membership guard. Built-in points now reach the inherited rejection,
which identifies the object, field, attempted value and immutable membership.
Both setter overloads still reject before mutation. User-defined ephemeris
barycenters retain their separate rejection. Physics and layouts are unchanged.

Both original DE405/SPK421 SSB editor and numerical lifecycles and 20 related
calculated-point checks pass headless and visible: 44 executions. Maximum
position/velocity differences are 2.616e-5 km and 5.648e-12 km/s, within the
unchanged 0.002 km and 2e-8 km/s bounds. All 1,334 original inputs are intact.

Review 058 and checkpoint 045 bind the evidence under
build/nasa-coverage/coverage95-20261108-builtin-ssb-guard-current-review.
Canonical acceptance remains 1,078/1,158; no full-campaign claim is added.
