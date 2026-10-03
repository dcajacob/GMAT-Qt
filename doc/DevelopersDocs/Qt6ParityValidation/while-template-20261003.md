# While template: demonstrated repair

Raw chapter artifacts remain under `build/example-qualification/20261003/extra-help/leo`, with focused check artifacts under `build/example-qualification/20261003/single-step`.

The actual LEO Qt form attempt on app614e rejected a correctly typed `LEOsat.ElapsedDays <10` because the generated body referenced undeclared VariableName. The frontend template now uses a valid comment placeholder. Native LEO Part1 later constructed the loop and single-step body through actual input, ran the complete ten-day mission and closed normally0/0/0; no engine algorithm changed.

The new focused `QtGui.WhileTemplate` check passed0.35 seconds in `build/example-qualification/20261003/single-step/fourth-check-failed/LastTest.log` lines3–22. It exercises menu insertion, typed Apply, body append, unrelated-source preservation, Unicode Save/reopen and a known1200-second run. The enclosing check batch failed a separate SingleStep fixture later; the passing While check is retained and not repeated. Test binary SHA `58798988a6b177d083b69678a1412be87e937403211b9c00c18c579bec4f9e94`; source SHA `b14377133ce82c8f5f6ad38be41aded723268391586d077f1c61f845e1a9032c`. Runtime/compiled-production identities are in `while-focused-fixture-diagnosis.json` and the batch `combined-identities.json`.

Earlier focused attempts with elapsed condition<1200 and then<900 reported1800.000000209548. The original floating-boundary-only explanation was insufficient. Existing While.cpp197–255 clones elapsed parameters; Execute319–331 resets their initial epoch on branch entry; TimeData.cpp505–508 establishes that local origin at first evaluation. After one outside600-second step the local branch clock starts0;900/1200 require two body steps, giving global1800. The corrected fixture<300 requires one body600 step and preserves the1200 expectation and1e-6 tolerance. Failed diagnostics/argv/source are retained in `single-step/while-diagnostic` and prior check folders.

Private X11/software GL and focused frontend evidence only; no host/GPU acceptance or independent flight-dynamics claim.
