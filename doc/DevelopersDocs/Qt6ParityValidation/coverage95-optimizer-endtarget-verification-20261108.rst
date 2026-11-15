Optimization terminator compatibility
====================================

The implemented worksheet spelling ``EndTarget`` passes the optimization
termination checks. ``EndOptimize`` remains supported. No production repair
was needed in this component; it verifies the November 5 implementation.

The exact candidate is FRC!E54: "The system shall allow a user to define the
end of an optimization sequence by issuing an EndTarget command." Its text,
identity and fingerprint are preserved. Neighboring FRC!E53 describes beginning
a Target sequence and FRC!E55 describes all commands within a target sequence;
neither is substituted for the literal optimization-ending obligation.

Evidence
--------

Stage: ``build/nasa-coverage/coverage95-20261108-optimizer-endtarget-validation``.

* ``QtAcceptance.OptimizerEndTarget`` passes eleven native cases. Eight run
  actual scalar optimization: both spellings in direct and If-contained
  sequences, with a Target inside Optimize, and with Optimize inside Target.
  Each spelling pair produces bit-identical results. The independent optimum
  is U=2 and Cost=0; nested targeting additionally requires V=4 and Quantity=5.
  Keep=41 and the successor After=7 verify retained data and forward execution.
  Forty-eight scalar results are checked using the existing 2e-5 operation
  bound, with exact sentinel checks.
* Three negative cases require rejection: EndOptimize closing Target, EndTarget
  closing If, and an orphan EndTarget. The retained interpreter errors identify
  each actual mismatched or surplus marker.
* Four GUI cases pass unattended and visibly: the two unchanged canonical
  EndOptimize structural operation tests and two new EndTarget alias cases,
  each at Original and OuterIf positions. The new fixture changes only the
  original closing spelling and identifying metadata. Its numerical truth,
  inputs and bounds are otherwise preserved. The original worker is unchanged.
* Actual Mission Tree actions check structural protection, insertion before
  and after the terminator, parent ownership, exact source preservation,
  Undo/Redo, summaries and Unicode Save/reopen. Sixteen raw reports provide
  192 independent circular-orbit/scalar checks; sixteen nonempty summaries
  provide 96 command-state checks. The visible profile records 102 native
  exposures, with zero screenshots.

The source argument is shared across optimizer providers: structural matching
accepts the alias only for the innermost Optimize; append guards distinguish
the two solver owners; EndTarget initialization permits Target and Optimize;
its execution preserves the normal back-link/summary behavior. Qt's branch-end
lookup recognizes either marker. Numerical convergence of every optimizer is
outside this spelling/termination obligation.

``component-audit-v1.json`` independently checks the inputs, literal results,
pair equivalence, specific errors, actual GUI artifacts and identities. The
1,155 native source and 38 binary guard entries are identical before and after.
Both GUI profiles share 1,197 source and 114 binary identities. The archive
retains 2,331 non-ELF source versions; 139 loaded native ELF images are identified
without copying them. All 1,334 protected original inputs remain unchanged.
One preexisting renamed-main return warning in the included protected lifecycle
helper remains; the build and all selected executions succeed.

Qualification boundary
----------------------

This component supplies evidence for exact row admission. Its publication adds
zero ledger credit by itself. At publication, the preceding canonical union is
1,076/1,158 with 25 more needed for 95%. A separate frozen-reader row admission
must prove the full FRC!E54 conjunction and exact one-row delta before changing
that union. Broad solver/function scopes, scientific, platform, native OS input,
full current-build and full NASA qualification remain open.
