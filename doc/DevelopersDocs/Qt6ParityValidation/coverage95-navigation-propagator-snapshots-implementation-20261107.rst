Navigation propagator snapshot resets
=====================================

Explicit Save now resets the complete Estimator/Simulator Propagator selection before writing its captured default and spacecraft-specific mappings, including empty selections. The reader recognizes only a raw empty braced list before generic Parameter/name lookup and forwards it to each native owner. Native reset clears names, mappings and selection scratch, retires owned PropSetup definitions through NavigationPropagationCopies::Publish, reconnects the measurement manager to the same collection containers and marks the solver for native preparation. Estimator also clears its force-derivative cache and defers initialization. Serialization never clears the live source. Ordinary nonempty assignments, native propagator mapping output, equations, parameter IDs, class layouts and numerical domains are unchanged. Compilation only; full any-Resource persistence/navigation/function/command scope remains open and no row or verification credit is added.

Source findings and implementation
----------------------------------

Estimator and Simulator have a specialized Propagator writer: it records a default name and then per-propagator spacecraft lists. Their indexed setters append names and spacecraft mappings, while the scalar setter replaces only the first name. Consequently later snapshots can retain earlier mappings. The new explicit-script writer emits Propagator = {} before the existing output. Outside ObjectScriptExportScope, the old writer path remains.

Interpreter dispatch checks the original raw token before Parameter lookup and selects the new native clear only for Estimator/Simulator Propagator. Each native scalar setter accepts the explicit reset, clears its name/mapping containers and scratch selection, marks native initialization pending and retires its definition clones through the existing native ownership helper. The measurement manager continues to refer to the same collection containers. Estimator force-derivative cache and delayed-initialization flags are updated at this boundary. Live source serialization only reads and writes text.

Limits
------

This packet restores declared propagation selection, not an active estimator/simulator integration, posterior, output stream or measurement history. The native solver is marked uninitialized and later Run commands must prepare its new definitions. Propagator-dependent measurement models own their own calculation clones and keep their native reinitialization lifecycle; no active-session resume is added. Native identity/domain checks for nonempty assignments and overlapping spacecraft mappings remain. Other navigation list/default/provider and any-Resource persistence scopes remain open. No runtime reload or navigation evidence, parameter IDs, class instance layouts, virtual contracts, numerical equations or tolerances change.

Compilation and retained evidence
---------------------------------

Build v1 returned 0 in 16.07s, progress 68/68, 0 existing compiler deprecation warnings; no compiler errors. Compiler session 79938 is terminal. Final hashes match build v1. Four source/help preimages, nine unchanged references and four planning snapshots preceded changes. Indexed/combined patches, hashes, source findings and compiler receipts are retained. Delta whitespace is clean and the Save help XML parses. The initial scratch edit failed to parse before any write; its unfinished work was executed from a corrected script after checking every current source against its captured preimage. The earlier runner-path invocation started no compiler; only build v1 compiled.

No new tests, native cleanup/initialization/reload/GUI/model/helper/MATLAB/mission/numerical/runtime runs, acceptance auditors/readers/seals or evidence admission occurred. Original requirements/text/fingerprints/fixtures/assertions/tolerances/denominator/gates/dispositions remain unchanged. Tracking-v127 retains 1093/1158 (94.39%), 65 unresolved engineering rows; recorded source-versioned verified baseline remains 1053/1158 (90.9%). All 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next implementation
-------------------

Continue complete navigation definitions and measurement/filter/solve-for/residual-plot list snapshots, resource default/input/reference/equation and external-file portability semantics. Finish coherent changed owned component/storage graphs, function I/O/scoping/shared-global/dependency lifetimes, all-command solver/estimation/MATLAB behavior and every original worklist dependency before runtime verification. Deferred checks include long-to-short/changed/empty propagation mappings in appended snapshots, default/override restoration, quoted or encoded brace strings, read failure, repeated native cleanup and subsequent estimator/simulator preparation/runs.
