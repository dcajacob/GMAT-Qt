"""Audit bounded function-owned frame/propagator scope without promoting strict time probes."""
import argparse, copy, hashlib, json, math, re, tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load, require, sha

PREFIX = 'QtGui.NasaFunctionOwned'
POSITIVE = {'CoordinateSystem','PropSetupStoppedTime'}
REG = {'QtGui.NasaFunctionFrameCoordinate':'NasaFunctionFrameTests.cpp',
       'QtGui.NativeNasaFunctionPropagatorGlobalExisting':'NasaFunctionPropagatorGlobalTests.cpp'}


def circle(radius, seconds):
    n = math.sqrt(400000/radius**3); a = n*seconds; v = n*radius
    return [radius*math.cos(a),radius*math.sin(a),0,-v*math.sin(a),v*math.cos(a),0]


def native(value, prop, local, executed=False):
    require(value['type'] == ('PropSetup' if prop else 'CoordinateSystem') and value['name'] == ('FunctionOwned' if local else 'Shared') and value['automatic'] and value['global'] and value['local'] == executed, 'Owned native type/name/flags differ')
    for key in ['pointer','owned_component_pointer']:
        require(re.fullmatch('[0-9a-f]+',value[key]) and value[key] != '0', 'Invalid native owned pointer')
    if prop:
        require(value['method'] == ('PrinceDormand78' if local else 'RungeKutta89') and value['accuracy'] == (1e-12 if local else 1e-9) and value['initial_step'] == (1 if local else 3) and value['min_step'] == 0 and value['max_step'] == 5 and value['fm'] == ('FunctionFM' if local else 'MainFM') and value['central'] == 'Earth' and value['point_masses'] == ['Earth'] and value['primary_bodies'] == [] and value['drag'] == 'None' and value['srp'] == 'Off', 'Owned method/force literals differ')
    else:
        require(value['origin'] == 'Earth' and value['axes'] == ('MJ2000Ec' if local else 'MJ2000Eq'), 'Owned frame literals differ')


def contract(proof):
    key = proof['case']; prop = key == 'PropSetupStoppedTime'
    require(key in POSITIVE and proof['checkpoint'] == 'function-declared-owned-resource-scope' and proof['outcome'] == 'passed' and proof['type'] == ('Propagator' if prop else 'CoordinateSystem') and proof['native_type'] == ('PropSetup' if prop else 'CoordinateSystem') and proof['stop_time_policy'] == ('measured-endpoint-and-default-one-microsecond-duration-bound' if prop else 'fixed-input-endpoint') and proof['missions'] == 3 and proof['function_calls'] == 12 and proof['reader_calls'] == 9 and proof['file_decisions'] == [dict(saved=False),dict(saved=True)], 'Owned scope identity/count/policy differs')
    require([v['phase'] for v in proof['phases']] == [0,1,2] and [v['explicit_rebuild'] for v in proof['phases']] == [True,True,False], 'Owned no-Build repeat missing')
    for phase in proof['phases']:
        require(phase['caller_anchor_state'] == [7000,100,200,-.1,7.5,.8] and phase['caller_anchor_mass'] == 987, 'Owned caller isolation differs')
        require(phase['main_writes'] == ['Create '+proof['type']+' Shared;']*4 and phase['function_writes'] == ['Create '+proof['type']+' FunctionOwned;']*3, 'Owned actual Script constructor Write differs')
        native(phase['main_resource'],prop,False)
        require([v['path'] for v in phase['identities']] == ['direct-1','direct-2','nested'], 'Owned direct/nested reader context missing')
        for row in phase['identities']:
            require(row['shared_global_pointer'] == row['runtime_shared_pointer'] == phase['main_resource']['pointer'] != row['configured_shared_pointer'], 'Owned implicit-global identity differs')
            native(row['declaration'],prop,True)
            require(row['declaration']['pointer'] not in {row['runtime_shared_pointer'],row['configured_shared_pointer']} and row['propagations'] == (1 if prop else 0), 'Function declaration aliases main or route differs')
            commands = row['fcs_commands']
            require(sum(v['type'] == 'Propagate' for v in commands) == row['propagations'] and any(v['type'] == 'Write' for v in commands) and not any(v['type'] == 'Create' for v in commands), 'Owned declaration-mode FCS differs')
            if prop:
                executed = row['executed_prop_setup']; native(executed,True,True,True)
                t = executed['integrator_elapsed_seconds']
                require(math.isfinite(t) and abs(t-30) <= 1e-6 and executed['force_elapsed_seconds'] == t == phase['oracle_function_interval_seconds'] and executed['stop_tolerance'] == 1e-7, 'Executed endpoint/duration differs')
            else: require(row['executed_prop_setup'] == {}, 'Frame falsely reports propagation')
        if prop:
            require(phase['requested_function_interval_seconds'] == 30 and phase['stopping_time_absolute_bound_seconds'] == 1e-6, 'Default duration bound changed')
            main, local = circle(9000,15), circle(7000,phase['oracle_function_interval_seconds'])
            bounds = [1e-7]*3+[1e-10]*3
        else:
            main, local = [7000,100,200,-.1,7.5,.8], [1000,-200,300,1.25,-.5,.75]
            e = math.radians(84381.448/3600)
            for offset in [0,3]:
                y,z = local[offset+1:offset+3]
                local[offset+1:offset+3] = [math.cos(e)*y-math.sin(e)*z,math.sin(e)*y+math.cos(e)*z]
            bounds = [1e-9]*6
        expected, limits = list(main), list(bounds)
        for mark in [42,43,44]: expected += [mark]+local; limits += [0]+bounds
        expected += [103,104]; limits += [0,0]
        require(phase['absolute_bounds'] == limits and len(phase['raw_values']) == len(phase['expected_values']) == len(expected) == 29, 'Owned oracle width/bounds differ')
        require(all(math.isfinite(x) and abs(x-y) <= b for x,y,b in zip(phase['raw_values'],expected,limits)), 'Owned independent raw state/marker differs')
        require(all(abs(x-y) <= 1e-12 for x,y in zip(phase['expected_values'],expected)), 'Owned stored oracle differs from rederivation')


def audit(repo, stage):
    bindings = load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for n,d in bindings.items(): require(Path(n).is_file() and not Path(n).is_symlink() and sha(n) == d, 'Changed regular input '+n)
    def bound(p):
        p = Path(p); require(str(p) in bindings,'Unbound artifact '+str(p)); return p
    def capsule(name):
        with tarfile.open(bound(stage/name)) as t:
            require(all(v.isfile() for v in t.getmembers()), 'Nonregular source member')
            return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    visible = capsule('visible-executed-sources-v1.tar.gz'); before = load(bound(stage/'visible-inputs-before-v1.json'))
    require(before == load(bound(stage/'visible-inputs-after-v1.json')) and before['sources'] == {n:hashlib.sha256(v).hexdigest() for n,v in visible.items()}, 'Visible source/runtime inputs changed')
    old = load(bound(repo/'build/nasa-coverage/coverage95-20261103-function-body-scope/visible-inputs-before-v1.json'))['sources']
    for n in REG.values(): require(hashlib.sha256(visible['src/qtgui/tests/'+n]).hexdigest() == old['src/qtgui/tests/'+n], 'Original regression assertion/source changed')
    launch = load(bound(stage/'visible-launch-v1.json')); maps = load(bound(stage/'loaded-images-v1.json'))['processes']
    require(launch['returncode'] == 0 and launch['inputs_unchanged'] and launch['mapped_processes'] == len(maps) == 4, 'Visible process mapping incomplete')
    versions = [('headless-v1',{PREFIX+'CoordinateSystem',PREFIX+'PropSetup'}|set(REG),{PREFIX+'CoordinateSystem'}|set(REG)),('headless-v2',{PREFIX+'PropSetup'},set()),('headless-v3',{PREFIX+'PropSetup'},set()),('headless-v4',{PREFIX+'PropSetup'},set()),('headless-v5',{PREFIX+k for k in POSITIVE},{PREFIX+k for k in POSITIVE}),('visible-v1',{PREFIX+k for k in POSITIVE}|set(REG),{PREFIX+k for k in POSITIVE}|set(REG))]
    selected, failures, superseded, exposures = [], [], [], 0
    for version,expected,passed in versions:
        invocation = 'function-owned-scope-'+version; receipt = load(bound(repo/'build/gui-acceptance'/invocation/'results.json'))
        frozen = visible if version == 'visible-v1' else capsule(version.replace('headless-','headless-executed-sources-')+'.tar.gz')
        require(receipt.get('finished_utc') and receipt['end_reason'] == 'completed-selection' and all(n in frozen and hashlib.sha256(frozen[n]).hexdigest() == d for n,d in receipt['source_hashes'].items()), 'Executed source/terminal state differs')
        active = {v['name']:v for v in receipt['results'] if v['status'] != 'not-run'}
        require(set(active) == expected,'Native selection changed')
        for name,row in active.items():
            good = name in passed
            require(row['status'] == ('passed' if good else 'failed') and (row['returncode'] == 0) == good,'Native status differs')
            log = bound(row['log']); case = log.parent; tests = list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'))
            require(len(tests) == 1 and tests[0].get('name') == name and tests[0].find('skipped') is None and (tests[0].find('failure') is None) == good,'Exact JUnit differs')
            item = dict(invocation=invocation,test=name)
            if not good:
                require(('Segmentation fault' if version == 'headless-v3' else 'Owned independent main/function state or marker differs') in log.read_text(),'Retained strict probe failure lost')
                failures.append(item); continue
            native_proofs = [json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text())]
            require(len(native_proofs) == 1 and native_proofs[0]['outcome'] == 'passed','Native emitted proof differs')
            proof = native_proofs[0]
            if (case/'proof.json').is_file(): require(load(bound(case/'proof.json')) == proof,'Stored proof differs')
            if version == 'headless-v1' and name == PREFIX+'CoordinateSystem': superseded.append(item); continue
            if name not in REG:
                require((case/'proof.json').is_file(),'Stored owned proof absent'); contract(proof)
                caller = bound(case/'caller.script').read_text(); original = bound(case/'initial.gmf').read_text(); edited = bound(case/'edited.gmf').read_text(); nested = bound(case/'nested.gmf').read_text()
                require(original.replace('PrivateOnly = 103;','PrivateOnly = 41;') == edited and 'Mark = PrivateOnly + Calls;' in edited and '[Mark, State] = ReadShared();' in nested and not re.search(r'Global[^;]*(?:Shared|FunctionOwned)',caller+edited+nested),'Exact edit/automatic syntax differs')
                require(not re.search(r'\bCreate\s+\w+\s+FunctionOwned\s*;',caller) and 'Create '+proof['type']+' FunctionOwned;' in edited,'Function declaration location differs')
                prototypes = load(bound(case/'prototypes-before.json'))
                for phase in proof['phases']:
                    tag = str(phase['phase']); require([float(v) for v in bound(case/(tag+'-values.txt')).read_text().split()] == phase['raw_values'] and load(bound(case/(tag+'-prototypes.json'))) == prototypes and all(load(bound(case/(tag+'-invariants.json'))).values()), 'Raw state/source/prototype invariant differs')
                    messages = bound(case/(tag+'-messages.txt')).read_text()
                    for name2,key2 in [('Shared','main_writes'),('FunctionOwned','function_writes')]:
                        require(re.findall(r'^Create '+proof['type']+' '+name2+r';',messages,re.M) == phase[key2],'Raw constructor Write differs')
            else:
                checkpoint = 'function-frame-Coordinate' if name == 'QtGui.NasaFunctionFrameCoordinate' else 'function-propagator-global'
                require(proof['checkpoint'] == checkpoint,'Original regression proof identity differs')
            if version == 'visible-v1':
                display = receipt['visible_display']; require(display['start_minimized'] and display['resizable'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'] and receipt['max_screenshots'] == 0 and row['trace']['native_exposure_events'] > 0 and not row['trace']['screenshots'],'Visible policy differs')
                exposures += row['trace']['native_exposure_events']; traces = list(case.glob('trace-*.jsonl')); require(len(traces) == 1,'Visible trace missing'); process = maps[traces[0].stem.removeprefix('trace-')]
                exes = [n for n in row['registered_command'] if n in receipt['binary_hashes']]
                require(len(exes) == 1 and process['executable'] == exes[0] and all(before['images'].get(n) == d for n,d in process['images'].items()),'Mapped image identity differs')
                for n,d in before['images'].items():
                    if n == exes[0] or Path(n).name.startswith(('libGmatBase','libGmatUtil')): require(process['images'].get(n) == d,'Actual executable/core image unmapped')
            selected.append(item)
    require(len(selected) == 8 and len(failures) == 4 and len(superseded) == 1,'Owned scope totals differ')
    require('error:' in bound(stage/'build-v1.log').read_text() and bound(stage/'NasaFunctionOwnedScopeTests.compile-v1.cpp').is_file(),'Compile failure/source lost')
    debug = stage/'gdb-v1'; require(load(bound(debug/'inputs-before.json')) == load(bound(debug/'inputs-after.json')) and load(bound(debug/'invocation.json'))['inputs_unchanged'] and 'StopCondition::GetStopDifference()' in bound(debug/'gdb.log').read_text(),'Diagnostic crash backtrace/input identity lost')
    strict = load(bound(repo/'build/gui-acceptance/function-owned-scope-headless-v4'/(PREFIX+'PropSetup')/'0-oracle-diagnostics.json'))
    require(strict['absolute_bounds'] == ([1e-7]*3+[1e-10]*3)+sum(([0]+[1e-7]*3+[1e-10]*3 for _ in range(3)),[])+[0,0] and any(abs(x-y) > b for x,y,b in zip(strict['raw_values'],strict['expected_values'],strict['absolute_bounds'])),'Strict fixed-endpoint failure/bounds lost')
    for row in strict['identities']:
        executed = row['executed_prop_setup']; require(executed['stop_tolerance'] == 1e-12 and executed['integrator_elapsed_seconds'] == executed['force_elapsed_seconds'] and abs(executed['integrator_elapsed_seconds']-30) > 1e-12,'Strict requested/actual endpoint discrepancy lost')
    negatives = []; samples = {k:load(bound(repo/'build/gui-acceptance/function-owned-scope-visible-v1'/(PREFIX+k)/'proof.json')) for k in POSITIVE}
    controls = [('configured-global-substitution','CoordinateSystem',lambda p:p['phases'][0]['identities'][0].__setitem__('shared_global_pointer',p['phases'][0]['identities'][0]['configured_shared_pointer'])),('wrong-owned-frame','CoordinateSystem',lambda p:p['phases'][0]['identities'][0]['declaration'].__setitem__('axes','MJ2000Eq')),('wrong-raw-state','CoordinateSystem',lambda p:p['phases'][0]['raw_values'].__setitem__(8,0)),('wrong-owned-integrator','PropSetupStoppedTime',lambda p:p['phases'][0]['identities'][0]['executed_prop_setup'].__setitem__('method','RungeKutta89')),('duration-outside-default-bound','PropSetupStoppedTime',lambda p:p['phases'][0]['identities'][0]['executed_prop_setup'].__setitem__('integrator_elapsed_seconds',30.000002)),('false-strict-time-promotion','PropSetupStoppedTime',lambda p:p.__setitem__('case','PropSetup')),('omitted-repeat','CoordinateSystem',lambda p:p['phases'].pop())]
    for label,key,corrupt in controls:
        bad = copy.deepcopy(samples[key]); corrupt(bad)
        try: contract(bad)
        except (ValueError,AssertionError): negatives.append(label)
        else: raise ValueError('Accepted corrupt owned proof '+label)
    return dict(status='passed-bounded-function-declared-frame-and-propagator;strict-time-open',qualified_keys=[],selected=selected,retained_GUI_failures=failures,superseded_frame_pass=superseded,retained_compile_failures=1,retained_debug_crash_runs=1,new_scope_GUI_passes=4,affected_original_regression_GUI_passes=4,scope_missions=12,function_calls=48,reader_calls=36,implicit_main_global_identities=36,owned_declaration_checks=36,executed_propagator_checks=18,default_duration_checks=18,raw_scalars=348,state_components=288,marker_scalars=60,caller_anchor_components=72,Script_constructor_lines=84,actual_propagations=24,visible_mapped_processes=4,visible_exposure_events=exposures,negative_controls=negatives,strict_time_probe_qualified=False,production_changed_files=[],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--stage',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args = parser.parse_args()
    result = audit(Path(__file__).resolve().parents[3],args.stage.resolve());args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
