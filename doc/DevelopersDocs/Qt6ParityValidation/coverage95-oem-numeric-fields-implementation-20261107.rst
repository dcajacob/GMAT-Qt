OEM B-1 numeric-field rejection
===============================

Original requirement: FRR!E375, with format parent FRR!E374. The pinned standard
is CCSDS 502.0-B-1, sections 4.3.3(e,f) and 4.3.5(a): data records contain an epoch
and six Cartesian components, expressed using the specified decimal syntax.

The concrete source gap was that CCSDSOEMWriter::AddDataForWriting accepted a
nonfinite state without validation and WriteDataSegment passed its components
to sprintf. That path could serialize nan or inf, which do not meet the numeric
syntax. A short vector could also reach unchecked component indexing.

The shared CcsdsOemB1Text::RequireState helper now rejects nonfinite epochs and
each nonfinite Cartesian component, with a field-specific error. It also requires
at least six components before accessing them. The OEM 1.0 direct writer calls
the helper before adding a record; EphemWriterCCSDS calls it before its buffer
can flush or accept a sample. Finite values are neither converted nor replaced.
Version 2.0, acceleration/covariance policy, orbit calculations, interpolation
and numerical tolerances are unchanged. No public API, layout or virtual slot
is added.

Bounded source review
----------------------

The existing implementation already has the following routes. These observations
avoid treating a lack of current runtime evidence as a missing implementation:

* CCSDSEMWriter::WriteHeader supplies the header and UTC creation date;
  CcsdsOemB1Text validates printable ASCII, line lengths and comment prefixes.
* CCSDSOEMSegment::GetMetaDataForWriting supplies and validates the metadata;
  EphemWriterCCSDS::WriteCcsdsOemMetaData supplies the selected frame, origin,
  time system, bounds and supported interpolation degree.
* CCSDSOEMWriter formats record epochs, suppresses adjacent equal formatted
  epochs and counts serialized records for the degree recommendation.
* For FRR!E406, EphemerisFile::ShouldSuppressTrialOutput and its data/event paths
  suppress user output while solving. SolverBranchCommand::GetSubscriberRunState
  accounts for enclosing solvers; RunEstimator::Execute selects SOLVING or
  SOLVEDPASS according to the estimator final pass. These are existing paths,
  not newly established runtime correctness.

No further format extension is selected here. Complete trajectory/frame/time
correctness and the existing retained scientific failures require later
verification against the current build. The incomplete original coordinate-system
list remains a specification dependency; this repair does not supply a new list.

Compilation and limits
------------------------

The integrated build returned 0 in 2.95 seconds, completing 71/71 steps with no
warnings. Its observed compiler session is terminal. Source hashes match that
build. The three source preimages, patch, five reference hashes and build receipt
are retained under
build/nasa-coverage/coverage95-20261107-implementation-oem-numeric-fields.

No tests or runtime verification ran. The original worklist, requirements,
assertions, fixtures, tolerances, dispositions and gates are unchanged. No row
credit is assigned; the verified baseline remains 1053/1158 (90.9%) and the carried
engineering estimate remains 1093/1158 (94.39%). This is not completion of the
whole format requirement or campaign.

The next bounded area is the original function I/O, command and scoping clauses:
FRR!E450, E477, E479, E491 and All Others!E241-E244. Reconcile those clauses with
their existing implementation before selecting any additional code change.
