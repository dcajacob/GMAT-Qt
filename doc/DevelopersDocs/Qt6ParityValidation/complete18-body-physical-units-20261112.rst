Body physical-unit captions
===========================

The body-model offset fields lacked a unit in their editor captions.  The
captions now say ``km``, matching the existing body-model translation and the
shipped unit definition.  This is a one-line caption change in
``CelestialBodyPanel.cpp``; it changes no native value, geometry transform,
validation rule or public API.

The saved pilot contains the three original missing-caption failures.  The
repair is checked across the eleven default bodies and the four user-body
factory types, both headlessly and visibly.  The other twelve physical-unit
captions per body are also checked.  Their 180 passing headless observations
are reused from before the caption-only repair; the visible profile checks
all 225 fields after it.  The complete record contains 450 passing unit
observations and retains the three pilot failures.  No mission or full
regression was run for this display change.

The stage is
``build/nasa-coverage/complete18-20261112-body-physical-units-v1``.
It also resolves six documentation lookup ambiguities: StepSize belongs to
five separately named ephemeris-propagator sections, and OrbitErrorCovariance
has an exact GUI field label inside its formatted table entry.  These mapping
repairs do not themselves qualify a GUI field.

The change completes 60 entries in the saved numeric-field worklist.  The
remaining 369 saved entries are a review queue, not a count of known defects.
Sun/Star observations are additional coverage; the older worklist did not
contain Star records, so they are not subtracted from its remaining count.
Whole-row All Others!E13 acceptance still needs the complete current input
inventory, field-meaning and actual GUI-evidence conjunction, including
conditional units and expressions.  The canonical coverage remains
1,111/1,158 and ten of eighteen agreed requirements.  Original assertions
and failure receipts are preserved; the existing, separately authorized
window-title helper change remains the only protected-input exception.
