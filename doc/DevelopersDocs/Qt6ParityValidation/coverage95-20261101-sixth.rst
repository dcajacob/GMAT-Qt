Coordinate-frame state definition acceptance
============================================

The sixth 95-percent increment admits **FRR!E9** and raises the original
source-versioned requirement union from **996/1,158 (86.0%)** to
**997/1,158 (86.1%)**. The target is **1,101**, so **104 more complete
requirements** are needed. Historical coverage remains **1,816/2,367 (76.7%)**.
The original **161 requirement** and **551 historical** gaps remain explicit in
``build/nasa-coverage/coverage95-20261101/coverage.json``. The goal remains active.

What is proved
--------------

FRR!E9 enumerates Cartesian, SphericalAZFPA and SphericalRADEC definition in
any coordinate system declared by FRR-35. Its three original representation
children and frame-creation prerequisites remain mandatory at their sealed
baseline versions. The new conjunction covers all 16 declared axis categories,
all five origin classes and all three representations through 19 configured
frames: **57 new registered cases**, each with three states/epochs.

All 57 cases pass unattended and visibly. Three earlier unattended pilot passes
are reused at their recorded versions; 54 missing or expanded cases ran next.
All 57 visible cases ran after the repairs. Eight affected regressions pass in
both modes: the original Cartesian/spherical/Keplerian definitions, resource
round trips, Workflow and the Target/Optimize root solver trees. These are
focused regressions following shared changes, not an unrelated suite replay.

Each new case checks actual frame/representation popups and six typed fields,
units, invalid Apply/OK recovery, pending Cancel isolation, complete configured
resource serialization, exact Undo/Redo, Unicode save/reopen, the read-only
report viewer, native frame/axis/origin bindings and actual mission output.
Cartesian/spherical geometry is computed independently of GMAT. Original
position and velocity bounds remain **1e-6 km** and **1e-9 km/s**. Three octants
and epochs are checked. Circular object-reference rejection also checks that
both visible and backing selectors restore the previous frame.

The complete declared-category conjunction is combined with the reviewed
common invertible six-state conversion argument. This factors the unbounded
valid frame-parameter cross-product; it does not claim that every physical
parameter combination or astronomical frame model was independently tested.
The source-versioned qualification and 20 corrupt/incomplete-proof rejection
checks are in ``frame-state-qualification.json`` and
``frame-state-negative-audit.json`` under the same campaign directory.

Repairs and failures for scrutiny
---------------------------------

This increment changes one **core** file and one Qt product file:

* ``src/base/interpreter/Interpreter.cpp`` supplies a spacecraft's existing
  J2000 reference-body binding before FinalPass evaluates frames that depend on
  another spacecraft. Previously those frames could fail before Run with
  ``J2000 body Earth not yet set for Reference``. No physics equation is changed;
  interpretation and initialization behavior does change.
* ``src/qtgui/ResourceEditor.cpp`` synchronizes the visible selector from the
  actual backing choice after conversion/rejection. A stale signal argument
  could previously conceal a restored/rejected frame behind a different label.

The exact incremental patch is ``sixth-product-changes.patch``. The full attempt
review is ``sixth-change-and-failure-review.json``. Eight failed native attempts
and two failed new-test builds are retained. The preview-only initialization
and ineffective Qt factory override were removed. Existing test assertions and
fixtures remain unchanged; new assertions were added for the demonstrated
selector failure. Of the 104 previously pending core paths, 103 remain unchanged
from the campaign snapshot and the interpreter is the one changed path.
Prior four Qt path repairs remain unchanged. Technical core approval remains
open; this passing family is not complete current-build or full NASA approval.

Run it yourself
---------------

From the repository root, build the reusable target::

  cmake --build build/linux-gui --target GmatQtNasaFrameStateDefinitionTests -j2

Run the 57 cases unattended::

  ctest --test-dir build/linux-gui -R '^QtGui.NativeNasaFrameState' --output-on-failure

Or watch them in the existing resizable/scaled test desktop. It starts minimized;
restore its window to watch it. It does not enter fullscreen::

  python src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
    --filter '^QtGui.NativeNasaFrameState' --visible --display-route nested \
    --display-scale auto --start-minimized --pause-ms 0 \
    --max-screenshots 0 --max-checkpoints 200 --report-every 60

The runner prints the fresh temporary output directory. Narrow the filter to a
single case to inspect it without replaying the family. Native failures preserve
their assertions and diagnostics. For this campaign, source/runtime identities
were additionally frozen with ``run_frame_states.py`` and ``record_frame_state_inputs.py``;
no runtime binaries were copied. Passing screenshots are disabled. Unneeded
completed traces and duplicate reports are losslessly compressed, preserving
all admission inputs. ``sixth-retention-audit.json`` records hashes and savings.

Recheck the existing qualification without executing a GUI::

  python src/qtgui/tests/AuditFrameStateAcceptance.py \
    --qualification build/nasa-coverage/coverage95-20261101/frame-state-qualification.json

Remaining scope
---------------

This increment does not close FRR!E5's separate three-representation inertial
celestial-origin conjunction, FRR!E13's broader all-supported-representation
conversion scope, singular/conic/model science gates, or FRR!E52's ambiguous
fixed-offset attitude wording. E5's finite list must not be expanded to all 14
current native representations; E13 explicitly has the broader scope.
``sixth-next-scope-clarification.json`` and ``next-after-frame-state.json`` retain
that distinction and the next gate without assigning more credit.

The original 59 prerequisite holds are still explicit. If all persist, even
proving every other original requirement reaches at most **1,099/1,158 (94.9%)**;
95% requires resolving at least two held obligations. That is an optimistic
feasibility bound, not a promise or progress estimate. Historical OS/MATLAB,
removed-feature and source-conflict obligations are not retired for arithmetic.
The available authority is the pinned original workbook/RTTM corpus; current
NASA corpus qualification and independent core/science approval remain open.
