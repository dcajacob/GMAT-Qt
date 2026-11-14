Interrupted solver ephemeris and same-session recovery
=====================================================

Twelve new native GUI passes cover Target/DC and Optimize/Yukon, each with a
Stop command, the actual debugger End mission button, and a runtime sqrt(-V)
error. Every case has three phases in the same MainWindow: interrupted trial,
successful recovery, and another success without Build. Visible and unattended
results agree: 12 interrupted phases write zero OEM data rows, while 24 recovery
phases each write exactly 25 final-solution knots at 0:5:120 seconds.

The debugger stop occurs after an inner accepted pass while the enclosing solver
is still a trial. It must use the visible debuggerEnd button and return Stopped.
Its first recovery also runs without Build. The script Stop cases return Stopped;
the runtime negative-square-root cases return Failed with a native sqrt
diagnostic. Neither reaches Done or changes Keep from 41 to 99. Recoveries do
reach Done and Keep=99. Script faults are removed by the exact retained source
transformation before their first recovery; the second recovery never rebuilds.
Source/history and execution-lock checks apply to all phases.

The audit independently checks all 600 OEM knots/3,600 components, 18,400 raw
state publications and 736 raw trial passes. Trial radii derive from raw U,V
reports as 7000+100*U+10*V, with literal mu=398600.4415. Accepted U=2,V=3 gives
radius=7230. Both numerical solver clones must be FINISHED for a pass to count
as globally accepted. Each successful run has exactly one such pass; interrupted
runs have none. State bounds remain 2e-5 km and 2e-8 km/s, and the output schedule
bound remains 0.002 seconds. The second success checks against stale or appended
output from the previous run, not merely file existence.

No production code changed. The four-file nested-output repair from the prior
stage is byte-identical. No previously passing native family was replayed.
The six new registrations are
QtGui.NasaInterruptedEphemeris{NestedDC,NestedYukon}{StopCommand,DebuggerStop,RuntimeError}.
They are dependencies of the acceptance-build targets.

Stage ``build/nasa-coverage/coverage95-20261103-interrupted-ephemeris`` retains the
original compile failure and source-v1: QtTest was included after the retained
helper introduced a duration name that collided with QtTest's chrono template.
Source-v2 moves the three Qt includes before that helper. Every test assertion
and numerical reference is unchanged. All six native cases pass on their first
execution in each mode.

Audit v1 binds 412 regular files, excluding runtime copies and symlinks. It checks
fixture origins and exact fault/recovery transformations, receipts/JUnit/stdout/
raw proof agreement, before/after source/image identities, all six mapped visible
processes, numeric references and nested membership. Visible windows were initially
minimized, resizable and nonfullscreen; screenshots=0. Five corrupt-proof controls
reject an aborted knot, missing recovery, wrong outcome, executed continuation and
stale appended knot. All native selections are terminal.

Build and replay from the repository root::

   cmake --build build/linux-gui --target GmatQtNasaInterruptedEphemerisTests -j 4
   python3 src/qtgui/tests/RunInterruptedEphemeris.py
   python3 src/qtgui/tests/RunInterruptedEphemeris.py --visible
   python3 src/qtgui/tests/AuditInterruptedEphemeris.py \
     --stage build/nasa-coverage/coverage95-20261103-interrupted-ephemeris \
     --output /tmp/interrupted-ephemeris-recheck.json

FRR!E406 remains open. The stage's exact-row worklist distinguishes currently
proved DC/Yukon OEM membership from candidate older batch/format evidence and
still-missing terminal-nonconvergence, estimator interruption/recovery and
independent filter/smoother STK membership checks. Do not replay passed DC/Yukon
regimes or require unrelated arbitrary-orbit science to substitute for this
output-membership obligation. Reaudit candidate evidence at recorded source
versions and add only genuinely missing cases.

Coverage remains 1,019/1,158 (88.0%), 139 open and 82 needed. Historical coverage
remains 1,816/2,367 (76.7%). No requirement key is assigned. The denominator,
fingerprints, 59 holds and separate scientific/core/current-build/full-NASA
qualification gates remain intact.
