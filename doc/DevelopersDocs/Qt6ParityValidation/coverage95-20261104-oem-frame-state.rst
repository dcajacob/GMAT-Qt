OEM state, unit and declared frame evidence
=========================================

Coverage remains **1,051/1,158 (90.8%)**:107 original requirements open,50 needed
for1101. Historical1816/2367 and all59 original hold identities are unchanged.
This component provides evidence without admitting a partial requirement.

Nine new GUI cases export complete six-component states in three declared frames:
Earth equatorial,Earth ecliptic,and equatorial axes centered on another propagated
spacecraft. Each frame is exercised with circles in the XY,YZ and ZX planes.
The second spacecraft follows a separate8000km circle;the exported spacecraft has
radius7000km. Independent equations derive both circles and their six-state
difference. Ecliptic output uses the declared IAU1976 J2000 mean obliquity,
84381.448arcsec,published in `NASA/JPL Astrodynamic Parameters
<https://ssd.jpl.nasa.gov/astro_par.html>`_. This convention is explicit in the new
private sender/receiver profile;no retrospective legacySatId agreement is invented.

All nine cases pass headless and visible. Each retains five native-step records
at0,5,10,15 and20seconds:45 unique output states,90 retained across both profiles.
Maximum component errors are2.7284841053187847e-12km and3.552713678800501e-15km/s,
below unchanged2e-5km and2e-8km/s bounds. Every record is checked independently,
including units,epoch,identity,center/frame metadata,B1 syntax and UTC creation.
Paired final reports verify the Earth-frame analytic result,sentinel,and exact
same-frame terminal output at2e-10km/2e-13km-per-second bounds.

The new fixture adapts two inherited expressions for the new frames:expected
metadata is case-specific,and the exact terminal comparison uses the selected
output-frame report. Two new assertions check that report's shape and the actual
native exporter frame. All54 other inherited expressions are retained verbatim.
The original drivers,catalogs,assertions and tolerances remain byte-identical,
including the already approved ShortArc expected_degree1. No new assertion
exception is requested or inferred.

No production change was needed. All285 pending core source hashes and the four
common application/library binary identities match the preceding text component.
Only the new target,registrations,fixtures,oracle and evidence reader were added.
The visible display starts minimized,is resizable and is not fullscreen. All18
native identities and one display identity were observed terminal in their actual
execution host namespace. No runtime binary/kernel copies or immutable bindings
to runtime alias targets are created.

The prior text component replays after the build without native reruns,retaining
the sealed1051 prefix. Full1158-row and107-gap audits preserve canonical row
records/statuses and all59 hold keys exactly,with progress annotations only.

Limits and next work
--------------------

This matrix proves the three explicit frame/origin kinds on the native sample
grid. It does not qualify arbitrary frame astronomy,force models,origins or
interpolated epochs. The three-sampleNarrowAtLast science failure remains red.
E404's requested-order error/fallback conflict,E381's incomplete frame list,E374's
format conjunction,and broader scientific/technical/current-build/NASA gates
remain open. The source path review is in ``state-unit-frame-provenance-v1.json``.

The next concrete implementation gap is E380:the original FRR2.2 enumerates
A1,TAI,UTC,TT andTDB,but the OEM writer fixes output toUTC. The exact original
rows and proposed implementation constraints are retained in
``E380-time-system-next-review-v1.json``. This review assigns no credit.

Evidence: ``build/nasa-coverage/coverage95-20261104-oem-frame-state``.
Replay without native execution::

   PYTHONDONTWRITEBYTECODE=1 python3 build/nasa-coverage/coverage95-20261104-oem-frame-state/verify-component-v1.py

Reusable registrations are the nine ``QtGui.NasaOemFrameState*`` cases. Execute
only new/affected cases with fresh matching source/build/image-map evidence when
further changes require runtime validation.
