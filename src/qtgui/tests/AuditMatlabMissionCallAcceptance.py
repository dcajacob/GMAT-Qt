"""Admit the exact MATLAB Mission Tree row; retain wider and failed menu scopes."""
import argparse
import copy
import difflib
import hashlib
import json
from pathlib import Path
import re
import sys
import tarfile
import xml.etree.ElementTree as ET
import NasaGuiCoverage as nasa
import BuildNasaCoverageExtension as common
import AuditMatlabFunctionCreation as frozen_tools
sha, load, require = common.sha, common.load, common.require
KIND = 'source-versioned-MATLAB-Mission-Tree-command-creation-and-configuration'
KEYS = ['All Others!E180']
PREFIX = 'QtGui.NasaMatlabMissionCall'
CASES = ['Beginning','Middle','End','After','AfterBranch','If','Else','For','While','Target','Optimize','Nested','ScriptEvent','NestedScriptEvent']
CATALOGUES = ['Target','OptimizeAddObjective','OptimizeCapacity']
CALL = "'MATLAB call Ω' [Copy, Result] = MatBeta(Other, Input);"
INNER = "'MATLAB inner Ω' [Copy, Result] = MatBeta(Other, Input);"
PRODUCTION = ['src/qtgui/MainWindow.cpp','src/qtgui/CommandEditor.cpp','src/qtgui/CommandForm.cpp','src/qtgui/MissionModel.cpp','src/qtgui/ScriptEventDialog.cpp','src/base/command/GmatCommand.cpp','plugins/MatlabInterfacePlugin/src/base/command/CallMatlabFunction.cpp','plugins/MatlabInterfacePlugin/src/base/interface/MatlabInterface.cpp']
PARENTS = {'If':['If'],'Else':['If'],'For':['For'],'While':['While'],'Target':['Target'],'Optimize':['Optimize'],'Nested':['For','If']}


def check(proof, case, directory=None):
    event = case in ['ScriptEvent','NestedScriptEvent']
    require(proof['case'] == case and proof['outcome'] == 'passed' and proof['actual_command_type'] == 'CallMatlabFunction'
            and proof['new_requirement_credit'] == 0 and proof['exact_source_undo_redo'] is True and proof['idempotent_apply'] is True,
            'Wrong native MATLAB capability/credit/history')
    if event:
        depth = 2 if case == 'NestedScriptEvent' else 1
        require(proof['checkpoint'] == 'matlab-script-event-call' and proof['script_event_depth'] == depth
                and proof['creation_cancelled'] is True and proof['body_dialog_cancelled'] is True, 'Missing actual Script Event editor protocol')
    else:
        operation = 'Append command…' if case == 'End' else 'Insert after…' if case in ['After','AfterBranch'] else 'Insert before…' if case in ['Beginning','Middle','Else'] else 'Append inside branch…'
        require(proof['checkpoint'] == 'matlab-mission-call' and proof['template'] == 'Call MATLAB function'
                and proof['insertion_menu'] == operation and proof['parent_types'] == PARENTS.get(case, [])
                and proof['picker_cancels'] == 3 and proof['insertion_cancelled'] is True, 'Missing exact Mission Tree placement/configuration')
        if case == 'Middle': require(proof.get('existing_editor_cancelled') is True, 'Existing named-call editor/Cancel not proved')
    calls = 2 if case in ['For','While','Target','Optimize','Nested'] else 1
    keep = 43 if calls == 2 else 42
    values = [7,-3,(-3)-4*7,2*7+(-3),keep]
    reported = [7,-3,0,0,42] if case in ['End','After'] else values[:-1]+[41 if case == 'Beginning' else keep]
    report = ','.join(str(x) for x in reported)+'\n'
    trace = '-3,7\n'*calls
    require([p['phase'] for p in proof['phases']] == ['initial','no-build-repeat','reopened'], 'Missing actual repeated/reopened missions')
    for p in proof['phases']:
        require(p['native_values'] == values and p['report'] == report and p['actual_calls'] == trace
                and p['matlab_calls'] == calls and p['viewer_matches'] is True, 'Independent native/report/real MATLAB call oracle differs')
        if directory:
            require((directory/(p['phase']+'-report.csv')).read_text() == report
                    and (directory/(p['phase']+'-calls.csv')).read_text() == trace, 'Raw MATLAB report/call log differs')
            observed = directory/(p['phase']+'-native-observed.json')
            if observed.exists(): require(load(observed) == dict(zip(['Input','Other','Copy','Result','Keep'], values)), 'Raw native object values differ')
    if directory:
        source = (directory/'final.script').read_text()
        require(source.count(INNER if event else CALL) == 1 and 'Input = 7;' in source and 'Other = -3;' in source
                and 'MatBeta.FunctionPath =' in source and '% Preserve ' in source, 'Exact authored named call/caller controls absent')
        body = (directory/'MatBeta.m').read_text()
        require('a = x-4*y;' in body and 'b = 2*y+x;' in body and "fprintf(f, '%.17g,%.17g\\n', x, y);" in body,
                'Actual MATLAB body/call witness differs')
        if event:
            nodes = load(directory/'expanded-native-nodes.json'); nesting = 0; found = 0
            for n in nodes:
                if n['type'] == 'BeginScript': nesting += 1
                if n['type'] == 'CallMatlabFunction':
                    require(nesting == depth and n['editable'] is True and n['statement'] == INNER, 'Native inner MATLAB source/name/grouping differs'); found += 1
                if n['type'] == 'EndScript': nesting -= 1
                require(nesting >= 0, 'Invalid native Script Event boundaries')
            require(nesting == 0 and found == 1, 'Missing native grouped MATLAB call')
        else:
            snapshot = load(directory/'native-snapshot.json')
            nodes = [n for n in snapshot['nodes'] if n['type'] == 'CallMatlabFunction']
            require(snapshot['authored'] == source and len(nodes) == 1 and nodes[0]['editable'] is True
                    and nodes[0]['statement'] == ('% Preserve mission tail Ω\n'+CALL if case == 'End' else CALL), 'Exact source-mapped MATLAB command absent')
            require(load(directory/'native-parent-types.json') == PARENTS.get(case, []), 'Raw native parent hierarchy differs')
    return dict(missions=3, matlab_calls=3*calls)


def window_policy(receipt):
    display = receipt.get('visible_display', {})
    require(receipt['mode'] == 'visible' and receipt['max_screenshots'] == 0 and display.get('resizable')
            and display.get('start_minimized') and 'Iconic' in display.get('initial_host_window_state','')
            and '_NET_WM_STATE_FULLSCREEN' not in display.get('initial_host_window_state',''), 'Actual visible window policy missing')


def qualify(repo, path, inventory):
    q = load(path)
    require(q['contract_kind'] == KIND and q['qualified_keys'] == KEYS, 'Changed original MATLAB Mission Tree scope')
    gates = ['scientific_approval','technical_core_approval','full_current_build_qualified','full_NASA_qualified','MATLAB_bundle_qualified','full_menu_catalogue_qualified']
    require(all(q.get(g) is False for g in gates), 'Broader qualification gate waived')
    for p,h in q['artifact_sha256'].items(): require(not Path(p).is_symlink() and sha(p) == h, 'Changed regular artifact '+p)
    def bound(p): require(str(p) in q['artifact_sha256'], 'Unbound artifact '+str(p)); return Path(p)
    with tarfile.open(bound(q['reader_source_archive'])) as tar:
        require(set(tar.getnames()) == set(q['reader_source_sha256']), 'Reader source partition differs')
        for p,h in q['reader_source_sha256'].items(): require(hashlib.sha256(tar.extractfile(p).read()).hexdigest() == h, 'Frozen reader differs')
    for module in [sys.modules[__name__], nasa, common, frozen_tools]:
        p = 'src/qtgui/tests/'+Path(module.__file__).name
        require(sha(repo/p) == q['reader_source_sha256'][p], 'Executing reader source changed')
    known = {r['key']:r for r in inventory['requirements']}
    row = lambda k: dict(key=k,declared_id=known[k]['declared_id'],fingerprint=known[k]['fingerprint'],source_text=known[k]['text'])
    scope = load(bound(q['scope_review'])); before = load(bound(q['prior_coverage']))
    require(q['authority_sha256'] == inventory['sources'] == scope['authority_sha256']
            and q['original_requirements'] == scope['candidate_rows'] == [row(KEYS[0])]
            and scope['inherited_parents'] == [row('All Others!E160')] and scope['verified_sibling_examples'] == [row('All Others!E179')], 'Original source text/fingerprint/inheritance differs')
    require(scope['canonical_before_sha256'] == sha(q['prior_coverage']) and before['coverage']['requirements']['verified'] == 1021
            and before['coverage']['historical_cases']['verified'] == 1816 and before['requirements_still_needed'] == 80, 'Prior exact union changed')
    remaining = {r['key'] for r in before['remaining_original_rows']['requirements']}
    require(set(KEYS+['All Others!E160']) <= remaining and 'All Others!E179' not in remaining, 'Parent/sibling row decomposition changed')
    original = {r['key']:r for r in load(bound(q['original_closure_review']))['reviewed_open_rows']}
    require(scope['original_closure_records'] == {KEYS[0]:original[KEYS[0]]}
            and scope['retained_bundle_gate'] == original[KEYS[0]]['original_row']['closure_gate']
            and scope['MATLAB_bundle_qualified'] is False and scope['full_menu_catalogue_qualified'] is False, 'Original gate record overwritten')
    prior_holds = load(bound(q['prior_hold_overlay'])); contracts = load(bound(q['contract_review']))
    require(len(set(prior_holds['original_hold_keys_retained'])) == 59 and prior_holds['resolved_original_hold_keys'] == ['All Others!E152','FRR!E478']
            and set(prior_holds['unresolved_original_hold_keys']) <= remaining and len(prior_holds['unresolved_original_hold_keys']) == 57,
            'Original hold identities/history changed')
    require({r['key'] for r in scope['excluded_unresolved_MATLAB_rows']} == set(contracts['MATLAB_hold_keys'])-{'All Others!E152','FRR!E478',KEYS[0]}
            and len(scope['excluded_unresolved_MATLAB_rows']) == 34, 'Unrelated MATLAB siblings imported or omitted')
    stage = Path(q['stage']); cache = {}; archives = {}; summaries = {}; proofs = {}; counts = dict(missions=0,matlab_calls=0)
    qa = load(bound(stage/'source-qa-v2.json')); require(set(qa['production_sha256_common_to_selected_source_versions']) == set(PRODUCTION), 'Relevant production partition changed')
    def archive(v):
        if v not in archives:
            archives[v] = frozen_tools.archive(bound(stage/f'executed-sources-v{v}.tar.gz'), load(bound(stage/f'executed-source-sha256-v{v}.json')))
        return archives[v]
    def run(name, version, mode):
        if name in cache: return cache[name]
        receipt = load(bound(stage/name/'results.json')); mapped = load(bound(stage/(name+'-loaded-images.json')))
        require(receipt['mode'] == mode and receipt.get('finished_utc') and receipt['end_reason'] == 'completed-selection'
                and mapped['inputs_unchanged'] is True and mapped['runtime_binary_copies'] == 0, 'Nonterminal/mutated native selection')
        active = {r['name']:r for r in receipt['results'] if r['status'] != 'not-run'}
        require((mapped['returncode'] == 0) == all(r['status'] == 'passed' for r in active.values()), 'Batch outcome hidden/reclassified')
        frozen = archive(version); require(mapped['source_sha256'] == {p:hashlib.sha256(b).hexdigest() for p,b in frozen.items()}, 'Executed source archive differs')
        require(all(mapped['source_sha256'].get(p) == h for p,h in receipt['source_hashes'].items()), 'Runtime source omitted from custody')
        require(all(mapped['source_sha256'][p] == h for p,h in qa['production_sha256_common_to_selected_source_versions'].items()), 'Relevant production code differs across selected passes')
        union = {}
        for process in mapped['processes']:
            for snap in process['snapshots']:
                require(hashlib.sha256(snap['maps'].encode()).hexdigest() == snap['maps_sha256'], 'Raw native map hash differs')
                paths = set()
                for line in snap['maps'].splitlines():
                    parts = line.split(None,5)
                    if len(parts) == 6 and (parts[5].startswith(str(repo)+'/') or parts[5].startswith('/usr/local/MATLAB/R2026b/')): paths.add(parts[5])
                require(paths == set(snap['project_images']), 'Raw mapped project/SDK partition differs'); union.update(snap['project_images'])
        require(union == mapped['loaded_project_images'], 'Loaded-image union differs')
        if mode == 'visible': window_policy(receipt)
        cache[name] = receipt,mapped,active
        return cache[name]
    def context(name, version, mode, testname, case, role, passing):
        receipt,mapped,active = run(name,version,mode); result = active[testname]; directory = Path(result['log']).parent
        require(result['status'] == ('passed' if passing else 'failed') and ((result['returncode'] == 0) if passing else (result['returncode'] != 0)), 'Native result differs from retained status')
        for p in directory.rglob('*'):
            if p.is_file() and not p.is_symlink(): bound(p)
        tests = list(ET.parse(bound(directory/'ctest.xml')).getroot().iter('testcase'))
        require(len(tests) == 1 and tests[0].get('name') == testname and tests[0].find('skipped') is None
                and (tests[0].find('failure') is None) == passing, 'Native terminal JUnit differs')
        processes = [p for p in mapped['processes'] if p['role'] == role and p['command'][-1] == case]
        require(len(processes) == 1, 'Actual selected native process absent/duplicated'); process = processes[0]
        executable = process['executable']
        require(executable in result['registered_command'] and mapped['loaded_project_images'][executable] == receipt['binary_hashes'][executable]
                and any(Path(p).name.startswith('libMatlabInterface.so') for p in mapped['loaded_project_images'])
                and any(p.startswith('/usr/local/MATLAB/R2026b/') and Path(p).name == 'libeng.so' for p in mapped['loaded_project_images']), 'Actual native/plugin/SDK mapping absent')
        trace = [json.loads(line) for line in bound(directory/('trace-'+str(process['pid'])+'.jsonl')).read_text().splitlines()]
        require(trace[0]['event'] == 'start' and trace[-1]['event'] == 'summary' and trace[-1]['screenshots'] == 0, 'Terminal native observer absent')
        exposures = sum(r['event'] == 'native_exposure' and r.get('exposed',False) for r in trace)
        require(exposures == result['trace']['qt_window_exposure_events'], 'Raw Qt window exposure census differs')
        require(mode != 'visible' or exposures == result['trace']['native_exposure_events'] > 0, 'Visible native exposure census differs')
        return result,directory,dict(run=name,source_version=version,pid=process['pid'],native_exposures=result['trace']['native_exposure_events'] if mode == 'visible' else 0,raw_Qt_window_exposures=exposures,batch_returncode=mapped['returncode'])
    require(set(q['cases']) == {'headless','visible'}, 'Missing mode')
    for mode,selections in q['cases'].items():
        require(set(selections) == set(CASES), 'Missing whole placement/editor matrix')
        for case,binding in selections.items():
            event = case in ['ScriptEvent','NestedScriptEvent']; role = 'script-event-call-test' if event else 'mission-call-test'
            result,directory,summary = context(binding['run'],binding['source_version'],mode,PREFIX+case,case,role,True)
            proof = load(bound(directory/'proof.json')); emitted = re.findall(r'NASA_CHECK (\{[^\n]+\})',Path(result['log']).read_text())
            require(len(emitted) == 1 and json.loads(emitted[0]) == proof, 'Actual stdout differs from proof')
            c = check(proof,case,directory)
            for k in counts: counts[k] += c[k]
            if binding['source_version'] >= 11:
                require([load(bound(directory/f'fresh-template-{i}.json')) for i in [1,2]] == [dict(text='',modified=False)]*2, 'New insertion drafts were not empty/unmodified')
            summaries[mode+'-'+case] = summary; proofs[case] = proof
    catalogue_summaries = {}
    for mode,binding in q['auxiliary_runs'].items():
        for case in CATALOGUES:
            testname = 'QtGui.NasaMatlabMissionCatalogue'+case
            result,directory,summary = context(binding['run'],binding['source_version'],mode,testname,case,'mission-catalogue-test',False)
            data = load(bound(directory/'catalogue-result.json')); queries = load(bound(directory/'catalogue.json'))
            require(data['query_count'] == len(queries) == 10 and data['unexpected'] == [] and len(data['missing']) == 10
                    and all(x.endswith(':PrepareMissionSequence') for x in data['missing']), 'Different or hidden full-catalogue failure')
            require('CallMatlabFunction' in data['native_viewable'] and 'Call MATLAB function' in data['expected_base'], 'Native MATLAB catalogue type absent')
            for query in queries:
                require(query['actual'].count('Call MATLAB function') == 1 and query['expected'].count('Call MATLAB function') == 1
                        and query['missing'] == ['PrepareMissionSequence'] and query['unexpected'] == [], 'New MATLAB menu entry incomplete')
            append = next(x for x in queries if x['tag'] == 'branch-append')
            require(append['defaults']['Call MATLAB function'] == '[OutputVariable] = MatlabFunctionName(InputVariable);', 'MATLAB catalogue preview differs')
            identity = load(bound(directory/'plugin-identity.json')); terminal = load(bound(directory/'launcher-terminal.json'))
            require(identity['private_startup_is_symlink'] is False and terminal['inputs_unchanged'] is True and terminal['returncode'] == 1
                    and sha(bound(identity['original_startup'])) == identity['original_startup_sha256'] == terminal['original_startup_sha256']
                    and sha(bound(directory/'selected-startup.txt')) == identity['selected_startup_sha256'], 'Catalogue launcher changed its original private input')
            catalogue_summaries[mode+'-'+case] = summary
    # Preserve the native counterexamples and every original assertion contract.
    old = archive(2); after_template = archive(3); old_label = archive(4); after_label = archive(5)
    driver = 'src/qtgui/tests/NasaMatlabMissionCallTests.cpp'
    require(old[driver] == after_template[driver] and old_label[driver] == after_label[driver], 'Assertions changed across production repairs')
    for a,b,p,patch in [(2,3,'src/qtgui/MainWindow.cpp','MainWindow-MATLAB-template-v1.patch'),(4,5,'src/base/command/GmatCommand.cpp','GmatCommand-MATLAB-label-v1.patch')]:
        expected = ''.join(difflib.unified_diff(archive(a)[p].decode().splitlines(True),archive(b)[p].decode().splitlines(True),fromfile='before/'+p,tofile='after/'+p))
        require(bound(stage/patch).read_text() == expected, 'Exact production increment differs')
    before_snapshot = load(bound(stage/'named-before-v1'/('QtGui.NasaMatlabMissionCallMiddle')/'native-snapshot.json'))
    node = next(n for n in before_snapshot['nodes'] if n['type'] == 'CallMatlabFunction')
    require(node['editable'] is False and 'MATLAB call Ω' not in node['statement'] and "'MATLAB call Ω'" in before_snapshot['authored'], 'Original lost-label failure erased')
    interruption = load(bound(stage/'visible-interruption-v1.json')); stopped = load(bound(stage/'remaining-visible-v1/results.json'))
    require(interruption['status'] == 'graceful-interruption-requested' and interruption['SDK_executable_in_test_group'] is False
            and stopped['end_reason'] == 'interrupted' and sum(r['status'] == 'interrupted' for r in stopped['results']) == 1, 'Interrupted modal selection erased/reclassified')
    # Recompute assertion preservation from frozen code, including the one
    # exact comment-span oracle correction; report counters alone are insufficient.
    original_assertions = frozen_tools.assertions(archive(1)[driver].decode())
    corrected = [a.replace('node.statement.trimmed()==call,', 'node.statement.trimmed()==(mode=="End"?"% Preserve mission tail Ω\\n"+call:call),') for a in original_assertions]
    current_assertions = frozen_tools.assertions(archive(12)[driver].decode())
    at = 0
    for a in current_assertions:
        if at < len(corrected) and a == corrected[at]: at += 1
    require(at == len(original_assertions) == 49 and sum(a != b for a,b in zip(original_assertions,corrected)) == 1, 'Original native constraints changed beyond exact comment-span correction')
    catalogue_path = 'src/qtgui/tests/NasaSolverMenuCatalogueTests.cpp'
    old_assertions = frozen_tools.assertions(archive(2)[catalogue_path].decode()); current = frozen_tools.assertions(archive(12)[catalogue_path].decode()); at = 0
    for a in current:
        if at < len(old_assertions) and a == old_assertions[at]: at += 1
    require(at == len(old_assertions) == 55, 'Pre-existing full-catalogue assertions removed or weakened')
    prior_core = load(bound(q['prior_core_inventory'])); current_core = load(bound(q['current_core_inventory']))
    a = {r['path']:r['working_sha256'] for r in prior_core['files']}; b = {r['path']:r['working_sha256'] for r in current_core['files']}
    require(len(a) == len(b) == 248 and set(a) == set(b) and [p for p in a if a[p] != b[p]] == ['src/base/command/GmatCommand.cpp']
            and b['src/base/command/GmatCommand.cpp'] == hashlib.sha256(after_label['src/base/command/GmatCommand.cpp']).hexdigest(), 'Core increment or prior core identities changed')
    qa2 = load(bound(stage/'source-qa-v2.json')); require(qa2['retained_or_precisely_corrected_original_native_require_expressions'] == 49
            and qa2['original_catalogue_require_expressions_preserved'] == 55 and qa2['relaxed_numerical_bounds'] is False, 'Original assertion scope changed')
    negative = []
    mutations = [('wrong-result','Middle',lambda p:p['phases'][0]['native_values'].__setitem__(2,0)),('wrong-call-count','Target',lambda p:p['phases'][0].update(actual_calls='-3,7\n',matlab_calls=1)),('missing-reopen','For',lambda p:p['phases'].pop()),('wrong-parent','Nested',lambda p:p.update(parent_types=['If'])),('wrong-operation','After',lambda p:p.update(insertion_menu='Insert before…')),('lost-cancel','Middle',lambda p:p.update(picker_cancels=2)),('lost-existing-editor','Middle',lambda p:p.update(existing_editor_cancelled=False)),('wrong-event-depth','NestedScriptEvent',lambda p:p.update(script_event_depth=1)),('wrong-backend','If',lambda p:p.update(actual_command_type='CallGmatFunction')),('unearned-credit','End',lambda p:p.update(new_requirement_credit=1))]
    for label,case,mutate in mutations:
        corrupt = copy.deepcopy(proofs[case]); mutate(corrupt)
        try: check(corrupt,case)
        except (ValueError,AssertionError): negative.append(label)
        else: raise ValueError('Accepted corrupt MATLAB Mission Tree proof '+label)
    require(counts == dict(missions=84,matlab_calls=114), 'Mission/call census differs')
    q['audit_summary'] = dict(qualified_keys=KEYS,GUI_case_contexts=28,positive_missions=84,actual_MATLAB_calls=114,
        visible_native_exposures=sum(x['native_exposures'] for x in summaries.values()),case_source_bindings=summaries,
        auxiliary_catalogue_contexts=catalogue_summaries,auxiliary_queries=60,full_menu_catalogue_status='failed;unqualified;PrepareMissionSequence omitted',
        negative_controls=negative,original_hold_identities_retained=59,resolved_original_hold_keys=['All Others!E152','FRR!E478',KEYS[0]],
        unresolved_original_hold_count=56,unresolved_original_MATLAB_count=34,retained_bundle_gate_status='open',native_interruptions=1,
        pending_core_paths=248,unchanged_prior_core_paths=247,scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)
    return q,KEYS


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--qualification',type=Path,required=True); parser.add_argument('--output',type=Path,required=True); args = parser.parse_args()
    repo = Path(__file__).resolve().parents[3]; q,keys = qualify(repo,args.qualification,nasa.inventory(repo)); require(not args.output.exists(),'Audit output exists')
    args.output.write_text(json.dumps(dict(status='passed',qualified_keys=keys,summary=q['audit_summary'],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,MATLAB_bundle_qualified=False,full_menu_catalogue_qualified=False),indent=2)+'\n')
    print(json.dumps({k:q['audit_summary'][k] for k in ['qualified_keys','GUI_case_contexts','positive_missions','actual_MATLAB_calls','negative_controls']}))
