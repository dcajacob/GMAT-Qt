"""Exact FRR E519/E520 admission from immutable body-access GUI evidence."""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import re
import tarfile
import xml.etree.ElementTree as ET

import NasaGuiCoverage as nasa
import BuildNasaCoverageExtension as common
import AuditNasaCoveragePrefix as prefix

KEYS = ['FRR!E519', 'FRR!E520']
KIND = 'source-versioned-worksheet-orbit-body-access'
BODIES = ['Sun', 'Mercury', 'Venus', 'Luna', 'Mars', 'Jupiter', 'Saturn',
          'Uranus', 'Neptune', 'Pluto', 'UserMoon', 'UserAsteroid', 'UserComet', 'UserPlanet']
PROPERTIES = ['RMAG', 'SemilatusRectum']
PROFILES = ['headless', 'visible']
CONTEXT = [f'FRR!E{i}' for i in [*range(183, 195), *range(213, 221), *range(501, 526)]]
GATES = ['scientific_approval', 'technical_core_approval', 'full_current_build_qualified',
         'full_NASA_qualified', 'native_OS_input_qualified']
CLAUSES = ['original-body-and-property-domain', 'generic-origin-and-mu-calculation',
           'readonly-native-and-typed-write-guards', 'configured-and-fresh-selector-guards',
           'policy-Apply-Cancel-history-Unicode', 'independent-read-report-function-results',
           'source-versioned-Earth-reuse', 'paired-missing-body-runtime',
           'original-inputs-and-unrelated-gaps-retained']
STATES = [[7000, -1000, 2000, 1, 6, 2], [8100, 1200, -600, -1.5, 5.75, .25]]
sha, load, require = common.sha, common.load, common.require


def expected(prop):
    result = []
    for x, y, z, vx, vy, vz in STATES:
        if prop == 'RMAG':
            value = math.sqrt(x*x + y*y + z*z)
        else:
            h = [y*vz-z*vy, z*vx-x*vz, x*vy-y*vx]
            value = sum(v*v for v in h) / 400000.
        result.extend([value, 3., value])
    return result


def trace(folder, profile):
    paths = list(folder.glob('trace-*.jsonl'))
    require(paths, 'Missing actual GUI trace')
    exposures = 0
    for path in paths:
        events = [json.loads(line) for line in path.read_text().splitlines()]
        require(any(e['event'] == 'start' for e in events) and
                any(e['event'] == 'summary' for e in events), 'Incomplete GUI trace')
        platforms = {e['platform'] for e in events if e.get('platform')}
        # The observer records platform names only at viewing checkpoints.
        # Offscreen runs deliberately have none; their runner binds the QPA
        # environment. Visible evidence must contain actual xcb checkpoints.
        require(platforms == {'xcb'} if profile == 'visible' else
                platforms <= {'offscreen'}, 'Wrong recorded GUI platform')
        require(not any(e.get('screenshot') for e in events), 'Unexpected screenshot claim')
        if profile == 'visible':
            count = sum(e['event'] == 'native_exposure' and e.get('exposed', False) for e in events)
            require(count > 0, 'Visible worker was not exposed')
            exposures += count
    return exposures


def case_contract(proof, raw, script, prop, body, bound):
    require(proof['outcome'] == 'passed' and proof['mode'] == prop, 'Wrong property/outcome')
    origin = 'ReferenceBody' if body.startswith('User') else body
    if body != 'Earth':
        require(proof['body_case'] == body and proof['origin'] == origin and
                proof['reference_mu'] == 400000 and proof['result_bound_km'] == 1e-5,
                'Wrong actual origin or predeclared execution bound')
        kind = body[4:] if body.startswith('User') else 'Star' if body == 'Sun' else 'Moon' if body == 'Luna' else 'Planet'
        require(proof['origin_type'] == kind, 'Wrong native celestial-body class')
    refs = ['Sat.'+origin+'.'+prop]
    fresh = ['Fresh.'+origin+'.'+prop]
    require(proof['references'] == refs and proof['fresh_references'] == fresh,
            'Missing actual configured/fresh origin references')
    require(proof['apply_rejections'] == 4 and proof['cancel_count'] == 3,
            'Incomplete readonly Apply/Cancel transactions')
    guards, diagnostics = proof['guards'], proof['diagnostics']
    require(len(guards) == len(diagnostics) == 4, 'Missing selector or typed rejections')
    wanted = {(field, ref) for field in ['Destination', 'Outputs'] for ref in refs+fresh}
    require({(r['field'], r['reference']) for r in guards} == wanted and
            all(r['accepted'] is False for r in guards), 'Readonly selector admitted writes')
    require({(r['field'], r['reference']) for r in diagnostics} == wanted and
            all(r['diagnostic'] for r in diagnostics), 'Missing typed write rejection/recovery')
    for owner in ('Sat', 'Fresh'):
        require(re.search(r'(?m)^\s*(?:GMAT\s+)?'+owner+
                          r'\.OrbitParameterAccess\s*=\s*Worksheet\s*;', script),
                'Worksheet readonly policy not actually persisted')
    truth = expected(prop)
    require(len(raw) == 6 and raw == proof['actual'] and len(proof['expected']) == 6,
            'Raw report/typed result census differs')
    require(all(math.isfinite(v) and abs(v-w) <= bound for v, w in zip(raw, truth)) and
            all(abs(v-w) <= bound for v, w in zip(proof['expected'], truth)),
            'Independent norm/cross-product result failed')
    require(raw[1] == raw[4] == 3., 'Independent function result changed')
    return max(abs(v-w) for v, w in zip(raw, truth))


def proof_from_log(path):
    records = [json.loads(line.split('NASA_CHECK ', 1)[1])
               for line in path.read_text().splitlines() if 'NASA_CHECK ' in line]
    require(len(records) == 1, 'Missing or repeated GUI proof')
    return records[0]


def component(stage, earth):
    plan = load(stage/'plan-v1.json')
    require(plan['new_body_cases'] == BODIES and plan['properties'] == PROPERTIES and
            plan['profiles'] == PROFILES and plan['new_result_bound'] == 1e-5,
            'Changed predeclared missing-body matrix')
    build = load(stage/'build-worker-v1.json')
    signatures = []
    maximum = exposures = 0
    scenarios = 0
    for profile in PROFILES:
        for body in BODIES:
            for prop in PROPERTIES:
                folder = stage/(profile+'-v1')/(prop+'-'+body)
                record = load(folder/'result.json')
                require(record['status'] == 'passed' and record['returncode'] == 0 and
                        record['case'] == body+':'+prop and record['profile'] == profile and
                        record['timeout_seconds'] == 60, 'Incomplete actual GUI execution')
                sig = record['signature']
                require(sig['source_sha256'] == plan['new_worker_sha256'] == build['source_sha256'] and
                        sig['binary_sha256'] == build['binary_sha256'] and sig['profile'] == profile,
                        'Wrong executed source/binary/profile')
                signatures.append({k:v for k,v in sig.items() if k != 'profile'})
                loaded = [line.strip() for path in folder.glob('loader.*')
                          for line in path.read_text().splitlines()
                          if 'calling init:' in line and 'libGmatBase.so' in line]
                require(loaded == record['actual_loaded_GmatBase'] and loaded and
                        all('coverage95-20261110-de-delta-boundaries-v1/runtime-v1/lib/' in s for s in loaded),
                        'New body worker did not load its bound backend')
                p = proof_from_log(folder/'test.log')
                require(record['proofs'] == [p], 'Stored GUI proof differs from actual log')
                raw = [float(x) for x in (folder/'values.txt').read_text().split()]
                # Admission retains the original Earth 1e-9 mathematical bound,
                # independently rechecking the new worker's retained raw values.
                maximum = max(maximum, case_contract(p, raw,
                    (folder/'mission.script').read_text(), prop, body, 1e-9))
                exposures += trace(folder, profile)
                scenarios += 1
    require(all(sig == signatures[0] for sig in signatures), 'Mixed body-worker input versions')
    for profile, old_profile in [('headless', 'gui-offscreen-v1'), ('visible', 'gui-visible-v1')]:
        selected = {r['name']:r for r in load(earth/old_profile/'results.json')['results']
                    if r['status'] == 'passed'}
        for prop in PROPERTIES:
            name = 'QtGui.WorksheetOrbitAccess'+prop
            require(name in selected and selected[name]['returncode'] == 0, 'Saved Earth case did not pass')
            folder = earth/old_profile/name
            p = proof_from_log(folder/'test.log')
            raw = [float(x) for x in (folder/'values.txt').read_text().split()]
            maximum = max(maximum, case_contract(p, raw,
                (folder/'mission.script').read_text(), prop, 'Earth', 1e-9))
            tests = list(ET.parse(folder/'ctest.xml').iter('testcase'))
            require(len(tests) == 1 and tests[0].get('name') == name and
                    all(tests[0].find(tag) is None for tag in ['failure', 'error', 'skipped']),
                    'Saved Earth CTest failed or changed')
            exposures += trace(folder, profile)
    return {'new_GUI_scenarios': scenarios, 'reused_Earth_GUI_scenarios': 4,
            'declared_default_bodies': 11, 'user_body_factory_types': 4,
            'independent_scalar_results': 360, 'mathematical_admission_bound': 1e-9,
            'maximum_independent_error': maximum, 'visible_native_exposures': exposures,
            'original_assertions_changed_in_component': 0, 'native_tests_rerun_for_admission': 0}


def structure(q, scope, inventory):
    known = {r['key']:r for r in inventory['requirements']}
    require(q['contract_kind'] == KIND and q['qualified_keys'] == KEYS and
            q['authority_sha256'] == inventory['sources'], 'Different/extra body access credit')
    require(scope['candidate_rows'] == [known[k] for k in KEYS] and
            scope['complete_original_context'] == [known[k] for k in CONTEXT],
            'Original row/fingerprint/body/parent context changed')
    require(scope['new_body_cases'] == BODIES and scope['reused_body'] == 'Earth' and
            scope['profiles'] == PROFILES and scope['properties'] == PROPERTIES,
            'Missing original body or GUI profile')
    require(scope['access_policy'] == 'explicit Worksheet' and scope['native_default_changed'] is False and
            scope['mathematical_admission_bound'] == 1e-9 and scope['mu'] == 400000,
            'Readonly contract or original arithmetic limit changed')
    require(scope['generic_source_argument'] and scope['original_assertions_changed_in_component'] == 0 and
            scope['other_row_credit'] == 0 and scope['newly_resolved_hold_keys'] == [] and
            scope['parent_requirements_closed'] == [] and scope['failures_waived'] is False,
            'Unsupported generic scope, parent, hold or waived failure')
    require(set(scope['clauses']) == set(CLAUSES) and
            all(c['satisfied'] is True and c['evidence'] and
                all(p in q['artifact_sha256'] for p in c['evidence'])
                for c in scope['clauses'].values()), 'Unbound or incomplete original conjunction')
    require(all(q[k] is False and scope[k] is False for k in GATES), 'Broader qualification promoted')


def negative_controls(q, scope, inventory):
    """Reject unsupported credit and tampered numerical/write evidence offline."""
    rejected = 0
    def rejects(call):
        nonlocal rejected
        try:
            call()
        except (ValueError, KeyError):
            rejected += 1
        else:
            raise ValueError('Damaged body-access evidence was accepted')
    for field, value in [('qualified_keys', KEYS+['FRR!E501']),
                         ('full_current_build_qualified', True)]:
        changed = copy.deepcopy(q); changed[field] = value
        rejects(lambda: structure(changed, scope, inventory))
    for field, value in [('parent_requirements_closed', ['FRR!E501']),
                         ('failures_waived', True), ('native_default_changed', True)]:
        changed = copy.deepcopy(scope); changed[field] = value
        rejects(lambda: structure(q, changed, inventory))
    folder = Path(q['component_stage'])/'headless-v1/RMAG-Sun'
    proof = proof_from_log(folder/'test.log')
    raw = [float(x) for x in (folder/'values.txt').read_text().split()]
    script = (folder/'mission.script').read_text()
    for field, value in [('origin', 'Earth'), ('guards', proof['guards'][:-1]),
                         ('cancel_count', 0), ('outcome', 'failed')]:
        changed = copy.deepcopy(proof); changed[field] = value
        rejects(lambda: case_contract(changed, raw, script, 'RMAG', 'Sun', 1e-9))
    changed = copy.deepcopy(proof); changed_raw = raw.copy(); changed_raw[0] += 1e-6
    changed['actual'] = changed_raw
    rejects(lambda: case_contract(changed, changed_raw, script, 'RMAG', 'Sun', 1e-9))
    rejects(lambda: case_contract(proof, raw,
        script.replace('OrbitParameterAccess = Worksheet', 'OrbitParameterAccess = Native'),
        'RMAG', 'Sun', 1e-9))
    return rejected


def qualify(repo, path, inventory):
    q = load(path)
    for name, digest in q['artifact_sha256'].items():
        require(sha(name) == digest, 'Changed body-access evidence: '+name)
    scope = load(q['scope_review'])
    structure(q, scope, inventory)
    frozen = {}
    for archive, identities in [('reader_source_archive', 'reader_source_sha256'),
                                ('proof_source_archive', 'proof_source_sha256')]:
        require(q[archive] in q['artifact_sha256'], 'Unbound source archive')
        with tarfile.open(q[archive]) as bundle:
            require(set(bundle.getnames()) == set(q[identities]), 'Source partition changed')
            for name, digest in q[identities].items():
                raw = bundle.extractfile(name).read()
                require(not raw.startswith(b'\x7fELF') and hashlib.sha256(raw).hexdigest() == digest,
                        'Changed/non-source capsule member')
                if identities == 'proof_source_sha256': frozen[name] = raw.decode(errors='replace')
    original = load(q['protected_input_baseline'])
    authorized = load(q['authorized_input_manifest'])
    guard = load(q['component_input_integrity'])
    require(len(original) == len(authorized) == 1334 and
            all(q['proof_source_sha256'].get(n) == h for n,h in original.items()),
            'Historical protected source versions omitted')
    require(sorted(n for n in original if original[n] != authorized[n]) ==
            ['src/qtgui/tests/PlotIntegrationStepTests.cpp', 'src/qtgui/tests/SharedPlaybackTests.cpp'] and
            guard['authorized_inputs_final'] == authorized and guard['changed_by_this_component'] == [],
            'Unacknowledged original-input amendment')
    worker = frozen['src/qtgui/tests/WorksheetOrbitBodyAccessTests.cpp']
    require(hashlib.sha256(worker.encode()).hexdigest() == load(Path(q['component_stage'])/'build-worker-v1.json')['source_sha256'],
            'Executed body worker source capsule differs')
    for token in ['reportPropertyType', 'findText(type)<0', 'Read-only report viewer differs',
                  'source->undo()', 'source->redo()', 'window.saveScriptTo',
                  'GetGravitationalConstant()==400000', 'bodySetup', 'TwoBodyPropagation']:
        require(token in worker, 'Missing actual GUI/body/read arithmetic assertion')
    policy = frozen['src/base/parameter/Parameter.cpp']
    require(all(token in policy for token in ['Parameter::IsSettable', '"Spacecraft"',
        '"RMAG"', '"SemilatusRectum"', '"Worksheet"']), 'Missing generic owner-policy guard')
    require('if (!IsSettable())' in frozen['src/base/parameter/OrbitReal.cpp'] and
            'GetRelativeCartState(mOrigin)' in frozen['src/base/parameter/OrbitData.cpp'],
            'Missing native readonly guard or generic body-relative calculation')
    previous = load(q['prior_full_audit'])
    require(len(previous['original_hold_keys']) == 59 and not set(KEYS) & set(previous['original_hold_keys']),
            'Separate formal hold identities/statuses changed')
    facts = component(Path(q['component_stage']), Path(q['earth_stage']))
    damaged = negative_controls(q, scope, inventory)
    return q, KEYS, {'status':'passed', 'qualified_keys':KEYS, 'whole_row_conjunction':True,
        'component':facts, 'new_requirement_credit':2, 'newly_resolved_original_hold_keys':[],
        'damaged_admission_controls_rejected':damaged,
        'native_tests_rerun':0, 'runtime_binary_copies':0, **{k:False for k in GATES}}


def build(repo, baseline, path, output):
    seal, inventory, keys = prefix.baseline(repo, baseline)
    original = {kind:set(values) for kind,values in keys.items()}
    _, incoming, audit = qualify(repo, path, inventory)
    known = {kind:{row['key']:row for row in inventory[kind]} for kind in keys}
    require(not set(incoming) & keys['requirements'], 'Duplicate body-access credit')
    keys['requirements'] = common.merge_keys(keys['requirements'], incoming, known['requirements'])
    coverage = {kind:{'verified':len(keys[kind]), 'total':len(known[kind]),
        'percent':round(100*len(keys[kind])/len(known[kind]),1),
        'new_verified':len(keys[kind])-len(original[kind])} for kind in keys}
    remaining = {kind:[row for row in seal['remaining_original_rows'][kind]
        if row['key'] not in keys[kind]] for kind in keys}
    result = {'status':'passed', 'scope':'Sealed source-versioned prefix plus complete original body-relative readonly RMAG/SemilatusRectum capability',
        'baseline':str(baseline), 'baseline_sha256':sha(baseline), 'authority_sha256':inventory['sources'],
        'coverage':coverage, 'target_requirements':1101, 'target_reached':len(keys['requirements'])>=1101,
        'requirements_still_needed':max(0,1101-len(keys['requirements'])),
        'additions':[{'key':key, 'kind':'requirements', 'fingerprint':known['requirements'][key]['fingerprint'],
            'source_text':known['requirements'][key]['text'], 'qualification':str(path),
            'qualification_sha256':sha(path), 'status':'verified-source-versioned'} for key in incoming],
        'remaining_counts':{kind:len(rows) for kind,rows in remaining.items()},
        'remaining_original_rows':remaining, 'technical_core_approval':False,
        'full_current_build_qualified':False, 'full_NASA_qualified':False,
        'limits':['Original 1158/2367 denominators, texts and fingerprints are unchanged.',
                  'Only FRR E519/E520 are added; broader body/frame parameter parents and all59 formal hold statuses remain open/unchanged.',
                  'Explicit Worksheet capability is qualified; Native compatibility remains. Physical ephemeris, arbitrary coordinate-system, native OS input, platform and full-build approval remain separate.']}
    with Path(output).open('x') as file:
        json.dump(result, file, indent=2); file.write('\n')
    return result, audit


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--qualification', type=Path, required=True)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.baseline:
        result, _ = build(args.repo, args.baseline, args.qualification, args.output)
        print(json.dumps({'coverage':result['coverage'], 'still_needed':result['requirements_still_needed']}))
    else:
        _, _, result = qualify(args.repo, args.qualification, nasa.inventory(args.repo))
        with args.output.open('x') as file:
            json.dump(result, file, indent=2); file.write('\n')
        print(json.dumps(result))
