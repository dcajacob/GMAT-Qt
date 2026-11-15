"""Run bounded Propagate/discovery witnesses through the actual integrated C ABI.

No GUI, copied runtime images or original-row credit. Retain all case outcomes.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import subprocess
import tarfile
import uuid
import xml.etree.ElementTree as ET

MU = 398600.4418
CASES = ('prepared-graph', 'prepared-multiple-setups', 'completed-mixed', 'executed-branches', 'backward',
         'scalar-goals', 'scalar-measured', 'stopped-graph')
SHA = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()


def fixture(name):
    radii = {'A': 7000., 'B': 8000., 'C': 9000., 'D': 10000.}
    text = f'Earth.Mu = {MU};\n'
    for craft, radius in radii.items():
        text += (f'Create Spacecraft {craft};\n{craft}.DateFormat = A1ModJulian;\n'
                 f"{craft}.Epoch = '21545';\n{craft}.CoordinateSystem = EarthMJ2000Eq;\n"
                 f'{craft}.DisplayStateType = Cartesian;\n')
        state = [radius, 0, 0, 0, math.sqrt(MU/radius), 0]
        text += ''.join(f'{craft}.{field} = {value:.17g};\n' for field, value in
                        zip(('X', 'Y', 'Z', 'VX', 'VY', 'VZ'), state))
    for index in (1, 2, 3):
        text += (f'Create ForceModel FM{index};\nFM{index}.PrimaryBodies = {{}};\n'
                 f'FM{index}.PointMasses = {{Earth}};\nFM{index}.Drag = None;\n'
                 f'FM{index}.SRP = Off;\nCreate Propagator P{index};\n'
                 f'P{index}.FM = FM{index};\nP{index}.InitialStepSize = 10;\n'
                 f'P{index}.Accuracy = 1e-12;\n')
    text += ('Create Variable Flag Index Goal Never;\nFlag = 1;\nGoal = 60;\nNever = 2;\n'
             'Create Array Grid[2,2];\nGrid(1,1) = 30;\nGrid(1,2) = 3;\nGrid(2,1) = 4;\n')
    durations = dict.fromkeys(radii, 0.)
    if name == 'prepared-multiple-setups':
        text += ("PrepareMissionSequence;\n"
                 "Propagate Synchronized P1(A) P2(C, D) P3(B, 'STM') {A.ElapsedSecs = 30};\n")
    elif name == 'prepared-graph':
        text += ("PrepareMissionSequence;\nIf Flag > 0\nIf Flag < 0\n"
                 "Propagate P1(A) {A.ElapsedSecs = 30};\nElse\nFor Index = 1:1:2\n"
                 "Propagate P3(B, 'STM') {B.ElapsedSecs = 40};\nEndFor\nEndIf\nElse\nEndIf\n"
                 "Propagate P2(C, D) {C.ElapsedSecs = 50};\n")
    else:
        text += 'BeginMissionSequence;\n'
        if name == 'executed-branches':
            text += ("If Flag > 0\nPropagate P1(A) {A.ElapsedSecs = 30};\nElse\n"
                     "Propagate P3(B, 'STM') {B.ElapsedSecs = 40};\nEndIf\n"
                     "For Index = 1:1:1\nPropagate P2(C, D) {C.ElapsedSecs = 20};\nEndFor\n")
            durations.update(A=30., C=20., D=20.)
        elif name == 'stopped-graph':
            text += ("Propagate P1(A) {A.ElapsedSecs = 10};\nStop;\nIf Flag < 0\n"
                     "Propagate P3(B, 'STM') {B.ElapsedSecs = 40};\nElse\nEndIf\n"
                     "Propagate P2(C, D) {C.ElapsedSecs = 50};\n")
            durations.update(A=10.)
        elif name == 'scalar-measured':
            text += ("Propagate P1(A) {Never = 999, A.ElapsedSecs = 20};\n"
                     "Propagate P3(B, 'STM') {Grid(1,2) = 999, B.ElapsedSecs = 25};\n"
                     "Propagate P2(C, D) {Grid(2,1) = Goal, C.ElapsedSecs = 35};\n")
            durations.update(A=20., B=25., C=35., D=35.)
        elif name == 'scalar-goals':
            text += ("Propagate P1(A) {A.ElapsedSecs = Goal};\n"
                     "Propagate P3(B, 'STM') {B.ElapsedSecs = Grid(1,1)};\n"
                     "Propagate P2(C, D) {C.ElapsedSecs = 45};\n")
            durations.update(A=60., B=30., C=45., D=45.)
        else:
            backward = name == 'backward'
            durations.update(A=-30. if backward else 30., B=-40. if backward else 40.,
                             C=-50. if backward else 50., D=-50. if backward else 50.)
            direction = 'BackProp ' if backward else ''
            for setup, arguments, craft in [('P1', 'A', 'A'), ('P3', "B, 'STM'", 'B'), ('P2', 'C, D', 'C')]:
                text += f'Propagate {direction}{setup}({arguments}) {{{craft}.ElapsedSecs = {durations[craft]:g}}};\n'
    models = []
    for model, dimension, craft_names in [('FM1', 6, ['A']), ('FM3', 42, ['B']), ('FM2', 12, ['C', 'D'])]:
        # STM is command-wide in the native Propagate parser: all three
        # setups in this one command carry their spacecraft's matrices.
        if name == 'prepared-multiple-setups':
            dimension = 42*len(craft_names)
        state = []
        for craft in craft_names:
            radius = radii[craft]
            angular_rate = math.sqrt(MU/radius**3)
            angle = angular_rate*durations[craft]
            state.extend([radius*math.cos(angle), radius*math.sin(angle), 0.,
                          -radius*angular_rate*math.sin(angle), radius*angular_rate*math.cos(angle), 0.])
        models.append(dict(name=model, dimension=dimension, cartesian=len(state), state=state,
                           epoch=21545.+durations[craft_names[0]]/86400.))
    crafts = {craft: 21545.+seconds/86400. for craft, seconds in durations.items()}
    expected = dict(case=name, run_status=-4 if name == 'stopped-graph' else 0,
                    spacecraft=crafts, models=models, mu=MU,
                    position_tolerance_km=2e-5, velocity_tolerance_km_s=3e-8,
                    epoch_tolerance_s=2e-6, independent_oracle='literal planar circular two-body equations')
    manifest = f"{len(models)} {len(crafts)} {expected['run_status']}\n"
    manifest += ''.join(f'{craft} {epoch:.17g}\n' for craft, epoch in crafts.items())
    for model in models:
        manifest += f"{model['name']} {model['dimension']} {model['cartesian']} {model['epoch']:.17g}\n"
        manifest += ' '.join(f'{value:.17g}' for value in model['state'])+'\n'
    return text, manifest, expected


def run(args):
    repo = args.repo.resolve()
    startup = args.startup.resolve()
    app = startup.parent.parent
    executable = args.executable.resolve()
    output = (args.output or args.output_root/('propagate-'+uuid.uuid4().hex[:12])).resolve()
    output.mkdir(parents=True, exist_ok=False)
    source_paths = {p for folder in ('src/base', 'src/gmatutil', 'plugins') for p in (repo/folder).rglob('*')
                    if p.is_file() and p.suffix in ('.c', '.cpp', '.h', '.hpp', '.inc')}
    source_paths.update([repo/'src/qtgui/CMakeLists.txt', Path(__file__).resolve(),
                         Path(__file__).with_name('CapiPropagateAcceptanceTests.c').resolve()])
    images = {executable}
    for p in app.rglob('*'):
        if p.is_file() and not p.is_symlink():
            with p.open('rb') as stream:
                if stream.read(4) == b'\x7fELF':
                    images.add(p.resolve())
    bindings = [*sorted(source_paths), *sorted(images), startup]
    before = {str(p): SHA(p) for p in bindings}
    (output/'inputs-before.json').write_text(json.dumps(before, indent=2)+'\n')
    with tarfile.open(output/'sources.tar.gz', 'w:gz') as bundle:
        for p in sorted(source_paths):
            bundle.add(p, arcname=str(p.relative_to(repo)), recursive=False)
    results = []
    for name in args.case or CASES:
        case_dir = output/name
        case_dir.mkdir()
        work = case_dir/'session'
        work.mkdir()
        (work/'output').mkdir()
        text = startup.read_text()
        for key, value in [('ROOT_PATH', str(app)+'/'), ('OUTPUT_PATH', str(work/'output')+'/'),
                           ('PERSONALIZATION_FILE', str(work/'personalization.ini'))]:
            text = re.sub(r'^'+key+r'\s*=.*$', lambda m: key+' = '+value, text, flags=re.M)
        text = re.sub(r'^PLUGIN\s*=\s*(.+?)\s*$',
                      lambda m: 'PLUGIN = '+str((startup.parent/m[1]).resolve()), text, flags=re.M)
        (work/'gmat_startup_file.txt').write_text(text)
        script, manifest, expected = fixture(name)
        script_path = work/'acceptance.script'
        script_path.write_text(script)
        (work/'expectations.txt').write_text(manifest)
        (case_dir/'oracle.json').write_text(json.dumps(expected, indent=2)+'\n')
        call = [str(executable), str(script_path), str(work/'expectations.txt'),
                'combined' if name in ('backward', 'scalar-goals') else 'separate']
        env = dict(os.environ)
        env['LD_LIBRARY_PATH'] = str(app/'bin')+':'+env.get('LD_LIBRARY_PATH', '')
        (case_dir/'command.json').write_text(json.dumps(dict(command=call, cwd=str(work),
                                           LD_LIBRARY_PATH=env['LD_LIBRARY_PATH']), indent=2)+'\n')
        try:
            process = subprocess.run(call, cwd=work, env=env, capture_output=True, text=True, timeout=90)
            code, log = process.returncode, process.stdout+process.stderr
        except subprocess.TimeoutExpired as error:
            code = 124
            def decoded(value):
                return value.decode(errors='replace') if isinstance(value, bytes) else value or ''
            log = decoded(error.stdout)+decoded(error.stderr)+'\nCLIENT TIMEOUT\n'
        (case_dir/'client.log').write_text(log)
        proofs = [json.loads(line[len('NASA_CHECK '):]) for line in log.splitlines() if line.startswith('NASA_CHECK ')]
        loaded = sorted({str(Path(line[len('NASA_LOADED '):]).resolve()) for line in log.splitlines()
                         if line.startswith('NASA_LOADED ') and line[len('NASA_LOADED '):].startswith(str(repo)+'/')})
        models = [json.loads(line[len('NASA_MODEL '):]) for line in log.splitlines() if line.startswith('NASA_MODEL ')]
        passes = code == 0 and len(proofs) == 1 and proofs[0]['outcome'] == 'passed'
        passes = passes and len(models) == 3 and loaded and set(loaded) <= set(before)
        passes = passes and str((app/'bin/libCInterface.so').resolve()) in loaded
        comparisons = 0
        for actual, wanted in zip(models, expected['models']):
            passes = passes and actual['name'] == wanted['name'] and actual['dimension'] == wanted['dimension']
            for i, value in enumerate(actual['state'][:wanted['cartesian']]):
                tolerance = 2e-5 if i % 6 < 3 else 3e-8
                passes = passes and math.isfinite(value) and abs(value-wanted['state'][i]) <= tolerance
                comparisons += 1
        result = dict(case=name, status='passed' if passes else 'failed', returncode=code,
                      proofs=proofs, independent_state_scalars=comparisons,
                      loaded_project_images={p: before[p] for p in loaded if p in before},
                      unbound_loaded_project_images=sorted(set(loaded)-set(before)),
                      command=call, input_sha256={str(p): SHA(p) for p in work.iterdir() if p.is_file()})
        (case_dir/'receipt.json').write_text(json.dumps(result, indent=2)+'\n')
        print(json.dumps({k: result[k] for k in ('case', 'status', 'returncode', 'independent_state_scalars')}), flush=True)
        results.append(result)
    after = {str(p): SHA(p) for p in bindings}
    (output/'inputs-after.json').write_text(json.dumps(after, indent=2)+'\n')
    unchanged = before == after
    suite = ET.Element('testsuite', name='CapiPropagate', tests=str(len(results)),
                       failures=str(sum(r['status'] != 'passed' for r in results)))
    for result in results:
        case = ET.SubElement(suite, 'testcase', name='QtAcceptance.CapiPropagate.'+result['case'])
        if result['status'] != 'passed' or not unchanged:
            ET.SubElement(case, 'failure', message='C client assertion, independent result, image identity or input guard failed')
    ET.ElementTree(suite).write(output/'junit.xml', encoding='unicode')
    summary = dict(status='passed' if unchanged and all(r['status'] == 'passed' for r in results) else 'failed',
                   results=results, inputs_unchanged=unchanged, source_paths=len(source_paths),
                   runtime_images=len(images), GUI_executions=0, screenshots=0, runtime_copies=0,
                   original_assertion_changes=0, new_requirement_credit=0,
                   full_NASA_qualified=False, full_current_build_qualified=False, goal_complete=False)
    (output/'receipt.json').write_text(json.dumps(summary, indent=2)+'\n')
    return 0 if summary['status'] == 'passed' else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--startup', type=Path, required=True)
    parser.add_argument('--case', choices=CASES, action='append')
    destination = parser.add_mutually_exclusive_group(required=True)
    destination.add_argument('--output', type=Path)
    destination.add_argument('--output-root', type=Path)
    raise SystemExit(run(parser.parse_args()))
