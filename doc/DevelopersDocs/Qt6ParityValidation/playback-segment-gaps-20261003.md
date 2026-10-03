# Animation pauses between retained segments — 2026-10-03

The user reported pauses at segment transitions in multi-segment animations,
including Mars B Plane. Replay used the cumulative publisher callback serial,
while Current solver mode removes trial samples without renumbering retained
frames. The resulting empty intervals occupied playback time. No calculation
wall-clock time is recorded in this replay mechanism.

`PlotWidget::updateReplayFrame` now maps the slider to the sorted unique frame IDs
of retained curve samples. Original sample IDs remain unchanged, so cameras,
vectors, provider identities and breaks retain their original association.
Both shared and local controls use this mapping. The index is cached until the
publication serial, history generation or retained point count changes; trailing
trial removal, retention trimming and reopened views are covered.

## Mars B Plane affected example

One private offscreen probe uses the rebuilt frontend and the unchanged shipped
`Tut_Mars_B_Plane_Targeting.script`, translated through the existing OF converter.
It completes once in 2.018 s. Each of its three views retains 1,178 unique frames
between 5,977 and 7,872. A 478-frame hole separates **Prop 3 days** (frame 6,454)
from **Prop 12 Days to TCM** (frame 6,933). Under the old mapping it consumed
25.224% of playback, approximately **0.757 s** of the nominal three-second sweep.
At 50%, the old mapping selected empty frame 6,924; the repair selects retained
frame 7,129. Eight shared/local positions per view and all 100 master animation
advances now select retained samples and finish at Latest.

The original sample SHA256
**f334cf650aaa435f132de1d8e7823333378a0d40cd179e0bb0e3143af4ff3178**
matches its frozen example receipt. `DifferentialCorrectorDC1.data` is
**byte-identical** to that source-matched prior run, SHA256
**165c8d5cea3af50829ad9bb855c6220243e847ce2402375eb0f1024fac083350**.
No extra numerical reference run was needed. The standalone probe checks replay
selection through Qt controls; this is not a native input/rendered-pixel test.

The [compact Mars receipt and complete raw-file manifest](playback-segment-gaps-mars-20261003.json)
and [standalone probe source](playback-segment-gaps-mars-probe-20261003.cpp) are
retained. Full raw receipt, exact compile/link commands, converted source,
private startup/output/settings and stdout/stderr remain under
`build/linux-gui/playback-segment-gaps-20261003/raw/`. The one affected example
run does not repeat tutorial authoring or the shipped-example corpus.

## Focused regression and actual rebuilt runtime

The new synthetic regression first failed with **Replay paused in discarded
solver frames between segments**, 0.18 s. Its initial compile attempt needed the
explicit `XyPlotClearFromBreak` arguments; that test-only signature correction
preceded the reproduced runtime failure. The corrected case retains frames
1, 2, 203, 204 and 205 after discarded trials. At 50%, all Orbit/Ground/XY views
now select frame 203 instead of dwelling in the discarded interval. It also
checks a removed trailing trial interval through cumulative frame 405, incoming
samples and cleanup at the same serial, local controls, close/reopen and
MaxPlotPoints trimming. The numerical arrays and retained IDs are not changed.

After the application repair, only directly affected checks ran:

| Check | Result |
| --- | --- |
| QtGui.SharedPlayback | PASS, 0.20 s |
| QtGui.OwnedPlotBatch | PASS, 0.07 s |
| QtGui.SolverPlots | PASS, 1.30 s |
| QtGui.Plots | PASS, 2.63 s |
| QtGui.PlotsHiDPI | PASS, 2.92 s |

All five pass in 7.13 s. These are offscreen checks, not native host qualification.
The build targets were GmatQt and the four affected test executables; no full
matrix, corpus or tutorial construction was repeated.

Rebuilt `application/bin/GmatQt` resolves to GmatQt-R2026a, **6,151,088 bytes**,
SHA256 **af0c42cfecd5f7b7c0109b69fe656548afecad2ee83fe874caae365def1e4948**.
The base library **2082a341f5f123e81bad362972cd2b28a0a4753047ae51b5c09609e246be430c**,
util **f9844517a2de0dfa1ddbd7b8180a63452aa6917a8fcd0b87217f304547a717a8** and
startup **5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a**
remain byte-identical to the previous runtime. There is no numerical-engine,
mission-physics or solver-setting change.

Raw [baseline failure](playback-segment-gaps-baseline-20261003.txt),
[application/test build](playback-segment-gaps-build-20261003.txt) and
[affected checks](playback-segment-gaps-checks-20261003.txt) are retained.

Live host testing remains stopped. The saved crash establishes Mutter's null
Wayland resource fault, but GMAT client attribution, a safe/repaired host and
physical-GPU acceptance remain unproven. Windows/macOS/MATLAB stay deferred;
all nine original acceptance gates and their remaining limits stay in scope.
Full Linux GUI qualification is not claimed.
