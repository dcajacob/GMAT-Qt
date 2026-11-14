95 percent campaign: remaining orbit display persistence
========================================================

Six new Save cases pass on their first unattended and visible executions:
BrouwerMeanShort, BrouwerMeanLong and four Planetodetic spheroids (sphere,
Earthlike, oblate and southern latitude). All 142 preceding native state/source
cases are reused at their recorded versions. Combined evidence has **148 cases,
296 contexts and 10,190 writable-property comparisons**. No new native failure,
core change or Qt production change is introduced.

Coverage remains **998/1,158 requirements (86.2%)**, with 103 complete requirements
needed for 95%. Historical coverage remains **1,816/2,367 (76.7%)**. This increment
awards no complete requirement. Accounting for all 14 current Spacecraft display
representation names does not prove every anomaly, frame, body, domain, reference
graph or hidden persistent setting. FRC!E132 remains open; original authorities,
denominators, fingerprints and qualification gates remain unchanged.

Selected graphs and physical state
------------------------------------

Each case explicitly saves Earth and Subject. Planetodetic cases additionally
save PlanetodeticFixed with its BodyFixed axes, alongside the configured Earth
flattening and radius. The native Save picker, ordered selection and empty guard,
exact Apply/source retention, following mission statement, independent export
rebuild, writable native fields and owned objects retain their existing checks.
No preceding case is replayed to obtain this evidence.

The unchanged native Cartesian helper reads Spacecraft.GetState("Cartesian")
before Save and after independent rebuild. For Planetodetic, those six components
are expressed in PlanetodeticFixed; the comparison does not silently treat them
as inertial components. The original position/velocity bounds remain 1e-6 km and
1e-9 km/s. All 49 preceding balanced require expressions, including the helper
declaration, and the complete shared native driver remain byte-identical.

The new cases reuse their exact six original input literals. Initial physical
state is checked by Cartesian truth rather than requiring nonlinear inverse
getters to echo every input bit. All writable native display properties still
undergo their original strict before/after persistence comparison. No older
case, assertion, bound or field comparison is changed.

Retained oracle limits
------------------------

Planetodetic truth recomputes spheroid-normal position and the retained radial
HFPA/north/east velocity convention from the frozen oracle source. Sixteen native
captures cover both phases and modes for the four spheroids. Independent Earth
orientation, ellipsoid-horizon velocity conventions, interior/evolute, near-pole
and broader domains remain unqualified.

Brouwer truth executes the two retained NASA scalar prototypes through the
existing restricted adapter at their exact recorded hashes, then independently
solves the Kepler equation and constructs the Cartesian conic basis. Eight native
captures cover both phases and modes. This is prototype/source parity plus conic
geometry, not independent physical averaging, all inverse/singular-domain proof
or FreeFlyer certification. MATLAB integration requirements are not qualified by
executing the adapter in Python.

The 24 new captures contain **144 Cartesian component checks**. Their sources,
original fixture inputs, reference frames, bounds and raw captures are bound by
the independent conjunction audit.

Reference version recovery
---------------------------

The original Brouwer fixture cites an older orbit-state guide hash. The current
dirty guide contains earlier unit-wording and anomaly-field additions. The exact
historical guide bytes were recovered read-only from the other checkout and
retained separately at the original hash. Both prototype files and their adapter
still match the fixture's recorded hashes. Neither the fixture nor the current
guide is changed, and guide text is not a numerical oracle or new NASA credit.
The identities and distinction are recorded in
save-orbit-modes-reference-reconciliation.json.

An initial audit preflight rejected a relative-versus-absolute artifact path in
that new reference manifest. The path binding was corrected, without changing
source content or native results; both audit logs remain retained. There were
zero failed native attempts in this increment.

Audit and storage
-------------------

All 21 preceding Save core review paths and Qt production sources remain
unchanged. The preceding 53 shared checker and 11 Cartesian contract passes are
reused with unchanged sources. The new audit rejects ten corrupt conjunctions
and sixteen isolated corrupt copies of real native capture records after a valid
baseline. These controls cover wrong position/velocity, frame/object, missing
capture, widened bounds, nonfinite values, changed expected literals, missing
reference identity, altered oracle kind and omitted graph membership.

Visible cases use automatic scaling in initially minimized, resizable,
non-fullscreen nested windows. No passing screenshots, runtime binary copies or
existing kernel copies are retained. Lossless compaction of 34 duplicate reports
and traces saved 14,950,400 allocated bytes (14.3 MiB), preserving every admission
artifact. Expanded descriptors and representation-name accounting are retained
separately; earlier seals and inventories remain unchanged.

Audit the conjunction without native replay::

   python src/qtgui/tests/AuditSaveOrbitModeEvidence.py \
     --evidence build/nasa-coverage/coverage95-20261101/save-orbit-modes-evidence.json

To watch these six cases, choose a fresh output directory and restore the
initially minimized test window::

   python src/qtgui/tests/RunAcceptance.py \
     --build-dir build/linux-gui --filter '^QtGui.NasaSaveOrbitMode' \
     --visible --display-route nested --display-scale auto --start-minimized \
     --pause-ms 350 --max-checkpoints 200 --max-screenshots 0 --report-every 6 \
     --output build/gui-acceptance/save-orbit-modes-watch

RunAcceptance uses Python regular expressions. The older broad
'^QtGui.NasaSaveOrbit' selector now also includes these new OrbitMode cases.
Use '^QtGui.NasaSaveOrbit(?!Mode)' to watch only the preceding eleven definitions.
Omit visible/display flags for unattended operation. Frozen receipts qualify
only their recorded source/runtime versions, not the complete latest build.

Remaining scope
-----------------

Anomaly variants, local/named frames, additional body/reference graphs,
state/rate/sequence partitions, optional FK and file-backed post-rebuild
initialization, external dependency portability and hidden persistent fields
remain open. Parent force-order, computed getter and earlier retired-editor-clean
observation limitations remain inherited. The name inventory is not complete
any-Resource Save qualification.

All 59 original prerequisite holds remain unresolved. Even if every other row
closed, the scope would reach only 1,099/1,158 (94.9%); at least two held
obligations need substantive resolution for 1,101. Further source conflicts may
lower achievable coverage. No unanswered prerequisite decision is assumed.
Private NASA, historical platforms, scientific, technical core approval and
complete current-build gates remain open. The 95% goal is not complete.
