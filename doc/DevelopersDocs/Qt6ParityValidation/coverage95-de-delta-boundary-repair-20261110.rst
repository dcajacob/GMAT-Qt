DE position-difference and record-boundary repair
================================================

The next bounded increment addresses the cross-granule/record delta gap left
open by the DE file-domain review. It supports the existing FRR!E13 and FRR!E144
work without claiming either complete parent requirement. The stopping condition
was a fixed independent boundary matrix plus the directly affected existing DE
and lunar controls. No full-catalogue regression was repeated.

DeFile::Interpolate_State_Delta previously evaluated both endpoints with the
first endpoint's polynomial, even when the second endpoint belonged to another
granule or record. Separately, record selection collapsed a distant split-epoch
difference into one double before taking a ceiling. That could lose a 50 ns
boundary offset, select the wrong record and produce extremely large state
and delta errors after a distant cached query. Both scalar and precise reads
could encounter this record-selection error.

The shared reader now reduces whole days before testing the remaining split
epoch. Scalar record reads delegate to that same selection path, and a loaded
record must actually contain the requested epoch. Cross-granule/record deltas
select each endpoint's own coefficients and difference the two series with a
coupled recurrence and compensated summation. The existing same-granule delta
recurrence and ordinary state/libration interpolation formulas are unchanged.
No clock model, ephemeris coefficient, class layout or virtual ordering changed.

The new DeFileDeltaBoundaryTests independently decodes the supplied little-endian
DE405 and uses split-epoch, long-double Clenshaw evaluation. Its 440 scenarios
cover Moon, Mars, Sun, Mercury and Earth; TT override and TDB; forward/reverse
requests; caches warmed from earlier/later epochs; granule and record crossings;
exact boundaries, file endpoints, wide spans and 100 ns intervals. Reference
Earth-centered positions use the file's mass ratio. Fixture A1 epochs use the
native clock conversion, whose preceding MathSpec controls remain separate.

The unchanged pre-repair backend fails 185 scenarios. The identical reproducer
binary passes all 440 with the repaired backend and unchanged limits: 2e-6 km
for wide deltas and states, and 2e-9 km for short deltas. Maximum short-delta
error is 1.538e-10 km; maximum wide-delta error is 2.535e-8 km. Maximum precise
and scalar state errors are 4.253e-8 km and 3.994e-8 km. Earlier reproducer
versions and their failures remain archived; later versions added state controls
before changing production and did not relax any limit.

The existing DeFileDomain control passes all 619 checks against the repaired
backend, retaining its endpoint, invalid/recovery, local-delta and clock limits.
Both original lunar orientation workflows pass headless with their unchanged
105-row reference matrices: DE405 across two lifecycle phases and IAU2002
across three. Loader traces confirm the new backend was actually loaded.

CMake now registers QtAcceptance.DeFileDeltaBoundaries with the existing
support/core labels and a 60-second limit. The actual generated target compiled
and linked, and its registered CTest invocation passes all 440 scenarios. The
private configure's initially missing templates and the initial missing support
object in the manual link are retained setup failures; final configuration and
target integration pass without changing product or test assertions for them.

Evidence and the exact three-file patch are under
build/nasa-coverage/coverage95-20261110-de-delta-boundaries-v1, especially
verification-summary-v1.json and repair-v1.patch. Only the owning DeFile object
was recompiled for the backend; other objects and runtime dependencies came from
the preceding coherent aggregate. All 1,334 authorized original test inputs,
593 saved aggregate runtime inputs and both canonical acceptance records remain
unchanged. The two named observer-test amendments remain the only historical
test-input exceptions. No installed runtime, package or release was updated.

This is selected shipped-DE405 coefficient consistency on Linux, not physical
ephemeris accuracy, all providers/platforms or complete representation/FK5
qualification. Long-double precision differs by platform. The earlier 4,060-case
full result remains bound to its own frozen source snapshot; this later patch
has targeted verification only. Recorded acceptance remains 1,078/1,158 (93.1%),
with 80 open scopes. No new requirement credit, denominator change or canonical
admission was recorded; the broader campaign remains incomplete.
