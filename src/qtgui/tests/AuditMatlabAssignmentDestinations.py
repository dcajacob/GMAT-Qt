"""Audit source-versioned MATLAB destination witnesses without admitting rows.

The selection file joins every catalog case in both GUI profiles. A case may
come from a failed batch; the original batch and all its failures stay intact.
"""
import argparse
import copy
import hashlib
import json
import math
import importlib
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import xml.etree.ElementTree as ET

STAGE = 'build/nasa-coverage/coverage95-20261104-matlab-assignment-destinations'
PRIOR = 'build/nasa-coverage/coverage95-20261104-function-scope-clause-join'
PREFIX = 'QtGui.NasaMatlabAssignmentDestination'
INPUTS = [2.123456789012345, -3.234567890123456,
          8.345678901234567, 4.456789012345678]
MARKERS = ['main', 'first', 'second', 'nested']
DATES = [f'0{i} Jan 2000 12:00:00.000' for i in range(2, 6)]
PHASES = ['initial', 'reopened', 'repeat-no-build']


def check(ok, message):
    if not ok:
        raise ValueError(message)


def load(path):
    return json.loads(Path(path).read_text())


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def compare(actual, expected, bound=1e-12):
    if isinstance(expected, list):
        check(isinstance(actual, list) and len(actual) == len(expected),
              'Destination dimensions differ')
        for a, b in zip(actual, expected):
            compare(a, b, bound)
    elif isinstance(expected, str):
        check(actual == expected, 'Destination string differs')
    else:
        check(isinstance(actual, (int, float)) and math.isfinite(actual)
              and abs(actual - expected) <= bound, 'Destination value differs')


def matlab_result(spec, i):
    x = INPUTS[i]
    if spec['kind'] == 'string':
        return DATES[i]
    if spec['kind'] == 'parameter-vector':
        if spec['parameter'] == 'Quaternion':
            return [2*x/(1+x*x), 0, 0, (1-x*x)/(1+x*x)]
        return [x + j*.125 for j in range(1, spec['length'] + 1)]
    return 3*x + 1


def destination_result(spec, i):
    result = matlab_result(spec, i)
    if spec['kind'] == 'array-element':
        result = [(20 if i else 0) + 10*(j//3+1) + j % 3 + 1
                  for j in range(6)]
        result[1 if i else 5] = matlab_result(spec, i)
    elif spec.get('parameter') == 'Quaternion':
        largest = max(range(4), key=lambda j: abs(result[j]))
        if result[largest] < 0:
            result = [-v for v in result]
    return result


def validate_proof(proof, spec, directory=None):
    check(proof['case'] == spec['key'] and proof['new_requirement_credit'] == 0
          and proof['whole_row_qualified'] is False,
          'Wrong identity or automatic row credit')
    if spec.get('expected_rejection'):
        check(proof['checkpoint'] == 'matlab-cartesian-disallowed-output'
              and proof['outcome'] == 'rejection-checked'
              and proof['factory_settable'] is False
              and proof['missions'] == proof['matlab_calls'] == 0
              and 'PrivateCarrier.EarthMJ2000Eq.Cartesian' in proof['diagnostic']
              and 'disallowed CallMatlabFunction Output Parameter' in proof['diagnostic'],
              'Exact Cartesian rejection contract differs')
        return
    phases = proof['phases']
    check([p['phase'] for p in phases] == PHASES and proof['missions'] == 3,
          'Three native mission boundaries missing')
    negative = spec.get('expected_runtime_rejection', False)
    check(proof['matlab_calls'] == (3 if negative else 12)
          and proof['gmat_scope_calls'] == (0 if negative else 12),
          'Native function-call census differs')
    if not negative:
        check(proof['checkpoint'] == 'matlab-assignment-destination'
              and proof['outcome'] == 'passed'
              and proof['root_destination'] == spec['root_destination']
              and proof['private_destination'] == spec['private_destination']
              and proof['total_function_calls'] == 24
              and proof['file_decisions'] == [{'applied': False}, {'applied': True}],
              'Typed editor/Cancel/Apply/destination contract differs')
        if spec.get('entry') == 'Assignment':
            check(proof['assignment_entry'] is True
                  and proof['original_command'] == 'Unused = Input;',
                  'Actual Assignment entry missing')
    else:
        check(proof['checkpoint'] == 'matlab-destination-result-rejection'
              and proof['outcome'] == 'rejection-checked'
              and proof['variant'] == spec['variant'],
              'Runtime type/dimension rejection identity differs')
    for phase in phases:
        check(phase['prototypes_unchanged'] is True, 'Prototype invariant missing')
        if negative:
            compare(phase['destinations'], [11, 12, 13, 21, 22, 23]
                    if spec['kind'] == 'whole-array' else [0, 0, 0, 1], 0)
            check(phase['outcome'] == 'rejection-checked'
                  and phase['matlab_calls'] == 1 and phase['gmat_scope_calls'] == 0
                  and phase['return_descriptor'] == spec['return_descriptor'],
                  'Rejected output, descriptor or continuation differs')
        else:
            compare(phase['destinations'], [destination_result(spec, i) for i in range(4)])
            compare(phase['inputs'], MARKERS if spec['kind'] == 'string' else INPUTS, 0)
            check(phase['matlab_calls'] == 4 and phase['gmat_scope_calls'] == 4
                  and phase['exercise_calls'] == 3 and phase['unused'] == -901
                  and phase['continuation'] == 42, 'Caller/call/continuation invariant differs')
        runtime = phase['matlab_runtime_log'].strip().splitlines()
        check(len(runtime) == (1 if negative else 4), 'Actual MATLAB runtime census differs')
        for row in runtime:
            fields = row.split(',')
            check(len(fields) == 3 and int(fields[0]) > 0
                  and len(fields[1]) == 5 and fields[1][:4].isdigit()
                  and fields[1][-1] in 'ab' and fields[2] == '1',
                  'Actual MATLAB PID/release/license absent')
        if directory:
            name = phase['phase']
            check((directory/(name+'-matlab-runtime.csv')).read_text().strip()
                  == phase['matlab_runtime_log'].strip(), 'Raw MATLAB identity differs')
            if negative:
                check((directory/(name+'-matlab-returns.csv')).read_text().strip()
                      == spec['return_descriptor'], 'Actual rejected Engine type/shape differs')
                diagnostic = (directory/(name+'-messages.txt')).read_text()
                check('CallMatlabFunction cannot assign result to' in diagnostic
                      and 'mismatch' in diagnostic, 'Native rejection diagnostic absent')
            else:
                check(load(directory/(name+'-observed.json')) == phase['destinations'],
                      'Raw native result differs from proof')
                check((directory/(name+'-report.csv')).read_text().strip()
                      == phase['raw_report'].strip(), 'Raw report differs')
                calls = (directory/(name+'-matlab-calls.csv')).read_text().strip().splitlines()
                check(len(calls) == 4, 'Raw MATLAB call census differs')
                for i, row in enumerate(calls):
                    fields = row.split(',')
                    if spec['kind'] == 'string':
                        check(fields == [MARKERS[i], DATES[i]], 'Raw MATLAB string result differs')
                    else:
                        compare(float(fields[0]), INPUTS[i], 1e-15)
                        wanted = matlab_result(spec, i)
                        compare([float(v) for v in fields[1:]],
                                wanted if isinstance(wanted, list) else [wanted])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--selections', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--manifest', type=Path)
    args = parser.parse_args()
    repo = args.repo.resolve()
    stage = repo/STAGE
    selection = load(args.selections)
    if args.manifest:
        manifest = load(args.manifest)
        check(sha(__file__) == manifest['auditor_sha256'], 'Frozen auditor identity differs')
        for path, digest in manifest['regular_artifact_sha256'].items():
            check(not (repo/path).is_symlink() and sha(repo/path) == digest,
                  'Immutable artifact differs: '+path)
        for path, target in manifest['symbolic_links'].items():
            check((repo/path).is_symlink() and str((repo/path).readlink()) == target,
                  'Recorded temporal alias changed: '+path)
    baseline = repo/'build/nasa-coverage/coverage95-20261103-matlab-c-interface/coverage-snapshot-v1.json'
    check(sha(baseline) == '4d7da4f31ba802900eb7a9eec2fd085dff86cd1ba365b1c23ed5959962bc34e6',
          'Original authority ledger changed')
    contract = load(stage/'source-contract-review-v3.json')
    check(contract['writable_vector_types'] == ['Quaternion', 'ThrustAngle1', 'ThrustAngle2']
          and contract['cartesian_settable'] is False and contract['eligibility_exclusions'] == []
          and contract['new_requirement_credit'] == 0, 'Writable scope was excluded or narrowed')
    prior = repo/PRIOR
    check(sha(prior/'evidence-manifest-v1.json') ==
          'd5ef284ca8bc1ecf821cd8db3b17e8eb475255e5f1aa237a5572252761fa9ace',
          'Original complete scope-clause seal changed')
    with tempfile.TemporaryDirectory(prefix='gmat-matlab-destination-prior-') as folder:
        root = Path(folder)
        script, output = root/'audit.py', root/'audit.json'
        with tarfile.open(prior/'reviewed-sources-v1.tar.gz') as archive:
            script.write_bytes(archive.extractfile(
                'src/qtgui/tests/AuditFunctionScopeClauses.py').read())
        completed = subprocess.run([sys.executable, str(script), '--repo', str(repo),
                                   '--manifest', str(prior/'evidence-manifest-v1.json'),
                                   '--output', str(output)], cwd=root, capture_output=True, text=True)
        check(completed.returncode == 0, 'Frozen predecessor replay failed: '+completed.stderr[-1600:])
        check(output.read_bytes() == (prior/'evidence-audit-v1.json').read_bytes(),
              'Frozen complete predecessor/owned chain differs')
    remaining = load(stage/'remaining-requirement-audit-v1.json')
    original_remaining = load(prior/'remaining-requirement-audit-v1.json')
    check(len(remaining['rows']) == len(original_remaining['rows']) == 108
          and len(set(remaining['original_hold_keys'])) == 59,
          'Original exact rows or prerequisite identities lost')
    for key, value in original_remaining.items():
        if key != 'rows':
            check(remaining[key] == value, 'Original remaining scope changed: '+key)
    for before_row, after_row in zip(original_remaining['rows'], remaining['rows']):
        check(all(after_row[k] == v for k, v in before_row.items()),
              'Original row/fingerprint/gap/conjunction was changed')
        extra = after_row['matlab_assignment_destination_increment']
        check(extra['new_requirement_credit'] == 0 and extra['whole_row_qualified'] is False
              and extra['original_gap_preserved'] is True,
              'Original open row was implicitly admitted')
    prior_metadata = load(prior/'exact-scope-clause-join-v3.json')['rows']
    expected_metadata = [dict(factory_type=r['factory_type'], metadata=r['native_parameter_metadata'])
                         for r in prior_metadata if 'native_parameter_metadata' in r]
    check(contract['native_parameter_metadata'] == expected_metadata,
          'A Parameter type/return category/writability obligation was omitted')
    catalogs = {}
    archives = {}
    for version in selection['source_versions']:
        identities = load(stage/f'executed-source-sha256-v{version}.json')
        with tarfile.open(stage/f'executed-sources-v{version}.tar.gz') as archive:
            check(set(archive.getnames()) == set(identities), 'Frozen source universe differs')
            for name, digest in identities.items():
                check(hashlib.sha256(archive.extractfile(name).read()).hexdigest() == digest,
                      'Frozen source bytes differ: '+name)
            catalogs[version] = json.loads(archive.extractfile(
                'src/qtgui/tests/NasaMatlabAssignmentDestinationCases.json').read())
        archives[version] = identities
    increment = load(stage/'core-increment-v10.json')
    check(increment['pending_paths'] == 280 and len(increment['unchanged_paths']) == 276
          and increment['header_layout_changes'] == increment['api_signature_changes']
          == increment['eligibility_exclusions'] == [], 'Original core inventory/API scope differs')
    changed = {r['path']: r for r in increment['increment_paths']}
    check(set(changed) == {
        'plugins/MatlabInterfacePlugin/src/base/command/CallMatlabFunction.cpp',
        'plugins/MatlabInterfacePlugin/src/base/interface/MatlabInterface.cpp',
        'src/base/interpreter/Interpreter.cpp',
        'src/base/command/GmatCommand.cpp'},
        'Production increment is not the exact four owning sources')
    before = load(stage/'core-before-v4.json')['sha256']
    current = archives[selection['catalog_version']]
    for name, row in changed.items():
        check(before[name] == row['preimage_sha256'] == sha(repo/row['preimage'])
              and current[name] == row['postimage_sha256'] == sha(repo/row['postimage']),
              'Exact core pre/postimage differs')
    for name, digest in increment['unchanged_paths'].items():
        check(before[name] == current[name] == digest, 'Unrelated original core path changed')
    old = load(prior/'reviewed-source-sha256-v1.json')
    differences = {n for n, h in old.items()
                   if n.startswith(('src/base/', 'src/gmatutil/', 'plugins/'))
                   and current.get(n) != h}
    check(differences == set(changed), 'Additional owning source change was not retained')
    original_io = 'src/qtgui/tests/NasaMatlabFunctionIoTests.cpp'
    check(all(m[original_io] == old[original_io] for m in archives.values()),
          'An original MATLAB I/O assertion source changed')
    specs = {c['key']: c for c in catalogs[selection['catalog_version']]['cases']}
    expected = {'RealParameter', 'StringParameter', 'ArrayElement', 'CartesianParameter',
                'Quaternion', 'AssignmentEntryRealParameter',
                *[f'ThrustAngle{a}N{n}' for a in [1, 2] for n in [1, 3, 4]],
                *['Quaternion'+s for s in ['Short', 'Matrix', 'String', 'Complex', 'Sparse', 'ND']],
                'ArrayWholeSmall', 'ArrayWholeLarge'}
    check(set(specs) == expected and set(selection['cases']) == {'headless', 'visible'},
          'Missing original destination/type/dimension/profile obligation')
    runs = {}
    proofs = []
    exposures = 0
    for mode, cases in selection['cases'].items():
        check(set(cases) == expected, 'A typed destination or profile was omitted')
        for case, record in cases.items():
            name, version = record['run'], record['source_version']
            if name not in runs:
                receipt = load(stage/name/'results.json')
                maps = load(stage/(name+'-loaded-images.json'))
                build = load(stage/f'build-receipt-v{version}.json')
                check(receipt['mode'] == mode and receipt.get('finished_utc')
                      and receipt['end_reason'] == 'completed-selection'
                      and build['status'] == 'passed' and build['returncode'] == 0
                      and build['inputs_unchanged'] and maps['inputs_unchanged']
                      and maps['runtime_binary_copies'] == 0 and maps['processes']
                      and build['source_sha256'] == maps['source_sha256'] == archives[version]
                      and build['binary_sha256'] == maps['binary_sha256']
                      and maps['build_receipt_sha256'] == sha(stage/f'build-receipt-v{version}.json'),
                      'Nonterminal, unbuilt, unmapped or mutated native input')
                active = {r['name']: r for r in receipt['results'] if r['status'] != 'not-run'}
                check((maps['returncode'] == 0) == all(r['status'] == 'passed' for r in active.values()),
                      'Failed batch was reclassified')
                if mode == 'visible':
                    display = receipt['visible_display']
                    check(display['resizable'] and display['start_minimized']
                          and 'Iconic' in display['initial_host_window_state']
                          and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'],
                          'Native visible window policy missing')
                runs[name] = active
            native = runs[name][PREFIX+case]
            check(native['status'] == 'passed' and native['returncode'] == 0,
                  'Selected native case did not pass')
            directory = stage/name/(PREFIX+case)
            junit = ET.parse(directory/'ctest.xml').getroot()
            check(int(junit.get('failures', '0')) == 0
                  and not list(junit.iter('failure')) and not list(junit.iter('error'))
                  and any(t.get('name') == PREFIX+case for t in junit.iter('testcase')),
                  'Exact passing JUnit missing')
            if mode == 'visible':
                count = native['trace']['native_exposure_events']
                check(count > 0 and not native['trace']['screenshots'], 'Native visible exposure absent')
                exposures += count
            proof = load(directory/'proof.json')
            validate_proof(proof, specs[case], directory)
            proofs.append((proof, specs[case]))
    # Reuse the original assertion reader at its archived source. Supported
    # Variable/Array/String I/O witnesses are distinct from the unresolved
    # Object probe and are not relabeled by this destination increment.
    check(set(selection['regressions']) == {'headless', 'visible'},
          'Affected original I/O profile missing')
    io_stage = repo/'build/nasa-coverage/coverage95-20261103-matlab-function-io'
    io_seal = load(io_stage/'evidence-manifest-v1.json')
    qualification = load(io_stage/'qualification-v1.json')
    check(sha(io_stage/'qualification-v1.json') == io_seal['qualification_sha256'],
          'Retained original I/O qualification differs')
    for name, digest in io_seal['artifact_sha256'].items():
        check(not (repo/name).is_symlink() and sha(repo/name) == digest,
              'Retained immutable I/O witness differs: '+name)
    with tempfile.TemporaryDirectory(prefix='gmat-matlab-destination-io-reader-') as folder:
        root = Path(folder)
        package = root/'src/qtgui/tests'
        package.mkdir(parents=True)
        with tarfile.open(stage/f'executed-sources-v{selection["catalog_version"]}.tar.gz') as archive:
            for member in archive.getmembers():
                if member.name.startswith('src/qtgui/tests/') and member.name.endswith('.py'):
                    (package/Path(member.name).name).write_bytes(archive.extractfile(member).read())
        reader = package/'AuditMatlabRowAcceptance.py'
        check(sha(reader) == qualification['admission_source_sha256'][
            'src/qtgui/tests/AuditMatlabRowAcceptance.py'], 'Retained assertion reader differs')
        sys.path.insert(0, str(package))
        try:
            original = importlib.import_module('AuditMatlabRowAcceptance')
            for mode in ['headless', 'visible']:
                regressions = selection['regressions'][mode]
                check(set(regressions) == {'Scalar', 'Array', 'InOut'},
                      'Affected supported scalar/array/formal-alias regression omitted')
                for case, record in regressions.items():
                    name = record['run']
                    check(name in runs, 'Regression run lacks its verified source/build/maps')
                    native = runs[name]['QtGui.NasaMatlabFunctionIo'+case]
                    check(native['status'] == 'passed' and native['returncode'] == 0,
                          'Affected original I/O regression failed')
                    if mode == 'visible':
                        check(native['trace']['native_exposure_events'] > 0,
                              'Affected original I/O visible exposure absent')
                    directory = stage/name/('QtGui.NasaMatlabFunctionIo'+case)
                    original.check(load(directory/'proof.json'), case, directory)
                for case in ['Scalar', 'Array', 'String']:
                    record = qualification['cases'][mode][case]
                    directory = io_stage/record['run']/('QtGui.NasaMatlabFunctionIo'+case)
                    original.check(load(directory/'proof.json'), case, directory)
        finally:
            sys.path.pop(0)
    controls = 0
    for proof, spec in proofs:
        mutations = [lambda p: p.update(new_requirement_credit=1)]
        if not spec.get('expected_rejection'):
            mutations += [lambda p: p['phases'].pop(),
                          lambda p: p['phases'][0].update(prototypes_unchanged=False),
                          lambda p: p['phases'][0].update(matlab_runtime_log='12,2026b,0')]
        for mutate in mutations:
            broken = copy.deepcopy(proof)
            mutate(broken)
            rejected = False
            try:
                validate_proof(broken, spec)
            except (ValueError, KeyError, IndexError):
                rejected = True
            check(rejected, 'Damaged credit/phase/prototype/license model escaped')
            controls += 1
    with tempfile.TemporaryDirectory(prefix='gmat-matlab-destination-named-review-') as folder:
        output = Path(folder)/'named.json'
        completed = subprocess.run([sys.executable, str(stage/'audit-named-calls-v2.py'),
                                   '--repo', str(repo), '--selections', str(args.selections),
                                   '--output', str(output)], capture_output=True, text=True)
        check(completed.returncode == 0, 'Named/support audit failed: '+completed.stderr[-1800:])
        named = load(output)
        check(named['new_named_GUI_contexts'] == named['original_call_regression_contexts'] == 4
              and named['serializer_cases'] == 8 and named['exact_serializations'] == 16
              and named['new_requirement_credit'] == 0 and named['whole_row_qualified'] is False,
              'Named/helper scope was widened')
    result = dict(status='passed-source-versioned-typed-destination-audit;row-admission-separate',
                  selected_native_contexts=len(proofs), visible_native_exposures=exposures,
                  damaged_models_rejected=controls, source_versions=selection['source_versions'],
                  exact_core_increment_paths=4, unchanged_prior_core_paths=276,
                  pending_core_paths=280, frozen_full_predecessor_chain_replays=1,
                  affected_original_IO_contexts=6, retained_typed_IO_contexts=6,
                  named_function_and_support_audit=named,
                  new_requirement_credit=0, whole_row_qualified=False,
                  scientific_technical_current_build_NASA_qualified=False)
    check(not args.output.exists(), 'Never overwrite an evidence audit')
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
