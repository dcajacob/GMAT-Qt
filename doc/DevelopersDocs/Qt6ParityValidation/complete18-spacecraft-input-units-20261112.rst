Spacecraft input-unit evidence
==============================

Spacecraft model translation fields did not identify their units. They now show
``model units`` in the primary editor, with a tooltip stating that translation
uses local model coordinates before scale and rotation. The resource metadata
provides the same fallback for ModelOffsetX/Y/Z. Native transforms, values and
public interfaces are unchanged. This intentionally preserves the distinction
from celestial-body model offsets, which use kilometres after transformation.

This batch verifies 59 spacecraft field/unit pairs in both headless and visible
profiles. It reuses five source-versioned passing runs for 22 basic orbit fields
and eight additional headless Delaunay/modified-equinoctial fields. Their 342
unit observations retain the original scope and numerical assertions; they are
not relabeled as qualification of the current whole build.

The remaining coverage adds 66 passing unit observations: Brouwer mean elements,
incoming/outgoing asymptotes, Delaunay and modified-equinoctial visible controls,
mass, area, model rotation and model translation. Twenty-six passing headless
observations are reused from before the caption repair. Only the three failed
translation labels are rerun headlessly after repair. The original three
failures remain available. These new checks execute no missions.

The frozen reader compares exact field names and independently defined units,
including case-sensitive Delaunay angles versus actions. It checks current
guide definitions, archived assertion sources, source/build/run identities,
native exposure, source/history/file/native-state isolation, and the exact
two-file production patch. Incorrect unit scales and missing observations are
rejected. Supplementary tests extend the display worker without changing its
existing routes or the original protected tests.

The saved numeric-unit worklist falls from 182 to 123 entries. It remains an
incomplete inventory of conditional current inputs, not a count of defects.
Spacecraft identifiers and covariance units require their own reconciliation.
All Others E13 remains open. Coverage is unchanged at 1,111/1,158 and ten of the
eighteen agreed rows. No full regression or new whole-row credit is claimed.

Evidence is sealed under
``build/nasa-coverage/complete18-20261112-spacecraft-input-units-v1``.
