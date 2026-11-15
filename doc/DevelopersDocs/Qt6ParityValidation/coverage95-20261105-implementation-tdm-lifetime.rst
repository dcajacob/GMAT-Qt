Implementation first: TDM reader ownership and final observations
================================================================

Three production CPP files and one private class header are repaired. Four
affected translation units compile cleanly, including the existing factory
consumer of that header. Runtime and GUI validation remain deferred under the
user's implementation-first direction. No requirement is newly credited:
verified coverage remains 1053/1158, with 105 open and 48 needed for 1101.
The 59 original hold identities and historical 1816/2367 coverage are unchanged.

TdmReadWriter now owns its document explicitly. After schema validation it
adopts the document from Xerces using the installed API's ownership contract.
Copies allocate an independent parser/error handler, deep-clone the complete
document, and map the body, segment, data and observation cursors to the same
child paths in that cloned document. The observation template, current index,
document URI and transmission-band map are copied as well. Assignment stages
the replacement before exchanging resource state. Closing or destroying either
reader consequently releases its own objects. The existing TdmObType wrapper
continues to create a fresh stream when copied; the cursor-preserving copy
behavior belongs to the low-level TdmReadWriter API.

Finalize now clears document cursors, releases the adopted document, deletes
the parser and error handler, and balances only a successful Xerces Initialize.
It succeeds on repeated calls and also cleans up readers never initialized.
Initialize recreates the error handler after finalization and records ownership
of the Xerces initialization before later allocation can fail. XML exception
messages are materialized before their exception objects and platform resources
are released. TdmObType assignment releases the previous reader rather than
leaking it. Close/Finalize clear the open/validation state, and subsequent Open
can initialize and validate again. Parser input sources and active transcoded
strings now use scoped ownership.

The final observation was also being dropped: LoadRecord can fill an epoch's
data while returning a null next-segment template at EOF, and the wrapper used
that null value to delete the record it had just read. The wrapper now returns
the completed record and reports EOF on the next call. Repeated EOF reads return
null safely. Empty/malformed structural elements and missing required header
attributes report measurement errors rather than dereferencing absent nodes.
Non-element DOM children are checked before being cast to elements. XML namespace
prefixes are resolved through local element names for metadata and data tags.

Two string comparisons omitted their equality checks, so uppercase TRANSMIT and
START did not select their intended time-tag/integration-reference flags. Both
comparisons are corrected. TdmErrorHandler copies its error classification along
with its name. Observation epochGT is populated consistently with the existing
scalar epoch. The legacy date parser now tokenizes a writable buffer and checks
for missing date/time components instead of modifying a const string's c_str or
reading uninitialized pointers. This retains the existing scalar parser's time
precision; it is not a nanosecond-precision qualification of the TDM pipeline.

The added document pointer changes the exported TdmReadWriter class layout.
Validation must rebuild EstimationPlugin and any external client using that
class with the matching header. No external ABI compatibility, full-build,
scientific or full-NASA gate is cleared. Shared application libraries and public
preview packages are unchanged; the new code has not been loaded at runtime.

Exact preimages, before/after hashes and the incremental patch are under
``build/nasa-coverage/coverage95-20261105-implementation-tdm-lifetime``.
``incremental-core.patch`` has SHA256
``c9408bf36e019d89b4e58d1abfee93adfbc80ab8918d09cfeb43aee43b9f6b6a``.
The patch passes a reverse-application check against the live sources and adds
no trailing whitespace. ``compile-results-v2.json`` records the final four clean
compilations and unchanged header inputs; the earlier compile result and patch
are retained. The installed Xerces AbstractDOMParser, DOMNode, DOMDocument and
PlatformUtils headers supplied the adoption, deep-copy, release and repeated
initialization contracts used here. No test, fixture, registration, source
capsule or evidence seal was added, and no old native pass was rerun.

After implementation, validate independent active-document copies and assignments
at intermediate epochs, after source destruction, and at EOF; repeated
Initialize/Finalize/Open/Close; final epochs and segment transitions; multiple
readers with balanced Xerces lifetimes; XML prefixes, Unicode content and schema
errors; both time-reference flag values; and actual estimator/filter use of
epochGT. Preserve all original assertions and tolerances. Add the required
unattended and initially minimized/resizable visible GUI coverage when testing
resumes. Precise TDM timestamp parsing, unsupported metadata/input domains and
the remaining whole-row conjunctions stay open. The 95 percent goal remains
active/incomplete.
