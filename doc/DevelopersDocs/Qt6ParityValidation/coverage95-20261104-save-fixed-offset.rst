95 percent campaign: Save fixed-offset persistence
=================================================

The four newly affected CoordinateSystemFixed Save cases pass in both headless
and initially minimized, resizable, nonfullscreen visible profiles. This adds
584 writable native-property comparisons and 192 independent DCM/rate component
checks. The retained Save conjunction now contains 152 cases, 304 qualified
contexts and 10,774 writable-property comparisons. The prior 148 native cases
were reused through their raw-evidence readers.

This component does not close original FRC!E132 (Save any Resource), and does not
alter the FRC!E135 filename discrepancy. Canonical coverage remains 1053/1158,
with 105 open and 48 required to reach 1101. The 59 original hold identities,
their existing dispositions and separate historical 1816/2367 coverage remain
unchanged. Scientific, technical/core, external ABI, full current-build and full
NASA approval remain unqualified.

Native evidence
---------------

The stage is ``build/nasa-coverage/coverage95-20261104-save-fixed-offset``.
``evidence-replay-v2.json`` independently checks the native logs, raw exports,
standalone rebuilds, executed reload probes, actual process maps and frozen
compiler inputs. Its four cases cover:

* Identity offset in the built-in ecliptic frame, including default omission.
* Nonzero offset in a saved custom inertial frame with Pluto as origin.
* A negative, scaled quaternion in a saved custom rotating-body frame, including
  its saved asteroid and explicit physical kernel reference.
* A 180 degree offset in the ecliptic frame.

The source-versioned generic Save driver retains every original assertion and
its 1e-12 relative property comparison. It exercises the actual command editor,
ordered object picker, empty-selection guard, exact Apply transaction, live
editor cleanliness and following mission statement. The export alone rebuilds
before a new mission evaluates its saved attitude. Independent scalar-last
quaternion geometry, fixed obliquity and prescribed body rotation check all nine
DCM components and three rates before export and after rebuilt execution. This
does not expand the original physical-frame qualification bounds.

The isolated executable uses the unchanged source6 frontend archive and source4
core libraries from the completed E52 component. No shared application binary
was built or copied. Successful headless executions are
``fixed-offset-named-headless-v2`` and ``fixed-offset-remaining-headless-v2``;
the actual visible execution is ``fixed-offset-visible-v3``. The latter records
four native processes, five native exposure events per case and an
initially Iconic, resizable nested desktop. Its fifth mapped process is the
acceptance observer probe, not another Save case.

Retained failures and routing correction
---------------------------------------

The initial new Named fixture inherited the single-Spacecraft filename even
though it selected several resources. Its mission completed, then the expected
file lookup failed. ``build-v1`` and ``fixed-offset-named-headless-v1`` retain
the original fixture, driver, build receipt and failing result. Version2 changes
only the new fixture's filename to the existing underscore-joined multi-object
convention. ``new-fixture-filename-correction-v2.json`` records this correction.
No original fixture, production default, NASA assertion or tolerance changed.

``fixed-offset-visible-v2`` created a visible desktop but actually routed all
four tests headlessly: the isolated executable's new name was absent from the
normal CMake target-to-source lookup. The strict raw-evidence audit rejected
it. These four duplicate support passes receive no visible-profile credit.
``visible-route-review-v3.json``, ``failed-reader-v1.py`` and its failing audit
log retain that event. Runner3 maps the executable to the exact generic Save
driver it compiles; the official classifier, native-platform and exposure
gates stay intact. Only the missing visible profile was then executed.

The broad pre-run helper inventory also observed the first reader version.
That version remains in ``failed-reader-v1.py`` and is explicitly distinguished
from the corrected post-run reader. It is not compiler provenance. Further
native executions require fresh source/helper guards; replay the retained raw
evidence instead of repeating these completed cases.

Remaining original Save scope
-----------------------------

``persistence-inventory-v1.json`` indexes the selected headless snapshots for
all 152 cases: 111 native types, 2516 distinct type/name/descriptor-type keys,
1428 keys observed read-only and 230 keys with a native runtime getter error.
These are inventory counts, not additional failed requirements. Inherited and
computed descriptors occur repeatedly across types. Read-only/cloaked metadata
and getter errors alone cannot establish that state is transient; owning source
and native-consumer semantics still need review.

Of 18 native ForceModel/ODEModel order observations, one differs after reload:
``QtGui.NasaSaveForceCombinedNRL`` moves RelativisticCorrection ahead of
DragForce. The earlier comparator checks these owned forces by unique type/name,
and preserves this order gap separately. Do not silently treat the difference
as harmless or remove an original assertion. Next work should establish the
source/consumer order contract and test the affected behavior if needed.

The inherited anomaly/frame/body/reference, hidden persistent-field,
attitude/rate/sequence, optional FK/file-backed post-reload initialization,
dependency-portability and earlier retired-editor observation gaps remain
explicit. Four current Save editor transactions do not retroactively prove an
older retired widget's cleanliness. Complete any-Resource credit requires the
whole original conjunction and qualification audit.

Read-only replay
----------------

Use a fresh output path for each replay, from the repository root::

   python3 build/nasa-coverage/coverage95-20261104-save-fixed-offset/verify-component-v1.py \
     --output build/nasa-coverage/coverage95-20261104-save-fixed-offset/component-replay-next.json
   python3 src/qtgui/tests/AuditSaveFixedOffsetEvidence.py \
     --output build/nasa-coverage/coverage95-20261104-save-fixed-offset/evidence-replay-next.json
   python3 src/qtgui/tests/InventorySavePersistence.py \
     --component build/nasa-coverage/coverage95-20261104-save-fixed-offset/evidence-replay-v2.json \
     --output build/nasa-coverage/coverage95-20261104-save-fixed-offset/persistence-inventory-next.json

Main CMake has four reusable ``QtGui.NasaSaveFixedOffset*`` registrations on
``GmatQtNasaSaveResourceTests``. The generator and input catalog are
``GenerateSaveFixedOffsetCases.py`` and ``NasaSaveFixedOffsetCases.json``.
The isolated build and map-observing launch recipes are retained in the stage.
No experimental campaign changes were added to the public preview packages.
