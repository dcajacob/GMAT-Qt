POV include sources, 2026-11-05
==============================

The native Qt importer now expands literal #include filenames while reading
tokens. Included content can supply supported declarations, object definitions,
instances or fragments of geometry data. It shares the existing parser and
global declarations rather than creating and combining independent models.

POV's primary `include reference
<https://www.povray.org/documentation/view/3.60/236/>`_ specifies insertion at
the point of the directive and nesting up to ten include files. This increment
implements that literal-filename portion. String identifiers/functions as
filenames, file-local declarations and the remaining preprocessor language
are not yet implemented. Included content must use the same supported grammar
as inline content at that position.

Source handling
---------------

Each active file has its own text, cursor, line number and canonical identity.
An include's EOF resumes its parent's token stream without joining adjacent
numbers or identifiers. Comments and quoted strings are tokenized before
directives are recognized. Include-looking text inside those constructs does
not open a file. The existing supported non-include directives remain visible
to the parser; they are not executed prematurely while reading lookahead.

Relative filenames search the including file's directory, the main model
directory, then the include directories passed to the importer. The viewport
passes its configured OSG data-file search paths. Absolute filenames are read
directly. Missing, unreadable and invalid UTF-8 files produce diagnostics.
Canonical identities detect active recursion, while later non-nested inclusion
of the same file remains allowed. The ten-level limit is separate from the
existing model/expression nesting limit.

The original two-argument readPovModel entry point remains available. A new
overload accepts include search directories. Files are read with QFile, decoded
as UTF-8 and closed after loading their text. UTF-8 BOMs are accepted. Quoted
filenames support escaped quotes/backslashes and the ordinary formatting
escapes; see the primary `string literal reference
<https://www.povray.org/documentation/3.7.0/r3_3.html>`_.

Per-file locations travel with tokens and triangle data. Parse errors and
geometry diagnostics identify their file and line. Deferred mesh validation
uses the owning mesh's source rather than whichever parent token follows it.
String tokens cannot act as punctuation/object names, and braces inside quoted
render-only data do not change block nesting. Mesh list counts retain integer
range checks but no longer compare against only the main file's character
count, which could reject valid large data lists supplied through an include.
List entries are still parsed and checked individually.

Scope and build
---------------

This extends the static mesh/finite-primitive importer. It does not implement
filename expressions, #local/#undef, macros/loops, other primitives, CSG,
layered/image textures or per-vertex texture blending. FRR!E44/E46 remain partial;
the conservative implementation count is unchanged.

The configured private builds pass: v1 six steps in 7.44 seconds; v2 five steps
in 4.34 seconds. The second build includes source-location and quoted-token
corrections. No model, test executable, GUI check, renderer check, acceptance
audit or numerical run was launched. Later verification must cover top-level
and geometry-fragment inclusion, nested/repeated/recursive sources, EOF and
lookahead boundaries, relative/absolute/search-path/Unicode resolution, quoted
and comment text, malformed inputs and error-source attribution.

Original fixtures, assertions, tolerances, requirement fingerprints, denominator,
ledger and qualification gates are unchanged. No installed/shared application
or public package was updated. Implementation stage:
``build/nasa-coverage/coverage95-20261105-implementation-pov-includes``.
