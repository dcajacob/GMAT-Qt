# Event tutorial startup-kernel wording — 2026-10-02

Tutorial7's SolarSystem panel list incorrectly included a Leap Second Kernel field. The active wx control had been deliberately hidden/removed (`UniversePanel.cpp:801–802`); the runtime rejects deprecated `LSKFilename` and directs users to startup `LSK_FILE` (`SolarSystem.cpp:4444–4446`). This was an inaccurate Help instruction, not an active control to restore.

The narrow XML correction removes that item from the panel list and explains the separate startup entries: `LSK_FILE` selects SPICE's leap-second kernel; `LEAP_SECS_FILE` selects GMAT's internal `tai-utc.dat`. The tutorial uses the supplied files without changing either setting. Its existing illustration already omits the deprecated control. No kernel, numerical model or tutorial step was changed.

The established **build-qt-help** target completed with exit0 and copied the rebuilt offline Help into the application. [Focused readback](event-help-startup-kernels-20261002.readback.txt) passes: generated/installed `ch11s03.html` match, the corrected wording is present, obsolete panel wording is absent, and the StartupFile link resolves to a readable installed page. The section is **10,119 bytes / SHAcf9b32fc…**. [Receipt](event-help-startup-kernels-20261002.receipt.json) binds XML and installed outputs, build log and checker.

This does not add full GUI LSK management or requalify kernel/timescale behavior. All12 completed tutorial constructions/executions remain at their recorded scope and were not repeated. Host/GPU and other full-qualification limits remain explicit.
