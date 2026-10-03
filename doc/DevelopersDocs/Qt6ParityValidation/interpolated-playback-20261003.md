# Two animation modes and playback speed buttons — 2026-10-03

The user observed nonlinear animation speed in Mars B Plane and requested
Propagation steps / Interpolated steps plus buttons to decrease/increase speed.
Retained-sample replay already removed pauses caused by discarded solver frame
IDs. Adaptive numerical steps still made equal playback ticks cover unequal
simulation intervals. The two modes now make that choice visible:

- **Interpolated steps** is the default for Orbit and Ground Track. Equal
  playback ticks advance equal amounts of connected forward simulation time.
  Positions use linear interpolation, orientations use quaternion SLERP, and
  cameras, vectors, lights, labels, markers and trajectory tips use the same
  presentation cursor. Ground longitude follows the short dateline route.
- **Propagation steps** visits the retained publisher samples in original order,
  preserving the previous recorded-step behavior and skipping discarded IDs.
- **Slower / Faster** buttons are available in the shared Animation toolbar and
  individual Orbit/Ground Track controls. They step through 0.25×, 0.5×, 1×,
  2× and 4×; the corresponding button disables at each limit. Mode and speed
  use a separate local row so narrow tiled views remain usable.

The nominal sweep remains three seconds at 1×. Shared playback is a percentage
of each plot's retained history, not a common epoch. XY lacks simulation-epoch
history and continues to use recorded samples. Disconnected/reset edges jump
without consuming interpolation time; curves are never blended across a break,
solver-status change or unavailable object. Histories with no connected forward
time, including wholly backward histories, fall back to recorded steps. These
limits preserve actual mission ordering instead of fabricating connecting arcs.
Interpolation approximates the display between recorded results and does not
change engine states, numerical accuracy, data admission or raw exports.

## Relevant verification and corrections

The new synthetic check covers irregular times 0, 1, 10 and 100 seconds with
sparse frame IDs. At 50%, Interpolated displays time/position 50; Propagation
selects the original time/position 1. It checks matching camera/vector/path/
marker poses, steady 10% advances, untouched raw samples and XY export bytes,
rotation, dateline, disconnected jumps, default mode and local speed limits.
SharedPlayback covers the global mode and both speed buttons alongside its
existing retained-frame gap/tail/live invalidation/reopen cases.

The initial six-check run passed SharedPlayback, OwnedPlotBatch and SolverPlots.
InterpolatedPlayback initially tried the unsupported Orbit raw export; its
fixture now uses supported XY export plus an exact Orbit raw snapshot.
Plots/PlotsHiDPI exposed controls wider than a 330-pixel tile. Icon-only speed
buttons and the separate options row repair that production layout failure.
Only the three failed checks were retried, and all passed. Extending the new
check to native rendering initially used QObject::findChild on OrbitRenderer,
which has no Q_OBJECT; the test now finds QOpenGLWidget and uses dynamic_cast.
This compile correction and the original logs are preserved.

| Check | Passing result |
| --- | --- |
| QtGui.SharedPlayback | 0.19 s, offscreen |
| QtGui.OwnedPlotBatch | 0.08 s, offscreen |
| QtGui.SolverPlots | 1.27 s, offscreen |
| QtGui.Plots | 2.59 s, offscreen, corrected narrow layout |
| QtGui.PlotsHiDPI | 2.72 s, offscreen, corrected narrow layout |
| QtGui.InterpolatedPlayback | 0.06 s, final extended check |
| QtGui.NativeInterpolatedPlayback | 0.55 s, owned Xvfb / software OpenGL |

The native check requires a valid OpenGL renderer, visible interpolated object
pixels, and distinct Propagation/Interpolated trajectory-camera framebuffers.
Its CTest entry creates a private Xvfb display with LIBGL_ALWAYS_SOFTWARE=1.
This tests the changed native renderer without touching the user's live desktop
or claiming physical GPU acceptance. Raw captures remain in the evidence folder.
[Build logs](interpolated-playback-build-20261003.txt) and
[check logs](interpolated-playback-checks-20261003.txt) retain failures/corrections.
Committed log copies trim trailing whitespace; exact raw logs remain in the
evidence folder.

## One affected Mars B Plane run

One fresh private offscreen run of unchanged
`application/samples/Tut_Mars_B_Plane_Targeting.script` completed in 1.968 s.
The existing OF conversion route was used. Source SHA256 is
**f334cf650aaa435f132de1d8e7823333378a0d40cd179e0bb0e3143af4ff3178**, matching
the frozen original example receipt. Each of three views retains 1178 unique
frame IDs 5977–7872. Both modes, shared/local controls, 100 master advances,
99 fractional poses and 99 camera-target interpolations per view pass.
Camera-target interpolation error is zero, and all raw retained numerical,
camera and vector histories remain byte-identical after replay actions.

At equal 1% advances, Propagation epoch changes range from −9915.809407 to
8299221.465784 seconds. Interpolated has 99 connected intervals advancing
**266462.842845 seconds**, with less than 3.15e-7 s observed spread. The final
99%→Latest interval crosses the original disconnected epoch reset; it differs
by **−10813.862363 s**, matching the retained reset sum within 4.66e-10 s.
Uniform connected progress is verified; globally monotonic mission epoch is
not claimed across recorded solver/reset discontinuities.

The entire `DifferentialCorrectorDC1.data` remains byte-identical to the frozen,
source-matched pre-change report, SHA256
**165c8d5cea3af50829ad9bb855c6220243e847ce2402375eb0f1024fac083350**.
See [compact Mars receipt](interpolated-playback-mars-20261003.json) and
[probe source](interpolated-playback-mars-probe-20261003.cpp). Full recorded
samples, build commands, private startup, numerical report, stdout/stderr,
cadence-validation script and manifest are preserved under
`build/linux-gui/interpolated-playback-20261003/raw/`. Cadence validation reads
that recorded receipt and did not repeat the numerical run. No tutorial,
corpus or unaffected passing suite was repeated.

## Rebuilt application and qualification limits

`application/bin/GmatQt` resolves to GmatQt-R2026a, **6,187,872 bytes**, SHA256
**e126f7a1348a29bd527665064800d1aa517cfa0f599b383f4fdb11d6339ca798**.
Base **2082a341f5f123e81bad362972cd2b28a0a4753047ae51b5c09609e246be430c**,
util **f9844517a2de0dfa1ddbd7b8180a63452aa6917a8fcd0b87217f304547a717a8**,
and Qt startup **5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a**
remain unchanged. All application changes are in the Qt GUI; the original
`/home/dan/GIT/GMAT/GMAT` checkout remains untouched.

Live host testing remains stopped. The saved crash establishes Mutter's null
Wayland resource fault; GMAT client attribution, host repair and physical-GPU
acceptance remain unproven. All nine acceptance gates, held/external/domain
requirements and Windows/macOS/MATLAB deferrals remain in scope. Full Linux
GUI qualification is not claimed.
