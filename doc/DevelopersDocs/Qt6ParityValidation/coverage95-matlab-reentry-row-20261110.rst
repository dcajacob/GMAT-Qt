MATLAB-to-GMAT function restriction — 2026-11-10
===============================================

The candidate stage ``coverage95-20261110-matlab-reentry-row-v2`` adds only
FRR!E479 to the sealed 1,084-row prefix. The original wording/fingerprint remain.
Successful sealing and replay establish 1,085/1,158 (93.7%), with 73 open and
16 still needed. Only this row's formal hold changes: 43 of the original 59
hold identities resolve; 16 remain, including five of 37 original MATLAB holds.

An actual licensed R2026b function sent a valid numeric callback request to the
active GMAT mission through the real authenticated Qt loopback connection. It
received the precise GMAT-called MATLAB restriction diagnostic before any
callback write or dispatch. The MATLAB result was 3*4+2 = 14, the next GMAT
statement reached 42, and a normal callback dispatch succeeded after return.
The actual MATLAB engine and GMAT plugin library identities are retained.

Two saved native guard runs cover the common callback and script-control
operations, nested unwinding, cross-thread receipt/dispatch and normal recovery.
The source binding shows real engine evaluation enters the same scope, and the
legacy socket receipt and native operations enforce it before side effects.
This proves the reentry restriction through the supported integration; it does
not restrict independently launched external programs or qualify the whole
MATLAB interface and Windows shared/private process identity.

Both initial vendor startup failures are retained. The successful run used the
normal installed provider connection with a private preference directory, then
removed generated preference scratch. The first offline reader missed CTest's
numeric log prefix; the corrected reader matches the same logged proof without
rerunning any native test. All 1,334 protected input preimages, prior failure
receipts, denominator and unrelated records remain. No full regression is run.
