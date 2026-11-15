"""Conjoin saved Mission Tree operation witnesses under native context rules."""
from pathlib import Path
import json
import math
import hashlib
import importlib

KEY = 'All Others!E196'
FINGERPRINT = '78f59a2dfad28dde8b80ad2f84fc503ae39f639f662440b8fc81ffd18291c0f3'
READERS = {
    'extension': 'AuditCommandOperationExtension', 'control': 'AuditCommandControlOperations',
    'output': 'AuditCommandOutputOperations', 'structural': 'AuditCommandStructuralOperations',
    'alias': 'AuditCommandAliasOperations', 'backend': 'AuditCommandBackendOperations',
    'navigation': 'AuditCommandNavigationOperations',
}
load = lambda p: json.loads(Path(p).read_text())
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()


def need(value, message):
    if not value:
        raise ValueError(message)


def compare(a, b, bounds):
    need(len(a) == len(b) == len(bounds), 'Changed operation result shape')
    need(all(math.isfinite(x) and abs(x-y) <= t for x,y,t in zip(a,b,bounds)), 'Independent operation result differs')


def circle(seconds):
    radius = 7000.; mu = 398600.4415
    speed = math.sqrt(mu/radius); theta = math.sqrt(mu/radius**3)*seconds
    return [radius*math.cos(theta), radius*math.sin(theta), 0, -speed*math.sin(theta), speed*math.cos(theta), 0]


def check_special(kind, proof):
    need(proof['outcome'] == 'passed', 'Special-command check did not pass')
    if kind == 'matlab':
        need(proof['family'] == 'CallMatlabFunction' and proof['position'] in ['Root','Branch'], 'Wrong MATLAB operation context')
        need(proof['operations'] == ['open-context-reuse','close-tree','open-double-click','insert-before',
             'insert-after','delete-cancel-confirm','summary-context','unicode-reopen'], 'Missing MATLAB operation')
        need([v['phase'] for v in proof['results']] == [0,1], 'Missing MATLAB reopen run')
        for row in proof['results']:
            compare(row['raw'], [20]+circle(20)+[41,0,1,4,0], [1e-4]+[2e-5]*3+[2e-8]*3+[1e-12]*5)
            compare(row['expected'], [20]+circle(20)+[41,0,1,4,0], [1e-10]*12)
    elif kind == 'starter':
        need(proof['family'] == 'BeginMissionSequence' and proof['position'] == 'Original', 'Wrong starter context')
        need(proof['menu_states'] == {'Delete command':False, 'Edit command…':False, 'Insert after…':True,
             'Insert before…':False, 'missionCloseCommand':True, 'missionCommandSummary':True}, 'Starter protection changed')
        need(proof['operations'] == ['context-protection','doubleclick-protection','close-no-owner',
             'insert-after','summary-context','unicode-reopen'], 'Missing starter operation disposition')
        need(proof['insertions'] == [{'parent_label':'','parent_type':'Root','side':'after'}], 'Starter insertion moved')
        need([x['phase'] for x in proof['results']] == [0,1], 'Starter reopen missing')
        for row in proof['results']:
            compare(row['raw'], [20]+circle(20)+[41,0,1,0,0], [1e-4]+[2e-5]*3+[2e-8]*3+[1e-12]*5)
            compare(row['summary_expected'], circle(0), [1e-10]*6)
    elif kind == 'prepare':
        need(proof['family'] == 'PrepareMissionSequence' and proof['position'] == 'Original', 'Wrong prepare context')
        need(proof['operations'] == ['open-context-reuse','close-tree','open-double-click','insert-before-native-rejected',
             'insert-after','delete-cancel-native-rejected','summary-context','unicode-reopen'], 'Missing prepare operation disposition')
        need([x['phase'] for x in proof['results']] == [0,1], 'Prepare reopen missing')
        for row in proof['results']:
            p = row['prepared']
            need(p['Count'] == 0 and p['epoch'] == 21545 and p['report_exists'] is False, 'Prepare executed the following mission')
            compare(p['cartesian'], circle(0), [1e-10]*6)
            compare(row['normal'], [20]+circle(20)+[41,21,1,0,0], [1e-4]+[2e-5]*3+[2e-8]*3+[1e-12]*5)
    elif kind == 'hidden':
        need(proof['factory_results'] == [{'created':True,'requested':'NoOp'}, {'created':True,'requested':'EndScript'},
             {'created':True,'requested':'Create'}, {'created':False,'requested':'CallFunction'}], 'Wrong hidden/parser factory roles')
        cases = {v['family']:v for v in proof['cases']}
        need(set(cases) == {'NoOp','EndScript','Create'}, 'Missing hidden command checks')
        for family, count, tail in [('NoOp',2,21),('EndScript',1,3)]:
            row = cases[family]
            need(row['built'] and row['graph_count'] == count and row['tree_count'] == row['actual_tree_count'] == 0,
                 'Hidden command became an ordinary editable tree command')
            need(row['expanded_count'] == (0 if family == 'NoOp' else 1), 'Expanded structural boundary changed')
            compare(row['raw'], [20]+circle(20)+[41,tail,1,0,0], [1e-4]+[2e-5]*3+[2e-8]*3+[1e-12]*5)
        need(cases['Create']['built'] is False and cases['Create']['reason'] ==
             'GMAT currently requires that all objects are created before the mission sequence begins', 'Definition-only rule lost')
    else:
        raise ValueError('Unknown special operation class')


def audit_evidence(index):
    modules = {k:importlib.import_module(v) for k,v in READERS.items()}
    records = index['records']
    seen = {}
    for name, record in records.items():
        run = Path(record['run']); proof = load(run/'proof.json'); result = load(run/'result.json')
        need(result['status'] == 'passed' and result['returncode'] == 0, 'Original GUI check failed')
        if record['reader'] == 'matlab':
            need(result['requested_visibility'] == record['profile'], 'MATLAB run used the wrong visibility route')
            if record['profile'] == 'visible':
                need(result['trace']['native_exposure_events'] > 0, 'MATLAB visible run never exposed a native widget')
        lines = (run/'test.log').read_text().splitlines()
        emitted = [json.loads(line.split('NASA_CHECK ',1)[1]) for line in lines if 'NASA_CHECK ' in line]
        need(emitted == [proof], 'Assertion receipt differs from executed output')
        if record['reader'] in modules:
            modules[record['reader']].check(proof, proof['family'], proof['position'])
            seen.setdefault(proof['family'], set()).add(proof['position'])
        else:
            check_special(record['reader'], proof)
            if record['reader'] == 'matlab':
                seen.setdefault('CallMatlabFunction', set()).add(proof['position'])
        for phase in proof.get('results', []):
            raw = run/(str(phase['phase'])+'-states.csv')
            if raw.exists() and 'raw' in phase:
                need([float(v) for v in raw.read_text().strip().split(',')] == phase['raw'], 'Raw operation output differs')
            if record['reader'] == 'matlab':
                import re
                summary = (run/(str(phase['phase'])+'-summary.txt')).read_text()
                need('Spacecraft       : Sat' in summary and 'Coordinate System: EarthMJ2000Eq' in summary, 'MATLAB summary identity missing')
                for axis, wanted in zip(['X','Y','Z','VX','VY','VZ'], circle(20)):
                    values = re.findall(r'(?:^|\s)'+axis+r'\s*=\s*([-+0-9.eE]+)', summary)
                    need(len(values) == 1 and abs(float(values[0])-wanted) <= 2e-5, 'MATLAB command summary state differs')
    need(len(records) == 105, 'Operation witness set changed')
    need(len(index['factory_identifiers']) == len(set(index['factory_identifiers'])) == 54, 'Command universe changed')
    for record in records.values():
        if record['reader'] == 'matlab':
            census = load(Path(record['run'])/'factory-commands.json')
            need(len(census) == 54 and set(census) == set(index['factory_identifiers']), 'Executed MATLAB operation census differs')
    mapping = index['command_dispositions']
    need(set(mapping) == set(index['factory_identifiers']), 'A factory command lacks a GUI disposition')
    for name, row in mapping.items():
        need(row['evidence'] and all(v in records for v in row['evidence']), 'Unsupported operation claim for '+name)
        need(row['role'] in ['editable','constructor-alias','paired-structural','protected-starter','hidden-structural','definition-only','parser-placeholder'], 'Unknown GUI role')
        if row['role'] in ['editable','constructor-alias']:
            groups = {records[k]['reader'] for k in row['evidence']}
            need(not groups & {'starter','prepare','hidden','structural'}, 'Editable command mapped only to a protected marker')
            families = {load(Path(records[k]['run'])/'proof.json')['family'] for k in row['evidence']}
            need(all(len(seen[f]) >= 2 for f in families if f != 'PrepareMissionSequence'), 'Root/nested operation evidence missing')
    matlab = [v for v in records.values() if v['reader'] == 'matlab']
    need({(v['profile'],v['position']) for v in matlab} ==
         {(profile,position) for profile in ['headless','visible'] for position in ['Root','Branch']}, 'Missing MATLAB visibility/context witness')
    return dict(status='passed', command_identifiers=54, recorded_GUI_witnesses=105,
                ordinary_and_alias_checks=98, protected_and_hidden_checks=3,
                new_matlab_GUI_checks=4, native_tests_run=0)


def qualify(repo, path, inventory, admitted):
    q = load(path)
    need(q['contract_kind'] == 'complete-mission-tree-operation-conjunction' and q['qualified_keys'] == [KEY], 'Wrong operation parent')
    known = {r['key']:r for r in inventory['requirements']}
    need(q['authority_sha256'] == inventory['sources'] and q['candidate_rows'] == [known[KEY]] and known[KEY]['fingerprint'] == FINGERPRINT,
         'Original operation parent changed')
    children = ['All Others!E'+str(n) for n in range(197,203)]
    need(set(children) <= set(admitted) and q['children'] == [known[k] for k in children], 'Original six operation children incomplete')
    for p,h in q['artifact_sha256'].items():need(sha(p) == h, 'Immutable operation evidence changed: '+p)
    decision = load(q['scope_decision'])
    need(decision['answer'] == 'Yes—preserve GMAT’s normal context rules' and KEY in decision['applies_to'], 'Native operation scope not authorized')
    index = load(q['evidence_index'])
    need(all(q['artifact_sha256'].get(p) == h for p,h in index['artifact_sha256'].items()), 'Unbound operation evidence')
    result = audit_evidence(index)
    protection = load(q['protected_check'])
    need(protection == {'status':'passed','count':1334,'unchanged':1333,
         'authorized_display_helper_changes':['src/qtgui/tests/VisibleTestDisplay.py'],
         'unexpected_changes':[], 'original_assertions_changed':0}, 'Unexpected original input change')
    title = load(q['authorized_title_delta'])
    old = Path(title['before']).read_bytes(); new = Path(title['after']).read_bytes()
    need(hashlib.sha256(old).hexdigest() == title['before_sha256'] and
         hashlib.sha256(new).hexdigest() == title['after_sha256'], 'Title source receipt changed')
    need(old.replace(b"TITLE = 'GMAT GUI acceptance \xe2\x80\x94 live test desktop'",
         b"# Xephyr stores -title in legacy WM_NAME/STRING, so keep it ASCII.\nTITLE = 'GMAT GUI acceptance - live test desktop'") == new,
         'Title patch changed more than its documented separator')
    fixed = load(title['native_property_after'])
    need(fixed['status'] == 'passed' and all(bytes.fromhex(p['raw_hex']).isascii() and p['utf8'] == p['latin1'] for p in fixed['properties']),
         'Corrected title lacks a native X11 witness')
    need(q['all_insertion_positions_claimed'] is False and q['native_tests_rerun_for_admission'] == 0, 'Different operation/position scope')
    need(all(q[k] is False for k in ['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified']), 'Unjustified qualification gate')
    result.update(qualified_keys=[KEY], whole_row_conjunction=True, new_requirement_credit=1, children=children,
                  source_versioned_only=True, original_inputs_unchanged=1333, authorized_title_helper_changes=1,
                  original_assertions_changed=0, normal_context_rules_preserved=True,
                  all_insertion_positions_claimed=False, scientific_approval=False, technical_core_approval=False,
                  full_current_build_qualified=False, full_NASA_qualified=False)
    return q, [KEY], result
