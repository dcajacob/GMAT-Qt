SPK origin settings acceptance, 2026-11-08
==========================================

Exact original FRR!E303/E305 are admitted with the complete original ephemeris-
propagator settings list. Coverage is1075/1158 (92.8%), with83 original gaps and26
more needed for the unchanged1101 target. Historical operations remain1816/2367.
This is recorded source-version reuse; full current-build, NASA, scientific/core,
physical-frame, external-ABI, platform and native-OS-input approvals remain open.

Implementation and verification
-------------------------------

The core SPK origin setter and translation paths already worked. PropagatorSettings
still made CentralBody read-only and omitted it from pending values. The correction
retains that value, enables its existing editor and replaces the stale ignored-
origin tooltip. The default Earth origin and core validation remain unchanged.

The actual resource-tree editor passes3 origins (Earth, Mars and the existing
user-defined Probe) by3 epoch modes (spacecraft-inherited, explicit TDB and kernel
start), in both profiles. A changed75s step produces distinct final offsets195,
315 and75 TDB seconds. Tests cover pending type-switch/Discard, absent/non-body/
Moon-alias invalid-origin atomicity and diagnostics, Apply, exact Undo/Redo, Unicode
save/reopen, nested/direct clones and the real Propagate-owned runtime object.

There are18 new GUI case passes and2 unchanged numerical-family regression passes,
with4 retained original readonly-origin conflicts. The passed headless pilot is
reused alongside the remaining8 cases on identical sources/binaries. All3 run
inventories match; the1441-source/33-ELF before/after guard also matches. Visible
Xephyr starts minimized/resizable, with121 exposure
events and no screenshots. These are Qt widget events, not native OS input proof.

The reader independently recalculates18 nominal affine ICRF vectors (108 scalars)
from literal prescribed SPK knots. Worst measured position/velocity errors are
1.6763795e-07km and1.99840144e-15km/s,
inside the unchanged new-case2e-5km/2e-9km/s bounds. Raw CSV, reported epoch modes,
steps, actual control traces, native owner identity and retained input/output
artifacts are checked. Existing independently qualified step/start-epoch children
E304/E306/E307/E308 retain their recorded scope and receive no duplicate credit.

Internal origin translation adds108 same-frame FK5 coherence comparisons and12
observable non-Earth effects. Shared native body offsets support that coherence;
they are not an independent physical ephemeris or frame oracle. Arbitrary kernel
coverage/quality/directions, all bodies/frames/time standards and general scientific
accuracy are not inferred from the bounded affine witness.

Preserved failures and scope
----------------------------

Original NasaSpkDraft Create/Edit require the SPK origin to be read-only and its
changes rejected. Both fail unchanged in each profile (CTest8/native child1).
They are retained, not waived or reported passing. The original numerical-family
creation case passes unchanged in both profiles. All1334 protected original
inputs, assertions, fixtures and tolerances still match their baseline hashes.

Three earlier new-witness failures are retained. Their independent ICRF report
already passed; the added raw-vector comparison incorrectly used ICRF truth for
FK5 internal coordinates. Trying precise epochs ruled out the first hypothesis.
SpiceOrbitKernelReader source establishes the ICRF-to-FK5 rotation. Only the new
check was corrected to compare the native FK5 relative/J2000 vectors and shared
precise body offset; no product code or numerical limit changed for that repair.

All59 original formal holds retain identities/statuses (42 resolved/17 unresolved),
all37 MATLAB identities and6 unresolved MATLAB holds remain. These2 settings rows
are outside that hold inventory. Only their2 full-audit/closure records change;
other1156 requirement and137 closure records remain identical. Source text,
fingerprints,1158/2367 denominators and all broader qualification gates are retained.

Native component: ``build/nasa-coverage/coverage95-20261108-spk-origin``.
Canonical admission: ``build/nasa-coverage/coverage95-20261108-spk-origin-row-v1``.
The latter contains the exact conjunction,69 frozen readers, source archive,
ledger/delta, byte-identical reader replay, manifest/receipt and sealed prefix.
No additional native execution is needed for admission/replay.

The campaign Goal remains active. Next verify the already implemented worksheet-
default Save filename rule (FRC!E135), retaining LegacyData compatibility and all
original legacy-default assertion conflicts. Do not promote selected serialization
examples to the separate universal any-Resource Save requirement FRC!E132.
