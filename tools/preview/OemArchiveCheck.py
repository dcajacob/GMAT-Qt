"""Check sparse Hermite output through the extracted release executable."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess


def check(root, target, output, environment):
    output.mkdir(parents=True, exist_ok=True)
    fixture_path = Path(__file__).resolve().parents[2] / 'src/qtgui/tests/NasaOemShortWindowCases.json'
    fixture = json.loads(fixture_path.read_text())
    spec = next(case for case in fixture['cases'] if case['key'] == 'NarrowAtLast')
    binary = root / 'Contents/MacOS' if target.startswith('macos') else root / 'bin'
    executable = root / 'run-gmat' if target.startswith('linux') else binary / (
        'GmatQt.exe' if target.startswith('windows') else 'GmatQt')
    startup = root.parent / 'gmat_startup_qt.txt' if target.startswith('macos') else binary / 'gmat_startup_qt.txt'
    oem = output / 'Sparse Hermite Δ.oem'
    script = output / 'Sparse Hermite Δ.script'
    script.write_text(f"""Earth.Mu = {spec['mu']:.17g};
Create Spacecraft Sat;
Sat.Id = '{fixture['object_id']}';
Sat.DateFormat = UTCGregorian;
Sat.Epoch = '01 Jan 2000 12:00:00.000';
Sat.CoordinateSystem = EarthMJ2000Eq;
Sat.DisplayStateType = Cartesian;
Sat.X = 7000;
Sat.Y = 0;
Sat.Z = 0;
Sat.VX = 0;
Sat.VY = {spec['speed']:.17g};
Sat.VZ = 0;
Create ForceModel FM;
FM.PrimaryBodies = {{}};
FM.PointMasses = {{Earth}};
Create Propagator PropA;
PropA.FM = FM;
PropA.Type = RungeKutta89;
PropA.InitialStepSize = 5;
PropA.MinStep = 5;
PropA.MaxStep = 5;
PropA.Accuracy = 1e-12;
Create EphemerisFile Export;
Export.Spacecraft = Sat;
Export.FileFormat = CCSDS-OEM;
Export.FormatVersion = '1.0';
Export.Filename = '{oem.as_posix()}';
Export.CoordinateSystem = EarthMJ2000Eq;
Export.Interpolator = Hermite;
Export.InterpolationOrder = 7;
Export.StepSize = IntegratorSteps;
Export.EpochFormat = UTCGregorian;
Export.InitialEpoch = '{spec['initial_epoch']}';
Export.FinalEpoch = '{spec['final_epoch']}';
BeginMissionSequence;
{spec['mission']}
""", encoding='utf-8')
    env = dict(environment, QT_QPA_PLATFORM='offscreen')
    if target.startswith('linux'):
        env.update(QT_QPA_PLATFORM='xcb', LIBGL_ALWAYS_SOFTWARE='1')
    if target.startswith('windows'):
        env['QT_QPA_FONTDIR'] = str(Path(os.environ.get('SystemRoot', 'C:/Windows')) / 'Fonts')
    command = [str(executable), str(script), '--startup', str(startup), '--run',
               '--screenshot', str(output / 'result.png'), '--settings-dir', str(output / 'settings')]
    if target.startswith('linux'):
        command = ['xvfb-run', '-a', '-s', '-screen 0 1600x1000x24', *command]
    with (output / 'run.log').open('wb') as log:
        result = subprocess.run(command, cwd=output, env=env, stdout=log,
                                stderr=subprocess.STDOUT, timeout=180)
    if result.returncode and target.startswith('linux'):
        debug_env = dict(env,
            LD_LIBRARY_PATH=str(root / 'bin') + ':' + str(root / 'lib'),
            QT_PLUGIN_PATH=str(root / 'qt/plugins'),
            QT_QPA_PLATFORM_PLUGIN_PATH=str(root / 'qt/plugins/platforms'),
            OSG_LIBRARY_PATH=str(root / 'osgPlugins'))
        diagnostic = ['xvfb-run', '-a', 'gdb', '--batch', '-ex', 'run', '-ex', 'thread apply all bt',
                      '--args', str(binary / 'GmatQt'), *command[5:]]
        with (output / 'debugger.log').open('wb') as log:
            subprocess.run(diagnostic, cwd=output, env=debug_env, stdout=log,
                           stderr=subprocess.STDOUT, timeout=180)
    if result.returncode:
        raise RuntimeError(f'Extracted Hermite mission failed with exit {result.returncode}')
    text = oem.read_text(encoding='utf-8')
    assert re.search(r'INTERPOLATION\s*=\s*HERMITE', text)
    assert re.search(r'INTERPOLATION_DEGREE\s*=\s*3\b', text)
    rows = [line.split() for line in text.splitlines() if re.match(r'^\d{4}-\d\d-\d\dT', line)]
    expected = spec['expected_segments'][0]
    assert len(rows) == len(expected)
    errors = []
    for actual, reference in zip(rows, expected):
        assert len(actual) == 7
        seconds = float(actual[0].split(':')[-1])
        assert abs(seconds-reference[0]) < .002
        error = [abs(float(value)-truth) for value, truth in zip(actual[1:], reference[1:])]
        assert max(error[:3]) < spec['position_tolerance'], error
        assert max(error[3:]) < spec['velocity_tolerance'], error
        errors.append(error)
    receipt = {'status': 'passed', 'case': spec['key'], 'interpolator': 'Hermite',
               'fixture_sha256': hashlib.sha256(fixture_path.read_bytes()).hexdigest(),
               'max_position_error_km': max(max(error[:3]) for error in errors),
               'max_velocity_error_km_s': max(max(error[3:]) for error in errors),
               'position_tolerance': spec['position_tolerance'],
               'velocity_tolerance': spec['velocity_tolerance'],
               'unicode_output': oem.name, 'serialized_degree': 3, 'records': len(rows)}
    (output / 'verification.json').write_text(json.dumps(receipt, indent=2)+'\n')
    return receipt
