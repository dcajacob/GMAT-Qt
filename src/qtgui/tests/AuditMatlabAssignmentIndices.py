"""Replay exact MATLAB index witnesses; retain prior rows, holds and failures."""
import argparse
import collections
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import tarfile
import tempfile
import xml.etree.ElementTree as ET

STAGE = 'build/nasa-coverage/coverage95-20261104-matlab-assignment-indices'
PRIOR = 'build/nasa-coverage/coverage95-20261104-matlab-assignment-destinations'
PREFIX = 'QtGui.NasaMatlabAssignmentDestination'
CASES = {'DynamicArrayOutput', 'DynamicArrayInput', 'DynamicArrayInOut',
         'DynamicArrayAlias', 'NestedArrayIndices', 'ParameterArrayIndices',
         'LiteralArrayInput', 'OutputIndexZero', 'OutputIndexFraction',
         'OutputIndexBounds', 'InputIndexBounds', 'InputIndexZero', 'InputIndexFraction'}
PHASES = ['initial', 'reopened', 'repeat-no-build']
CORE = {'plugins/MatlabInterfacePlugin/src/base/command/CallMatlabFunction.cpp',
        'src/base/interpreter/Interpreter.cpp'}


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


def assertions(text):
    result = []
    for match in re.finditer(r'\brequire\(', text):
        index, depth, quote = match.end(), 1, None
        while index < len(text) and depth:
            ch = text[index]
            if quote:
                if ch == '\\':
                    index += 2
                    continue
                if ch == quote:
                    quote = None
            elif ch in '\"\'':
                quote = ch
            elif ch == '(':
                depth += 1
            elif ch == ')':
                depth -= 1
            index += 1
        result.append(text[match.start():index])
    return collections.Counter(result)


def validate(proof, spec, original, directory=None):
    negative = spec.get('expected_index_rejection', False)
    check(proof['case'] == spec['key'] and proof['new_requirement_credit'] == 0
          and proof['whole_row_qualified'] is False, 'Index identity or implicit credit differs')
    check([p['phase'] for p in proof['phases']] == PHASES and proof['missions'] == 3,
          'Missing initial/reopen/repeat index mission')
    if negative:
        check(proof['checkpoint'] == 'matlab-array-index-rejection'
              and proof['outcome'] == 'rejection-checked'
              and proof['dynamic_input'] == spec.get('dynamic_input', False)
              and proof['matlab_calls'] == proof['gmat_scope_calls'] == 0,
              'Invalid-index invocation or rejection differs')
        indices = [spec.get('root_runtime_row', 2), spec.get('root_runtime_column', 3)]
        for phase in proof['phases']:
            check(phase['indices'] == indices and phase['destinations'] == [11, 12, 13, 21, 22, 23]
                  and phase['outcome'] == 'rejection-checked'
                  and phase['matlab_calls'] == phase['gmat_scope_calls'] == 0
                  and phase['prototypes_unchanged'] is True,
                  'Index update, destination or pre-invocation guard differs')
            if directory:
                name = phase['phase']
                text = (directory/(name+'-messages.txt')).read_text()
                check('MATLAB array element index' in text
                      and 'integer within the configured array dimensions' in text,
                      'Actual index rejection diagnostic missing')
                for suffix in ['calls', 'runtime', 'returns']:
                    check((directory/(name+'-matlab-'+suffix+'.csv')).read_bytes() == b'',
                          'Rejected index invoked the MATLAB function body')
    else:
        original.validate_proof(proof, spec, directory)
        for flag in ['dynamic_indices', 'dynamic_input', 'element_alias', 'nested_indices',
                     'parameter_indices', 'literal_index_input']:
            # Source2 predates the four optional source3 variant fields. Its
            # exact frozen catalog contains none of those true variants.
            check(proof.get(flag, False) == spec.get(flag, False),
                  'Index semantic variant differs: '+flag)
        if directory:
            before = load(directory/'prototypes-before.json')
            check('Row = 1;' in before['Row'] and 'Col = 1;' in before['Col'],
                  'Configured/runtime index distinction missing')
            for name in PHASES:
                check(load(directory/(name+'-runtime-indices.json')) == [2, 3],
                      'Actual updated caller indices differ')
                check(load(directory/(name+'-prototypes.json')) == before
                      and load(directory/(name+'-observed-prototypes.json')) == before,
                      'Configured prototype bytes changed')
                check(all(load(directory/(name+'-invariants.json')).values()),
                      'Source/history/file/lock invariant differs')
                if spec.get('dynamic_input') and not spec.get('element_alias'):
                    original.compare(load(directory/(name+'-feed.json')),
                                     [61, 62, 63, 71, 72, original.INPUTS[0]], 0)
    if directory:
        initial = (directory/'initial.script').read_text()
        applied = (directory/'applied.script').read_text()
        check((directory/'cancelled.script').read_text() == initial
              and (directory/'final.script').read_text() == applied,
              'Index editor Cancel/Apply/roundtrip bytes differ')
        check('Row = 1;\nCol = 1;' in initial
              and 'Row = 2;\nCol = 3;' in (directory/'Exercise.gmf').read_text(),
              'Distinct main/private configured indices missing')
        check(spec['root_destination']+' = MatDestination' in applied
              or '['+spec['root_destination']+'] = MatDestination' in applied,
              'Authored index output expression missing')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--manifest', type=Path)
    parser.add_argument('--selections', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    repo = args.repo.resolve()
    stage, prior = repo/STAGE, repo/PRIOR
    if args.manifest:
        seal = load(args.manifest)
        check(sha(__file__) == seal['auditor_sha256'], 'Frozen index reader differs')
        for name, digest in seal['regular_artifact_sha256'].items():
            check(not (repo/name).is_symlink() and sha(repo/name) == digest,
                  'Immutable index artifact differs: '+name)
        for name, target in seal['symbolic_links'].items():
            check((repo/name).is_symlink() and str((repo/name).readlink()) == target,
                  'Temporal runtime alias changed: '+name)
    check(sha(prior/'evidence-manifest-v1.json') ==
          '7037aac196f9828b707f35289f3a598ad2f0b588094c8596b34732ab44304d9a',
          'Frozen destination predecessor seal differs')
    selection = load(args.selections)
    archives, catalogs = {}, {}
    for version in range(1, 6):
        identities = load(stage/f'executed-source-sha256-v{version}.json')
        with tarfile.open(stage/f'executed-sources-v{version}.tar.gz') as archive:
            check(set(archive.getnames()) == set(identities), 'Source capsule universe differs')
            for name, digest in identities.items():
                check(hashlib.sha256(archive.extractfile(name).read()).hexdigest() == digest,
                      'Frozen source bytes differ: '+name)
            catalogs[version] = json.loads(archive.extractfile(
                'src/qtgui/tests/NasaMatlabAssignmentDestinationCases.json').read())
        archives[version] = identities
    with tempfile.TemporaryDirectory(prefix='gmat-index-readers-') as folder:
        root = Path(folder)
        with tarfile.open(prior/'reviewed-sources-v1.tar.gz') as archive:
            reader = root/'destination.py'
            reader.write_bytes(archive.extractfile(
                'src/qtgui/tests/AuditMatlabAssignmentDestinations.py').read())
        completed = subprocess.run([sys.executable, str(reader), '--repo', str(repo),
            '--manifest', str(prior/'evidence-manifest-v1.json'),
            '--selections', str(prior/'selections-v1.json'), '--output', str(root/'prior.json')],
            capture_output=True, text=True)
        check(completed.returncode == 0, 'Frozen complete predecessor failed: '+completed.stderr[-1800:])
        check((root/'prior.json').read_bytes() == (prior/'evidence-audit-v1.json').read_bytes(),
              'Frozen destination/predecessor/owned chain replay differs')
        original = module(reader, 'frozen_destination')
        package = root/'src/qtgui/tests'
        package.mkdir(parents=True)
        with tarfile.open(stage/'executed-sources-v5.tar.gz') as archive:
            for member in archive.getmembers():
                if member.name.startswith('src/qtgui/tests/') and member.name.endswith('.py'):
                    (package/Path(member.name).name).write_bytes(archive.extractfile(member).read())
        sys.path.insert(0, str(package))
        try:
            io = __import__('AuditMatlabRowAcceptance')
            specs = {s['key']: s for s in catalogs[5]['cases']}
            with tarfile.open(prior/'executed-sources-v10.tar.gz') as archive:
                old_specs = json.loads(archive.extractfile(
                    'src/qtgui/tests/NasaMatlabAssignmentDestinationCases.json').read())['cases']
                old_driver = archive.extractfile(
                    'src/qtgui/tests/NasaMatlabAssignmentDestinationTests.cpp').read().decode()
            check(set(specs) == {s['key'] for s in old_specs} | CASES,
                  'Original or new catalog obligation omitted')
            check(all(specs[s['key']] == s for s in old_specs), 'Original typed catalog/bounds changed')
            with tarfile.open(stage/'executed-sources-v5.tar.gz') as archive:
                new_driver = archive.extractfile(
                    'src/qtgui/tests/NasaMatlabAssignmentDestinationTests.cpp').read().decode()
            original_assertions, current_assertions = assertions(old_driver), assertions(new_driver)
            check(sum(original_assertions.values()) == 63
                  and not original_assertions-current_assertions,
                  'An original complete assertion expression was removed or weakened')
            for version in archives:
                check(archives[version]['src/qtgui/tests/NasaMatlabFunctionIoTests.cpp'] ==
                      archives[1]['src/qtgui/tests/NasaMatlabFunctionIoTests.cpp'],
                      'Original I/O assertion source changed')
            runs, proofs, exposures = {}, [], 0

            def native(record, name, mode):
                nonlocal exposures
                run, version = record['run'], record['source_version']
                if run not in runs:
                    receipt = load(stage/run/'results.json')
                    maps = load(stage/(run+'-loaded-images.json'))
                    build = load(stage/f'build-receipt-v{version}.json')
                    check(receipt['mode'] == mode and receipt.get('finished_utc')
                          and receipt['end_reason'] == 'completed-selection'
                          and maps['inputs_unchanged'] and maps['runtime_binary_copies'] == 0
                          and build['status'] == 'passed' and build['inputs_unchanged']
                          and build['returncode'] == 0 and maps['processes']
                          and maps['source_sha256'] == build['source_sha256'] == archives[version]
                          and maps['binary_sha256'] == build['binary_sha256']
                          and maps['build_receipt_sha256'] == sha(stage/f'build-receipt-v{version}.json'),
                          'Unbuilt/nonterminal/mutated/unmapped index run')
                    active = {r['name']: r for r in receipt['results'] if r['status'] != 'not-run'}
                    check((maps['returncode'] == 0) == all(r['status'] == 'passed' for r in active.values()),
                          'Original failed batch was relabeled')
                    if mode == 'visible':
                        display = receipt['visible_display']
                        check(display['resizable'] and display['start_minimized']
                              and 'Iconic' in display['initial_host_window_state']
                              and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'],
                              'Visible minimized/resizable policy differs')
                    runs[run] = active
                result = runs[run][name]
                check(result['status'] == 'passed' and result['returncode'] == 0,
                      'Selected index/regression native case failed')
                directory = stage/run/name
                junit = ET.parse(directory/'ctest.xml').getroot()
                check(not list(junit.iter('failure')) and not list(junit.iter('error'))
                      and any(t.get('name') == name for t in junit.iter('testcase')),
                      'Matching passing native JUnit absent')
                if mode == 'visible':
                    count = result['trace']['native_exposure_events']
                    check(count > 0 and not result['trace']['screenshots'], 'Native visible exposure absent')
                    exposures += count
                return directory

            for mode in ['headless', 'visible']:
                check(set(selection['cases'][mode]) == CASES, 'New index case/profile missing')
                for key, record in selection['cases'][mode].items():
                    directory = native(record, PREFIX+key, mode)
                    proof = load(directory/'proof.json')
                    validate(proof, specs[key], original, directory)
                    if record['source_version'] >= 4:
                        declarations = load(directory/'native-call-declarations.json')
                        check(len(declarations) == 1 and declarations[0]['outputs'] ==
                              [specs[key]['root_destination']], 'Parsed output index expression differs')
                    proofs.append((proof, specs[key]))
                check(set(selection['regressions'][mode]) ==
                      {'ArrayElement', 'Array', 'NumericProperty', 'StringProperty'},
                      'Affected array/property regression omitted')
                for key, record in selection['regressions'][mode].items():
                    name = PREFIX+key if key == 'ArrayElement' else 'QtGui.NasaMatlabFunctionIo'+key
                    directory = native(record, name, mode)
                    if key == 'ArrayElement':
                        original.validate_proof(load(directory/'proof.json'), specs[key], directory)
                    else:
                        io.check(load(directory/'proof.json'), key, directory)
            controls = 0
            for proof, spec in proofs:
                mutations = [lambda p: p.update(new_requirement_credit=1),
                             lambda p: p['phases'].pop(),
                             lambda p: p['phases'][0].update(prototypes_unchanged=False),
                             lambda p: p['phases'][0].update(destinations=[0])]
                for mutate in mutations:
                    damaged = copy.deepcopy(proof)
                    mutate(damaged)
                    rejected = False
                    try:
                        validate(damaged, spec, original)
                    except (ValueError, KeyError, IndexError):
                        rejected = True
                    check(rejected, 'Damaged index model escaped')
                    controls += 1
            expected_failures = {
                'dynamic-indices-before-fix-headless-v1': ['DynamicArrayOutput', 'DynamicArrayInput', 'DynamicArrayInOut'],
                'index-edges-headless-v3': ['NestedArrayIndices', 'ParameterArrayIndices'],
                'index-nested-parameters-headless-v4': ['NestedArrayIndices']}
            for run, keys in expected_failures.items():
                receipt = load(stage/run/'results.json')
                maps = load(stage/(run+'-loaded-images.json'))
                failed = [r['name'] for r in receipt['results'] if r['status'] == 'failed']
                check(failed == [PREFIX+k for k in keys] and maps['returncode'] != 0
                      and maps['inputs_unchanged'] and maps['processes'],
                      'Original native failures were removed or relabeled')
        finally:
            sys.path.pop(0)
    increment = load(stage/'core-increment-v5.json')
    before = load(stage/'core-before-v1.json')['sha256']
    check(increment['pending_paths'] == 280 and len(increment['unchanged_paths']) == 278
          and {r['path'] for r in increment['increment_paths']} == CORE
          and increment['header_layout_changes'] == increment['api_signature_changes'] == [],
          'Index core scope/API inventory differs')
    for row in increment['increment_paths']:
        check(before[row['path']] == row['preimage_sha256'] == sha(repo/row['preimage'])
              and archives[5][row['path']] == row['postimage_sha256'] == sha(repo/row['postimage']),
              'Exact index production pre/postimage differs')
    for name, digest in increment['unchanged_paths'].items():
        check(before[name] == archives[5][name] == digest, 'Unrelated pending core path changed')
    prior_sources = load(prior/'reviewed-source-sha256-v1.json')
    check({n for n, h in prior_sources.items() if n.startswith(('src/base/', 'src/gmatutil/', 'plugins/'))
           and archives[5].get(n) != h} == CORE, 'Unretained additional production change')
    remaining = load(stage/'remaining-requirement-audit-v1.json')
    old_remaining = load(prior/'remaining-requirement-audit-v1.json')
    check(len(remaining['rows']) == len(old_remaining['rows']) == 108,
          'Original open exact row omitted')
    for key, value in old_remaining.items():
        if key != 'rows':
            check(remaining[key] == value, 'Original coverage/hold/gate field changed: '+key)
    for old, new in zip(old_remaining['rows'], remaining['rows']):
        check(all(new[k] == v for k, v in old.items()), 'Original row/fingerprint/gap changed')
        check(new['matlab_assignment_indices_increment']['new_requirement_credit'] == 0
              and new['matlab_assignment_indices_increment']['whole_row_qualified'] is False,
              'Original open row implicitly admitted')
    terminal = load(stage/'terminal-process-observations-v1.json')
    check(terminal['all_terminal'] and terminal['forced_interruptions'] == 0,
          'Native process termination unproven')
    result = dict(status='passed-source-versioned-MATLAB-array-index-component-audit',
                  selected_new_native_contexts=len(proofs), affected_regression_contexts=8,
                  visible_native_exposures=exposures, damaged_models_rejected=controls,
                  original_assertions_preserved=63, added_assertions=13, retained_native_failures=6,
                  source_versions=list(range(1, 6)), exact_core_increment_paths=2,
                  unchanged_pending_core_paths=278, pending_core_paths=280,
                  original_remaining_rows=108, original_hold_identities=59,
                  frozen_full_predecessor_chain_replays=1, new_requirement_credit=0,
                  whole_row_qualified=False, scientific_technical_current_build_NASA_qualified=False)
    check(not args.output.exists(), 'Never overwrite an evidence audit')
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
