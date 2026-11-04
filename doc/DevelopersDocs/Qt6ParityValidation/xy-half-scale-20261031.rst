Half-scale XY marker repair
============================

The specific 0.5x XY raster failure is repaired. All original XYAxisTests.cpp
source bytes, color thresholds, expected coordinates, clipping checks, capture
DPR/dimension checks and numerical/source/report assertions are unchanged.
The five affected XY registrations pass visibly at 0.5x, 1x, 1.5x, 2x and 3x.
The original 0.5x failure and an offscreen repaired pass are retained separately.

Cause and change
----------------

``PlotWidget.cpp::drawMarker`` used a one-logical-pixel pen. At device pixel
ratio 0.5 this becomes a half-physical-pixel stroke. The prior diagnostic located
the marker at the expected position but found insufficient color coverage;
resampling could not recover full-color pixels that were absent from the image.

The marker pen now has a minimum width of one physical pixel when DPR is below
one (logical width 1/DPR). At DPR one and above the existing width remains one.
Marker positions, sizes, shapes, antialiasing, clipping and image resolution
are unchanged. This shared 2D marker helper also applies to ground-track markers
when requested at reduced DPR; arbitrary ground-track marker cases are not
qualified by this XY regression. No core, solver, force, integration or
estimation source is changed by this repair.

``QtGui.XYAxesSubunitDPI`` is now a permanent CTest registration with
``QT_SCALE_FACTOR=0.5``. It runs the full existing XY test, including display
controls, Cancel, invalid-input correction, persistence, exact source/report
preservation, independent marker placement, clipping, grid and label checks.
The acceptance mechanism selector includes the new registration.

Evidence and reproduction
--------------------------

Evidence root: ``build/release-check/20261031-xy-half-scale-repair/``.

* ``baseline.log/.xml``: reproduced the original 0.5x placement/color failure.
* ``stroke-floor.log/.xml``: unchanged XY assertions pass offscreen with the
  repaired production helper and scale 0.5.
* ``automated.log/.xml``: the permanent QtGui.XYAxesSubunitDPI registration also
  passes in ordinary automated/offscreen mode.
* ``visible/results.json``, ``visible/junit.xml`` and the five individual result
  directories: five passes on xcb, eight native exposure events per case,
  zero screenshots. The nested Xephyr desktop started minimized, was resizable,
  used no fullscreen mode and closed after completion.
* ``validation-summary.json``: input hash checks, preserved original test bytes,
  reference-preservation audit and exact selected case identities.
* ``traceability.log``: all 41 traceability support assertions pass.
* ``increment.patch`` and ``before.json``: exact increment against the pre-repair
  dirty checkout, not against HEAD. The redundant large before-metadata JSON is
  retained losslessly as ``.json.gz``; its round-trip and hash record is in
  ``compressed-before-metadata.json``. No binary/runtime snapshot was copied.
* ``sealed-handoff-verification.log``: the earlier sealed handoff remains intact;
  this verifier executed zero product tests.

Rebuild and run just the new automated regression from the repository root::

   cmake --build build/linux-gui --target GmatQtXYAxisTests -j4
   ctest --test-dir build/linux-gui -C Release \
      -R '^QtGui\.XYAxesSubunitDPI$' --no-tests=error --output-on-failure

Watch the affected scale matrix, starting minimized::

   python3 src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
      --visible --start-minimized --scale 1 --max-screenshots 0 \
      --filter '^QtGui\.XYAxes(SubunitDPI|FractionalDPI|HiDPI|HighScale)?$' \
      --output build/gui-acceptance/NEW-XY-SCALE-RUN

Use a new output directory for another deliberate run. There is no need to
repeat the broader catalogue for this bounded repair. The four normal/higher
scale cases were replayed because the production drawing helper changed.

Coverage boundaries
---------------------

The 245 existing proof references to the prior PlotWidget.cpp retain their
exact original source bytes under
``xy-half-scale-20261031/source-baseline/src/qtgui/PlotWidget.cpp``. Their expected
hashes, anchors, tolerances and proof facets are preserved. They are not silently
upgraded to qualification of this rendering build.

SUBUNIT-DPI-RASTER now describes the remaining arbitrary subunit scales, marker
shapes, pixel phases and native platform rasterization domain. The concrete
0.5x square-marker failure is no longer an unresolved failure. This is not
qualification of every marker, DPI or physical GPU/compositor combination.
The twelve disabled nested solver source-conflict tests remain unchanged.
Broader NASA qualification and independent review of accumulated core changes
remain deferred. No requirement-scope or historical-scope credit was added:
the retained source-versioned handoff remains 742/1,158 (64.1%) requirement
scopes and 1,803/2,367 (76.2%) historical scopes. The filtered replay's 0% NASA
scope figure describes that five-case receipt alone, not lost project coverage.
