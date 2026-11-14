Native resource-list snapshot replacement
=========================================

Explicit Save snapshots now reset ReportFile Add, XYPlot YVariables, OrbitPlot Add, the four writable DataFilter selection lists, SolarPowerSystem ShadowBodies, CelestialBody DSNMediaFileDirectories and ContactLocator EventTypes before recording their entries. Empty and shorter later snapshots therefore have an explicit native replacement path. Interpreter forwards exact empty object lists before reference lookup and empty DataFilter string lists before generic brace stripping. Existing native Clear actions release subscriber wrappers/plot members; XYPlot retains X, and OrbitPlot informs its integration sampler through its existing action. DataFilter uses its native empty setters and excludes read-only Strands. Existing ShadowBodies explicit-empty/default handling, DSN clearing and ContactLocator EventTypes clearing are reused. Ordinary serialization, nonempty assignment behavior, native setters, ownership policies, class layouts and Save pre-render/file I/O remain unchanged. Three source/help paths compile; Save help XML parses. Runtime reconstruction and complete any-Resource/function/resource lifecycle remain open. No row or verification credit.

Implementation
--------------

Indexed and append-style native setters can retain old members when a later saved snapshot contains fewer entries. The common writer now emits an empty assignment before each current list, including empty lists, for the named writable native properties. The existing explicit script-export/mode condition remains the gate; ordinary and MATLAB serialization retain their previous routes. Read-only/cloaked/parameter-order checks remain in the owning serialization paths.

The object-list reader recognizes unquoted empty braces before Parameter/reference lookup. Subscriber lists reuse native Clear actions; DataFilter ObservedObjects/Trackers use the native scalar empty setter. Quoted brace text is not promoted to a clear by the added object-list branch. FileNames/DataTypes empty braces reach the native DataFilter setter before generic empty-array stripping. Existing solar-shadow, DSN-directory and contact-event reader operations are reused. DataFilter Strands remains read-only and is excluded.

These are serialization and typed-input changes. Save does not clear the live source object during export. All native owner implementations, shared wrapper/invocation lease policies, report streams, plot X state and Save file operations remain byte-identical references. Public list syntax reproduces configured effective selections; class-private default-state distinctions are not generally qualified by this change. The complete any-Resource requirement remains open.

Compilation and retained evidence
---------------------------------

Build v1 returned 0 in 11.14 seconds; last printed progress 67/67; no compiler diagnostics.

Build v2 returned 0 in 11.17 seconds; last printed progress 67/67; no compiler diagnostics.

Both compiler sessions are terminal. Final hashes match successful build v2. Three changed source/help preimages, nine unchanged references and four planning preimages are retained, together with indexed/combined patches, source/build/final hashes and compiler receipts. Four additional native-owner references were captured after terminal build v1 and before the expanded edit. Changed-line whitespace is clean and Save help XML parses. Configured dependencies may relink existing probes; none were run.

No new tests, runtime list or Save/reload checks, GUI/model/helper/MATLAB/function/mission/numerical execution, acceptance auditors/readers/seals or evidence admission occurred. Original requirement text, fingerprints, fixtures, assertions, tolerances, denominator, gates and all original dispositions remain unchanged. FRC!E132 retains its original disposition. Tracking-v120 retains 1093/1158 (94.39%), 65 unresolved engineering rows; recorded verified baseline remains 1053/1158 (90.9%), all 105 original acceptance rows open. The full implementation audit and goal remain incomplete.

Next implementation
-------------------

Continue specialized any-Resource serializers, native defaults and remaining writable lists, equation/alias/input policies, hidden/read-only reconstruction, complete reference graphs and external-file portability. Keep subscriber/filter/default reconstruction and reader compatibility for deferred verification. Review original function I/O/scoping/resource/property/output staging and command/solver/estimator semantics against owning source, including retained dependencies and supported caller/session lifetimes. Continue body/frame/kernel/ephemeris, resource-tree, representative POV/exporter scope and every explicit specification/external/historical requirement. Preserve original assertions and defer runtime verification until the full implementation pass is complete.
