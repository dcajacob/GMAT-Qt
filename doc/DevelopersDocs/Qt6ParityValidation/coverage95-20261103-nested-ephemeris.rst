Nested final-solution ephemeris repair
=====================================

Two new native GUI cases reproduced a core defect: an inner Target's accepted
pass was written into OEM output while its enclosing Target/Optimize was still
iterating. Before repair, NestedDC wrote 175 knots with radii 7030..7230 km and
NestedYukon wrote 425 with radii 7029.99..7230.01 km. Each should contain only the
single globally accepted 7230 km trajectory: 25 knots at 0:5:120 seconds.

Both unchanged cases now pass unattended and visibly, as do four affected
regressions in each mode: flat Target/Optimize OEM output and Nested/Mixed live
plot membership. This gives four new GUI passes and eight regression passes.
The independent audit checks all 100 new OEM knots/600 Cartesian components,
112 raw trial result rows and 2,800 raw state publications across both modes.
There are 18 nested passes per DC run and 38 per Yukon run, with respectively
six and sixteen locally accepted inner passes that are still outer trials.
Exactly one pass per mission has both numerical solver clones FINISHED.

The four-file production delta is limited to EphemerisFile.cpp/.hpp and
SolverBranchCommand.cpp/.hpp. EphemerisFile now opts into nested subscriber-state
classification. The existing plot entry point delegates to the shared helper;
the classifier's existing active-solver scan is unchanged. Publisher state,
report routing, numerical solver equations and physical state propagation are
unchanged. The change affects the common ephemeris subscriber gate for formats;
new numerical qualification here is limited to the two OEM cases.

The stage ``build/nasa-coverage/coverage95-20261103-nested-ephemeris`` preserves
exact four-file preimages, core-v3.patch, pre-edit core image hashes, three source
versions and all failed outcomes. The first two attempts used an offscreen
registration incompatible with the retained Rig assertion; v2 changes only the
registration to native xcb. Both v2 cases then fail the original final-trajectory
assertion. v3 fixes the core without changing any test assertion or fixture.
The old core image hashes were captured after terminal baseline execution and
before mutation; this is not a claim of baseline process-map capture.

Source-v3 reviews/archives, executed-source-v3, before/after identities and all
six visible process maps bind the current native execution. Visible windows
were initially minimized, resizable and nonfullscreen; screenshots=0. All runs
are terminal. Only two new cases and four affected regressions were selected.

Audit v2 is authoritative. It binds 288 regular files, checks exact source/core
deltas, runtime identities, fixture origins, native/JUnit/stdout/proof outcomes,
independent circle and scalar results, full trial/publication partitions and
OEM time/frame metadata. The initial audit v1 incorrectly compared padded,
case-preserving metadata text to unpadded uppercase strings; its source/log and
manifest remain. v2 parses allowed whitespace/case while preserving field values
and every numerical assertion. Four negative controls reject trial output,
a missing knot, incorrect solver acceptance and a missing nested partition.

The orbit oracle uses literal mu=398600.4415, with each trial's radius independently
derived as 7000+100*U+10*V from raw control reports. Every 120-second trial result
and every 5-second raw publication is checked. The accepted values U=2,V=3 give
radius=7230. Bounds remain 2e-5 km and 2e-8 km/s; OEM epoch/schedule tolerance is
0.002 seconds. No new general force, arbitrary orbit, estimator/filter/smoother,
non-OEM, failure/Stop/recovery or scientific approval follows from this repair.

Build and replay from the repository root::

   cmake --build build/linux-gui --target GmatQtNasaNestedEphemerisTests \
     GmatQtNasaSolverEphemerisTests GmatQtNasaSolverPublicationTests -j 4
   python3 src/qtgui/tests/RunNestedEphemeris.py
   python3 src/qtgui/tests/RunNestedEphemeris.py --visible
   python3 src/qtgui/tests/AuditNestedEphemeris.py \
     --stage build/nasa-coverage/coverage95-20261103-nested-ephemeris \
     --output /tmp/nested-ephemeris-recheck.json

The two new ordinary registrations are QtGui.NasaNestedEphemerisNestedDC and
QtGui.NasaNestedEphemerisNestedYukon. The target is part of acceptance-build
prerequisites. Replays capture current source identities into unique output
folders and do not overwrite this source-versioned evidence.

No exact requirement key is assigned. FRR!E406 remains open for the complete
iterative-output obligation. Continue its failed/stopped/recovery cases and
remaining estimator/filter/smoother/format boundaries, reusing prior passing
proofs at their recorded source versions. Do not require unrelated full orbital
science qualification as a substitute for the output-membership scope, or use
this bounded repair to waive missing output regimes.

Coverage remains 1,019/1,158 (88.0%), 139 open and 82 needed. Historical coverage
remains 1,816/2,367 (76.7%). The denominator, fingerprints, 59 prerequisite holds
and scientific/core/current-build/full-NASA gates remain unchanged.
