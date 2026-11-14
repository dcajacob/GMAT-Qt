Function Antenna return ownership repair, 2026-11-08
===================================================

The broad current regression exposed a valid function returning an Antenna
that incorrectly failed with a circular dependency. Antenna inherits Imager's
explicit ownership provider. Its configured global field of view is borrowed,
so an empty owned-object list is meaningful. FunctionManager formerly treated
that empty list as missing metadata and fell back to reference enumeration.
This falsely made the global field of view a private child of the returned
Antenna, triggering a cycle and threatening shared-reference identity.

FunctionManager now honors an explicitly registered ownership provider even
when it reports no owned children. Existing fallback for objects without
ownership metadata remains. Actual cycle detection and native ownership,
copy, lifetime and transient-force guards remain. No object layout or virtual
slot, physical equation, original test, fixture or tolerance changes.

The unchanged Antenna case failed before and passes after the repair. Fifteen
original reference and owned-component controls pass in each of headless and
visible modes, including shared field-of-view pointer identity, prototype and
caller isolation, repeated calls, mission, source/history and external-file
behavior. Together with the repaired single-case run, this is 31 passing GUI
executions. The original native provider check passes too. Its first native
launch failed to connect to the display before assertions; that log is kept.
All 1,334 protected original inputs are unchanged.

The first full run remains a versioned partial result: 781 passed, 3 failed,
1 interrupted. It was deliberately stopped to repair this real defect. Its
other two failures and the interrupted attitude case remain visible; previous
passing cases are not claimed as verified against the newly changed library.
The complete current suite is rebuilt and rerun next without another user
prompt. Canonical acceptance remains 1078/1158 (93.1%); zero new row credit.
Full function/resource/science/platform requirements remain open.

Stage: build/nasa-coverage/coverage95-20261108-function-antenna-return-current-review
