"""Run the existing independent eclipse/contact tests with packaged libraries."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time

import BuildPreviewPackage as builder


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_event_results(args, payload, root, binary, executables, environment, patches):
    output = args.output.resolve()
    source = builder.REPO/'src/qtgui/tests'
    data = root/'Contents/Resources/data' if args.platform.startswith('macos') else root/'data'
    # The original tests derive their physical-input root from application/bin.
    # Supply that layout without changing their assertions or fixture data.
    input_root = output/'physical-inputs'
    application = input_root/'application'
    (application/'bin').mkdir(parents=True, exist_ok=True)
    alias = application/'data'
    if not alias.exists():
        if os.name == 'nt':
            subprocess.run(['cmd', '/c', 'mklink', '/J', str(alias), str(data)], check=True)
        else:
            alias.symlink_to(data, target_is_directory=True)
    startup = payload/'gmat_startup_qt.txt' if args.platform.startswith('macos') else binary/'gmat_startup_qt.txt'
    startup_text = startup.read_text()
    startup_text = re.sub(r'^(\w+\s*=\s*)([^\n]*)$', lambda match: match[1]+(
        str((binary/match[2].strip()).resolve()) if match[2].strip().startswith(('../', './'))
        else match[2]), startup_text, flags=re.M)
    if args.platform.startswith('macos'):
        def plugin_path(match):
            name = Path(match[1].strip()).name
            library = root/'Contents/Frameworks'/(name+'.R2026a.dylib')
            return 'PLUGIN = '+str(library.with_suffix('')) if library.is_file() else match[0]
        startup_text = re.sub(r'^PLUGIN\s*=\s*(.+)$', plugin_path, startup_text, flags=re.M)
    rows = []
    protected = {}
    physical_inputs = {}
    text_input_formats = {}
    for family, executable in zip(['Eclipse', 'Contact'], executables):
        fixture = source/('Nasa'+family+'ResultCases.json')
        cases = json.loads(fixture.read_text())
        for name in ['Nasa'+family+'ResultTests.cpp', fixture.name, family+'GeometryOracle.py']:
            protected[name] = digest(source/name)
        for path, expected in cases['physical_inputs']['kernels'].items():
            actual = digest(input_root/path)
            if actual != expected:
                raw = (input_root/path).read_bytes()
                if Path(path).suffix.lower() not in {'.tls', '.tpc', '.txt'} or hashlib.sha256(raw.replace(b'\r\n', b'\n')).hexdigest() != expected:
                    raise RuntimeError('Packaged physical input differs from the frozen oracle: '+path)
                text_input_formats[path] = {'packaged_sha256': actual,
                    'canonical_lf_sha256': expected, 'difference': 'CRLF line endings only'}
                cases['physical_inputs']['kernels'][path] = actual
            physical_inputs[path] = actual
        executed_fixture = output/fixture.name
        executed_fixture.write_text(json.dumps(cases, indent=2)+'\n')
        for case in cases['cases']:
            name = 'QtGui.Nasa'+family+'Result'+case['key']
            selected = os.environ.get('GMAT_EVENT_CASES', '').split(',')
            if selected != [''] and name not in selected:
                continue
            folder = output/name
            folder.mkdir(exist_ok=False)
            selected = startup_text
            for key, value in {'OUTPUT_PATH': str(folder)+'/',
                               'LOG_FILE': str(folder/'GmatLog.txt'),
                               'PERSONALIZATION_FILE': str(folder/'MyGmat.ini')}.items():
                selected = re.sub(r'^'+key+r'\s*=.*$', lambda _: key+' = '+value,
                                  selected, flags=re.M)
            private_startup = folder/'startup.txt'
            private_startup.write_text(selected)
            child_environment = dict(environment, QT_QPA_PLATFORM='offscreen',
                GMAT_QT_ACCEPTANCE_MODE='offscreen', LIBGL_ALWAYS_SOFTWARE='1',
                GMAT_QT_ACCEPTANCE_DIR=str(folder),
                GMAT_QT_ACCEPTANCE_STARTUP_ORIGINAL=str(application/'bin/startup.txt'))
            command = [str(executable), str(private_startup), str(executed_fixture), case['key']]
            started = time.monotonic()
            with (folder/'test.log').open('wb') as log:
                try:
                    result = subprocess.run(command, cwd=binary, env=child_environment,
                                            stdout=log, stderr=subprocess.STDOUT, timeout=180)
                    code = result.returncode
                except subprocess.TimeoutExpired:
                    code = 'timeout'
            proof_path = folder/'proof.json'
            proof = json.loads(proof_path.read_text()) if proof_path.is_file() else {}
            passed = code == 0 and proof.get('outcome') == 'passed'
            row = {'name': name, 'status': 'passed' if passed else 'failed', 'exit_code': code,
                   'seconds': time.monotonic()-started, 'executable_sha256': digest(executable),
                   'time_tolerance': case['time_tolerance'],
                   'duration_tolerance': case['duration_tolerance'],
                   'artifacts': {p.name: digest(p) for p in folder.iterdir() if p.is_file()}}
            rows.append(row)
            (folder/'result.json').write_text(json.dumps(row, indent=2)+'\n')
            print(name, row['status'], flush=True)
            if not passed:
                print((folder/'test.log').read_text(errors='replace')[-3000:], flush=True)
    if any(digest(source/name) != expected for name, expected in protected.items()):
        raise RuntimeError('Protected event expectations changed during execution')
    metadata_path = root/'Contents/Resources/PYTHON-API.json' if args.platform.startswith('macos') else root/'PYTHON-API.json'
    receipt = {'platform': args.platform, 'status': 'passed' if all(r['status']=='passed' for r in rows) else 'failed',
        'source_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=builder.REPO, text=True).strip(),
        'archive_source': json.loads(metadata_path.read_text())['source_commit'],
        'archive_sha256': digest(args.archive), 'extracted_root': str(root),
        'targeted_library_patches': patches, 'graphics_profile': 'offscreen',
        'tests': rows, 'protected_sources': protected, 'physical_inputs': physical_inputs,
        'text_input_formats': text_input_formats,
        'oracle_policy': 'Reuse frozen independently computed geometry and physical-input hashes',
        'full_regression_run': False}
    (output/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps({'platform': args.platform, 'status': receipt['status'], 'cases': len(rows)}), flush=True)
    return 0 if receipt['status']=='passed' else 1
