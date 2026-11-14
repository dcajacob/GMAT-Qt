Native subscriber binding frames
================================

Nested execution can temporarily replace native subscriber references as well as ElementWrapper arrays. The prior common frame retained the enclosing wrappers/internal frame/solar system, and ReportFile/XYPlot retained their Parameter slots. Other native subscribers could retain callee references or lose enclosing bindings. This change extends actual-identity binding restoration; it does not enable subscriber retained-value providers or complete the broader function I/O requirement.

Native reference state
----------------------

SubscriberBindingState stages typed borrowed pointer members, pointer vectors and pointer-keyed maps before common wrapper detachment. Each captured object has its actual weak lifetime identity. Restoration clears expired slots and removes expired map keys. Stable container members are captured, rather than element addresses that nested initialization can invalidate. Lexical role guards detect changed configurations; incompatible bindings clear and readiness becomes false, with restoration failure reported. The helper restores once, publishes all remaining actions after an individual failure, owns no referenced engine object and retires no writers, numerical history or renderers. Captured metadata has nonallocating default/swap requirements.

OrbitPlot retains its view frame and both native space-point lists. OrbitView layers its view-up frame, origins and camera points over OrbitPlot; GroundTrackPlot layers its central body. Legacy OpenGlPlot covers both frames, origins, camera points and object lists. GroundTrack covers its solar/body bindings, spacecraft/body/point/station lists, spacecraft map and matching publication column indices. EphemerisFile covers spacecraft, output frame and acceleration-model slots. MatlabWorkspace covers Parameters and its Interface identity; DataCallback covers Parameters. Opaque callback userdata and native engine-session ownership remain outside this helper.

DynamicDataDisplay restores Parameter references by stable row/column accessors. The complete lexical grid is compared before restoration. If its layout or parameter/owner roles changed, all current cell references clear, including cells created during the call; current values, colors and resource declarations persist for ordinary rebuilding. Removed cell locations are never dereferenced. Existing common binding states and ReportFile/XYPlot frames remain layered. Native references restore before common wrappers can retire; whole-value commit dismissal remains active through every layer.

Common frames now also retain data coordinates, the data MJ2000 origin, current provider identity and the current publication labels, provider ID and propagation direction. GroundTrack restores column indices with its spacecraft index map. These are current input bindings; accumulated samples and physical output owners are not rolled back. Late renderer/writer interiors, sampling-history object references, owned frames and force-table lifetimes remain separate unfinished work.

Wrapper graph copies
---------------------

Subscriber copy construction/assignment now stages one independent clone per actual X/Y wrapper, preserving aliases across axes and repeated entries, including null slots. ElementWrapper CloneValue validates native type/owned wrapper structure. A clone returning a source, target or already staged peer is rejected without taking that pointer's ownership. Both axes finish staging before old wrappers retire; publication uses swaps and transfers clone ownership after staging. Failed staging releases its private clones and leaves the old wrapper arrays available. Other native fields retain their existing assignment guarantees; this is not a whole-resource transactional Copy contract. Optional builds that disable native wrapper cloning retain their existing borrowed-wrapper path.

Compilation
-----------

v1: 295/295 steps, 254.3 seconds, return 0.
v2: 73/73 steps, 8.04 seconds, return 0.

Both configured builds passed and their tool sessions closed. Core, Qt, console, enabled plugins, MATLAB and Python 3.14 consumers rebuilt. The optional DataCallback translation unit also passed a standalone syntax compilation using current core compiler flags and its own include directory. Legacy OpenGlPlot is outside the configured source set: standalone compilation reports 39 obsolete-interface errors, and compiling the captured preimage reports the exact same diagnostic list. Its compilation remains unqualified. Logs and terminal receipts preserve both outcomes; no new legacy error was observed by this comparison.

No existing object fields or virtual slots were added. Nine types override the existing SuspendFunctionBindings route; the new helper is privately staged per invocation. All 21 final source hashes match the successful final configured build and standalone compiler snapshot. Preimages, indexed deltas, final hashes, native reference catalog and four planning preimages are retained under the batch stage. Added-line whitespace is clean.

Remaining implementation
------------------------

Complete subscriber cold/new/local value and reference providers across every native subtype, including native writer/renderer interiors, retained history/sampling references, owned frames, force-table/container lifetimes, callback userdata and independent wrapper-owned object graphs. Complete direct API/native Copy/Initialize and property-edit commit/rebinding, self-mutating callbacks, component-changing output/cache repair, shared Parameter owner/RefData and global-resource interiors. Finish legacy/shared/external-map ownership, direct/finalization/custom-manager reentry, all command/plugin and interpreted-function providers and arbitrary scope/interior/service/engine-reset/later lifetimes. Continue all remaining original body/kernel/NAIF/scientific/resource requirements and independent POV/rotation research. Preserve complete scope and verification deferral.

Owned writer/renderer interiors, output destinations, native histories, source-to-clone owned-wrapper object registration, private Qt settings, direct public API/copy/property changes and already-retired or arbitrary interior references are not qualified by these frames. Explicit declaration edits that invalidate an enclosing binding need a complete retained-reference rebuild; the current frame reports the mismatch. Full broad function/resource/scientific clauses remain in scope.

No runtime tests, GUI, missions, function/model/solver execution, numerical comparisons, acceptance readers or seals ran. Compilation may relink existing test executables; none were executed. No packages, installed/shared runtime, commits or pushes. Assertions, fixtures, tolerances, original texts/fingerprints, denominator, ledger and gates remain unchanged.

Tracking-v63 remains 1093/1158 (94.39%), with 65 other rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
