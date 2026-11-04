Independent FK5 reference and 95-percent prerequisites
====================================================

Coverage remains **998/1,158 requirements (86.2%)**, with **103 complete
original obligations** needed for 95 percent. Historical coverage remains
**1,816/2,367 (76.7%)**. This investigation awards no new requirement credit.
No GUI mission was replayed and no production/core file changed.

Independent reference work
--------------------------

The eight retained FK5/ICRF impulse/thrust cases already establish bounded
directional transformations, raw/native state agreement and the internal
MJ2000Eq/FK5 integration basis. Repeating those cases would not close the
remaining FRR!E144 force-family and independent-reference obligations.

``Fk5StandardsOracle.py`` instead composes an independent reference using
ERFA/SOFA's IAU1976/1980 equinox reduction and IAU2006/2000A CIO reduction,
through one common terrestrial frame. The reference does not call GMAT time
or coordinate APIs or use the observed GMAT table as its expected result.
It keeps split Julian dates, converts A1 through TAI/TT/UTC/UT1 using an explicit
A1-minus-TAI constant and ERFA, and interpolates the retained EOP UT1 input.
Unsupported dates, missing daily EOP coverage and UTC leap intervals reject.
Common polar motion cancels; a separate contract checks that cancellation.

PyERFA2.0.1.5, ERFA2.0.1 and SOFA20231011 are pinned and fingerprinted.
The dependency is installed only in ``/tmp/gmat-fk5-reference-env``; system
Python and GMAT's runtime are unchanged. Thirteen reference contracts pass,
including published ERFA matrix/sidereal-time/equinox constants, time-system
separation, direction/inverse checks and invalid-input controls. The original
published validation source is retained with its license/provenance.

Primary reference routines and validation constants are available from
`ERFA v2.0.1 <https://github.com/liberfa/erfa/tree/v2.0.1>`_. In particular,
``pnm80`` defines the IAU1976/1980 matrix, ``c2teqx`` the equinox composition,
and ``c2t06a``/``c2tcio`` the IAU2006/2000A terrestrial transformation.

The three retained comparison epochs are A1 MJD21545,25300.25 and30000.
The maximum element difference from the shipped rotation table is approximately
**1.30e-9**. Across three asymmetric vectors of orbital scale, the largest
position difference is approximately **1.02e-5 km (10.2 mm)**. These are
diagnostic differences, **not a demonstrated GMAT bug or an acceptance pass**.
No residual correction, fitted time shift, table edit or relaxed existing
assertion is used.

Source reconciliation remains necessary. The shipped CoordinateSystem guide
describes a time-dependent table obtained through EarthFixed, while the math
chapter also describes a constant frame-bias matrix. The modern chapter names
``iauXy06``/``iauS06a`` whereas the direct matrix reference uses the equivalent
matrix-based SOFA route; their small numerical differences are separate from
data-generation provenance. The actual table generator, exact historical time
and reduction conventions, interpolation error budget and intended accuracy
must be established before choosing a production repair or claiming full
independent FK5 truth. The nine-family static force inventory is preserved as
planning evidence; it is not a qualified runtime inventory or force matrix.

Why 95 percent needs a prerequisite change
------------------------------------------

``95-prerequisite-ceiling-recheck.json`` revalidates all 59 original holds,
including their unchanged fingerprints and source text:

* 37 MATLAB/interface/fmincon obligations;
* 11 removed user-body two-body obligations;
* eight named historical-OS obligations;
* three unspecified minimum-hardware targets.

All 59 remain among the 160 unqualified original rows. Even if all other
101 rows closed, the total would be **1,099/1,158 (94.9%)**, below the required
**1,101**. At least **two held obligations must be resolved**. This is an
optimistic mathematical ceiling, not a forecast that the other101 rows are
already satisfiable or inexpensive.

The current configured MATLAB root is empty and no ``matlab`` command is
identified on PATH. The native host is modern Linux; the campaign has no new
qualified historical-OS receipts. The source still explicitly deprecates
user-body two-body propagation, and the original hardware placeholders remain
undefined. This does not claim an environment or license cannot exist
elsewhere. A user decision on prerequisite access or broader restoration work
has been requested; elapsed time is not treated as an answer.

No original scope is retired to change the arithmetic. The goal remains active.
The independent/core, full-current-build and full-NASA qualification flags stay
false.

Reusable commands and limits
-----------------------------

Create the isolated reference environment and exercise its contracts::

   python -m venv --system-site-packages /tmp/gmat-fk5-reference-env
   /tmp/gmat-fk5-reference-env/bin/python -m pip install \
     -r src/qtgui/tests/Fk5StandardsOracle.requirements.txt
   /tmp/gmat-fk5-reference-env/bin/python src/qtgui/tests/Fk5StandardsOracleTests.py -v

Run the diagnostic comparison::

   /tmp/gmat-fk5-reference-env/bin/python src/qtgui/tests/Fk5StandardsOracle.py \
     --table application/data/icrf/ICRF_Table.txt \
     --eop application/data/planetary_coeff/eopc04_08.62-now \
     --epoch 21545 --epoch 25300.25 --epoch 30000 \
     --output /tmp/gmat-fk5-standard-comparison.json

This is a reference-development diagnostic, not an E144 admission tool or a
new default GUI/CTest acceptance registration. Its pinned external dependency
and remaining convention/accuracy questions remain explicit. It does not
replace the retained GUI evidence. Additional force-family GUI cases are not
launched on the strength of this unreconciled reference.
The comparison currently exits with code2 for the diagnostic disagreement;
that exit code does not classify the difference as a GMAT bug. The separate
13 published-reference/time contracts pass.

The campaign's ``fk5-independent-reference`` directory retains comparisons,
contracts, source/model fingerprints, dependency identity, published validation
source, the nine-family static inventory and the final investigation seal.
All 104 pending core paths, original FK5 tests/fixtures/oracle and previous
sealed reports remain unchanged. No screenshots or runtime image copies were
created. The small isolated dependency environment is a new reference-tool
installation, not a copied GMAT runtime.
