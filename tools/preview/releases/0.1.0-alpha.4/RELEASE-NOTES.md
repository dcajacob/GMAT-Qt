# GMAT Qt 0.1.0-alpha.4

Prebuilt Qt desktop packages for Linux x86-64, Windows x64, macOS Apple Silicon and macOS Intel. Each archive includes matching native libraries and plugins plus the CPython 3.12 core, station and navigation API.

Source revision `a22ef7e636203b2ccfaba51deb7fac37f5cb5ffa`.

## Changes

- Repaired GPS smoother initialization and completed MATLAB data output.
- Preserved historical spacecraft-relative frame states during buffered solver plot playback.
- Corrected gravity-file and GMAT-function paths inside macOS bundles.
- Preserved Unicode filenames for Windows eclipse and contact reports.

## Download and run

Extract the complete archive and retain its directory structure.

| Platform | Archive | Launch |
| --- | --- | --- |
| Linux x86-64 | `GMAT-Qt-0.1.0-alpha.4-linux-x86_64.tar.gz` | `./run-gmat` |
| Windows x64 | `GMAT-Qt-0.1.0-alpha.4-windows-x64.zip` | `bin/GmatQt.exe` |
| macOS Apple Silicon | `GMAT-Qt-0.1.0-alpha.4-macos-arm64.zip` | `GMAT-Qt.app` |
| macOS Intel | `GMAT-Qt-0.1.0-alpha.4-macos-x64.zip` | `GMAT-Qt.app` |

Linux targets Ubuntu 24.04-compatible systems. macOS requires version 15 or newer. Keep the startup file beside the Mac app. Mac bundles use ad-hoc signatures and may require first-launch approval through Privacy & Security.

The desktop runs independently of Python. Install **CPython 3.12 with the same architecture as the archive** to use the Python API and GMAT MCP. The optional desktop bridge starts with `--mcp-bridge`.

`SHA256SUMS` covers the four archives using their download filenames. Individual `.sha256` files are also attached.

## GMAT MCP quick start

Install the GMAT MCP 0.4.0rc1 release bundle using CPython 3.12. Its installer creates a virtual environment and verifies dependency hashes. [GMAT MCP 0.4.0rc1](https://github.com/dcajacob/GMAT-mcp/releases/tag/v0.4.0rc1)

```sh
python3.12 /path/to/gmat-mcp-0.4.0rc1/install.py --destination /path/to/mcp-env
/path/to/mcp-env/bin/gmat-mcp init --root /path/to/GMAT-Qt --python /path/to/python3.12 --output profiles.toml
/path/to/mcp-env/bin/gmat-mcp --config profiles.toml doctor
/path/to/GMAT-Qt/run-gmat --mcp-bridge /path/to/mission.script
/path/to/mcp-env/bin/gmat-mcp --config profiles.toml serve
```

On Windows, use `py -3.12` for the installer, `mcp-env\Scripts\gmat-mcp.exe` for the MCP commands, the full Python 3.12 executable path for `--python`, and `bin\GmatQt.exe --mcp-bridge` for the GUI.

On macOS, pass `GMAT-Qt.app` as the installation root and launch `GMAT-Qt.app/Contents/MacOS/GmatQt --mcp-bridge`.

Use the generated `gmat-qt` profile with `create_session` for isolated workers and `attach_desktop` after `list_desktops` discovers the opted-in GUI. The same profile validates live mission edits.

## Qualification

All four actual archives passed fresh native-platform extraction checks, the existing package mission and save/reopen checks, three plot types, independent mixed-eclipse and multi-station-contact missions with Unicode reports, and the one-profile MCP worker/desktop workflow. The desktop also started with the selected Python installation hidden. Source/build outputs were hidden during archive qualification.

The prior GUI tutorial campaign completed all 17 standard-package tutorials per platform, with all 16 available stock-reference comparisons passing. The 48 independent eclipse/contact checks and tutorial results retain their recorded source revisions. Their passing evidence was reused and integrity-checked. No full regression suite was rerun.

The consolidated `release-receipt.json`, four platform API verification receipts and `reused-evidence.json` are attached. Existing release assets are preserved.

## Remaining limitations

Hosted graphics checks cover the recorded rendering profiles. Hardware-specific GPU behavior remains unqualified. Mac apps use ad-hoc signing without notarization.

The portable packages exclude MATLAB/fmincon, embedded Python command and force-model plugins, VF13ad and external OpenFrames plugins. The CPython API includes core, station and navigation bindings. The optional VF13ad lunar tutorial passed separately in the full-plugin Linux build.

Sparse OEM interpolation retains a strict endpoint-accuracy failure. The optional VF13ad plugin retains a strict exact-optimum failure for forward finite differences. Online providers depend on availability and account access. The Linux review host's previously reported stuck Home-key input fault remains unresolved.

See the attached `RELEASE-NOTES.rst` for the complete scope and earlier release changes.
