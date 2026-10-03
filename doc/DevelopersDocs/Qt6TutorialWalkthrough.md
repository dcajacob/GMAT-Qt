# Qt 6 Help tutorial walkthrough qualification

## Scope and current state — 2026-10-02

The user requested this as the next qualification phase after the current Linux
replacement work: follow the Help tutorials as a person using GMAT would, build
the scenarios through the Qt application, and establish that the GUI can complete
them. Shipped tutorial mission scripts are reserved for comparison afterward.
This extends the existing goal; it does not replace its unfinished acceptance
requirements. Windows, macOS and MATLAB remain deferred.

**All twelve published tutorials are Passed for bounded private actual-input construction.** See
[Tutorial 1](Qt6ParityValidation/help-tutorial-01-20261002.md),
[Tutorial 2](Qt6ParityValidation/help-tutorial-02-20261002.md),
[Tutorial 3](Qt6ParityValidation/help-tutorial-03-20261002.md),
[Tutorial 4](Qt6ParityValidation/help-tutorial-04-20261002.md) and
[Tutorial 5](Qt6ParityValidation/help-tutorial-05-20261002.md),
[Tutorial 6](Qt6ParityValidation/help-tutorial-06-20261002.md),
[Tutorial 7](Qt6ParityValidation/help-tutorial-07-20261002.md) and
[Tutorial 8](Qt6ParityValidation/help-tutorial-08-20261002.md) and
[Tutorial 9](Qt6ParityValidation/help-tutorial-09-20261002.md),
[Tutorial 10](Qt6ParityValidation/help-tutorial-10-20261002.md) and
[Tutorial 11](Qt6ParityValidation/help-tutorial-11-20261002.md) and
[Tutorial 12](Qt6ParityValidation/help-tutorial-12-20261002.md). Each includes
independent Help-driven construction, execution and result checks, save/reopen,
and comparison after freezing the authored mission. Tutorial 4 covers resource
and command forms, B-plane targeting and Mars capture. Its resource fixes have
[focused evidence](Qt6ParityValidation/tutorial-resource-fixes-20261002.md).
Tutorial 5 covers all five Help-taught code-editor stages. Its original Step 5
sample reproduced a line-search failure; appending a nominal Stage 4 starting
guess preserves its physics and constraints and passes one changed-reference GUI
verification. The frozen own construction preceded all new reference access.
Tutorials 10–11 complete DSN/GN estimation and GPS simulation/filter/smoother,
warm start, covariance/plots, ephemeris/prediction and Linux console workflows.
The shared residual-plot data-loss repair is verified through the affected
actual GUI stages. Tutorial 12 retains its frozen literal original walkthrough
and separately verifies the reviewed tracking-frame repair. The installed
offline Help is rebuilt with that correction. Tutorial 6 completes functions, Global sharing and Mars capture. Tutorial 8 independently passed electric hardware/finite-burn forms
and matched the corrected reference summary. Tutorial 7 completes its core
eclipse/contact workflow, with optional exercises and GUI LSK management
unclaimed. The current Help now accurately describes the startup-only
leap-second kernel setting; [the wording/readback record](Qt6ParityValidation/event-help-startup-kernels-20261002.md)
records the narrow correction without repeating this walkthrough. The earlier shipped-script corpus
and repairs are not GUI walkthrough evidence.
The current runtime is in `/home/dan/GIT/GMAT-Qt/application/bin`. Live testing on
the user's desktop remains stopped following the GNOME Shell crash. A private,
authenticated X11/software-GL route with actual mouse/keyboard input is now
established; see [isolated input evidence](Qt6ParityValidation/isolated-x11-input-20261002.md).
It supports independent GUI construction. Separate
[private software Wayland lifecycle](Qt6ParityValidation/private-wayland-20261002.md)
and [complementary private portal/Output input](Qt6ParityValidation/private-wayland-followup-20261002.md)
also have bounded evidence. Host GNOME/Wayland, host portals, physical hardware
and the compositor-crash cause remain unqualified. Offscreen fixtures do not
substitute for interactive construction. All twelve tutorials used that bounded route through
actual Help/resource/mission controls or the expressly Help-taught code editor,
with separate per-chapter evidence. These passes do not close the host desktop
or full Linux replacement gates.

The published order comes from `doc/help/src/Part_Tutorials.xml`; prerequisite
notes below come from the chapter introductions. Use the Help actually available
from the application when walking through it, recording missing/stale chapters
or links against the source. Reading an inventory is not a completed tutorial.

## Construction and comparison procedure

1. Start a new mission with the application's normal defaults. Where a chapter
   extends an earlier tutorial, use the mission independently built during that
   earlier walkthrough. Do not open, copy, insert or programmatically seed the
   shipped tutorial mission to construct the scenario. If the Help starts by
   loading an example, reproduce its configuration from the written instructions;
   insufficient instructions are a documented Help gap.
2. Follow the chapter's steps in order using visible resource editors, mission
   controls, plots and output. Enter the settings through the UI. Save a concise
   action record with chapter/step references, screenshots where useful, resource
   settings and branch/command structure. Engine API calls, generated scripts and
   model-setter test fixtures do not substitute for interactive construction.
3. Use the application's script/function editor when the chapter explicitly
   teaches authoring code, entering the chapter's instructions independently.
   Record script-editor steps separately from resource/mission editor coverage.
   A missing Qt control is a concrete capability finding: implement reasonable
   gaps in the authorized Linux scope and retry the affected step. Do not silently
   bypass missing GUI functionality with a prebuilt reference script.
4. Required observations, kernels, function support files and other inputs
   expressly called for by the Help are declared dependencies. Record their
   provenance and identity. Their use does not authorize importing reference
   mission/resource configuration. Preserve generated data needed by later
   chapters. Missing public inputs remain open findings; deferred/proprietary
   dependencies require an explicit, evidence-backed disposition.
5. Run each independently authored scenario and inspect the specified reports,
   plots, targets and tolerances. Save/reopen it and confirm that the constructed
   settings and mission sequence survive. For staged tutorials, preserve each
   milestone and distinguish intentional Stops from completed solves.
6. Before consulting the shipped mission script, preserve the authored mission,
   its hash, action record, runtime/build identity and outputs. Then compare
   resource settings, mission logic and numerical/visual results to the reference.
   Explain substantive differences; formatting, ordering and Qt metadata alone
   do not establish a failure. Previous exposure during corpus qualification is
   disclosed; the walkthrough must still be independently driven by the Help.
7. Record each outcome as pending, in progress, passed, failed, or blocked, with
   the exact failed step or dependency and bounded evidence. A pass requires
   successful interactive construction, execution, the chapter's result checks,
   save/reopen and the final comparison. Avoid repeating unchanged successful
   tests or expensive reference runs when existing compatible evidence suffices.

## Published tutorial sequence

All twelve published tutorials are **Passed** with the bounded evidence linked
below. Numbering follows
the Tutorials part of the Help.

| # | Chapter source and title | Prerequisites and staged coverage |
|---|---|---|
| 1 | `Tut_SimulatingAnOrbit.xml` — Simulating an Orbit | **Passed** — [actual-input walkthrough](Qt6ParityValidation/help-tutorial-01-20261002.md). None. Spacecraft, propagator, propagate to periapsis, command summary/frame, animation, save/reopen and post-freeze reference comparison. |
| 2 | `Tut_SimpleOrbitTransfer.xml` — Simple Orbit Transfer | **Passed** — [actual-input walkthrough](Qt6ParityValidation/help-tutorial-02-20261002.md). Tutorial 1. TOI/GOI/DC1, ordered Hohmann targeting, seven-iteration solve, Apply Corrections/one-iteration rerun, save/reopen and post-freeze complete solver-report equality. |
| 3 | `Tut_TargetFiniteBurn.xml` — Target Finite Burn to Raise Apogee | **Passed** — [actual-input walkthrough](Qt6ParityValidation/help-tutorial-03-20261002.md). Tutorials 1–2. Hardware/FiniteBurn/DC1/BurnDuration, ordered finite-burn targeting, 13-iteration solve, four exported summaries, explicit All-history view, save/reopen and post-freeze complete solver-report equality. |
| 4 | `Tut_Mars_B_Plane_Targeting.xml` — Mars B-Plane Targeting | **Passed** — [actual-input walkthrough](Qt6ParityValidation/help-tutorial-04-20261002.md). Independent resource/command forms, B-plane 6→1 and capture 11→1 solves, all four corrections, four exported summaries/three views, own save/reopen/freeze and single post-freeze reference GUI comparison. Default omissions and OpenFrames/native differences remain explicit. Tutorials 1–2 and B-plane concepts. |
| 5 | `Tut_OptimalLunarFlyby.xml` — Optimal Lunar Flyby using Multiple Shooting | **Passed** — [actual-input walkthrough](Qt6ParityValidation/help-tutorial-05-20261002.md). All five independently authored Help code-editor stages, expected configuration Stop, patch/full/alternate solves and >=5000 km exercise, actual own save/reopen/freeze, original failed-reference comparison, starting-guess-only repair and one verified updated-sample GUI run. Tutorials 1–2, 4 and GMAT Fundamentals training/videos; VF13ad. Source Help copies synced but not separately run; original sample Step3 physics/epoch/report limitations retained. |
| 6 | `Tut_UsingGMATFunctions.xml` — Mars B-Plane Targeting Using GMAT Functions | **Passed** — [actual-input walkthrough](Qt6ParityValidation/help-tutorial-06-20261002.md). Independent New/resources/mission controls, literal Help function editor, Global and repaired named no-output call, function/capture targets converge in 6+11 iterations, actual Mars summaries/mass/RMAG, shared three-view animation and close/reopen, own source/function readback/freeze and compatible post-freeze comparison. Current-only trial history and controlled SIGTERM cleanup limits retained. Tutorials 1–2, 4 and B-plane concepts. |
| 7 | `Tut_EventLocation.xml` — Finding Eclipses and Station Contacts | **Passed** — [actual-input walkthrough](Qt6ParityValidation/help-tutorial-07-20261002.md). Own Tutorial 2 prerequisite, Earth geometry, EclipseLocator and Hyderabad/ContactLocator forms, actual Output reports, save/reopen/freeze and post-freeze Help/prerequisite comparison. Three portions of one 2105.5299296-s eclipse and two contacts; blank-target correction retained. No final shipped Tutorial7 script; prior Tutorial2 reference run reused. Optional station-network/burn-coverage exercise and full GUI LSK management unclaimed. |
| 8 | `Tut_ElectricPropulsion.xml` — Electric Propulsion | **Passed** — [actual-input walkthrough](Qt6ParityValidation/help-tutorial-08-20261002.md). Independent electric hardware/attachment and named finite-burn forms, two-day run, spiral/fuel inspection, exported summary and own save/reopen/freeze. Two stale reference settings repaired; one changed-reference run yields complete propagation-summary equality. Tutorial 1; tutorial 3 referenced for targeting. |
| 9 | `Tut_Simulate_DSN_Range_and_Doppler_Data.xml` — Simulate DSN Range and Doppler Data | **Passed** — [actual-input walkthrough](Qt6ParityValidation/help-tutorial-09-20261002.md). Independent empty script-editor Help stages, four-observation short run, three-station/ramp/noisy 21-day run with 1348 paired observations, own save/reopen/freeze and post-freeze source/settings comparison using earlier source-matched corpus evidence. Full own observations retained for Tutorial10; stochastic/scientific equality unclaimed. Basic Mission Design Tutorials. |
| 10 | `Tut_Orbit_Estimation_using_DSN_Range_and_Doppler_Data.xml` — Orbit Estimation using DSN Range and Doppler Data | **Passed** — [actual-input walkthrough](Qt6ParityValidation/help-tutorial-10-20261002.md). Independent Help script-editor DSN estimate with own Tutorial9 observations, repaired six residual plots and report-filtered209-point export; mandatory own GN simulation/three-iteration estimate, six plots/209-point export, preserved storage failure and one justified retry. All own save/reopen/freeze and post-freeze source-matched comparison; GN truth accuracy/scientific equality/MATLAB analysis limits explicit. |
| 11 | `Tut_FilterSmoother_GpsPosVec.xml` — Filter and Smoother Orbit Determination using GPS_PosVec Data | **Passed** — [actual-input walkthrough](Qt6ParityValidation/help-tutorial-11-20261002.md). Independent Help editor simulation, cold filter/smoother, repaired residual plots, 18:00 warm state/covariance readback, ephemeris and one-day prediction, Linux GmatConsole Appendix B with complete GUI/console ephemeris equality, own save/reopen/freeze and post-freeze comparison to distinct related examples. Storage/harness failures and bounded cleanup retained; MATLAB-dependent analysis/conditioning explicitly deferred. |
| 12 | `Tut_Simulate_and_Estimate_Inter_Spacecraft_Tracking.xml` — Simulate and Estimate Inter-Spacecraft Tracking | **Passed** — [actual-input walkthrough](Qt6ParityValidation/help-tutorial-12-20261002.md). Independent literal Help keyboard/editor construction, simulation/estimation, actual save/reopen/readback; shared residual failure preserved and one unchanged-own retry exports 806/804 points. Freeze precedes 126-statement reference comparison. Original tracking frame is physically unsuitable; reviewed two-frame EarthFixed repair completes one separate 3.406-s GUI verification, two iterations, 824/822 exported points, plausible near-geosynchronous initial orbit. Original authored source preserved; scientific/host/full replacement limits explicit. |

Chapters 9, 10 and 12 describe legacy script-only navigation workflows. Chapter 5
also requires authoring an optimization sequence. Record what the Qt resource and
mission controls can actually construct, and identify remaining script-editor
boundaries; successful backend execution alone is not GUI capability coverage.
The local optional VF13ad component has previous execution evidence, which does
not establish tutorial 5's interactive workflow.

## Additional source tutorials — current 2026-10-03

The [availability audit](Qt6ParityValidation/help-extra-chapters-20261002.md)
retains the original twelve source chapters outside the Tutorials book and their
195 image references. The user-approved follow-up independently constructs seven
usable chapters before publication. Six walkthroughs are now complete; one
remain in progress. The original twelve published tutorial passes above are
preserved and were not rerun. Until the book is rebuilt and its installed pages
verified, these remain qualified source chapters awaiting publication.

| Chapter source | Current disposition |
|---|---|
| `Tut_CreatingReport.xml` | **Passed** — [independent GUI construction/run/reopen](Qt6ParityValidation/help-creating-report-20261003.md), three reports and taught ScriptEvent header statements; supplied script read only after freeze. |
| `Tut_ForceModels.xml` | **Passed with stated fixed-step accuracy limitation** — [independent GUI construction/run/reopen](Qt6ParityValidation/help-force-models-20261003.md), six force setups and six0.1-day legs. Requested1e-12 SRP accuracy is not established; its stop guard continuation and warning are explicit. |
| `Tut_FormationRendezvous.xml` | **Passed** — [independent GUI construction/run/reopen](Qt6ParityValidation/help-formation-rendezvous-20261003.md), four spacecraft, two separately converged position/velocity targets, twenty Report commands and fresh source readback before comparison. This is not simultaneous six-state docking. |
| `Tut_LEOStationKeeping.xml` | **Passed** — [Parts1/2 actual-input construction/run/reopen](Qt6ParityValidation/help-leo-station-keeping-20261003.md), both ten-day histories and105 converged targets, frozen before reference comparison; the342km condition is a correction trigger, not a minimum-altitude guarantee. |
| `Tut_ACEStationKeeping.xml` | **Passed** — [independent GUI construction/run/reopen](Qt6ParityValidation/help-ace-station-keeping-20261003.md), seven converged targets, eight finite report records and two viewers; full comparison followed freeze. Original missing link and retained early attempts are explicit. |
| `Tut_LunarTransfer.xml` | **In progress** — independent resource/viewer/target construction, literal EarthToMoon.BdotT goal retained. No walkthrough pass claimed yet. |
| `Tut_MarsB-PlaneTargeting.xml` | **Passed with explicit frame/coast interpretation** — [older chapter actual-input walk](Qt6ParityValidation/help-mars-b-plane-legacy-20261003.md), two converged targets, original nine finite report rows and fresh reopen; changed-output-only diagnostic confirms the final Mars ellipse. Separate from the already-passed modern tutorial. |
| `Tut_L2Transfer.xml` | **Held** — original two sign brackets miss the required L2 plane; bounded failure/domain evidence retained, no ephemeris extension or objective change. |
| `Tut_MATLAB.xml` | **Deferred** — Running GMAT Scripts from MATLAB. |
| `Tut_MinFuelOrbitTransfer.xml` | **Deferred** — MATLAB/fmincon dependency. |
| `Tut_AlgebraicOptimization.xml` | **Unresolved legacy optimizer identity/dependency** — not a walkthrough pass. |
| `Tut_MinFuelLunarTransfer.xml` | **Unresolved legacy optimizer identity/dependency** — not a walkthrough pass. |

Each completed walkthrough freezes its own GUI-authored mission, result and fresh
readback before reading a supplied script. Differences remain documented instead
of retrofitting the construction to the reference. Original attempt failures,
bounded stops and per-session app identities remain in the ignored artifact tree.
[Implicit Earth gravity](Qt6ParityValidation/implicit-gravity-20261003.md),
[single-step propagation](Qt6ParityValidation/single-step-20261003.md),
[valid While insertion](Qt6ParityValidation/while-template-20261003.md) and
[coordinate Rename/window size](Qt6ParityValidation/axis-command-usability-20261003.md)
record concrete GUI blockers found by these walks and their focused repairs.
These additions preserve the nine replacement acceptance gates. Live host testing
remains stopped; Mutter client attribution, safe host condition and physical GPU
acceptance remain unproven. Windows, macOS and MATLAB remain deferred.
