"""Replay immutable extensions of the sealed prefix toward the agreed 18 rows."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import tarfile
import tempfile

import AuditNasaCoveragePrefix as legacy

KIND = 'sealed-agreed-requirement-extension-v1'
AGREED = ['All Others!E13', 'All Others!E156', 'All Others!E157',
          'All Others!E160', 'All Others!E196', 'FRC!E161', 'FRC!E55', 'FRC!E7',
          'FRR!E13', 'FRR!E374', 'FRR!E375', 'FRR!E406', 'FRR!E450',
          'FRR!E472', 'FRR!E477', 'FRR!E525', 'FRR!E526', 'FRR!E536']


def load(path):
    return json.loads(Path(path).read_text())


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise ValueError(message)


def qualify(repo, path, inventory, admitted):
    q = load(path)
    require(q['agreed_keys'] == AGREED, 'Agreed scope changed')
    with tempfile.TemporaryDirectory(prefix='gmat-frozen-agreed-reader-') as directory:
        with tarfile.open(q['reader_archive']) as bundle:
            require(set(bundle.getnames()) == set(q['reader_sha256']), 'Frozen readers changed')
            for name, digest in q['reader_sha256'].items():
                data = bundle.extractfile(name).read()
                require(hashlib.sha256(data).hexdigest() == digest, 'Frozen reader bytes changed')
                (Path(directory)/Path(name).name).write_bytes(data)
        module_path = Path(directory)/(q['reader_module']+'.py')
        require('src/qtgui/tests/'+module_path.name in q['reader_sha256'], 'Unbound qualifier')
        spec = importlib.util.spec_from_file_location('frozen_agreed_'+sha(module_path), module_path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module.qualify(Path(repo), Path(path), inventory, admitted)


def baseline(repo, path):
    prefix = load(path)
    if prefix['kind'] == legacy.KIND:
        return legacy.baseline(Path(repo), Path(path))
    require(prefix['kind'] == KIND and prefix['agreed_keys'] == AGREED, 'Different extension scope')
    for name, digest in prefix['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed sealed extension artifact: '+name)
    receipt = load(prefix['receipt'])
    manifest = load(prefix['manifest'])
    require(sha(prefix['manifest']) == receipt['manifest_sha256'], 'Extension seal chain differs')
    for name, digest in manifest['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed extension evidence: '+name)
    _, inventory, keys = baseline(repo, prefix['previous_prefix'])
    require(sha(prefix['previous_prefix']) == prefix['previous_prefix_sha256'], 'Previous prefix changed')
    ledger, audit = build(Path(repo), Path(prefix['previous_prefix']),
                          Path(prefix['qualification']), baseline_result=(None, inventory, keys))
    require(ledger == load(prefix['ledger']) and audit == load(prefix['admission_audit']),
            'Frozen evidence does not replay to canonical ledger')
    for row in ledger['additions']:
        keys['requirements'].add(row['key'])
    return prefix, inventory, keys


def build(repo, previous, qualification, baseline_result=None):
    _, inventory, keys = baseline_result or baseline(repo, previous)
    keys = {kind: set(values) for kind, values in keys.items()}
    q, incoming, audit = qualify(repo, qualification, inventory, keys['requirements'])
    known = {kind: {row['key']: row for row in inventory[kind]} for kind in keys}
    require(len(incoming) == len(set(incoming)) and
            not set(incoming) & keys['requirements'], 'Duplicate extension credit')
    require(set(incoming) <= set(known['requirements']), 'Unknown original row')
    keys['requirements'].update(incoming)
    coverage = {kind: dict(verified=len(keys[kind]), total=len(known[kind]),
                           percent=round(100*len(keys[kind])/len(known[kind]), 1))
                for kind in keys}
    require(coverage['requirements']['total'] == 1158 and
            coverage['historical_cases']['total'] == 2367, 'Original denominator changed')
    remaining = {kind: [row for row in inventory[kind] if row['key'] not in keys[kind]]
                 for kind in keys}
    done = [key for key in AGREED if key in keys['requirements']]
    ledger = dict(status='passed', scope='Source-versioned extension toward the agreed 18 complete rows',
                  baseline=str(previous), baseline_sha256=sha(previous),
                  authority_sha256=inventory['sources'], coverage=coverage,
                  agreed_keys=AGREED, agreed_completed=done,
                  agreed_remaining=[key for key in AGREED if key not in done],
                  goal_complete=len(done) == 18,
                  additions=[dict(key=key, kind='requirements', fingerprint=known['requirements'][key]['fingerprint'],
                                  source_text=known['requirements'][key]['text'],
                                  qualification=str(qualification), qualification_sha256=sha(qualification),
                                  status='verified-source-versioned') for key in incoming],
                  remaining_original_rows=remaining,
                  new_requirement_credit=len(incoming), native_tests_rerun_for_admission=0,
                  scientific_approval=False, technical_core_approval=False,
                  full_current_build_qualified=False, full_NASA_qualified=False)
    return ledger, audit


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--previous', type=Path)
    parser.add_argument('--qualification', type=Path)
    parser.add_argument('--replay-prefix', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--audit-output', type=Path)
    args = parser.parse_args()
    if args.replay_prefix:
        prefix, inventory, keys = baseline(args.repo, args.replay_prefix)
        result = dict(status='passed', verified=len(keys['requirements']), total=1158,
                      agreed_complete=sum(k in keys['requirements'] for k in AGREED),
                      agreed_total=18, native_tests_run=0)
    else:
        require(args.previous and args.qualification and args.audit_output, 'Missing extension input')
        result, audit = build(args.repo, args.previous, args.qualification)
        with args.audit_output.open('x') as file:
            json.dump(audit, file, indent=2); file.write('\n')
    with args.output.open('x') as file:
        json.dump(result, file, indent=2); file.write('\n')
    print(json.dumps({k: result[k] for k in result if k in
                      ['status', 'coverage', 'agreed_complete', 'agreed_completed', 'native_tests_run']}))
