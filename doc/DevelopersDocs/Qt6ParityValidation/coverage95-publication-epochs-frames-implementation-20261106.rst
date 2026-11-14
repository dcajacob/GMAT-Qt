Per-spacecraft publication epochs and frames
============================================

A native packet has one legacy All.epoch and broadcast data coordinate system, while independent propagator groups can hold different times and retained spacecraft can refer to distinct internal frames. Identity-correct indices alone did not resolve that difference. This batch carries actual timing/frame metadata through ephemeris and plot output.

Native metadata and scoped views
--------------------------------

Each CartesianColumns record can now carry a GmatTime epoch and the actual spacecraft internal CoordinateSystem with a weak lifetime identity. PropagationEnabledCommand takes the epoch from that record's native PSM state, preserving its precision mode, and the frame from the actual spacecraft rather than a same-name configured object. Publication validates finite epochs and live advertised frames; readback checks frame identity again before dereference. Legacy records without timing/frame metadata keep their prior fallback paths.

A compact column scope preserves the original publication's data/count/columns in addition to its current view. This permits an origin snapshot to verify its original packet while a plot converts a filtered view. Each new Publish dispatch still starts from its own consumed request, so a nested legacy publication cannot inherit the outer root context. Scope exit restores both current and root metadata. Provider-keyed context remains attached to the current publisher.

PlotOriginScope packet snapshots now include ordinary Cartesian columns when accepted integration records are absent. Ordinary states are stored by value, retaining valid packet-copy behavior rather than pointers into temporary Cartesian vectors. Matching scope queries recognize the original columns through a compact view; accepted-step matching continues using the integration packet. Native spacecraft clocks can provide a fallback only at the exact requested epoch. Source/view frame traversal is shared and identity-visited, including nested coordinate-system references and directly exposed spacecraft origins/primary/secondary/reference objects. It does not claim arbitrary composite-point dependency discovery.

Ephemeris input frame and time
-------------------------------

EphemerisFile retrieves the matching spacecraft's epoch instead of always using dat[0]. SPK published-epoch metadata prefers that actual object epoch, with its existing detached sampled-SPK override retained. Propagator-change events also use the matching group's epoch.

Before an incoming sample enters a writer's interpolation buffer, RebaseOrbitData normalizes its position, velocity, covariance and optional acceleration from the advertised source frame into the writer's retained input frame. Previous buffered samples therefore retain a consistent frame. Identical frame pointers take the existing no-conversion path. The existing writer conversion routine was factored into a common between-frame helper; its rotation/covariance and acceleration limitations, Real-time conversion policy and output-frame behavior remain in place. No new force, acceleration, orbit or covariance formula was introduced. Attitude quaternion reference semantics are unchanged.

This batch does not retarget an already buffered writer to a transient source frame. It does not invent interpolation for unavailable reference spacecraft at arbitrary requested output times or claim support for unusual internal frames beyond the native engine contract. Such broader composite-reference/output-frame behavior remains part of the scientific/source-scope review.

Timestamped plots and curve continuity
---------------------------------------

Publisher groups advertised spacecraft by their actual GmatTime epochs, filtering on the plot's actual bound objects. OrbitPlot uses these groups for typed ordinary data, including one-curve views whose coordinate frame can depend on a different spacecraft. Collection frequency still counts the original publication once; pose/availability metadata still observes skipped collection packets. Each selected timestamp gets a scoped packet view, actual source-frame conversion and exact-epoch reference snapshots. Missing coherent references omit the selected sample and mark the gap instead of substituting another group's live state. Other groups' curves are deferred. Deferred slots retain native previous-data flags rather than being reset as absent.

Orbit arc-pose and Cartesian buffering use the advertised frame and the precise converter overload when native object epochs exist. Accepted integration samples inherit the original frame metadata while retaining their endpoint epochs. View and relevant input frames contribute their tracked spacecraft dependencies to the snapshot packet.

GroundTrack applies the same actual-epoch grouping after its original collection/countdown decision. Availability masks distinguish selected, deferred and truly absent spacecraft, preserving per-curve continuity when groups have different times. Ordinary and accepted-step conversion use matching origin snapshots and the advertised input frame, with native precise-time conversion where available. Its body-fixed target frame and geodetic calculation are unchanged. Rendering callbacks still use the native Real MJD timestamp representation; precision is not manufactured beyond that interface.

Compilation and boundaries
---------------------------

Configured compilation v1: 241/241 steps, 186.19 seconds, return 0.
Configured compilation v2: 70/70 steps, 5.69 seconds, return 0.

Ten paths have preimages taken before their first edits/v1, before/build/final hashes, indexed deltas, source map and compiler terminal receipts. Final sources match the final successful compilation snapshot; all compiler tool sessions are terminal and added-line whitespace is clean. Matching configured core, Qt/console, plugins, MATLAB and Python consumers rebuilt as needed. No changed C++ translation unit is outside the configured source set.

Publication records, compact scope and origin-snapshot packet layouts changed. Added methods are nonvirtual; no virtual slots or fields in Subscriber/EphemerisFile/OrbitPlot/GroundTrack/EphemerisWriter were added by this batch. External prebuilt binary compatibility with the changed metadata/scope layouts remains unqualified.

No runtime/GUI tests, missions, numerical matrices, acceptance audits/readers, evidence admission, assertion/fixture/tolerance/denominator changes, packages, installed/shared runtime changes, commits, pushes or external messages were performed. Compilation establishes source integration only. Complete original function/resource/command, public edit/lifetime, scientific/format, specification, external and historical-platform scopes remain open. No all/any parent earns partial row credit.

Tracking-v68 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
