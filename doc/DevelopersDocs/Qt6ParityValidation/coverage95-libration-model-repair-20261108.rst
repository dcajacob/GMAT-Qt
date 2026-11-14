Libration velocity model repair, 2026-11-08
==========================================

Both LibrationPoint state overloads now retain the model documented in the
Calculation Objects Mathematical Specification: the relative r cross a term
is assumed zero. Newly available general ephemeris accelerations had leaked
into this approximation and changed triangular-point velocities. Only the
libration model implementation changed; native celestial-body, barycenter and
object-frame acceleration providers, positions, equilibrium equations, root
class headers and virtual ordering are untouched.

The completed broad baseline retains eleven failures on these paths: nine
calculated-point cases (including L3 cases edited to L4) and OriginFrameL4/L5.
After rebuilding actual consumers, all thirty original calculated-point,
origin-frame and object-frame derivative cases pass in both headless and
visible profiles: sixty passing executions. Three original acceleration
force/lifetime/handoff support cases also pass.

A separate precise-epoch API supplement passes all nineteen calculated-point
cases with the original independent geometry model and tolerances. Its native
point and oracle ephemeris inputs both use GmatTime. An earlier mixed-route
supplement (precise point versus legacy body inputs) produced nine passes and
ten millimetre-scale position failures; these remain retained separately and
were not hidden by changing product clocks or tolerances. Original test files
were never edited.

Evidence: build/nasa-coverage/coverage95-20261108-libration-model-current-review.
All 1,334 protected original test inputs and the canonical accepted prefix
remain unchanged. This finite regression repair grants no new NASA row credit.
Canonical coverage remains 1,078/1,158 (93.1%); the campaign, full parent scopes,
remaining product failures and final coherent current regression remain open.
