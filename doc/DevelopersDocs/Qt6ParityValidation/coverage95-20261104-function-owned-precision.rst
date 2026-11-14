Code500 precision and temporal artifact custody
==============================================

Coverage remains **1,050/1,158 (90.7%)**, with 108 exact open rows and 51 more
needed for the unchanged1,101 target. Historical operations remain 1,816/2,367.
All 59 original hold identities survive:30 resolved,29 unresolved, including seven
MATLAB rows. No whole requirement is admitted. Scientific, technical core, full
current-build and NASA qualification remain false.

This increment closes the retained Code500 owned-type test in both profiles.
The complete36-type owned universe now has 33 source-versioned witnesses. The
remaining types are **Topocentric, STK, CCSDS-OEM**. FRR!E491 still requires those
types and the full E492-E500 resource/eligible Global conjunction;the 566 factory
roles and 321 Parameter-owner obligations are not reduced to these witnesses.

Repairs and evidence
--------------------

The actual default-case propagators and spacecraft use the existing non-precision
clock path at the default 1e-6 s stop tolerance. Diagnostics record their flags,
configured and armed epochs, the selected StartEpoch source and live clocks.
No StopTolerance, trajectory, case inventory or numerical bound is changed.

Code500 now decodes record calendar/seconds fields with GmatTime and supplies
relative seconds to the existing five-point not-a-knot spline. Both Real and
precise queries share this interpolation path. The Real overload preserves
representable endpoint identity:the rounded MJD may lie34.7 ns before the exact
file boundary. The precise overload keeps its supplied epoch. Precise
initialization and UpdateState also retain their selected epoch path. State
field reads use the existing endian-aware decoder in both clock modes.

The focused file matrix exposed two additional defects. The propagator used
the reader object's default output time system instead of its input header,
shifting A1 files by about 32 seconds. A new const, nonvirtual GetInputTimeSystem
accessor exposes the already decoded input indicator;GetTimeSystem's output
configuration behavior is preserved. Also, an unsigned ``size()-2`` expression
disabled the final stencil adjustment for a one-record file, causing out-of-range
access near the end of a50-state file. The signed comparison and final-node
rounding now keep the five-point stencil within its valid data.

Four newly pending core paths are preserved exactly:
``Code500Propagator.cpp/.hpp`` and ``Code500EphemerisFile.cpp/.hpp``. The pending
inventory is267 paths;all 263 earlier paths are unchanged. No data member, virtual
slot, eligibility flag, coordinate transform or spline formula changes. Exact
incremental pre/postimages, patch and complete HEAD-relative inventory remain
under ``core-inventory-v1`` and ``core-increment-v1.*`` in the stage.

Final GUI execution is source 10;the registered contract finishes at source 12.
The registration follows the default-on plugin option because plugin targets
are declared after src. Source 12 leaves every selected native image byte-identical
to source 10. The original Code500 function test plus six
affected original tests pass unattended and visibly:14 contexts. The latter
cover binary Toggle, reversal, filter/smoother Code500 output, and batch little/
big-endian output. The new function witness retains all three missions, repeated/
nested calls, Unicode reopen, no-Build replay, actual Report/ReportViewer, counter,
caller shadow, and source/history/configured-prototype/disk isolation checks.
It supplies six missions,24 callbacks and 144 independently checked state values.
All seven selected visible contexts start minimized, resizable, nonfullscreen:
110 native exposures. No screenshots or runtime/kernel copies are retained.

The new registered ``QtAcceptance.Code500EpochInterpolation`` runs the native
recorder and its independent auditor as one contract. A native recording alone
does not establish numerical success. The 24-file matrix covers A1/UTC, both byte
orders, linear/cubic states,9/50/112-point files, forward/reverse query ordering,
Real, precise and rounded-precise APIs. Of 1,104 queries,1,040 valid states pass
6,240 component checks at the original1e-7 km/1e-10 km/s bounds;64 precise queries
outside the encoded span are rejected. The auditor solves22 distinct systems
of 16 cubic equations independently using 60-digit Decimal arithmetic. Raw day,
integer-second and fractional-second components avoid classifying decimal MJD
serialization noise as physical extrapolation. This is a finite matrix, not
general leap, format, provider or scientific qualification.

All 43 original native require calls are textually preserved. The complete32-case
catalog and original1e-7 km position,1e-10 km/s velocity and 1e-6 s stop bounds remain
identical across all twelve source capsules. Of 1,062 previous test/helper paths,
1,059 are unchanged. Only owned-type diagnostics, runner source-review references
and the descriptive core inventory change;older test assertions are unchanged.
Nine plausibly damaged function/spline proofs are independently rejected.

Temporal-link correction
------------------------

The preceding seals incorrectly hashed thin runtime symlinks by dereferencing
their targets. A later utility-library build therefore breaks their literal
manifest replay even though no immutable regular artifact changes. The first
failed prior replay is retained;it is not reported as a successful frozen replay.

For the owned-type prefix, the new audit verifies6,876 regular artifact bindings
and inventories2,610 temporal aliases separately. For the environment prefix,
it verifies8,278 regular bindings and inventories1,485 aliases. Of the 4,095
aliases,2,184 have hashes corroborated by historical loaded-image records;the
other1,911 fixture aliases supply no execution evidence. No current target bytes
are substituted for old binaries, and a later seal hash never relabels an earlier
invocation. Actual source/build/process-map records retain their own versions.

The actual frozen contracts revalidate all 56 selected owned-type proofs and 30
affected original contexts from the preceding checkpoint. Their selected maps,
immutable proofs, JUnit, exposures, identities and independent arithmetic remain
valid as historical evidence. This is explicitly different from claiming that
the old dereferencing manifests now replay literally. The new seal hashes
regular artifacts and records symbolic-link routes separately, so rebuilding a
runtime target does not change the meaning of an immutable artifact binding.

Retained failures and remaining work
------------------------------------

The stage retains 11 GUI failures:the original four-type diagnostic failures in
both profiles, the first relative-time transition in both profiles, and a traced
boundary diagnosis. It also retains the support harness's missing-startup
failure, the A1-header failure,96 invalid standard-library exceptions recorded
before the final stencil repair, a no-matching-GUI-label preflight with no native
process, and two failed offline oracle attempts. The latter exposed Decimal
``0**0`` handling and rounded-MJD endpoint classification;the corrected auditor
uses a constant polynomial term and raw clock components. No failure is replaced
by a weaker bound or a constant trajectory. Eleven builds pass;source 11 retains a registration-order build failure and the
runner refusal that prevented stale-binary execution. Their source
capsules, logs and actual image hashes survive.

``remaining-requirement-audit-v1.json`` preserves every prior field, fingerprint,
gap and gate for all 108 exact rows and all 59 hold identities. The new matrix
preserves all 566 old rows/eligibility decisions and appends only the Code500
witness. The previous Code500 gap is retained as history in the three-type
worklist. Continue the Topocentric rebuild/serialization drift and the STK/OEM
file-time/interpolation paths before the full resource/Global join. Code500
closure does not award E491 or replace the full95percent objective.

Replay
------

Stage:``build/nasa-coverage/coverage95-20261104-function-owned-precision``.
Final audit:``evidence-audit-v2.json``. Seal:``evidence-manifest-v2.json``.
Offline helpers and documentation are a post-execution reviewed-source addendum;
the runtime code, test inputs and selected source 10 GUI/source 12 support execution remain distinct.
From the repository root, choose a fresh output path::

  python3 src/qtgui/tests/AuditFunctionOwnedPrecision.py \
    --repo /home/dan/GIT/GMAT-Qt \
    --manifest build/nasa-coverage/coverage95-20261104-function-owned-precision/evidence-manifest-v2.json \
    --output /tmp/gmat-code500-evidence-new.json

The frozen auditor and its sibling helper can be extracted from the final source
archive and run from another directory with explicit ``--repo`` and ``--stage``.
The audit never executes GMAT, overwrites evidence or grants requirement credit.
For the reusable native-plus-oracle contract::

  cmake --build build/linux-gui --target GmatCode500EpochInterpolationTests
  ctest --test-dir build/linux-gui -R '^QtAcceptance.Code500EpochInterpolation$' --output-on-failure

For affected GUI work, use the existing function-owned runner with a fresh output
and matching new source/build capsule;retain the initially minimized, resizable
visible profile. Reuse other source-versioned passes instead of rerunning them.
