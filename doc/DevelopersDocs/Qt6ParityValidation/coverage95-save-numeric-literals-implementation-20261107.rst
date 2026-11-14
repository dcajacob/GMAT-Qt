Explicit Save numeric literals
=============================

Explicit Save numeric serialization now uses at least Real::max_digits10 significant digits in native numeric streams while preserving a higher configured precision. Scoped formatting selects the classic decimal locale and default floating notation without modifying global locale or display settings. Variable values and Array cells bypass their hard-coded 16-digit display conversion only during explicit exports; the Array path also writes current values under the alternative initial-expression build policy. Common vector properties and OpenGlPlot viewpoint/reference/direction vectors use explicit numeric vector literals. GmatBase native scalar/matrix/element/string-array writers and existing spacecraft/attitude/Fmincon stream preparation inherit the scoped format. Ordinary serialization and inactive MATLAB formatting retain their prior branches. No equation, setter, coordinate transformation, tolerance or resource API/layout changes. Compilation only; full any-Resource Save scope remains open and no row or verification credit is added.

Source findings and implementation
----------------------------------

Variable and Array serialized values through hard-coded 16-digit conversions. Other native writers followed display precision, and vector formatting could use an intermediate locale-sensitive string. These paths can discard stored finite Real precision when Save recreates mission values. ObjectScriptExportScope now supplies a thread-scoped precision floor, classic-locale/default-float stream preparation and direct scalar/vector literals. It does not change the process locale or global display configuration. Existing ObjectMatlabExportScope stream preparation delegates the script policy before applying its own existing MATLAB policy. GmatBase GetDataPrecision honors the explicit script floor and WriteParameterValue prepares its stream; non-export calls retain their prior precision and formatting.

Variable only selects its numeric literal helper during explicit Save. Array similarly writes the current nonzero cell before either compile-time native initializer branch. Save retains its existing explicit zero-cell policy. Plot viewpoint strings use the numeric vector helper only during explicit exports. Existing native setters and algorithms are unchanged.

Limits
------

Formatting is an implementation change, not proof of numerical reload equivalence. The new helpers preserve finite stored Real precision; they do not add a non-finite literal protocol or change special resource domains. Time conversions, Cartesian/orbital/attitude representations and native computed properties keep their own algorithms. Specialized serializers returning preformatted numeric strings outside these inspected paths, complete object/default/reference/portability policies and every original any-Resource obligation remain open. The alternative Array policy is handled in source but the configured integrated build exercises its own preprocessor selection only. No global GmatGlobal setting, process locale, class instance layout, virtual contract or parameter ID changes.

Compilation and retained evidence
---------------------------------

Build v1 returned 0 in 10.87s, progress 76/76, 0 existing compiler deprecation warnings; no compiler errors. Compiler session 44711 is terminal. Final hashes match build v1. Eight source/help preimages, eleven unchanged reference files and four planning snapshots preceded changes. Indexed/combined patches, hashes, source findings and compiler receipts are retained. Delta whitespace is clean and the Save help XML parses. The first follow-up edit stopped at a missing next-method delimiter before any write; the unfinished Array/plot/help work was completed using the actual end-of-file boundary, without repeating the capture.

No new tests, numerical/locale/reload/GUI/model/helper/MATLAB/mission/runtime runs, acceptance auditors/readers/seals or evidence admission occurred. Original requirements/text/fingerprints/fixtures/assertions/tolerances/denominator/gates/dispositions remain unchanged. Tracking-v126 retains 1093/1158 (94.39%), 65 unresolved engineering rows; recorded source-versioned verified baseline remains 1053/1158 (90.9%). All 105 original acceptance rows remain open. Full implementation audit and goal remain incomplete.

Next implementation
-------------------

Continue specialized any-Resource serialization, hidden/default/list/equation/input/reference and external-file portability policies, including resource writers that manufacture numeric strings outside the common path. Preserve native time and coordinate/attitude representation computations; do not infer scientific round-trip qualification from formatting. Continue changed owned component/storage graphs and full original function/scoping/command/session/provider work across the existing worklist before runtime verification. Deferred verification includes finite binary64 edge values, signed zero, low/high display precision, numeric locales, Array build policies, nested/failure scope restoration, ordinary/MATLAB output equivalence and representative resource reloads.
