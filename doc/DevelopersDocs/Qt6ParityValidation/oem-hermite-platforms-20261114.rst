OEM Hermite platform qualification
==================================

The five Hermite scenarios and the interpolation sufficiency policy passed on
Windows x64, macOS Apple Silicon and macOS Intel. Linux passes from the completed
targeted development check were reused. All runs retained the original export
accuracy bounds of 2e-5 km and 2e-8 km/s. No full regression was run.

Source and execution
--------------------

The Hermite implementation is commit 3089594492937d76919685ea60af8240c201e318.
The two Mac jobs built e4179175aed826b9ff79650371789c26a47ff03d in workflow
43665317617. Both passed six cases. Their GUI, core, station and navigation
bindings were compiled together using CPython 3.12.

The initial Windows build compiled successfully, then exposed a separate
Unicode OEM pathname defect. The CCSDS reader and writer still used Windows
narrow-string file operations. The equivalent eclipse/contact report defect
was already repaired for alpha.4 in EventLocator.cpp. The Windows repair in
27249d0b5d12ea156115bc577abcf1cf5a68324a uses native filesystem paths constructed
from UTF-8 for CCSDS output, existence checks and OEM input. These additions are
compiled only on Windows. The non-Windows source equivalence check is retained.

The original interpolation sufficiency test assumes LF immediately after the
OEM header. Windows produces CRLF. Its protected source remains byte-identical.
A supplementary executable is generated from it with only three header-check
inputs normalized from CRLF to LF. Its numerical assertions and tolerances are
identical. The original failed Windows receipts are retained beside the fixed
results.

Workflow 43669445676 built and tested Windows only. All six cases passed. The
five Hermite tests also retain Unicode filename creation, read-back, save/reopen,
GUI Apply and undo/redo coverage. The sparse NarrowAtLast maximum component
errors were 2.6131070285e-7 km and 1.1043243653e-8 km/s on Windows and Intel Mac.
Apple Silicon differed only at floating-point rounding scale.

The new workflow is .github/workflows/oem-hermite.yml. It uses the existing
release build helper and the six-case tools/preview/OemHermiteSuite.json.
RunSharedSuite.py can select a separate test plan and API-enabled build while
preserving its existing default suite. It saves per-case logs, numerical proofs,
fixture hashes and native build provenance.

Preserved evidence and limitations
----------------------------------

The companion directory contains the original and repaired Windows results,
both Mac results, source fingerprints, policy proofs and native-build archive
hashes. Retained compiled inputs permit subsequent packaging without compiling
these platforms again. They are build inputs, not distributable release archives.

All seventeen published alpha.4 assets retain their recorded size and digest.
The alpha.4 tag still points to a22ef7e636203b2ccfaba51deb7fac37f5cb5ffa. Existing
releases were preserved. The new fix is on codex/oem-hermite and has not been
merged into main or packaged into a new release.

The previously saved stale-panel QtGui.Ephemeris failure remains recorded.
The original Lagrange NarrowAtLast failure still applies when Lagrange is
selected. Passing Hermite evidence does not relabel that original failure.
Hosted offscreen tests exercise GUI transactions and native numerical output.
Hardware-specific graphics behavior was outside this targeted check.
