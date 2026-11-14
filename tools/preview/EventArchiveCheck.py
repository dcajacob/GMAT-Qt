"""Check two independent event missions using the extracted release GUI."""
import datetime
import hashlib
import json
import math
import os
from pathlib import Path
import re
import subprocess


def check(root, target, output, environment):
    output.mkdir(parents=True, exist_ok=True)
    source = Path(__file__).resolve().parents[2]/'src/qtgui/tests'
    resources = root/'Contents/Resources' if target.startswith('macos') else root
    binary = root/'Contents/MacOS' if target.startswith('macos') else root/'bin'
    executable = root/'run-gmat' if target.startswith('linux') else binary/(
        'GmatQt.exe' if target.startswith('windows') else 'GmatQt')
    startup = root.parent/'gmat_startup_qt.txt' if target.startswith('macos') else binary/'gmat_startup_qt.txt'
    env = dict(environment, QT_QPA_PLATFORM='offscreen')
    if target.startswith('linux'):
        env.update(QT_QPA_PLATFORM='xcb', LIBGL_ALWAYS_SOFTWARE='1')
    if target.startswith('windows'):
        env['QT_QPA_FONTDIR'] = str(Path(os.environ.get('SystemRoot', 'C:/Windows'))/'Fonts')
    records = []
    stamp = r'(\d{2} [A-Za-z]{3} \d{4} \d{2}:\d{2}:\d{2}\.\d{3})'
    epoch = datetime.datetime(2000, 1, 1, 12)

    def seconds(value):
        return (datetime.datetime.strptime(value, '%d %b %Y %H:%M:%S.%f')-epoch).total_seconds()

    for family, key in [('Eclipse', 'Mixed'), ('Contact', 'MultiStation')]:
        fixture_path = source/('Nasa'+family+'ResultCases.json')
        fixture = json.loads(fixture_path.read_text())
        spec = next(c for c in fixture['cases'] if c['key'] == key)
        folder = output/family
        folder.mkdir()
        report = folder/'Intervals Δ.txt'
        final = folder/'Final Δ.csv'
        lines = ["SolarSystem.SPKFilename = '"+(resources/'data/planetary_ephem/spk/DE421AllPlanets.bsp').as_posix()+"';"]
        components = ['X', 'Y', 'Z', 'VX', 'VY', 'VZ']
        for name, values in [('Sat', spec['initial_state']), ('Other', spec['other_state'])]:
            lines += ['Create Spacecraft '+name+';', name+'.DateFormat = UTCGregorian;',
                      name+".Epoch = '01 Jan 2000 12:00:00.000';",
                      name+'.CoordinateSystem = EarthMJ2000Eq;', name+'.DisplayStateType = Cartesian;']
            lines += [f'{name}.{field} = {value:.17g};' for field, value in zip(components, values)]
        if family == 'Contact':
            lines += [f"Earth.EquatorialRadius = {fixture['equatorial_radius']:.17g};",
                      f"Earth.Flattening = {fixture['flattening']:.17g};"]
            for station in fixture['stations']:
                name = station['name']
                lines += ['Create GroundStation '+name+';', name+'.StateType = Spherical;',
                          name+'.HorizonReference = Ellipsoid;']
                lines += [f'{name}.Location{i} = {station[field]:.17g};'
                          for i, field in enumerate(['latitude', 'longitude', 'height'], 1)]
                lines += [f"{name}.MinimumElevationAngle = {station['minimum_elevation']:.17g};"]
        lines += ['Create ForceModel FM;', 'FM.PrimaryBodies = {};', 'FM.PointMasses = {Earth};',
                  'Create Propagator Prop;', 'Prop.FM = FM;', 'Prop.Type = RungeKutta89;',
                  'Prop.InitialStepSize = 10;', 'Prop.MinStep = 10;', 'Prop.MaxStep = 10;',
                  'Prop.Accuracy = 1e-12;', 'Create '+family+'Locator Events;',
                  "Events.Filename = '"+report.as_posix()+"';", 'Events.StepSize = 5;',
                  'Events.UseLightTimeDelay = false;', 'Events.UseStellarAberration = false;',
                  'Events.WriteReport = true;', 'Events.RunMode = Automatic;']
        if family == 'Eclipse':
            lines += ['Events.Spacecraft = Sat;', 'Events.OccultingBodies = {Earth};',
                      'Events.EclipseTypes = {Umbra, Penumbra};']
        else:
            lines += ['Events.Target = Sat;',
                      'Events.Observers = {North, East, Dark};', 'Events.ReportFormat = Legacy;']
        lines += ['Create ReportFile Truth;', "Truth.Filename = '"+final.as_posix()+"';",
                  'Truth.WriteHeaders = false;', 'Truth.FixedWidth = false;',
                  "Truth.Delimiter = ',';", 'Truth.Precision = 17;', 'BeginMissionSequence;',
                  'Propagate Prop(Sat,Other) {Sat.ElapsedSecs = 6500};',
                  'Report Truth '+' '.join('Sat.EarthMJ2000Eq.'+field for field in components)+';']
        script = folder/'Events Δ.script'
        script.write_text('\n'.join(lines)+'\n', encoding='utf-8')
        report.write_text('UNCHANGED sentinel\n', encoding='utf-8')
        command = [str(executable), str(script), '--startup', str(startup), '--run',
                   '--screenshot', str(folder/'result.png'), '--settings-dir', str(folder/'settings')]
        if target.startswith('linux'):
            command = ['xvfb-run', '-a', '-s', '-screen 0 1600x1000x24', *command]
        with (folder/'run.log').open('wb') as log:
            result = subprocess.run(command, cwd=folder, env=env, stdout=log,
                                    stderr=subprocess.STDOUT, timeout=180)
        if result.returncode:
            raise RuntimeError(f'{family} archive mission failed with {result.returncode}')
        values = [float(v) for v in final.read_text().strip().split(',')]
        assert len(values) == 6
        assert all(math.isfinite(a) and abs(a-b) < (2e-5 if i < 3 else 2e-8)
                   for i, (a, b) in enumerate(zip(values, spec['final_state'])))
        text = report.read_text(encoding='utf-8')
        actual = {}
        if family == 'Eclipse':
            pattern = re.compile('^'+stamp+r'\s+'+stamp+r'\s+([\deE.+-]+)\s+(\S+)\s+(Umbra|Penumbra|Antumbra)\s+(\d+)\s+([\deE.+-]+)\s*$', re.M)
            observed = [dict(start=seconds(m[1]), end=seconds(m[2]), duration=float(m[3]),
                body=m[4], type=m[5], group=int(m[6])) for m in pattern.finditer(text)]
            assert len(observed) == len(spec['events'])
            for a, b in zip(observed, spec['events']):
                assert all(a[field] == b[field] for field in ['body', 'type', 'group'])
                assert all(abs(a[field]-b[field]) < spec['time_tolerance'] for field in ['start', 'end'])
                assert abs(a['duration']-b['duration']) < spec['duration_tolerance']
            actual['intervals'] = observed
        else:
            pattern = re.compile('^'+stamp+r'\s+'+stamp+r'\s+([\deE.+-]+)\s*$', re.M)
            for section in text.split('Observer: ')[1:]:
                observer = section.splitlines()[0].strip()
                assert observer in spec['observers'] and observer not in actual
                actual[observer] = [dict(start=seconds(m[1]), end=seconds(m[2]), duration=float(m[3]))
                                    for m in pattern.finditer(section)]
                expected = spec['events'][observer]
                assert len(actual[observer]) == len(expected)
                for a, b in zip(actual[observer], expected):
                    assert all(abs(a[field]-b[field]) < spec['time_tolerance'] for field in ['start', 'end'])
                    assert abs(a['duration']-b['duration']) < spec['duration_tolerance']
                if not expected:
                    assert 'There are no contact events' in section
            assert set(actual) == set(spec['observers'])
        records.append({'family': family, 'case': key, 'status': 'passed',
            'fixture_sha256': hashlib.sha256(fixture_path.read_bytes()).hexdigest(),
            'report_sha256': hashlib.sha256(report.read_bytes()).hexdigest(),
            'final_state': values, 'observed': actual,
            'time_tolerance': spec['time_tolerance'], 'duration_tolerance': spec['duration_tolerance']})
    receipt = {'status': 'passed', 'platform': target, 'cases': records,
               'scope': 'Extracted archive event-plugin loading, independent timing, Unicode reports and final states'}
    (output/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    return receipt
