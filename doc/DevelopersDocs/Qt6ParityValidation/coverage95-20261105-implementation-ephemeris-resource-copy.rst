Ephemeris resource copy lifecycle
================================

Implementation-first repair in EphemerisFile.cpp only. A copied EphemerisFile
has no writer, but previously inherited isInitialized/isEphemFileOpened and
terminal flags from its source. Initialize could then return immediately
without creating a writer. Assignment also discarded its owned writer pointer
without finishing or deleting it.

Copies now have detached writer/terminal flags, fresh segment and maneuver
history, no stale publication packet/provider identity, and an initialized
fileType value. Their next Initialize performs normal setup. Assignment sends
the destination writer through its existing terminal path, deletes it after
successful completion, and only then copies source configuration. A flush
exception retains the destination writer and occurs before source configuration
is copied. Self-assignment remains a no-op. Copy rejects null/incompatible
sources before accessing them or touching the destination writer.

Inherited publisher context, including solver run state, maneuver state and
propagation direction, is preserved. It is distinct from the detached output's
segment history and flags. V2 retains that context; v1 was compiled before this
refinement and is not the final runtime check version. No generic Subscriber or
internal writer-copy policy, header, layout, virtual slot, numerical algorithm,
clock, interpolation order or tolerance was changed.

No atomic rollback of externally written output is promised. Concurrent outputs
sharing a filename and complete initialized/derived-resource copy domains are
not qualified by this repair. It does not alter the GUI's clone eligibility or
promote the existing EphemerisFile copy_contract=false fixture flag.

Build and reused checks
-----------------------

Stage: build/nasa-coverage/coverage95-20261105-implementation-ephemeris-resource-copy.
Exact preimage and incremental-v1/v2.patch retained. Final patch SHA256:
``f48201fdcc7928906256265732fd217453190775dc2def060408ace6ad7fcc7a``.
Private builds pass in 4.08/3.53 seconds with no warnings/errors. The matched
integrated-current runtime is updated; summary.json records final image hashes.
Seven monitored installed images and public code/releases remain untouched.

Existing EphemerisTests and NasaFunctionRemainingResourceTests sources compile
unchanged against the private current frontend/core. Prior unchanged SPK output
and interrupted-ephemeris executables are reused with unchanged headers/ABI.
Four existing native GUI checks pass against final v2:

* EphemerisTests: all five original variants, CCSDS-OEM, STK-TimePosVel,
  Code-500 little/big endian and SPK, retaining configuration/history/path,
  directory/error recovery, plugin readback and analytic-circle assertions.
* Function remaining-resource EphemerisFile: its original main/private/Global
  creation, selected field, identity and repeated-call assertions.
* SPK Degree7: original 34-knot output, raw-state/clock/reader checks and
  continuation sentinel.
* NestedDC runtime error: original failed-trial output exclusion and two
  successful 25-row recoveries.

These cover affected configuration clones, initialized runtime delivery and
normal/error recovery. They do not directly inject active assignment, invalid
Copy inputs or destination flush failures. Those focused checks remain deferred.
The legacy five-format executable emits its original five PASS lines rather
than a NASA_CHECK receipt; those lines and raw logs are retained, without
inventing raw export payloads that the executable does not save.

Native xcb uses a resizable nested desktop that starts minimized. Zero
screenshots; private Base mapped, old installed Base absent; all handles
terminal. Compiler commands, per-run command.json files and the existing
startup/display helpers provide replay with new output directories. No new
tests, fixtures, registrations, matrices, readers or seals were added, and no
original assertion changed.

No original row admitted. Verified 1053/1158, 105 open and 48 needed for 1101.
All 59 prerequisite identities and separate historical 1816/2367 coverage remain
intact. Full resource/scoping/Save and current-build/science/platform gates remain
open. Goal active/incomplete; continue supported implementation gaps before new
test expansion.
