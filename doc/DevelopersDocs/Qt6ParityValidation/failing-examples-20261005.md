# Failing examples — repairs and explanations, 2026-10-05

This resolves or explains all **27 non-completing source rows** in the [previous failure ledger](shipped-examples-20261002.failures.md): 17 failed builds, five unexpected execution failures, two intentional tutorial Stops and three timeouts. Six previously non-completing examples now execute successfully. Seventeen need absent/deferred components; two are deliberately stopped tutorial checkpoints; Starlink lacks historical inputs; the L2 transfer still fails its event-defined evaluation. No claim of full GUI or general scientific qualification follows.

Work is on `codex/qt6-gui` in `/home/dan/GIT/GMAT-Qt`. The original checkout and its application were untouched. Only the three TLE example scripts and a small dated input subset change; application/core/plugin/startup code did not change, so no rebuild or unrelated passing checks were required. All new children use the actual current GmatQt, isolated startup/output/settings and offscreen software rendering. No live-host test was needed.

## Verified repairs and recovered completions

| Example | New result | Result checks and precise scope |
| --- | --- | --- |
| FalconSats | Build0.470s; full run17.455s, exit0 | All three spacecraft retained, 4320 one-minute propagation steps; 74 dated Colorado Springs visibility rows. June2020 elements and epoch replace inconsistent 2019/2020 data extrapolated to today. |
| Falconsat7Contacts | Build0.420s; full run15.302s, exit0 | All4320 steps retained; 78 Colorado Springs and135 White Sands rows. All dated within23–26June2020, monotonically ordered, finite azimuth in[0,360) and elevation in(0,90]. |
| GSFCSats | Final build0.470s; full run67.414s, exit0 | All22 named spacecraft and full five-day propagation retained, using23June2020. The first93.245s run had a missed December2019 mission assignment and is excluded from the corrected-date claim; that attempt and exact source hash remain archived. |
| VF13ad MarsPatchConic | Unchanged source,10.845s, exit0 | Optimizer converged; one final report row, cost7.246938849141352. Six last reported constraint residuals are below configured FeasibilityTolerance1; max0.847586631775. |
| VF13ad MarsLaunchWindowAnalysis | Unchanged source,129.083s, exit0 | All20 original launch windows, finite output and20 optimizer convergence notices; last printed residuals satisfy the configured bound. |
| Yukon MarsLaunchWindowAnalysis | Unchanged source,151.272s, exit0 | All20 original launch windows, finite output and20 convergence notices. **Physical feasibility is not proved:** last printed position residuals reach 22.5516413897km despite the success notices. Accepted nominal states need separate validation before claiming physical patch-point agreement. |

The optimizer retries allow1200s. Each completed below300s on the current runtime; the earlier300s/7200s timeouts remain preserved. This evidence does not identify which runtime, host-load or presentation difference caused the older slow execution. No window count, objective, optimizer, force model or convergence tolerance was changed. The VF13ad results establish reported convergence and bounded residuals, not an independently proved global optimum.

## Historical TLE scope and data

The [bundled subset](../../../plugins/TLEPropagatorPlugin/TLE/ExampleSatellites-2020-06-23.txt) is4200 bytes with25 spacecraft. Each three-line record is byte-identical to the original June2020 catalog in [Thinking Systems’ R2020a distribution](https://www.thinksysinc.com/archives/GMATR2020aDownloads.html); all50 element-line checksums validate. Its [README](../../../plugins/TLEPropagatorPlugin/TLE/README.md) documents archive/catalog hashes and how to use it. All lists, propagator settings, station calculations and three-/five-day durations remain.

These are explicitly historical repaired demonstrations, not a reproduction of December2019 GSFC geometry or current tracking. The retained [CelesTrak SATCAT record](tle-dependency-inputs-20261002.md) gives FalconSat-7 decay on2July2021; its unchanged current-time contact scenario cannot represent a still-orbiting satellite. Live tracking requires recent elements for spacecraft still in orbit. Original sources and failures are retained, not relabeled as successful original scenarios.

## Why the other examples do not complete here

- **MarsGRAM2005 (one):** interpreter cannot create the atmosphere model; `libMarsGRAM`, its compatible legacy FORTRAN namelist and `binFiles` data are absent. The local ForceModel contract explicitly requires them. NASA lists [Mars-GRAM2010](https://software.nasa.gov/software/MFS-33158-1) as a General Public Release, but that does not establish availability/compatibility of this2005 GMAT integration. Replacing it with another atmosphere would change the example.
- **MATLAB/Fmincon (four):** `PLUGIN_MATLABINTERFACE=OFF`; required MatlabFunction/FminconOptimizer factories are unavailable. MATLAB remains deferred. Compatible MATLAB/interface components and the Optimization Toolbox for Fmincon are prerequisites.
- **SNOPT (three):** required SNOPT solver factory/integration/libraries are absent from the selected runtime. Compatible licensed components must be supplied and enabled. Another optimizer would demonstrate a different integration.
- **CSALT/OptimalControl (nine):** `GMAT_INCLUDE_CSALT=OFF`; requested OptimalControl/Trajectory/Phase factories are unavailable. `CMakeLists.txt:408-431` requires both `snopt7_cpp` and `snopt7`; neither library is found in this checkout. Supplying dependencies, enabling/building the components and executing the originals are needed; one CMake flag alone is insufficient. These are first demonstrated blockers, not a promise that no later issue will appear.
- **Two MultipleShooting Step1 scripts:** their Stop is an intentional initial-guess QA checkpoint. `Tut_OptimalLunarFlyby.xml:1093-1107` explains it; Step2 explicitly removes it at1218-1224. Retain raw stopped/failed status and use the already completed later tutorial steps for optimization. Removing this Stop would destroy the instructional checkpoint.
- **Starlink:** all117 spacecraft and12December2019 epochs remain unchanged. Matching historical `active.txt` is absent. The June2020 catalog is missing STARLINK-46 and STARLINK-67 and cannot reproduce the original epoch. The prepared117 NORAD IDs and archival-access instructions are in the [input record](tle-dependency-inputs-20261002.md). No fabricated elements, removed spacecraft or human archive request is substituted.
- **L2 transfer:** the10000/16000 B·T energy bracket has opposite signs, but trial15109.375 never reaches the required L2-entry plane; unbounded propagation exhausts DE405 at A1epoch95008.539427940 before final targeting. The existing160-day diagnostic remains on a bound Earth-return orbit. [Exact phase evidence](l2-tutorial-bracket-2026-10-02.md) establishes why the current evaluation fails. A reachable continuous transfer seed/bracket is still needed; longer ephemerides, a time cap or relaxed objectives would not prove this transfer.

## Complete disposition table

| Original non-completing source | Current disposition |
| --- | --- |
| `application/samples/Ex_MarsGRAM2005.script` | Missing component |
| `application/samples/Ex_Yukon_MarsLaunchWindowAnalysis.script` | Completed on current runtime |
| `application/samples/NeedMatlab/Ex_CallMatlabFunctions.script` | MATLAB deferred |
| `application/samples/NeedMatlab/Ex_MatlabEnv.script` | MATLAB deferred |
| `application/samples/NeedSNOPT/Ex_AlgebraicOptimization.script` | Missing component |
| `application/samples/NeedSNOPT/Ex_MinFuelLunarTransfer.script` | Missing component |
| `application/samples/NeedSNOPT/Ex_OptFiniteBurn.script` | Missing component |
| `application/samples/NeedVF13ad/Ex_MarsLaunchWindowAnalysis.script` | Completed on current runtime |
| `application/samples/NeedVF13ad/Ex_MarsPatchConic.script` | Completed on current runtime |
| `application/samples/NeedVF13ad/Tut_MultipleShootingTutorial_Step1.script` | Intended Step 1 Stop |
| `application/samples/OptimalControl/Ex_CelestialBodyRendezvous_Mars.script` | Missing component |
| `application/samples/OptimalControl/Ex_EMTGSpacecraft_SCOpt_BusPowerType0.script` | Missing component |
| `application/samples/OptimalControl/Ex_EMTGSpacecraft_SCOpt_PowerSupplyCurve1.script` | Missing component |
| `application/samples/OptimalControl/Ex_EMTGSpacecraft_SCOpt_ThrustType1.script` | Missing component |
| `application/samples/OptimalControl/Ex_EarthToMarsSOI_C3Eq0_CSALTTutorial.script` | Missing component |
| `application/samples/OptimalControl/Ex_IntegratedFlyby_MarsFlyby.script` | Missing component |
| `application/samples/OptimalControl/Ex_OCPropTest_GEO_4x4.script` | Missing component |
| `application/samples/OptimalControl/Ex_PCLaunch_ConstrainedC3AndDLA_EarthLaunch_EarthOrigin.script` | Missing component |
| `application/samples/OptimalControl/Ex_Phase_Type_ImplicitRKOrder6.script` | Missing component |
| `plugins/TLEPropagatorPlugin/samples/NeedTlePropagator/FalconSats.script` | Completed historical repair |
| `plugins/TLEPropagatorPlugin/samples/NeedTlePropagator/Falconsat7Contacts.script` | Completed historical repair |
| `plugins/TLEPropagatorPlugin/samples/NeedTlePropagator/GSFCSats.script` | Completed historical repair |
| `plugins/TLEPropagatorPlugin/samples/NeedTlePropagator/Starlink.script` | Missing December 2019 TLE data |
| `application/docs/help/files/scripts/Ex_LowEarthMinFuelTransfer.script` | MATLAB deferred |
| `application/docs/help/files/scripts/Ex_MinFuelLunarCapture.script` | MATLAB deferred |
| `application/docs/help/files/scripts/LunarTransferToL2-tutorial.script` | L2 event-domain failure |
| `application/docs/help/files/scripts/Tut_MultipleShootingTutorial_Step1.script` | Intended Step 1 Stop |

## Evidence boundaries and remaining scientific requirements

The [receipt](failing-examples-20261005.receipt.json) preserves every original failed/timeout stage, its source/runtime identities, exact reason and required next action, plus all new stages and result checks. The [raw archive](failing-examples-20261005.raw.zip) retains every new attempt, helpers, commands, logs, reports, screenshots, frozen originals, checksum/provenance checks and the source-matched current inventory. All27 retained original diagnostic logs checked against their recorded hashes; none was missing or mismatched.

Four earlier passing sample sources changed **before this task** relative to the October2 manifest: VF13ad Step5 in both sample/Help locations, ElectricPropulsionModelling, and InterSpacecraftTracking. Their earlier passing records are retained in history; they are not automatically claimed as current-source evidence and were not rerun here. The merged current-source inventory therefore has143 matching passed builds,17 build failures and4 unverified builds;139 matching completed runs,4 stopped/failed runs and21 unverified/unattempted runs. Of the latter21,17 are prevented by build failures and4 reflect those earlier source changes. This is a provenance reconciliation, not four newly observed failures or a fresh full-corpus matrix.

The separate independently constructed [Lunar lesson](help-lunar-transfer-20261003.md) remains held: both recorded LOI variants miss ECC0±1e-5, and roughly88.07° Moon-inertial inclination does not establish exactly90° polar capture. The [synchronized thrust case](multi-thrust-live-20261002.md) retains its propagator failure and unproved step-boundary attribution; the independent case is already verified. These are not among the27 corpus rows, and this work does not silently close them or the FOV/domain limits.

All nine full-replacement gates remain in scope. Existing authorized host progress retains its own limits; historical GMAT client attribution and Mutter repair remain unproved. Windows/macOS/MATLAB remain deferred. No engine rewrite, host upgrade, completed tutorial repetition or unrelated passing audit occurred.
