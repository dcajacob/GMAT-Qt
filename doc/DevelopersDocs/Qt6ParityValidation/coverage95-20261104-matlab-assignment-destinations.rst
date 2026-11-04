MATLAB assignment destinations: component qualification
======================================================

The original requirement count remains **1,050/1,158 (90.7%)**. The target is
1,101; 51 more exact requirements are needed. Historical operations remain
1,816/2,367. All59 prerequisite identities and the108 exact remaining rows,
fingerprints,source text,gaps and broader qualification gates are retained.
This component stage assigns no whole-row credit. FRC!E176 (FRC-19.3.8,
``1208c603c9962c90deb486ca333c0c3323b8535b632aef38632400dc2807090e``)
requires a separate exact-row conjunction/admission review. It is distinct from
already-counted Mission Tree item All Others!E180.

Changes and native evidence
---------------------------

Five defects are repaired in four previously pending production files:

* ``CallMatlabFunction.cpp`` uses received matrix dimensions for element-output
  column-major indexing and bounds checks. A private2x3 receiver previously got
  zero at(1,2) despite a nonzero actual MATLAB result in its1x2 returned matrix.
* ``MatlabInterface.cpp`` copies at most the received allocation and caller
  capacity,while returning the actual count for validation. Dense real2D output
  and the existing logical-scalar conversion are retained;complex,sparse and
  higher-dimensional double results are rejected before copying into dense
  GMAT values. Output dimensions are initialized and unsupported arrays released.
* ``CallMatlabFunction.cpp`` transfers actual writable Rvector results through
  the existing Parameter setter. Quaternion and both thrust-angle types are
  covered,including lengths1,3,4 and MATLAB row/column orientations. Existing
  type/dimension guards remain;no implicit dimension-resizing bypass is added.
* ``Interpreter.cpp`` keeps a named Assignment's label outside the output
  brackets when lowering a standalone function RHS into a native function call.
* ``GmatCommand.cpp`` serializes builtin function labels before the outputs,
  using the actual optional GMAT prefix. It previously inserted a label into
  the second argument. No source-mapping rule is relaxed.

There are no header,layout,slot or public-signature changes. The280-path pending
production inventory has four exact increments and276 unchanged paths.
Pre/postimages and ``core-increment-v10.patch`` are retained in the stage.

All20 new destination/rejection cases pass both profiles:40 contexts. They cover
real/string properties,array elements,all three writable vector types,an actual
Assignment Destination/Expression edit,typed malformed results,and the exact
qualified Cartesian rejection. Positive flows preserve typed Cancel/Apply,
idempotence,Undo/Redo,caller inputs,prototypes,source/history/files,Unicode reload,
repeat without an explicit Build action,and the actual checked report viewer.
``runMission`` itself performs its normal fresh Build. Explicit test NAIF IDs
preserve complete prototype equality;no fields are filtered.

Quaternion MATLAB tuples remain independently checked as returned. Native
attitude readback reconstructs from a cosine matrix and makes its largest
component positive. The algebraic destination oracle predicts that exact sign;
no either-sign acceptance,rounding or tolerance relaxation is used. The original
Cartesian positive premise was incorrect: its factory metadata is read-only.
Its retained rejection does not qualify writable vectors or every read-only
spelling. All324 retained Parameter metadata records survive the scope audit.

Two named GMAT/builtin variants reuse the unchanged Script Event driver and
original fixtures/oracles/bounds. Their four GUI contexts pass. Ten affected
original GUI regressions pass across both profiles:scalar,array,in/out,and the
original GMAT/builtin function calls. Eight standalone prefix/mode/output cases
pass16 exact serializations. The old whole-object I/O probe repeats its original
Build failure;it remains open and is not relabeled as an admitted regression.

The40 destination contexts give66 positive and48 rejected missions,264 positive
and48 rejected actual MATLAB calls,and264 positive GMAT scope calls. Actual
runtime metadata identifies licensed MATLAB2026b. The GMAT library suffixR2026a
is a product suffix,not the observed MATLAB release. All581 mapped native
identities,53 actual MATLAB runtime PIDs and three visible servers were observed
terminal. No native/kernel copies,forced native interruptions or termination of
unrelated/licensing services were used.

Custody and failures
--------------------

Ten frozen source capsules retain all failures and repairs. Source1 compiled
but its build-capture wrapper looked for the plugin in the wrong directory;
it authorizes no execution. Source2/3 retain fixture pointer/getter/prototype
failures and the actual element-stride failure. Source4 retains restricted-X
launch failures and native recovery. Source5 retains all three writable-vector
failures. Source6 retains the quaternion representation mismatch. Source7/8
retain the corrected native Assignment-type assertion and the actual label
lowering failure. Source9 retains the builtin serializer failure. Source10
passes all27 selected visible cases and the focused builtin recovery/support.
Old passing versions are reused for unaffected contexts.

The two initial offline audit failures and exact reader sources are retained:
a flat Python extraction violated the original package layout;then an archive
lookup assumed coverage of two older regression templates absent from its
inventory. Their exact actual runtime copies and proof hashes match in both
profiles. ``runtime-template-custody-v1.json`` records the omission and recovery.
Do not claim those omitted files were guarded by the original execution source
inventory or relabel the old archives. New named fixtures were already frozen.
``capture-sources-v2.py`` includes the originals for future captures.

Replay
------

The stage is
``build/nasa-coverage/coverage95-20261104-matlab-assignment-destinations``.
``selections-v1.json`` joins each exact case/profile to its source-specific run.
``component-audit-v3.json`` and ``named-calls-audit-v1.json`` record passing
component reviews. The reusable reader is
``src/qtgui/tests/AuditMatlabAssignmentDestinations.py``;its named/support reader
is retained as ``audit-named-calls-v2.py`` in the stage. Post-native audit/custody/
document additions are review sources,not relabeled runtime inputs.

Replay the sealed audit with a fresh output path::

  python3 src/qtgui/tests/AuditMatlabAssignmentDestinations.py \
    --repo /home/dan/GIT/GMAT-Qt \
    --manifest build/nasa-coverage/coverage95-20261104-matlab-assignment-destinations/evidence-manifest-v1.json \
    --selections build/nasa-coverage/coverage95-20261104-matlab-assignment-destinations/selections-v1.json \
    --output /tmp/matlab-destinations-audit.json

For new native execution,capture a fresh unused source version with the v2
collector,build the two MATLAB targets,the original Script Event target and
``GmatFunctionCallLabelTests`` using ``build-target-v2.py``,then use fresh run
names. Do not run old guarded versions against changed current review sources.
The dedicated runner accepts this new/affected-only filter::

  ^QtGui.Nasa(MatlabAssignmentDestination.*|MatlabFunctionIo(Scalar|Array|InOut)|AssignmentNamedCall(GmatFunction|BuiltinGmatFunction)|ScriptEventExtraCall(GmatFunction|BuiltinGmatFunction))$

Run it unattended and with ``--visible``;the visible runner requests initially
minimized,resizable,nonfullscreen nested windows. The standalone support test
is registered as ``QtAcceptance.FunctionCallLabels`` and runs directly through
CTest;it is not counted as a GUI profile.

Next action
-----------

Use the complete typed/operator witnesses for the original FRC!E176 conjunction
review. Reuse these passes. Preserve all108 original gaps,all59 hold identities,
the Assignment/CallMatlabFunction distinction,other MATLAB I/O/source questions,
and every scientific,technical,current-build and broader NASA gate. Do not
infer whole-row credit from this component seal or rerun unrelated scope cases.
