Timing, field-of-view and station units
======================================

Nineteen exact resource-type/field pairs had empty display-unit metadata.
Four event-search inputs already displayed seconds in their primary forms;
the other gaps included field-of-view angles, station weather inputs, filter
time spans, process-noise update time, simulation measurement time, transponder
delay and tracking settings. A narrow fallback table now supplies their
documented physical units only when native metadata is empty.

Native calculations, setters, values and public interfaces are unchanged.
Original headless results retain all nineteen inconsistencies, including the
four existing primary-label/backing-metadata mismatches. A supplementary repair
fixture selects precisely the failed fields with unchanged unit expectations.
The nineteen already-passing observations are reused without rerunning them.

There are 76 passing GUI observations across 38 conditional field/configuration
pairs, covering 29 entries in the saved unit worklist. Both profiles exercise
station locations in Cartesian and Spherical forms with Sphere and Ellipsoid
horizons. The batch also observes existing half-life, SNC spectral-density,
range-unit, Doppler-interval and radio-frequency labels. It does not execute
missions or qualify additional process-noise models or measurement physics.

The frozen reader checks explicit guide units, native angular conversion and
the default SNC model, source/build/runtime identity, exact patch scope, raw
labels, state/history/file isolation and native visibility. Its negative
controls reject changed scales and missing conditional observations.

The saved numeric-unit worklist decreases from 123 to 94 entries. This is not a
defect count or the complete current input universe. All Others E13 remains
open. Canonical acceptance stays at 1,111/1,158, ten of eighteen agreed rows.
No full regression or whole-current-build qualification is claimed.

Evidence is sealed under
``build/nasa-coverage/complete18-20261112-timing-geometry-units-v1``.
