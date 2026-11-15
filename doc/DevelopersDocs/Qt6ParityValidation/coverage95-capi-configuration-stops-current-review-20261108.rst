C API configuration and stops current review, 2026-11-08
==========================================================

LoadScript uses the native interpreter and command factories; RunScript uses
Moderator::RunMission. PrepareMissionSequence retains preparation-only behavior,
while BeginMissionSequence executes the configured commands. Runtime model and
epoch observations are invalidated before operations that can replace the
sandbox. Empty-name FindOdeModel returns native success status zero; named
queries return model IDs. No new ID alias or independent C-only propagator is
introduced.

The current unchanged original workers pass28 cases:8 C API propagation/graph
cases,13 SPK/numerical discovery/count-policy cases and7 scalar-stop clock,
wrapper and reinitialization cases. They exercise prepared/completed missions,
branches, backward propagation, scalar goals/measured operands, stopped runs,
ephemeris policies, formations/multiple clocks and stale angle/day metadata.
Their native and independent numerical assertions, image identity and source/
fixture guards all pass. These are native C/C++ checks, not GUI executions.

The first28 runs are retained as failures. Their native assertions passed, but
the surrounding original image-inventory guard rejected VF13ad because the
startup loaded its unchanged provider outside the regular runtime-file scan.
The correction hard-links the same provider inode into the private runtime and
uses a separate startup to select it. No provider bytes are copied or changed;
its SHA remains27ecf054f2ded98a5ed97cd344f203f214cfcab9e8f148298a08bbccb7d1106d.
The original startup, tests, assertions, fixtures and tolerances are preserved.
The rerun passes the complete original inventory guard, with before/after
bindings for34 runtime images. The setup receipt records both hard links and
the sole startup-line difference.

All1,334 protected original inputs remain unchanged. There is no product edit
or acceptance credit. Full dynamics/FRR13 (including Mars-GRAM), all FRC10
parameter/domain/scientific conjunctions and historical/native-platform gates
remain open. Full56-row context is retained; the adjacent spacecraft and access
parents are not replaced by the selected propagation examples.

Stage: build/nasa-coverage/coverage95-20261108-capi-configuration-stops-current-review
Continue directly to the historical/undefined source dispositions and then the
known reproducible OEM endpoint failure. Canonical acceptance remains1078/1158.
