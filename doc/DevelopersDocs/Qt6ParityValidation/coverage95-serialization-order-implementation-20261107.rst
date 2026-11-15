Resource serialization property order and owned storage
======================================================

The common GmatBase serializer previously completed a nonempty property order by appending only GmatBase IDs. A remaining count mismatch constructed an exception but did not throw it. The subsequent loop could index beyond the order, omit fields, repeat IDs or use invalid IDs. Preparing a script also changed the live ordering metadata.

Local native ordering
---------------------

An internal header-only helper validates the current count and the preferred identifiers before property access. It retains each preferred identifier once in its original order, rejects duplicates/out-of-range IDs and appends every missing current native ID in ascending native order. A partial preferred order is supported without assuming that only base-class properties are missing. The helper returns local owned storage; no serialization cache is installed on the live resource.

Existing hardware constructors intentionally order class properties while leaving inherited base fields for completion. Thruster deliberately places axes before direction values. Those definitions are unchanged, and the helper preserves their relative order. Native read-only/cloaking/default and mode policies still decide which ordered properties are written. This does not make hidden fields writable or invent persistence setters.

Spacecraft owns its order storage
--------------------------------

Spacecraft's specialized serializer formerly allocated a raw array and released it only at the successful end. State-representation conversion, property getters and owned serializers can throw before that release. Its preferred order now uses an IntegerArray and the same local completion helper. It honors the current virtual parameter count and cannot overflow the order buffer while constructing the preferred sequence.

Static source comparison retains all 56 original preferred identifier occurrences in exactly the same sequence, including the conditional ephemeris-name placement. State representation, epoch/frame conversion, parameter getters, attitude/owned serialization, filtering and value formatting are unchanged. This is storage/metadata implementation, not numerical qualification.

Compilation and remaining scope
-------------------------------

Configured build v1 returned 0 in 12.53 seconds; last printed Ninja progress 67/67. No warning/error diagnostic lines. Session 77316 terminated with runner/compiler return 0.

Three existing source/help preimages, one new-header absent marker, two unchanged hardware reference preimages and four planning snapshots precede edits. Indexed patches, before/build/final hashes, source map, compiler log/terminal receipt and the retained spacecraft identifier sequence are in the stage directory. Final source hashes match the successful build capture. Added-line whitespace is clean and Save help XML parses. No existing instance layout, virtual slot or public method signature changes; matching configured consumers rebuilt. Other-platform/toolchain/runtime behavior remains unqualified.

Save continues to render before output I/O, so these metadata errors propagate before Save opens or truncates files. No new class-specific serializer, quoting convention, default/list setter, hidden/read-only reconstruction, complete reference-graph export or external-file packaging is claimed. FRC!E132 remains open at its full any-Resource scope.

No runtime/GUI/MATLAB/helper/model/mission/numerical tests, new tests, acceptance auditors/readers/seals or evidence admission occurred. Requirements, fingerprints, source text, assertions, fixtures, tolerances, denominator, gates and original dispositions remain unchanged. Tracking-v107 remains 1093/1158 (94.39%), 65 unresolved engineering rows; recorded source-versioned verified baseline 1053/1158 (90.9%), all 105 original acceptance rows open. The full implementation audit and goal are incomplete.

Next implementation
-------------------

Finish the original any-Resource persistence scope through the remaining class-specific serializers, default/list resets, string literal handling, native input/alias modes, hidden/read-only reconstruction, actual dependency graphs and external-file portability. Continue the full original function/solver/estimator, resource-tree, POV, body/frame/kernel and ephemeris worklist and explicit specification/external/historical dependencies. Runtime verification follows the complete implementation pass.
