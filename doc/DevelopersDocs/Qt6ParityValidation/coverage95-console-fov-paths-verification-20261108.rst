Shared CustomFOV path preservation
=================================

Creating a CustomFOV now preserves an existing FOV_DATA_PATH definition,
including an unresolved path alias. The constructor installs the existing
``../data/hardware`` default only when no definition exists. Previously every
construction replaced the user's configured path. Qt's interpreter wrapper
compensated for that reset; the native console had no such compensation.

Only the constructor's default-registration condition and an explicit exception
header changed. Public interfaces, geometry, file parsing, numerical algorithms
and the existing GUI workaround remain unchanged.

Verification
------------

Stage: ``build/nasa-coverage/coverage95-20261108-console-fov-paths``.
Registered native entry: ``QtAcceptance.CustomFovNativePaths``.

* Four native cases pass: absent/default path, configured absolute directory,
  raw path alias, and missing-file rejection. Each constructs two FOVs. Four
  positive file initializations check all 32 cone/clock values against the
  literal file vertices; both missing-file initializations must reject.
* Three actual display-free GmatConsole cases pass. Two missions reference
  two file-backed FOVs through Imagers attached to a Spacecraft, initialize
  successfully and report the unchanged literal 41. Their startup files use
  absolute and alias directories. The third mission requires a missing file
  to fail without producing the report. Each observed console process loaded
  130 ELF images and no Qt GUI or wx libraries.
* The unchanged QtGui.CustomFovPaths and QtGui.CustomFovControls tests pass
  unattended and visibly. The former contributes 62 explicit path checks
  across both profiles, including creation, failed edits, reconstruction,
  default installation, raw aliases and Unicode Save/reopen. The visible
  profile records five native exposures and zero screenshots.

The corrected pre-repair baseline passes only two of the seven native/console
cases. It retains three constructor-path failures and two positive console
mission failures. An earlier new console fixture left the FOVs unreferenced,
so it did not initialize them and incorrectly reached its report even for a
missing file. That failed negative control is retained. Only the new fixture's
hardware attachment was corrected; its assertions were preserved. No original
test, input or numerical bound changed.

``component-audit-v1.json`` independently checks the literal vertices, raw path
definitions, console reports/status/logs/loaded images, original GUI results,
source and binary guards, and all 1,334 protected original inputs. It retains
1,153 native source and 36 binary guard entries, 1,191 GUI source and 114 GUI
binary identities, and 2,329 exact non-ELF source versions. The native witness
loaded 139 ELF images. Runtime files are symlinked; no executable or kernel
copies are included in the evidence archive. ``publication-v1.json`` binds the
component; its read-only verifier does not replay missions.

Scope
-----

This repairs a demonstrated frontend parity defect under All Others!E20.
Shared engine paths and these selected missions do not prove whole-product
console/GUI parity. That parent remains open. No requirement credit is added;
the campaign remains 1,076/1,158 (92.9%), 82 open, with 25 more needed for 95%.
Full current-build, NASA, scientific, platform and native operating-system input
qualification remain open.
