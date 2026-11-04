"""Remaining grouped backend contexts, reusing shipped/prequalified fixtures.

Expected motion is a closed-form point-mass circle. Contact event times reuse
the existing independently derived, kernel-pinned contact oracle unchanged.
"""
import argparse
import datetime
import hashlib
import json
import math
from pathlib import Path
import re


def cases():
    root = Path(__file__).resolve().parent
    repo = root.parent.parent.parent
    original = root/'mission-command-names/Propagate.json'
    old = json.loads(original.read_text())
    setup, body = old['script'].split('BeginMissionSequence;')
    props = [line for line in body.splitlines() if line.startswith('Propagate ')]
    report = next(line for line in body.splitlines() if line.startswith("Report 'Witness' Values "))
    assert len(props) == 2
    base = '\n'.join(props)+'\n'
    result = []

    def add(family, command, mission, extra='', tail=(0, 1, 0, 0), **flags):
        result.append(dict(family=family, command=command,
            script=setup+extra+'BeginMissionSequence;\n'+mission,
            expected_tail=list(tail), velocity_increment=[0, 0, 0],
            fixture_origin='mission-command-names/Propagate.json',
            fixture_origin_sha256=hashlib.sha256(original.read_bytes()).hexdigest(), **flags))

    tvhf = repo/'application/samples/SupportFiles/Ex_FileInterface_TVHF.sv'
    add('Set', 'Set', "Set Sat Reader (Data = {'Cr'});\nReport Extra Sat.A1ModJulian Sat.Cr Sat.X Sat.Y Sat.Z Sat.VX Sat.VY Sat.VZ;\n"+
        base+'Cost = Sat.Cr;\n'+report+'\n',
        "Sat.Cr = 1.8;\nCreate FileInterface Reader;\nReader.Filename = '@INPUT@';\nReader.Format = 'TVHF_ASCII';\nExtra.Delimiter = ',';\n",
        (0, 1, 1.4, 0), support_files=[dict(name='input.sv', text=tvhf.read_text())],
        expected_extra_rows=[[21545, 1.4, 7000, 0, 0, 0, math.sqrt(398600.4415/7000), 0]],
        extra_tolerances=[1e-11, 1e-12, 1e-8, 1e-8, 1e-8, 1e-10, 1e-10, 1e-10],
        source_inputs={'application/samples/SupportFiles/Ex_FileInterface_TVHF.sv':hashlib.sha256(tvhf.read_bytes()).hexdigest()})

    history = ('BeginThrust {Segment}\nStart_Epoch = 01 Jan 2000 12:00:00.000\n'
        'Thrust_Vector_Coordinate_System = EarthMJ2000Eq\nThrust_Vector_Interpolation_Method = None\n'
        'Mass_Flow_Rate_Interpolation_Method = None\nModelAccelAndMassRate\n'
        '0.0 0 0 0 0.5\n120.0 0 0 0 0.5\nEndThrust {Segment}\n')
    extra = ("Sat.DateFormat = UTCGregorian;\nSat.Epoch = '01 Jan 2000 12:00:00.000';\n"
        "Create ChemicalTank Fuel;\nFuel.FuelMass = 100;\nSat.Tanks = {Fuel};\n"
        "Create ThrustSegment Segment;\nSegment.MassSource = {Fuel};\nCreate ThrustHistoryFile History;\n"
        "History.FileName = '@HISTORY@';\nHistory.AddThrustSegment = {Segment};\nExtra.Delimiter = ',';\n")
    add('FileThrust', 'BeginFileThrust', 'BeginFileThrust History(Sat);\n'+props[0]+
        '\nReport Extra Sat.ElapsedSecs Sat.Fuel.FuelMass;\nEndFileThrust History(Sat);\n'+props[1]+
        '\nReport Extra Sat.ElapsedSecs Sat.Fuel.FuelMass;\nCost = Sat.Fuel.FuelMass;\n'+report+'\n',
        extra, (0, 1, 95, 0), required_types=['EndFileThrust'],
        support_files=[dict(name='history.thf', text=history)],
        expected_extra_rows=[[10, 95], [20, 95]], extra_tolerances=[1e-4, 1e-8],
        oracle='Zero acceleration with independent 0.5 kg/s mass flow for first10s, then coast. Orbit stays the original point-mass circle; this is command activation/deactivation, not thrust-vector physics qualification.')

    contact_path = root/'NasaContactResultCases.json'
    contact = json.loads(contact_path.read_text())
    case = next(c for c in contact['cases'] if c['key'] == 'SingleStation')
    extra = "Sat.DateFormat = UTCGregorian;\nSat.Epoch = '01 Jan 2000 12:00:00.000';\nSat.NAIFId = -100070;\n"
    for axis, value in zip(['X', 'Y', 'Z', 'VX', 'VY', 'VZ'], case['initial_state']):
        extra += f'Sat.{axis} = {value:.17g};\n'
    extra += ("SolarSystem.SPKFilename = '@DATA_ROOT@/planetary_ephem/spk/DE421AllPlanets.bsp';\n"
        f"Earth.EquatorialRadius = {contact['equatorial_radius']:.17g};\nEarth.Flattening = {contact['flattening']:.17g};\n")
    station = next(s for s in contact['stations'] if s['name'] == 'East')
    extra += 'Create GroundStation East;\nEast.StateType = Spherical;\nEast.HorizonReference = Ellipsoid;\n'
    for key, value in zip(['Location1', 'Location2', 'Location3', 'MinimumElevationAngle'],
                          [station['latitude'], station['longitude'], station['height'], station['minimum_elevation']]):
        extra += f'East.{key} = {value:.17g};\n'
    extra += ("Create ContactLocator Events;\nEvents.Target = Sat;\nEvents.Observers = {East};\n"
        "Events.RunMode = Manual;\nEvents.ReportFormat = Legacy;\nEvents.WriteReport = true;\nEvents.UseEntireInterval = true;\n"
        "Events.UseLightTimeDelay = false;\nEvents.UseStellarAberration = false;\nEvents.StepSize = 5;\nEvents.Filename = '@EVENT@';\n")
    add('FindEvents', 'FindEvents', 'Propagate P(Sat) {Sat.ElapsedSecs = 6500};\nFindEvents Events {Append = false};\n'+report+'\n',
        extra, orbit_duration=6500, expected_cartesian=case['final_state'],
        contact_reference=case, physical_inputs=contact['physical_inputs']['kernels'],
        source_inputs={'src/qtgui/tests/NasaContactResultCases.json':hashlib.sha256(contact_path.read_bytes()).hexdigest()})

    navigation = repo/'application/samples/Navigation/Ex_FilterSmoother_GpsPosVec.script'
    nav = navigation.read_text().split('BeginMissionSequence', 1)[0]
    nav = re.sub(r'(?<!\.)\bFM\b', 'NavigationFM', nav)
    nav = re.sub(r'\bProp\b', 'NavProp', nav)

    def set_value(key, value):
        nonlocal nav
        pattern = r'^'+re.escape(key)+r'\s*=[^\n]*'
        assert re.search(pattern, nav, re.M), key
        nav = re.sub(pattern, lambda _: key+' = '+value+';', nav, flags=re.M)

    radius, mu, duration = 7000, 398600.4415, 600
    speed, rate = math.sqrt(mu/radius), math.sqrt(mu/radius**3)
    state0 = [radius, 0, 0, 0, speed, 0]
    state1 = [radius*math.cos(rate*duration), radius*math.sin(rate*duration), 0,
              -speed*math.sin(rate*duration), speed*math.cos(rate*duration), 0]
    for satellite in ['SimSat', 'EstSat']:
        for axis, value in zip(['X', 'Y', 'Z', 'VX', 'VY', 'VZ'], state0):
            set_value(satellite+'.'+axis, f'{value:.17g}')
    for key, value in [('Sim.FinalEpoch', "'10 Jun 2010 00:10:00.000'"),
        ('Sim.MeasurementTimeStep', '60'), ('Sim.AddNoise', 'Off'), ('EstSat.SolveFors', '{CartesianState}'),
        ('EKF.ShowProgress', 'True'), ('FPS.ShowProgress', 'True'), ('EKF.ShowAllResiduals', 'On'), ('FPS.ShowAllResiduals', 'On'),
        ('EKF.ReportFile', "'@FILTER@'"), ('FPS.ReportFile', "'@SMOOTHER@'"),
        ('EKF.OutputWarmStartFile', "'@WARM@'"), ('FilterCdReport.Filename', "'@SCRATCH@'"),
        ('NavProp.InitialStepSize', '10'), ('NavProp.MaxStep', '10')]:
        set_value(key, value)
    nav = nav.replace("'Ex_FilterSmoother_GpsPosVec.gmd'", "'@OBS@'")
    set_value('NavigationFM.PrimaryBodies', '{}')
    set_value('NavigationFM.SRP', 'Off')
    nav = re.sub(r'^NavigationFM\.(?:GravityField|Drag)\.[^\n]*\n', '', nav, flags=re.M)
    # PropSetup clones its ODEModel on assignment. Complete the force definition
    # before NavProp.FM, rather than changing the original after it was cloned.
    nav = nav.replace('Create Propagator NavProp;',
        'NavigationFM.PointMasses = {Earth};\nNavigationFM.Drag = None;\n'
        'NavigationFM.RelativisticCorrection = Off;\nCreate Propagator NavProp;')
    nav += "Extra.Delimiter = ',';\n"
    # JD-2430000, with TAI-UTC=34 s in2010 and the existing GMAT A1-TAI offset.
    utc = datetime.datetime(2010, 6, 10, tzinfo=datetime.timezone.utc)
    epoch = 10587.5 + utc.timestamp()/86400 + (34+0.0343817)/86400
    fields = 'A1ModJulian X Y Z VX VY VZ'.split()
    def state_report(phase, satellite):
        return f'Count = {phase};\nReport Extra Count '+' '.join(satellite+'.'+f for f in fields)+';\n'
    mission = base+'RunSimulator Sim;\n'+state_report(1, 'SimSat')
    mission += 'EstSat.OrbitErrorCovariance = diag([1e-2 1e-2 1e-2 2.5e-7 2.5e-7 2.5e-7]);\n'
    mission += 'RunEstimator EKF;\n'+state_report(2, 'EstSat')+'RunSmoother FPS;\n'+state_report(3, 'EstSat')+report+'\n'
    add('Navigation', 'RunSimulator', mission, nav, (3, 1, 0, 0),
        required_types=['RunEstimator', 'RunSmoother'], navigation=True,
        expected_extra_rows=[[1, epoch+duration/86400, *state1], [2, epoch+duration/86400, *state1], [3, epoch+duration/86400, *state1]],
        extra_tolerances=[0, 2e-10, 2e-5, 2e-5, 2e-5, 2e-8, 2e-8, 2e-8],
        expected_observations=11,
        source_inputs={'application/samples/Navigation/Ex_FilterSmoother_GpsPosVec.script':hashlib.sha256(navigation.read_bytes()).hexdigest()})
    return result


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output-dir', type=Path, required=True)
    a = p.parse_args()
    a.output_dir.mkdir(parents=True, exist_ok=True)
    for case in cases():
        path = a.output_dir/(case['family']+'.json')
        if path.exists():
            raise SystemExit('Refusing to overwrite '+str(path))
        path.write_text(json.dumps(case, indent=2)+'\n')
    print('Generated four contexts covering all seven remaining loaded backend types')
