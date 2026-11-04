Function command scope and remaining dependency evidence
=======================================================

Coverage remains **1,019/1,158 (88.0%)**, with 139 open original requirements
and 82 more needed for 95 percent. Historical coverage remains
**1,816/2,367 (76.7%)**. This read-only scope review adds no requirement credit,
native execution or production change. All 59 prerequisite holds and separate
scientific/core/current-build/full-NASA gates remain unchanged.

FRR!E477 command inventory
--------------------------

The exact original requirement says users may employ any GMAT-supported command
inside a GMAT function. The previously qualified Script Event conjunction
retains a 53-entry runtime census. ``ReviewFunctionCommandScope.py`` checks all
2,494 artifact identities in that qualification against its canonical-ledger
hash, then maps existing fixture candidates onto every census entry. Nineteen
existing function claim candidates are preserved for on-demand receipt review.
Main-sequence and Script Event proofs are not relabeled as function proofs.
The census is its recorded runtime version, not a newly executed plugin census.

A concrete restriction precedes the missing full command matrix. The native
``SaveMission::Execute`` condition explicitly requires ``currentFunction == NULL``.
The incomplete SaveMission guide says it saves the mission but supplies no
function exception. A terminal main-sequence command exports the mission;
inside a GMAT function it returns without exporting. Source acceptance and a
successful function return alone do not establish the original command-effect
obligation.

Four retained comparisons, terminal-main and function cases in both execution
modes, pass their original assertions. The review rechecks result/proof/log/
JUnit identities, every independent circle/control column in the continuation
report, the actual function body and its independently expected return value 4.
The original terminal case expects an export; the function case explicitly
expects none. Both visible contexts retain native exposures. The enclosing
qualification supplies original source/runtime custody without any mission
replay, reference rebinding or receipt rewrite.

FRR!E477 remains open as a source/runtime-policy restriction plus an incomplete
function-context conjunction. Do not launch a large passing matrix to work
around this gap. Do not change the preserved ``expect_saved=false`` assertion
without a supported-behavior decision. The Create/dispatch/structural/starter
dispositions from the main or Script Event inventory also need their own
function-context assessment; a candidate fixture is not a qualification.

The first scope output is retained as v1. V2 additionally binds the exact
requirements workbook and historical matrix to the canonical authority hashes.
The authoritative output is ``scope-review-v2.json`` under
``build/nasa-coverage/coverage95-20261103-function-command-scope/``. Regular input
bindings record both physical and uncompressed payload identities. Source
archives, stage receipt and compile/whitespace verification accompany it.

Read-only recheck, when new evidence warrants it::

   python3 src/qtgui/tests/ReviewFunctionCommandScope.py \
     --output /tmp/function-command-scope.json

MarsGRAM availability check
---------------------------

The public GMAT tracker provides a concrete dependency explanation for the six
open Mars-GRAM rows, FRR!E120-E125. As accessed on 2026-11-03,
`GMT-8441 <https://gmat.atlassian.net/browse/GMT-8441>`_ describes the GMAT
MarsGRAM2005 plugin as internal and open for public-release work. The recorded
maintainer comment identifies unresolved distribution licensing; its earliest
listed work version is R2027a, not a delivery promise. This is search-indexed
primary issue content; direct opening did not return readable issue lines.
These are availability facts, not a legal determination or conformance evidence.

The shipped ForceModel guide names ``libMarsGRAM`` and compatible MarsGRAM2005
inputs. Obtaining another Mars-GRAM version does not establish that exact plugin
or its compatibility. No plugin/model was acquired, substituted or enabled;
no external person was contacted. The six original obligations remain in the
denominator and unqualified. This supplemental dependency finding does not
alter the existing 59-hold list or resolve its 1,099/1,158 optimistic ceiling.

Next work should target a complete source-backed obligation, or resolve one
of the recorded behavior/dependency questions. More passing variants of an
already conflicted row do not advance the required 1,101-instance goal.
