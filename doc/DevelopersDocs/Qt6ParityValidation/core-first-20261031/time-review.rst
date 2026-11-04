TimeSystemConverter: review and per-call leap-state repair
=========================================================

The eight preexisting changed hunks in the Real/GmatTime string-format overloads
were reviewed against preserved HEAD/current sources and their callers. Two
wrong-timestamp defects were reproduced and repaired. Only TimeSystemConverter.cpp
changes in production during this package. Public signatures, class layout,
virtual slots, time-scale equations and existing numerical bounds are unchanged.
This author review is not independent scientific approval.

``build/release-check/20261031-core-first/review.json`` binds every original hunk
and the source/caller identities. The original eight-hunk patch is preserved under
``20261031-reviewed-candidate/core-inventory/files/91/change.patch``. The new
``increment.patch`` and goal-start source retain a separate reviewable repair.

Original hunk/caller review
----------------------------

REAL-LOCAL-LABEL / GT-LOCAL-LABEL add stack-local Gregorian leap-label detection.
REAL-LABEL-VALIDATION / GT-LABEL-VALIDATION use the validated packed
``YYYYMMDD.HHMMSSmmm`` representation before ordinary Date handling normalizes
seconds=60, and reject non-UTC leap labels. The packed representation and validity
contract were inspected in GregorianDate and the existing Gregorian-to-MJD
helpers. Broad malformed-format acceptance is not inferred.

REAL-LEAP-MAPPING / GT-LEAP-MAPPING preserve the explicit positive UTC leap label:
convert its preceding UTC second to TAI, advance one second, and require the
before/after table offsets to produce a two-second interval across that label.
The existing 1e-5-second transition guard remains unchanged. The precise overload
retains GmatTime through the arithmetic. This is bounded positive-leap handling,
not a new time-scale or negative-leap model.

REAL-OUTPUT-LABEL / GT-OUTPUT-LABEL preserve seconds=60 for UTC output. Inspection
of the surrounding identity and uniform-input paths exposed reliance on caller
output storage. The original local input-label flag alone could not fix those
paths.

Relevant callers include Spacecraft's string epoch input/formatting and
TimeData::SetTimeString; their converter parameters are unchanged. Numeric
SpiceInterface conversions remain on their existing overloads. The generic
numeric converters already know whether a uniform instant lies inside a UTC
leap insertion; the repair requests that information only when needed for UTC
text or an explicit output flag.

Confirmed defects and repair
------------------------------

* TIME-STALE-LEAP-OUTPUT: after a valid leap conversion set the output flag true,
  an ordinary UTC identity conversion reused it. The unmodified library rendered
  ``01 Jan 2009 00:00:00.125`` as ``31 Dec 2008 23:59:60.125``. The flag was still
  true. The new ResetFlag regression fails before the repair.
* TIME-OPTIONAL-FLAG-LABEL: a uniform instant at
  ``31 Dec 2008 23:59:60.000`` rendered as ``01 Jan 2009 00:00:00.000`` when the
  optional output flag was omitted. The new ImplicitFlag regression fails before
  the repair using independent NAIF-derived uniform input.

Each overload now owns a fresh output status. Explicit Gregorian leap input sets
it true; other conversions request the existing detector when producing UTC
Gregorian text or when the caller requests the flag. Identity conversions retain
the original numeric epoch; non-UTC identity status is derived from that instant
when requested. Normal UTC identity clears stale state. Finally, the status is
copied out if requested and UTC text uses the local value regardless of that
pointer. No incoming flag is treated as an input.

Uniform-to-uniform formatting without a requested flag does not acquire a leap-
table dependency. This matters for early initialization and is separately tested.
The formatting defects existed in the goal-start library; this review does not
assert exclusive authorship or that all surrounding legacy code originated in
the preexisting eight-hunk change.

Proof and limits
-----------------

The unchanged LeapSecondInputs contract passes after repair: 128 numeric checks
at the original 100-microsecond bound, 64 UTC-label round trips and fourteen
invalid/non-UTC rejections. Four new contracts pass in both public overloads:

* ResetFlag: eight sequential leap/ordinary identity observations.
* ImplicitFlag: 384 exact millisecond UTC-label checks using four NAIF-derived
  uniform scales, four insertion dates/fractions, and absent/false/true flags.
* IdentityFlag: 128 in/out-of-leap checks with unchanged numeric epochs and both
  initial flag values, across those uniform scales and overloads.
* UniformNoTable: four identity/TAI-to-TT formatting checks without a leap table.

The existing actual-widget SPK LeapSecond scenario also passes offscreen with its
unchanged assertions: eleven resource Applies/missions and 33 independently
checked report rows. Its epoch/state limits remain 1.1ms for Gregorian inputs,
100 microseconds for numeric inputs, 2e-5 km and 2e-8 km/s. These are different
bounds for different inputs, not new relaxed tolerances.

Independent NAIF implementation checks still share the supplied leap history and
A1 offset convention. Physical clocks, all leap history/future policy, negative
leaps, UT1/EOP, submillisecond formatting, nanosecond stopping and all general
invalid-input/exception-atomicity domains remain unqualified. UTC ModJulian inside
a positive leap insertion remains ambiguous; use an explicit Gregorian label or
uniform scale. No broad frame/ephemeris or platform qualification follows.
