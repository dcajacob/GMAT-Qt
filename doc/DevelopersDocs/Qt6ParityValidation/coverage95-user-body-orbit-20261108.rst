User-body orbit acceptance, 2026-11-08
=====================================

Eleven original requirements are admitted: FRR!E220, E228 through E236, and
E238. Verified coverage is 1065/1158 (92.0%), with 93 original gaps and 36 more
needed for the unchanged 1101-row target. The historical operation union
remains 1816/2367. The campaign remains active.

Implemented behavior and repair
-------------------------------

All four original user-body types support the complete orbital-property and
TwoBody/SPICE mode conjunction. The GUI edits InitialEpoch, SMA, ECC, INC,
RAAN, AOP and TA, preserves drafts through mode changes, rejects seven invalid
inputs atomically, and supports Apply, Cancel/Discard, Undo/Redo and Unicode
save/reload. Existing SPICE kernel selection and central-body creation proofs
are reused with fresh unchanged regressions. Returning to committed SPICE
restores the original kernel state and a completed witness mission.

Verification found a real time-interface mismatch: the A1Mjd TwoBody wrapper
composed its parent state through GmatTime while ordinary A1 callers obtained
the parent through the Real backend. Sun-centered relative states consequently
had a 2.7 mm discrepancy. CelestialBody.cpp now composes the relative orbit
with the parent's matching A1Mjd interface. The precision-time overload,
propagation equations, numerical tolerances, class layout and virtual API are
unchanged. The diagnostic failure and source preimage are retained.

Evidence and limits
-------------------

Both unattended and visible profiles pass all 14 selected registered checks:
five new body cases and nine unchanged original SPICE/parent-creation checks.
This is 28 passes on identical recorded source/runtime hashes. The visible
Xephyr desktop starts minimized, is resizable and records 200 native exposures.
Qt widget events and exposure do not establish native OS input or other-platform
qualification.

Independent Cartesian RK4, perifocal rotation and closed circular solutions
check elliptic, hyperbolic and circular cases, forward/backward propagation,
long circular spans, both time interfaces, acceleration, and one nested parent.
The frozen admission reader independently recalculates 130 queries, 260 state
vectors and 1560 Cartesian scalar comparisons. Maximum position and velocity
errors are 1.058e-6 km and 3.064e-10 km/s, within the unchanged 2e-6 km and
2e-9 km/s limits. These bounded cases do not approve arbitrary physical models,
frames, scientific/core behavior or the full current build.

The new test's initial compile error and the pre-repair A1 failures are retained.
The original 1334 protected inputs remain unchanged. The exact delta changes
only eleven records in the full 1158-row audit and the 139-row closure overlay;
all other 1147 requirement records and 128 closure records remain identical.
All 59 original hold identities remain: these eleven implemented restorations
bring the resolved count to 42, leaving 17 unresolved. All 37 MATLAB identities
and six unresolved MATLAB holds remain unchanged. No administrative retirement,
assertion changes or gate relaxation contributes credit.

Canonical evidence
-------------------

The native component publication is
``build/nasa-coverage/coverage95-20261108-user-body-orbit/publication-v1.json``.
The row admission stage is
``build/nasa-coverage/coverage95-20261108-user-body-orbit-row``.
Use its qualification, row-admission audit, exact-delta audit, canonical ledger,
manifest, receipt and sealed-current-prefix files. The 65-reader frozen ledger
replay reproduces the canonical ledger byte for byte. Replay the sealed stage
with ``verify-stage-v1.py --output`` and a fresh output path; no native tests
are rerun. Source archives include all protected inputs and the executed
revisions/preimages. No runtime binary copies are added.

Full NASA, scientific/core, external ABI, physical-frame and platform gates
remain open. Engineering presence remains the carried 1093/1158 (94.39%), a
different measure from acceptance. Next research the original orientation
labels, especially the undefined rotation drift rate, using historical NASA
GUI/documentation and IAU conventions. Do not invent spin acceleration or
repeat completed body, interpolation, function, command or plot matrices.
