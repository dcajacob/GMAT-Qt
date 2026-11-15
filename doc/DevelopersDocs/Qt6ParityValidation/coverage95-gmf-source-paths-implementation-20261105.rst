GMF source policy and path resolution, 2026-11-05
================================================

The FRR!E480 implementation pass completes the existing SourceFormat
compatibility path under the user's instruction to support both current and
worksheet behavior. Native remains the default. LegacyASCII requires the main
function file and all included source fragments to have a nonempty basename
ending in .gmf and seven-bit ASCII contents, including comments. General include
filenames remain available in Native mode; nested function resources choose
their own policy.

The resolved main file is checked by Interpreter::CheckFunctionDefinition in
the configuration and full-definition passes. Function interpretation repeats
that entry before building the command sequence. ScriptInterpreter checks every
included file before changing its parser/stream stacks. The existing binary
byte scan rejects non-ASCII bytes/BOMs and reports offsets/read errors. A cached
sequence cannot be relabeled with another SourceFormat; function copies clear
the sequence and must build their source under the copied policy.

Repairs in this batch
---------------------

* Strict include fragments now have the same .gmf requirement as the main file.
* Main-file suffix validation uses the final filename, so dots in parent
  directories no longer hide a wrong extension. A null function pointer returns
  failure before dereferencing it.
* Relative FunctionPath values, including those without a leading dot, resolve
  against the GMAT script working directory. A valid file is a match as well as
  a directory; it is not redirected merely because a directory-only query fails.
  An existing binary-directory match remains a fallback.
* Existing directories or explicit trailing separators select directory mode,
  even when a directory name contains .gmf. Missing targets remain relative to
  the script directory unless an existing binary-directory fallback is found.
  Empty paths are rejected before indexing the string.
* File/directory resolution and the cached-source check precede committing the
  function name/path or updating the search list. A compiled sequence rejects
  a changed resolved FunctionPath until rebuild; assigning its exact current
  path is a no-op. This prevents running the old sequence under a new filename.

No class layout, public signature, command grammar, output-arity rule, fixture,
assertion or numerical tolerance changes. Earlier notes that LegacyASCII allows
non-.gmf include fragments are superseded by this implementation.

Build and tracking
-------------------

Stage: ``build/nasa-coverage/coverage95-20261105-implementation-gmf-source-paths``.
The private configured build passes 68 steps in 8.03 seconds; session 64333 is
terminal. Base, function plugin, Qt, console and dependent plugin/Python targets
were built; test/probe clients were only linked. No test, GUI, function/model
execution, numerical campaign, acceptance reader or seal was run. No installed
runtime, package, commit or push update.

The engineering tally adds FRR!E480 as implementation present: 1084/1158
(93.61%), with 31 additional original rows, 26 new/compatibility and 5 existing.
This is not acceptance evidence or a completed full implementation audit.
Recorded verification stays 1053/1158 (90.9%); all 105 original acceptance rows,
their fingerprints and qualification gates remain open and unchanged.

Later verification must cover both source modes, main/nested includes and byte
errors, relative files/directories, dotted paths and wrong suffixes, empty and
missing paths, cached path/policy rejection and rebuilding. Compilation does not
prove file-resolution or runtime behavior.
