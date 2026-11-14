Explicit Save exports across source-file boundaries
===================================================

GmatBase intentionally suppresses declarations that came from another source file during ordinary script saving. Interpreter marks included-file resources with IsCreatedFromMainScript false, and GetGeneratingString returns an empty string unless force generation or a diagnostic mode is requested. Save previously used ordinary SCRIPTING mode without an explicit generation request. It could therefore render an empty resource fragment. Nested owned-object serializers apply the same common filter independently, so enabling only a root flag would not address the complete path.

Scoped native export request
----------------------------

ObjectScriptExportScope is a small exported native stack type with a thread-local active request implemented in GmatBase.cpp. Construction saves the prior request; destruction restores it, including exceptional unwind and nested exports. Copy/assignment are disabled. No GmatBase instance fields or virtual slots change, and no resource source-file/force-generation flags are temporarily modified.

Within the scope, GmatBase's creation-source filter permits SCRIPTING, OWNED_OBJECT, NO_COMMENTS and OBJECT_EXPORT. SHOW_SCRIPT and DEBUG_INSPECT keep their existing behavior. MATLAB_STRUCT and EPHEM_HEADER do not gain a new bypass. Outside the scope the existing source-file filter and per-object force-generation option behave as before.

Save activates the scope around attachment-graph collection, duplicate runtime-state comparisons and all output rendering. Consequently same-name included resources do not compare as two empty strings merely because their main-source flags are false. Ordinary nested owned serializers inherit the active request through their existing call stack; no additional owned/reference traversal or initialization is introduced. The request ends before any output file is opened, truncated, appended or flushed. Existing render-before-I/O ordering remains in place.

This follows the intent of the existing explicit Write command's force-generation request while avoiding mutation of saved live resource flags and carrying the request through nested owned serializers. Write itself is unchanged.

Native boundaries preserved
---------------------------

Object/parameter cloaking and read-only policy remain native. The change does not make every hidden descriptor writable or serialize arbitrary runtime caches. Default-cloaked SolarSystem/body fields and default resets in appended resource snapshots still require owning-source persistence work. It does not claim complete external-file portability, arbitrary reference-graph reconstruction, representation/initialization coverage or universal scientific equivalence.

Existing class-specific serialization, attachment/root priority, propagator force-model handling, runtime String snapshots, array-zero resets, built-in coordinate-system handling and both filename styles retain their behavior. Native mission commands are not made Resource objects by this request. Legacy command-specific source filtering remains independent. No force, covariance, attitude, propagation, event or solver formula changes.

The help chapter now states that explicit resource snapshots include serialized owned components declared in included/function sources while ordinary script saving retains the original source-file association.

Compilation and remaining scope
-------------------------------
Configured compilation v1: 67/67 steps, 8.25 seconds, return 0.

Four paths have preimages before their first edits, with an explicit absent-file marker for the new scope header. Final hashes match the successful configured compilation snapshot. Indexed deltas use /dev/null for that new header. Logs and the terminal compiler result are retained; added-line whitespace is clean and the help XML parses. Matching configured native/Qt/console/plugin/Python consumers linked against the updated core as required.

The batch adds an exported nonvirtual scope class; existing class instance layouts and virtual slots are unchanged. External prebuilt binary compatibility and Windows/macOS toolchain/runtime qualification are not established by the Linux build.

No runtime/GUI/mission/numerical tests, acceptance audits/readers, evidence admission, assertion/fixture/tolerance changes, denominator changes, packages, installed/shared runtime changes, commits, pushes or external messages were performed. Compilation establishes source integration only. Original FRC E132 remains open at its full any-Resource scope; partial implementation earns no parent or verification credit.

Tracking-v71 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
