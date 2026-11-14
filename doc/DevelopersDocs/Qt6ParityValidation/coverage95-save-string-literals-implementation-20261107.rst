Save byte literals and explicit empty String snapshots
=====================================================

Explicit Save snapshots now use a new ASCII byte-literal form for native string/filename properties, quoted string-list entries and runtime String-resource values when ordinary quoting is unsafe. Printable simple values retain ordinary quotes; apostrophes, control/non-ASCII bytes and structural punctuation are encoded as paired hexadecimal bytes in an explicit @-prefixed quoted literal. Ordinary quoted interiors, Windows backslashes, repeated apostrophes and global StringUtil behavior are unchanged. Script/Math/Validator admission, property/String input and function literal materialization recognize the new form and decode at typed input boundaries. Encoded data does not become a resource-name alias. Empty String snapshots now include an explicit empty assignment. Ordinary script/MATLAB serialization retains its previous writer route, and String export mode guards restrict the new output to explicit script-export modes. Ten source/help paths (one new internal header) compile; Save help XML parses. Save pre-render-before-file-I/O and function ownership/result/solver code remain unchanged. New encoded snapshots require the updated reader; specialized serializers, equations, external portability and full any-Resource/function scope remain open. No string/script/runtime round-trip verification or row credit.

Representation and input boundaries
-----------------------------------

The existing parser strips ordinary single-quote delimiters and retains the interior bytes. A global unescape change would reinterpret existing Windows paths or repeated apostrophes. The new internal helper instead recognizes an explicit @-prefixed quoted hexadecimal form. Each pair of digits represents one byte; both input hex cases are accepted, and serialization uses uppercase digits. Empty payloads are valid. Missing closing delimiters, odd digit counts and nonhex digits produce a native UtilityException. Output size is checked before reserving the encoded representation.

Simple printable ASCII retains its prior single-quoted form. Values containing apostrophes, control/non-ASCII bytes, brackets/braces, semicolon, percent or equals characters use the encoded form. The payload therefore cannot be split into physical lines, comments or structural property tokens before it reaches its typed reader. Encoding preserves the original bytes rather than changing Unicode, slash or newline conventions. This is a new format extension, not a claim that an older NASA/GMAT reader already understands it.

Literal admission now covers assignment classification, MathParser literal classification, Validator wrapper/parameter decisions, function-call input admission and native function literal materialization. Property and String-resource readers and StringWrapper decode once at the input boundary. Native setters and evaluated resource-to-resource values remain raw values. Explicit encoded literals always denote string data in the wrapper builder, including when their decoded bytes also name a resource; legacy quoted object-name lookup keeps its previous branch. The function literal cache and every other FunctionManager ownership/result/solver path compare unchanged after normalizing the two deliberate literal decisions and include.

Snapshot writing and compatibility
----------------------------------

The common native STRING_TYPE/FILENAME_TYPE writer and quoted list writer use the helper only during explicit script export. EQUATION_TYPE and ordinary/MATLAB writer routes retain their existing behavior. StringVar uses the same explicit script-mode guard and now emits an empty assignment in that scope. Save already snapshots current String Value into a detached expression and clears its alias, so this emits the runtime bytes and lets a later empty snapshot clear an earlier value. The live String dependency/expression is not edited.

Save command source and global StringUtil source compare byte-identical to preimages. Snapshot rendering still finishes before any output file is opened, so encoding failures do not start the file-writing phase. No new plugin protocol, class layout, virtual slot or external endpoint is introduced. Configured core/plugin/frontend binaries were rebuilt together. Encoded snapshots require the updated reader; older readers and specialized class serializers are not qualified by this change. Full native string/equation/alias/default/list/reference/external-file persistence and the complete original function I/O/scope/command requirements remain open.

Compilation and retained evidence
---------------------------------

Configured build v1 returned 0 in 13.76 seconds; last printed progress 73/73. No compiler diagnostics.
Configured build v2 returned 0 in 3.68 seconds; last printed progress 66/66. No compiler diagnostics.

Nine existing source/help preimages, one new-header absence, two unchanged references and four planning snapshots preceded the corresponding edits. Indexed patches, before/build/final hashes, source/research notes, compiler logs and terminal receipts remain in the stage. Final sources match v2, changed-line whitespace is clean and Save help XML parses. Existing configured test/probe binaries were relinked as build dependencies; none were executed.

No string/literal/helper/parser/GUI/MATLAB/function/model/mission/numerical runtime tests, new tests, Save/reload round trips, malformed-input checks, acceptance auditors/readers/seals or evidence admission occurred. Original requirements, fingerprints, source text, fixtures, assertions, tolerances, denominator, gates and dispositions remain unchanged. FRC!E132 and FRR!E450 retain their original dispositions. Tracking-v118 retains 1093/1158 (94.39%), 65 unresolved engineering rows; source-versioned verified baseline 1053/1158 (90.9%), all 105 original acceptance rows open. The full implementation audit and goal remain incomplete.

Next implementation
-------------------

Finish remaining class-specific any-Resource Save serialization: specialized string/equation and alias/input modes, native list/default resets, hidden/read-only reconstruction, reference graphs and external-file portability. Review the remaining original function I/O/scoping/resource/property/output staging and command/solver/estimator semantics against actual owning code, including retained dependency contexts and supported caller/session lifetimes. Continue body/frame/kernel/ephemeris, resource-tree, representative POV/exporter scope and all explicit specification/external/historical work. Preserve the full original requirement set and defer runtime verification until the implementation pass is complete.
