# Transmitter solve-for control — 2026-10-04

The selected estimation plugin exposes a writable `Transmitter.SolveFors` string
array. Qt's generic resource form omitted this type from its supported lists,
so the field was absent from both creation and editing. The form now offers the
engine-supported **FrequencyBias** choice and permits clearing the list.
Other names and duplicates are rejected without changing source or configured
objects. This is an engine-supported operation; no broader wx save-parity claim
is inferred from its generic string-array panel.

Three narrow ResourceProperties changes expose the list, supply the picker
choice and reject unsupported entries. All use exact Transmitter type checks,
so inherited Oscillator schemas and other solve-for types are unchanged. The
existing string-array serializer emits `{'FrequencyBias'}` or `{}`; Apply and
reopen reconstruct the engine objects through the existing interpreter.
No engine, plugin or estimator calculation code changed.

## Reproduced failure and focused result

The new check first failed while building its fixture because it assigned the
inactive Antenna.Delay field. That assignment was removed; the valid default
Antenna is sufficient for PrimaryAntenna. The corrected pre-fix check then failed
in **0.19 s** with `Writable transmitter SolveFors GUI control is omitted`.
After the three production hunks and actual GmatQt rebuild,
**QtGui.TransmitterSolveFors passes in 0.32 s**.

The check uses the actual resource creator, visible property and selection
dialog, and resource-tree editing panel. It verifies pending selection, picker
and parent Cancel/discard, creation, FrequencyBias/empty-list Apply, wrong-name
and duplicate rollback, exact unrelated UTF-8 source, one transaction per
successful Undo/Redo, and Unicode Save bytes/fresh reopen. The complete new
workflow passes without running any numerical mission or estimator. Exact-type
review found no affected existing suite requiring repetition. Completed plugin,
example, tutorial and workflow audits remain at their original recorded scope.

[Build logs](transmitter-solvefors-build-20261004.txt) and
[check logs](transmitter-solvefors-checks-20261004.txt) retain the fixture failure,
corrected missing-control baseline and passing repair. Committed logs trim
trailing whitespace; exact originals and final detailed CTest output remain in
`build/linux-gui/transmitter-solvefors-20261004/raw/`.

## Runtime and qualification scope

The rebuilt actual `application/bin/GmatQt` is **6,230,920 bytes**, SHA256
`ca4f6f601b4c82ddaf72ab564053412134443cde452331144722c6182131e8f5`.
The [receipt](transmitter-solvefors-receipt-20261004.json) binds application, base,
util and Qt startup; the latter three remain byte-identical. Work stays in
`/home/dan/GIT/GMAT-Qt` on `codex/qt6-gui`; the original checkout is untouched.

The earlier [segment-label checks](segment-labels-20261004.md) remain bound to
their recorded app. This focused offscreen resource check is not native host or
physical-GPU acceptance. Live host testing remains stopped; the saved Mutter
null Wayland resource fault is established, while GMAT client attribution and
host repair remain unproven. All nine gates and held/domain/dependency limits
remain in scope. Windows/macOS/MATLAB remain deferred; full qualification is not
claimed.
