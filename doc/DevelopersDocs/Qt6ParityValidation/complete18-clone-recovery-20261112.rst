User-defined body clone recovery
================================

The partial stage ``build/nasa-coverage/complete18-20261112-clone-conjunction-v1``
repairs a current Resource Tree clone regression. The physical-body copy
path emitted full native snapshots for ordinary user-defined bodies. An
untouched factory body has unset mass and radius values of zero. Replaying
those values as explicit assignments caused the native interpreter to reject
the copy. The original Asteroid case and a supplementary backend witness
both reproduced the failure.

The repair limits physical-model snapshots to bodies retaining a default
physical model. Ordinary user-defined bodies use their authored settings
and contextual defaults. This restores the prior valid handling of unset
factory values, with the public clone API intact.

All eight original default/modified user-body cases now pass for Asteroid,
Comet, Moon and Planet in both headless X11 and the visible desktop profile.
These sixteen test executions report forty-eight native missions. A focused
backend witness also passes. Two normal CTest entrypoints verify user-body
recovery and the existing Sun copy route. The new copy tests now use the
native X11 launcher through CTest. Original assertions and failure receipts
are retained. The visible test desktop was closed after completion.

The evidence index also reuses twenty-five passing source-versioned
resource-lifecycle cases with recorded clone checkpoints. The worksheet
refers to FRAC-4.5, which is absent from its identifier list. Its unique
adjacent resource catalog is FRAC-6.1, with all thirty-three resource kinds
at All Others E121-E153. This mapping is recorded for the parent audit.
The remaining direct-copy witnesses are PropSetup, VF13ad, MatlabFunction
and GmatFunction. The complete parent admission audit remains pending.

No requirement row is admitted by this partial stage. Coverage remains
1,113 of 1,158 requirements and 12 of the 18 agreed rows.
