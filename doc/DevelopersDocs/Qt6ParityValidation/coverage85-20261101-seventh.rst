85 percent campaign: reusable acceptance report formats
========================================================

Source-versioned requirements coverage is **975/1,158 (84.2%)**, with **10
more needed** for985. The new exact source rows are ``All Others!E327/E328/E329``:
the complete report-format parent and its XLS and TeX children. HTML remains
an existing credited child and is checked through the new CLI as well. There
are51 additions since the sealed924-row baseline. Historical coverage remains
**1,816/2,367 (76.7%)**. All **183 remaining requirements and551 historical
gaps** keep their original text, fingerprints, scope and closure gates.

This increment implements acceptance-harness capabilities. **It does not add
features to GMAT or change the GUI or numerical engine.** All101 existing
changed core files remain byte-identical and pending independent review.

Actual format contracts
-------------------------

``AcceptanceReportExports.py`` exports recorded catalogues or truth-file
comparisons as genuine Excel97 BIFF8 compound workbooks, standalone LuaLaTeX
source and the existing HTML format. Counts remain numeric; exact decimal
comparison values remain literal strings, including9007199254740993 versus
9007199254740992. Source paths/hashes, all summary counts, test outcomes,
requirement identities, metadata, errors and structural/numeric differences
remain inspectable. Extra record fields are preserved as JSON detail cells.

The five new contract tests use the existing independent mixed-outcome fixture
without replaying its tests. They check actual CLI exports, native LibreOffice
XLS readback, the compound workbook's BIFF8 header, complete metadata and exact
counts, passed/failed/timeout/skipped/interrupted/unrun/reused distinctions,
duplicate declared requirement IDs, literal formula and TeX input, safe output
aliases, converter failures and explicit no-truncation limits. Every comparison
value is retained with its exact decimal text, unit, difference and tolerance.
LuaLaTeX must compile with no missing glyphs or overfull lines, and extracted
PDF text must contain the independently specified values. Native XLS readback
also checks string types and wrapping of long fields.

LibreOffice runs headlessly with an isolated temporary profile. Export requires
LibreOffice; the contract additionally requires LuaLaTeX, DejaVu Sans,
``pdftotext`` and Python ``olefile``. No interpreter or application binary is
copied. Their actual entry points have unchanged before/after hashes.
Excel97 limits are explicit:65536rows,256columns and32767UTF16units per cell;
oversized data rejects instead of being silently truncated. Full arbitrary
Office/TeX/locale/renderer compatibility is not claimed.

Failures and final visual verification
----------------------------------------

The first registration attempt failed when the sandbox prevented LibreOffice
conversion. The second reached native output and caught overflowing TeX
identity hashes. The writer now inserts nonprinting line-break opportunities
without changing their characters. An intermediate explicit-text style then
lost wrapping through automatic-style inheritance during Excel conversion.
Visual review caught the clipping. Concrete styles now carry their own wrap
and font properties; the original data/layout assertions remain, with new
native string-type and wrap assertions. The final five-contract registration
passes. Earlier failed attempts, intermediate passes and their source versions
remain recorded; none is rewritten or attributed to the final writer.

All seven workbook sheets and both compiled TeX PDFs were visually reviewed.
The generic workbook preview engine abbreviates long numeric strings despite
literal underlying cells. Native Calc PDF and BIFF8 readback independently
confirm the exact full digits. That preview limitation is recorded rather than
treated as evidence of rounded workbook data. Temporary preview PNGs are
deleted; no passing screenshot is retained. Native PDF/XLS/TeX examples are
small, explicitly synthetic contract fixtures, not new GMAT-run receipts.

No established GMAT GUI, engine, catalogue or comparator registration was
replayed. Only this new or changed five-contract registration was exercised.
Additional render/export checks do not replay a test registration. Nine
coherently rehashed corrupt proof copies reject missing formats/parent children,
wrong counts, failed execution, removed native assertions, changed tool inputs
and unsupported source credit. They execute no test, exporter, GUI or engine.

Frozen source files, actual receipts/logs/JUnit, qualified reports and source
archives remain available. Unneeded run-local catalogues and failed source
reports are losslessly compressed with original and compressed hashes. The
successful qualification inputs remain unchanged. Coverage records explicitly
classify these additions as harness contracts; they are not native GUI passes.

This is not full current-build, host/GPU, scientific, platform or NASA
qualification. C API all-command summaries, linked configuration families,
broader scalar/frame families and the other exported gaps remain open. The
85percent goal remains active.

Reuse and evidence audit
---------------------------

From the checkout root, a new run of just this harness contract is::

   python src/qtgui/tests/RunHarnessAcceptance.py \
     --build-dir build/linux-gui \
     --review src/qtgui/tests/NasaReportExportTraceability.json \
     --output /tmp/gmat-report-format-new-run

The destination must not already exist. Export a recorded catalogue with::

   python src/qtgui/tests/AcceptanceReportExports.py \
     --catalogue /tmp/gmat-report-format-new-run/catalogue.json \
     --xls /tmp/gmat-report-format-new-run/acceptance.xls \
     --tex /tmp/gmat-report-format-new-run/acceptance.tex \
     --html /tmp/gmat-report-format-new-run/acceptance.html

Use ``--comparison`` instead of ``--catalogue`` for an existing comparison JSON.
No credit is inferred simply by exporting a recorded result. Audit the sealed
evidence without replaying tests::

   python src/qtgui/tests/AuditReportExportAcceptance.py \
     --qualification build/nasa-coverage/coverage85-20261101/reports-qualification.json
   python build/nasa-coverage/coverage85-20261101/negative_report_qualification.py
   python build/nasa-coverage/coverage85-20261101/audit_seventh.py

The cumulative ``coverage.json`` lists the entire qualification union and every
original remaining row. Earlier increment archives retain their original
source identities and evidence; old passes are never attributed to a later
complete binary.
