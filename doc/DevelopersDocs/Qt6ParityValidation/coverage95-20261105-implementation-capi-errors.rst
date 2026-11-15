C-interface object bounds and exception diagnostics
==================================================

Implementation-first repair in CInterfacePluginFunctions.cpp only. A small native
C diagnostic reproduced GetObjectName(CountObjects()) returning the previous valid
name and message. Negative indexes instead reported a missing Moderator. Missing
plugin-library lookup returned null while leaving an unrelated object-name message.
Object enumeration, summary generation and plugin lookup also lacked ordinary
native/standard exception guards present on other C entry points.

GetObjectName now returns an explicit range diagnostic for both invalid bounds;
a valid following query recovers normally. CountObjects,GetObjectName,GetRunSummary
and GetPluginFunction translate native/standard/unknown failures into their C
return/message channels. A summary failure replaces its partial result with an
error diagnostic. Valid summary text still shares getLastMessage; successful
CountObjects does not overwrite it. Plugin lookup rejects null/empty names before
string conversion and reports an unresolved function/library. It continues to
look up already-loaded GMAT libraries; it does not add library-loading behavior.
No signatures,layout,members,slots,model IDs or numerical formulas change.

Stage: build/nasa-coverage/coverage95-20261105-implementation-capi-errors.
Exact source preimage and incremental-v1.patch retained. Patch SHA256:
b9374a10090d13f1ca91bfc0a832089fb93536d8d5f8b72b80e09740d0d65e7f.
Reverse patch check passes. Private CInterface/summary-client build passes1.91s
without diagnostics. summary.json records final images; private Base unchanged
from the structural-walk increment. Seven monitored installed images unchanged;
public code/packages unchanged by this increment.

One small manual C diagnostic was authored,compiled once and run before/after
against the same unchanged existing basic script fixture. It observes both object
bounds,valid-name recovery,valid plugin factory lookup,missing library and an
after-only null function name. The null call was not attempted against the unsafe
preimage. Actual factory lookup returns1; after-failure names/messages are no
longer stale. Sources,compiler command,inputs,commands,logs and receipts remain.

Five unchanged existing external-C checks pass: basic133,stm481,summary790,
stopped891,runtimeerror891 assertions;3186 total. The summary cases retain exact
command identities,metadata isolation,cached values,repeat-call bytes and
successful reload after interruption/runtime failure. Existing fixture builders
and metadata observer are reused; no original assertion changes.

Replay run-existing.py and saved commands with fresh private startup/output
directories and private bin/plugins on LD_LIBRARY_PATH. StartGmat reads the
isolated gmat_startup_file.txt. Display variables are removed. Native image maps
or loader traces identify private CInterface/Base and exclude installed images.
All compiler/native handles terminal;zero GUI executions/screenshots. No new
suite tests,fixture definitions,registrations,matrices,readers or seals. The manual
diagnostic is engineering evidence,not a newly qualified acceptance suite.

Forced exceptions in enumeration/summary callbacks and full native ABI/force/
provider/lifetime combinations remain deferred. No whole-interface or universal
exception-containment qualification. E249/E251/E252 remain open;no original row
admitted. Verified1053/1158,105open,48neededfor1101;59holds,31resolved/28unresolved
including6MATLAB,37MATLAB identities,historical1816/2367 unchanged. Goal active/
incomplete; new formal test expansion remains deferred.
