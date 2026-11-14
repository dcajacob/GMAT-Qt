POV cancellation and background model ownership
==============================================

The Qt POV reader now accepts a caller cancellation predicate and observes current-thread interruption. File reads use checked 64 KiB chunks; token scanners, raw inactive/macro scans, source-loop replay, value/array copies, primitive/mesh expansion and detached geometry building poll cooperatively. Cancellation returns no partial model. The spacecraft preview and orbit scene now use owned background POV jobs with queued GUI completion, independent cancellation state, detached CPU geometry and captured include paths. Changing a selection, deleting its owner or rejecting preview validation cancels the corresponding job; per-request identity and cancellation checks reject stale results. Validation keeps pending OK intent until completion, and field changes revoke it. Workers never access widgets or the live scene graph; application shutdown requests cancellation and joins outstanding threads. Plot export refuses a pending-model frame before opening its output file. Nine source/help paths (one new internal job header) compile; visualization help XML parses. Language/geometry/material/transform/shader formulas and legacy loader remain unchanged. OS/library operations have cooperative cancellation latency; broader language/array/dictionary/function/geometry/texture/exporter and full campaign scope remain open. No runtime/thread/GUI/model/rendering verification or row credit.

Reader cancellation
-------------------

The existing reader signatures remain available. A new overload accepts a thread-safe predicate evaluated on the importing thread, and all overloads observe QThread interruption. Each parse owns its control object; there is no global cancellation flag or GUI event pump. File input checks between 64 KiB reads and before/after the existing whole-source UTF-8 decode. Raw tokens and character-distance checks cover long comments, strings, identifiers and numbers, including inactive controls and captured macro bodies. Value copies check initialized entries and sparse holes; geometry generation and scene building check their outer iterations. Cancellation unwinds unpublished work and returns an empty model with a cancellation diagnostic. No iteration or elapsed-time cutoff changes source semantics.

Detached model jobs
-------------------

The new internal PovModelLoad handle owns independent cancellation state. Its worker captures file/include settings, builds new CPU geometry, prepares arrays and bounds, and applies the existing normalization formula. It does not access widgets, mutate registry paths or modify an installed scene. Completion is queued to a live caller context; canceling a handle suppresses completion. The GUI joins a completed model to the scene on its next normal synchronization. Curve request identity prevents an earlier path/pose request from publishing over a newer one, and removed curves cancel their jobs. Owner destruction also cancels pending work. Workers self-delete through finished/deleteLater; application shutdown requests cancellation and waits for outstanding workers. No thread termination is used.

The spacecraft model dialog validates POV files asynchronously. A file edit cancels earlier validation immediately; other field changes revoke pending OK intent. A pending OK request completes only after successful validation. Reject cancels validation and closes the dialog. Legacy direct readability calls remain synchronous; they retain default reader interruption, while direct clients can use the new predicate overload. Other model formats and their existing reader route are unchanged.

Image export
------------

A framebuffer capture returns no image while a selected POV request remains pending. PNG export checks this result before opening its QSaveFile and asks the user to wait. It cannot silently commit the temporary marker or default body while that model is loading. Export-path collision checks and other plot behavior compare unchanged with preimages.

Limits and retained evidence
----------------------------

Cancellation is cooperative. OS file operations, allocation/deallocation, the whole Unicode conversion and individual Qt/OSG operations may finish before it is observed. A hard maximum cancellation latency is not claimed. Source-loop behavior has no arbitrary budget. Thread/GUI/shutdown races, responsiveness, first-frame/export behavior and rendering remain unqualified pending the later verification phase. Qt lifecycle semantics and installed OSG visitor interfaces were inspected; primary references remain in research-note.json. No newer QStringDecoder finalize API was introduced.

Configured build v1 returned 0 in 18.88 seconds; last printed Ninja progress 11/12. Session 10114 is terminal. One existing QImage::mirrored deprecation warning from OrbitRenderer.cpp remains; no compiler errors.

Eight existing source/help preimages, one new-header absence, two unchanged references and four planning snapshots preceded the corresponding edits. Indexed patches, before/build/final hashes, source/research notes, build log and terminal receipt remain in the stage. Final sources match the successful compiler capture, changed-line whitespace is clean and visualization help XML parses. Numeric/string/logical and material/transform sections, conditional/switch/break formulas, material/GLSL helpers, orbit transform and scene synchronization compare byte-identical where recorded. Mission dynamics, original assertions and original acceptance dispositions are unchanged. Public OrbitRenderer layout/signatures and the legacy loader compare unchanged. The internal dialog and frontend must be rebuilt together, as in the retained configured build.

No runtime/GUI/MATLAB/helper/model/mission/numerical tests, new tests, cancellation/worker/thread evaluation, shader compilation, rendering, acceptance auditors/readers/seals or evidence admission occurred. Original requirements, fingerprints, source text, fixtures, assertions, tolerances, denominator, gates and dispositions remain unchanged. FRR!E44/E46 remain partial. Tracking-v116 retains 1093/1158 (94.39%), 65 unresolved engineering rows; source-versioned verified baseline 1053/1158 (90.9%), all 105 original acceptance rows open. The complete implementation audit and goal remain incomplete.

Next implementation
-------------------

Continue remaining POV directive-generated array headers/initializers, native function/string/color/array/dictionary variants, richer geometry/CSG and advanced textures, interleaved macro/directive filename lifecycles and representative exporter research. Preserve nonterminating-loop semantics with cooperative cancellation rather than iteration cutoffs. Legacy direct readability calls remain synchronous; their default reader observes thread interruption, while the new reader overload accepts caller cancellation. Continue the complete original function/solver/resource/persistence/body/frame/kernel/ephemeris worklist and explicit specification/external/historical dependencies. Preserve original clauses and defer runtime verification until the implementation pass is complete.
