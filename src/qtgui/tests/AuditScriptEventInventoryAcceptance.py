"""Admit the exact Script Event obligation only after its full inventory review."""
import hashlib
import json
from pathlib import Path
import re
import tarfile
import xml.etree.ElementTree as ET

KEY = 'FRC!E138'
FINGERPRINT = 'e34f93e1116a7647e3a96848bbb7c3e1ecc5e9617a52895ff040d252e37145e9'


def load(path):
    return json.loads(Path(path).read_text())


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def qualify(repo, path, inventory):
    q = load(path)
    require(q['contract_kind'] == 'source-versioned-complete-ScriptEvent-supported-command-conjunction' and q['qualified_keys'] == [KEY], 'Changed Script Event obligation')
    require(q['authority_sha256'] == inventory['sources'] and all(q[k] is False for k in ['technical_core_approval','full_current_build_qualified','full_NASA_qualified']), 'Changed original authority or qualification scope')
    original = next(r for r in inventory['requirements'] if r['key'] == KEY)
    require(original['fingerprint'] == FINGERPRINT, 'Original source fingerprint differs')
    review = load(q['source_review'])
    require(review['key'] == KEY and review['fingerprint'] == FINGERPRINT and review['source_text'] == original['text'], 'Original requirement text differs')
    required = {q[k] for k in ['source_review','independent_review','prior_seal','inventory','starter_audit','capi_regression','capi_log','capi_junit','capi_inputs_before','capi_inputs_after','capi_sources','navigation_regression','navigation_log','navigation_junit']}
    require(required <= set(q['artifact_sha256']), 'Essential conjunction evidence not bound')
    for p, expected in q['artifact_sha256'].items():
        require(sha(p) == expected, 'Changed Script Event evidence: '+p)
    independent = load(q['independent_review'])
    require(independent['status'] == 'passed' and independent['qualified_keys'] == [KEY] and independent['complete_supported_command_conjunction'] is True, 'Whole-row independent review missing')
    prior = load(q['prior_seal'])
    require(prior['status'] == 'passed-bounded-evidence-seal' and not prior['goal_complete'], 'Preceding bounded seal differs')
    for p, expected in prior['artifacts'].items():
        require(p in q['artifact_sha256'] and sha(p) == expected, 'Recorded source-version ancestry changed')
    starter = load(q['starter_audit'])
    require(starter['status'] == 'passed-source-versioned-starter-contexts' and len(starter['selected']) == 4, 'Unique starter evidence missing')
    for p, expected in starter['consumed_sha256'].items():
        require(p in q['artifact_sha256'] and sha(p) == expected, 'Starter evidence changed')
    require({(x['family'],x['mode']) for x in starter['selected']} == {(f,m) for f in ['BeginMissionSequence','PrepareMissionSequence'] for m in ['headless','visible']}, 'Incomplete unique-starter modes')
    matrix = load(q['inventory'])
    require(matrix['original_key'] == KEY and matrix['original_fingerprint'] == FINGERPRINT and len(matrix['rows']) == 53, 'Changed supported-command census')
    names = {r['factory_type'] for r in matrix['rows']}
    require(len(names) == 53 and matrix['missing_loaded_executable_types'] == [], 'Missing or duplicate loaded command entry')
    witnesses = [r for r in matrix['rows'] if r['status'] == 'bounded-contained-command-witnesses-available']
    aliases = [r for r in matrix['rows'] if r['status'].startswith('source-backed-')]
    require(len(witnesses) == 47 and {r['factory_type'] for r in aliases} == {'Equation','ScriptEvent','CallFunction','Create'}, 'Incomplete concrete/alias conjunction')
    require(set(matrix['other_dispositions_open']) == {'BeginMissionSequence','PrepareMissionSequence'}, 'Unresolved entries silently removed')
    for row in witnesses:
        proofs = row.get('available_proofs', []) + [x['path'] for x in row.get('additional_evidence', [])]
        visible = False
        require(proofs, 'Unwitnessed contained command: '+row['factory_type'])
        for p in proofs:
            result_path = str(Path(p).with_name('result.json'))
            require(p in q['artifact_sha256'] and result_path in q['artifact_sha256'], 'Unbound old proof/result')
            proof, result = load(p), load(result_path)
            require(proof['outcome'] == 'passed' and result['status'] == 'passed' and result['returncode'] == 0, 'Nonpassing concrete witness')
            visible |= result['requested_visibility'] == 'visible'
        require(visible, 'Missing GUI witness: '+row['factory_type'])
    for selected in starter['selected']:
        proof = load(Path(selected['receipt']).with_name('proof.json'))
        require(set(proof['factory_types']) == names and len(proof['factory_types']) == 53, 'Fresh runtime census differs from reviewed full inventory')
    capi = load(q['capi_regression'])
    require(capi['status'] == 'passed' and capi['returncode'] == 0 and capi['inputs_unchanged'] and capi['loaded_project_images'] and capi['proofs'] == [dict(checkpoint='c-api-spacecraft-epoch',outcome='passed',assertions=190,readings=21,units='A1ModJulian',public_ABI=True,private_probe=False,read_only=True,run_calls=9)], 'Affected native preparation regression missing')
    capi_log = Path(q['capi_log']).read_text()
    require([json.loads(line[len('NASA_CHECK '):]) for line in capi_log.splitlines() if line.startswith('NASA_CHECK ')] == capi['proofs'], 'Native client/proof differs')
    before, after = load(q['capi_inputs_before']), load(q['capi_inputs_after'])
    require(before == after and all(before[name] == value for name,value in capi['binaries'].items()), 'C-API input/image identity changed')
    loaded = {line[len('NASA_LOADED '):] for line in capi_log.splitlines() if line.startswith('NASA_LOADED '+str(repo)+'/')}
    require(loaded == set(capi['loaded_project_images']) and all(before[name] == value for name,value in capi['loaded_project_images'].items()), 'Actual loaded native images differ')
    with tarfile.open(q['capi_sources']) as archive:
        frozen = {m.name:hashlib.sha256(archive.extractfile(m).read()).hexdigest() for m in archive if m.isfile()}
    require(frozen == capi['sources'] and all(before[str(Path(repo)/name)] == value for name,value in frozen.items()), 'Native source archive differs')
    capi_tests = list(ET.parse(q['capi_junit']).getroot().iter('testcase'))
    require(len(capi_tests) == 1 and capi_tests[0].get('name') == 'QtAcceptance.NasaCInterfaceSpacecraftepoch' and capi_tests[0].find('failure') is None and capi_tests[0].find('error') is None, 'Native JUnit outcome differs')
    navigation = load(q['navigation_regression'])
    require(navigation['name'] == 'QtGui.MissionNavigation' and navigation['status'] == 'passed' and navigation['returncode'] == 0 and navigation['requested_visibility'] == 'visible', 'Affected mission/source-mapping regression missing')
    navigation_tests = list(ET.parse(q['navigation_junit']).getroot().iter('testcase'))
    require(len(navigation_tests) == 1 and navigation_tests[0].get('name') == navigation['name'] and navigation_tests[0].find('failure') is None and navigation_tests[0].find('error') is None, 'Navigation JUnit outcome differs')
    text = '\n'.join(x.text or '' for x in navigation_tests[0].findall('system-out'))
    prefix = text.rsplit('...\n[This part', 1)[0]
    require(prefix and prefix in re.sub(r'(?m)^\d+: ?', '', Path(q['navigation_log']).read_text()), 'Navigation JUnit/process output differs')
    return q, [KEY]
