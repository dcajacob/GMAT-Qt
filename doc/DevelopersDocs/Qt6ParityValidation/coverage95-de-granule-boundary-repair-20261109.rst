DE record and granule boundary repair
=====================================

The original IAU2002 lunar workflow corrupted its prescribed inertial state
after moving to an earlier epoch. A passive native trace of the same 842-command
mission found the first Cartesian assignment producing a position of order
1e45 km. No mission command or input value was changed for that trace.

The DE reader's descending interval searches used a strict lower-bound
comparison. At an exact first-granule boundary approached from a later cached
record, no segment was selected and the normalized polynomial time used zero
as its origin. The reader extrapolated far outside the intended interval.
Seven interpolation paths now include the lower boundary. The two coefficient
reload guards also use inclusive lower comparisons. Coefficients, clock
offsets, class layouts and virtual ordering are unchanged.

A public-API probe reproduced eight failures in 48 backward-ordered samples
against the supplied DE405 CSPICE kernel. The same probe now passes all samples;
maximum position/velocity differences are 1.536e-7 km and 3.961e-13 km/s.
Both original 105-row lunar models pass headless and visible. DE405 completes
two lifecycle phases and IAU2002 three, with all original inertial-input and
5e-7 km / 5e-10 km/s model limits intact. Related SSB/calculated-point controls
also pass: 46 original GUI executions. The ordinary loader regression passes.

Whole-file endpoint handling remains open. A separate probe finds valid data
instants shortly after the first coefficient epoch rejected by an early A1-time
check. The stored file-begin offset also needs correction. The supplied SPK has
no coverage at that earliest epoch, so it provides no numerical reference there.
The current repair does not qualify that domain or general physical accuracy.

Review 061/checkpoint 048 bind the evidence under
build/nasa-coverage/coverage95-20261108-lunar-state-transition-current-review.
All 1,334 original inputs, reference identities and canonical campaign artifacts
are intact. Acceptance remains 1,078/1,158 (93.1%); full rebuild/regression and
unresolved requirements/providers/platforms remain open.
