Subscriber catalog providers and publication identity
=====================================================

Eight native classes gain exact-type retained value-reference providers: OrbitPlot, OrbitView, GroundTrackPlot, GroundTrack, OwnedPlot, TextEphemFile, MatlabWorkspace and DataCallback. Unknown derived classes do not inherit admission. These extend the four providers in tracking-v64; EphemerisFile keeps its specialized resource binder. The configured factory maps OrbitView/OpenGLPlot/Enhanced3DView aliases to OrbitView and GroundTrack/GroundTrackPlot aliases to GroundTrack. The legacy raw OpenGlPlot implementation is not admitted or brought into the configured build.

Completed native values
-----------------------

Clone, typed Copy, copy constructors and assignments use the existing native-copy guard and copy-retirement tokens. Base construction/assignment defers graph completion until most-derived fields exist. MatlabWorkspace and TextEphemFile now have explicit typed Copy overrides. GroundTrack's constructor-time assignment runs under the native guard and completes once. Assignment retains the target's physical output identity; independent clones use independent backend identities.

Geometry providers map the actual view/up frames, origins, camera objects, central body, solar system, plotted SpacePoints, satellite/station/body/point lists and spacecraft-keyed state map. Role names remain declarations and curve labels. GroundTrack's owned runtime frames and buffers keep native cold-instance ownership and are reconstructed by Initialize. TextEphemFile inherits ReportFile reference mapping. MATLAB and callback Parameter arrays reconnect by actual identity; MATLAB's dormant interface pointer clears without starting an engine. Callback function/userdata are borrowed host configuration with an explicit lifetime comment; copying a void pointer does not transfer ownership from its host.

Subscriber now asks a protected virtual native-binding readiness predicate before ObjectInitializer/GmatFunction reuses retained bindings. Geometry prototypes with no ElementWrappers no longer appear ready solely because their empty wrapper arrays match. The four geometry classes require their native frame/object lists before skipping ordinary reference lookup.

Owned native state repairs
--------------------------

OwnedPlot now copies all eight curve-style vectors (color, line width/style, marker/size, line/marker/high-low flags), preserves the source ShowPlot flag, starts a copied physical-window flag cold and preserves assignment's target-window ownership. Its own parameter IDs/counts/arrays begin at SubscriberParamCount, avoiding overlap with inherited Subscriber fields. SetData/SetCurveData respect ShowPlot and lazily initialize retained outputs through the common native activation route.

TextEphemFile stages an independent native-type-checked interpolator Clone in a unique owner. Construction releases that owner only after graph completion succeeds. Assignment stages before mutation and deletes the displaced interpolator before publishing its replacement, repairing the previous leak. Its explicit typed Copy prevents inherited ReportFile Copy behavior from rejecting or slicing this native subtype. Native cold numeric/interpolation buffer policy and filename conventions are preserved.

Identity-bound publication
--------------------------

A function renames copied spacecraft to its formal argument. The plot can hold that correct object while its logical curve role still uses the caller's spelling. Previously OrbitPlot searched only the old role's .X/.Y/... labels, losing valid data; name matching also lets distinct scoped objects overwrite each other's state columns.

Publisher now carries optional synchronous CartesianColumns metadata, keyed by actual spacecraft identity and six packet indices. Metadata is frozen for dispatch, validates objects and disjoint in-range indices and restores outer contexts during nested publications. Legacy publications clear inherited metadata even if they reuse the same data pointer. PublishPropagatePacket derives this metadata from the native PSM layout for every ordinary packet, whether or not accepted-step consumers exist. Formation member selection uses the actual native member ordinal and validates its declaration role; it does not search for a current instance name. Accepted-step states reuse that same identity layout.

PropagationEnabledCommand fills the registered state block in native PSM order, with shape/extent/registration checks, rather than writing each value into the first matching label. The numerical state source remains the native PSM state, and covariance/acceleration/attitude code is unchanged. This does not change integration or force calculations.

OrbitPlot data buffering, availability and arc-pose lookup select published columns by bound object identity and retain logical curve/color role labels. The old scope-flag filter remains for legacy name-only packets; identity packets reject a same-name callee object through actual identity matching. Accepted-step packets carry their own compact column scope while leaving original integration/origin metadata active for coherent frame conversion. Arc-start/decimation roles translate back to logical curve roles. GroundTrack consumes the same actual-object columns for ordinary and compact samples. A metadata-present packet with no matching identity is authoritative absence; legacy providers keep their prior label path, using the bound object's current name where available.

Original requirement scope
--------------------------

The passive original-scope-source-extract.json preserves the original function I/O and scoping text/fingerprints. FRR E452/E460 concern entire objects; E466 preserves non-global inputs unless returned; E471 allows subscribers inside functions; E492/E493 allow local resources and main-sequence resource types in functions. The shipped Resource_GmatFunction guide documents Global spacecraft and Global subscribers for continuous report/plot data across main and function execution. Native cold Clone/Initialize output policy is distinct from this shared Global history path. The original text does not define deep copying external GUI/file history or ownership of external host callback data; these are not added as new mandatory clauses.

These changes advance the broad tracked parent requirements. They do not by themselves complete every resource/callable/command, local/global combination, public edit or exceptional/reentrant path, and no all/any parent earns partial row credit. Color/segment callbacks still identify subjects by name and need a separate identity path; copied renderer details, declaration edits, component replacement and shared-global dependency lifetime remain work. Other original resource/scientific/specification/external/historical rows remain unchanged.

Compilation and boundaries
---------------------------

Configured build v1: 295/295 steps, 230.97 seconds, return 0.
Configured build v2: 235/235 steps, 234.46 seconds, return 0.
Optional DataCallback translation-unit syntax compilation: return 0, 1.01 seconds.

Twenty-two source paths have preimages, before/build/final hashes, indexed patches, source map and terminal receipts. Eighteen preimages precede v1; four publication/command files were added after v1 and before their first edits/v2. Final sources match the final successful configured and optional compilation snapshots. All compiler tool sessions closed before finalization. Added-line whitespace is clean.

One protected Subscriber virtual slot was added, with no new instance fields in existing production classes. Publisher adds a nested metadata/scope API and an optional argument to a nonvirtual function, changing that exported function's mangled ABI. Matching configured consumers were rebuilt; external prebuilt binary compatibility is unqualified. The optional callback compilation is a syntax check, not a linked/runtime plugin qualification. Legacy raw OpenGlPlot keeps the previously recorded unconfigured compilation issues.

No runtime tests, GUI, missions, numerical matrices, acceptance audits/readers, evidence admission, assertion/fixture/tolerance/denominator changes, packages, installed/shared runtime changes, commits, pushes or external messages were performed. Recorded source-versioned verification remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Tracking-v65 remains 1093/1158 (94.39%), with 65 other rows. Full goal remains active and incomplete.
