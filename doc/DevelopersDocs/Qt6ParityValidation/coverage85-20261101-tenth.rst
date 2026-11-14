85 percent campaign: console plotting exceptions and Ubuntu execution
=======================================================================

Source-versioned requirements coverage is **983/1,158 (84.9%)**. **Two more
requirements** are needed to reach the unchanged985-row target. Exact additions
are ``All Others!E21/E22/E24`` (the named XY, Orbit View and Ground Track console
exceptions) and ``All Others!E286`` (console execution on Ubuntu). Historical
coverage remains **1,816/2,367 (76.7%)**. All **175 remaining requirements and551
historical gaps** retain their original scope, text, fingerprints and closure
gates. **Whole-product parity E20 and the Linux/Fedora parent E285 remain
unqualified.** GUI-interface exception E23 also remains open.

Actual console contracts
---------------------------

The actual ``application/bin/GmatConsole-R2026a`` executes eight cases:
a no-plot baseline, XY alone, Orbit View alone, both GroundTrack aliases,
all plots together, and invalid configurations for each graphical class.
Each positive mission propagates two spacecraft through independently specified
circular orbits, reports three complete rows, and preserves a shared variable.
All **225 numerical fields** match the independent literal-mu calculations.
Every positive case's report is also byte-identical to the no-plot baseline.

The console writes the actual configured subscriber objects in script form.
It therefore accepts and validates plot configurations while omitting graphical
presentation. Missing XY parameters, an unknown orbit frame and an unknown
ground-track body are each rejected with a nonzero status and no mission report.
These expected negative results are passing tests, not unresolved failures.

Each process runs without DISPLAY, WAYLAND_DISPLAY, XAUTHORITY or WAYLAND_SOCKET.
Observed process maps bind its real executable and loaded project libraries;
no Qt or X11 library loads. Source review confirms that PlotInterface starts
with a null receiver and the console driver installs no plot receiver. Existing
sealed, numerically checked visible GUI evidence supplies positive delivery of
all three graphical classes, including live trial/accepted publication. That
evidence retains its actual earlier source/runtime versions and was not replayed
or attributed to the newest complete binary.

The literal named plotting exceptions are closed by this paired mechanism/result
evidence. This does not qualify every GUI mechanism, all non-graphical console
capabilities, whole-product parity, native input/GPU behavior or GUI-interface
exception E23.

Measured Ubuntu scope
------------------------

The Ubuntu child inherits a console-on-Linux requirement. The native console
artifact executes these eight checked cases on the measured **Ubuntu26.04.1 LTS**
host. Original /etc/os-release contents/hash, executable/loaded-image hashes,
actual commands, private startup files, scripts, logs and reports are bound
before/after execution. This is source-versioned availability on that measured
Ubuntu release. It is not every Ubuntu version, a redistributable package or
installation certification, Fedora, Windows, macOS, or full Linux qualification.
No platform parent or unrelated platform child receives credit.

Reuse and automation
------------------------

The new workflow first passed directly. A new CTest entry point then ran the same
eight new cases once to validate automation integration. Both executions passed;
there are no failed new console cases. This integration replay is explicitly
recorded. No established GUI or unrelated engine/comparator case was replayed.
The CTest entry point is registered only when the GmatConsole target exists, and
check-qt now depends on that target. All assertions from the first direct run
remain; only an optional unique output-root CLI destination was added.

Ten coherently rehashed corrupt proof copies reject missing cases, wrong orbit
results, accepted invalid configurations, GUI libraries, missing process maps,
missing actual subscribers, a live display, a wrong measured distribution,
inflated current-build claims and extra parity-parent credit. These audits run
no console, GUI or engine mission.

There are **no new GMAT product/core/scientific changes** in this increment.
All101 previously tracked core files remain byte-identical to the preceding
increment and pending independent technical review. Test registration/runner,
proof auditing and documentation changed. No screenshots or runtime binary
copies are retained. Full current-build, scientific, platform and NASA
qualification remain open. The85percent goal remains active.

Run and audit
----------------

Run just this new reusable automation contract::

   ctest --test-dir build/linux-gui \
     -R '^QtAcceptance.ConsolePlotExceptions$' --output-on-failure

Or run selected cases directly, without repeating others::

   python src/qtgui/tests/RunConsolePlotAcceptance.py \
     --case xy --case invalid-xy --output /tmp/gmat-console-xy-new-run

The output must not exist. Omit --case for all eight cases. --output-root creates
a unique child directory suitable for repeated automation. Native console tests
create no GUI windows. Existing GUI evidence keeps the previously requested
resizable, non-fullscreen, initially minimized window policy.

Audit preserved evidence without replaying missions::

   python src/qtgui/tests/AuditConsolePlotAcceptance.py \
     --qualification build/nasa-coverage/coverage85-20261101/console-plot-qualification.json
   python build/nasa-coverage/coverage85-20261101/negative_console_qualification.py
   python build/nasa-coverage/coverage85-20261101/audit_tenth.py

The canonical coverage.json retains every remaining original row. The next
candidate is the still-open scalar-parameter family for NonlinearConstraint:
its child FRC!E105 and parent FRC!E104 need a declared full family inventory,
representative partitions and independent operand/result checks. Existing
selected-X/variable/array passes will be reused; they do not alone close this
family. No forecast credit is included in the983 numerator.
