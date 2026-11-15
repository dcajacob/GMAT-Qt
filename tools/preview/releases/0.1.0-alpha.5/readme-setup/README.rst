Alpha.5 README setup verification
==================================

The README at commit 61df35218ea93a36b6affd3e195e4114f3894c22 was verified against
published alpha.5 packages on Linux x86-64, Windows x64, macOS Apple Silicon and
macOS Intel with CPython 3.12, 3.13, 3.14 and 3.15. All sixteen combinations passed.

New checks ran the README's exact Python example and required its Sat output.
They generated MCP profiles with init, kept them unchanged and used the README's
GUI launch path with default startup/discovery to attach through each profile.
Rendering and settings were isolated for testing. Linux used a private X11 display.

The original package qualification supplies each version's passing MCP install,
doctor, isolated propagation, live mission editing/execution/results and Python-free
GUI startup. Those passes remain bound to their recorded source and archive hashes.
No native missions or full regression suite were rerun for this README check.

Linux used a local copy matching the verified public download's SHA256. Windows
and both Macs downloaded the published archive. The complete archive manifests
were checked before and after execution. Windows README files use CRLF line
endings. Their logical text and executed example match the LF versions.

The platform directories preserve the original receipts, generated profiles and
logs. summary.json identifies their source revisions and workflow. SHA256.json
protects the saved files. A read-only replay is available with

.. code-block:: sh

   python3 tools/preview/VerifyAlpha5ReadmeSetup.py --review-saved tools/preview/releases/0.1.0-alpha.5/readme-setup

The initial Linux harness left four private test GUI processes after their checks.
Those processes were closed. Process-group cleanup was corrected in the harness.
The original test results are retained. Published release files are unchanged.
