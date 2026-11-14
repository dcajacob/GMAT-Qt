Actual Assignment editor: MATLAB RHS result categories
======================================================

Coverage remains 1,050/1,158, with 108 exact requirements open and 51 needed.
Historical operations remain 1,816/2,367. All 59 prerequisite identities and
broader qualification gates survive. This component assigns no row credit.

The stage is ``build/nasa-coverage/coverage95-20261104-matlab-assignment-operator``.
Three new catalog cases enter MATLAB RHS calls through actual Assignment
Destination/Expression controls: String Parameter, writable Quaternion and
nested Array indices. The earlier real-Parameter Assignment case is an affected
regression. Eight contexts pass across both GUI profiles, with 24 missions,
96 actual MATLAB calls and 96 GMAT scope calls. The vector headless pass is
reused from source1; corrected string/nested and original real passes use source2.

The original 33 catalog cases, numerical bounds and 76 assertion expressions
are preserved. One assertion checks the string case's distinct initial seed.
Actual initial GMAT Assignment type, native CallMatlabFunction lowering,
single-equals pending/applied source, Cancel/Apply/idempotence, Undo/Redo,
Unicode reopen, repeat missions, actual viewer, independent return/destination
oracles and caller/prototype/history/file invariants remain exercised.
All four visible contexts pass in initially minimized, resizable, nonfullscreen
windows. All 76 mapped native identities, eight recorded MATLAB runtime PIDs
and one visible server were observed terminal without forced interruption or
terminating licensing services. Licensed runtime remains MATLAB 2026b.

Two failures are retained in the original source1 batch. The new string fixture
incorrectly seeded a numeric Variable Assignment with a String input; it failed
before editing. Source2 uses the numeric literal41 only for that new seed and
asserts its exact initial expression. This is a fixture correction. The nested
Array test reached actual Apply but the interpreter mistook the quoted command
label before recursively indexed LHS for an invalid command. Source2 recognizes
the labelled Array prefix in this early command-name guard. Native wrappers
and actual function validation still check objects, indices, types and results.

``core-increment-v2.json/.patch`` retain full pre/postimages for the sole
production increment, ``src/base/interpreter/Interpreter.cpp``. Of 280 pending
core paths, 279 are unchanged. There is no header, layout or exported signature
change. Both frozen source capsules and matching successful builds are retained.
``fixture-conservation-v2.json`` records the exact catalog/assertion conservation.

``row-scope-review-v1.json`` retains the original FRC162--178 context and all
324 native Parameter metadata records. The factory-settable system Parameters
partition into 224 real, five string and three Rvector types. The three Rvector
types are Quaternion, ThrustAngle1 and ThrustAngle2, all covered by the earlier
destination component. Explicit user Variable/String/Array categories remain
separate from factory settable flags. No type or original clause is excluded.
General unmarshalling delegates to existing native virtual setter contracts;
Parameter physics/ownership and complete current-build approval are separate.

The exact row under review is FRC!E176, FRC-19.3.8, Assignment RHS Matlab
Function. It is distinct from already admitted All Others!E180. Actual user
``=`` behavior is observed even though the native lowered command is a distinct
CallMatlabFunction type. The original leaf does not explicitly require compound
MATLAB math expressions; the existing documented/native restriction is retained,
without retiring the original row or adding a stronger invented obligation.

The reusable registrations are
``QtGui.NasaMatlabAssignmentDestinationAssignmentEntry*`` in the shared catalog,
driver and dedicated runner. Fresh execution requires a new output path::

  python3 src/qtgui/tests/RunMatlabAssignmentDestinations.py --visible \
    --filter '^QtGui.NasaMatlabAssignmentDestinationAssignmentEntry(StringParameter|Quaternion|NestedArrayIndices|RealParameter)$' \
    --output build/nasa-coverage/fresh-matlab-assignment-operator

The complete component audit is::

  python3 src/qtgui/tests/AuditMatlabAssignmentOperator.py \
    --repo /home/dan/GIT/GMAT-Qt \
    --manifest build/nasa-coverage/coverage95-20261104-matlab-assignment-operator/evidence-manifest-v1.json \
    --selections build/nasa-coverage/coverage95-20261104-matlab-assignment-operator/selections-v1.json \
    --output /tmp/matlab-assignment-operator-fresh-audit.json

The audit validates both source capsules, builds, actual process maps, passing
JUnit, visible policy/exposure, exact source entry and original independent typed
oracles. The frozen typed reader checks common backend clauses; a separate
mandatory entry check verifies the actual Assignment seed and editor decisions,
including the new numeric string seed. Native proofs are not rewritten.
Forty damaged entry/backend models must be rejected. The complete frozen index,
destination and earlier predecessor/owned-resource chain is replayed. All 108
original remaining rows/fields and 59 hold identities are retained with a
zero-credit operator annotation. Immutable artifacts and temporal aliases remain
separate. Post-native readers/docs are review additions to execution source1/2.

After component sealing and frozen replay, complete the exact FRC!E176
conjunction/admission review and update the qualification ledger only if it
passes. Original Object/Literals I/O, separate MATLAB process/provider/API
obligations, source conflicts and other gaps remain explicit. No whole MATLAB
bundle, scientific, technical, complete-current-build or NASA approval follows.
