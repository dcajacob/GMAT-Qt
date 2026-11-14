"""Admit exact MATLAB resource/in-function-call rows; keep the bundle gate open."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import sys
import tarfile
import xml.etree.ElementTree as ET
import NasaGuiCoverage as nasa
import AuditMatlabFunctionCreation as creation
import AuditMatlabCleanupEvidence as cleanup
from BuildNasaCoverageExtension import load, require, sha, validate_visible

KIND = 'source-versioned-MATLAB-resource-creation-and-function-invocation'
KEYS = ['All Others!E152', 'FRR!E478']
PREFIX = 'QtGui.NasaMatlabFunctionIo'
POSITIVES = ['Scalar', 'ScalarGlobal', 'Array', 'String', 'QuotedString', 'NumericProperty',
             'StringProperty', 'NoInput', 'NoOutput', 'InOut', 'LiteralRecovery']
NEGATIVES = ['LiteralClose', 'UndeclaredOutput', 'WrongAliasType']
MODULES = ['AuditMatlabRowAcceptance', 'AuditMatlabFunctionCreation',
           'AuditMatlabCleanupEvidence', 'NasaGuiCoverage', 'BuildNasaCoverageExtension']


def expected(case):
    kind = 'Array' if case == 'Array' else 'String' if case in ['String', 'QuotedString', 'StringProperty'] else 'Scalar'
    values = [2.5, -3, 8, 4]
    if kind == 'Array':
        values = [[i+10*j for i in range(1, 7)] for j in range(4)]
        outputs = [[a[5]+1, a[1]-2, 2*a[3], a[0]+3, -a[4], a[2]+4] for a in values]
    elif kind == 'String':
        values = (["pilot''s Ω, log", "local''s α", "other''s β", "nested''s γ"] if case == 'QuotedString'
                  else ['alpha, beta', 'private first', 'private second', 'nested value'])
        outputs = [v+'|done' for v in values]
        if case == 'StringProperty': outputs[0] = '01 Jan 2000 12:00:00.000|done'
    else:
        outputs = [3*x+1 for x in values]
        if case == 'NumericProperty': outputs[0] = 3001
        if case == 'NoInput': outputs = [17]*4
        if case == 'NoOutput': outputs = [41]+[x+1 for x in values[1:]]
        if case == 'InOut': values[0], outputs[0] = outputs[0], values[0]
    def flat(v):
        if isinstance(v, list): return [s for x in v for s in flat(x)]
        return [v if isinstance(v, str) else format(v, '.17g')]
    return kind, values, outputs, ','.join(flat(values)+flat(outputs)+['41', '3'])+'\n42,3\n'


def check(proof, case, directory=None):
    require(proof['case'] == case and proof['new_requirement_credit'] == 0, 'Wrong native identity/automatic credit')
    if case in ['LiteralClose', 'LiteralRecovery']:
        cleanup.check_proof(proof, case, directory)
        if case == 'LiteralClose': return
    if case == 'UndeclaredOutput':
        require(proof['checkpoint'] == 'matlab-function-undeclared-output' and proof['outcome'] == 'rejection-checked'
                and proof['expected_rejection'] is True, 'Undeclared output guard missing')
        if directory: require('Missing output declaration of "out"' in (directory/'build-messages.txt').read_text(), 'Native missing-declaration diagnostic missing')
        return
    if case == 'WrongAliasType':
        require(proof['checkpoint'] == 'matlab-function-output-type-rejection' and proof['outcome'] == 'rejection-checked'
                and proof['actual_type'] == 'ChemicalTank' and proof['caller_mass'] == 12
                and proof['caller_output'] == 2.5 and proof['keep'] == 41, 'Actual unsupported type/caller guard missing')
        if directory:
            text = (directory/'rejected-messages.txt').read_text()
            require('unsupported MATLAB output type' in text and 'ChemicalTank' in text, 'Actual type diagnostic missing')
        return
    kind, values, outputs, report = expected(case)
    require(proof['checkpoint'] == 'matlab-function-io-'+case and proof['kind'] == kind
            and proof['outcome'] == 'passed' and [r['phase'] for r in proof['phases']]
            == ['initial', 'no-build-repeat', 'reopened'], 'Missing typed/context/repeat/reopen proof')
    for row in proof['phases']:
        require(row['inputs'] == values and row['outputs'] == outputs and row['report'] == report
                and row['function_calls'] == 3 and row['matlab_calls'] == 4 and row['viewer_matches'] is True,
                'Independent MATLAB typed/caller/output oracle differs')
        if directory:
            require(load(directory/(row['phase']+'-inputs.json')) == values
                    and load(directory/(row['phase']+'-outputs.json')) == outputs
                    and (directory/(row['phase']+'-report.csv')).read_text() == report, 'Raw data differs from proof')
            if case == 'NoOutput': require((directory/(row['phase']+'-no-output.txt')).read_text() == '2.5\n-3\n8\n4\n', 'Zero-output MATLAB calls/order not proved')
    if directory:
        gm = directory/('recovered-Exercise.gmf' if case == 'LiteralRecovery' else 'Exercise.gmf')
        text = gm.read_text()
        require('function [out] = Exercise(' in text and 'FunctionCalls = FunctionCalls + 1;' in text
                and ('MatIo();' if case == 'NoInput' else 'MatIo(input);') in text, 'Actual inner MATLAB call absent')
        nested = (directory/'NestedExercise.gmf').read_text()
        require('NestedExercise(' in nested and '[out] = Exercise(' in nested, 'Actual nested call absent')
        require((directory/'MatIo.m').read_text().startswith('function '), 'Real MATLAB source missing')


def qualify(repo, path, inventory):
    q = load(path)
    require(q['contract_kind'] == KIND and q['qualified_keys'] == KEYS, 'Wrong complete original MATLAB scopes')
    require(all(q.get(k) is False for k in ['scientific_approval', 'technical_core_approval',
            'full_current_build_qualified', 'full_NASA_qualified', 'MATLAB_bundle_qualified']), 'Broader qualification gate waived')
    for p, h in q['artifact_sha256'].items():
        require(Path(p).is_file() and not Path(p).is_symlink() and sha(p) == h, 'Changed/nonregular artifact '+p)
    with tarfile.open(q['admission_source_archive']) as bundle:
        require(set(bundle.getnames()) == set(q['admission_source_sha256']), 'Admission reader partition differs')
        for name, h in q['admission_source_sha256'].items():
            require(hashlib.sha256(bundle.extractfile(name).read()).hexdigest() == h, 'Frozen admission reader differs')
    for name in MODULES:
        module = sys.modules[__name__] if name == 'AuditMatlabRowAcceptance' else sys.modules[name]
        source = 'src/qtgui/tests/'+name+'.py'
        require(Path(module.__file__).resolve() == repo/source and sha(repo/source) == q['admission_source_sha256'][source],
                'Executing admission reader differs: '+name)
    require(q['authority_sha256'] == inventory['sources'], 'Original workbook/matrix authority changed')
    known = {r['key']: r for r in inventory['requirements']}
    row = lambda k: dict(key=k, declared_id=known[k]['declared_id'], fingerprint=known[k]['fingerprint'], source_text=known[k]['text'])
    require(q['original_requirements'] == [row(k) for k in KEYS], 'Exact original row/fingerprint changed')
    scope = load(q['scope_review'])
    require(scope['candidate_rows'] == q['original_requirements'] and scope['authority_sha256'] == inventory['sources']
            and scope['inherited_parents'] == [row(k) for k in ['All Others!E120', 'FRR!E472']]
            and scope['bundle_gate_original_row_count'] == 37 and scope['bundle_status'] == 'open;not-qualified',
            'Source inheritance or retained bundle gate changed')
    before = load(q['prior_coverage'])
    require(scope['canonical_before_sha256'] == sha(q['prior_coverage'])
            and before['coverage']['requirements']['verified'] == 1019
            and before['coverage']['historical_cases']['verified'] == 1816, 'Prior source-versioned union changed')
    remaining = {r['key'] for r in before['remaining_original_rows']['requirements']}
    require(set(KEYS+['All Others!E120', 'FRR!E472', 'FRR!E477']) <= remaining
            and 'All Others!E153' not in remaining and 'FRR!E474' not in remaining,
            'Parent/sibling conjunction policy changed')
    original_review = load(q['original_closure_review'])
    originals = {r['key']: r for r in original_review['reviewed_open_rows']}
    require(scope['original_closure_records'] == {k: originals[k] for k in KEYS}, 'Original gate record overwritten')
    contracts = load(q['contract_review'])
    require(len(scope['excluded_rows']) == 35 and {r['key'] for r in scope['excluded_rows']}
            == set(contracts['MATLAB_hold_keys'])-set(KEYS)
            and all(r['retained_closure_record']['original_row']['closure_gate'] == scope['retained_bundle_gate']
                    for r in contracts['MATLAB_original_rows']), 'Bundle gate or unresolved sibling partition changed')
    require(q['original_hold_keys'] == load(q['hold_receipt'])['unchanged_original_hold_keys']
            and len(set(q['original_hold_keys'])) == 59 and set(q['original_hold_keys']) <= remaining,
            'Original59 hold identities/history changed')
    # Reuse the complete creation audit and cleanup counterexample audit at
    # their recorded sources. Neither reruns a native mission or waives gates.
    cr = creation.audit(repo, Path(q['creation_stage']))
    cl = cleanup.audit(repo, Path(q['io_stage']))
    require(cr['new_requirement_credit'] == cl['new_requirement_credit'] == 0
            and cr['original_key'] == 'All Others!E152' and cl['status'].startswith('cleanup-repair-verified'),
            'Required supporting audits missing')
    stage = Path(q['io_stage']); helper = load(stage/'runtime-helper-binding-v1.json')
    cache, source_cache, summaries, proofs = {}, {}, {}, {}
    require(set(q['cases']) == {'headless', 'visible'}, 'Missing mode')
    for mode, selections in q['cases'].items():
        require(set(selections) == set(POSITIVES+NEGATIVES), 'Missing supported/negative profile')
        for case, record in selections.items():
            name, version = record['run'], record['source_version']
            if name not in cache:
                receipt = load(stage/name/'results.json'); mapped = load(stage/(name+'-loaded-images.json'))
                require(receipt['mode'] == mode and receipt.get('finished_utc') and receipt['end_reason'] == 'completed-selection'
                        and mapped['inputs_unchanged'] is True and mapped['runtime_binary_copies'] == 0, 'Nonterminal/mutated run')
                # A selected passing case may belong to an older failed batch.
                # Keep the actual batch return code and every other result.
                active = {r['name']: r for r in receipt['results'] if r['status'] != 'not-run'}
                require((mapped['returncode'] == 0) == all(r['status'] == 'passed' for r in active.values()), 'Batch status reclassified')
                observed_images = {}
                for process in mapped['processes']:
                    for snapshot in process['snapshots']:
                        require(hashlib.sha256(snapshot['maps'].encode()).hexdigest() == snapshot['maps_sha256']
                                and all(p in snapshot['maps'] for p in snapshot['project_images']),
                                'Raw process mapping differs')
                        observed_images.update(snapshot['project_images'])
                require(observed_images == mapped['loaded_project_images'], 'Actual loaded-image census differs')
                cache[name] = receipt, mapped, active
            receipt, mapped, active = cache[name]
            if version not in source_cache:
                source_cache[version] = creation.archive(stage/f'executed-sources-v{version}.tar.gz', load(stage/f'executed-source-sha256-v{version}.json'))
            frozen = source_cache[version]
            source_hashes = {p: hashlib.sha256(b).hexdigest() for p, b in frozen.items()}
            require(mapped['source_sha256'] == source_hashes, 'Native source version differs')
            source_hashes.setdefault(helper['member'], helper['source_sha256'])
            require(all(source_hashes.get(p) == h for p, h in receipt['source_hashes'].items()), 'Declared runtime source omitted')
            result = active[PREFIX+case]
            require(result['status'] == 'passed' and result['returncode'] == 0, 'Selected case not passed')
            directory = Path(result['log']).parent
            tests = list(ET.parse(directory/'ctest.xml').getroot().iter('testcase'))
            require(len(tests) == 1 and tests[0].get('name') == PREFIX+case
                    and not any(tests[0].find(k) is not None for k in ['failure', 'error', 'skipped']), 'Passing JUnit absent')
            proof = load(directory/'proof.json')
            emitted = re.findall(r'NASA_CHECK (\{[^\n]+\})', Path(result['log']).read_text())
            require(len(emitted) == 1 and json.loads(emitted[0]) == proof, 'Native stdout/proof differ')
            check(proof, case, directory); proofs[case] = proof
            process = [p for p in mapped['processes'] if p['role'] == 'I/O-test' and p['command'][-1] == case]
            require(len(process) == 1 and process[0]['executable'] in result['registered_command']
                    and mapped['loaded_project_images'][process[0]['executable']] == receipt['binary_hashes'][process[0]['executable']]
                    and any(Path(p).name.startswith('libMatlabInterface.so') for p in mapped['loaded_project_images'])
                    and any(p.startswith('/usr/local/MATLAB/R2026b/') and Path(p).name == 'libeng.so' for p in mapped['loaded_project_images']),
                    'Actual native/plugin/Engine mapping absent')
            traces = [json.loads(l) for l in (directory/('trace-'+str(process[0]['pid'])+'.jsonl')).read_text().splitlines()]
            require(traces[0]['event'] == 'start' and traces[-1]['event'] == 'summary' and traces[-1]['screenshots'] == 0,
                    'Terminal observer absent')
            if mode == 'visible':
                evidence = {n: dict(log=r['log'], log_sha256=sha(r['log']), junit=str(Path(r['log']).parent/'ctest.xml'),
                            junit_sha256=sha(Path(r['log']).parent/'ctest.xml')) for n, r in active.items()}
                validate_visible(receipt, evidence)
                require(sum(r['event'] == 'native_exposure' and r.get('exposed', False) for r in traces)
                        == result['trace']['native_exposure_events'] > 0, 'Actual visible exposure count differs')
            summaries[mode+'-'+case] = dict(run=name, source_version=version, pid=process[0]['pid'],
                    native_exposures=result['trace']['native_exposure_events'], batch_returncode=mapped['returncode'])
    # Never reclassify the unresolved whole-object/literal probes as passes.
    failures = {r['name']: r for r in load(stage/'headless-v2/results.json')['results']}
    require(all(failures[PREFIX+c]['status'] == 'failed' for c in ['Object', 'Literals']), 'Unresolved original probes hidden')
    negatives = []
    for label, case, mutate in [('wrong-result', 'Array', lambda p:p['phases'][0]['outputs'][0].__setitem__(0, 999)),
        ('quote-loss', 'QuotedString', lambda p:p['phases'][0]['outputs'].__setitem__(0, "pilot's Ω, log|done")),
        ('missing-nesting-call', 'Scalar', lambda p:p['phases'][0].update(function_calls=2)),
        ('missing-reopen', 'Scalar', lambda p:p['phases'].pop()),
        ('lost-no-output-call', 'NoOutput', lambda p:p['phases'][0].update(matlab_calls=3)),
        ('wrong-type-guard', 'WrongAliasType', lambda p:p.update(actual_type='Variable')),
        ('unearned-native-credit', 'Scalar', lambda p:p.update(new_requirement_credit=2))]:
        corrupt = copy.deepcopy(proofs[case]); mutate(corrupt)
        try: check(corrupt, case)
        except (ValueError, AssertionError): negatives.append(label)
        else: raise ValueError('Accepted corrupt MATLAB scope proof '+label)
    q['audit_summary'] = dict(qualified_keys=KEYS, profiles_per_mode=14, GUI_case_contexts=28,
        positive_missions=66, actual_MATLAB_calls=264, MATLAB_calls_inside_functions=198,
        GMAT_nested_wrapper_calls=66, native_exposures=sum(r['native_exposures'] for r in summaries.values()),
        case_source_bindings=summaries, negative_controls=negatives, retained_bundle_gate_status='open',
        unresolved_original_MATLAB_siblings=35, unresolved_positive_probes=['Object', 'Literals'],
        creation_component_audit_reused=True, cleanup_counterexample_audit_reused=True)
    return q, KEYS


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--qualification', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args(); repo = Path(__file__).resolve().parents[3]
    q, keys = qualify(repo, args.qualification, nasa.inventory(repo))
    require(not args.output.exists(), 'Audit output exists')
    args.output.write_text(json.dumps(dict(status='passed', qualified_keys=keys, summary=q['audit_summary'],
        scientific_approval=False, technical_core_approval=False, full_current_build_qualified=False,
        full_NASA_qualified=False, MATLAB_bundle_qualified=False), indent=2)+'\n')
    print(json.dumps({k:q['audit_summary'][k] for k in ['qualified_keys', 'GUI_case_contexts', 'actual_MATLAB_calls', 'negative_controls']}))
