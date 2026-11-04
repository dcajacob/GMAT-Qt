MATLAB resource creation and calls inside GMAT functions
========================================================

The independently rebuilt ledger admits exactly ``All Others!E152`` (FRAC-6.1.32)
and ``FRR!E478`` (FRR-37.2.6). Coverage is **1,021/1,158 (88.2%)** with 137 rows
open and **80 more needed** for the unchanged 1,101-row target. Historical operation
coverage remains 1,816/2,367. Scientific, technical-core, full-current-build,
full-NASA and the complete MATLAB-bundle gates remain false.

Authority and exact row scope
----------------------------

The unchanged workbook says "32)  Matlab function" under the Resource Tree
create/configure parent E120, and allows MATLAB to be called inside functions
in FRR E478 under parent E472. The original closure queue copied the identical
``matlab-fmincon`` planning bundle gate onto all 37 MATLAB rows. The preserved
``row-scope-review-v1.json`` reviews the original parent/child records and existing
separately verified siblings E153 and E474. It assigns general I/O, process modes,
reverse interface and optimizer obligations to their separate original rows.
The parent rows remain open. The original copied bundle gate remains recorded
and unqualified; no unrelated sibling or parent receives credit.

All 139 prior source-row records, fingerprints, texts, gaps and closure records
survive in ``exact-row-closure-overlay-v1.json``. All 59 original hold identities
survive with two resolved and 57 unresolved, including 35 unresolved MATLAB rows.
MATLAB availability is verified. Older unavailable statements remain historical.
All 321 Parameter whole obligations, E477/E491/E406 and the four plot rows remain
open; 298 bound Parameter names and 23 queued names confer no automatic credit.

E152 reuses the complete sealed creation component: actual Resource Tree Add,
creator Cancel/accept, native five-field census, writable FunctionPath with Unicode
path draft/Apply/Undo/Redo, save/reopen, four real MATLAB calls, raw CSV and the
actual ReportViewer. Its former zero-credit receipt remains immutable. The new
independent qualification owns the admission after the exact-row scope review.

Native call matrix and source versions
-------------------------------------

Fourteen profiles pass individually in both headless and visible modes. Eleven
positive profiles execute initial, no-explicit-Build repeat and Unicode-save/reopen
missions. Each mission makes one main MATLAB call and three calls inside GMAT
functions: two direct Exercise calls and NestedExercise calling Exercise.
The FunctionCalls counter is three; the nested wrapper is counted separately.

* Scalar and explicit Global MATLAB-resource calls;
* full 2-by-3 Array transfer with unequal entries and independent permutation;
* String and Unicode strings containing two actual apostrophes;
* numeric and string property inputs;
* zero-input and zero-output MATLAB calls, including an exact ordered side-effect file;
* formal-input/output aliases;
* literal rejection with same-window corrected-source recovery and clean close;
* undeclared output and unsupported actual ChemicalTank output-alias rejection.

Private arguments are separate data frames. Automatic Global handling means these
cases do not establish independent private MatlabFunction resource identities.
The property-input cases do not claim full carrier-state or property-output tests.

The admitted I/O evidence contains **28 GUI case contexts, 66 positive missions,
264 actual MATLAB calls, 198 calls inside GMAT functions, 66 nested wrapper calls
and 28 visible native exposures**. The prior creation component is separate.
The auditor compares native objects, raw files, exact reports, real readonly
ReportViewer text, source/history, preserved .m/.gmf files, JUnit, terminal process
observers and actual project/SDK process maps. Visible windows start minimized,
remain resizable and nonfullscreen. Screenshots, copied runtime binaries and
copied kernels: zero. Licensing services terminated: zero.

The exact per-case run, source version, PID and batch return code are in
``row-admission-audit-v1.json``. Headless Scalar/Recovery/Close reuse v7; five other
passing headless profiles reuse v3; string profiles use v9; alias/guard profiles
use v12. Visible cleanup uses v7, String/QuotedString v9, and remaining profiles
v12. The mixed headless-v2 batch remains failed, with only its individually passing
cases reused. Every source capsule v1 through v12 remains. v8 onward contains
2,955 watched sources. Earlier offline-helper provenance is explicitly bound to
the prior sealed source archive by ``runtime-helper-binding-v1.json``.

Production repairs and retained failures
---------------------------------------

Three production paths change, with four exact incremental patches preserved:

* Sandbox.cpp finalizes distinct live UserDefinedFunction graphs before deleting
  sandbox resources after failed initialization. It does not dereference the
  borrowed main sequence during Clear. The original close crash and the first,
  unsuccessful main-sequence repair remain with SIGSEGV logs and debugger traces.
  Previously working Build recovery passes before and after the final repair.
* CallMatlabFunction.cpp doubles actual apostrophes when encoding MATLAB character
  literals for String variables and string-valued Parameters. The corrected fixture
  preserves the original generated caller bytes. Before repair, two apostrophes
  arrive as one; after repair, exact Unicode/quote bytes survive. Driver and oracle
  are identical across the production repair.
* Interpreter.cpp permits deferred output lookup only for an exact declared formal
  input name in a CallMatlabFunction inside a function. CallMatlabFunction validates
  its actual Array/real/string bound object, using lexical currentFunction. Other
  missing outputs retain rejection; actual ChemicalTank alias output is rejected
  without changing the payload, sentinel, source or caller output. The same driver
  and oracle run before and after this repair.

No new API, class layout, writable flag or numerical formula is introduced by these
increments. The inventory contains **248 pending core paths**: 245 prior hashes
unchanged, existing Sandbox.cpp and Interpreter.cpp incremented, and the newly
changed CallMatlabFunction.cpp. Independent technical review remains pending.
Global startup and Fmincon's non-Windows CMake guard are unchanged.

All initial validator-path failures, the once-only FileManager initialization
mistake, failed cleanup repair, genuine quoted-string wire loss, formal-alias Build
failure, rejected-output test diagnostic correction and fixture-edit syntax error
remain. Object and Literals are still failed positive source-conflict probes. They
remain registered, unqualified and visible in the failed batch; this is not an
all-green 16-case suite. The selected acceptance/regression matrix has 14 cases.
The original 24 runtime require expressions remain in order; the CLI census grows
by four regression cases. Source QA preserves the exact fixture semantic corrections.

Independent audit and ledger
----------------------------

``AuditMatlabCleanupEvidence.py`` checks the original crash, failed first repair,
exact successful patch, previously working recovery and six corrupt-proof controls.
``AuditMatlabRowAcceptance.py`` reuses that audit and the sealed creation audit,
checks exact row scope and all 28 case contexts, and rejects seven corrupt proofs.
The qualification binds 3,204 terminal regular artifacts before admission. The
ledger rebuild runs all 20 independent qualifications, preserving both original
denominators. Its first attempt failed because a pinned ERFA package in /tmp had
been removed. Reinstalling pyerfa 2.0.1.5 restores every recorded package hash;
``rebuild-ledger-v2.log`` passes without a native replay or changed qualification.

``source-qa-v1.json`` and ``core-inventory-v2`` preserve exact increments and current
core hashes. ``evidence-manifest-v1.json``, ``final-sources-v1.tar.gz`` and
``stage-receipt-v1.json`` bind the stage once sealed. Runtime aliases are inventoried
as symlinks, outside the regular immutable-artifact set.

Replay
------

The reusable target links the shared native observer and actual MATLAB C APIs.
The installed licensed SDK, csh and an available display route are prerequisites.
Use a fresh output directory on every run. To run the 14 supported/regression cases
on the current sources::

   cmake --build build/linux-gui --target GmatQtNasaMatlabFunctionIoTests -j2
   python3 src/qtgui/tests/RunMatlabFunctionIoAcceptance.py --filter '^QtGui.NasaMatlabFunctionIo(Scalar|ScalarGlobal|Array|String|QuotedString|NumericProperty|StringProperty|NoInput|NoOutput|InOut|LiteralRecovery|LiteralClose|UndeclaredOutput|WrongAliasType)$' --output build/gui-acceptance/matlab-io-fresh-headless
   python3 src/qtgui/tests/RunMatlabFunctionIoAcceptance.py --visible --filter '^QtGui.NasaMatlabFunctionIo(Scalar|ScalarGlobal|Array|String|QuotedString|NumericProperty|StringProperty|NoInput|NoOutput|InOut|LiteralRecovery|LiteralClose|UndeclaredOutput|WrongAliasType)$' --output build/gui-acceptance/matlab-io-fresh-visible

The default filter includes the two failed exploratory probes. Re-audit recorded
source-versioned evidence without rerunning native missions::

   python3 src/qtgui/tests/AuditMatlabRowAcceptance.py --qualification build/nasa-coverage/coverage95-20261103-matlab-function-io/qualification-v1.json --output /tmp/matlab-row-readback-fresh.json
   python3 src/qtgui/tests/TrackCoreChanges.py --verify build/nasa-coverage/coverage95-20261103-matlab-function-io/core-inventory-v2

The frozen run-with-maps-vN scripts require their recorded sources and images.
Later offline dispatcher, inventory-description and documentation edits are explicit
new sources; they do not invalidate older source capsules or justify a native rerun.
Fresh qualification evidence also needs source/image custody, process maps and
independent admission. A fresh native pass alone does not update the ledger.

Remaining work
--------------

General MATLAB I/O remains open, including documented array elements and property
outputs, type/shape/error and wider lifetime contracts. The original workbook has
no FRR-36 declaration, only its I/O reference; FRR-37 function-I/O clauses are not
silently substituted. Whole-object/literal source conflicts remain explicit.
The installed engine.h states engOpenSingleUse is unsupported on Unix; its symbol
alone proves no process-isolation contract. Shared/single-use behavior, reverse
interface and Qt optimizer callbacks need their own implementation and acceptance.
The full 95% goal remains active and incomplete.
