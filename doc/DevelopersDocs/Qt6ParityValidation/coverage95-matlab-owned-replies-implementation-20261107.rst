Owned MATLAB replies and modern wx text exchange
================================================

Shared object/parameter replies previously copied serialized values into fixed character arrays, and the legacy callback adapter imposed a 1024-byte limit. Those bounds did not cover large structures, strings, arrays or constraint results. The registered wx connection also ignored the declared Poke size/encoding and returned a borrowed byte pointer without setting the requested reply size. This packet implements owned replies and counted modern wx text exchange.

Owning source
-------------

The wx GUI build registers GmatConnection.cpp and GmatServer creates that class. The raw GmatSocketServer GUI bridge is not registered and refers to socketrequest/socketresult fields absent from GmatMainFrame. The initial framing investigation therefore changed direction toward the actual registered connection and shared core reply API. This does not remove original historical/platform requirements or claim the unregistered prototypes are functional. They are unchanged in this packet.

Core reply values and compatibility adapters
-------------------------------------------

GetParameterString and GetGmatObjectString return complete owned values using the existing ToString and MATLAB_STRUCT representations. The character entrypoints retain their signatures and borrow dynamic thread-local storage until the next call to that same adapter. Callback results use the same ownership policy without the old fixed length limit. Adapters clear previous storage before a new attempt and reject embedded NUL data explicitly rather than silently shortening it. Owned APIs retain the full string.

A sandbox object takes precedence by actual identity. A non-Parameter object is no longer cast to Parameter; the existing undefined sentinel is returned when the parameter interface cannot resolve a Parameter. A missing/null internal object falls back to configuration for object serialization, including the case where lookup returns null without throwing. Empty/NUL-containing names and null character pointers fail explicitly. Formatting, numeric precision and serialized resource representations are unchanged. This does not establish completeness of every resource schema, getter or cross-scope value contract.

Modern wx text exchange
-----------------------

OnRequest copies an owned reply into connection-local encoded storage, selects the requested locale/UTF-8/UTF-16/UTF-32 converter and sets the exact byte count including the appropriate NUL terminator. It rejects unsupported or unrepresentable encoding and clears old reply state on a failed request. UTF-16 and UTF-32 are handled explicitly rather than assuming the peer's format equals native wchar_t width. Counted replies can carry interior NUL bytes; legacy character clients still have their declared limitation.

OnPoke accepts the script item, validates its explicit length and terminator, decodes the declared text encoding and dispatches an owned UTF-8 command. Empty, unterminated, malformed or interior-NUL commands are rejected. Exceptions become a negative IPC result. ReadReply validates request names before accessing their last character and routes to the owned core object/parameter methods. Native scope checks and fixed optimizer protocol behavior are retained.

The changes cover the modern wx callback signatures. Older wx branches, MEX client casts/protocol assumptions, other platform transports and independent MATLAB caller provenance remain unqualified. The core borrowed storage contract does not introduce concurrent native mission support or a new lease on arbitrary resource getters.

Compilation and dependencies
----------------------------

Six source/help preimages and four planning snapshots preceded edits. All final participating source hashes match configured v2. Indexed patches, before/build/final hashes, source map, compiler logs and terminal receipts remain in the stage directory. Delta whitespace is clean and two help XML files parse.

Configured build v1: return 0, 22.02 seconds, final printed progress 75/75.
Configured build v2: return 0, 0.16 seconds, final printed progress 1/1.
Additional syntax compilation wx-connection-syntax-v1-receipt.json: return 0, 1.18 seconds, src/gui/app/GmatConnection.cpp.
Additional syntax compilation wx-connection-syntax-v2-receipt.json: return 0, 1.62 seconds, src/gui/app/GmatConnection.cpp.
Additional syntax compilation wx-gmat-server-syntax-v1-receipt.json: return 0, 1.34 seconds, src/gui/app/GmatServer.cpp.

Configured v1 builds the changed core; configured v2 retains the final source capture after a wx-only validation adjustment. The final modern GmatConnection and its GmatServer factory compile against authentic wx 3.2.9 development headers. All compiler processes are terminal and all diagnostic logs are empty. These are compilation checks, not a linked wx application, IPC exercise or MATLAB verification.

The first restricted-network package download failed DNS resolution. An auto-reviewed network escalation then downloaded the two Ubuntu development packages. Their SHA256 values match distribution metadata, and dpkg-deb extracted them only under the stage's wx-development-root. No system packages or shared runtime were installed/updated. Header/dependency acquisition is retained in wx-headers-receipt.json. The staged headers remain available for subsequent compilation.

The core additions are nonvirtual and do not change GmatInterface instance layout or existing virtual slots. GmatConnection adds a private reply buffer, so matching wx consumers require rebuilding; both the connection and its factory were syntax-checked against the matching header. Configured native consumers rebuilt. External binary compatibility, wx application linking and other platforms remain unqualified.

Primary vendor sources
----------------------

* https://github.com/wxWidgets/wxWidgets/blob/v3.2.8/include/wx/ipcbase.h — OnRequest/OnPoke byte counts include the terminator; text formats distinguish UTF-8/UTF-16/UTF-32 and native Unicode aliases.
* https://github.com/wxWidgets/wxWidgets/blob/v3.2.8/src/common/ipcbase.cpp — GetTextFromData assumes complete terminated buffers and handles only native Unicode among wide formats; explicit converters avoid casting declared UTF-16/UTF-32 data as native wchar_t.
* https://github.com/wxWidgets/wxWidgets/blob/v3.2.8/include/wx/strconv.h — Explicit-length ToWChar/FromWChar conversions and GetMBNulLen support counted replies and format-sized terminators.

Remaining implementation
------------------------

Finish nested optimizer base-workspace preservation: the existing PUSH/POP helper captures seven globals, while the legacy driver also uses GMAToptions, X0, Lower, Upper, X, fVal, exitFlag and GMATfminconOutput in the base workspace; final solution callbacks can execute a child optimizer after the parent assigns its exit flag. Preserve the script interface and full parent values/existence/bindings. Continue complete original function I/O/scoping/resource/property/output and all-command solver semantics, external caller provenance, shared globals/dependency retention, supported protocol/client behavior, transport interruption/acknowledgment and file/session lifecycle. Treat unregistered raw socket prototypes separately from the registered wx interface; preserve all actual historical/platform obligations. Finish coherent changed-component/membership/topology/storage preparation, state/result/posterior and estimator/smoother semantics, public restoration-buffer contracts, shared external values/lifetimes and source/stream/participant/error/noise/hardware providers. Continue private renderer, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool, ephemeris B-1/format/final-iterative/scientific and specification/external/historical dependencies, plus independent POV/rotation research. Defer runtime verification until the full implementation pass is complete.

No runtime/GUI/MATLAB/helper/model/mission/numerical tests, new tests, acceptance auditors/readers/seals or evidence admission occurred. Assertions, fixtures, tolerances, denominator, gates, fingerprints, original source text and dispositions are unchanged. No commits, pushes, application release packages, system package installations, installed/shared runtime changes or external messages occurred.

Tracking-v104 remains 1093/1158 (94.39%), with 65 unresolved rows. Recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. No row or verification credit is claimed. Full implementation audit incomplete; goal active.
