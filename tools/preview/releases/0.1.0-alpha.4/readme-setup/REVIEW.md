# Alpha.4 README setup verification

All four native platforms passed the documented setup paths after the two README corrections.

| Platform | Direct Python API | MCP profile and worker | Default GUI and desktop attachment |
| --- | --- | --- | --- |
| linux-x86_64 | Passed | Passed | Passed |
| windows-x64 | Passed | Passed | Passed |
| macos-arm64 | Passed | Passed | Passed |
| macos-x64 | Passed | Passed | Passed |

The corrected Python example changes to the binary directory and opens the startup by basename. This avoids the legacy parser collapsing an absolute path inside a Mac app. Intel Mac MCP setup builds the locked cryptography dependency and requires the documented compiler tools, OpenSSL and Rust.

Each native run downloaded the published archive into a fresh directory and verified its existing checksum and file manifest. Passing package missions, numerical results, save/reopen and Python-free GUI evidence were reused for identical archive bytes. No full regression or native mission rerun was performed.

The installed MCP executable used the generated profile without edits. Worker object access covered core, station and navigation bindings. The GUI used default startup and discovery paths with --mcp-bridge. Detaching preserved the GUI process.

The first attempts remain under initial and second. Linux, Windows and Apple Silicon finish in workflow 43660660269. Intel Mac finishes in workflow 43660986580. The Intel dependency compilation needed more than the initial 180-second test allowance.

Hosted runners supplied compiler tooling. First-launch Gatekeeper approval and hardware-specific GPU behavior remain outside these automated checks.

[Detailed receipt](summary.json)

The `initial`, `second` and `final` directories retain the native runner receipts,
logs and generated profiles. `provenance.json` records the source revisions and
workflow runs. `SHA256.json` preserves the exact file bytes, including Windows
line endings.

Review the saved evidence from the repository root with

```sh
python3 tools/preview/VerifyReadmeSetup.py --review-saved tools/preview/releases/0.1.0-alpha.4/readme-setup
```

The Published README setup workflow defaults to this saved-evidence review.
Its manual `run_fresh` option explicitly enables native setup checks for a selected
platform. The saved review reads the committed receipts without downloading
archives or launching GMAT.
