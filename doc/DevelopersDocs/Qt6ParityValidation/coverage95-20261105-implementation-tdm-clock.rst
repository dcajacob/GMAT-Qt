Implementation first: precise TDM observation clocks
====================================================

The TDM clock path now constructs GmatTime epochs and normalizes them to the
native A1 scale before setting the scalar compatibility epoch. It no longer
constructs epochGT from an already rounded Real MJD. The scalar ParseEpoch entry
is retained as a wrapper, while observation loading uses ParseEpochGT directly.
The observation's epochSystem tag is explicitly A1MJD, matching both returned
clock representations and the estimator's time-system-aware filtering path.

UTC parsing uses the existing CCSDSEMSegment precise parser, including its
calendar and known-leap-second checks. Other supported native scales use validated
calendar/day-of-year fields, separate whole/fractional seconds and the existing
GmatTime converter. Nominal GPS epochs are converted through TAI with the
19-second scale offset. SCLK and unknown scales report a clock-correlation error
instead of being silently treated as another scale. They remain unsupported
pending the required correlation mechanism and inputs.

Every segment must declare one TIME_SYSTEM. The parser retains that source scale
separately from the normalized output tag and preserves it through active-reader
copy/assignment. Missing or duplicate declarations report measurement errors.
The supported native scale names are A1, TAI, UTC, UT1, TDB and TT; GPS has the
explicit nominal conversion above. This supported set is not a claim of complete
TDM standard conformance or support for arbitrary clock correlations.

The calendar grammar accepts the existing calendar-date/day-of-year forms,
fractional seconds and optional Z terminator. Invalid field structure and
non-UTC leap-second labels are rejected. Measurements are grouped by exact
native GmatTime equality rather than raw text, so equivalent timestamp spellings
can share a record without introducing an approximate comparison tolerance.

ObservationData's constructor now initializes its two time-reference flags.
TDM segment initialization resets their receive/end defaults before applying
explicit metadata, so a previous segment's settings cannot leak into the next.
Generic Clear retains its existing stream-setting behavior. Complete integration
reference handling, including MIDDLE and actual measurement-model consumption of
the stored flags, remains separate implementation work; no science claim is
inferred merely from parsing those fields.

The historical `CCSDS 503.0-B-1 standard
<https://ccsds.org/Pubs/503x0b1s.pdf>`_, table 3-3, identifies the required time
system and time-reference metadata. The nominal GPS/TAI relation is documented by
`USNO <https://www.cnmoc.usff.navy.mil/Our-Commands/United-States-Naval-Observatory/Precise-Time-Department/Global-Positioning-System/USNO-GPS-Time-Transfer/Leap-Seconds/>`_.
The installed native converter and date/parser implementations define the actual
conversion used. Physical clock calibration, arbitrary data ordering, other TDM
metadata and end-to-end estimator behavior remain unqualified.

Three production files change in this increment: TdmReadWriter.cpp/.hpp and
ObservationData.cpp. The private source-scale string extends the previously
changed reader layout, so EstimationPlugin and clients must use a matching
rebuilt header/module. No external ABI, scientific, full-build or full-NASA gate
is cleared. Shared binaries and public preview packages are unchanged.

Exact preimages and the incremental patch are under
``build/nasa-coverage/coverage95-20261105-implementation-tdm-clock``.
The patch SHA256 is
``47cc6bd04c3afc556f797fbedddcb8ee5a6968cff1581506aaf966bd5fa3e4f0``.
The reverse-application and added-whitespace checks pass. ``compile-results.json``
records four clean compilations: the changed reader and observation record,
plus the existing TdmObType and ObTypeFactory header consumers. The installed
Util image already exports the precise CCSDS parser and GmatTime converter used
by this source. Object compilation is not a linked runtime or precision proof.

No new test, fixture, registration, source capsule or evidence seal was added,
and no old native test was rerun. Under the user's implementation-first direction,
validation remains deferred. Backfill independent nanosecond/fractional and
midnight/year/day-of-year/leap boundaries, UTC/TAI/GPS and native-scale equivalent
instants, declared/missing/unsupported scale metadata, exact record grouping,
active copied-reader state and actual estimator/filter results. Add the relevant
unattended and initially minimized/resizable visible GUI routes when testing
resumes; preserve all original assertions and tolerances.

Verified acceptance remains 1053/1158, with 105 open and 48 needed for 1101.
The original 59 hold identities and historical 1816/2367 coverage are unchanged.
No original whole-row credit is assigned, and the 95 percent goal remains active
and incomplete.
