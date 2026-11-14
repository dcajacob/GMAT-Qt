"""Retain native measured-scalar clock, wrapper and reinitialization evidence."""
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

CASES = ('clock-single', 'clock-formation', 'clock-multiple', 'clock-goal-index', 'wrappers',
         'reinitialize-angle', 'reinitialize-days')
SHA = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()


def fixture(mode):
    text = 'Earth.Mu = 398600.4418;\n'
    starts = {'A': 21545., 'B': 21545., 'C': 21546.}
    radii = {'A': 7000., 'B': 8000., 'C': 9000.}
    for craft, radius in radii.items():
        text += (f'Create Spacecraft {craft};\n{craft}.DateFormat = A1ModJulian;\n'
                 f"{craft}.Epoch = '{starts[craft]:g}';\n{craft}.CoordinateSystem = EarthMJ2000Eq;\n"
                 f'{craft}.DisplayStateType = Cartesian;\n')
        values = [radius, 0, 0, 0, math.sqrt(398600.4418/radius), 0]
        text += ''.join(f'{craft}.{field} = {value:.17g};\n' for field, value in
                        zip(['X', 'Y', 'Z', 'VX', 'VY', 'VZ'], values))
    text += ('Create ForceModel FM;\nFM.PrimaryBodies = {};\nFM.PointMasses = {Earth};\n'
             'FM.Drag = None;\nFM.SRP = Off;\nCreate Propagator P;\nP.FM = FM;\n'
             'P.InitialStepSize = 10;\nP.Accuracy = 1e-12;\n'
             'Create Variable Never Value Index;\nNever = 2;\nValue = 2;\nIndex = 1;\n'
             'Create Array Grid[2,2];\nGrid(1,2) = 3;\nGrid(2,2) = 9;\n')
    prepared = mode in ('wrappers', 'reinitialize-angle', 'reinitialize-days')
    if mode == 'clock-formation':
        text += 'Create Formation Team;\nTeam.Add = {A, B};\n'
    text += 'PrepareMissionSequence;\n' if prepared else 'BeginMissionSequence;\n'
    expected = dict(mode=mode, base_epoch=21545., conditions=[], spacecraft=[],
                    position_tolerance_km=2e-5, velocity_tolerance_km_s=3e-8,
                    epoch_tolerance_seconds=2e-6, new_requirement_credit=0)
    if mode == 'wrappers':
        text += "Propagate 'Bound wrappers' P(A) {Value = 999, Grid(Index,2) = 999, A.ElapsedSecs = 20};\n"
    elif mode == 'reinitialize-angle':
        text += "Propagate 'Angle metadata' P(A) {A.Earth.TA = 359};\n"
    elif mode == 'reinitialize-days':
        text += "Propagate 'Time metadata' P(A) {A.ElapsedDays = 1};\n"
    else:
        group = 'Team' if mode == 'clock-formation' else 'A, B' if mode == 'clock-multiple' else 'A'
        stops = ['Never = C.A1ModJulian', 'Grid(Index,2) = C.ElapsedSecs', 'A.ElapsedSecs = 20']
        expected['conditions'] = [dict(lhs='Never', user_scalar=True, value=2.),
                                  dict(lhs='Grid(Index,2)', user_scalar=True, value=3.),
                                  dict(lhs='A.ElapsedSecs', user_scalar=False, value=20.)]
        if mode == 'clock-goal-index':
            text += 'Index = 2;\n'
            stops = ['A.ElapsedSecs = Grid(Index,2)']
            expected['conditions'] = [dict(lhs='A.ElapsedSecs', user_scalar=False, value=9.)]
        if mode == 'clock-multiple':
            stops.append('B.ElapsedSecs = 35')
            expected['conditions'].append(dict(lhs='B.ElapsedSecs', user_scalar=False, value=20.))
        text += f"Propagate 'Clock routing' P({group}) {{{', '.join(stops)}}};\n"
        for craft, radius in radii.items():
            seconds = (9. if craft == 'A' else 0.) if mode == 'clock-goal-index' else 20. if craft == 'A' or (craft == 'B' and mode != 'clock-single') else 0.
            angular_rate = math.sqrt(398600.4418/radius**3);angle = seconds*angular_rate
            state = [radius*math.cos(angle), radius*math.sin(angle), 0.,
                     -radius*angular_rate*math.sin(angle), radius*angular_rate*math.cos(angle), 0.]
            expected['spacecraft'].append(dict(name=craft, epoch=starts[craft]+seconds/86400., state=state))
    return text, expected


def run(args):
    repo = args.repo.resolve();startup = args.startup.resolve();app = startup.parent.parent
    executable = args.executable.resolve()
    output = (args.output or args.output_root/('scalar-stops-'+uuid.uuid4().hex[:12])).resolve()
    output.mkdir(parents=True, exist_ok=False)
    sources = {p for folder in ('src/base', 'src/gmatutil', 'plugins') for p in (repo/folder).rglob('*')
               if p.is_file() and p.suffix in ('.c', '.cpp', '.h', '.hpp', '.inc')}
    sources.update(repo/n for n in ['src/qtgui/CMakeLists.txt', 'src/qtgui/tests/ScalarStopBoundaryTests.cpp',
                                   'src/qtgui/tests/RunScalarStopBoundaries.py'])
    images = {executable}
    for p in app.rglob('*'):
        if p.is_file() and not p.is_symlink():
            with p.open('rb') as f:
                if f.read(4) == b'\x7fELF': images.add(p.resolve())
    inputs = [*sorted(sources), *sorted(images), startup]
    before = {str(p): SHA(p) for p in inputs}
    (output/'inputs-before.json').write_text(json.dumps(before, indent=2)+'\n')
    with tarfile.open(output/'sources.tar.gz', 'w:gz') as bundle:
        for p in sorted(sources): bundle.add(p, arcname=str(p.relative_to(repo)), recursive=False)
    results = []
    for mode in args.case or CASES:
        d = output/mode;d.mkdir();work = d/'session';work.mkdir();(work/'output').mkdir()
        text = startup.read_text()
        for key, value in [('ROOT_PATH', str(app)+'/'), ('OUTPUT_PATH', str(work/'output')+'/'), ('PERSONALIZATION_FILE', str(work/'personalization.ini'))]:
            text = re.sub(r'^'+key+r'\s*=.*$', lambda m: key+' = '+value, text, flags=re.M)
        text = re.sub(r'^PLUGIN\s*=\s*(.+?)\s*$', lambda m: 'PLUGIN = '+str((startup.parent/m[1]).resolve()), text, flags=re.M)
        (work/'gmat_startup_file.txt').write_text(text)
        script, expected = fixture(mode)
        (work/'acceptance.script').write_text(script);(work/'expected.json').write_text(json.dumps(expected, indent=2)+'\n')
        fixture_before = {str(p): SHA(p) for p in work.iterdir() if p.is_file()}
        command = [str(executable), str(work/'acceptance.script'), mode, str(work/'expected.json'), str(d/'proof.json')]
        env = dict(os.environ);env['LD_LIBRARY_PATH'] = str(app/'bin')+':'+env.get('LD_LIBRARY_PATH', '')
        (d/'command.json').write_text(json.dumps(dict(command=command, cwd=str(work), LD_LIBRARY_PATH=env['LD_LIBRARY_PATH']), indent=2)+'\n')
        try:
            p = subprocess.run(command, cwd=work, env=env, capture_output=True, text=True, timeout=60)
            code, log = p.returncode, p.stdout+p.stderr
        except subprocess.TimeoutExpired as e:
            decode = lambda v: v.decode(errors='replace') if isinstance(v, bytes) else v or ''
            code, log = 124, decode(e.stdout)+decode(e.stderr)+'\nCLIENT TIMEOUT\n'
        (d/'client.log').write_text(log)
        fixture_after = {n: SHA(n) for n in fixture_before}
        proofs = [json.loads(line[len('NASA_CHECK '):]) for line in log.splitlines() if line.startswith('NASA_CHECK ')]
        loaded = sorted({str(Path(line[len('NASA_LOADED '):]).resolve()) for line in log.splitlines()
                         if line.startswith('NASA_LOADED ') and line[len('NASA_LOADED '):].startswith(str(repo)+'/')})
        ok = code == 0 and fixture_before == fixture_after and len(proofs) == 1 and proofs[0]['outcome'] == 'passed'
        ok = ok and loaded and set(loaded) <= set(before) and str((app/'bin/libCInterface.so').resolve()) in loaded
        if (d/'proof.json').exists(): ok = ok and json.loads((d/'proof.json').read_text()) == (proofs[0] if proofs else None)
        else: ok = False
        record = dict(case=mode, status='passed' if ok else 'failed', returncode=code, proofs=proofs,
                      loaded_project_images={n: before[n] for n in loaded if n in before}, unbound_project_images=sorted(set(loaded)-set(before)),
                      fixture_inputs_before=fixture_before, fixture_inputs_after=fixture_after)
        (d/'receipt.json').write_text(json.dumps(record, indent=2)+'\n');results.append(record)
        print(json.dumps(dict(case=mode, status=record['status'], returncode=code, error=proofs[0].get('error') if proofs else None)), flush=True)
    after = {str(p): SHA(p) for p in inputs};(output/'inputs-after.json').write_text(json.dumps(after, indent=2)+'\n')
    ok = before == after and all(r['status'] == 'passed' for r in results)
    suite = ET.Element('testsuite', name='ScalarStopBoundaries', tests=str(len(results)), failures=str(sum(r['status'] != 'passed' for r in results)))
    for record in results:
        case = ET.SubElement(suite, 'testcase', name='QtAcceptance.ScalarStopBoundaries.'+record['case'])
        if record['status'] != 'passed' or before != after: ET.SubElement(case, 'failure', message='Native scalar, fixture or runtime identity assertion failed')
    ET.ElementTree(suite).write(output/'junit.xml', encoding='unicode')
    result = dict(status='passed' if ok else 'failed', results=results, inputs_unchanged=before == after,
                  source_paths=len(sources), runtime_images=len(images), GUI_executions=0, screenshots=0,
                  runtime_copies=0, original_assertion_changes=0, new_requirement_credit=0,
                  full_NASA_qualified=False, full_current_build_qualified=False, goal_complete=False)
    (output/'receipt.json').write_text(json.dumps(result, indent=2)+'\n')
    return 0 if ok else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--executable', type=Path, required=True);parser.add_argument('--startup', type=Path, required=True)
    parser.add_argument('--case', choices=CASES, action='append')
    dest = parser.add_mutually_exclusive_group(required=True)
    dest.add_argument('--output', type=Path);dest.add_argument('--output-root', type=Path)
    raise SystemExit(run(parser.parse_args()))
