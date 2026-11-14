Math-node active-state Clone and named main repair — 2026-11-03
================================================================

Coverage remains **1,019/1,158 (88.0%)**, 139 open and **82 more needed**.
Historical operations remain 1,816/2,367 (76.7%). No exact requirement is newly
admitted. Authority fingerprints, original denominators/assertions, whole-row
conjunctions, all 59 holds and separate qualification gates remain unchanged.

Demonstrated failure and one-file repair
---------------------------------------

The preceding named-object census retained 44 math-node main Run failures with
an unsized-table diagnostic. New direct native controls reproduce 266 failures
in one failed CTest invocation: six controls for every advertised spelling
(264), plus two protected stored-string controls. The spellings map to 42
concrete classes; Deg2Rad/DegToRad and Rad2Deg/RadToDeg aliases remain separate
entries. No factory name or failed prior observation is removed.

The six controls are default Clone, literal scalar Clone, literal matrix Clone,
MathNode-base scalar assignment, MathNode-base matrix assignment with a different
destination shape, and matrix reshape. Scalars use 17.25; matrices use independent
2x3 [2,-3,5.5,7,11,13] and 1x2 [-11,17] data. Controls also check actual
function-input/number flags, source/clone independence, name preservation and
self-assignment. A concrete MathElement derivative exposes the existing
protected stored-string field to check both base copy paths with orbit Ω string.
These are storage/copy controls, not expression evaluations.

The original failures comprise 132 unsized-table errors, 88 dimension errors
and 44 lost-value/flag comparisons, plus two unsized stored-string failures.
The copy constructor incorrectly copied isFunction into isFunctionInput,
unconditionally copied its default unsized matrix and omitted the stored string.
Base assignment also copied an unsized/differently-sized matrix and omitted the
string. SetMatrixValue did not resize an already-sized cache.

MathNode.cpp now copies a matrix cache when it is sized, preserves the actual
function-input flag and stored string, resizes sized destination caches before
assignment, and resizes for a new valid SetMatrixValue shape. All 266 controls
pass with their original literals, flags and assertions. Exact preimage and
core-v3.patch retain the one-file increment. There is no new member, virtual
slot or mathematical formula, and TableTemplate/Rmatrix error guards are unchanged.

The contract concerns active typed state. A scalar assignment can retain an
unused destination matrix cache, as SetRealValue already does; a later matrix
setter resizes before storing a new shape. Inactive cache bitwise equality,
derived polymorphic Copy, child/wrapper graph ownership and every initialized
expression state are not qualified by these controls.

Native GUI integration and source custody
----------------------------------------

All 44 unchanged named-main probe cases now Create, appear under the requested
configured name, Build and complete a named Write mission, appear with the
expected runtime name/type, report independent continuation 42, and recover to
a separate control mission without the old configured object. Both modes have
the same successful outcomes. Source/history invariants and raw source/messages/
reports remain. These are bare named-object Create/Write routes, including nodes
without an operand graph; no mathematical-function calculation is inferred from
their default object serialization.

Three unchanged affected regressions also pass in both modes: Array function
frames, direct recursion and mutual recursion. Their assertion-source hashes
match the preceding attitude checkpoint. Final selection is **94 GUI passes**:
88 repaired named-main cases (44 per mode) and six original regression passes.
The new named cases provide 88 completed Write missions and 88 independent
recovery controls. Native support remains separate from GUI/mission totals.

Headless v3 runs the 44 new repaired routes; the separate regression invocation
runs only the three affected cases. Visible v1 passes all 47 cases with 47 actual
mapped processes and unchanged before/after source/runtime hashes. Windows are
initially minimized, resizable and nonfullscreen; zero screenshots are retained.
There are no runtime ELF or kernel copies, and no physical-GPU/human-observation
claim. Every build/native handle is terminal. One failed native support invocation
with all 266 failed controls remains; there are no GUI, compile or setup-only
failures and no superseded passes in this stage. Previous named-main failures
remain in their original source-versioned stage.

Exact executed source capsules preserve native support v1/v3, headless routes,
headless regressions and visible v1 separately. Five corrupt-proof controls reject
a lost input flag, wrong matrix literal, changed destination name, omitted
factory spelling and failed named main mission. The complete prior 566 rows
remain unchanged, with two new main-use contexts on each of the 44 rows and
every whole-scope qualification flag false.

Authoritative stage: build/nasa-coverage/coverage95-20261103-function-math-clone/.
Use evidence-audit-v1.json, evidence-inputs-v1.json, stage-receipt-v1.json,
seal-verification-v1.json and the executed/final source capsules.
math-main-matrix-v1.json retains all candidate roles and prior observations.
The current schema-2 core inventory is
build/nasa-coverage/core-change-review/20261103-function-math-clone/:
**228 pending paths**, 215 tracked differences and 13 untracked files. All prior
227 working hashes are unchanged; MathNode.cpp is newly pending. This is a
worktree-versus-HEAD review aid, not exclusive authorship, a release comparison
or technical/scientific approval.

Remaining obligations and focused replay
----------------------------------------

Named main use is now demonstrated, so add actual function-private/Global parity
for all 44 entries next. Direct MathNode-base assignment is not derived
polymorphic Copy; do not assume private reset or graph ownership is solved.
Retain all owner routes, automatic-global 28 and owner-bound parameter 321
obligations, reuse existing resource/owner evidence, and preserve the original
E491 parent conjunction. Whole E491/E493/E497 stays open; already-counted children
must not be awarded again. E477/SaveMission, E406/terminal output, prior source/
science conflicts and all 59 holds remain. Canonical coverage remains the
default-orientation coverage-snapshot-v1.json. The full goal is active/incomplete.

::

  cmake --build build/linux-gui --target GmatMathNodeCloneTests GmatQtNasaFunctionOwnedNamedTests -j4
  ctest --test-dir build/linux-gui --no-tests=error -R '^QtAcceptance.MathNodeClone$' --output-on-failure
  python3 src/qtgui/tests/RunFunctionMathClone.py --filter '^QtGui.NasaFunctionOwnedNamedAdd$'
  python3 src/qtgui/tests/RunFunctionMathClone.py --visible --filter '^QtGui.NasaFunctionOwnedNamedAdd$'
  python3 src/qtgui/tests/AuditFunctionMathClone.py --stage build/nasa-coverage/coverage95-20261103-function-math-clone --output /tmp/function-math-clone-audit.json
  python3 src/qtgui/tests/TrackCoreChanges.py --verify build/nasa-coverage/core-change-review/20261103-function-math-clone

The runner defaults to all 44 named probes and supports --filter/--output.
Existing named registrations are reused; both acceptance build targets include
the new support executable. Native X/engine locks/timeouts remain; X-socket
launches require sandbox escalation. Reuse completed versions and run only new
or affected mechanisms. Replay instructions do not claim additional executions.
