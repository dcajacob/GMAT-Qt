Ephemeris clock and publication transitions
===========================================

Ephemeris state publication now resolves the requested epoch, evaluates the native provider and only then converts that fresh state to the propagated-object origin. Both Real and precision companion clocks are synchronized during publication, SetTime and buffered reversion; reversion takes the captured epoch directly rather than reconstructing it through elapsed-time subtraction/addition. STK refresh converts its state at the actual Real or precision query epoch instead of lastEpoch. Explicit-duration Step restores the selected default ephemStep on success, false return and exceptions through a local scope guard. Code500/CCSDS conversion temporaries now use fixed native six-element stack arrays, removing conversion-exception leaks. Native single-spacecraft-per-SPK, range/type/domain and scientific algorithm policies remain. Compilation only; complete function/solver/ephemeris scopes remain open. No row or verification credit.

Owning-source findings and implementation
------------------------------------------

EphemerisPropagator UpdateSpaceObject(Real) previously called ReturnFromOrigin before resolving the requested epoch and invoking UpdateState, then copied the old j2kState into the manager. The precision branch already evaluated before converting. Both branches now resolve their clock, synchronize its companion, invoke the native provider, convert using that same resolved epoch and finally publish the manager vector/epoch. This retains existing object mapping and provider policies while removing stale Real-time state publication.

RevertSpaceObject previously rebuilt the captured precision epoch from initialEpochGT plus a Real elapsed-time difference, and did not synchronize its Real companion; the Real branch similarly reconstructed its clock and did not synchronize its precision companion. Reversion now selects the buffered epoch directly, derives elapsed time and its companion, then uses the existing native update/origin-conversion path. SetTime now maintains currentEpoch/currentEpochGT alongside timeFromEpoch, so the native provider can see a changed epoch and perform its existing last-epoch synchronization.

EphemerisPropagator Step(Real dt) restored ephemStep only after its virtual Step returned. A provider/coordinate conversion exception left the temporary duration selected for subsequent steps. A local non-owning scope guard now restores the prior selection during all exits; existing return/error signaling and successful HasEphemPropagated behavior are preserved. This does not roll back a failed provider's entire state/session.

StkEPropagator UpdateState queried ephem at currentEpoch/currentEpochGT but used lastEpoch for coordinate conversion, including a Real conversion in precision mode. It now selects the matching CoordinateConverter overload at the same epoch used for interpolation. Code500 and CCSDS UpdateState allocated six-Real temporary arrays and deleted them only after conversion; those fixed-size temporaries now use stack storage with identical values and converter arguments.

The local propagator guide explicitly restricts one spacecraft per SPK propagator and excludes STM/orbit-Jacobian propagation. That policy is retained; the source TODOs for broader SPK grouping do not silently change the campaign contract. Native stop refinement calls RevertSpaceObject before TakeFinalStep, and the existing manager BufferState supplies its captured epoch. No runtime stop/epoch/coordinate/numerical checks were run.

Limits
------

This repairs native clock/state publication order and selected exception lifetimes. It does not prove scientific equivalence, implement broader SPK spacecraft/STM support, replace native buffering/object-mapping policy or make arbitrary provider/file/SPICE/session failures atomic. General changed owned component/storage graphs, borrowed lifetimes, original function I/O/scoping/output behavior, complete solver/estimator/MATLAB lifecycle and original ephemeris format/B-1/final-iterative requirements remain open. No class layout, native parameter IDs, virtual/public API contracts, interpolation equations, domains, assertions or tolerances change. Stop refinement, precise epoch round trips, rotating-frame calculations, coordinate exceptions, subsequent stepping after failure and all broader runtime behavior remain unverified.

Compilation and retained evidence
---------------------------------

Build v1 returned 0 in 1.4s, progress 7/7, 0 existing compiler deprecation warnings. All compiler sessions are terminal. Final hashes match build v1. 5 source/help preimages, 15 unchanged reference files and four planning snapshots preceded edits. Indexed/combined patches, hashes, source findings and compiler receipts are retained. New text passes whitespace checks (0 inherited whitespace lines recognized by the diff). Participating help XML files parse.

No new tests, native/model/helper/parser/GUI/MATLAB/mission/numerical/runtime execution, acceptance auditors/readers/seals or evidence admission occurred. Original requirements/text/fingerprints/fixtures/assertions/tolerances/denominator/gates/dispositions remain unchanged. No row or verification credit. Tracking-v134 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next implementation
-------------------

Continue original function I/O/property/output staging, coherent changed owned resource/component/storage preparation, shared globals/dependency lifetimes and all-command target/optimizer/estimator/MATLAB lifecycle. Finish remaining native ephemeris provider/buffering/revert/sampling and original format/B-1/final-iterative/scientific clauses without expanding documented provider domains speculatively. Continue every remaining any-Resource serialization/default/reference/portability, body/frame/kernel, resource-tree, POV and specification/external/historical worklist item before runtime verification. Deferred checks include Real/precision state refresh at explicit/sentinel epochs, exact buffered epoch restoration, SetTime then native step, moving/rotating origins, STK epoch-dependent frame conversion, explicit-duration failure followed by ordinary stepping, Code500/CCSDS conversion exceptions, stopping-condition final steps and subsequent solver/function propagation.
