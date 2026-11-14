Numerical text input units — targeted evidence
=============================================

Twenty-two numerical fields stored as strings or enumerations now carry
explicit GUI units. Date fields show days for modified Julian formats and
calendar date for Gregorian formats, along with the selected time-system
representation. Numeric ephemeris steps show seconds. Transponder turnaround
ratios show dimensionless units while retaining decimal and fraction syntax.
The existing value conversions, native setters and serialization are unchanged.

Two additional candidate fields, BatchEstimator.EstimationEpoch and its
BatchEstimatorInv alias, accept only FromParticipants. The native rejection
and factory implementation establish that these are symbolic choices, not
editable numerical dates. Their original rejected fixture is preserved.

The failing baseline contains 44 distinct unit observations, assembled from
a 34-observation prefix and the 10 missing tail observations. Two initial
fixture failures remain preserved. Forty headless passes were reused after
repairing the inspector's label-versus-input lookup; only its four false
negatives were rerun. All 44 observations also pass visibly. The resulting
88 passing observations cover both date formats and both ratio syntaxes.

Source, undo history, files and native objects are unchanged by inspection.
Native libraries retain their hashes. The seven-file production patch changes
unit descriptions and their format-change display updates only. Original
protected tests remain unchanged apart from the separately authorized ASCII
test-desktop title helper. No mission or full regression ran for this batch.

This is supplementary evidence with no whole-row credit. Live format-change
witnesses and the complete default/configured/nested/command conjunction still
remain before All Others!E13 can be admitted. Canonical coverage remains
1,111/1,158, with 10/18 agreed requirements complete.
