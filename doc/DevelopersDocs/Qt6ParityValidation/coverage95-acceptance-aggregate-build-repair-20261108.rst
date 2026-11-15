Acceptance aggregate build repair, 2026-11-08
================================================

The complete acceptance-consumer build succeeded, but preflight found three
registered GUI test executable paths missing from both disk and the aggregate
dependency graph: GmatQtSpkEpochOracle, GmatQtNasaMatlabCInterfaceTests and
GmatQtMcpBridgeTests. Existing executables alone were not accepted as proof:
the Ninja dependency closure was checked for all514 registered executable paths.

The aggregate originally collected only already-declared GmatQt*Tests targets.
That omitted the differently named SPK oracle and the two later declarations.
CMake now explicitly includes the oracle in build-qt-acceptance, connects the
MATLAB C-interface test to check-qt/build-qt-acceptance as well as its older
optional aggregate names, and connects the MCP bridge test to both aggregates.
No original test, assertion, fixture, tolerance or runtime API is changed.

The MATLAB wrapper's unchanged identity guard rejected an older cached wrapper
from a different build. A fresh private wrapper directory was configured;
the old wrapper remains untouched. Generation with licensed MATLAB succeeds.
This guard failure and all initial build/preflight outputs are retained.

The repaired full build passes. Every one of514 registered executable paths
now exists and occurs in the aggregate dependency graph. The three unchanged
registered tests all pass: SPK epoch input oracle, real MATLAB C-interface and
MCP bridge. All4,341 archived source inputs are unchanged except the documented
CMake repair, and all1,334 protected original test inputs remain unchanged.

This is implementation/integration progress, not new NASA acceptance credit.
Canonical1078/1158 and all unresolved provider/scientific/platform/definition
requirements remain open. Continue immediately with the broad current GUI
regression run using execution-review-v2.json (zero claims), retaining every
failure and unmet prerequisite. The full run must be observed to terminal
before editing its owning sources. Its handle record is a route, not proof
that a process is still alive.

Stage: build/nasa-coverage/coverage95-20261108-full-current-feature-regression
