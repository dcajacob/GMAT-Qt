"""Exercise actual C-loaded SPK/numerical discovery and count-policy configuration."""
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

CASES = {
    'prepared-first': ('Native', 'single', 'prepared', 0, 0),
    'stopped-first': ('Native', 'single', 'stopped', 0, -4),
    'nested-first': ('Native', 'single', 'nested', 0, 0),
    'single-group-one': ('SingleGroup', 'single', 'prepared', 0, 0),
    'single-setup-repeated': ('SingleSetup', 'repeated', 'prepared', 0, 0),
    'native-distinct': ('Native', 'distinct', 'prepared', 0, 0),
    'single-setup-distinct-rejected': ('SingleSetup', 'distinct', 'prepared', 0, -2),
    'single-group-repeated-rejected': ('SingleGroup', 'repeated', 'prepared', 0, -2),
    'single-group-distinct-rejected': ('SingleGroup', 'distinct', 'prepared', 0, -2),
    'quoted-policy': ("'SingleSetup'", 'repeated', 'prepared', 0, 0),
    'policy-only-backward-synchronized': ('SingleGroup', 'single', 'backward', 0, 0),
    'invalid-policy-rejected': ('InvalidPolicy', 'single', 'prepared', -2, 0),
    'duplicate-policy-rejected': ('Native, EphemerisPolicy = SingleGroup', 'single', 'prepared', -2, 0),
}
SHA = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()


def fixture(name, asset):
    policy, grouping, form, load_status, run_status = CASES[name]
    text = 'Earth.Mu = 398600.4418;\n'
    epochs = {'A': 21545., 'B': 21545., 'E': 21545.+120/86400., 'F': 21545.+120/86400.}
    for craft, radius in [('A', 7000.), ('B', 8000.), ('E', 10000.), ('F', 11000.)]:
        text += (f'Create Spacecraft {craft};\n{craft}.DateFormat = A1ModJulian;\n'
                 f"{craft}.Epoch = '{epochs[craft]:.17g}';\n{craft}.CoordinateSystem = EarthMJ2000Eq;\n"
                 f'{craft}.DisplayStateType = Cartesian;\n')
        state = [radius, 0, 0, 0, math.sqrt(398600.4418/radius), 0]
        text += ''.join(f'{craft}.{key} = {value:.17g};\n' for key, value in
                        zip(['X', 'Y', 'Z', 'VX', 'VY', 'VZ'], state))
        if craft in ('E', 'F'):
            text += f"{craft}.NAIFId = -199990;\n{craft}.OrbitSpiceKernelName = {{'{asset}'}};\n"
    for i in (1, 2):
        text += (f'Create ForceModel FM{i};\nFM{i}.PrimaryBodies = {{}};\nFM{i}.PointMasses = {{Earth}};\n'
                 f'FM{i}.Drag = None;\nFM{i}.SRP = Off;\nCreate Propagator P{i};\nP{i}.FM = FM{i};\n')
        text += (f'Create Propagator Read{i};\nRead{i}.Type = SPK;\nRead{i}.CentralBody = Earth;\n'
                 f'Read{i}.EpochFormat = TDBModJulian;\nRead{i}.StartEpoch = FromSpacecraft;\nRead{i}.StepSize = 60;\n')
    text += 'Create Variable Flag;\nFlag = 1;\n'
    groups = {'single': 'Read1(E)', 'repeated': 'Read1(E) Read1(F)', 'distinct': 'Read1(E) Read2(F)'}[grouping]
    modifiers = 'Synchronized BackProp ' if form == 'backward' else ''
    stop = '' if form == 'backward' else 'E.ElapsedSecs = 60, '
    ephemeris = f'Propagate {modifiers}{groups} {{{stop}EphemerisPolicy = {policy}}};\n'
    numerical = "Propagate P1(A) {A.ElapsedSecs = 20};\nPropagate P2(B, 'STM') {B.ElapsedSecs = 30};\n"
    text += 'BeginMissionSequence;\nStop;\n' if form == 'stopped' else 'PrepareMissionSequence;\n'
    if form == 'nested':
        text += 'If Flag > 0\n'+ephemeris+'If Flag < 0\n'+numerical+'Else\nEndIf\nElse\nEndIf\n'
    else:
        text += ephemeris+numerical
    models = [dict(name='FM1', dimension=6, state=[7000., 0., 0., 0., math.sqrt(398600.4418/7000.), 0.]),
              dict(name='FM2', dimension=42, state=[8000., 0., 0., 0., math.sqrt(398600.4418/8000.), 0.])]
    expected = dict(case=name, policy=policy, grouping=grouping, form=form, load_status=load_status,
                    run_status=run_status, spacecraft_A1Mjd=epochs, models=models,
                    SPK_body_id=-199990, SPK_center_id=399, SPK_file=str(asset), SPK_sha256=SHA(asset),
                    scope='No SPK step expected; prove actual configuration and discovery without epoch advance.')
    manifest = f'{load_status} {run_status} 4 2\n'
    manifest += ''.join(f'{craft} {epoch:.17g}\n' for craft, epoch in epochs.items())
    for model in models:
        manifest += f"{model['name']} {model['dimension']} 6 21545\n"+' '.join(f'{v:.17g}' for v in model['state'])+'\n'
    return text, manifest, expected


def run(args):
    repo = args.repo.resolve();startup = args.startup.resolve();app = startup.parent.parent
    executable = args.executable.resolve();asset = args.kernel.resolve()
    assert SHA(asset) == '0708f58440f7b463484540e92bce6de3f544a1dd0cb0109b570e7d2e1db8c6d7'
    output = (args.output or args.output_root/('ephemeris-'+uuid.uuid4().hex[:12])).resolve()
    output.mkdir(parents=True, exist_ok=False)
    sources = {p for folder in ('src/base', 'src/gmatutil', 'plugins') for p in (repo/folder).rglob('*')
               if p.is_file() and p.suffix in ('.c', '.cpp', '.h', '.hpp', '.inc')}
    sources.update(repo/n for n in ['src/qtgui/CMakeLists.txt', 'src/qtgui/tests/CapiPropagateAcceptanceTests.c',
                                   'src/qtgui/tests/CapiEphemerisDiscoveryTests.c', 'src/qtgui/tests/RunCapiEphemerisDiscovery.py'])
    images = {executable}
    for p in app.rglob('*'):
        if p.is_file() and not p.is_symlink():
            with p.open('rb') as f:
                if f.read(4) == b'\x7fELF': images.add(p.resolve())
    inputs = [*sorted(sources), *sorted(images), startup, asset]
    before = {str(p): SHA(p) for p in inputs}
    (output/'inputs-before.json').write_text(json.dumps(before, indent=2)+'\n')
    with tarfile.open(output/'sources.tar.gz', 'w:gz') as bundle:
        for p in sorted(sources): bundle.add(p, arcname=str(p.relative_to(repo)), recursive=False)
    results = []
    for name in args.case or CASES:
        d = output/name;d.mkdir();work = d/'session';work.mkdir();(work/'output').mkdir()
        text = startup.read_text()
        for key, value in [('ROOT_PATH', str(app)+'/'), ('OUTPUT_PATH', str(work/'output')+'/'), ('PERSONALIZATION_FILE', str(work/'personalization.ini'))]:
            text = re.sub(r'^'+key+r'\s*=.*$', lambda m: key+' = '+value, text, flags=re.M)
        text = re.sub(r'^PLUGIN\s*=\s*(.+?)\s*$', lambda m: 'PLUGIN = '+str((startup.parent/m[1]).resolve()), text, flags=re.M)
        (work/'gmat_startup_file.txt').write_text(text)
        script, manifest, expected = fixture(name, asset)
        (work/'acceptance.script').write_text(script);(work/'expectations.txt').write_text(manifest)
        (work/'recovery.script').write_text(fixture('prepared-first', asset)[0])
        (d/'oracle.json').write_text(json.dumps(expected, indent=2)+'\n')
        fixture_before = {str(p): SHA(p) for p in work.iterdir() if p.is_file()}
        command = [str(executable), str(work/'acceptance.script'), str(work/'expectations.txt'), str(work/'recovery.script'), name]
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
        record = dict(case=name, status='passed' if ok else 'failed', returncode=code, proofs=proofs,
                      loaded_project_images={n: before[n] for n in loaded if n in before}, unbound_project_images=sorted(set(loaded)-set(before)),
                      fixture_inputs_before=fixture_before, fixture_inputs_after=fixture_after)
        (d/'receipt.json').write_text(json.dumps(record, indent=2)+'\n');results.append(record)
        print(json.dumps({k: record[k] for k in ('case', 'status', 'returncode')}), flush=True)
    after = {str(p): SHA(p) for p in inputs};(output/'inputs-after.json').write_text(json.dumps(after, indent=2)+'\n')
    ok = before == after and all(r['status'] == 'passed' for r in results)
    suite = ET.Element('testsuite', name='CapiEphemerisDiscovery', tests=str(len(results)), failures=str(sum(r['status'] != 'passed' for r in results)))
    for record in results:
        case = ET.SubElement(suite, 'testcase', name='QtAcceptance.CapiEphemerisDiscovery.'+record['case'])
        if record['status'] != 'passed' or before != after: ET.SubElement(case, 'failure', message='C API, fixture or runtime identity assertion failed')
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
    parser.add_argument('--kernel', type=Path, required=True);parser.add_argument('--case', choices=CASES, action='append')
    dest = parser.add_mutually_exclusive_group(required=True)
    dest.add_argument('--output', type=Path);dest.add_argument('--output-root', type=Path)
    raise SystemExit(run(parser.parse_args()))
