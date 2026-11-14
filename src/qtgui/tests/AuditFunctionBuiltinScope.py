"""Audit main/direct/nested built-ins, exact literals and formal-object binding."""
import argparse, copy, hashlib, json, re, tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load, require, sha

PREFIX = 'QtGui.NasaFunctionBuiltinScope'
REG = 'QtGui.NativeNasaSolverMenuCatalogueCommands'
LITERAL = [7100,20,-30,-.125,7.25,.375]


def contract(proof, spec):
    require(proof['checkpoint'] == 'function-builtin-scope-' + spec['key'] and proof['outcome'] == 'passed' and proof['builtin_type'] == spec['builtin_type'] and proof['missions'] == 3 and proof['function_calls'] == 12 and proof['local_builtin_calls'] == 9 and proof['main_builtin_calls'] == 3 and proof['file_decisions'] == [dict(saved=False,recovery=False),dict(saved=True,recovery=False)], 'Builtin scope identity/count/edit differs')
    require([v['phase'] for v in proof['runs']] == ['initial','reopened','repeat-no-build'], 'Builtin no-Build repeat absent')
    def values(actual, expected):
        require(len(actual) == len(expected) and all(abs(a-b) <= spec['tolerance'] for a,b in zip(actual,expected)), 'Builtin independent literal differs')
    for row in proof['runs']:
        values(row['main'], spec['main_expected'])
        for name in ['first','second','third']:
            values(row[name], spec['expected'])
        require(row['main'] != row['first'] and row['exercise_calls'] == 3 and row['function_calls'] == 4 and row['main_builtin_calls'] == 1 and row['continuation'] == 42 and row['private_name_absent'] and row['configured_prototypes_unchanged'], 'Main/private/caller scope differs')
        require(set(row['caller_runtime']) == set(spec['main_runtime_expected']), 'Runtime caller object omitted')
        for name, expected in spec['main_runtime_expected'].items():
            actual = dict(row['caller_runtime'][name])
            if 'values' in expected:
                values(actual.pop('values'), expected['values'])
            wanted = {k:v for k,v in expected.items() if k != 'values'}
            if wanted.get('type') == 'Variable':
                require(abs(actual.pop('value')-wanted['value']) <= spec['tolerance'], 'Runtime caller scalar changed')
                wanted.pop('value')
            require(actual == wanted, 'Runtime caller metadata/string changed')
        require([v['function'] for v in row['identities']] == ['Exercise','Exercise','NestedExercise'], 'Direct/nested identities omitted')
        for v in row['identities']:
            require(v['global_store_object_pointer'] == v['runtime_object_pointer'] and re.fullmatch('[0-9a-f]+',v['runtime_object_pointer']) and v['runtime_object_pointer'] != '0', 'Shared counter identity differs')


def audit(repo, stage):
    bindings = load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for n,d in bindings.items():
        require(Path(n).is_file() and not Path(n).is_symlink() and sha(n) == d, 'Changed regular binding ' + n)
    def bound(p):
        p = Path(p)
        require(str(p) in bindings, 'Unbound artifact ' + str(p))
        return p
    def capsule(name):
        with tarfile.open(bound(stage/name)) as t:
            require(all(v.isfile() for v in t.getmembers()), 'Nonregular source member')
            return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    visible = capsule('visible-executed-sources-v1.tar.gz')
    before = load(bound(stage/'visible-inputs-before-v1.json'))
    require(before == load(bound(stage/'visible-inputs-after-v1.json')) and {n:hashlib.sha256(b).hexdigest() for n,b in visible.items()} == before['sources'], 'Visible inputs changed')
    specs = {v['key']:v for v in json.loads(visible['build/linux-gui/src/qtgui/NasaFunctionBuiltinScopeCases.json'])['cases']}
    require(set(specs) == {'Angle','GetLastState','GetEphemStates'} and [specs[k]['columns'] for k in ['Angle','GetLastState','GetEphemStates']] == [1,6,14] and all(v['tolerance'] == 1e-9 for v in specs.values()), 'Literal matrix narrowed')
    baseline = load(bound(repo/'build/nasa-coverage/coverage95-20261103-function-math-resources/visible-inputs-before-v1.json'))['sources']
    require(hashlib.sha256(visible['src/qtgui/tests/NasaFunctionBuiltinTests.cpp']).hexdigest() == baseline['src/qtgui/tests/NasaFunctionBuiltinTests.cpp'] and hashlib.sha256(visible['src/qtgui/tests/NasaSolverMenuCatalogueTests.cpp']).hexdigest() == baseline['src/qtgui/tests/NasaSolverMenuCatalogueTests.cpp'], 'Original assertions changed')
    selections = {1:({PREFIX+k for k in specs}, {PREFIX+'Angle'}),2:({PREFIX+'GetEphemStates'},set()),3:({PREFIX+'GetEphemStates'},{PREFIX+'GetEphemStates'}),5:({PREFIX+'GetLastState',REG},{REG}),6:({PREFIX+'GetLastState',REG},{PREFIX+'GetLastState',REG}),7:({PREFIX+k for k in specs},{PREFIX+k for k in specs})}
    versions = [(f'function-builtin-scope-headless-v{v}',f'headless-executed-sources-v{v}.tar.gz',names,passed) for v,(names,passed) in selections.items()]
    versions += [('function-builtin-scope-visible-v1',None,{PREFIX+k for k in specs}|{REG},{PREFIX+k for k in specs}|{REG})]
    selected, failures, superseded, exposures = [], [], [], 0
    maps = load(bound(stage/'loaded-images-v1.json'))['processes']
    launch = load(bound(stage/'visible-launch-v1.json'))
    require(launch['returncode'] == 0 and launch['inputs_unchanged'] and launch['mapped_processes'] == len(maps) == 4, 'Visible native maps incomplete')
    for invocation, archive, expected, passed in versions:
        receipt = load(bound(repo/'build/gui-acceptance'/invocation/'results.json'))
        require(receipt.get('finished_utc') and receipt['end_reason'] == 'completed-selection', 'Nonterminal invocation')
        frozen = capsule(archive) if archive else visible
        require(all(n in frozen and hashlib.sha256(frozen[n]).hexdigest() == d for n,d in receipt['source_hashes'].items()), 'Executed source identity differs')
        active = {v['name']:v for v in receipt['results'] if v['status'] != 'not-run'}
        require(set(active) == expected, 'Native selection changed')
        for name,row in active.items():
            good = name in passed
            require(row['status'] == ('passed' if good else 'failed') and (row['returncode'] == 0) == good, 'Native status changed')
            log = bound(row['log']); case = log.parent
            tests = list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'))
            require(len(tests) == 1 and tests[0].get('name') == name and (tests[0].find('failure') is None) == good and tests[0].find('skipped') is None, 'Native JUnit differs')
            item = dict(invocation=invocation,test=name)
            if not good:
                failures.append(item)
                require(('BUILTIN_DIFF GetLastState index=0 actual=0 expected=7100' if name.endswith('GetLastState') else 'Built-in script did not build') in log.read_text(), 'Original failure lost')
                continue
            emitted = [json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text())]
            require(len(emitted) == 1 and load(bound(case/'proof.json')) == emitted[0] and emitted[0]['outcome'] == 'passed', 'Native proof differs')
            chosen = archive is None or invocation.endswith('-v7') or invocation.endswith('-v6') and name == REG
            if not chosen:
                superseded.append(item)
                continue
            if name != REG:
                spec = specs[name.removeprefix(PREFIX)]; proof = emitted[0]; contract(proof,spec)
                prototypes = load(bound(case/'prototypes-before.json'))
                require(set(prototypes) == set(spec['caller_objects']), 'Caller prototypes omitted')
                for phase in proof['runs']:
                    tag = phase['phase']; rows = [[float(v) for v in line.split(',')] for line in bound(case/(tag+'-values.csv')).read_text().strip().splitlines()]
                    require(rows == [phase[k] for k in ['main','first','second','third']] and load(bound(case/(tag+'-caller-runtime.json'))) == phase['caller_runtime'] and load(bound(case/(tag+'-prototypes.json'))) == prototypes, 'Raw/native/caller/prototype evidence differs')
                    if spec['key'] == 'GetEphemStates':
                        text = bound(case/(tag+'-messages.txt')).read_text()
                        for which,literals in [('StartEpoch',['02 Jan 2018 00:00:00.000']+['01 Jan 2018 00:00:00.000']*3),('EndEpoch',['02 Jan 2018 00:02:00.000']+['01 Jan 2018 00:01:00.000']*3)]:
                            require(re.findall(r'^'+which+r" = '([^']+)';",text,re.M) == literals, 'Actual returned epoch strings differ')
            if archive is None:
                display = receipt['visible_display']; require(display['start_minimized'] and display['resizable'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'] and receipt['max_screenshots'] == 0 and row['trace']['native_exposure_events'] > 0 and not row['trace']['screenshots'], 'Visible policy differs')
                exposures += row['trace']['native_exposure_events']; traces = list(case.glob('trace-*.jsonl')); require(len(traces) == 1, 'Visible trace absent')
                process = maps[traces[0].stem.removeprefix('trace-')]
                exes = [p for p in row['registered_command'] if p in receipt['binary_hashes']]
                require(len(exes) == 1 and process['executable'] == exes[0] and all(before['images'].get(p) == d for p,d in process['images'].items()), 'Mapped native image changed')
                for p,d in before['images'].items():
                    if p == exes[0] or Path(p).name.startswith(('libGmatBase','libGmatUtil')):
                        require(process['images'].get(p) == d, 'Actual executable/core image unmapped')
            selected.append(item)
    require(len(selected) == 8 and len(failures) == 4 and len(superseded) == 4, 'Native totals differ')
    for version in range(1,6):
        root = stage/f'binding-support-v{version}'; launch = load(bound(root/'launch.json')); inputs = load(bound(root/'inputs-before.json')); frozen = capsule(f'binding-support-executed-sources-v{version}.tar.gz')
        require(inputs == load(bound(root/'inputs-after.json')) and launch['inputs_unchanged'] and launch['returncode'] == (0 if version in {3,5} else 8) and all(hashlib.sha256(frozen[n]).hexdigest() == d for n,d in inputs['sources'].items()), 'Support source/status differs')
        tests = list(ET.parse(bound(root/'ctest.xml')).getroot().iter('testcase')); require(len(tests) == 1 and tests[0].get('name') == 'QtAcceptance.GetLastStateBinding' and (tests[0].find('failure') is None) == (version in {3,5}), 'Support JUnit differs')
        if version == 1:
            require('VEHICLE_EPHEM_SPK_PATH not in the gmat_startup_file' in bound(root/'test.log').read_text(), 'Support setup failure lost')
            continue
        proof = load(bound(root/'proof.json')); require(proof['controls'] == len(proof['rows']) == 3 and proof['failed_controls'] == {2:2,3:0,4:3,5:0}[version], 'Binding control totals differ')
        for row in proof['rows']:
            require(row['expected'] == LITERAL and row['observed'] == (row['formal_placeholder'] if version == 2 else LITERAL), 'Binding source literal differs')
            if version in {4,5}: require(row['stored_output_identity'] == (version == 5), 'Formal output identity differs')
    negatives = []; sample = load(bound(repo/'build/gui-acceptance/function-builtin-scope-visible-v1'/ (PREFIX+'GetLastState')/'proof.json'))
    for label, corrupt in [('zero-private-state',lambda p:p['runs'][0].__setitem__('first',[0]*6)),('caller-shadow-mutated',lambda p:p['runs'][0]['caller_runtime']['Local']['values'].__setitem__(0,7100)),('nested-output-missing',lambda p:p['runs'][0].__setitem__('third',[0]*6)),('repeat-omitted',lambda p:p['runs'].pop()),('wrong-global-counter',lambda p:p['runs'][0]['identities'][0].__setitem__('global_store_object_pointer','wrong'))]:
        bad = copy.deepcopy(sample); corrupt(bad)
        try: contract(bad,specs['GetLastState'])
        except (ValueError,AssertionError): negatives.append(label)
        else: raise ValueError('Accepted corrupt scope '+label)
    return dict(status='passed-three-builtins-and-GetLastState-formal-binding',qualified_keys=[],selected=selected,retained_GUI_failures=failures,superseded_GUI_passes=superseded,superseded_input_only_support_pass=3,retained_support_failures=[1,2,4],retained_setup_only_support_failures=1,passed_binding_controls=3,new_scope_GUI_passes=6,affected_original_regression_GUI_passes=2,scope_missions=18,function_calls=72,local_builtin_calls=54,main_builtin_calls=18,raw_report_scalars=504,exact_epoch_strings=48,shared_counter_identities=54,runtime_caller_object_checks=66,visible_mapped_processes=4,visible_exposure_events=exposures,negative_controls=negatives,production_changed_files=['src/base/function/GetLastState.cpp'],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--stage',type=Path,required=True); parser.add_argument('--output',type=Path,required=True); args = parser.parse_args()
    result = audit(Path(__file__).resolve().parents[3],args.stage.resolve()); args.output.write_text(json.dumps(result,indent=2)+'\n'); print(json.dumps(result,indent=2))
