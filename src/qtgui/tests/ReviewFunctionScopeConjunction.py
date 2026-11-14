"""Join retained function-scope witnesses without promoting a bounded child.

This review verifies artifact custody, terminal GUI/JUnit/native correspondence
and recorded assertion identities. It does not recompute old numerical oracles,
attribute old runs to the current binary, or infer unrecorded type coverage.
"""
import argparse
import collections
import copy
import hashlib
import json
import re
from pathlib import Path
import xml.etree.ElementTree as ET

from BuildNasaCoverageExtension import require
import NasaGuiCoverage


def digest(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def validate_join(rows, baseline, children):
    require(len(rows) == len(baseline) == 566, 'Factory row omitted')
    require([v['factory_type'] for v in rows] == [v['factory_type'] for v in baseline], 'Factory order/name changed')
    for row, old in zip(rows, baseline):
        require({k: v for k, v in row.items() if k != 'conjunction'} == old, 'Prior role/evidence changed')
        require(not row['qualified'] and not row['whole_function_scope_qualified'] and not row['conjunction']['whole_obligation_qualified'], 'Unsupported universal admission')
        if row['conjunction']['main_private_explicit_global_bounded']:
            contexts = row['conjunction']['contexts']
            require(len(contexts) == 2 and {v['mode'] for v in contexts} == {'headless', 'visible'}, 'Private/Global context omitted')
            require(all(v['kind'] == 'main-private-explicit-global' for v in contexts), 'Different operation substituted')
    require(len(children) == 9 and {v['key'] for v in children} == {'FRR!E' + str(v) for v in range(492, 501)} and all(v['already_recorded'] and v['new_credit'] == 0 for v in children), 'Duplicate/missing child credit')


def review(repo, stage):
    inputs = {}

    def read(p):
        p = Path(p)
        if not p.is_absolute():
            p = repo / p
        require(p.is_file() and not p.is_symlink(), 'Missing regular artifact ' + str(p))
        inputs[str(p)] = digest(p)
        return p

    def load(p):
        return json.loads(read(p).read_text())

    def native(receipt_path, test, source=None, source_sha=None, proof_path=None, embedded_proof=None):
        receipt_path = read(receipt_path)
        receipt = load(receipt_path)
        require(receipt.get('finished_utc') and receipt['end_reason'] == 'completed-selection', 'Retained invocation nonterminal')
        results = [v for v in receipt['results'] if v['name'] == test]
        require(len(results) == 1 and results[0]['status'] == 'passed' and results[0]['returncode'] == 0, 'Retained case did not pass')
        result = results[0]
        log = read(result['log'])
        tests = list(ET.parse(read(log.parent / 'ctest.xml')).getroot().iter('testcase'))
        require(len(tests) == 1 and tests[0].get('name') == test and all(tests[0].find(v) is None for v in ['failure', 'error', 'skipped']), 'Retained JUnit differs')
        emitted = [json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})', log.read_text())]
        require(len(emitted) == 1 and emitted[0]['outcome'] == 'passed', 'Retained native proof absent')
        proof_path = Path(proof_path or log.parent / 'proof.json')
        if proof_path.is_file():
            proof_path = read(proof_path)
            proof = load(proof_path)
        else:
            require(embedded_proof is not None, 'Missing proof file and bound embedded proof')
            proof_path, proof = log, embedded_proof
        require(proof == emitted[0], 'Retained proof differs from native log')
        if source:
            require(source in receipt['source_hashes'] and (source_sha is None or receipt['source_hashes'][source] == source_sha), 'Assertion source unbound')
        elif source_sha:
            require(source_sha in receipt['source_hashes'].values(), 'Assertion identity unbound')
        item = dict(test=test, receipt=str(receipt_path), receipt_sha256=digest(receipt_path), proof=str(proof_path), proof_sha256=digest(proof_path), assertion_source=source, assertion_source_sha256=receipt['source_hashes'].get(source) if source else source_sha, original_binary_only=True, scope=proof.get('scope', ''))
        return item, proof

    prior = repo / 'build/nasa-coverage/coverage95-20261103-function-math-resources'
    require(digest(read(prior / 'stage-receipt-v1.json')) == '1444fea43db246359e00f0a621a897c3205f15d7b5ee129ff8e8caabe29ee5db', 'Previous seal changed')
    receipt = load(prior / 'stage-receipt-v1.json')
    sealed = load(prior / 'evidence-inputs-v1.json')['artifact_sha256']
    for p, value in sealed.items():
        require(digest(read(p)) == value, 'Previous regular binding changed')
    require(digest(read(prior / 'evidence-inputs-v1.json')) == receipt['manifest_sha256'] and digest(read(prior / 'evidence-audit-v1.json')) == receipt['audit_sha256'], 'Previous audit binding changed')
    baseline = load(prior / 'math-scope-matrix-v1.json')['rows']
    current = load(receipt['canonical_ledger'])
    require(digest(read(receipt['canonical_ledger'])) == receipt['canonical_ledger_sha256'] and current['requirements_still_needed'] == 82, 'Canonical ledger changed')
    original = NasaGuiCoverage.inventory(repo)
    for p, value in original['sources'].items():
        require(digest(read(p)) == value, 'Original authority changed')
    require(original['sources'] == current['authority_sha256'] and len(original['requirements']) == 1158 and len(original['historical_cases']) == 2367, 'Original universe changed')
    requirements = {v['key']: v for v in original['requirements']}
    remaining = {v['key'] for v in current['remaining_original_rows']['requirements']}
    require('FRR!E491' in remaining, 'Open parent changed')
    children = [dict(key='FRR!E' + str(n), text=requirements['FRR!E' + str(n)]['text'], fingerprint=requirements['FRR!E' + str(n)]['fingerprint'], already_recorded='FRR!E' + str(n) not in remaining, new_credit=0) for n in range(492, 501)]
    require(len(receipt['unchanged_prerequisite_holds']) == 59, 'Hold count changed')
    rows = copy.deepcopy(baseline)
    by_name = {v['factory_type']: v for v in rows}
    for row in rows:
        row['conjunction'] = dict(contexts=[], main_private_explicit_global_bounded=False, whole_obligation_qualified=False, gaps=[])
    drivers = dict(new_global_witnesses='NasaFunctionGlobalResourceTests.cpp', remaining_resource_witnesses='NasaFunctionRemainingResourceTests.cpp', attitude_scope_witnesses='NasaFunctionRemainingResourceTests.cpp', math_private_global_witnesses='NasaFunctionMathResourceTests.cpp')
    bounded = []
    for row in rows:
        fields = [k for k in drivers if row[k]]
        if not fields:
            continue
        require(len(fields) == 1, 'Overlapping bounded family requires explicit selection')
        field = fields[0]
        for context in row[field]:
            p = Path(context['proof'])
            array = row['factory_type'] == 'Array'
            driver = 'NasaFunctionGlobalArrayTests.cpp' if array else drivers[field]
            item, proof = native(p.parents[1] / 'results.json', p.parent.name, 'src/qtgui/tests/' + driver, proof_path=p)
            require(digest(p) == context['proof_sha256'] and (proof['checkpoint'] == 'function-global-array-return' if array else proof['type'] == row['factory_type']), 'Bounded factory/context differs')
            item.update(mode=context['mode'], kind='selected-array-private-global-return' if array else 'main-private-explicit-global')
            row['conjunction']['contexts'].append(item)
        if row['factory_type'] != 'Array':
            row['conjunction']['main_private_explicit_global_bounded'] = True
            bounded.append(row['factory_type'])
    require(len(bounded) == 125, 'Bounded resource set differs')

    def add(name, kind, item):
        require(name in by_name, 'Context type outside complete factory map')
        by_name[name]['conjunction']['contexts'].append(dict(item, kind=kind))

    # Reuse both positive and domain-recovery records, without calling a
    # recovery-only result a new factory spelling or a whole resource contract.
    old = load('build/nasa-coverage/coverage95-20261103-function-resource-eligibility/scope-review-v1.json')
    builtin_types = set()
    for context in old['retained_contexts']:
        if context['group'] not in {'function-builtin-review', 'function-reference-resource-review'} or context['case'] == 'Catalog':
            continue
        item, proof = native(context['receipt'], context['test'], source_sha=context['assertion_source_sha256'], proof_path=context['proof'])
        require(digest(context['proof']) == context['proof_sha256'] and digest(context['receipt']) == context['receipt_sha256'] and context['assertion_hash_in_original_receipt'], 'Old context binding differs')
        item['mode'] = 'headless' if context['mode'] == 'unattended' else context['mode']
        name = proof.get('builtin_type', proof.get('type'))
        if context['group'] == 'function-builtin-review':
            builtin_types.add(name)
            add(name, 'retained-builtin-local-use-and-selected-oracle', item)
        elif name in {'Barycenter', 'LibrationPoint'}:
            add(name, 'retained-function-local-reference-resource-and-output', item)
    require(len(builtin_types) == 15, 'Retained builtin spelling set differs')
    prop_contexts = load('build/nasa-coverage/function-propagator-global-review/case-contexts.json')
    for context in prop_contexts:
        claim = context['claim']
        p = context['receipt']['receipt']
        require(digest(read(p)) == context['receipt']['receipt_sha256'], 'Propagator context changed')
        item, proof = native(p, claim['test'], claim['source'], claim['source_sha256'])
        require(proof['automatic_global'] and proof['inner_propagations'] == 6 and proof['missions'] == 2, 'Automatic propagator proof differs')
        item['mode'] = 'headless' if context['mode'] == 'unattended' else context['mode']
        add('PropSetup', 'retained-main-created-automatic-global-setup-use', item)
        add('PrinceDormand78', 'retained-owned-method-use-via-automatic-global-PropSetup', item)
    frame = load('build/nasa-coverage/function-frame-review/validation-summary.json')
    for group in frame['source_bound_evidence'].values():
        for context in group['passed_details']:
            case = context['name'].removeprefix('QtGui.NasaFunctionFrame')
            if case not in {'Array', 'String', 'Automatic', 'Coordinate', 'Defaults'}:
                continue
            require(digest(read(group['receipt'])) == group['receipt_sha256'], 'Frame receipt changed')
            item, proof = native(group['receipt'], context['name'], 'src/qtgui/tests/NasaFunctionFrameTests.cpp', embedded_proof=context['proof'])
            require(proof == context['proof'] and proof['mode'] == case, 'Frame native proof differs')
            item['mode'] = 'visible'
            if case in {'Array', 'String'}:
                add(case, 'retained-recursive-user-resource-use', item)
            elif case == 'Automatic':
                add('Planet', 'retained-existing-Earth-solar-model-use-only', item)
            else:
                add('CoordinateSystem', 'retained-main-created-automatic-global-frame-use', item)
                add('MJ2000Eq', 'retained-owned-axes-use-via-automatic-global-frame', item)
    io = load('build/nasa-coverage/function-types-review/case-contexts.json')
    for context in io:
        if context['case'] not in {'Scalar', 'Array', 'String'}:
            continue
        p = context['receipt']['receipt']
        require(digest(read(p)) == context['receipt']['receipt_sha256'], 'Typed resource receipt changed')
        item, proof = native(p, context['test'], 'src/qtgui/tests/NasaFunctionIoTests.cpp', embedded_proof=context['proof'])
        require(proof == context['proof'], 'Typed resource native proof differs')
        item['mode'] = 'headless' if context['mode'] == 'unattended' else context['mode']
        add({'Scalar': 'Variable', 'Array': 'Array', 'String': 'String'}[context['case']], 'retained-typed-user-resource-function-use', item)
    # NestedShared actually creates, resolves and calls ReadShared in a function.
    # This binds one helper route; it does not establish every function resource
    # or an automatic-global pointer identity for the helper object itself.
    for context in by_name['Add']['conjunction']['contexts']:
        item = dict(context)
        nested = read(Path(item['proof']).parent / 'nested.gmf').read_text()
        require('Create GmatFunction ReadShared;' in nested and '[Mark] = ReadShared();' in nested, 'Nested helper route absent')
        add('GmatFunction', 'retained-function-local-helper-construction-and-call', item)
    vector_source = 'src/qtgui/tests/NasaFunctionVectorResourceTests.cpp'
    source_text = read(vector_source).read_text()
    vectors = re.findall(r'\{"(\w+)",\{([^}]+)\},(?:false|true)\}', source_text)
    parameter_types = {n for n, _ in vectors}
    parameter_types.update(v for _, raw in vectors for v in re.findall(r'"([^"\n]+)"', raw))
    require(len(vectors) == 6 and len(parameter_types) == 41 and all(by_name[n]['role'] == 'owner-bound-system-parameter' for n in parameter_types), 'Vector/component parameter names differ')
    for invocation, mode in [('function-vectors-headless-v11', 'headless'), ('function-eligibility-visible-v1', 'visible')]:
        item, proof = native(repo / 'build/gui-acceptance' / invocation / 'results.json', 'QtGui.NasaFunctionVectorResource', vector_source, digest(repo / vector_source))
        require(proof['types'] == [n for n, _ in vectors] and proof['component_comparisons'] == 756 and proof['missions'] == 3, 'Vector routing scope differs')
        item['mode'] = mode
        for name in parameter_types:
            add(name, 'retained-main-private-global-owner-vector-or-component-use', item)

    automatic = [v for v in rows if v['role'].startswith('resource-candidate') and v['nominal_category_base_global_default']]
    require(len(automatic) == 28, 'Nominal automatic resource set differs')
    for row in rows:
        clause = row['conjunction']
        if clause['main_private_explicit_global_bounded']:
            clause['gaps'].append('Selected construction/Write/property/scope only; configured provider/graph operations and universal use remain unqualified.')
        elif row['role'] == 'owner-bound-system-parameter':
            clause['gaps'].append('Retain owner/dependency binding, domains and global-owner scope; no bare-resource exclusion or 321-way science inference.')
            if not clause['contexts']:
                clause['gaps'].append('No exact-type execution context joined by this review; search retained parameter families before scheduling new runs.')
        elif row['category_id'] in {110, 152}:
            clause['gaps'].append('Owner construction/use inside functions and automatic/shared-owner identity need type-specific evidence; named-probe failure is not an exclusion.')
        elif row['category_id'] == 109:
            clause['gaps'].append('Separate E477 command obligation; preserve SaveMission conflict and existing command matrix, without adding resource exclusions.')
        elif row in automatic:
            clause['gaps'].append('Constructor-table default alone is not an eligibility exception; join actual instances, function-local construction/use and automatic-global identity.')
            if row['category_id'] == 117:
                clause['gaps'].append('Existing Earth use does not establish function-local creation/use of all five advertised celestial-body classes.')
        else:
            clause['gaps'].append('Keep exact resource creation/use/eligible-Global obligation and inspect retained evidence before adding tests.')
    validate_join(rows, baseline, children)
    negatives = []
    for label, corrupt in [
        ('missing-factory-row', lambda x, y: x.pop()),
        ('unsupported-whole-admission', lambda x, y: x[0]['conjunction'].__setitem__('whole_obligation_qualified', True)),
        ('missing-Global-mode', lambda x, y: next(v for v in x if v['factory_type'] == 'Add')['conjunction']['contexts'].pop()),
        ('changed-prior-role', lambda x, y: x[0].__setitem__('role', 'excluded')),
        ('duplicate-child-credit', lambda x, y: y[0].__setitem__('new_credit', 1)),
    ]:
        a, b = copy.deepcopy(rows), copy.deepcopy(children)
        corrupt(a, b)
        try:
            validate_join(a, baseline, b)
        except (ValueError, AssertionError):
            negatives.append(label)
        else:
            raise ValueError('Accepted corrupt conjunction ' + label)
    for p, value in inputs.items():
        require(digest(p) == value, 'Input changed during review')
    no_context = sorted(v['factory_type'] for v in automatic if not v['conjunction']['contexts'])
    return dict(status='passed-retained-function-scope-conjunction-review;parent-open', qualified_keys=[], parent=requirements['FRR!E491'], already_recorded_children=children, coverage=current['coverage'], requirements_still_needed=82, unchanged_prerequisite_holds=receipt['unchanged_prerequisite_holds'], factory_entries=566, role_counts=dict(collections.Counter(v['role'] for v in rows)), bounded_main_private_explicit_global_types=sorted(bounded), nominal_automatic_resource_types=[v['factory_type'] for v in automatic], nominal_automatic_without_joined_context=no_context, joined_vector_component_parameter_types=sorted(parameter_types), system_parameter_types_without_joined_execution=sorted(v['factory_type'] for v in rows if v['role'] == 'owner-bound-system-parameter' and not v['conjunction']['contexts']), owned_component_types_with_joined_execution=sorted(v['factory_type'] for v in rows if v['category_id'] in {110, 152} and v['conjunction']['contexts']), rows=rows, negative_controls=negatives, artifact_sha256=inputs, new_native_runs=0, production_changed_files=[], retained_numerical_oracles_recomputed=False, old_evidence_attributed_to_current_binary=False, scientific_approval=False, technical_core_approval=False, full_current_build_qualified=False, full_NASA_qualified=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = review(Path(__file__).resolve().parents[3], args.stage.resolve())
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: result[k] for k in ['status', 'factory_entries', 'nominal_automatic_without_joined_context', 'owned_component_types_with_joined_execution', 'negative_controls']}, indent=2))
    print('Bounded resource types:', len(result['bounded_main_private_explicit_global_types']))
    print('Joined vector/component parameters:', len(result['joined_vector_component_parameter_types']))
    print('System parameters still needing retained-context search:', len(result['system_parameter_types_without_joined_execution']))
