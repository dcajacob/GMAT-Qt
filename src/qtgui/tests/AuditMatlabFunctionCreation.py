"""Audit real MATLAB resource creation evidence without automatic NASA credit.

The exact Resource Tree leaf and inherited parent are retained alongside the
original MATLAB bundle closure gate. Wider I/O, sharing/isolation and optimizer
conjunctions remain open. This reader never runs a GUI or modifies old evidence.
"""
import argparse
import copy
import difflib
import hashlib
import json
from pathlib import Path
import re
import tarfile
import xml.etree.ElementTree as ET

import NasaGuiCoverage as nasa
from BuildNasaCoverageExtension import require, validate_visible

TEST = 'QtGui.NasaMatlabFunctionCreation'
KEY = 'All Others!E152'
DRIVER = 'src/qtgui/tests/NasaMatlabFunctionCreationTests.cpp'
BODY = ('function [total, difference] = MatProbe(a, b)\n'
        'total = a + b;\ndifference = a - b;\nend\n')
METADATA = [dict(field=n, type=t, read_only=ro) for n, t, ro in [
    ('Covariance', 'Rmatrix', True), ('FunctionPath', 'Filename', False),
    ('FunctionName', 'String', True), ('Input', 'StringArray', True),
    ('Output', 'StringArray', True)]]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(path):
    return json.loads(Path(path).read_text())


def archive(path, expected):
    with tarfile.open(path) as bundle:
        members = bundle.getmembers()
        require(len(members) == len(expected) and all(m.isfile() for m in members)
                and {m.name for m in members} == set(expected), 'Source archive partition differs')
        result = {}
        for member in members:
            require(not Path(member.name).is_absolute() and '..' not in Path(member.name).parts,
                    'Unsafe source member')
            data = bundle.extractfile(member).read()
            require(hashlib.sha256(data).hexdigest() == expected[member.name], 'Frozen source differs')
            result[member.name] = data
        return result


def assertions(source):
    # Parenthesis balancing ignores C++ strings/comments. Preserve the actual
    # original expressions, not only an assertion count or message inventory.
    token = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[()]')
    result = []
    for match in re.finditer(r'\brequire\(', source):
        depth = 1
        for item in token.finditer(source, match.end()):
            if item[0] == '(':
                depth += 1
            elif item[0] == ')':
                depth -= 1
                if not depth:
                    result.append(source[match.start():item.end()])
                    break
        else:
            raise ValueError('Unbalanced native assertion')
    return result


def check_proof(proof, directory=None):
    require(proof['checkpoint'] == 'matlab-function-resource-creation'
            and proof['outcome'] == 'passed' and proof['original_key'] == KEY
            and proof['new_requirement_credit'] == 0, 'Wrong constituent proof/credit')
    require(proof['creation'] == [dict(accepted=False, creator=True, menu=True),
                                dict(accepted=True, creator=True, menu=True)],
            'Missing actual Add/creator Cancel/accept')
    require(proof['native_type'] == 'MatlabFunction' and proof['parameter_metadata'] == METADATA,
            'Native resource type/complete declared field census differs')
    path = proof['native_function_path']
    require(Path(path).name == 'MatProbe.m' and 'MATLAB functions Ω' in path
            and Path(path).is_absolute(), 'Configured Unicode MATLAB path differs')
    require(proof['function_source'] == BODY, 'Independent MATLAB function changed')
    phases = proof['phases']
    require([p['phase'] for p in phases] == ['initial', 'reopened'], 'Missing save/reopen phase')
    # Truth derives from the prescribed function and input literals, not GMAT
    # or MATLAB output. Exact integer arithmetic has no relaxed tolerance.
    a, b, keep = 7, -3, 41
    truth = [a, b, a + b, a - b, keep + 1]
    report = f'{a+b},{a-b},{keep}\n{keep+1}\n'
    for phase in phases:
        require(phase['native_values'] == truth and phase['report'] == report
                and phase['viewer_matches'] is True, 'Independent call/raw/viewer result differs')
        if directory:
            require((directory/(phase['phase']+'-report.csv')).read_text() == report,
                    'Raw report differs from proof')
    recovery = proof['error_recovery']
    require(recovery['intentional_error_rejected'] is True
            and 'Expected MATLAB acceptance failure' in recovery['intentional_error']
            and recovery['following_eval_succeeded'] is True
            and recovery['following_eval_error'] == ''
            and recovery['following_value'] == 314160
            and recovery['rows'] == recovery['columns'] == recovery['elements'] == 1
            and recovery['syntax_error_rejected'] is True
            and recovery['post_syntax_value'] == 314161
            and recovery['scratch_named_expression_value'] == 10,
            'Runtime/syntax error propagation or subsequent workspace evaluation differs')
    require(all(s in proof['limits'] for s in ['broader MATLAB I/O', 'sharing/isolation',
                                               'optimizer', 'reverse-interface']),
            'Wider integration limits omitted')
    if directory:
        require((directory/'MatProbe.m').read_text() == BODY
                and load(directory/'error-recovery.json') == recovery, 'Prescribed source/recovery bytes differ')
        seed = (directory/'seed.script').read_text()
        applied = (directory/'applied.script').read_text()
        caller = (directory/'caller.script').read_text()
        for statement in ['Input = 7;', 'Other = -3;', 'Total = 0;', 'Difference = 0;',
                          'Keep = 41;', "Values.Delimiter = ',';", 'Values.Precision = 25;']:
            require(statement in seed, 'Prescribed input/precision changed')
        mission = 'BeginMissionSequence;\nReport Values Keep;\n'
        require(applied.endswith(mission) and seed.endswith(mission)
                and 'Create MatlabFunction MatProbe;' in applied
                and "MatProbe.FunctionPath = '"+path+"';" in applied,
                'Actual Apply resource/path/following mission differs')
        require(caller == applied[:-len(mission)] + 'BeginMissionSequence;\n'
                '[Total, Difference] = MatProbe(Input, Other);\n'
                'Report Values Total Difference Keep;\nKeep = Keep + 1;\nReport Values Keep;\n',
                'Actual caller differs from prescribed native function route')
        startup = (directory/'startup.txt').read_text()
        require(re.search(r'^MATLAB_MODE\s*=\s*SHARED\s*$', startup, re.M)
                and re.search(r'^PLUGIN\s*=\s*/[^\n]*libMatlabInterface(?:\.so(?:\.R\d+[ab])?)?\s*$', startup, re.M),
                'Actual isolated startup/provider/mode differs')


def audit(repo, stage):
    inputs = load(stage/'audit-inputs-v1.json')
    for p, h in inputs['artifact_sha256'].items():
        require(Path(p).is_file() and not Path(p).is_symlink() and sha(p) == h,
                'Changed/nonregular artifact '+p)
    inventory = nasa.inventory(repo)
    require(inputs['authority_sha256'] == inventory['sources']
            and len(inventory['requirements']) == 1158
            and len(inventory['historical_cases']) == 2367, 'Original NASA authority changed')
    known = {r['key']: r for r in inventory['requirements']}
    closure = load(inputs['closure_review'])
    original = next(r['original_row'] for r in closure['reviewed_open_rows'] if r['key'] == KEY)
    require(original == inputs['retained_original_row'], 'Original recorded gate was rewritten')
    require(inputs['scope_rows'] == [dict(key=k, source_text=known[k]['text'],
            fingerprint=known[k]['fingerprint']) for k in
            [KEY, 'All Others!E120', 'All Others!E241', 'All Others!E242',
             'All Others!E243', 'All Others!E244']], 'Leaf/inherited/wider scope identity differs')
    coverage = load(inputs['canonical_ledger'])
    remaining = {r['key'] for r in coverage['remaining_original_rows']['requirements']}
    require(KEY in remaining and coverage['coverage']['requirements']['verified'] == 1019
            and coverage['coverage']['historical_cases']['verified'] == 1816,
            'Constituent evidence silently changed coverage')
    holds = load(inputs['hold_receipt'])['unchanged_prerequisite_holds']
    require(inputs['unchanged_original_hold_keys'] == holds and len(set(holds)) == 59
            and set(holds) <= remaining, 'Original59 holds not preserved open')
    frozen = archive(stage/'executed-sources-v6.tar.gz', load(stage/'executed-source-sha256-v6.json'))
    for name, binding in inputs['production_and_launcher_preimages'].items():
        with tarfile.open(binding['archive']) as bundle:
            preimage = bundle.extractfile(name).read()
        require(hashlib.sha256(preimage).hexdigest() == binding['sha256']
                and hashlib.sha256(frozen[name]).hexdigest() == binding['final_sha256'],
                'Exact production/launcher increment identity differs')
        patch = ''.join(difflib.unified_diff(preimage.decode().splitlines(True),
            frozen[name].decode().splitlines(True), fromfile='before/'+name, tofile='after/'+name))
        require(Path(binding['patch']).read_text() == patch, 'Exact increment patch differs')
    old = archive(stage/'executed-sources-v2.tar.gz', load(stage/'executed-source-sha256-v2.json'))
    original_assertions, final_assertions = (assertions(x[DRIVER].decode()) for x in [old, frozen])
    at = 0
    for item in final_assertions:
        if at < len(original_assertions) and item == original_assertions[at]:
            at += 1
    require(at == len(original_assertions), 'Original creation/call assertions removed or changed')
    cmake = frozen['src/qtgui/CMakeLists.txt'].decode()
    block = cmake[cmake.index('# Plugins are declared after src/qtgui'):]
    require('$<TARGET_OBJECTS:GmatQtAcceptanceSupport>' in block
            and 'ENVIRONMENT "GMAT_QT_CAPTURE_CHILD_OUTPUT=1"' in block
            and 'RESOURCE_LOCK gmat_qt_engine' in block, 'Observer/output custody/engine lock omitted')
    require('Matlab_ENG_LIBRARY' in frozen['plugins/MatlabInterfacePlugin/src/base/CMakeLists.txt'].decode(),
            'Actual Engine provider dependency missing')
    modes, image_sets, proof = {}, [], None
    for mode, name in [('headless', 'headless-v5'), ('visible', 'visible-v2')]:
        root = stage/name
        receipt = load(root/'results.json')
        mapped = load(stage/(name+'-loaded-images.json'))
        require(receipt['mode'] == mode and receipt.get('finished_utc')
                and receipt['end_reason'] == 'completed-selection'
                and mapped['returncode'] == 0 and mapped['inputs_unchanged'] is True
                and mapped['runtime_binary_copies'] == 0, 'Nonterminal/failed/mutated run')
        require(mapped['source_sha256'] == load(stage/'executed-source-sha256-v6.json')
                and all(mapped['source_sha256'].get(p) == h for p, h in receipt['source_hashes'].items()),
                'Actual native sources differ from frozen version')
        active = [r for r in receipt['results'] if r['status'] != 'not-run']
        require(len(active) == 1 and active[0]['name'] == TEST, 'Extra/missing executed native selection')
        row = active[0]
        require(row['status'] == 'passed' and row['returncode'] == 0, 'Native case failed')
        directory = Path(row['log']).parent
        cases = list(ET.parse(directory/'ctest.xml').getroot().iter('testcase'))
        require(len(cases) == 1 and cases[0].get('name') == TEST
                and not any(cases[0].find(k) is not None for k in ['failure', 'error', 'skipped']),
                'Missing exact passing JUnit')
        proof = load(directory/'proof.json')
        checks = re.findall(r'NASA_CHECK (\{[^\n]+\})', Path(row['log']).read_text())
        require(len(checks) == 1 and json.loads(checks[0]) == proof, 'Proof differs from native stdout')
        check_proof(proof, directory)
        require(len(mapped['processes']) == 1, 'Missing actual native process')
        process = mapped['processes'][0]
        images = mapped['loaded_project_images']
        require(process['executable'] in row['registered_command']
                and images[process['executable']] == mapped['executable_sha256']
                == receipt['binary_hashes'][process['executable']]
                and any(Path(p).name.startswith('libMatlabInterface.so') for p in images)
                and any(p.startswith('/usr/local/MATLAB/R2026b/') and Path(p).name == 'libeng.so' for p in images),
                'Actual executable/plugin/SDK Engine identity absent')
        union = {}
        for snapshot in process['snapshots']:
            require(hashlib.sha256(snapshot['maps'].encode()).hexdigest() == snapshot['maps_sha256'],
                    'Raw process maps changed')
            require(all(p in snapshot['maps'] for p in snapshot['project_images']), 'Image lacks raw mapping')
            union.update(snapshot['project_images'])
        require(union == images, 'Loaded image census differs from raw mapped snapshots')
        traces = list(directory.glob('trace-*.jsonl'))
        require(len(traces) == 1 and traces[0].stem == 'trace-'+str(process['pid']),
                'Observer trace/native process identity differs')
        trace = [json.loads(line) for line in traces[0].read_text().splitlines()]
        require(trace[0]['event'] == 'start' and trace[-1]['event'] == 'summary'
                and trace[-1]['screenshots'] == 0, 'Missing terminal screenshot-free observer trace')
        if mode == 'visible':
            evidence = {TEST: dict(log=row['log'], log_sha256=sha(row['log']),
                        junit=str(directory/'ctest.xml'), junit_sha256=sha(directory/'ctest.xml'))}
            validate_visible(receipt, evidence)
            exposures = [r for r in trace if r['event'] == 'native_exposure' and r.get('exposed')]
            require(len(exposures) == row['trace']['native_exposure_events'] > 0
                    and row['trace']['recorded_platforms'] == ['xcb'], 'Actual native exposures differ')
        require(row['trace']['instrumented_processes'] == 1 and not row['trace']['screenshots'],
                'Observer missing or screenshots retained')
        image_sets.append(images)
        modes[mode] = dict(status='passed', native_processes=1, loaded_images=len(images),
            native_exposures=row['trace']['native_exposure_events'], scalar_calls=2,
            report_rows=4, native_value_comparisons=10)
    require(image_sets[0] == image_sets[1], 'Headless/visible images differ')
    # Retain functional failures even where a later test/launcher correction
    # passes. A passed proof emitted before a timeout is not a passed run.
    failures = {}
    for name, expected in [('headless-v1', 'Ambiguous MATLAB report viewer'),
                           ('headless-v2', 'MATLAB error state contaminated'),
                           ('headless-v3', 'Timeout')]:
        row = next(r for r in load(stage/name/'results.json')['results'] if r['name'] == TEST)
        require(row['status'] == 'failed' and row['returncode'] != 0
                and expected in Path(row['log']).read_text(), 'Original failed run not preserved')
        failures[name] = dict(returncode=row['returncode'], reason=expected)
    stale = load(stage/'headless-v2'/TEST/'error-recovery.json')
    require(stale['following_value'] == 314160 and stale['following_eval_succeeded'] is False,
            'Actual stale error failure lost')
    prior_core, current_core = (load(inputs[k]) for k in ['prior_core_inventory', 'core_inventory'])
    old_core = {r['path']: r['working_sha256'] for r in prior_core['files']}
    new_core = {r['path']: r['working_sha256'] for r in current_core['files']}
    require(len(old_core) == 245 and len(new_core) == 247
            and set(new_core)-set(old_core) == {'plugins/MatlabInterfacePlugin/src/base/interface/MatlabInterface.cpp',
                                               'plugins/MatlabInterfacePlugin/src/base/CMakeLists.txt'}
            and all(new_core[p] == h for p, h in old_core.items()), 'Unexpected core-path increment')
    negatives = []
    for label, mutate in [
        ('missing-Cancel', lambda p: p['creation'].pop(0)),
        ('wrong-native-type', lambda p: p.update(native_type='GmatFunction')),
        ('wrong-writability', lambda p: p['parameter_metadata'][1].update(read_only=True)),
        ('missing-field', lambda p: p['parameter_metadata'].pop()),
        ('wrong-source', lambda p: p.update(function_source=BODY.replace('+', '-'))),
        ('wrong-path', lambda p: p.update(native_function_path='/tmp/MatProbe.m')),
        ('wrong-result', lambda p: p['phases'][0]['native_values'].__setitem__(2, 5)),
        ('wrong-raw-report', lambda p: p['phases'][0].update(report='5,10,41\n42\n')),
        ('missing-reopen', lambda p: p['phases'].pop()),
        ('wrong-viewer', lambda p: p['phases'][0].update(viewer_matches=False)),
        ('stale-error', lambda p: p['error_recovery'].update(following_eval_succeeded=False)),
        ('syntax-accepted', lambda p: p['error_recovery'].update(syntax_error_rejected=False)),
        ('namespace-clobber', lambda p: p['error_recovery'].update(scratch_named_expression_value=0)),
        ('unearned-credit', lambda p: p.update(new_requirement_credit=1))]:
        corrupt = copy.deepcopy(proof)
        mutate(corrupt)
        try:
            check_proof(corrupt)
        except (ValueError, AssertionError, KeyError):
            negatives.append(label)
        else:
            raise ValueError('Accepted corrupt proof '+label)
    return dict(status='passed', scope='source-versioned MATLAB creation/call/error-recovery constituents',
        qualified_keys=[], new_requirement_credit=0, original_key=KEY,
        retained_original_closure_gate=original['closure_gate'], whole_row_status='open',
        remaining_conjunctions=['Original bundle gate: general type/result/error I/O',
            'Independent process sharing and single-use isolation', 'GMAT optimizer integration',
            'Reverse interface and general workspace namespace hygiene remain unqualified'],
        original_assertions_preserved=len(original_assertions), final_assertions=len(final_assertions),
        modes=modes, preserved_failed_native_runs=failures, negative_controls=negatives,
        pending_core_paths=247, unchanged_prior_core_paths=245,
        unchanged_original_hold_keys=holds, canonical_ledger=inputs['canonical_ledger'],
        canonical_ledger_sha256=sha(inputs['canonical_ledger']), coverage=coverage['coverage'],
        requirements_still_needed=82, screenshots=0, runtime_binary_copies=0, kernel_copies=0,
        scientific_approval=False, technical_core_approval=False,
        full_current_build_qualified=False, full_NASA_qualified=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = audit(Path(__file__).resolve().parents[3], args.stage.resolve())
    require(not args.output.exists(), 'Audit output already exists')
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k: result[k] for k in ['status', 'qualified_keys', 'new_requirement_credit',
          'original_assertions_preserved', 'modes', 'negative_controls', 'requirements_still_needed']}))
