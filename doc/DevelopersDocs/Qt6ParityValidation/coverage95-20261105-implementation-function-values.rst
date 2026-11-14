Typed function input values and snapshot ownership
=================================================

Implementation-first change in FunctionManager.cpp. The shipped
Resource_GmatFunction guide permits real and string resource parameters, named
resources, Variables, Strings and Arrays as inputs. The existing named-resource
lookup/clone route is unchanged. This repair addresses the fallback that
materializes values absent from that lookup.

CreateObject previously selected only by wrapper class, with an unknown class
printing an error and returning a default zero-valued Variable. It now evaluates
the reported value type: real/integer values become Variables, string-like
values become Strings, and matrices become Arrays. Unsupported types raise
FunctionException instead of silently supplying zero. Matrix dimensions are set
before SetRmatrix; the previous fallback attempted that setter on an unsized
Array. Array-element owner binding is checked before evaluation.

Wrappers and pending values have scoped ownership, including when evaluation
or construction fails. Repeated literal inputs reuse their cached snapshot;
each formal still receives its independent clone. Dynamic values are evaluated
before replacing their owned cache entry. Superseded snapshots are released
unless a live formal, caller/global store or another cache retains their
identity. This preserves the old GMT-5717 aliased-frame protection. Refresh no
longer separately replaces/deletes that same snapshot. An evaluation failure
leaves the previous cache entry untouched; no whole-call atomic rollback is
claimed.

No header, ABI, numerical algorithm, time conversion, tolerance or original
assertion changed. No new tests, fixtures, registrations, matrices, readers or
seals were added. Boolean/vector/arbitrary object-reference coercion and general
expression-language support are not inferred from this materialization repair.

Build and existing checks
-------------------------

Stage: build/nasa-coverage/coverage95-20261105-implementation-function-values.
Exact preimage and incremental-v1/v2.patch are retained. V2 changes only method
ownership documentation after the v1 implementation; both builds pass without
diagnostics in 4.45/4.28 seconds. Final patch SHA256:
``34b19b3068c0aaa88036bf4da9de65d9c7a0c6cf2a7d05390c51e91d79b2cc42``.
Reverse application checks pass. Seven monitored installed images remain
unchanged. The matched private integrated-current runtime is updated; current
image hashes are in summary.json. Older receipts retain their original versions.

Eleven unchanged existing GUI cases pass against the final private core:
Scalar, Array, String, Object, Properties, Literals and Nested I/O;
StringNumber, StringNumberInvalid and NumberString built-ins; and Recursion.
The original string/number conversion rejection, actual GUI correction/recovery,
typed values, copy semantics, recurrence and source/history assertions remain
intact. Existing executables from the input-binding increment are reused with
unchanged headers/ABI and the actual updated private core. No test rebuild or
shared-output mutation was needed.

Native xcb runs use a resizable nested desktop that starts minimized. Zero
screenshots; process maps identify private Base and exclude the old installed
Base; all test/display handles are terminal. Saved command.json files and the
existing startup/display helpers supply replay with fresh output directories.

Direct unsupported-type, native property/matrix fallback, repeated-literal
allocation, dynamic-cache alias and throwing-evaluation backfill remains
deferred. These existing checks are affected regression evidence, not proof of
every new guard or the universal function I/O/scoping conjunctions. Public
code/releases and qualification gates are unchanged. No original row admitted:
verified 1053/1158, 105 open and 48 needed for 1101. All 59 prerequisite identities
and separate historical 1816/2367 coverage remain intact; goal active/incomplete.
