# OF LastN solver display: bounded design lead

Read-only source review at GMAT-Qt b2b7c52e on 2026-10-04. No tests, numerical runs or repository edits were performed. This is an active operation to implement, not qualification evidence.

## Observed gap

`src/qtgui/ScriptCompatibility.cpp:427–451` passes `SolverIterations` to the converted OrbitView unchanged and comments `SolverIterLastN`. The active legacy `LastN` choice therefore reaches `src/base/subscriber/Subscriber.cpp:1372–1420`, whose accepted values are All, Current and None. A LastN import cannot build through this route. The wx controls are active: `OpenFramesInterface/src/gui/subscriber/wx/OFConfigurationPanel.cpp:455–490,1068–1072,1366,2207–2230`.

## Exact legacy behavior and ordering

The paths below are relative to `/home/dan/GIT/OpenFramesInterface/`.
Its OpenFramesInterface.cpp SHA256 is
`3674716e61175f3d20f02c98408ab72e1c77b9f3c9e2f0b37d97f4c1b0514ddf`.
The separate R2026a reference source under
`/home/dan/Downloads/GMAT/GMAT-src_and_data-R2026a/GMAT-R2026a/depends/OpenFramesInterface/`
has SHA256 `1eb0d0dfdcc54b7e7ccf87a77a0c1729e8dfbf9a28342fc6dcab678bab7b61ef`;
its corresponding mode setter, nonnegative count setter and initialization
also retain the behavior below. Line numbers here refer to the GIT reference.

- `src/base/subscriber/OpenFramesInterface.cpp:179–182` defines All, LastN, Current and None. Constructor lines213–214 starts in All with backing LastN count1. `SetStringParameter` lines1354–1361 resets the backing count to0 for All/None and1 for Current; LastN retains its previous backing count.
- `SetIntegerParameter` lines2226–2241 accepts any nonnegative integer but changes the backing count only while mode is LastN. Negative values fail even in another mode. Consequently source order matters: a count written before LastN is ignored; Current then LastN retains1; explicit All then LastN retains0. The constructor's initial All/backing1 differs from explicitly assigning All. The getter lines2173–2181 returns0 for All/None,1 for Current, and the backing count for LastN.
- Initialization lines3179–3184 supplies a maximum of **one solver trajectory for Current**, or `N * TRAJ_PER_SOLVER_ITER` for LastN. The constant is2 in `src/base/subscriber/OpenFramesInterface.hpp:62`. Thus LastN1 is a two-trajectory cap, not Current's one-trajectory cap. LastN0 means **unlimited**, because `src/gui/subscriber/OFSpaceObject.cpp:785–794` prunes only when the maximum is positive.
- `src/gui/subscriber/TrajectoryDealer.cpp:90–139` creates and extends separate monotonic solver trajectories; nonmonotonic/duplicate epochs do not simply form new solver iteration groups. End-of-receive in OFI lines3352–3382 finalizes a solver trajectory. `OFSpaceObject.cpp:771–799` shows the newly finalized trajectory before removing oldest ones. The budget is per object's finalized solver trajectories, not a demonstrated complete optimization or targeting iteration identifier.
- On SOLVEDPASS, OFI lines3361–3379 alternates removal of floor/ceil(2/2), one recent solver trajectory on each of the two callbacks, then finalizes the accepted regular trajectory. This cleanup must remain distinct from the LastN cap. Empty/in-progress solver trajectories must not consume a finalized visible-trajectory slot.
- The wx mode-change handler sets a zero LastN entry to1 when selecting LastN and disables the count for other modes (`OFConfigurationPanel.cpp:2207–2230`). The script-level zero remains valid/unlimited.

Do not market the operation as retaining exactly N complete numerical iterations: the legacy implementation budgets two trajectory pieces per requested iteration, regardless of solver method or nested structure.

## Frontend implementation boundaries

A conversion/Qt metadata setting can retain the requested LastN mode and count while supplying **All** to the existing numerical subscriber so trial samples are available. Add a pending LastN selection/count control to the Orbit resource editor; preserve unrelated source, literal expressions, source-order import semantics, Undo/Redo and explicit setting changes. Strict metadata parsing should validate the supported mode and nonnegative integral count. Current/None/All behavior must stay independently defined rather than treating LastN1 as Current.

The existing Qt `solverScope`/`solverBreaks` tags do **not** provide Orbit iteration identities. They are used for XY and Ground Track (`QtPlotReceiver.cpp:997–1039,1098,1147–1173`); OrbitPlot's TakeAction (`src/base/subscriber/OrbitPlot.cpp:636–671`) does not forward SolverScope/MarkBreak. Do not infer a LastN implementation from those tests.

The raw presentation metadata already has provider identity and a trial bit (`OrbitPlot.cpp:2194–2237`; QtPlotReceiver's SetOrbitProvider) and an explicit `FinalizeOrbitArc` callback (`OrbitView.cpp:2135–2152`). A LastN request can opt into that existing metadata route and group each object's received trial points by finalized trajectory, independent of accepted regular-arc identity. Cap only trial groups; never remove accepted points or regular segment labels, and keep mission reports and numerical calculations unchanged. Keep corresponding camera/vector histories and replay cursors consistent when a trial group is discarded, and clear the bookkeeping on normal data/reset lifecycle operations.

The current callback payload contains a boolean trial flag, **not a distinct SOLVEDPASS state**. Exact accepted-pass cleanup cannot be established by guessing from an epoch reset, provider change or generic trial-to-accepted transition. If that legacy cleanup is included in the claim, first establish sufficient presentation lifecycle metadata, or record the remaining limitation explicitly. Adding a display callback is separate from changing solver/numerical algorithms; no algorithm rewrite is needed or justified.

Complexity is moderate and larger than the coordinate-label repair: conversion and controls are small, but trajectory lifecycle, accepted-pass cleanup and replay/history consistency require careful bounded evidence. Prioritize it after the current receiver omission and simple coordinate-label controls.

## Focused future verification

1. New conversion fixtures: default backing1; ignored pre-LastN count; explicit All/Current resets; LastN count0; invalid negative/noninteger values; original-source preservation on rejection.
2. Synthetic callbacks with three or more finalized trial trajectories: All retains all, Current remains its existing operation, LastN1 retains two, LastN2 retains four, and LastN0 retains all. Empty groups do not evict a visible group. Accepted arcs remain intact.
3. Matching trial camera/vector/replay cleanup; clear/trim/reopen lifecycle; independent regular segment-label retention. Exercise nested/multiple providers only to verify declared trajectory-count behavior, without claiming new solver-domain qualification.
4. One affected independently authored short targeting or optimization mission with an unchanged complete numerical report compared to its source-matched pre-change/All reference. Inspect actual pending controls and retained trial counts; no tutorial/corpus rerun or live-host testing.

## Separate coordinate-label lead

`AxesLabels` and `FrameLabel` are active, simpler plot operations: wx offers Show Axes Labels/Label Reference Frame (`OFConfigurationPanel.cpp:447–448,1063–1064,1325–1328,1348–1351`); `wx/OFScene.cpp:1076–1098` toggles X/Y/Z text and the coordinate-system name at the origin, including FrameLabel with Axes Off. Defaults are axes labels On/frame label Off (`CoordinateOptions.cpp:40–44`). Qt retains these only as warnings/comments (`ScriptCompatibility.cpp:427–451`); fallback always draws X/Y/Z with axes (`PlotWidget.cpp:285–290`), native global axes are unlabelled lines (`OrbitRenderer.cpp:393–397`), and there is no independent origin-name control. Add validated plot metadata, pending controls and matching native/fallback overlays without engine changes. AxesLength is stored by the legacy editor but never consumed by the inspected backend, which uses fixed2*EarthRadius; it should not be bundled as invented scale parity.
