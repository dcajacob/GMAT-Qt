Estimator SPK sampling by actual spacecraft group
=================================================

The accepted batch-estimator SPK path previously rounded the first propagation group's epoch for all targets. It also treated every active SPK writer in the publisher as a target, including spacecraft absent from the command's publication. A writer could create an output file or alter inactive-gap state before later discovering that a typed packet belonged to another spacecraft.

Sampling inputs
---------------

RunEstimator now obtains the publication columns once and intersects active SPK writer bindings with the actual published spacecraft identities. Each target selects its own numerical force-model group, precise epoch, retained internal frame and Cartesian values from the frozen publication packet. Group, epoch, frame lifetime, column bounds and finite values are checked before sampling.

The sampling input is an owning native Clone with the actual spacecraft type, epoch and frame retained. Cartesian values come from the packet rather than substituting the live spacecraft's state. Force derivatives and accumulated acceleration must be finite. The source spacecraft's weak identity, epoch, state and frame are checked again after force evaluation. A detached input does not advance a live propagator.

The existing local position/velocity sampling formula and one-microsecond maximum time cell are unchanged. SPICE rounds each actual target epoch independently. The published source state, source epoch, covariance, attitude and force/integration equations are unchanged. This implementation still requires a numerical force-model group for accepted batch SPK sampling; ephemeris-only propagator support and scientific accuracy remain open.

Writer effects and native iterative policy
-----------------------------------------

EphemerisFile returns early for a typed packet that does not contain its actual bound spacecraft. The filter precedes writer/file creation, inactive-gap marking and interpolation changes. Same-name peers and unrelated command/function spacecraft do not become substitutes. Untyped legacy packets and zero-length marker handling retain their existing routes.

End-of-run handling remains first so previously accepted buffers can close. Trial suppression and the internal background-recorder policy remain unchanged. SOLVEDPASS retains native final-replay semantics, including existing command policies for initial-guess and last-calculated results; this packet does not redefine that state as mathematical convergence. Existing batch, sequential estimator and smoother final-pass behavior is retained.

Compilation and retained evidence
--------------------------------

Configured C++ build v1 returned 0 in 5.8 seconds; last printed Ninja progress was 67/67. No warning/error diagnostic lines were emitted.

Four existing source/help preimages, two unchanged reference preimages and four planning snapshots preceded edits. Indexed patches and before/build/final source hashes are retained in the stage directory. Final source hashes match the successful build capture. Compiler completion was recovered from the subprocess receipt, final log and absence of live build processes after the original tool output was lost to context rollover; the build was not restarted. Delta whitespace is clean and both help XML files parse.

No public instance layout, virtual slot or API shape changes are introduced. Matching configured consumers compile. Runtime, numerical accuracy, other platforms, conditional non-SPICE configurations, force-provider side effects and full ephemeris lifecycle/format behavior remain unqualified. Compilation is implementation feedback, not acceptance evidence.

No runtime/GUI/MATLAB/helper/model/mission/numerical tests, new tests, acceptance auditors/readers/seals or evidence admission occurred. Original requirements, fingerprints, source text, assertions, fixtures, tolerances, denominator, gates and acceptance dispositions are unchanged. The estimate remains 1093/1158 (94.39%), with 65 unresolved engineering rows. The recorded source-versioned verified baseline remains 1053/1158; all 105 original acceptance rows remain open. The full implementation audit and goal are incomplete.

Next implementation
-------------------

Complete remaining original ephemeris format/B-1/scientific and final-iterative lifecycle obligations, including supported non-numerical propagator sampling. Preserve native final-replay semantics rather than redefining SOLVEDPASS as mathematical convergence. Continue full function input/output/scoping/resource/property/result staging, all-command target/optimizer/estimator behavior, coherent changed-component/topology/storage preparation and restoration, native solver/posterior/configuration state, shared globals/dependency lifetimes and nested MATLAB session/transport semantics. Continue remaining POV, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool, resource-tree and external/specification/historical requirements. Defer runtime verification until the full implementation pass is complete.
