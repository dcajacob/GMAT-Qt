Save snapshots with native kernel and SolveFors list resets
==========================================================

Save appends resource snapshots during a run. Native script assignments set list entries by index, so a later shorter list can retain the earlier tail. An empty list may be omitted entirely by a specialized serializer. Spacecraft's list writer bypassed the common explicit-export handling. For SolveFors, the interpreter also discarded a braced empty value before reaching Spacecraft's existing complete-clear setter.

Existing native clear contracts
-------------------------------

Under ObjectScriptExportScope and script export modes, the common list writer emits a complete-clear assignment before the current contents of writable OrbitSpiceKernelName, AttitudeSpiceKernelName, PlanetarySpiceKernelName, SCClockSpiceKernelName and FrameSpiceKernelName lists on SpacePoint resources, and SolveFors on Spacecraft. Empty current lists emit only that reset. Nonempty current lists then use their existing formatting and indexed assignment path. The native property names include the celestial-body PlanetarySpiceKernelName alias.

The SpacePoint setters already parse a complete braced kernel list and dispatch to their typed full-list setter. These source files are unchanged and retained as reference preimages. The implementation uses that established clear boundary rather than inventing a new kernel list setter or altering kernel selection algorithms.

The interpreter now forwards a braced-empty Spacecraft SolveFors value to the existing SetStringParameter clear branch before removing braces. That native setter and its command-mode policy are unchanged. This is script configuration/persistence behavior; it does not qualify changing a live estimator's assembled covariance or bypass command-mode restrictions.

Spacecraft explicit script exports now delegate array values to the common writer. The existing MATLAB scope remains first-class and ordinary script generation outside ObjectScriptExportScope follows the prior route. Read-only/cloaking gates, field ordering, epoch/state conversion, native value getters and other list-specific contracts remain in force. No unsupported thruster attachment clear convention is introduced by this packet.

Compilation and retained evidence
---------------------------------

Configured build v1 returned 0 in 13.49 seconds; last printed Ninja progress 68/68. No warning/error diagnostic lines. Session 35067 terminated with runner/compiler return 0.

Four existing source/help preimages, one unchanged SpacePoint reference preimage and four planning snapshots preceded edits. Indexed deltas, before/build/final hashes, source map, source review, compiler log and terminal receipt remain in the stage directory. Final sources match the successful build capture, added-line whitespace is clean and Save help XML parses. No existing class layout, virtual slot or public signature changes. Matching configured consumers compile; other-platform/toolchain/runtime behavior remains unqualified.

Native kernel validation and full-list setters retain their existing configuration/cache side effects. This source change does not prove SPICE-pool/provider lifetime, scientific equivalence, reader initialization or external-file portability. Other object/string lists retain their own native policies. The full any-Resource persistence scope, specialized serializers, quoting, defaults and hidden/read-only/reference reconstruction remain open.

No runtime/GUI/MATLAB/helper/model/mission/numerical tests, new tests, acceptance auditors/readers/seals or evidence admission occurred. Original requirements, fingerprints, source text, assertions, fixtures, tolerances, denominator, gates and dispositions are preserved. Tracking-v108 remains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%), all 105 original acceptance rows open. The full implementation audit and goal are incomplete.

Next implementation
-------------------

Complete remaining class-specific any-Resource persistence: other list/default resets within actual native contracts, string literal handling, input/alias modes, hidden/read-only reconstruction, actual reference graphs and external-file portability. Continue the full original function/solver/estimator, resource-tree, POV, body/frame/kernel and ephemeris requirements and explicit specification/external/historical dependencies. Runtime verification follows the full implementation pass.
