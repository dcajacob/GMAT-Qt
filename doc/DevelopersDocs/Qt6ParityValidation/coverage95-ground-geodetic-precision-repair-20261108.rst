Ground-track latitude precision
================================

The original solver Toggle test compared its final ground-track point with the
native geodetic report. Latitude differed by 1.146e-8 degrees, exceeding the
unchanged 1e-8 limit. GroundTrack stopped its latitude iteration at 1e-7 radians,
while the native parameter calculation used 1e-12 radians.

GroundTrack now uses the same iteration precision. Formulas, frames, epochs,
sampling and class layouts are unchanged. The original latitude difference
drops to 1.079e-11 degrees. Corrector, optimizer and nested solver controls,
dense/sparse body-symbol controls and integration-skip boundary controls pass
headless and visible. Seven selected original plot cases also pass in both
profiles; the combined full plot parent remains open.

An added native regression checks 720 samples against independent forward
ellipsoid geometry, including poles, both hemispheres, four shapes and four
heights. The original implementation fails 300 samples. The repaired maximum
latitude error is 3.475e-12 degrees, within the additional 1e-9 bound. This is a
finite geometry check, not independent qualification of planetary shape data.

All 1,334 protected original inputs remain unchanged. Review 060/checkpoint 047
bind the evidence under
build/nasa-coverage/coverage95-20261108-solver-ground-endpoint-current-review.
Canonical acceptance remains 1,078/1,158 (93.1%); full rebuild/regression and
unresolved requirement/provider/platform gates remain open.
