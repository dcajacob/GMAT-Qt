85 percent campaign: SPK and CCSDS-TDM truth comparators
========================================================

Source-versioned requirements coverage is **979/1,158 (84.5%)**. **Six more
requirements** are needed to reach the unchanged985-row target. New exact rows
are ``All Others!E306`` (SPK file comparison) and ``All Others!E308`` (CCSDS-TDM
comparison). Historical coverage remains **1,816/2,367 (76.7%)**. All **179
remaining requirements and551 historical gaps** retain their original scope,
text, fingerprints and closure gates. No denominator or previous assertion was
removed. These are acceptance-harness capabilities, not new GUI mechanisms.

Reusable comparisons
-----------------------

``AcceptanceSpecializedCompare.py`` provides actual two-file comparisons and
JSON/standalone HTML reports through the existing comparator registry. Its
explicit CLI supports additional SPK interpretation options. Existing decimal
arithmetic, tolerance policies, input protection, numeric/structural differences
and report machinery are reused. The existing comparator implementation and
registration remain unchanged and were not replayed.

TDM1.0/2.0 KVN/XML comparisons check every supported scalar measurement, all
supplied semantic metadata, segment counts, participant/path identities,
reference frames, time systems, units, integration/correction settings and exact
clock-label identities. Calendar and day-of-year forms normalize without losing
fractional seconds or conflating a UTC leap-second label with midnight. Records
may be grouped by observable; each observable remains chronological and
keyword/timestamp pairs are unique. Missing, extra, duplicate, nonfinite,
unsupported and malformed records fail or error. Exact decimal phase counts above
2**53 retain their small differences. Creation date, originator and message ID
are explicitly listed volatile header exceptions. No unit/time conversion, full
XSD validation or leap-table validation is claimed. Unsupported observation types
and XML structures produce errors. This is a record comparator, not a complete
CCSDS conformance validator.

SPK compares geometric position/velocity states through actual CSPICE exports
in the built GMAT libraries. Each file runs in a fresh reader process, preventing
cross-file kernel precedence contamination. It compares all ephemeris object IDs
and complete disjoint coverage windows, with explicit observer, frame, TDB epoch
origin, step, sample cap and km/km/s units. Each coverage interval includes both
endpoints. Missing supporting ephemerides/frames, corrupt files and exceeded
resource limits error. The JSON and HTML report the interpretation and loaded
library identities. Different valid segment encodings need not be byte-identical.

**SPK is a sampled-state comparison, not a continuous-error bound.** A deviation
between samples can escape detection. Native CSPICE handles supported kernel
encodings, but the independent numerical contract directly exercises types8/13,
selected inertial frames, two objects, disjoint windows and explicit missing
support data. Arbitrary segment types, overlap/discontinuity behavior, external
kernel portfolios and scientific mission truth remain broader qualification
gates. This new tool does not silently declare those domains qualified.

Independent acceptance checks
-------------------------------

The new ``QtAcceptance.SpecializedComparatorContracts`` registration contains
ten contracts. Literal KVN/XML fixtures supply independently specified values,
including9007199254740993.00000001. Real type8 and13 binary SPKs encode analytic
constant-acceleration trajectories for two distinct objects. All six state
components at every reported sample are checked against the polynomial, with
explicit position/velocity units. A second trajectory has an independently
calculated interior deviation and derivative; its endpoint positions match,
ensuring the test checks interior samples. Coverage/object/frame/observer errors,
strict tolerances, field overrides, malformed input, actual CLI exit codes,
JSON/HTML and input preservation are also checked.

Three small retained comparisons contain152 numerical cells, two native binary
SPK input pairs, one KVN/XML input pair and their reports. They are synthetic
comparator contract fixtures, not new GMAT mission truth or runtime image copies.
The loaded CSPICE/GmatUtil libraries have identical hashes before/after the
registration. No GMAT startup, sandbox, mission, GUI, screenshot or runtime copy
is involved. All101 previously tracked core changes remain byte-identical to
the preceding increment and pending independent technical approval.

Failures and evidence reuse
-----------------------------

Three new/affected registration attempts are retained. The first failed two
new fixtures that expected strict-zero agreement across different SPK encodings;
tiny legitimate interpolation rounding differences correctly failed zero
tolerance. Those two fixtures now use matching encodings for unchanged
components. Their assertions remain identical, and the separate type8/type13
numerical oracle retains its original explicit1e-9 tolerance. All55 original
assertion calls remain, with four additional report-interpretation assertions.

The second registration passed all ten tests, but the new proof emission lacked
the required facet list. Coverage correctly stayed unverified. The final
registration emits those tested facets and includes interpretation/limit tables
in its reports; it passes all ten contracts and qualifies both rows. Old receipts
are unchanged. No established comparator, GUI or engine registration was replayed.

Nine coherently rehashed corrupted proof copies reject missing/duplicate scope,
inflated NASA claims, wrong frames, missing samples, corrupted state/delta or
exact phase values and unbound native libraries. One mutation initially replaced
a value with itself; it was corrected to change the value. No comparator or
engine is executed by these evidence checks.

Usage and audit
------------------

Compare supplied TDM files::

   python src/qtgui/tests/AcceptanceSpecializedCompare.py \
     --kind tdm --actual actual.tdm --truth truth.tdm \
     --absolute 0 --relative 0 --json comparison.json --html comparison.html

Compare SPKs with a deliberate frame, observer and grid, for example::

   python src/qtgui/tests/AcceptanceSpecializedCompare.py \
     --kind spk --actual actual.bsp --truth truth.bsp \
     --observer 399 --frame J2000 --step 10 --max-samples 10000 \
     --absolute 1e-9 --json comparison.json --html comparison.html

The example tolerance is illustrative; choose an appropriate policy for supplied
truth. ``--policy`` accepts existing per-field absolute/relative tolerances.
Default tolerances are zero. ``--library`` selects an explicit CSPICE-exporting
library; the default is this checkout's ``libGmatBase.so.R2026a``. Successful
sampling requires each file to supply the needed ephemeris chain in the selected
frame. Supporting-kernel loading is not currently exposed.

Run just these new contracts::

   python src/qtgui/tests/RunHarnessAcceptance.py \
     --build-dir build/linux-gui \
     --review src/qtgui/tests/NasaSpecializedComparatorTraceability.json \
     --metadata src/qtgui/tests/AcceptanceSpecializedComparatorMetadata.json \
     --output /tmp/gmat-specialized-comparator-new-run

The destination must not exist. Re-audit existing evidence without replay::

   python src/qtgui/tests/AuditSpecializedComparatorAcceptance.py \
     --qualification build/nasa-coverage/coverage85-20261101/specialized-comparators-qualification.json
   python build/nasa-coverage/coverage85-20261101/negative_specialized_qualification.py
   python build/nasa-coverage/coverage85-20261101/audit_ninth.py

Format semantics were checked against `CCSDS503.0-B-2 with Corrigendum1
<https://ccsds.org/Pubs/503x0b2c1.pdf>`_ and the `NAIF SPK guide
<https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/req/spk.html>`_. The original
2012 NASA requirement authority and denominators remain unchanged.

The goal remains active. Full current-build, NASA, scientific, platform and
arbitrary-domain qualification are not claimed. The canonical ``coverage.json``
contains the complete source-versioned union and every remaining row.
