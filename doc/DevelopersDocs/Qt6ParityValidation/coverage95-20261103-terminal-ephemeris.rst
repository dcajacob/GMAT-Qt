Terminal-pass membership and convergence status
==============================================

Two new cases pass unattended and visibly, giving four GUI passes and eight
phases. Nested Target/DC uses MaximumIterations=2; Nested Optimize/Yukon uses
MaximumIterations=1. Each iteration-limited outer clone terminates FINISHED
with EXCEEDED_ITERATIONS, while its inner Target converges. Output contains
exactly one 25-knot terminal trajectory at radius=7130 km (U=1,V=3). Increasing
only the iteration limit in the same window produces the converged 7230 km
trajectory (U=2,V=3), again with exactly 25 knots at 0:5:120 seconds.

The distinction is explicit: FINISHED and terminal-pass membership are not
assertions of convergence. Native status=-1 is required for the limited outer
clone, status=1 for the inner clone and both recovered clones. The original guide
allows applying last calculated values after nonconvergence and describes final-
solution ephemeris output. This stage proves native terminal-pass filtering;
it does not decide that a nonconverged iterate satisfies the exact original
requirement's mathematical meaning of solution. FRR!E406 remains uncredited.
No production behavior was changed to impose an unsupported convergence-only
output rule.

The independent audit checks all 200 OEM knots/1200 components, 4,600 raw state
publications and 184 trial passes. Trial radii derive from raw U,V control reports
as 7000+100*U+10*V with literal mu=398600.4415. Final controls also have separate
literal checks (1,3) and (2,3). Every raw publication and output knot is compared
to its circular reference. Position/velocity bounds remain 2e-5 km and 2e-8 km/s;
the OEM schedule bound is 0.002 seconds. Exactly one pass has both native solver
clones FINISHED; intermediate, perturbation and locally accepted inner passes
are separately observed. Actual debugger boundaries, Unicode reopen, exact
source/history and released execution locks are required.

Stage ``build/nasa-coverage/coverage95-20261103-terminal-ephemeris`` has 140 regular
artifact bindings. Source-v1, executed-source-v1, matching before/after identities
and both visible process maps bind exact execution. Native receipt/JUnit/stdout/
proof, raw controls/publications, metadata, source derivation and five corrupt-
proof controls are checked. The negative controls reject convergence confusion,
a wrong terminal control, trial output, missing recovery and an extra knot.
Both cases pass their first native execution in each mode. Visible windows are
initially minimized, resizable and nonfullscreen; screenshots=0. No production
source changed or older passing family was rerun. All sessions are terminal.

Build and replay from the repository root::

   cmake --build build/linux-gui --target GmatQtNasaTerminalEphemerisTests -j 4
   python3 src/qtgui/tests/RunTerminalEphemeris.py
   python3 src/qtgui/tests/RunTerminalEphemeris.py --visible
   python3 src/qtgui/tests/AuditTerminalEphemeris.py \
     --stage build/nasa-coverage/coverage95-20261103-terminal-ephemeris \
     --output /tmp/terminal-ephemeris-recheck.json

The ordinary registrations are QtGui.NasaTerminalEphemerisNestedDC and
QtGui.NasaTerminalEphemerisNestedYukon; their target is an acceptance-build
prerequisite. The updated E406 scope worklist preserves its original fingerprint
and lists independent every-row filter/smoother STK membership, batch-estimator
interruption/limit/recovery and remaining supported format/backend boundaries.
Reaudit existing batch-format and SPK evidence at its recorded versions. Preserve
the old one-step batch state/SVD predicates and the explicit nonconvergence
status; do not replace them with truth accuracy after one iteration.

Coverage remains 1,019/1,158 (88.0%), 139 open and 82 needed. Historical coverage
remains 1,816/2,367 (76.7%). No requirement key is assigned. The denominator,
fingerprints, 59 holds and scientific/core/current-build/full-NASA gates remain.
