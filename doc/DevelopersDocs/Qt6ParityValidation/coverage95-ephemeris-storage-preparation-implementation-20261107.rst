Ephemeris state and interpolation storage preparation
=====================================================

Ephemeris propagator preparation now uses the propagation state manager's complete packed extent directly, rather than multiplying that already-complete vector by the propagated-object count. Without a state manager, the native six-Cartesian-values-per-object fallback checks index capacity before multiplication. Required state/J2000 buffer allocations are staged together before retiring either old array; a compatible existing J2000 buffer keeps its address. Empty/unprepared groups report controlled preparation errors and the propagator stays unready on allocation failure. Native ephemeris query interpolation constructs its replacement Hermite/Lagrange cache before retiring the old cache and publishes type/order together. Compilation only; complete function/solver/ephemeris scopes remain open. No row or verification credit.

Owning-source findings and implementation
------------------------------------------

PropagationStateManager::BuildState obtains one complete stateSize from SortVector and calls state.SetSize(stateSize). EphemerisPropagator Initialize and its dimension-zero GetDimension path previously treated GetState()->GetSize() as a per-spacecraft extent and multiplied it by propObjects.size(). The common local extent helper now returns the already-packed manager size. Its no-manager fallback retains six values per registered object, with a native Integer capacity check before multiplication.

EphemerisPropagator Initialize previously deleted state before allocating its replacement, then potentially deleted the J2000 buffer before its own replacement allocation. A failed first allocation could leave a dangling field; a failed second allocation could partially publish the new layout. Initialize now marks preparation pending, computes/validates the prospective extent, stages both required arrays through unique_ptr, then publishes pointers and dimension without another allocation. J2000 storage with the same native extent remains in place. Empty groups and a manager with an unprepared zero state receive controlled errors before indexing propObjects or replacing buffers. Later native Propagator/file/provider preparation remains a separate phase.

Ephemeris::InterpolatePoint previously deleted its interpolation cache before constructing a new native Hermite/Lagrange object for a changed order or mode. The replacement is now constructed in a temporary owner before pointer/currentOrder publication; a construction/allocation exception retains the previous cache and order. Existing segment selection, precise time anchor, order degradation, stencil centering, native Clear/AddPoint/AddDerivative sequence and scientific routines are unchanged.

The production Hermite owner is the utility Ephemeris query reader. Each non-node query clears the native cache, fills the selected point stencil and only then adds matching derivatives before interpolation. That source path rebuilds scratch rather than restoring a public borrowed Hermite checkpoint. The fixed-layout typed restoration provider was not weakened or replaced with a speculative public-buffer API. Ephemeris propagator native buffering/revert still uses its existing GmatState/epoch path, and broader changed component/state/session contracts remain open.

Limits
------

This stages private allocation/publication and corrects packed extent accounting. It is not rollback of later ephemeris provider/file/SPICE initialization, native search state or active solver/estimation/subscriber/MATLAB sessions. General changed owned resource topology, public/borrowed storage preparation, Hermite typed checkpoint extent changes, external borrower lifetime, non-numerical propagator scientific sampling/revert and original ephemeris format/final-iterative obligations remain open. State replaces its buffer according to the existing native initialization policy; compatible J2000 storage retains its address. Native file/epoch/order/type/domain policies and numerical interpolation methods remain. No parameter IDs, class fields, virtual contracts or public native APIs change. Runtime, allocation-failure, multi-spacecraft, precision and numerical behavior remain unverified.

Compilation and retained evidence
---------------------------------

Build v1 returned 0 in 2.56s, progress 69/69, 0 existing compiler deprecation warnings. All compiler sessions are terminal. Final hashes match build v1. 3 source/help preimages, 15 unchanged reference files and four planning snapshots preceded edits. Indexed/combined patches, hashes, source findings and compiler receipts are retained. New text passes whitespace checks (0 inherited whitespace lines recognized by the diff). Participating help XML files parse.

No new tests, native/model/helper/parser/GUI/MATLAB/mission/numerical/runtime execution, acceptance auditors/readers/seals or evidence admission occurred. Original requirements/text/fingerprints/fixtures/assertions/tolerances/denominator/gates/dispositions remain unchanged. No row or verification credit. Tracking-v133 retains 1093/1158 (94.39%), 65 unresolved engineering rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next implementation
-------------------

Continue coherent changed owned resource/component/storage preparation and restoration, original function I/O/property/output staging, shared globals/dependency lifetimes and all-command target/optimizer/estimator/MATLAB lifecycle. Finish native non-numerical ephemeris provider/buffering/revert/sampling behavior and all original ephemeris format/B-1/final-iterative/scientific requirements. Continue every remaining any-Resource serialization/default/reference/portability, body/frame/kernel, resource-tree, POV and specification/external/historical worklist item before runtime verification. Deferred checks include multiple spacecraft with a complete packed PSM state, no-manager capacity and empty/unprepared groups, compatible/incompatible J2000 storage reuse, failed first/second buffer allocation, failed Hermite/Lagrange construction, changed segment order/mode, high-precision interpolation, native provider initialization failures and subsequent propagation/solver behavior.
