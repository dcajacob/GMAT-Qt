POV scoped declarations and strings, 2026-11-05
=============================================

The native importer now resolves filename string expressions and scoped
declarations. This extends the mesh, finite-primitive and include-source
increments. It does not close FRR!E44/E46 or establish full POV-Ray language
support. No behavioral verification has been performed in this phase.

Scoped symbols
--------------

One typed symbol table now replaces the separate numeric, object, material and
transform maps. Its alternatives are numeric/vector values, object prototypes,
typed materials, transforms and strings. Every include receives a child table;
tokens retain their actual source table, so lookup follows their lexical source
rather than whichever file happens to be active after lookahead.

The implemented rules follow POV's primary
`declare/local reference <https://www.povray.org/documentation/view/3.60/237>`_:
#local creates or replaces the current table's definition; #declare updates the
nearest existing definition or creates a new global name. #local in the main
file therefore uses the global table. Lookup stops at the nearest definition,
even when its type differs from an outer definition. #undef removes the nearest
definition and reveals any outer one. An absent definition is left absent.

Tables own their parent references and contain resolved values. Tokens retain
a table only as needed for parsing; model geometry does not retain symbol-table
bindings. Include-local values stop being visible when lookup resumes in the
parent source. Exported objects/materials retain their resolved data, and a
global reference to a local prototype can retain that prototype's geometry.

Declarations are processed at parser entry points for objects, meshes,
materials, transforms, numeric/vector/string values and skipped render-only
blocks. Unknown directives remain errors. Declaration processing uses the
existing nesting bound. Successive declarations are consumed iteratively at
statement boundaries rather than recursively treating later declarations as
part of the completed statement.

Filename and string expressions
-------------------------------

String literals and scoped string identifiers can be declared or used as include
filenames. The seven built-in string functions are implemented: concat, chr,
str, vstr, substr, strlwr and strupr. Function arguments support nested string
calls and the existing numeric/vector expression grammar. See the primary
`string grammar <https://www.povray.org/documentation/view/3.7.0/510>`_ and
`string function reference
<https://www.povray.org/documentation/view/3.8.2/232>`_.

Integer arguments truncate toward zero with range checks. Substrings use
one-based positions and checked lengths. Vector formatting clips its dimension
argument to 2..5 and uses existing scalar/vector promotion. Numeric formatting
uses a local classic-locale stream, fixed decimal output, signed width/padding
and the requested precision; negative precision uses six decimal places.
Case conversion uses QString's Unicode operations. chr accepts the 0..65535
code-unit range for this UTF-8 reader profile; see the primary
`UTF-8 character function reference
<https://www.povray.org/documentation/view/3.7.2/232/>`_.

Quoted strings now decode four-hex-digit Unicode escapes in addition to their
existing formatting/quote/backslash escapes. Malformed escapes are rejected.
Empty and NUL-containing include filenames are rejected before file lookup.

Include expressions are evaluated without crossing the including file's EOF or
expanding the following directive. A string or structured declaration commits
its value before advancing beyond its final token, so the next include can use
the new binding. #undef also commits before advancing to a following include.
Numeric/vector declarations retain their semicolon boundary. Root declaration
parsers distinguish their own closing token from nested object/material/list
closures; existing standalone object consumption remains available.

Remaining scope and build
-------------------------

Conditionals, arrays, macros/loops, additional primitives, CSG and advanced
textures remain unimplemented. The original worksheet only says POV, and the
preserved row rationale identifies no required POV feature profile or concrete
reference model. A text question requesting representative model paths/profile
is pending. Do not infer that the original requirement mandates every feature
of a complete ray tracer, or silently declare the current subset sufficient.
Keep the original rows open while resolving their actual model-import scope;
continue other implementation work in parallel with that clarification.

Configured private builds pass: v1 five steps/4.59 seconds; v2 five steps/4.45
seconds; v3 six steps/7.67 seconds. v2 connects declaration processing throughout
the parser; v3 includes filename validation and updated source/help comments.
No model, test executable, GUI check, renderer check, numerical campaign,
acceptance audit or fixture backfill was run. Later verification must cover
nested/repeated includes, cross-type shadowing/restoration, global/local updates,
undefinition, EOF/lookahead and semicolon boundaries, all string functions and
formatting, Unicode/escape/path errors and resource/viewport interaction.

Original assertions, fixtures, tolerances, requirement fingerprints, denominator,
ledger and qualification gates are unchanged. No installed/shared application
or public package was updated. No code was committed or pushed. Implementation stage:
``build/nasa-coverage/coverage95-20261105-implementation-pov-scopes-strings``.
