Defined celestial-body parameter qualification
==============================================

FRR!E501 and FRR!E502 join the original 22 parameters with all 11 default
origins and four user-body factory types. The existing qualified EA/MA and
RMAG/semilatus rectum runs remain source-versioned, as do the Earth parameter
and Earth/Mars beta-angle cases. The new independent conic, geodetic, clock
and beta checks cover the other routes. Sun-origin beta rejects its undefined
Sun-to-Sun direction before reporting a value or running the next command.

Three narrow repairs were checked against the original bounds. The apoapsis
and periapsis readers preserve the existing origin-relative state where it
is valid, while active publication samples retain priority. User moons
with explicit orientation use their own cartographic parameters, independent
of their orbit's central body. The original built-in Luna IAU case still
passes. Earth sidereal time subtracts the J2000 epoch before introducing
full-Julian-date rounding. An omitted Neptune periodic term was corrected in
the new independent expectation; the original raw execution was reused.

The audit retains the earlier precision and oracle failures. It does not
credit frame-dependent HA, arbitrary frame choices, an entire ephemeris
provider, a physical model, or the full current build. No original
assertion, tolerance, fingerprint, or denominator changed.
