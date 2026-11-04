MATLAB Assignment array indices: source-specific acceptance
=========================================================

The original requirement ledger remains **1,050/1,158 (90.7%)**, with 108
exact rows open and 51 further rows needed for the unchanged 1,101 target.
Historical operation coverage remains 1,816/2,367. All 59 prerequisite identities,
original fingerprints, recorded gaps and broader qualification gates survive.
This component assigns no requirement credit. FRC!E176 remains a separate
whole-row review, distinct from already admitted All Others!E180.

Native evidence
---------------

The stage is ``build/nasa-coverage/coverage95-20261104-matlab-assignment-indices``.
``selections-v1.json`` selects 26 new contexts: thirteen cases in both headless
and visible profiles. Eight affected regression contexts additionally cover
literal array-element outputs, whole-array I/O, numeric properties and string
properties. Reused passes retain their own exact source/build version and the
failed batches from which some passes were selected.

The new cases cover variable-index outputs, inputs, input/output and aliases;
nested array indices; real Parameter indices; literal-element inputs; and six
zero, fractional or out-of-range input/output guards. Configured indices differ
from runtime assignments, and main/private indices differ. Positive cases check
main, two direct private and one nested private MATLAB call during initial,
Unicode-reopened and repeat missions. All original numerical bounds, unused
cells, caller-input copies, configured prototypes, source/history/file checks,
continuation and actual report viewer checks remain. All 63 original assertion
expressions are retained verbatim; thirteen assertions are added.

Seven positive and six rejection cases in each profile make 78 new mission
attempts, including 168 actual MATLAB calls and 168 GMAT scope calls in the
positive contexts. Rejection cases require the actual bridge diagnostic,
unchanged destinations and caller state, and empty MATLAB body journals.
The eight affected regression contexts add 24 mission attempts. These counts
are component witnesses, not additional requirement or historical-operation
credits.

All seventeen selected source5 visible contexts passed in initially minimized,
resizable, nonfullscreen windows. Process observations record 327 mapped native
identities, eighteen MATLAB runtime PIDs and one visible server terminal, without
forced interruption or terminating unrelated processes or licensing services.
The runtime is licensed MATLAB 2026b; the GMAT R2026a binary suffix is separate.

Failures and exact production changes
------------------------------------

Five execution source capsules and five successful matching builds are retained.
Six native failures remain in three original failed batches:

* Source1: variable output names reached MATLAB unresolved, while variable
  inputs failed the old integer-only index parser (three failures).
* Source3: nested output indices reached malformed MATLAB evaluation, and
  dotted Parameter indices were misclassified as whole system Parameter names
  (two failures).
* Source4: the new native-declaration check identifies the nested output comma
  changed to a space by output-list parsing (one failure). The Parameter case
  passed and is reused.

``core-increment-v5.json`` and ``core-increment-v5.patch`` retain full preimages
and postimages for two existing pending paths. Of 280 pending core paths, 278
are unchanged. No public header, object layout, exported signature or slot is
changed. No type eligibility or original requirement is waived.

``CallMatlabFunction.cpp`` resolves indices through the command's actual local
and global object lookup, recursively reads native arrays/real Parameters, and
checks finite positive integer configured bounds before using numeric MATLAB
indices. ``Interpreter.cpp`` classifies dotted MATLAB array arguments as arrays
and preserves nested commas while splitting MATLAB output lists at top-level
separators. Other function command parsers retain their existing route.

Replay and remaining work
-------------------------

Reusable registrations are ``QtGui.NasaMatlabAssignmentDestination*`` in
``src/qtgui/CMakeLists.txt``. The catalog and native driver are
``NasaMatlabAssignmentDestinationCases.json`` and
``NasaMatlabAssignmentDestinationTests.cpp``. The dedicated runner is
``RunMatlabAssignmentDestinations.py``. To create fresh visible execution
evidence after building the registered target, use a new output directory::

  python3 src/qtgui/tests/RunMatlabAssignmentDestinations.py --visible \
    --filter '^QtGui.NasaMatlabAssignmentDestination(NestedArrayIndices|ParameterArrayIndices)$' \
    --output build/nasa-coverage/fresh-matlab-indices

Do not overwrite retained run directories or use newly built binary bytes to
relabel old source-versioned passes. The stage's ``run-with-maps-v1.py`` also
binds actual loaded project images, build receipts and before/after source
guards; it requires the matching successful source-versioned build receipt.

The complete offline audit command is::

  python3 src/qtgui/tests/AuditMatlabAssignmentIndices.py \
    --repo /home/dan/GIT/GMAT-Qt \
    --manifest build/nasa-coverage/coverage95-20261104-matlab-assignment-indices/evidence-manifest-v1.json \
    --selections build/nasa-coverage/coverage95-20261104-matlab-assignment-indices/selections-v1.json \
    --output /tmp/matlab-indices-fresh-audit.json

Use a nonexistent output path. Frozen replay extracts that exact reader from
``reviewed-sources-v1.tar.gz`` and compares its output with
``evidence-audit-v1.json``. The audit validates all five source capsules, source
and binary guards, actual maps, passing JUnit identities, visible exposure,
independent original destination/I/O oracles, 104 damaged-model rejections,
exact core conservation and all 108 remaining row fields. It replays the sealed
destination component and its complete predecessor/owned-resource chain.
Temporal runtime symlinks are recorded separately from immutable artifacts.

``remaining-requirement-audit-v1.json`` appends a zero-credit index increment to
each exact previous row without changing its prior fields. The source review
retains the original FRC162--178 context and the native index grammar. Next,
assess actual single-equals Assignment editor lowering for array and string
destinations against FRC!E176. Direct MATLAB call index witnesses and prior
real-Parameter Assignment lowering do not silently replace this final review.
Object/Literals I/O, separate MATLAB process/provider/API obligations, original
source conflicts and all other recorded gaps remain open. No scientific,
technical, complete-current-build, NASA or whole MATLAB bundle approval follows
from these component passes.
