85 percent campaign: external-C command summaries
=================================================

Source-versioned requirements coverage is **977/1,158 (84.4%)**. **Eight more
requirements** are needed to reach985. The new exact rows are ``All Others!E256``
(the optional post-session output parent) and ``All Others!E257`` (summaries for
all configured commands). The status-message child E258 reuses its existing
passing external-C evidence and receives no duplicate credit. Historical
coverage remains **1,816/2,367 (76.7%)**. All **181 remaining requirements and551
historical gaps** remain explicit with their original text and closure gates.

Native result checks
---------------------

Three real, strictly compiled C11 clients call the actual CInterface library:
normal execution, user Stop, and a deliberate mathematical runtime exception.
They perform **790 +891 +891 =2,572 assertions**. The normal23-command graph has
nested For/If/Else/While lists, ScriptEvent commands and an unexecuted branch.
Failed-run graphs contain24 and25 commands. Every configured command appears
exactly once in source order with its literal name and native type. Unexecuted
commands explicitly report unavailable data rather than stale spacecraft values.

The independent circular-orbit oracle checks96 cached values per successful
mission: six command checkpoints, two spacecraft and eight state/property
fields. Distinct10/20second propagation boundaries and later position/mass
assignments distinguish cached state from the final sandbox state. Independent
three-row reports check both spacecraft and loop results. Failed runs check32
cached values before interruption, one report row, explicit unavailable trailing
commands, and96 more values after successful reload recovery. Repeated queries
must preserve summary bytes, object counts, report bytes and command-name
metadata. The private observer reads metadata only; it supplies neither expected
identities nor numerical summary values.

Source review finds only base and BranchCommand implementations of structural
child lists and stable next pointers. This closes generic retrieval of configured
nodes through the common command graph; it does not qualify every numerical
field, orbit, frame, time format or external plugin payload. Scientific and
current-build qualification remain separate gates.

Underlying fixes and scrutiny
-------------------------------

The former C API followed only the main command list and emitted unnamed
records. It now traverses structural next/child lists with visited identities,
including branch alternatives, and temporarily supplies actual command names
with exception-safe restoration. Structural navigation avoids execution-state
GetNext overrides, including the solver override that changes Publisher state.
The shipped shared-message-buffer contract is preserved.

The stopped-run test also exposed incorrect status text: native return code-4
was called an unknown error. RunScript status messages now follow Moderator's
documented return-code table. Stop-4 and runtime exception-5, then success0,
are directly tested. Native return codes, exported C signatures and the public
header remain unchanged. Rare unknown initialization/execution exceptions,
invalid sandbox and absent-mission branches have source review only.

**One already tracked core/API file changed**,
``plugins/CInterfacePlugin/src/plugin/CInterfacePluginFunctions.cpp``. All100
other tracked core changes are byte-identical. The total remains101 files
pending independent technical review. All state/epoch setters, derivative
functions and numerical force/integrator/frame calculations are byte-identical
to the preceding increment. The separate mixed-dimension derivative-array
lifetime gap remains open. No technical approval is inferred.

Failures and reuse
-------------------

Seven new/affected native executions are retained, including three failed
attempts. The first exposed a fixture mistake: the second propagation duration
was20seconds, producing30 total rather than the independently asserted20. The
fixture duration became10; the numerical assertions stayed intact. The second
caught the actual missing branch/name defect. The initial stopped client assumed
return code-3; current Moderator documentation establishes-4. Correcting that
new draft assumption preserved the engine code and exposed the incorrect API
message, which was repaired. Original unrelated assertions remain intact.

Normal passes3 and4, the final stopped pass6 and runtime-error pass7 retain their
actual source/library hashes. Pass4 supplies normal proof; passes6/7 additionally
exercise recovery through the final library. An earlier pass is not attributed
to a later complete binary. No established GUI or unrelated engine test was
replayed. No screenshots or runtime binary copies are retained.

Ten coherently rehashed corrupt proof copies reject missing/duplicate names,
corrupted cached results, stale metadata, missing failed-run contexts, wrong
native status, replacement of cached values with final state, inflated NASA
claims and duplicate credit. The proof auditor runs no engine or GUI.

Reproduction and remaining limits
-----------------------------------

Build the three targets, then run only the new registrations::

   cmake --build build/linux-gui --target GmatNasaCapiSummaryTests \
     GmatNasaCapiStoppedSummaryTests GmatNasaCapiSummaryMetadataProbe
   ctest --test-dir build/linux-gui -R 'QtAcceptance.NasaCInterface(Summary|Stopped|Runtimeerror)$' --output-on-failure

These are external-C contracts, so no GMAT GUI window is expected. Audit the
existing evidence without replaying them::

   python src/qtgui/tests/AuditCapiSummaryAcceptance.py \
     --qualification build/nasa-coverage/coverage85-20261101/capi-summary-qualification.json
   python build/nasa-coverage/coverage85-20261101/negative_summary_qualification.py
   python build/nasa-coverage/coverage85-20261101/audit_eighth.py

The canonical ``coverage.json`` contains the complete union and remaining rows.
The85percent goal remains active. Full current-build, NASA, scientific, host/GPU,
platform, concurrency and arbitrary-domain qualification are not claimed.
