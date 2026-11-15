Console batch and argument error status
======================================

Implementation-first repair in src/console/driver.cpp for the console capability
scope retained under All Others!E20. This is a concrete delivery defect; complete
console/GUI parity remains unqualified. No new suite tests, registrations,
assertions in existing tests, matrices, readers or seals. Small command-line
smoke inputs are retained separately; no GUI execution was needed.

The previous native console returned exit status zero when --run lacked an
operand or --batch named a nonexistent file. Both false-success results are
retained under before-*/ with their executable hash. These cases now raise the
existing console exception and exit with failure. Missing/empty option values,
invalid verbosity and unknown options also fail explicitly. Invalid verbosity
no longer prints an uninitialized scriptfile buffer. Startup/log options are
validated before Moderator initialization, including a lone missing option.

Batch extraction now processes the final entry even without a trailing newline.
Percent comments consume the rest of their line; ordinary legacy whitespace
entries remain supported. Quoted entries preserve spaces, apostrophes and
backslashes. Matching outer filename quotes are removed without deleting
interior apostrophes. A malformed quoted entry is rejected. Batch execution
continues after individual script failures, prints its existing statistics and
failed-script list, then exits with failure if any script failed. Missing or
unreadable batch lists also fail instead of appearing successful.

Interactive token input uses std::string instead of an unbounded extraction
into a fixed buffer, and EOF ends the loop. Its existing token-oriented command
semantics and GUI-interface exceptions are unchanged. Headers, ABI, numerical
algorithms, original tolerances and requirement scope are unchanged.

Build and bounded smoke
-----------------------

Stage: build/nasa-coverage/coverage95-20261105-implementation-console-status.
Exact preimage and incremental-v1.patch are retained. Patch SHA256:
``29d40205708104193cdbd34a59b85d147027fe08355594700d024153813072ea``.
The private GmatConsole rebuild passes in 1.22 seconds without warnings/errors.
The private core image remains the previous SPK-metadata version. Seven
monitored installed images and the original shipped math sample are unchanged.
Public code/releases are untouched; summary.json records current image hashes.

Seven small CLI smokes pass: missing run operand, missing batch file, help,
unchanged shipped Ex_GMATFunction_Math, a valid batch without a final newline,
a mixed failed/successful batch without a final newline, and interactive EOF.
The valid batch has one whole-line comment and one quoted sample path containing
spaces and an apostrophe: one success, zero failures, one skip, status zero.
The mixed batch retains one failure and still runs its valid final entry: one
success, one failure, zero skips, status one. All three successful sample
executions report [0.25,-2.75,-2] through the actual runtime ReportFile.

Commands, temporary batch inputs, raw logs and status/report receipts remain
under before-*/after-*. Loader logs identify the private core and exclude the
older installed core. Display variables were removed, GUI executions and
screenshots are zero, and all handles are terminal. Replay uses command.json
files and the existing startup-isolation helper with fresh output directories.

New reusable CLI tests and full parser/filesystem/error/platform/capability
backfill remain deferred. These bounded smokes do not qualify the universal
console-parity requirement or Ubuntu/platform gates. No original row admitted:
verified 1053/1158, 105 open and 48 needed for 1101. All 59 prerequisite identities
and separate historical 1816/2367 coverage remain unchanged. Goal active and
incomplete.
