85 percent requirements campaign: first four closures
=====================================================

Requirements coverage is **928/1,158 (80.1%)**, up four from the sealed
924-requirement handoff. Historical coverage remains **1,816/2,367 (76.7%)**.
Reaching at least 85 percent requires 985 verified requirement keys; **57 more
are needed**. This goal remains active. Neither denominator nor source obligation
is reduced, and this percentage is not full NASA/current-build qualification.

Four complete new capability witnesses
---------------------------------------

``QtGui.NativeNasaPlotAxisSelection`` closes FRR!E342/E343/E344. The actual
resource-tree XY setup and parameter dialogs commit every pair in a 3-by-3
cross-product: a spacecraft scalar parameter, a user variable and a 1-by-1 array
element on both independent and dependent axes. The scalar is selected through
the property/object/coordinate-frame browser; variables and array elements use
the actual entry/index controls. A subsequent cancelled X picker must preserve
its accepted draft. Pending isolation, Apply, exact source Undo/Redo, Unicode
save/reopen and native reference identity are asserted.

Each of the nine combinations produces nine actual model samples. Unequal
independent literals (7000, 3 and 2) specify every X/Y pair, and all three raw
report rows independently check those values, the loop index and sentinel41.
A separate propagated spacecraft supplies real publication events while the
plotted values stay fixed. It is not used as an unverified scientific oracle.
The complete native case passes visibly in 16.821 seconds with 119 exposure
events and zero screenshots.

``QtGui.NativeNasaTrajectoryColors`` closes FRR!E320's specifically missing
celestial-body color result. It opens actual spacecraft and celestial-body panels
from the tree, uses the current Visual model and body color pickers, and checks
Cancel/pending isolation, Apply, exact Undo/Redo and Unicode reopening. Distinct
literal RGB tuples must reach all five configured trajectories and every sample:
Sat, Earth, Luna, Mars and Sun. This covers spacecraft, planet, moon and star
routes. A raw report independently matches a circular two-body orbit and
sentinel41 after editing, and source/history/file preservation is checked.
The native case passes visibly in 12.002 seconds with zero screenshots.

Both tests run in the requested initially minimized, resizable, non-fullscreen
nested desktop. Restore it to watch. Qt event delivery and software rendering
are explicit; native physical input/GPU/other-platform qualification is separate.
No existing passing GUI registration was replayed.

Failures, preservation and storage
-----------------------------------

The first new axis fixture correctly failed its unchanged minimum-sample
assertion: Report-only commands did not provide enough subscriber publication
events. Adding actual propagation of the separate Runner fixed the fixture;
no sample-count, coordinate or raw-report assertion was relaxed. The initial
sandboxed display launch also failed before execution; authorized desktop access
resolved it. Both diagnostics remain retained.

The first color fixture correctly rejected an inaccessible hidden legacy
spacecraft color control. It now navigates the actual current Visual model form.
The visibility assertion remains. Neither fixture failure required a production
or engine repair. All old assertion sources and core sources retain their initial
hashes; CMake adds only the two new executable/test registrations and dependencies.
The existing 100 changed core files remain separately unapproved.

An unsealed failed-run archive accidentally included the observer ELF executable
because the runner lists it among source inputs. Its binary member was removed,
its exact identity retained, and every text-source member preserved. The
correction is explicitly recorded. Qualified source archives reject ELF members;
runtime/probe binaries are identified without copying them. Failed summary JSON
was losslessly compressed, saving 15,259,496 bytes; original receipts, logs,
JUnit, traces, raw values and all qualified passing reports remain intact.
No passing screenshot was created.

Admission and accounting
-------------------------

``BuildNasaCoverageExtension.py`` verifies the prior sealed final handoff and adds
only complete new source-row proofs. It reconstructs new reviews from compressed
text-source archives, checks the real executable identity, pre-execution receipt
source hashes, independent before/after input maps, exact log/JUnit identity,
actual native exposure/window policy and the full NASA assertion conjunction.
Authority and observer-binary identities remain separate from copied text.
Temporary source reconstruction is removed after verification.

Six new support guards and six actual corrupt qualification attempts pass their
rejection checks: duplicate/already-counted/unknown keys, failed/partial/unexposed
or wrongly displayed results, altered log/JUnit, omitted/extra/duplicate declared
row credit, source/archive drift and an unsupported full-NASA claim. Synthetic
support counts add no NASA credit. Old passing source versions are not rehashed
to today's complete binary.

The explicit extension adds exactly FRR!E320/E342/E343/E344. It preserves every
other original scope, all inherited qualification gates and all prior technical
limits. **230 requirement and 551 historical gaps remain**. A further 85-percent
claim requires 57 additional complete requirement contracts, not more regression
passes or partial mappings. The next named plot obligations are OrbitView and
GroundTrack intermediate redraw cadence; solver-iteration publication and the
separately classified C-client/scientific/authority projects require their own
substantive checks. The CInterface plugin is currently disabled in this build;
its existing source/exports alone supply no interface credit.

Reproduction
--------------

Reuse these passes without GUI replay::

   python3 src/qtgui/tests/BuildNasaCoverageExtension.py \
     --baseline build/nasa-coverage/coverage85-20261101/sealed-baseline.json \
     --qualification build/nasa-coverage/coverage85-20261101/axis-qualification.json \
     --qualification build/nasa-coverage/coverage85-20261101/color-qualification.json \
     --output /tmp/gmat-coverage85-check.json

To deliberately watch either new case, select its exact name/review and a fresh
output directory::

   python3 src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
     --filter '^QtGui[.]NativeNasaPlotAxisSelection$' \
     --nasa-review src/qtgui/tests/NasaPlotAxisSelectionTraceability.json \
     --visible --start-minimized --max-screenshots 0 --output /tmp/axes-watch

   python3 src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
     --filter '^QtGui[.]NativeNasaTrajectoryColors$' \
     --nasa-review src/qtgui/tests/NasaTrajectoryColorTraceability.json \
     --visible --start-minimized --max-screenshots 0 --output /tmp/colors-watch

The same registrations run unattended through CTest. Source/build changes create
new receipts; the retained qualification remains bound to its own versions.

Evidence is in ``build/nasa-coverage/coverage85-20261101/``: ``coverage.json``
contains the exact four additions and every remaining original row;
``axis-qualification.json`` and ``color-qualification.json`` bind the native
passes; ``first-increment-audit.json`` records preservation and archive checks.
The source baseline and previous campaign artifacts remain unchanged. Full NASA,
whole-current-build and independent core/scientific approval remain false.
