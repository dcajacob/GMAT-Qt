Record-number input units
=========================

AcceptFilter and RejectFilter store RecordNumbers as string arrays containing
positive record indices and ranges. Their primary fields now explicitly show
record indices. Native list values, validation, selection and serialization
are unchanged. Two original missing-label failures are retained; two headless
and two visible observations pass. No mission ran, and the frozen reader
replays without a native test. This batch adds no whole-row credit.

The full unit requirement remains open for multi-argument function input unit
context and the final field/source-domain conjunction. Canonical coverage is
1,111/1,158, with 10/18 agreed requirements complete.
