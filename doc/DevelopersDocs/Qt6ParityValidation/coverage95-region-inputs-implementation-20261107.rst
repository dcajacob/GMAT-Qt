Region property inputs and complete snapshot replacement
========================================================

PlanetographicRegion now replaces complete Latitude/Longitude vectors through native typed setters and a class-specific script-array dispatch, including empty resets. Coordinate validation precedes replacement; indexed edits replace or append within native index/size bounds. Native latitude and longitude-span limits remain, with finite-value checks. Four label overloads use base-relative metadata indices instead of global IDs. AreaFileName validates paths/conflicts before mutation, supports accepted file-mode replacement and explicit clearing, and retires derived coordinates only after selection succeeds. Clearing file mode preserves materialized coordinates as inline data. Definition edits invalidate polygon readiness; native initialization also handles changed body shape and clears failed/partial geometry, while queries prepare dirty definitions. Explicit script-export snapshots clear filename and both coordinate lists before current inputs. Existing parameter IDs/layout, retained Clone/Copy/context code, ordinary/MATLAB writer paths and polygon algorithms remain unchanged. Five source/help paths compile; two help XML files parse. Full function/resource/input/file/cache/persistence runtime scope remains open. No row or verification credit.

Native input behavior
---------------------

Four label accessors indexed a three-entry metadata table using inherited global IDs. They now use the same base-relative offsets as the unchanged metadata lookup methods. Complete native Rvector setters validate every coordinate before replacing latitude or longitude. Region-specific script dispatch handles bracketed values, including empty vectors, through that complete setter. Indexed edits replace an existing entry or append at the next position, with bounds and native integer-size checks. Latitude/span limits and the matching minimum three-point initialization requirement remain; nonfinite values are rejected.

AreaFileName path and mixed-input checks precede mutation. An accepted file-mode selection clears old derived coordinates and geometry for native preparation. Empty file selection retains materialized coordinates as inline input. Native parsing of file contents remains a later preparation step; path validation does not qualify file contents or portable packaging. Existing read-only metadata still suppresses file-derived coordinate serialization.

Geometry and snapshots
----------------------

Successful input changes invalidate polygon readiness. Native initialization checks changed ellipsoid dimensions and clears failed/partial geometry. Queries prepare dirty definitions before the existing algorithm. After removing only the new query guard, the original polygon/interpolation/geometry source tail is byte-identical. Numerical formulas and tolerances are unchanged.

Explicit script-export WriteParameters writes empty AreaFileName, Latitude and Longitude before delegating to the common writer. This permits shorter lists and file/inline transitions in appended snapshots without changing the live source. Ordinary/MATLAB writer routes and native property ordering remain. Header normalization reproduces the original header after removing method declarations: no new instance fields, parameter IDs or virtual slots. Retained region Clone/Copy/context code, common writer and Save file operations remain unchanged.

Source inspection also confirmed that private child FOS calls preserve the actual live caller graph. This packet does not replace that route with speculative active-session copying. Full global/dependency/native session restoration remains open.

Compilation and retained evidence
---------------------------------

Build v1 returned 0 in 66.98 seconds; progress 142/142; one existing OrbitRenderer Qt image-mirroring deprecation warning; no compiler errors.

Build v2 returned 0 in 4.33 seconds; progress 66/66; no compiler diagnostics; no compiler errors.

Both compiler sessions are terminal. Final hashes match build v2. Five source/help preimages and four planning snapshots preceded edits; ten initial unchanged references and two vector-construction references captured after terminal v1 are retained. Indexed/combined patches, hashes, compiler receipts and source/research notes are retained. Delta whitespace is clean and two help XML files parse. Existing configured probes may link; none execute.

No new tests, script/resource/geometry/file/function/Save-reload runtime checks, GUI/model/helper/MATLAB/mission/numerical execution, acceptance auditors/readers/seals or evidence admission occurred. Original requirements, text, fingerprints, fixtures, assertions, tolerances, denominator, gates and dispositions remain unchanged. Tracking-v122 stays 1093/1158 (94.39%), 65 unresolved engineering rows; recorded verified baseline 1053/1158 (90.9%), all 105 original acceptance rows open. Full implementation audit and goal remain incomplete.

Next implementation
-------------------

Continue original function I/O/resource/property/scoping/output staging and caller/global/dependency lifetimes, all-command target/optimizer/estimator behavior, coherent changed-component/topology/storage preparation/restoration and nested MATLAB sessions. Private child FOS calls already preserve the live caller graph; do not substitute speculative active-session copies for that route. Keep region array edits, rejected inputs, file-mode transitions, dirty geometry/body-shape preparation and appended snapshots for deferred verification. Continue specialized any-Resource serializers, defaults/lists, hidden/equation/input policies, complete graphs and external portability, body/frame/kernel/ephemeris, resource-tree, representative POV/exporter scope and all explicit specification/external/historical dependencies. Preserve original assertions and defer runtime verification until the full implementation pass is finished.
