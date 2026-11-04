"""Audit actual Assignment editor routes; exact-row admission remains separate."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import xml.etree.ElementTree as ET

STAGE = 'build/nasa-coverage/coverage95-20261104-matlab-assignment-operator'
PRIOR = 'build/nasa-coverage/coverage95-20261104-matlab-assignment-indices'
PREFIX = 'QtGui.NasaMatlabAssignmentDestination'
CASES = {'AssignmentEntryStringParameter', 'AssignmentEntryQuaternion',
         'AssignmentEntryNestedArrayIndices'}
REGRESSION = 'AssignmentEntryRealParameter'


def check(ok, message):
    if not ok:
        raise ValueError(message)


def load(path):
    return json.loads(Path(path).read_text())


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def validate(proof, spec, typed, directory=None):
    # The frozen typed reader validates the common backend/destination clauses.
    # Validate the actual editor-entry clause here,including the string's numeric
    # seed,without rewriting the native proof or treating the entry as optional.
    backend = dict(spec)
    backend.pop('entry')
    typed.validate_proof(proof, backend, directory)
    seed = 'Unused = '+spec.get('initial_assignment_expression', 'Input')+';'
    check(proof['assignment_entry'] is True and proof['original_command'] == seed
          and proof['file_decisions'] == [{'applied': False}, {'applied': True}],
          'Actual Assignment entry/seed/editor decisions missing')
    if directory:
        initial = (directory/'initial.script').read_text()
        applied = (directory/'applied.script').read_text()
        check("'MATLAB result' "+seed in initial
              and (directory/'cancelled.script').read_text() == initial
              and (directory/'final.script').read_text() == applied,
              'Original Assignment source,Cancel or roundtrip differs')
        pending = (directory/'apply-pending-command.txt').read_text()
        expression = spec['root_destination']+' = MatDestination('
        check(expression in pending and expression in applied
              and 'CallMatlabFunction' not in pending,
              'Actual single-equals typed Assignment source missing')
        if spec.get('nested_indices'):
            declarations = load(directory/'native-call-declarations.json')
            check(declarations == [{'inputs': ['Feed(Indices(1,1),Indices(1,2))'],
                                     'outputs': [spec['root_destination']]}],
                  'Actual lowered nested argument names differ')
            for phase in typed.PHASES:
                check(load(directory/(phase+'-runtime-indices.json')) == [2, 3],
                      'Actual updated Assignment indices differ')
                typed.compare(load(directory/(phase+'-feed.json')),
                              [61, 62, 63, 71, 72, typed.INPUTS[0]], 0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--selections', type=Path, required=True)
    parser.add_argument('--manifest', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    repo = args.repo.resolve()
    stage, prior = repo/STAGE, repo/PRIOR
    if args.manifest:
        seal = load(args.manifest)
        check(sha(__file__) == seal['auditor_sha256'], 'Frozen Assignment reader differs')
        for name, digest in seal['regular_artifact_sha256'].items():
            check(not (repo/name).is_symlink() and sha(repo/name) == digest,
                  'Immutable Assignment artifact differs: '+name)
        for name, target in seal['symbolic_links'].items():
            check((repo/name).is_symlink() and str((repo/name).readlink()) == target,
                  'Recorded temporal alias differs: '+name)
    check(sha(prior/'evidence-manifest-v1.json') ==
          'b94ddb6cf48b8c3ddc8cc1676601e7b8204623ac25f0ee77a3e7feda6c8b1719',
          'Sealed runtime-index predecessor differs')
    archives, catalogs = {}, {}
    for version in [1, 2]:
        identities = load(stage/f'executed-source-sha256-v{version}.json')
        with tarfile.open(stage/f'executed-sources-v{version}.tar.gz') as archive:
            check(set(archive.getnames()) == set(identities), 'Source capsule universe differs')
            for name, digest in identities.items():
                check(hashlib.sha256(archive.extractfile(name).read()).hexdigest() == digest,
                      'Frozen source bytes differ: '+name)
            catalogs[version] = json.loads(archive.extractfile(
                'src/qtgui/tests/NasaMatlabAssignmentDestinationCases.json').read())
        archives[version] = identities
    selection = load(args.selections)
    with tempfile.TemporaryDirectory(prefix='gmat-assignment-operator-readers-') as folder:
        root = Path(folder)
        with tarfile.open(prior/'reviewed-sources-v1.tar.gz') as archive:
            for name in ['AuditMatlabAssignmentIndices', 'AuditMatlabAssignmentDestinations']:
                (root/(name+'.py')).write_bytes(archive.extractfile('src/qtgui/tests/'+name+'.py').read())
            old_catalog = json.loads(archive.extractfile(
                'src/qtgui/tests/NasaMatlabAssignmentDestinationCases.json').read())
            old_driver = archive.extractfile(
                'src/qtgui/tests/NasaMatlabAssignmentDestinationTests.cpp').read().decode()
        command = [sys.executable, str(root/'AuditMatlabAssignmentIndices.py'), '--repo', str(repo),
                   '--manifest', str(prior/'evidence-manifest-v1.json'), '--selections',
                   str(prior/'selections-v1.json'), '--output', str(root/'prior.json')]
        completed = subprocess.run(command, capture_output=True, text=True)
        check(completed.returncode == 0, 'Frozen complete index/predecessor chain failed: '+completed.stderr[-1800:])
        check((root/'prior.json').read_bytes() == (prior/'evidence-audit-v1.json').read_bytes(),
              'Complete frozen predecessor output differs')
        indices = module(root/'AuditMatlabAssignmentIndices.py', 'frozen_indices')
        typed = module(root/'AuditMatlabAssignmentDestinations.py', 'frozen_typed')
        specs = {v['key']: v for v in catalogs[2]['cases']}
        check(set(specs) == {v['key'] for v in old_catalog['cases']} | CASES
              and all(specs[v['key']] == v for v in old_catalog['cases']),
              'Original catalog case,oracle or bound changed')
        with tarfile.open(stage/'executed-sources-v2.tar.gz') as archive:
            driver = archive.extractfile('src/qtgui/tests/NasaMatlabAssignmentDestinationTests.cpp').read().decode()
        before, after = indices.assertions(old_driver), indices.assertions(driver)
        check(sum(before.values()) == 76 and sum(after.values()) == 77 and not before-after,
              'Original assertion expression removed/changed')
        runs, proofs, exposures = {}, [], 0
        for mode in ['headless', 'visible']:
            check(set(selection['cases'][mode]) == CASES
                  and set(selection['regressions'][mode]) == {REGRESSION},
                  'Actual Assignment type/profile or affected regression missing')
            for key, record in {**selection['cases'][mode], **selection['regressions'][mode]}.items():
                run, version = record['run'], record['source_version']
                if run not in runs:
                    receipt = load(stage/run/'results.json')
                    maps = load(stage/(run+'-loaded-images.json'))
                    build = load(stage/f'build-receipt-v{version}.json')
                    check(receipt['mode'] == mode and receipt.get('finished_utc')
                          and receipt['end_reason'] == 'completed-selection'
                          and maps['inputs_unchanged'] and maps['runtime_binary_copies'] == 0
                          and maps['processes'] and build['status'] == 'passed'
                          and build['returncode'] == 0 and build['inputs_unchanged']
                          and maps['source_sha256'] == build['source_sha256'] == archives[version]
                          and maps['binary_sha256'] == build['binary_sha256']
                          and maps['build_receipt_sha256'] == sha(stage/f'build-receipt-v{version}.json'),
                          'Nonterminal/unbuilt/mutated/unmapped Assignment run')
                    active = {r['name']: r for r in receipt['results'] if r['status'] != 'not-run'}
                    check((maps['returncode'] == 0) == all(r['status'] == 'passed' for r in active.values()),
                          'Failed Assignment batch was relabeled')
                    if mode == 'visible':
                        display = receipt['visible_display']
                        check(display['resizable'] and display['start_minimized']
                              and 'Iconic' in display['initial_host_window_state']
                              and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'],
                              'Visible minimized/resizable policy missing')
                    runs[run] = active
                result = runs[run][PREFIX+key]
                check(result['status'] == 'passed' and result['returncode'] == 0,
                      'Actual Assignment context did not pass')
                directory = stage/run/(PREFIX+key)
                junit = ET.parse(directory/'ctest.xml').getroot()
                check(not list(junit.iter('failure')) and not list(junit.iter('error'))
                      and any(t.get('name') == PREFIX+key for t in junit.iter('testcase')),
                      'Matching native JUnit missing')
                if mode == 'visible':
                    count = result['trace']['native_exposure_events']
                    check(count > 0 and not result['trace']['screenshots'], 'Visible native exposure missing')
                    exposures += count
                proof = load(directory/'proof.json')
                validate(proof, specs[key], typed, directory)
                proofs.append((proof, specs[key]))
        controls = 0
        for proof, spec in proofs:
            for mutate in [lambda p: p.update(assignment_entry=False),
                           lambda p: p.update(original_command='[Unused] = MatDestination(Input);'),
                           lambda p: p['phases'].pop(),
                           lambda p: p['phases'][0].update(destinations=[0]),
                           lambda p: p.update(new_requirement_credit=1)]:
                damaged = copy.deepcopy(proof)
                mutate(damaged)
                rejected = False
                try:
                    validate(damaged, spec, typed)
                except (ValueError, KeyError, IndexError):
                    rejected = True
                check(rejected, 'Damaged editor-entry/backend model escaped')
                controls += 1
    increment = load(stage/'core-increment-v2.json')
    before = load(stage/'core-before-v1.json')['sha256']
    check(increment['pending_paths'] == 280 and len(increment['unchanged_paths']) == 279
          and [r['path'] for r in increment['increment_paths']] == ['src/base/interpreter/Interpreter.cpp']
          and increment['header_layout_changes'] == increment['api_signature_changes'] == [],
          'Assignment core scope/API inventory differs')
    for row in increment['increment_paths']:
        check(before[row['path']] == row['preimage_sha256'] == sha(repo/row['preimage'])
              and archives[2][row['path']] == row['postimage_sha256'] == sha(repo/row['postimage']),
              'Exact Assignment production pre/postimage differs')
    for name, digest in increment['unchanged_paths'].items():
        check(before[name] == archives[2][name] == digest, 'Unrelated core increment')
    prior_sources = load(prior/'reviewed-source-sha256-v1.json')
    check({n for n, h in prior_sources.items()
           if n.startswith(('src/base/', 'src/gmatutil/', 'plugins/'))
           and archives[2].get(n) != h} == {'src/base/interpreter/Interpreter.cpp'},
          'Unretained additional owning-source change')
    first = load(stage/'assignment-entry-headless-v1/results.json')
    check([r['name'] for r in first['results'] if r['status'] == 'failed'] ==
          [PREFIX+'AssignmentEntryStringParameter', PREFIX+'AssignmentEntryNestedArrayIndices'],
          'Original fixture/product failures were removed or relabeled')
    scope = load(stage/'row-scope-review-v1.json')
    contract = load(repo/'build/nasa-coverage/coverage95-20261104-matlab-assignment-destinations/source-contract-review-v3.json')
    check(scope['original_context'] == contract['original_context']
          and scope['native_parameter_metadata'] == contract['native_parameter_metadata']
          and [r['count'] for r in scope['settable_parameter_partitions']] == [224, 5, 3]
          and scope['eligibility_exclusions'] == [], 'Original scope/type metadata narrowed')
    old_remaining = load(prior/'remaining-requirement-audit-v1.json')
    remaining = load(stage/'remaining-requirement-audit-v1.json')
    check(len(remaining['rows']) == len(old_remaining['rows']) == 108
          and len(set(remaining['original_hold_keys'])) == 59, 'Exact row/hold identity lost')
    for key, value in old_remaining.items():
        if key != 'rows':
            check(remaining[key] == value, 'Prior coverage/hold field changed: '+key)
    for old, new in zip(old_remaining['rows'], remaining['rows']):
        check(all(new[k] == v for k, v in old.items()), 'Original row/fingerprint/gap changed')
        check(new['matlab_assignment_operator_increment']['new_requirement_credit'] == 0
              and new['matlab_assignment_operator_increment']['whole_row_qualified'] is False,
              'Original row implicitly admitted')
    terminal = load(stage/'terminal-process-observations-v1.json')
    check(terminal['all_terminal'] and terminal['forced_interruptions'] == 0,
          'Recorded native termination unproven')
    result = dict(status='passed-source-versioned-actual-Assignment-editor-component-audit',
                  new_native_contexts=6, original_regression_contexts=2,
                  visible_native_exposures=exposures, damaged_models_rejected=controls,
                  original_cases_preserved=33, original_assertions_preserved=76,
                  added_assertions=1, exact_core_increment_paths=1, unchanged_core_paths=279,
                  retained_native_failures=2,
                  pending_core_paths=280, frozen_full_predecessor_chain_replays=1,
                  new_requirement_credit=0, whole_row_qualified=False,
                  scientific_technical_current_build_NASA_qualified=False)
    check(not args.output.exists(), 'Never overwrite an evidence audit')
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
