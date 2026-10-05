# Saved OF playback clock, synchronization and toolbar options

Qualification evidence, 2026-10-04, from c1c5f3d6 in `/home/dan/GIT/GMAT-Qt`, branch `codex/qt6-gui`. This records the bounded operation below, not full replacement qualification. The original `/home/dan/GIT/GMAT/GMAT` checkout remains untouched.

## Legacy contract and provenance

Source references below are relative to `/home/dan/GIT/OpenFramesInterface/` unless another root is given. Verified source identities: `src/base/subscriber/OpenFramesInterface.cpp` SHA256 `3674716e61175f3d20f02c98408ab72e1c77b9f3c9e2f0b37d97f4c1b0514ddf`; `src/gui/subscriber/wx/OFScene.cpp` `6daf7ee34c943b124dc9f2c35086c7aa97a67a2967dd60f849a9727e4fccb156`; `src/base/subscriber/CoordinateOptions.cpp` `9f201033cbfe2a42a755f46cf2c4c60340404eb7fe9e6b895dbb00a8f982b0ea`.

- The initial rate is **3600 simulated seconds per wall second** (`CoordinateOptions.cpp:40–47`). OFI initializes `ShowToolbar=true` and no synchronization parent (`OpenFramesInterface.cpp:212,218`; default-constructed parent name is empty). `PlaybackTimeScale` is a real setter without a positive-only guard (`OpenFramesInterface.cpp:2494–2496`). `OFScene.cpp:2257–2260` divides that rate by seconds/day before supplying the day-valued clock. Signed rates reverse time; zero holds it.
- `TimeSyncParent` stores a subscriber name (`OpenFramesInterface.cpp:1424–1427`), participates in reference enumeration (`2772–2774`), binds another OpenFramesInterface (`2888–2894`), and follows subscriber Rename (`980–986`). The active wx selector offers `Clock Time` or another interface (`OFConfigurationPanel.cpp:400–408,1305–1311`). The scene detaches for an empty parent or attaches to the parent's WindowProxy (`OFScene.cpp:1156–1165`); circular synchronization is rejected (`681–692`). Parent clock controls address the ultimate parent rather than matching independent percentage positions.
- Ordinary, non-solver publications grow their scene's min/max envelope and set/pause its current time (`OFScene.cpp:1958–1980,2136–2156`); reset clears that envelope (`984–990`). A synchronized group's most recent publisher supplies **that publisher's own lifetime regular-data envelope**, rather than a union of all viewers' intervals. The underlying clock source is `/home/dan/GIT/OpenFramesInterface/dep/OpenFrames-git` at verified HEAD `037643090d42c3ff804584f69b13b3b6511a8b66`: `src/WindowProxy.cpp:800–853` recursively forwards time/range/pause/rate to the ultimate parent; `857–891` copies inherited epoch/rate/pause/range on detach and rejects cycles; `1072–1087` advances by wall seconds times rate and clamps without wrapping or automatic pause. `include/OpenFrames/WindowProxy.hpp:221–256` forwards corresponding getters.
- `ShowToolbar` is an active saved Boolean (`OpenFramesInterface.cpp:1900–1901,1935–1937`; wx `OFConfigurationPanel.cpp:1323`). Its consumer hides the whole toolbar sizer (`OFWindowProxyPanel.cpp:254–264`). Assignments to all three settings retain normal source order: the final assignment wins.

## Implemented operation

Converted OF plots now retain `PlaybackTimeScale`, `TimeSyncParent` and `ShowToolbar` in `QtCameraSetting::playback`; their original assignments remain as source comments. Every converted OF receives legacy defaults 3600/independent/visible. Original Qt OrbitView scripts without playback metadata retain the existing approximately three-second percentage history sweep. The global percentage transport remains a separate deliberate Qt operation; it takes presentation ownership and pauses local simulation clocks, while a local clock action restores absolute-epoch presentation.

The strict nullable `playback` bundle contains `timeScale` (finite signed number or null), `syncParent` (empty independent name or bare resource identifier), and `showToolbar` (Boolean or null). An entirely absent/empty bundle normalizes to no override. Toolbar-only metadata leaves the existing Qt clock unchanged. A nonempty parent requires a clock rate; runtime validation requires existing OrbitView references and an acyclic graph. Converted child/parent plots share the ultimate parent's absolute **A1MJD** rate, epoch, range, pause, interpolation mode and speed multiplier. Zero holds time; negative reverses. Play from a paused applicable endpoint restarts at the opposite endpoint; subsequent ticks clamp. Following legacy `OFScene.cpp:2203–2220`, zero takes the forward branch: paused zero-rate Play at maximum resets to minimum and then holds. Repeated Play while already playing does not restart the clock. The clock controller keeps inherited state when a child detaches; saved graph changes apply when subscribers are configured. Viewer close/reopen retains group state; subscriber deletion/clear discards publication state and avoids an orphaned timer.

The OrbitView **Playback options…** dialog makes these settings pending until Apply. Cancel/discard is reversible; invalid rates, missing/wrong-type parents and cycles reject the complete transaction. Metadata participates in source retention, Save/reopen, Undo/Redo, resource creation and nested parent-reference Rename. Hiding toolbars leaves a visible restore button and a local `Ctrl+Shift+T` restore action. Local speed buttons/multiplier and the finite signed `Simulation s/s` field operate on the shared clock; an explicit rate edit resets the multiplier to one.

Implementation authorities: `src/qtgui/ScriptCompatibility.hpp/.cpp` (conversion/schema); `ResourceRename.cpp:290–293` (nested parent reference); `OrbitPlaybackOptionsDialog.cpp`, `ResourceEditor.cpp`, `MainWindow.cpp` (pending source transaction); `PlotSimulationClock.hpp`, `QtPlotReceiver.cpp`, `PlotWidget.cpp` (clock graph, publication and controls); `PlotEpochPlayback.hpp` and frozen `PlotModel.hpp` histories (per-object absolute-epoch presentation).

## Pose and history boundary

A shared epoch resolves each object's own recorded propagation arc, pose and attitude, plus frozen camera inputs and vector samples. Separate providers, backward arcs, gaps and sticky overlapping-arc selection remain explicit. Presentation uses frozen data rather than querying the mission engine during seek/replay. Regular raw publication callbacks supply envelope/current epoch independently of collected plot decimation; solver trials do not extend this accepted-time envelope.

This is still the Qt **progressive history** presentation, not a claim of literal full-path OpenFrames visuals. Compact raw first/last arc captures preserve endpoint poses and LIMIT selection when collection or MaxPlotPoints discards samples. They do **not** retain or reconstruct the complete raw trajectory between those endpoints; interpolation remains a display approximation between available frozen samples/endpoints. Propagation steps selects recorded presentation samples. Exports and numerical reports remain outside clock presentation mutation.

## Verification and retained corrections

Eight new checks and five directly affected regressions pass across the recorded
builds. The final application was rebuilt after the explicit-frame correction;
only newly affected/failing cases were rerun. Earlier passes remain bound to their
recorded build, rather than being relabeled as fresh full-matrix evidence.

| Check | Final relevant result | Raw log |
| --- | --- | --- |
| QtGui.NativeInterpolatedPlayback | PASS 0.51s | tests-01.log |
| QtGui.ResourceRename | PASS 0.71s | tests-01.log |
| QtGui.SharedPlayback | PASS 0.22s | tests-01.log |
| QtGui.InterpolatedPlayback | PASS 0.08s | tests-01.log |
| QtGui.NativeEpochPlayback | PASS 0.59s | tests-02.log |
| QtGui.PlaybackOptionsBackend | PASS 0.22s | tests-01.log |
| QtGui.PlaybackOptionsEditor | PASS 0.38s | tests-01.log |
| QtGui.PlaybackOptionsRuntime | PASS 0.21s | tests-02.log |
| QtGui.SimulationClock | PASS 0.02s | clock-test-03.log |
| QtGui.EpochPlaybackRendering | PASS 0.22s | tests-02.log |
| QtGui.EpochPlayback | PASS 0.03s | tests-02.log |
| QtGui.ClockArcCapture | PASS 0.27s | tests-02.log |
| QtGui.UnplottedBodyCameras | PASS 0.30s | tests-02.log |

The first build compiled the application but stopped at a missing test include;
`OrbitObjectAxes.hpp` supplies the fixture's pose helper. The first helper test
used a 1e-8 tolerance for a 100-unit eye offset transformed through the existing
Qt float quaternion/DCM. Diagnostic output is `107.00001192092896,
0.000011920928955078125` against `(107, 0)`; the two rotation assertions now use
2e-5, while strict position tests remain. No numerical rotation algorithm changed.

The existing undrawn-body camera check reproduced a real regression: active
simulation state overrode an explicitly requested finite historical frame. Camera,
object/vector/path/marker/label helpers now use simulation state only for the
absolute-playback sentinel, preserving finite frame queries. Its original test is
unchanged and passes. The clock fixture was also corrected to pause before
asserting reverse endpoint restart, matching the verified legacy resume rule.
All original failure output remains in the archive.

The new short point-mass mission propagates backward 37 seconds and then forward
to +53 seconds with `DataCollectFrequency=3` and `MaxPlotPoints=2`. Clock-only
metadata requests real accepted arcs; raw endpoints match independent Report
values, bounds survive collection, and frozen body attitude matches the engine.
After seeking/interpolating, the complete 315-byte report and 1,484-byte retained
sample serialization remain byte-identical. This is a new focused fixture, not a
repeat of the shipped Mars/example/tutorial corpus. The camera regression also
keeps its complete independent state report identical. Source and engine objects
are not queried or modified during retained replay.

Independent native/fallback pixels verify unequal object intervals, uniform
epoch motion, sticky overlaps, path tip/vector location and tracked camera motion.
Native checks use private Xvfb with `LIBGL_ALWAYS_SOFTWARE=1`; the separate renderer
receipt reports llvmpipe (LLVM 21.1.8, 256 bits), Mesa26.0.8, accelerated=no.
Dialog and local control screenshots were visually inspected. No live desktop
window, physical GPU, Mutter test or system change is involved.

The [machine receipt](playback-options-20261004-receipt.json) records source and
executable identities, final relevant outcomes, before/after comparisons and the
[complete raw archive](playback-options-20261004-raw.zip), including build logs,
first failures, source/report files, serialized histories and screenshots.

Final `application/bin/GmatQt`: **6,718,016 bytes**, SHA256
`6cfca8433843a301c4bc73b9481ad5b41a56473a6950733ad5c2ab6ed58496d2`. Base/util/Qt startup hashes remain unchanged.
The original `/home/dan/GIT/GMAT/GMAT` checkout remains untouched.

## Remaining acceptance scope

Offscreen or private software X11 evidence does not qualify live-host GNOME/Wayland,
the physical GPU or Mutter. Live host testing remains stopped: the saved crash
identifies a null Wayland resource fault in Mutter; GMAT client attribution and
host/GPU acceptance remain unproven. Per-plot `DrawModelFile` is still an active
unmet OF source preference. Device VR/MSAA, inactive AxesLength and the held
scientific/external-data cases retain their separate dispositions. All nine
replacement gates retain their full scope; full qualification is not claimed.
Windows, macOS and MATLAB remain deferred.
