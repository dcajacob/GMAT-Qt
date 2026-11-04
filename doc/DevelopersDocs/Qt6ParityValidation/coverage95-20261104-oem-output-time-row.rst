Exact E380 admission: all five OEM output clocks
===============================================

Coverage is **1,052/1,158**. Only original FRR!E380,FRR-31.4.2,fingerprint
``2b7f09868ac66ae59b18a3817b9db0cc5f9cb509da2cff15a82b918ab93ff232``,
is newly admitted.106 original requirements remain;49 more are needed for1101.
Historical1816/2367 and all59 original hold identities/statuses are unchanged.
The original37 MATLAB identities and false MATLAB bundle gate are retained.

E380 requires all systems listed in original FRR2.2:A1,TAI,UTC,TT andTDB.
OutputTimeSystem now selects actual OEM metadata and data epochs in each system;
EpochFormat still interprets input bounds. This resolves the earlier missing
capability through a distinct feature,while preserving the old deferral record
and UTC default. It does not reinterpret input conversion as output support.

The complete conjunction joins32 five-clock GUI matrix passes,four affected UTC
control passes,two committed-format recovery passes,and native support for all
five systems. Independent calendars include the2016 leap second and a TDB/TT
separation visible at millisecond precision. Every output state retains its
original numeric bounds. A1 has an explicit forward private interface definition;
the generic oracle's A1 gap is retained. Full evidence and source/core deltas are
in the `five-clock component <coverage95-20261104-oem-output-time.rst>`_.

The specialized admission reader rejects nine damaged scope models,including a
missing A1 system,missing TDB source context,input-only substitution,incomplete
epoch evidence,extra parent credit,retrospective A1 agreement,waived scientific
failure,changed fingerprint and full-NASA overclaim. The ledger replays byte-for-
byte with58 frozen admission readers and no native reruns. The ledger SHA256 is
``f8dfb5a9f820337c7a15b6f5b838efad1e9abc6166f64dbadf5a59872bef0c3e``.

All1158 requirement records are audited. Exactly E380 changes status;the other
1157 full records and138 of139 closure records are unchanged. The remaining106
records equal the previous107 minus E380. All59 hold keys/statuses and37 MATLAB
identities are identical. The old E380 closure is preserved verbatim under
``pre_admission_closure_record``.

No extra production change is introduced by admission. The component's286 pending
core paths remain exact. The new dispatch and frozen reader add evidence handling
only. Source-specific reuse does not qualify a full current build,Windows,
external ABI,arbitrary scientific domains or the entire NASA campaign.
NarrowAtLast remains red;E404,E381 and the E374/E375/E377/E406 conjunctions remain
open. The95percent goal remains active.

Canonical evidence is under
``build/nasa-coverage/coverage95-20261104-oem-output-time-row``.
Reuse ``sealed-current-prefix-v2.json`` for the1052 union;the file
``sealed-prior-prefix-v1.json`` remains the immutable1051 build baseline.
Replay without native execution::

   PYTHONDONTWRITEBYTECODE=1 python3 build/nasa-coverage/coverage95-20261104-oem-output-time-row/verify-stage-v2.py

Use ``evidence-manifest-v2.json``,``stage-receipt-v2.json`` and
``seal-verification-v2.json``. V2 directly binds all1592 component artifacts,for
1642 total bindings. Earlier descriptor-only bindings and the first replay helper
preflight failure are retained. The v2 current prefix replays with58 frozen
readers;the original1051 build baseline is unchanged.
