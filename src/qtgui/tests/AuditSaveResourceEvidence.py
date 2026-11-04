"""Check bounded Save native-property evidence; never award an any-Resource row.

Native engine getters supply the observed state, not generated script text or
GUI property mappings. This audit independently compares every captured writable
descriptor and owned object, literal witnesses, raw export and terminal receipts.
Read-only/computed fields, unexecuted resource families and other graph shapes
remain explicit gaps. Original NASA requirement identities are immutable.
"""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path

import NasaGuiCoverage as nasa
from AuditCapiAcceptance import load, require, sha
from AuditCentralBodyPropagationAcceptance import read_run

SOURCE = 'src/qtgui/tests/NasaSaveResourceTests.cpp'
FIXTURE = 'src/qtgui/tests/NasaSaveResourceCases.json'
FACETS = ['actual-save-editor-and-picker', 'selection-order-and-empty-selection-guard',
          'exact-source-Apply-and-following-statement',
          'native-typed-getters-and-owned-object-metadata',
          'standalone-export-rebuild-and-native-property-comparison']


def equal(a, b):
    if type(a) is not type(b):
        # JSON has a single number type; bool must never stand in for a number.
        if type(a) not in (int, float) or type(b) not in (int, float):
            return False
    if type(a) in (int, float):
        return math.isfinite(a) and math.isfinite(b) and abs(a-b) <= 1e-12 * max(1, abs(a))
    if isinstance(a, list):
        return len(a) == len(b) and all(equal(x, y) for x, y in zip(a, b))
    return a == b


def compare(before, after, path=''):
    require(set(before) == set(after) == {'name', 'type', 'fields', 'owned'},
            'Incomplete/substituted native object schema: '+path)
    require(before['type'] == after['type'] and before['name'] == after['name'],
            'Native resource identity changed: '+path)
    fields, got = before['fields'], after['fields']
    # ElectricThruster has a read-only computed Isp inherited from Thruster
    # and a separate writable Isp input. Native IDs, not labels, identify these
    # descriptors; still reject duplicate writable script-facing labels.
    writable = [f['name'] for f in fields if not f['read_only']]
    require(len(fields) == len(got) and len(set(writable)) == len(writable),
            'Incomplete/duplicate writable native parameter inventory: '+path)
    count = 0
    for index, (a, b) in enumerate(zip(fields, got)):
        require(a['id'] == b['id'] == index and all(a[k] == b[k] for k in
                ('name', 'type', 'read_only', 'cloaked')), 'Descriptor metadata changed: '+path)
        if a['read_only']:
            continue
        require('value' in a and 'value' in b and not any(k in f for f in (a, b)
                for k in ('getter_error', 'not_observed')), 'Unchecked writable descriptor: '+path+'.'+a['name'])
        require(equal(a['value'], b['value']), 'Writable native property differs: '+path+'.'+a['name'])
        count += 1
    require(len(before['owned']) == len(after['owned']), 'Owned resource count changed: '+path)
    children=after['owned']
    if before['type'] in ('ForceModel','ODEModel'):
        identity=lambda child:(child['type'],child['name'])
        left=[identity(child) for child in before['owned']];right=[identity(child) for child in children]
        require(len(set(left))==len(left) and len(set(right))==len(right) and set(left)==set(right),
                'Missing/duplicate native force identities: '+path)
        indexed={identity(child):child for child in children};children=[indexed[key] for key in left]
    for index, (a, b) in enumerate(zip(before['owned'], children)):
        count += compare(a, b, path+'/owned['+str(index)+']')
    return count


def emitted_case_metadata(case, code):
    """Model the exact earlier Qt JSON lookup side effect at its source version.

    Inputs remain compared exactly with the archived fixture and baseline
    script. Only the known absent optional member becomes null in metadata;
    nonempty commands, other fields and the input fixture are never normalized.
    """
    expected = dict(case)
    if 'before_save' not in case and 'spec["before_save"].toString()' in code:
        expected['before_save'] = None
    return expected


def check_witnesses(case, snapshot, phase):
    for witness in case['witnesses']:
        object_state=snapshot[witness['object']]
        path=witness.get('owned_types', [])
        require(isinstance(path,list) and all(isinstance(v,str) and v for v in path),
                'Invalid owned witness type path')
        for typename in path:
            matches=[child for child in object_state['owned'] if child['type']==typename]
            require(len(matches)==1, 'Owned witness path missing or ambiguous')
            object_state=matches[0]
        fields = {f['name']: f for f in object_state['fields']}
        field = fields[witness['field']]
        require(field['type'] == witness['type'] and equal(field['value'], witness['expected']),
                'Independent literal/reference witness differs ('+phase+')')


def expand_fixture_value(value, root):
    if isinstance(value, str):
        return value.replace('@FILES@', root)
    if isinstance(value, list):
        return [expand_fixture_value(v, root) for v in value]
    if isinstance(value, dict):
        return {k: expand_fixture_value(v, root) for k, v in value.items()}
    return value


def materialize_case_inputs(case, work, artifacts):
    declared = case.get('fixture_files', {})
    uses_assets = bool(declared) or '@FILES@' in json.dumps(case, ensure_ascii=False)
    manifest_path = work/'fixture-assets.json'
    if not uses_assets:
        require(not manifest_path.exists(), 'Undeclared fixture asset manifest')
        return case
    require(str(manifest_path) in artifacts, 'Fixture asset manifest not bound')
    manifest = load(manifest_path)
    require(set(manifest) == {'root', 'files'} and isinstance(manifest['files'], list),
            'Incomplete fixture asset manifest')
    root = Path(manifest['root'])
    require(root.is_absolute() and root.name == 'fixture-inputs', 'Invalid recorded fixture root')
    records = manifest['files']
    require(len(records) == len(declared) and {v['relative'] for v in records} == set(declared),
            'Missing/duplicate/extra declared fixture file')
    seen = set()
    for record in records:
        require(set(record) == {'relative', 'path', 'artifact', 'sha256', 'bytes'},
                'Incomplete fixture file record')
        relative = Path(record['relative']);artifact = Path(record['artifact'])
        require(not relative.is_absolute() and '..' not in relative.parts
                and len(artifact.parts) == 1 and artifact.name not in seen,
                'Unsafe/duplicate fixture file path')
        seen.add(artifact.name)
        data = declared[record['relative']].encode('utf-8');retained = work/artifact
        require(record['path'] == str(root/relative) and str(retained) in artifacts
                and retained.read_bytes() == data and record['bytes'] == len(data)
                and record['sha256'] == hashlib.sha256(data).hexdigest(),
                'Fixture input bytes, path or identity changed')
    result = copy.deepcopy(case)
    for name in ('setup', 'before_save', 'witnesses', 'reload_probe', 'probe_witnesses'):
        if name in result:
            result[name] = expand_fixture_value(result[name], str(root))
    return result


def check_case(case, result, sources, artifacts, fixture_path=FIXTURE):
    work = Path(result['log']).parent
    saved = load(work/'proof.json')
    require([p for p in nasa.proof_records(result) if p.get('checkpoint') == 'save-resource-native']
            == [saved], 'Native terminal proof differs from saved proof')
    require(result['status'] == 'passed' and result['returncode'] == 0
            and saved['outcome'] == 'passed' and saved['case'] == case['key']
            and saved['facets'] == FACETS and all(saved[k] == case[k] for k in
            ('selected', 'objects', 'witnesses')), 'Substituted Save native GUI contract')
    for name in ('case.json', 'proof.json', 'baseline.script', 'accepted.script', 'export.data',
                 'runtime-native.json', 'reloaded-native.json', 'native-getter-gaps.json',
                 'native-differences.json', 'build.messages.txt', 'run.messages.txt',
                 'reload.messages.txt', 'test.log', 'ctest.xml'):
        require(str(work/name) in artifacts, 'Essential native artifact omitted: '+name)
    code = sources[SOURCE].decode()
    require(load(work/'case.json') == emitted_case_metadata(case, code)
            and [c for c in json.loads(sources[fixture_path])['cases'] if c['key'] == case['key']] == [case],
            'Another fixture executed')
    before, after = load(work/'runtime-native.json'), load(work/'reloaded-native.json')
    require(set(before) == set(after) == set(case['objects']), 'Missing native root resource')
    count = sum(compare(before[n], after[n], n) for n in case['objects'])
    require(count == saved['writable_native_properties_checked'] and count > 0
            and load(work/'native-getter-gaps.json') == load(work/'native-differences.json') == [],
            'Native property checks understated, missing or failed')
    materialized = materialize_case_inputs(case, work, artifacts)
    check_witnesses(materialized, before, 'runtime')
    check_witnesses(materialized, after, 'reloaded')
    raw = (work/'export.data').read_text()
    require(raw.strip() and all(raw.count(token) == 1 for token in case['definitions']),
            'Raw export omitted/duplicated an expected definition')
    baseline, accepted = (work/'baseline.script').read_text(), (work/'accepted.script').read_text()
    expected_baseline = ("% Native Save resource inventory Ω\nCreate Variable Keep;\nKeep = 41;\n"
                         + materialized['setup'] + "BeginMissionSequence;\n"
                         + materialized.get('before_save', '') + "Save 'ExportFamily' Keep; % retain Ω\n"
                         + "Keep = Keep + 1; % outside export\n")
    require(baseline == expected_baseline, 'Baseline differs from exact archived fixture inputs')
    require(accepted == baseline.replace("Save 'ExportFamily' Keep; % retain Ω",
            "Save 'ExportFamily' "+' '.join(case['selected'])+"; % retain Ω")
            and accepted.endswith('Keep = Keep + 1; % outside export\n'),
            'Save GUI Apply changed unrelated source/following statement')
    if materialized.get('reload_probe'):
        for name in ('reload-probe.script', 'reload-probe.messages.txt', 'reload-probe-native.json'):
            require(str(work/name) in artifacts, 'Reload execution probe artifact omitted')
        require((work/'reload-probe.script').read_text() == raw+'\nBeginMissionSequence;\n'+materialized['reload_probe'],
                'Reload probe changed saved resource definitions')
        check_witnesses({'witnesses': materialized['probe_witnesses']},
                        load(work/'reload-probe-native.json'), 'executed-reload-probe')
        require('Save reloaded execution probe differs from independent literal' in code,
                'Native reloaded-execution assertion omitted')
    code = sources[SOURCE].decode()
    require('resourceProperties(' not in code.replace('// Independent of resourceProperties()', '')
            and 'GetGeneratingString(' not in code.replace('and every GetGeneratingString() path.', ''),
            'Serializer/GUI property list substituted for native property oracle')
    for token in ('GetParameterCount()', 'GetOwnedObjectCount()', 'GetOwnedObject(i)',
                  'Save resource following mission statement not executed',
                  'Writable native resource fields lack independent getter checks',
                  'Save changed writable native resource properties or owned objects'):
        require(token in code, 'Native assertion omitted: '+token)
    return count


def audit(repo, path):
    evidence = load(path)
    require(evidence['contract_kind'] == 'source-versioned-bounded-native-save-resource-evidence'
            and evidence['qualified_keys'] == [] and evidence['goal_complete'] is False,
            'Bounded Save pilots silently award original-row credit')
    inventory = nasa.inventory(repo)
    require(evidence['authority_sha256'] == inventory['sources'], 'NASA authority changed')
    for name, digest in evidence['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed retained artifact: '+name)
    matrix = load(evidence['fixture']);cases = {c['key']: c for c in matrix['cases']}
    require(len(cases) == len(matrix['cases']), 'Duplicate Save resource fixture')
    runs = {name: read_run(repo, spec, inventory, evidence['artifact_sha256'], {})
            for name, spec in evidence['runs'].items()}
    require(set(evidence['selected']) == {'headless', 'visible'}, 'Required mode omitted')
    count = 0
    for mode, selected in evidence['selected'].items():
        require(len(selected) == len(cases) and {e['case'] for e in selected} == set(cases),
                'Missing/duplicate passing Save case in mode '+mode)
        for entry in selected:
            receipt, results, sources = runs[entry['run']]
            name = 'QtGui.NasaSaveResource'+entry['case']
            require(receipt['mode'] == mode and entry['test'] == name, 'Substituted mode/test')
            count += check_case(cases[entry['case']], results[name], sources, evidence['artifact_sha256'])
    require(evidence['whole_FRC_E132_qualified'] is False and evidence['remaining_gaps'],
            'Any-Resource obligation hidden behind bounded pilots')
    return dict(status='bounded-native-Save-evidence-passed; original-row-gap-open',
                new_cases=len(cases), contexts=2*len(cases), writable_property_comparisons=count,
                original_row='FRC!E132', new_requirement_credit=[], goal_complete=False)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    p.add_argument('--evidence', type=Path, required=True)
    a = p.parse_args()
    print(json.dumps(audit(a.repo.resolve(), a.evidence)))
