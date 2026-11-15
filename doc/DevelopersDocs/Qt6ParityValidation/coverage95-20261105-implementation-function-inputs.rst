Function input binding and replacement ownership
===============================================

This implementation-first increment changes FunctionManager.cpp, Function.cpp
and FunctionRunner.cpp. No new tests, assertions, fixtures, registrations,
matrices, readers or seals were added. No header, layout, virtual slot or
scientific formula changed. All existing acceptance counts remain unchanged.

Production repairs
------------------

When a formal argument already existed, CreatePassingArgWrappers copied a new
clone into it but retained the temporary clone for wrapper binding and never
released that clone. The temporary now has scoped ownership; wrappers bind to
the actual formal object in the function store. Owned argument wrappers are
created by formal name against that store, rather than leaving a caller-side
variable, property or array-element wrapper live. The shared Validator map is
restored even when wrapper construction throws. Missing or unbound wrappers
raise FunctionException.

The changing-input-type flag now accumulates across every input instead of
being overwritten by the final input. GroundStation inputs retain their
existing borrowed ownership policy. Replacing a borrowed station switches the
formal alias to the supplied station instead of copying its configuration into
the previous caller station. Nested borrowed wrapper names bind to the actual
object name without renaming the caller's station.

SetInputWrapper and GetInputWrapper validate the function, argument index and
formal count before indexing. The former impossible negative-and-upper-bound
condition is removed. A null stored wrapper can be returned safely, and setting
the same wrapper no longer deletes it. Function's indexed input/output-name
getter also rejects negative indices through FunctionException.

SetPassedInput now validates before dereferencing or renaming its input. It
replaces a literal in createdLiterals itself, rather than deleting the literal
while leaving its pointer in that map. Successful retained replacements report
ownership to FunctionRunner, including replacement of an existing entry, so
the caller does not immediately delete the manager's new input. Self-pointer
replacement avoids deletion. Replacing an existing formal updates/reuses it
instead of leaking a rejected map insertion; a changed type requests refresh.
FunctionRunner rejects a failed replacement explicitly. This caller is behind
the optional __ALLOW_MATH_EXP_NODE__ execution path, which was not enabled or
runtime-qualified here. No claim of completing that experimental path is made.

Build and reused checks
-----------------------

Artifacts are under
``build/nasa-coverage/coverage95-20261105-implementation-function-inputs``.
``preimage`` and ``incremental-v1.patch``/``incremental-v2.patch`` retain the exact
edits. Final patch SHA256:
``6d3404838fb4fb81aca567fc4a324ec8ab08de5362d3d2b5784a6918d9b8dd42``.
Reverse application checks pass.

Both private incremental builds pass, in 3.81 and 3.89 seconds. They use the
existing matched integrated-current build with GmatBase, GmatFunction, GmatQt,
GmatConsole and affected plugin links. Seven monitored installed images remain
byte-identical; public code/releases are untouched. Final image hashes and
commands are in summary.json and build-command-v2.json. Final Base SHA256 is
``e66c4e1295cf76a128d94674612f9ccf9acbaa0ab174e346648e0842da1413e6``.
Use this increment's hashes for the updated private runtime; earlier integrated
and lifetime receipts remain valid historical records of their own images.

Existing NasaFunctionIoTests, NasaFunctionBuiltinTests and AcceptanceSupport
sources were compiled unchanged against the private current frontend/core.
The prior turn's unchanged current FunctionControl executable was reused with
the new core; this increment changes no ABI. Direct compiler/linker commands
are saved under existing-io/commands.json and existing-builtin/commands.json.
No normal shared-output test build was invoked.

V1, after the formal-binding/type-flag repair, passes eleven existing native
GUI cases: Scalar, Array, String, Object, Properties and Nested I/O;
QuaternionProduct, QuaternionProductZero, StringNumber and NumberString
built-ins; and direct Recursion. QuaternionProductZero retains its original
expected domain rejection and actual GUI correction/recovery assertions.

After the additional replacement-ownership repair, final v2 repeats the selected
Properties, QuaternionProductZero and Recursion cases successfully. V1 and v2
results remain separate and identify the actual mapped private core image.
The old installed core is absent from those process maps. Original assertions,
tolerances, numeric/string/copy oracles, GUI routes and failure expectations are
unchanged. These are affected regression checks, not new guard-specific tests.

Native xcb runs use resizable nested desktops that start minimized. Zero
screenshots were created; all test/display handles are terminal. Replay uses
saved per-case command.json files, isolated startup files and the existing
AcceptanceRuntime/VisibleTestDisplay helpers with new output directories.

Deferred qualification
----------------------

Direct backfill remains for multi-argument type transitions, initialized owned
resource copies, caller mutation during built-in argument evaluation, borrowed
station switches/nesting, negative/boundary wrapper API calls and replacement
ownership/destruction with optional expression execution enabled. These checks
do not establish the universal resource/global scoping or complete function
I/O parent requirements. Error cleanup and unsupported typed materialization
remain separate implementation review areas.

No original row was admitted. Verified coverage stays 1053/1158, 105 open and
48 needed for 1101. All 59 prerequisite identities, their 31 resolved/28
unresolved disposition, six unresolved MATLAB identities and separate
historical 1816/2367 coverage remain intact. The campaign stays active/incomplete.
