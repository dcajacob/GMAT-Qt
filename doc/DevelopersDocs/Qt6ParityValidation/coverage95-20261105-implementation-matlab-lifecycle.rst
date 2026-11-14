MATLAB engine ownership and lifecycle
=====================================

This implementation increment changes only ``MatlabInterface.cpp``. It repairs
named handle retirement/selection, successful close return codes, close-all
cleanup, unused automatic names, failed-open preservation of the active engine,
shared setup failure, mode validation/ownership, inactive configuration copies
and bounded fresh output capture. No provider API is substituted and no test
assertion, tolerance or fixture changes.

Mode changes require closed engines, preventing an open single-use handle map
from being dispatched as a shared connection. Reapplying the current mode keeps
the workspace. Cloning copies configuration into an inactive interface with an
independent buffer. Assignment to an active interface raises an explicit error.
Closing a selected named engine chooses the first remaining named connection;
closing a different engine retains the current selection. All attempted close
handles are forgotten, including already-terminated connections, so later calls
cannot reuse them. Close-all reports failure if any vendor close failed while
still processing the remaining handles.

The current vendor contracts were checked against these primary sources:

* https://uk.mathworks.com/help/matlab/apiref/engopensingleuse.html
  documents Windows single-use processes and no UNIX support.
* https://www.mathworks.com/help/matlab/apiref/engopen.html
  documents the distinct Windows COM and UNIX process/pipe behavior.
* https://www.mathworks.com/help/matlab/apiref/engclose.html
  documents zero on success and one on failure.
* https://www.mathworks.com/help/matlab/apiref/engoutputbuffer.html
  documents the lack of guaranteed null termination and return status.

Build and reused smoke
----------------------

Artifacts reside under
``build/nasa-coverage/coverage95-20261105-implementation-matlab-lifecycle``.
The exact preimage and incremental patch isolate this change from the dirty
checkout. ``build-v2`` contains the final compile/link commands, successful
receipts and the private candidate. The original compile/link and candidate
remain, as do both failed launches. No installed library was replaced.

The unchanged registered ``QtGui.NasaMatlabFunctionCreation`` executable passes
in both ``headless-smoke-v3`` and ``visible-smoke-v3``. Each directory contains
the exact command, result, raw test outputs and actual native process maps.
The mapped plugin is the final candidate and the mapped engine is the installed
R2026b C Engine; the older installed MatlabInterface is absent. These checks
cover prescribed shared-engine calls, independent results, real resource UI,
save/reopen, report viewer and recovery after runtime/syntax errors.

To replay, use the saved command with the candidate's ``plugins`` directory
prepended to ``LD_LIBRARY_PATH`` and the installed MATLAB ``bin`` prepended to
``PATH``. ``RunNativeTest.py`` supplies private X11 for unattended execution.
Keep ``GMAT_QT_CAPTURE_CHILD_OUTPUT=1`` so detached SDK services do not hold the
test's output pipe open. Select a new artifact directory through
``GMAT_QT_ACCEPTANCE_DIR``. Preserve every prior result directory.

For visible execution, reuse ``VisibleTestDisplay.start`` with
``resizable=True, start_minimized=True`` and pass its returned environment to
the command with ``GMAT_QT_ACCEPTANCE_MODE=visible``. Set
``GMAT_QT_ACCEPTANCE_MAX_SCREENSHOTS=0``. The retained display receipt confirms
the outer desktop started Iconic/HIDDEN; the observer confirms zero screenshots
and captures. Display access and licensed execution required the existing
outside-sandbox native route. The first sandbox attempt failed before GUI
initialization; the second attempt's explicit PLUGIN line incorrectly retained
the .so extension. The corrected PLUGIN value omits that extension. Both failures
are preserved and excluded from passing evidence.

The private link reuses other previously compiled objects and existing base
libraries. It establishes a bounded candidate smoke, not a complete build of
every current worktree change. All build, native and display handles finished.
No source capsule, evidence seal or acceptance-row admission was produced.

Deferred qualification
----------------------

After the implementation phase, backfill Windows single-use named-engine
selection/close/reopen and mixed close errors, independent process identities,
real interprocess shared mode, copies/assignment, invalid and active mode
changes, and short/long/invalid output-buffer boundaries. Reuse applicable
existing checks without weakening their original assertions. The literal
single-use requirement cannot be proved by substituting engOpen on UNIX.

Coverage stays 1053/1158, with 48 further qualified rows needed for 1101. All
original hold identities and MATLAB bundle gates remain; historical operations
stay 1816/2367. Public main and released packages are unchanged.
