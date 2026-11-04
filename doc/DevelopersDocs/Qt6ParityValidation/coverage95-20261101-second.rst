95 percent campaign: MATLAB file-location contract
======================================================

This increment reaches **990/1,158 (85.5%)** source-versioned requirements.
The 95 percent target remains **1,101**, with **111 more complete requirements**
needed. Historical coverage stays **1,816/2,367 (76.7%)**. All 168 remaining
requirements and 551 historical scopes remain explicit; the goal remains active.

Actual GUI repair and native checks
---------------------------------------

Set Paths lacked MATLAB function search-path controls even though native
FileManager lookup already existed. The new MATLAB Function tab supports the
ordered directory list, Browse, Add, Replace, Remove, Up and Down. Pending edits,
Cancel, invalid recovery, Apply, Unicode startup export/import and fresh-process
lookup are exercised by two new CTest registrations:

* ``QtGui.NasaMatlabFunctionPaths`` checks 85 original operation assertions
  (86 in the extended driver), real prescribed file priority/fallback/removal,
  fresh-process startup paths and an unrelated mission sentinel/source/history.
* ``QtGui.NasaMatlabPathBrowseImport`` checks 65 chooser/import assertions,
  actual Qt directory/file dialogs, Browse Cancel, pending startup import,
  native first-match lookup and source/history isolation.

Both pass unattended and with actual native window exposure in a resizable,
non-fullscreen, initially minimized test desktop. The affected ``QtGui.Paths``
and ``QtGui.NasaFunctionPathSearch`` regressions also pass in both routes.
``QtGui.PythonPaths`` passes unattended and as the registry's explicitly headless
support check during the visible campaign. Its receipt remains headless; no
native window exposure or visible Python qualification is inferred.

The `.m` files contain prescribed bytes, but their functions are **not executed**.
Truth is the native selected filename, search order and file contents, not an
emulated MATLAB result. The new contract grants no MATLAB/interface/fmincon
execution support; all 37 such prerequisite holds remain unqualified.

Complete parent scope, without dropped children
---------------------------------------------------

``All Others!E31`` now has actual GUI/native filename-lookup evidence. Its parent
``All Others!E26`` is the finite FRAC-2.6 list of **eleven** locations: report,
tracking data, log, GMAT function, MATLAB function, DE405, leap-second,
Earth-orientation, nutation, planetary-potential and SPK files.

The admission guard rejected the initial seven-child assumption before granting
any credit. The exact declared workbook hierarchy is now checked, and all ten
other child scopes retain their existing source-versioned passing clauses and
native consumers. The original first-increment speculative plan and its mistaken
seven-child description remain frozen in that earlier source archive; this
increment corrects the plan without altering any sealed passing receipt or
removing any source obligation. The new two-mode child does not substitute for
any of the other ten required location types.

Failures, regressions and product impact
--------------------------------------------

The initial missing-controls failure is preserved. The first visible campaign
also exposed unrelated-path failures because new validation rejected untouched
MATLAB defaults whose directories were absent. That was repaired: unchanged
inherited lists preserve existing behavior, while every changed/imported list is
validated completely. The original regression assertion sources are unchanged.
A visible chooser timing/selection failure was repaired in the new driver by
setting its real filename entry explicitly and reporting unexpected warnings
promptly. The five failed native attempts, initial test compilation error and
metadata admission rejections remain in ``second-change-and-failure-review.json``.

Four Qt files changed: PathSettings.hpp/cpp and PathSettingsDialog.hpp/cpp.
Before/after hashes and narrow patches are retained. **All 104 pending core files
are unchanged.** No force, integrator, coordinate, epoch, C API or optimizer
implementation changed in this increment. The shared PathSettings structure
changed, so all selected clients were rebuilt/relinked against the actual new
frontend. Passing records from previous milestones remain at their recorded
source/runtime versions. Full-current-build, independent core/scientific and
NASA qualification remain unproved.

Watch or automate
---------------------

::

   cmake --build build/linux-gui --target GmatQtNasaMatlabPathTests -j2
   ctest --test-dir build/linux-gui -R '^QtGui.NasaMatlab' --output-on-failure
   python src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
     --filter '^QtGui.NasaMatlab' --visible --display-route nested \
     --display-scale auto --start-minimized --pause-ms 0 --max-screenshots 0 \
     --nasa-review build/nasa-coverage/coverage95-20261101/matlab-path-runtime-review.json \
     --output build/gui-acceptance/matlab-path-watch

Restore the minimized desktop to watch. The runtime-only review awards no NASA
credit on its own; cumulative coverage comes from the complete audited union.
No passing screenshots or runtime binary copies are retained.

The corrected review of the original 173 gaps still identifies 59 prerequisite
holds; if all persist, even closing every other row would reach only 94.9 percent.
At least two held obligations must become provable to reach 95 percent. The
MATLAB installation/host information request remains open. Next work targets
the missing complete Minimize scalar family, reusing the 97-type inventory and
independent numerical oracles while adding actual objective execution evidence.
