CCSDS OEM format conjunction, 2026-11-11
======================================

FRR!E375 and its original two-format parent FRR!E374 are admitted using
source-versioned evidence. Coverage is 1,106/1,158 (95.5%), with 52 original
rows open. Five of the 18 agreed rows are complete; 13 remain. The denominator,
all 1,334 protected inputs, original numerical bounds and failure statuses are
preserved. This is not whole-current-build or scientific qualification.

Two demonstrated writer defects
------------------------------

The frozen ten-case supplementary witness initially passes five cases and fails
five. A second metadata block can change OBJECT_ID within an OEM1.0 file; an
in-place metadata rewrite can also replace its identity. Both violate the single
object contract in CCSDS502.0-B-1 section4.1. Separately, direct writer callers
can enable OEM2 acceleration/covariance fields in a file declaring OEM1.0. The
ordinary EphemerisFile resource already rejects those options, but the writer
boundary did not enforce the six-component B1 representation.

Two production owners change. The private chronology registry now previews a
metadata update and checks the file's object identity before writing bytes. It
compares text using B1 case, underscore and repeated-blank equivalence without
changing the serialized spelling. Reopen and clone start independent histories.
WriteDataSegment rejects OEM2 extensions in version1.0 before writing or consuming
buffered records. Public APIs, instance layouts, native equations, time values
and OEM2 output remain unchanged.

The unchanged frozen witness passes all ten cases after repair. Its five
rejections verify unchanged bytes, stream position and buffered record count,
then recover valid output and independently check every serialized state and
epoch. A combined OEM2 acceleration/covariance control checks its original
values and layout. The directly affected original chronology worker passes all
13 cases. Its eight positive files retain independent B1 checks; the optional
interpolation omission retains its explicit profile gap. No unrelated native
suite or full regression is run. An initial private-display connection failure
occurred before that worker's cases and remains a failed startup receipt; the
same isolated selection subsequently passed.

Complete format evidence and limits
-----------------------------------

The exact September2004 standard remains SHA256
fab306a994e5bce9cad8c64d77af172a5819af38faf459faab1b7e2f6163223c.
The clause review covers sections4.1,4.2/Tables4-1..3 and4.3.1..7. Saved text,
UTC creation, five output-time systems, independent six-state/unit/frame
reconstruction, explicit windows and record-count evidence are reused with
their actual executed sources. Five frozen component readers replay without
native execution. Retained post-chronology files also receive independent
format checks: nine have complete declared test interfaces, while four old
solver files retain the SATID ICD gap. No operational interface agreement is
invented.

Earlier planning bundled several distinct gates under the format row. The
explicit row-scope review now reconciles them against the original worksheet
and pinned standard. B1 section1.2 places underlying orbit accuracy outside
the format specification. NarrowAtLast therefore remains a scientific failure:
2.6942092880233304e-5km and2.9043781227255927e-8km/s exceed its unchanged
2e-5km and2e-8km/s bounds. Its two exported records correctly recommend degree1;
that format result neither fixes nor relabels the accuracy failure. No sampling,
clock, interpolation method, assertion or tolerance changes.

The unfinished coordinate enumeration FRR!E381 and option parent E377 remain
open. B1 provides examples rather than an exhaustive frame list. The format
admission establishes the complete mandatory wire clauses using declared
interfaces and independently reconstructed states, without asserting arbitrary
frame, force-model, physical-domain or platform qualification. Previously
admitted E380 and E404 provide the time-system and mandatory interpolation-error
policy conjunctions. Parent E374 lists exactly OEM and SPK; it conjoins the
new complete E375 result with already accepted E376 SPK evidence.

Evidence and immutable admission
--------------------------------

Artifacts are in
``build/nasa-coverage/complete18-20261111-oem-file-invariants-v1``. Use
``qualification-v3.json``, ``coverage-snapshot-v1.json`` and
``sealed-current-prefix-v1.json``. Earlier prepared qualifications and reader
startup failures remain superseded diagnostics. Eight evidence/admission
negative controls reject missing cases, wrong states, lost atomicity, leaked
extensions, changed fingerprints, a missing SPK child, an omitted mandatory
clause and a relabelled original accuracy failure.

Only the two admitted original records change. The other1,156 requirement
records, all2,367 historical-case records and the original hold identities and
statuses are preserved. Ledger replay launches zero native tests. Its explicit
historical artifact view supplies the exact previously sealed generated CTest
registration bytes; it does not replace the live build registration or weaken
an expected digest. The review package archive is unchanged.
