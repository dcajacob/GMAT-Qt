95 percent campaign: central-body propagation family
=======================================================

This increment reaches **992/1,158 (85.7%)** source-versioned requirements.
The target remains **1,101**, with **109 more complete requirements** needed.
Historical coverage stays **1,816/2,367 (76.7%)**. All 166 remaining requirements
and 551 historical scopes remain explicit; the goal remains active.

Complete declared body binding capability
--------------------------------------------

``FRR!E145`` requires propagation with respect to any celestial body defined
in FRR-16. The declared inventory is eleven named default bodies and four
configurable user-body types. This increment adds twelve missing trajectory
cells: Sun, Mercury, Venus, Mars, Jupiter, Saturn, Uranus, Neptune, Pluto, and
user-defined Moon, Comet and Planet. Existing Earth and Luna circles and the
independently analytic user-Asteroid harmonic trajectory remain mandatory at
their recorded source/runtime versions. Those passing tests were not rerun.

All twelve new cases pass unattended on native X11 and visibly in the resizable,
non-fullscreen, initially minimized desktop. Each exercises actual CentralBody
text input, pending Cancel and Apply, exact source and complete configured-graph
Undo/Redo, Unicode save/reopen and the read-only output viewer. Each successful
mission has four independent inclined circular samples, three actual Propagate
clones, the prescribed body/type/GM, the correct frame origin and exactly one
named point-mass contributor. Actual native states agree with raw report data.

The admission audit checks the complete fifteen-body/type inventory in both
modes, each exact source/runtime receipt, generic celestial-object name
resolution and the independent numerical results. A separate Cartesian ODE
integration with SciPy DOP853 checks all 576 new Cartesian values without
calling GMAT force, state or frame calculations. Only E145 receives new credit.
This proves body-relative propagation capability, not all force models,
ephemeris sources, numerical domains or astronomical model accuracy. User-body
types intentionally share the existing Ceres SPK as an ephemeris input; they
are configuration fixtures, not claims that Ceres is an astronomical comet,
planet or moon. Removed two-body ephemeris behavior is not restored or credited.

Failures and oracle timing
------------------------------

All thirteen failed native attempts and the initial test compilation failure
are preserved. Early harness/fixture failures came from offscreen registration,
sandbox X11 access, expecting a combo instead of the existing text entry,
automatically allocated spacecraft IDs changing between Builds, and confusing
per-command elapsed-stop durations with cumulative observation times.

The Sun exposed a distinct oracle issue: rounded real-valued A1 timestamps lose
enough duration precision to exceed a 5 mm position comparison at roughly
210 km/s. Its unrounded native propagator and ODE durations explain the
difference. The oracle now integrates over the actual unrounded durations,
independently checking those inputs against both requested and reported clocks.
The original **5 mm**, **1e-8 km/s** and **10 microsecond** bounds remain intact.
All original physical fixture values, observation phases, restoration and
numerical assertions remain. The first passing unattended Moon witness keeps
its earlier reported-A1 oracle and source version; it was not replayed for new
timing diagnostics. The complete visible cohort uses the final timing checks.

Two metadata admission failures are also retained. The final auditor binds the
executed SPK input before every run and reads the family's original explicit
body/state/binding record schema. It does not rewrite native records or relax
the generic NASA facets protocol. Fifteen corrupt proofs are rejected, including
missing original types/modes/retained cells, altered clocks or Cartesian values,
incorrect native force/state identity and unrelated source edits.

Product impact and retention
-------------------------------

No production/core file changed. All 104 pending core paths and the preceding
four Qt path repairs remain unchanged. Changes comprise a new test driver,
fixture and admission auditor, native CTest registrations and the cumulative
qualification dispatch. Complete current-build, independent core/scientific
and full NASA qualification remain open.

No passing screenshots or runtime binary copies were retained. Unreferenced
duplicate reports and traces were losslessly compressed; qualification inputs,
raw results, failures, exact source archives and receipts remain available.
The retention audit records original and compressed hashes and allocated sizes.

Watch or automate
---------------------

::

   cmake --build build/linux-gui --target GmatQtNasaCentralBodyPropagationTests -j2
   ctest --test-dir build/linux-gui -R '^QtGui.NativeNasaCentralBodyPropagation' --output-on-failure
   python src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
     --filter '^QtGui.NativeNasaCentralBodyPropagation' \
     --visible --display-route nested --display-scale auto --start-minimized \
     --pause-ms 0 --max-checkpoints 100 --max-screenshots 0 --report-every 6 \
     --nasa-review build/nasa-coverage/coverage95-20261101/body-propagation-runtime-review.json \
     --output build/gui-acceptance/central-body-watch

Restore the minimized desktop to watch. CTest uses private X11 when unattended;
it needs an available X11 platform. The runtime-only review grants no cumulative
NASA credit by itself. Admission additionally needs NumPy and SciPy (the audited
environment used SciPy 1.16.3). Cumulative credit comes from the complete union.

The other sixteen original science-matrix obligations remain unqualified.
The conservative prerequisite inventory still contains 59 holds; if all persist,
even completing every other row reaches only 94.9 percent. At least two held
obligations must become provable to reach 95 percent. No licensed MATLAB engine,
historical operating system, unspecified numeric target or retired behavior is
inferred from the new Linux body-binding evidence.
