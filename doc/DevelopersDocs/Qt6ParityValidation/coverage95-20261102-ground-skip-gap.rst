95 percent campaign: GroundTrack sampling counterexample
========================================================

Coverage remains **998/1,158 requirements (86.2%)** and **1,816/2,367 historical
scopes (76.7%)**. No new requirement is admitted. Original FRR!E634 asks for the
number of integration steps to skip between plotted points; FRR!E633 depends
on that row plus already admitted redraw/retention children E635/E636.

A proposed direct skip-count GUI translated N to the existing sampling interval
N+1. Four counts passed single-command missions, typed lifecycle/history/reopen
checks and independent circular-state/publication checks in both delivery modes.
Four affected unchanged regressions also passed. Those results were insufficient
for the original requirement: the collector counts publications, including a
non-step repeated initial state at a new Propagate command.

The independent implementation review rejected admission. A new visible native
diagnostic confirms its source-derived counterexample with two fixed-step
Propagate commands split after the first ten-second step::

   publication  integration step  collected
   1            0                 yes
   2            1                 no
   3            1                 yes

The configured interval is2, corresponding to one requested skipped integration
step. Collected integration steps0 and1 have zero skipped steps between them.
All three raw six-component states agree with the independent circular oracle;
the failure is sampling identity, not an invented numerical payload or orbit
truth discrepancy. The runtime writes publications.json and violation.json
before stopping at the intact assertion.

The misleading experimental control is withdrawn. GroundTrackDialog.cpp is
restored exactly to its pre-experiment hash, every previously tracked core path
is unchanged, and no new sampling mode or duplicate suppression is introduced.
Both E633 and E634 remain unqualified, as do the separate OrbitView E329/E330
initial-phase conflicts. No original assertion, source wording, denominator or
qualification gate is weakened to obtain a percentage increase.

Registered failing diagnostic
-------------------------------

QtGui.NasaGroundIntegrationSkipBoundary is registered with qt-gui and
qt-nasa-known-gap labels. Its assertion is enabled; no WILL_FAIL inversion or
passing reinterpretation is applied. A comprehensive GUI run therefore reports
this unresolved source requirement as a failure. It is a qualification gap in
existing sampling behavior, not a regression caused by a retained GUI change.

To watch the diagnostic, choose a fresh output directory and restore the
initially minimized, resizable, non-fullscreen test desktop::

   python src/qtgui/tests/RunAcceptance.py \
     --build-dir build/linux-gui \
     --filter '^QtGui.NasaGroundIntegrationSkipBoundary$' \
     --visible --display-route nested --display-scale auto --start-minimized \
     --pause-ms 350 --max-checkpoints 200 --max-screenshots 0 \
     --output build/gui-acceptance/ground-integration-boundary-watch

It currently fails with the recorded integration-step gap. Native xcb window
exposure is verified inside the available nested desktop. Host/compositor
observation is unqualified; the user-requested minimized host is not silently
raised and no claim that a person watched the host display is made.

Why a blanket deduplication is unsafe
--------------------------------------

The existing cadence test explicitly checks publication-ordinal collection.
Solver-publication tests additionally preserve raw publication order/membership.
Equal numeric packets can differ in provider, labels, colors, solver status,
precise epoch context or partial spacecraft availability. Suppressing all equal
epoch/payload packets would not be a demonstrated compatible repair.

A future integration-step implementation needs trustworthy step identity and a
defined treatment of command-initial states, same-epoch changes, solver trials,
multiple providers and metadata-only updates. It must preserve the existing
cadence and solver assertions and add boundary witnesses. No speculative core
feature is installed solely to obtain two coverage rows.

Preserved experiment and harness repair
-----------------------------------------

Seventeen exploratory acceptance attempts are retained: eight selected
single-segment contexts, four affected regression passes, four misrouted passes
excluded from visible credit, and one initial launcher failure. A further visible
boundary execution provides the genuine failing diagnostic. One diagnostic build
also needed an explicit shared-pointer-to-bool conversion; both build logs remain.

The first pilot correctly rejected offscreen execution; registration was changed
to the existing native X11 runner. A later requested visible run was incorrectly
classified as helper work. RunAcceptance now recognizes native-wrapper widget
workflows while retaining explicit offscreen guards and non-widget support
classification. At that source version exactly the four experimental cases
changed routes; the other2,449 were unchanged. Corrected receipts contain native
exposure evidence. The routing repair remains useful for the new diagnostic.

The bounded experiment is explicitly unqualified and audited separately::

   python src/qtgui/tests/AuditGroundTrackSkipAcceptance.py \
     --evidence build/nasa-coverage/coverage95-20261101/ground-skip-experiment-evidence.json

Its qualified_keys list is empty. The old proposed admission is retained under
an explicitly withdrawn filename, the exploratory sealer refuses to regenerate
it, and the coverage builder has no admission route for this experiment. Twelve
earlier corruption controls only validated the narrow contract; they do not
establish whole-row sufficiency and are superseded by the independent review.

Storage and remaining work
----------------------------

All actual receipts, source archives and the failing diagnostic are preserved.
No passing screenshots, runtime copies or existing kernel copies are retained.
Lossless compaction saved46,596,096 allocated bytes (44.4MiB), and removal of only
the obsolete experimental target's outputs recovered7,733,248 bytes (7.4MiB).
The live diagnostic binary remains available.

Rebuilding the earlier998-row union exposed a separate bookkeeping defect:
two older qualifications hashed a mutable audit-helper path. Exact historical
helper bytes were recovered from each qualification's hash-bound source archive.
Separate replay capsules relocate that review artifact without changing its
expected hash. A derived inertial capsule references the equivalent frame-state
replay document; the original dependency and hash remain explicitly preserved.
Independent review confirms unchanged scopes, native receipts and assertions.

All thirteen additions now replay successfully, with zero native reruns and no
new credit. The canonical report changes only six qualification metadata
references; all original rows, counts and remaining obligations are identical.
The original998 report and qualifications remain frozen separately. Replay still
imports the current helper: its only changed behavior projects passing cases
from mixed visible receipts, and these visible receipts have no failed cases,
so the effective checks are unchanged. Both helper versions and replay provenance
are retained; no expected source/native digest was replaced to accept new bytes.

The next finite candidate is the C-API's deferred spacecraft epoch getter,
All Others!E272, and its three-child post-run access parent E269. Cartesian and
STM children already have admitted native-client evidence. This requires review
of a real public ABI contract and independent epoch results; neither a private
probe nor a GUI/metadata substitute supplies that missing capability.

The 95% goal remains open, with103 complete requirements still needed. All59
prerequisite holds remain unresolved; their optimistic non-held ceiling is
1,099/1,158 (94.9%), so at least two held obligations need substantive resolution.
Private NASA, historical platform, independent scientific/core approval and
complete-current-build gates remain open.
