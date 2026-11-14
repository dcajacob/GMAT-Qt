Five OEM output time systems
===========================

At component completion,the canonical count remains1,051/1,158:107 original rows
open,50 needed. Historical1816/2367 and all59 hold identities are unchanged.
Exact E380 admission is a separate audit;this component assigns no credit.

Original FRR2.2 enumerates A1,TAI,UTC,TT andTDB. The new OutputTimeSystem property
and GUI selector apply these systems to OEM output. EpochFormat continues to
interpret input interval bounds. UTC remains the default,and CREATION_DATE stays
actual UTC. Metadata bounds,data epochs and duplicate-epoch counting share one
formatter,so a clock's millisecond rounding also determines its record count.

All16 new time cases pass both GUI profiles. They cover all five clocks at
J2000,April2000 and the2016 positive leap second,plus a TAI output window entered
in TT. Every serialized tag is compared with an independent calendar and every
state with the original circle bounds. The reference uses `NAIF's published
TT/TDB model <https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/req/time.html>`_,
whose stated approximation is about30microseconds,against millisecond output.
A1=TAI+0.0343817seconds is explicit in the new private interface agreement.
The generic B1 oracle's A1 gap remains visible and is resolved only by that
forward convention;no legacy or operational agreement is invented.

Two affected UTC controls also pass both profiles. Their complete payloads match
prior source-versioned outputs byte-for-byte after removing CREATION_DATE.
The committed non-UTC-to-SPK configuration transition,rebuild and exact Undo/Redo
recovery pass both profiles. A baseline missing-selector failure and a fixture
Resources-tab failure are retained. Source4 was captured but neither built nor
executed after a pre-build review found an old tree-item reference. The corrected
source5 reacquires the item.40GUI contexts were observed:38passes,two failures.

Native writer support passes all five systems. It checks high/low-level copies,
invalid values,metadata consistency,open-file clock/assignment rejection,and
selected-clock sub-millisecond groups. A1 emits two records while the other
systems emit one for the same three A1 inputs;the retained last state and degree
match each clock's groups. All41native and two initially minimized,resizable,
nonfullscreen display identities were observed terminal in their execution host
namespace. No runtime binary/kernel copies or alias-target artifact bindings.

Five previously pending core paths changed;one header becomes newly pending.
The complete inventory has286paths,with280previous paths unchanged. Two string
members,one appended parameterID and three nonvirtual methods are added;no
virtual slot is added. Exact preimages,incremental/full patches and image/source
bindings are retained. Linux checks do not confer external ABI,Windows,arbitrary
input-domain or independent core/scientific approval.

All original drivers,catalogs,oracle assertions and bounds are unchanged,including
the already approved ShortArc degree1. New fixture timestamp parsing uses exact
independent tags to handle the leap second,and metadata expects the selected
clock.54other inherited expressions remain verbatim. The prior frame component
replays without native reruns. Full1158-row/107-gap audits preserve canonical
records/statuses and all59 hold keys,with progress annotations only.

NarrowAtLast remains red. E404's error/fallback conflict,E381's incomplete frame
list,E374/E375/E377 conjunctions and all broader gates remain open. The precise
E380 scope review is ``row-scope-review-v1.json``. No general OEM or scientific
qualification follows from the five-clock option.

Evidence: ``build/nasa-coverage/coverage95-20261104-oem-output-time``.
Replay without native execution::

   PYTHONDONTWRITEBYTECODE=1 python3 build/nasa-coverage/coverage95-20261104-oem-output-time/verify-component-v1.py

Registrations:16 ``QtGui.NasaOemOutputTime*`` cases and
``QtAcceptance.OemOutputTimeWriter``. Reuse source-specific passes;execute only
new or affected cases when further changes require validation.
