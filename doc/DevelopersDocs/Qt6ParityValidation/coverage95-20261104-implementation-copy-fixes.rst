Implementation first: resource and measurement copies
=====================================================

Seven existing production CPP files have been repaired. All seven translation
units compile with the configured project's compiler and flags, into private
object files. New tests and linked runtime/GUI validation are deferred under
the user's implementation-first direction. No acceptance credit is assigned:
verified coverage remains 1053/1158, with 105 open and 48 needed for 1101.
The 59 original hold identities and historical 1816/2367 coverage are unchanged.

Changes
-------

EphemerisWriter's copy constructor discarded ephemVersion and replaced its
input coordinate-system pointer with the output coordinate-system pointer.
Its assignment also omitted the version. Copies now retain both frame pointers
and the version; the coordinate converter is copied in ordinary builds as well
as debug builds. This prevents a copy from changing its file version or treating
already transformed data as though it were in the output frame.

Subscriber's copy constructor left isEndOfDataBlock uninitialized. Assignment
retained the destination's prior end-of-receive and end-of-block flags. Both
paths now preserve those flags along with the existing run state.

Parameter copies now preserve their owned-object type, and assignment preserves
the owner name. OrbitData copies and assignment preserve the origin-dependent
parameter flag alongside the already copied origin and coordinate-system
references. Copied parameters consequently retain their ownership and origin
classification rather than uninitialized or destination metadata.

TrackingDataAdapter's copy constructor shared raw participant-list pointers even
though each adapter destructor deletes them. Assignment retained the old lists.
Copies and assignment now allocate separate lists, preserving all path names
and allowing either object to be destroyed independently. The allocation helper
cleans up partial copies if allocation fails. The copy constructor also clones
the owned measurement model, as assignment already did. MeasureModel now copies
its own participant names; assignment clears old runtime lists and references
before installing the new configuration. Runtime signal and propagator objects
are reconstructed through the existing rebinding/initialization path.

TrackingDataAdapter now initializes forceHorp to false and preserves all five
height-of-ray-path settings during copy and assignment. GetParticipants uses
the requested path index and reports an invalid index rather than returning
path zero or dereferencing an empty list.

GmatObType's copy constructor initializes its legacy-format warning flag.
Assignment copies that flag and invokes its existing ObType base assignment,
preserving stream configuration in addition to numeric output precision.

No public header, class layout, virtual method, original assertion or numeric
tolerance changes. These are source-level repairs; compilation alone does not
establish runtime correctness, scientific qualification or external ABI approval.

Review and checks
-----------------

The working artifacts are under
``build/nasa-coverage/coverage95-20261104-implementation-copy-fixes``:

* ``preimages`` and ``preimages-sha256.json`` preserve the exact seven inputs,
  including all prior pending work.
* ``incremental-core.patch`` contains only this increment; its SHA256 is
  ``fc9f10bab6a1bbbca926f4ea4e1093a672ff23ff1b1f6fd0e183787bac2aba49``.
* ``after-sha256.json`` records the resulting production sources.
* ``compile-commands.json``, ``compile-command-model.json`` and
  ``compile-results-v2.json`` retain the actual commands and seven successful
  compilations. All seven compiler logs are empty. Objects are private to this
  stage; shared application libraries and the frozen preview are untouched.

The incremental patch passes ``git apply --check --reverse`` against the current
sources and adds no trailing whitespace. No existing native test was rerun and
no new test driver, fixture, registration, source capsule or evidence seal was
created for this increment.

Validation to add after implementation
-------------------------------------

Exercise writer copies with distinct input/output frames and OEM versions;
subscriber flags across copy/assignment; owned and origin-dependent parameters
through function/Clone routes; adapters and models with multiple distinct paths,
independent mutation/destruction, self-assignment and all HORP settings; valid,
empty and invalid path lookup; and observation-reader stream configuration.
Use existing affected registrations and original assertions where applicable.
Run newly covered GUI routes unattended and visibly in initially minimized,
resizable windows when that validation phase resumes.

These fixes support the still-open resource Clone/Save, parameter/frame,
function-scope, ephemeris-format and iterative-output obligations. No whole-row
conjunction is discharged by this patch. The retained NarrowAtLast scientific
failure and all unavailable-getter, historical-source, external-provider and
platform gaps remain explicit. The 95 percent goal remains active/incomplete.
