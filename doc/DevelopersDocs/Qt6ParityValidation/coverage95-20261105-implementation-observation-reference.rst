Observation time-reference implementation
========================================

The user requested implementation first, with new tests deferred. This change
connects metadata already read from TDM observations to the native measurement
model. Compilation is the only validation completed for this increment.

Behavior implemented
--------------------

* Explicit transmit/receive references override an adapter's configured default
  for the current observation. Legacy streams retain their adapter default;
  their existing false epochAtEnd flag can still request transmit tagging.
  Subsequent observations and simulation calls recover the configured default.
* Integration references represent START, MIDDLE and END. Counted receive markers
  move forward by T, T/2 or zero to locate the existing end-path calculation.
* Counted transmit markers first traverse the native signal solution. The last
  receive epoch plus receiver hardware delay supplies the corresponding receive
  marker, separately for each selected strand. The receiver counting interval
  is then applied. Transmitter and receiver seconds are not equated by simply
  changing the sign of the start-path hardware-delay offset.
* Endpoints copied into internal range adapters explicitly carry A1/RECEIVE/END
  metadata. Their source file's tag and integration offset cannot be applied a
  second time. Temporary count settings and propagated states restore on errors.
* DSN, generic, TDRS, BRTS, TDRS three-link return and TDRS DOWD outputs retain the
  original observation timestamp. Pass bias lookup uses that same timestamp;
  internal end/start measurement caches retain their actual endpoint epochs.
* Angular output honors the chosen signal reference. GPS point observations
  convert their declared source time system to precise A1 through the existing
  converter. TDM unknown explicit reference codes raise an error.
* Legacy TDM conversion preserves fractional integration intervals, range moduli
  and uplink frequencies. Existing frequency unit heuristics are unchanged.

Build checks and review material
--------------------------------

Review material resides in
``build/nasa-coverage/coverage95-20261105-implementation-observation-reference``.
The incremental patch and exact preimages isolate these changes from the much
larger dirty checkout. ``changed-files.json`` records fifteen changed files.

``compile-results.json`` records fourteen clean translation units using the
existing configured compiler flags. ``compile-results-v2.json`` records two
additional angular/GPS units. Objects and logs stay in the private stage; none
replaced the application's libraries. All sixteen unique compilations passed
without diagnostics. Recorded source and header hashes match the live files.
The incremental patch passes reverse-application checking and adds no trailing
whitespace. No tests, fixtures, registrations or acceptance seals were added.

ObservationData and MeasureModel gained members. Rebuild the EstimationPlugin
and clients against matching headers before loading them. No ABI compatibility,
linked build, runtime, GUI or scientific qualification is inferred from these
object compilations. Public main and existing released packages are unchanged.

Deferred focused validation
---------------------------

When the implementation phase ends, exercise explicit and configured references
across copies, assignment, segment transitions and later simulation calls. Check
START/MIDDLE/END with fractional intervals and nonzero hardware delays, including
multiple strands and atTimeOffset. Verify error paths restore propagation state
and temporary count settings. Independently compare transmit-marker mapping,
physical endpoints, reported epochs and pass boundaries. Check finite-difference
sensitivities through the transmit-to-receive mapping before qualifying
estimation; the existing derivative machinery has not been validated for this
new path. Reuse applicable existing checks and preserve their original bounds.

Average-range integration, SCLK correlation, arbitrary TDM data ordering and
full TDM compliance are outside the implemented scope. This increment supports
measurement behavior but does not discharge an original remaining requirement.
Continue from the existing 105-row worklist without another blanket audit.
Verified coverage remains 1053/1158; 48 more qualified rows are needed for 1101.
All original holds, fingerprints, assertions and historical coverage remain.
