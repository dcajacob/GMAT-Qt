"""Admit exact FRR!E404 from preserved interpolation and paired GUI evidence."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import tarfile

import NasaGuiCoverage as nasa
import BuildNasaCoverage95 as builder
import BuildNasaCoverageExtension as common

load, sha, require = common.load, common.sha, common.require
KEY = 'FRR!E404'
KIND = 'source-versioned-complete-interpolation-insufficiency-policy'
FORMATS = ['CCSDS-OEM', 'STK-TimePosVel', 'Code-500']
DEGREES = [1, 3, 5, 7, 9]
GATES = ['scientific_approval', 'technical_core_approval',
         'full_current_build_qualified', 'full_NASA_qualified']
CLAUSES = {'original-row-and-method-context', 'authorized-two-policy-behavior',
           'Lagrange-all-supported-orders', 'Hermite-all-supported-odd-degrees',
           'EOF-events-explicit-input-and-output-sufficiency',
           'user-output-versus-pending-background-data',
           'actual-GUI-configuration-copy-persistence-and-recovery',
           'paired-visible-profile-and-message-window',
           'preserved-original-inputs-failures-and-source-versions'}


def structure(q, scope, inventory):
    known = {r['key']: r for r in inventory['requirements']}
    require(q['contract_kind'] == KIND and q['qualified_keys'] == [KEY], 'Extra or partial interpolation credit')
    require(q['authority_sha256'] == inventory['sources'], 'Original authority changed')
    require(scope['candidate_row'] == known[KEY], 'Original E404 text/fingerprint changed')
    require(scope['original_method_context'] == [known['FRR!E'+str(i)] for i in [374,375,376,382,383,393,394]],
            'Original OEM/Lagrange or SPK/Hermite method context omitted')
    require(scope['worksheet_policy'] == 'Error' and scope['compatibility_policy'] == 'ReduceOrder'
            and scope['native_default_policy'] == 'ReduceOrder' and scope['user_authorized_both_policies'],
            'Mandatory-error profile or explicit compatibility decision changed')
    require(scope['supported_Lagrange_orders'] == list(range(1,11)) and scope['supported_Hermite_degrees'] == DEGREES,
            'Supported requested orders narrowed')
    require(set(scope['clauses']) == CLAUSES and all(v['satisfied'] and v['evidence'] for v in scope['clauses'].values()),
            'Missing original interpolation conjunction')
    require(scope['original_assertions_changed'] == 0 and scope['failed_runs_retained']
            and scope['runtime_binary_copies'] == 0 and scope['whole_row_qualified'], 'Custody/assertion scope changed')
    require(scope['parent_sibling_credit'] == 0 and all(q[k] is False and scope[k] is False for k in GATES),
            'Interpolation admission promoted another row or approval')


def damaged_controls(q, scope, inventory):
    rejected = []
    for fault in ['extra-row','fingerprint','missing-method','missing-order','missing-odd-degree',
                  'compatibility-as-worksheet','missing-clause','lost-failures','assertions','wider-approval']:
        bq, bs = copy.deepcopy(q), copy.deepcopy(scope)
        if fault == 'extra-row': bq['qualified_keys'].append('FRR!E375')
        elif fault == 'fingerprint': bs['candidate_row']['fingerprint'] = '0'*64
        elif fault == 'missing-method': bs['original_method_context'].pop()
        elif fault == 'missing-order': bs['supported_Lagrange_orders'].pop()
        elif fault == 'missing-odd-degree': bs['supported_Hermite_degrees'].pop()
        elif fault == 'compatibility-as-worksheet': bs['worksheet_policy'] = 'ReduceOrder'
        elif fault == 'missing-clause': bs['clauses'].pop(next(iter(CLAUSES)))
        elif fault == 'lost-failures': bs['failed_runs_retained'] = False
        elif fault == 'assertions': bs['original_assertions_changed'] = 1
        else: bq['technical_core_approval'] = True
        try: structure(bq,bs,inventory)
        except (ValueError,AssertionError): rejected.append(fault)
        else: raise ValueError('Damaged interpolation admission accepted: '+fault)
    return rejected


def support(q):
    stage = Path(q['support_stage']); pub = load(stage/'publication-v1.json')
    for name, digest in pub['artifacts'].items(): require(sha(name) == digest, 'Changed interpolation artifact: '+name)
    runs = load(stage/'final-current-v13-results.json')
    require(len(runs['planned']) == len(runs['results']) == 24 and
            all(x['returncode'] == 0 for x in runs['results']) and
            {x['name'] for x in runs['results']} == set(runs['planned']), 'Final interpolation regressions incomplete')
    values = {}
    for mode, count in [('Lagrange',20),('Spk',5),('SpkBackground',10),('Policy',90),('Boundaries',260)]:
        p = stage/'final-current-v13'/('QtGui.InterpolationSufficiency'+mode)/'verification.json'
        d = load(p); require(d['case'] == mode and d['status'] == 'passed' and len(d['observations']) == count,
                            'Missing interpolation mode: '+mode)
        values[mode] = d['observations']
    require({(x['order'],x['direction']) for x in values['Lagrange']} ==
            {(o,d) for o in range(1,11) for d in [1,-1]}, 'Lagrange order/direction partition incomplete')
    require(all(x['minimum_independent_samples'] == x['order']+1 and x['duplicate_replacement'] and
                x['base_order_restored'] and x['max_error'] <= 1e-9 for x in values['Lagrange']),
            'Lagrange independent/sample restoration checks incomplete')
    require({x['degree'] for x in values['Spk']} == set(DEGREES), 'Hermite degree partition incomplete')
    require(all(x['minimum_states'] == max(2,(x['degree']+1)//2) and x['queries'] == 17 and
                x['preflight_failures'] == 3 and x['max_position_error_km'] <= 2e-8 and
                x['max_velocity_error_km_s'] <= 2e-10 and not x['physical_fk5_frame_qualified']
                for x in values['Spk']), 'Hermite independent/preflight conjunction incomplete')
    require({(x['degree'],x['background']) for x in values['SpkBackground']} ==
            {(d,b) for d in DEGREES for b in [False,True]}, 'User/background SPK partition incomplete')
    require(all(x['minimum_states'] == max(2,(x['degree']+1)//2) and
                x['short_input_rejected'] == (not x['background']) and
                x['scratch_completed_after_more_data'] == x['background'] for x in values['SpkBackground']),
            'User-output/scratch sufficiency policy incomplete')
    negatives = 0
    for mode in ['Policy','Boundaries']:
        expected = set()
        for fmt in FORMATS:
            for order in range(1,11):
                scenarios = ['Short','Exact'] if mode == 'Policy' else ['Assignment','Maneuver','Toggle','Explicit','Window']
                for scenario in scenarios:
                    for action in ['Error','ReduceOrder']:
                        if scenario == 'Exact' and action == 'ReduceOrder': continue
                        if fmt == 'Code-500' and scenario in ['Explicit','Window']: continue
                        expected.add((fmt,order,scenario,action))
        actual = {(x['format'],x['order'],x['scenario'],x['action']) for x in values[mode]}
        require(actual == expected and len(actual) == len(values[mode]), 'Missing/duplicate interpolation policy cell')
        for x in values[mode]:
            fmt,order,scenario,action = x['format'],x['order'],x['scenario'],x['action']
            exported = scenario == 'Explicit' and fmt == 'CCSDS-OEM' and order == 1
            strict = action == 'Error' and scenario != 'Exact' and (order > 1 or exported) and (mode == 'Policy' or fmt != 'Code-500')
            require(x['strict_failure_expected'] == x['run_failed'] == strict, 'Incorrect strict/compatibility outcome')
            directory = stage/'final-current-v13'/('QtGui.InterpolationSufficiency'+mode)
            stem = fmt+'-'+str(order)+'-'+scenario+'-'+action
            if strict:
                text = (directory/(stem+'-messages.txt')).read_text()
                require(all(t in text for t in ['Insufficient data','order '+str(order),'requires at least '+str(order+1),
                                                'received '+str(1 if exported else 2),stem+'.eph']),
                        'Actual requested-order error diagnostic omitted')
                negatives += 1
            else: require((directory/(stem+'.eph')).stat().st_size > 0, 'Successful interpolation output absent')
    require(negatives == 118, 'Expected interpolation errors incomplete')
    failures = load(stage/'retained-failures-v1.json')
    require(len(failures['failed_registered_cases']) == 11 and not failures['original_assertions_fixtures_tolerances_changed'],
            'Earlier failures or original-input fidelity omitted')
    return dict(final_registered_passes=24,observations=385,checked_expected_error_runs=negatives,
                Lagrange_cells=20,Hermite_curves=5,CSPICE_queries=85,user_background_cells=10)


def gui(q):
    profiles = [load(q['headless_results']),load(q['visible_results'])]
    require(profiles[0]['source_hashes'] == profiles[1]['source_hashes'] and
            profiles[0]['binary_hashes'] == profiles[1]['binary_hashes'], 'GUI profiles used different sources/runtime')
    names = {'QtGui.InterpolationPolicyGui'+x for x in FORMATS+['SPK']}
    exposures = 0
    for index, profile in enumerate(profiles):
        passed = [x for x in profile['results'] if x['status'] != 'not-run']
        require(len(passed) == 4 and {x['name'] for x in passed} == names and
                all(x['status'] == 'passed' and x['returncode'] == 0 for x in passed), 'GUI format/profile cases incomplete')
        require(profile['end_reason'] == 'completed-selection', 'GUI profile did not terminate')
        if index:
            display = profile['visible_display']
            require(display['start_minimized'] and 'Iconic' in display['initial_host_window_state'] and
                    'FULLSCREEN' not in display['initial_host_window_state'] and display['resizable'] and
                    not display['resize_errors'], 'Visible minimized/resizable/non-fullscreen profile lost')
        for case in passed:
            require(case['requested_visibility'] == ('visible' if index else 'headless') and
                    case['trace']['instrumented_processes'] == 1 and not case['trace']['screenshots'],
                    'GUI profile/observer/capture contract differs')
            require({'Resources','messageWindow','ephemeris_InterpolationOrder','ephemeris_InsufficientDataAction'} <=
                    set(case['trace']['named_objects']), 'Actual interpolation/message controls not observed')
            directory = Path(case['log']).parent; proof = load(directory/'gui-verification.json')
            fmt = proof['format']; spk = fmt == 'SPK'
            require(case['name'] == 'QtGui.InterpolationPolicyGui'+fmt and proof['status'] == 'passed' and
                    proof['strict_order'] == (9 if spk else 7) and proof['strict_policy_configured'] == (not spk) and
                    proof['SPK_policy_disabled'] == spk, 'GUI policy/type applicability narrowed')
            require(all(proof[x] for x in ['actual_resource_tree_editor','draft_cancel_discard','invalid_order_atomic',
                        'apply_undo_redo_idempotent','unicode_persistence','native_clone_copy','strict_run_failed',
                        'actual_message_window_error','recovery_completed']) and not proof['native_OS_input_qualified'],
                    'Actual GUI error/recovery or ownership proof incomplete')
            error = (directory/'strict-messages.txt').read_text()
            require(('SPK records must contain enough states for the interpolation degree' in error) if spk else
                    all(v in error for v in ['Insufficient data','order 7','requires at least 8','received 2']),
                    'Message Window did not display the native interpolation error')
            if not spk:
                require('Export.InsufficientDataAction = Error;' in (directory/'strict.script').read_text() and
                        'Export.InsufficientDataAction = Error;' not in (directory/'recovery.script').read_text() and
                        proof['invalid_policy_rejected'], 'Policy persistence/validation did not survive GUI round trip')
            if index:
                require(case['trace']['native_exposure_events'] > 0, 'Visible GUI did not expose natively')
                exposures += case['trace']['native_exposure_events']
    return dict(paired_GUI_cases=8,formats=4,native_exposure_events=exposures,native_OS_input_qualified=False)


def qualify(repo,path,inventory):
    q = load(path)
    for name,digest in q['artifact_sha256'].items(): require(sha(name) == digest, 'Changed interpolation admission artifact: '+name)
    scope = load(q['scope_review']); structure(q,scope,inventory)
    for archive_key,ids_key in [('reader_source_archive','reader_source_sha256'),('proof_source_archive','proof_source_sha256')]:
        require(q[archive_key] in q['artifact_sha256'], 'Unbound interpolation source archive')
        with tarfile.open(q[archive_key]) as archive:
            require(set(archive.getnames()) == set(q[ids_key]), 'Frozen interpolation source partition differs')
            for name,digest in q[ids_key].items():
                require(hashlib.sha256(archive.extractfile(name).read()).hexdigest() == digest, 'Frozen interpolation source changed')
    protected = load(q['protected_input_baseline'])
    require(len(protected) == 1334 and all(q['proof_source_sha256'].get(n) == v for n,v in protected.items()),
            'Original protected assertion/fixture source versions omitted')
    numerical = support(q); widgets = gui(q)
    require(load(q['build_receipt'])['returncode'] == 0, 'GUI implementation was not compiled')
    require(len(damaged_controls(q,scope,inventory)) == 10, 'Damaged admission controls incomplete')
    return q,[KEY],dict(status='passed',qualified_keys=[KEY],whole_row_conjunction=True,
                       support=numerical,GUI=widgets,damaged_admission_controls_rejected=10,
                       new_requirement_credit=1,native_tests_rerun=0,runtime_binary_copies=0,
                       **{k:False for k in GATES})


def build(repo,baseline,path,output):
    seal,inventory,keys = builder.baseline(repo,baseline)
    original = {k:set(v) for k,v in keys.items()}
    q,qualified,audit = qualify(repo,path,inventory)
    known = {k:{r['key']:r for r in inventory[k]} for k in keys}
    require(KEY not in keys['requirements'], 'Duplicate E404 admission')
    keys['requirements'] = common.merge_keys(keys['requirements'],qualified,known['requirements'])
    coverage = {k:dict(verified=len(keys[k]),total=len(known[k]),percent=round(100*len(keys[k])/len(known[k]),1),
                      new_verified=len(keys[k])-len(original[k])) for k in keys}
    remaining = {k:[r for r in seal['remaining_original_rows'][k] if r['key'] not in keys[k]] for k in keys}
    result = dict(status='passed',scope='95-percent campaign: sealed source-versioned prefix plus exact E404 interpolation policy.',
        baseline=str(baseline),baseline_sha256=sha(baseline),authority_sha256=inventory['sources'],coverage=coverage,
        target_requirements=1101,target_reached=len(keys['requirements'])>=1101,
        requirements_still_needed=max(0,1101-len(keys['requirements'])),
        additions=[dict(key=KEY,kind='requirements',fingerprint=known['requirements'][KEY]['fingerprint'],
                        source_text=known['requirements'][KEY]['text'],qualification=str(path),
                        qualification_sha256=sha(path),status='verified-source-versioned')],
        remaining_counts={k:len(v) for k,v in remaining.items()},remaining_original_rows=remaining,
        technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,
        limits=['Original 1158 requirement and 2367 historical denominators unchanged.',
                'E404 qualifies the explicit Error worksheet policy; default ReduceOrder remains authorized compatibility.',
                'Exact source-versioned capability evidence does not grant full current-build, scientific, platform or format approval.'])
    with Path(output).open('x') as stream: json.dump(result,stream,indent=2); stream.write('\n')
    return result,audit


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3])
    parser.add_argument('--qualification',type=Path,required=True)
    parser.add_argument('--baseline',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args(); repo = args.repo.resolve()
    if args.baseline:
        result,audit = build(repo,args.baseline,args.qualification,args.output)
        print(json.dumps({'coverage':result['coverage'],'requirements_still_needed':result['requirements_still_needed']}))
    else:
        _,_,result = qualify(repo,args.qualification,nasa.inventory(repo))
        with args.output.open('x') as stream: json.dump(result,stream,indent=2); stream.write('\n')
        print(json.dumps(result))
