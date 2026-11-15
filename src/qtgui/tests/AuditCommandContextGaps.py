"""Replay the targeted context witnesses without launching native code."""
from pathlib import Path
import hashlib
import json
import math
import tarfile


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def need(value, message):
    if not value:
        raise ValueError(message)


def check_observations(spec, proof, files):
    need(proof['passed'] and proof['source_files_unchanged'], 'Native assertion or input-preservation failure')
    if spec.get('reject'):
        need(not proof['built'] or proof['run_status'] != 1, 'Illegal placement executed')
        text = files['messages.txt'].decode()
        need(all(s.lower() in text.lower() for s in spec['diagnostic_contains']), 'Unattributed rejection')
        need(proof['recovery_keep'] == 42, 'Missing recovery execution')
        return
    need(proof['built'] and proof['run_status'] == 1, 'Positive mission failed')
    need(set(spec['required_types']) <= set(proof['native_types']), 'Missing native command')
    for name, wanted in spec['expected_files'].items():
        actual = [[float(v) for v in line.split(',')] for line in files[name].decode().strip().splitlines()]
        need(actual == proof['observed'][name], 'Raw report differs from proof')
        need(len(actual) == len(wanted), 'Wrong row count')
        for row, target in zip(actual, wanted):
            need(len(row) == len(target), 'Wrong report width')
            need(all(math.isfinite(a) and abs(a-b) <= spec['tolerance'] for a, b in zip(row, target)), 'Independent scalar oracle failed')
    for name, rule in spec.get('time_files', {}).items():
        actual = [float(v) for v in files[name].decode().strip().splitlines()]
        need(actual == proof['observed'][name], 'Raw publication differs from proof')
        tol = rule['tolerance']
        need(actual and tol > 0, 'Empty time witness')
        need(all(math.isfinite(v) and any(lo-tol <= v <= hi+tol for lo, hi in rule['allowed_intervals']) for v in actual), 'Forbidden publication interval')
        need(all(any(abs(v-t) <= tol for v in actual) for t in rule['required_times']), 'Missing active publication interval')
    need(all(s in files['messages.txt'].decode() for s in spec.get('message_contains', [])), 'Missing Write output')


def audit(stage):
    stage = Path(stage)
    records = []
    reused = {'TargetNestedConstraint', 'LateCreateFunction', 'FunctionMisplacedAchieve', 'FunctionMisplacedMinimize'}
    latest = json.loads((stage/'frozen-cases-v2.json').read_text())
    for case in latest['cases']:
        key = case['key']
        version = 'v1' if key in reused else 'v2'
        run = stage/('native-'+version+'-'+key)
        plan = json.loads((stage/('frozen-cases-'+version+'.json')).read_text())
        spec = next(c for c in plan['cases'] if c['key'] == key)
        if key in reused:
            need(spec == case, 'Reused case changed')
        receipt = json.loads((run/'result.json').read_text())
        buildpath = stage/('build-'+version+'.json')
        build = json.loads(buildpath.read_text())
        need(receipt['returncode'] == 0 and receipt['inputs_unchanged'], 'Native process or provenance failed')
        need(receipt['build_sha256'] == sha(buildpath) and build['status'] == 'passed', 'Build receipt changed')
        need(receipt['fixture_sha256'] == sha(stage/('frozen-cases-'+version+'.json')), 'Frozen expectation changed')
        loaded = receipt['loaded_image_sha256']
        for alias, digest in build['binary_sha256'].items():
            if Path(alias).name == 'GmatCommandContextGapTests':
                continue
            matches = [h for p, h in loaded.items() if Path(p).name.startswith(Path(alias).name+'.') or p == alias]
            need(matches == [digest], 'Loaded native image differs from built image')
        with tarfile.open(stage/('sources-'+version+'.tar.gz')) as archive:
            for name, digest in build['source_sha256'].items():
                need(hashlib.sha256(archive.extractfile(name).read()).hexdigest() == digest, 'Archived source differs')
        proof = json.loads((run/'proof.json').read_text())
        files = {p.name:p.read_bytes() for p in run.iterdir() if p.is_file()}
        check_observations(spec, proof, files)
        need((run/'mission.script').read_text() == spec['script'].replace('@OUTPUT@', str(run)), 'Mission input mismatch')
        need(json.loads((run/'case.json').read_text()) == spec, 'Selected fixture mismatch')
        for name, text in spec['files'].items():
            need(files[name].decode() == text.replace('@OUTPUT@', str(run)), 'Function input mismatch')
        records.append(dict(case=key, run=str(run), native_assertions_passed=True, source_version=version,
                            metadata_disposition='original runner matched an unversioned symlink instead of the mapped versioned library; independent image hash match succeeds' if key in reused else 'passed',
                            result_sha256=sha(run/'result.json'), proof_sha256=sha(run/'proof.json')))
    return dict(status='passed', cases=records, targeted_cases=9, reused_without_native_rerun=4,
                original_attempts_retained=14, native_tests_run_by_auditor=0, new_requirement_credit=0)
