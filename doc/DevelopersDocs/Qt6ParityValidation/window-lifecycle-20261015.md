# Focused window lifecycle validation — 2026-10-15

The four remaining lifecycle operations now have current-binary, actual-input
**private software Wayland** observations. Physical Intel/Wayland acceptance
remains **unqualified for all four**. No active-desktop input, physical-GPU test,
compositor restart, reboot, installation or security-setting change was performed.
This increment neither attributes nor repairs the historical host popup crash.

## State and runtime

The checkout was clean at `215be630`, `codex/qt6-gui`, 34 commits ahead of origin.
A fresh read-only `git ls-remote origin refs/heads/codex/qt6-gui` returned
`ad343e74d9540001b5307e3153249b453a5ce85b`, matching the local remote-tracking ref.
No fetch, push or release was performed. No repository AGENTS.md was found in
this checkout or its ancestor chain; the home Codex AGENTS.md was empty. No
applicable engineering skill was present in the exposed skill catalog or the
searched local SKILL.md files. The delegated scope and the two acceptance notes
supplied the working constraints.

The production executable was **not rebuilt or changed**:

| File | Size | SHA256 |
|---|---:|---|
| `application/bin/GmatQt-R2026a` | 6,750,008 | `e78e6ad9b27bec4c5f1fa80c060c60e6e76e211d827bc78f9ffbcc878efed865` |
| `application/bin/libGmatBase.so.R2026a` | 25,040,656 | `5d455c6b0300e930dd9d9550f03e087b68fcded2508fa1304e5a031af77a100e` |
| `application/bin/libGmatUtil.so.R2026a` | 2,929,904 | `f9844517a2de0dfa1ddbd7b8180a63452aa6917a8fcd0b87217f304547a717a8` |
| selected Qt startup | 10,260 | `5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a` |

Ubuntu 26.04.1, kernel `7.0.0-1015-oem`, GNOME Shell `50.1-0ubuntu1.3`,
Mutter `50.1-0ubuntu2.4`, Mesa `26.0.8-1ubuntu0.3` and Qt Wayland
`6.10.2+dfsg-7` were recorded. The helper retained bwrap `--unshare-all`, fresh
`/dev`, private runtime/HOME/bus/network, read-only application and necessary
OSG library bindings, and software-rendering requests. Compositor logs explicitly
report a **surfaceless renderer without GPU**. The owned 1600×1200 monitor at unit
scale and persistent RemoteDesktop input delivered real Wayland events. These
sessions have no access to the host display, GPU, input or system bus. Unsafe-mode
is the pre-existing helper option for its newly created private Shell only; no
host Shell setting was changed. Startup cloning changes only OUTPUT_PATH.

## Actual-input results

The main lifecycle session is `isolated-wayland-xgt2zaxd`. It ran the generated
default mission once; its visible message records completion in 0.670 s. This
is an affected lifecycle fixture, not a new numerical qualification or tutorial.

| Operation | Observed result | Evidence and limits |
|---|---|---|
| Viewer close/reopen | **PASS, private Wayland** | Held title-bar Close removed OrbitView; Output leaf selection and Return recreated it with Earth, trajectory and stars. [Closed](window-lifecycle-20261015/orbit-closed.png), [reopened](window-lifecycle-20261015/orbit-reopened.png); actions 22–39. |
| Distinct resize | **PASS, private Wayland** | Restore from maximized returned the main frame to 1282×890. Held bottom-right border drag produced 1093×782, confirmed by compositor frame readback, actions 42/49–56. [Resized](window-lifecycle-20261015/resized.png). The pre-existing MDI Orbit position is partly clipped after shrinking; no automatic retiling or full scene visibility in that smaller workspace is claimed. |
| Minimize/restore | **PASS, private Wayland** | Super+H made the same owned frame mapped=false/focus=false; Alt+Tab restored mapped=true/focus=true at 1093×782. [Minimized](window-lifecycle-20261015/minimized.png), [restored](window-lifecycle-20261015/restored.png); actions 58–65. Ground map/trajectory remain visible; Orbit retains its partially clipped position. |
| Normal interactive exit | **PASS for application; original session teardown failed** | Actual Alt+F4 on the focused mission window exited GmatQt **0**. The old helper then treated this expected exit as fatal and recorded sandbox **1**, private compositor **-11**, PipeWire **0**, application **0**. This is not a clean whole-session pass. The compositor log says `Shutting down GNOME Shell` before termination; no backtrace establishes its cause. |
| Repaired helper's normal exit observation | **PASS, clean private session** | `isolated-wayland-2l2tyl3j` used the same production binary without another mission run. Its new guarded `exit_application` action sent Alt+F4 to the sole mapped/focused owned main window, observed application **0** while compositor/PipeWire were alive, then ended the protocol normally. Application/compositor/PipeWire exits **0/0/0**, sandbox **0**. [Before close](window-lifecycle-20261015/normal-close-ready.png); exact response is in the receipt. |

Both sessions record unchanged host GNOME **PID16588 / start ticks5140** before
and after. Read-only host core metadata lookup found no new GNOME cores in the
bounded interval. That absence does not diagnose the private compositor's -11.
No historical popup reproduction was attempted on the user's desktop.

## Focused changes and verification

`WindowTests.cpp` previously ended successful checks with `app.exit(0)`, which
bypassed the normal main-window close path. It now requires accepted Close,
`lastWindowClosed` exactly once, hidden main window and normal event-loop return,
with a 55-second failure deadline. Each of three cycles also requires distinct
small/large main and viewer sizes, closed-viewer removal, deferred destruction of
the old subwindow and survival of the reopened viewer and its retained model.
Existing textured Earth/map, trajectory, native surface and source checks remain.

`ninja -C build/linux-gui -j1 GmatQtWindowTests` compiled one source and linked
one test executable; no engine/application target was rebuilt. The resulting
test SHA is `5e6fa9f5329d8b358e65c919f3c370857652b8e42ed5f1a6cadcdad69575dcd4`.
It passed **three cycles / normal event-loop close, exit0, 22.676s** on owned
Xvfb/xcb/software GL. Xvfb had no window manager: its top-level minimize check
establishes Qt client state, while actual compositor minimize/restore is supplied
by the separate Wayland observations above. This is not physical-GPU acceptance.
The test-only startup clone makes ROOT_PATH and relative PLUGIN assignments
absolute and redirects OUTPUT_PATH to the owned regression directory.

`IsolatedWaylandSession.py` now has an explicit terminal `exit_application`
action. It refuses foreign, unfocused or modal-window targets, waits at most five
seconds, rejects timeout/signal/nonzero application exits and checks that private
services remain alive before cleanup. Unexpected application exits still fail.
The worker protocol ends normally only after that explicit successful action.
Six new standard-library unit checks pass in **0.003s**, including target guards,
timeout/signals, compositor failure, terminal response and unexpected-exit checks.
CTest registration is `QtGui.IsolatedWaylandExit`; the checks were run directly
with `/usr/bin/python3 -B src/qtgui/tests/IsolatedWaylandExitTests.py -v`. The
existing CMake build tree was not reconfigured merely to run these Python checks.
`git diff --check` passes. No production frontend fix was indicated by this work.

## Preserved unsuccessful attempts

The raw archive retains the outer-sandbox namespace refusal before launch,
outer-sandbox private-X socket launch failure/abort message, the first ineffective
Escape/click against Welcome, the ineffective Alt+F8 resize attempt, all input
and readback traces, and the original expected-exit misclassification/private
compositor teardown failure. Held-button Welcome dismissal and border-drag resize
are counted only at their later confirmed readbacks. Preflight cleanup terminated
its app with SIGTERM; that run is not normal-exit proof. All owned sessions closed.

[Machine-readable receipt](window-lifecycle-20261015.receipt.json) binds source,
executable, library, helper and screenshot identities.
[Raw archive](window-lifecycle-20261015.raw.zip) contains all attempts, full logs,
all compositor PNGs, launch identities, protocol traces and a per-file SHA index.
The original temporary root was `/tmp/gmat-qt-lifecycle-20261015/`; the archive is
the durable evidence, independent of later `/tmp` cleanup.

## Remaining native-host step — requires user participation before execution

All four physical Intel/Wayland lifecycle rows remain **unqualified**. The October5
host rendering/popup/chooser results remain bound to their earlier runtime.
The historical NULL Wayland popup resource attribution and compositor repair are
unproven; this private teardown -11 has not been equated with that older mechanism.
No safe active-desktop experiment here resolves that risk.

Proposed bounded procedure, **not executed**:

1. Dan saves work and arranges a native GNOME Wayland login containing no unrelated
   unsaved applications, accepting that a compositor crash could terminate that
   login and lose its unsaved state. Use a disposable test login/session or a
   maintenance window Dan prepares; do not restart his active compositor, reboot,
   change permissions/security settings or create an account automatically.
2. Record binary/base/util/startup hashes, GPU device/driver, session/backend,
   GNOME PID/start ticks and scoped journal/core baseline. Launch one owned GmatQt
   with a private settings directory and OUTPUT_PATH clone, at the existing
   native scale, using the current bounded default fixture. Record its PID and
   wait status separately from any capture/helper process.
3. Dan directly performs one Orbit title-bar Close, selects its Output leaf and
   presses Return; capture absent/present viewer and retained Earth/trajectory.
   Drag the main border to a visibly different size and restore it; record settled
   frame dimensions and scene recovery. Use Window → Tile if needed to bring the
   existing MDI positions into the reduced workspace, recording that action.
4. Dan minimizes with Super+H, restores the exact owned app using GNOME's normal
   overview/app switcher, verifies both scenes, then closes the focused app with
   Alt+F4. Record **application exit0**, surviving host GNOME identity and scoped
   journal/core deltas. Stop on any unexpected failure. Do not repeat the known
   popup trigger or automate a compositor recovery.

The user must prepare that interruptible session and be available for the direct
focus/restore/close actions before this native-host step. This is a remaining
acceptance procedure, not a request to repeat the completed 108 wx audits, twelve
Help tutorials or numerical matrices. All nine original acceptance gates and the
existing scientific/dependency/platform holds remain open at their stated scopes.
