Worksheet RMAG and SemilatusRectum access review — 2026-11-08
============================================================

The current implementation already supports both policies. Native remains the
Spacecraft default and allows the existing setters. Explicit Worksheet access
makes RMAG and SemilatusRectum read-only while allowing them to be read, reported
and used as inputs. No product code or test input was changed in this review.

Owning routes
-------------

Spacecraft exposes and validates OrbitParameterAccess, preserves it in copies
and assignment, emits explicit Worksheet settings, and omits the Native default
from generated scripts. Parameter::IsSettable reads the actual owner policy.
OrbitReal::SetReal checks that contract before delegating; OrbitData::SetReal
independently rejects the two Worksheet writes before changing the state.
ParameterWrapper delegates to the actual Parameter, and Assignment consults its
settable metadata. The configured and newly assembled destination pickers also
filter by owner policy; report reads remain selectable.

The actual ParameterFactory creators are SphRMag in SphericalParameters.cpp and
SemilatusRectum in AngularParameters.cpp. The historical ModEquinP class is not
the active SemilatusRectum factory route. This distinction is retained in the
source review rather than assuming class ownership from a similar name.

Evidence and limits
-------------------

Stage: build/nasa-coverage/coverage95-20261108-worksheet-orbit-access-current-review.
The immutable plan retains all 88 positional FRR38 reference rows and the two
original target fingerprints, FRR!E519 and FRR!E520. Thirty current source owners
are archived and hash-verified. All 1,334 protected original inputs are unchanged.

Four unchanged original checks pass in EACH runner profile, eight executions:
WorksheetOrbitAccess RMAG/SemilatusRectum and NasaOrbitalParameter
RMAG/SemilatusRectum. The Worksheet witnesses select the policy in the actual
resource editor, check pending changes and discard, Apply/history/Unicode reload,
configured and fresh read-only destination rejection, recovery, report reads,
function-output destinations and actual report viewers. They exercise two
prescribed Earth states and independent norm/cross-product calculations with a
synthetic gravitational parameter. Native writes are independently checked by
the Worksheet witnesses and the original orbital-parameter cases. Existing
assertions and numerical bounds were retained.

This finite evidence does not demonstrate all bodies, frames, singular/invalid
states, nested function and copy lifecycles, native OS input, platform or complete
family/science/full-current-build conjunctions. No full requirement row receives
new credit. Canonical acceptance remains 1,078/1,158 (93.1%): 80 open, with 23
additional passes needed for 95%. The keyed current source review now covers
31 of the original 44 groups; reviewed does not mean fully qualified.

Continue automatically with the Save any Resource parent review, retaining all
five original FRC12 rows and the existing serializer/ownership implementation.
The overall campaign remains active.
