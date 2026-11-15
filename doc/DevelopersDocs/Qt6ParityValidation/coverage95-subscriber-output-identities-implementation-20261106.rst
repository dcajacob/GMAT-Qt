Subscriber output identities
============================

A cold clone or private function plot can have the same script name as a caller plot. Native plot initialization, old-name curve removal and Finalize previously sent that shared name to the receiver, allowing operations on the wrong actual window. This change gives private outputs independent backend identities while leaving script and display names intact. It is a prerequisite for subscriber function I/O; complete subscriber value providers are not enabled by this change.

Output ownership
----------------

Subscriber constructors allocate process-unique output identities. Definition copies keep their runtime output map/start/created fields cold and both common initialization flags false. Native assignment retains the target identity and output map. Before first run initialization, ObjectInitializer uses logical keys for main-scope subscribers and private keys for function subscribers. An already started output keeps its identity. InitializeForRun calls the actual virtual Initialize and records its returned readiness in Subscriber and GmatBase; exceptions leave readiness false. Historical inactive-subscriber false-return handling is not redefined. Existing actual initialized global subscribers continue through the v61 preservation path.

OrbitPlot, OrbitView, GroundTrackPlot, OpenGlPlot, GroundTrack, OwnedPlot, XyPlot and DynamicDataDisplay send their backend identity in native plot operations, including old-name keys. Rename metadata stays with the actual destination; cold XY copies have no existing physical window. New global XY resources create their own output; an initialized, genuinely shared global XY window retains its reuse path. A cold main object does not inherit a peer's old logical output key. Function subscriber finalization requests a private identity for an output that never started, so cold copies cannot close a caller window. This does not supply a complete file/writer, reference, data-history or direct API ownership protocol.

PlotInterface holds a synchronized string-only display-name registry. Subscriber destruction removes its cached labels. The registry owns no model, wrapper or engine objects. The unique counter detects overflow; identities are not script configuration or persistent resource names.

Qt receiver
-----------

Qt entries and native updates/removal use exact backend keys. Native paths never fall back to a display alias: creating a main plot must not remove a surviving function plot that has the same caption. Public show/model/clock access accepts an exact key or a uniquely resolvable logical name; a logical match shared by several private outputs is ambiguous. An exact main key continues to select its main output. The output tree stores backend keys and displays logical labels, so separately owned windows remain separately selectable. Window captions, plotName properties and default model titles remain logical; plotBackendName carries the separate actual key.

Widget callbacks retain actual keys, and end-of-run clock handling uses entry keys rather than model titles. Private keys do not enter saved main view persistence or script projection writes. Native camera definitions remain available through existing native setup; full copied private Qt comment settings for cameras, vectors, sensors and playback are not yet implemented. Logical-name-only external consumers cannot distinguish duplicate captions without using the actual keys.

Remaining implementation
------------------------

Complete cold/new/local subscriber value and reference providers across the full native catalog, including independent wrapper/native reference graphs, writer destinations, retained data and renderer rebinding. Finish direct Initialize/API/Copy ownership, partial-initialization cleanup, private Qt camera/vector/sensor/playback settings and legacy renderer labels. Complete shared Parameter owner/RefData and global-resource interiors, component-changing output/cache repair, self-mutating callbacks, legacy/shared/external-map ownership, direct/finalization/custom-manager reentry, all command/plugin and interpreted function providers, and arbitrary scope/interior/service/engine-reset/later lifetimes. Continue every original remaining body/kernel/NAIF/scientific/resource requirement and independent POV/rotation research. Preserve the complete worklist and verification deferral.

Subscriber value-provider admission remains unchanged. No complete function I/O, core/scoping, any-command or resource-tree parent is credited. Native direct Initialize callers, direct Copy/readiness behavior, partial initialization, live resource replacement, writer filenames, native history copying and arbitrary lifetime/concurrency/reset callbacks remain unfinished. Legacy OpenGlPlot labels and platform compilation remain unqualified. This batch isolates output operations; it does not establish scientific or GUI acceptance.

Compilation
-----------

v1: 299/299 steps, 212.48 seconds, return 0.
v2: 5/5 steps, 15.67 seconds, return 0.

The configured core/Qt/console/enabled-plugin/Python 3.14/MATLAB consumer build passed. GroundTrack and the other configured native plot units compiled; legacy OpenGlPlot.cpp is outside this configuration and has only a source change. Four Subscriber runtime fields and a Qt entry label were added; nonvirtual helpers leave virtual slots unchanged. Configured consumers rebuilt. Both tool build sessions closed before finalization. Preimages, before/build/final hashes, indexed deltas, source map, logs, terminal receipts and four planning preimages are retained in ``build/nasa-coverage/coverage95-20261106-implementation-subscriber-output-identities``. Added-line whitespace is clean.

No runtime tests, GUI, missions, function/model/solver execution, numerical comparisons, acceptance readers or seals ran. No packages, installed/shared runtime, commits or pushes. Original assertions, fixtures, tolerances, requirement texts/fingerprints, denominator, ledger and gates remain unchanged.

Tracking-v62 remains 1093/1158 (94.39%) implementation present and 65 other rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. The full goal remains active and incomplete.
