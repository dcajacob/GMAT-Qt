MATLAB session contracts and character transfer
==============================================

This batch addresses session selection, transport recovery and character-value transfer in the existing C Engine interface. It does not complete the original full MATLAB I/O or session-mode requirements.

Session selection and platform behavior
--------------------------------------

Open and Close dispatch on the selected mode before entering platform helpers. A non-Windows private-session request is rejected before changing state. An unnamed private open uses the same GmatMatlab key as ordinary MATLAB function calls, avoiding a fresh engine for every workspace open. Explicitly named private engines remain supported on Windows. Both command initialization and execution select/check the canonical default; MatlabWorkspace checks opening and selects it after each Parameter evaluation before publication. These changes do not open an engine while a function value is merely copied or rebound.

The legacy Mac helper keeps repeated opens idempotent and stages the new Engine pointer until format setup succeeds. A failed setup closes the candidate before publishing it. Mode dispatch no longer bypasses the disabled mode on that branch. No new shared-session emulation is introduced on Unix.

Qt describes Windows shared/private sessions and the process-local Unix session accurately. The unsupported private choice remains disabled on non-Windows platforms. Existing startup mode spellings, GUI cancellation and live-engine mode-change restrictions remain intact. The help chapter describes the same default selection and native named-private extension.

The vendor distinguishes the platforms: engOpenSingleUse is Windows-only; engOpen on Unix starts a process, and the C Engine cannot attach to an already running MATLAB session on Linux/macOS. Consequently the worksheet's literal cross-GMAT shared-engine wording is not claimed to be implemented on Unix. See the primary vendor references below.

Command-boundary recovery
-------------------------

MatlabEngineConnectionException derives from InterfaceException and represents the native nonzero engEvalString transport result. MATLAB-reported semantic errors remain ordinary interface exceptions. CallMatlabFunction catches only the transport type around the complete execution attempt, closes/reinitializes and retries once. A second transport error propagates. The input helper no longer catches by diagnostic substring or recursively invokes Execute, which could resume the enclosing helper and execute the outer command again.

This bounds automatic recovery; it does not promise exactly-once external MATLAB side effects when a connection disappears after execution. Initialization/open and close failures still propagate through their supported paths. Full transactional staging of all output families remains unfinished.

Character values and paths
-------------------------

CharacterLiteral constructs a MATLAB character expression, doubles payload apostrophes and encodes newline, carriage-return and NUL with char expressions. Internal directory/path changes use functional syntax with this literal helper rather than tokenizing a command-form cd and discarding text after its first space. StringVar inputs and string-valued Parameters share the encoder. Parameters evaluate once through EvaluateString; a valid string resembling a bracketed vector remains a string.

GetString owns the returned mxArray through every exit. It reads all native mxChar units, converts UTF-16 to UTF-8, rejects invalid/incomplete conversion and publishes the replacement only after success. This removes the fixed engine-output-buffer truncation and frees wrong-type results as well. Embedded NULs remain part of the std::string value; engine diagnostics retain their independent buffer. Native character arrays keep their previous column-major flattening convention. This change does not introduce the modern MATLAB string-class contract or claim that every downstream GMAT consumer preserves all embedded control characters.

The configured compiler reports the standard-library deprecation of wstring_convert; compilation succeeds under the project's current C++ standard. Replacing that conversion facility for a future standard/toolchain is separate maintenance work.

Compilation and limits
----------------------
Configured integrated compilation v1: 15/15 steps, 17.17 seconds, return 0.
Configured integrated compilation v2: 6/6 steps, 2.53 seconds, return 0.
Legacy Mac conditional syntax compilation: 0.51 seconds, return 0, using the Linux compiler and installed R2026b SDK. This is not macOS qualification.

All six source/help paths match the final successful integrated compilation snapshot and the conditional-syntax snapshot. Preimages precede the first edits; indexed deltas, final hashes, logs and recovered terminal compiler results are retained in the stage directory. Added-line whitespace is clean and the help XML parses. These are source checks only. Windows conditional paths were not built with a Windows toolchain.

The new interface methods are nonvirtual. The exception is a new derived type; existing instance fields and virtual slots are unchanged by this batch. Configured consumers rebuilt as required. External prebuilt binaries and cross-platform runtime behavior are unqualified.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers, evidence admission, assertion/fixture/tolerance changes, denominator changes, packages, installed/shared runtime changes, commits, pushes or external messages were performed. Original worksheet source text, fingerprints, dispositions and gates are preserved. All Others E241-E244 remain open; broad function parents receive no partial credit. FRR E479, the MATLAB-to-GMAT call restriction, was not implemented or credited by this batch. Whole-object/member encoding, full output ownership/staging, all original resource/scientific scopes and platform dependencies remain open.

Tracking-v69 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. The full goal is active and incomplete.

Primary vendor references
-------------------------

* `engOpen <https://www.mathworks.com/help/matlab/apiref/engopen.html>`_
* `engOpenSingleUse <https://www.mathworks.com/help/matlab/apiref/engopensingleuse.html>`_
* `Attaching to an existing MATLAB session <https://www.mathworks.com/help/matlab/matlab_external/attaching-to-an-existing-matlab-session.html>`_
* `engClose <https://www.mathworks.com/help/matlab/apiref/engclose.html>`_
* `mxGetString <https://www.mathworks.com/help/matlab/apiref/mxgetstring.html>`_
* `mxGetChars <https://www.mathworks.com/help/matlab/apiref/mxgetchars.html>`_
