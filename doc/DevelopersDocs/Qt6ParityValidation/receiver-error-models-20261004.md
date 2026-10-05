# Receiver error-model selection — 2026-10-04

The selected estimation plugin's writable `Receiver.ErrorModels` object list
was absent from Qt creation/editing because the generic list whitelist covered
GroundStation.ErrorModels only. Receiver now has an ordered selector of existing
ErrorModel resources, with pending selection, Cancel and Apply. Clearing removes
the assignment so a freshly interpreted Receiver keeps its empty default;
explicit `{}` is not accepted by the Receiver name setter.

Validation rejects missing resources, wrong types, duplicate names and two
models of the same measurement type. The latter follows Receiver.cpp746–751:
the engine warns and silently ignores a later distinct model of an already
bound type. Internal signal-path clone replacement is a different same-name
operation; the new guard applies only to authored Receiver.ErrorModels edits.
Unrelated edits retain imported source. No engine/plugin code changed.

## Focused evidence

The new **QtGui.ReceiverErrorModels** check first reproduced the absent writable
control in **0.20 s**. The implemented workflow now passes in **0.41 s**:

- Actual resource creator and visible typed selector; filtering excludes the
  antenna and variables, and selected model order is retained.
- Picker/parent Cancel, pending state, creation and editing, reorder, clear and
  restore, with wrong-type/missing/duplicate-name/type rollback.
- Exact unrelated UTF-8 source and original ErrorModel parameter values through
  reconstruction; one Undo/Redo transaction per accepted change.
- Unicode Save bytes and fresh reopen; ErrorModel Rename updates Receiver's
  references and reopened form, with exact source/reference restoration by Undo.

The first fixed run reached clear successfully but failed a test expectation:
the established source patcher leaves an empty insertion row when the last
explicit field is removed. A diagnostic proved the configured list was empty
and the form clean. The test now asserts the exact scoped clear/restore source
including that row, without modifying shared source-patching behavior.
The original failure and diagnostic remain in the logs. Only this new failing
check was retried; completed estimation/GPS missions and other passing suites
were not repeated.

[Build logs](receiver-error-models-build-20261004.txt) and
[check logs](receiver-error-models-checks-20261004.txt) retain the missing-control
baseline, expectation correction and final pass. Committed copies trim trailing
whitespace; exact logs and detailed CTest output remain under
`build/linux-gui/receiver-error-models-20261004/raw/`.

This check qualifies the new configuration/source transaction. Existing selected
plugin execution evidence remains at its recorded scope; no new GPS measurement,
sandbox binding or numerical-regime pass is claimed. GPSAdapter.cpp458–468
consumes the Receiver's bound error models, which explains the operation's use;
this source fact is not a substitute for executing a new measurement regime.

## Rebuilt runtime and remaining scope

Actual GmatQt is **6,235,016 bytes**, SHA256
`2ef7d511ff9d19436616f81e9f76fbe00caa81b00264bbeb10b2ffd970cd2b30`.
The [receipt](receiver-error-models-receipt-20261004.json) binds the executable,
base/util and Qt startup; the latter three remain byte-identical. All work stays
in `/home/dan/GIT/GMAT-Qt`; the original checkout remains untouched.

A separate [targeted OF source review](of-remaining-controls-20261004.md)
identifies still-missing active LastN solver retention and independent axes/frame
labels. These remain implementation requirements, not accepted warning-only
parity. All nine gates remain intact. Live host testing stays stopped; Mutter's
null Wayland resource fault is established but GMAT client attribution, host
repair and physical-GPU acceptance remain unproven. Held scientific cases,
external dependencies and Windows/macOS/MATLAB deferrals retain their existing
dispositions. Full qualification is not claimed.
