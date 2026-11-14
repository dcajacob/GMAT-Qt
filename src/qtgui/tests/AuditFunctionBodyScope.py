"""Audit supported body declarations, existing Sun and actual shared solar models."""
import argparse, copy, hashlib, json, re, tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load, require, sha

PREFIX = 'QtGui.NasaFunctionBody'
POSITIVE = {'Sun','Planet','Moon','Comet','Asteroid'}
REG = {'QtGui.NasaFunctionFrameAutomatic':'NasaFunctionFrameTests.cpp','QtGui.NativeNasaBodyScriptSun':'NasaBodyScriptTests.cpp'}


def contract(proof, spec):
    sun = spec['key'] == 'Sun'
    require(proof['checkpoint'] == 'body-resource-solar-system-scope' and proof['outcome'] == 'passed' and proof['case'] == spec['key'] and proof['type'] == spec['type'] and proof['central'] == spec['central'] and proof['creation_mode'] == spec['creation_mode'] and proof['missions'] == 3 and proof['function_calls'] == 12 and proof['reader_calls'] == 9 and proof['file_decisions'] == [dict(saved=False),dict(saved=True)], 'Body scope identity/count/edit differs')
    require([v['phase'] for v in proof['phases']] == [0,1,2] and [v['explicit_rebuild'] for v in proof['phases']] == [True,True,False], 'Body no-Build repeat omitted')
    for phase in proof['phases']:
        require(phase['raw_values'] == [42,43,44,103,104] and phase['function_created'] == (not sun) and phase['configured_function_body_absent'] == (not sun) and phase['same_name_caller_mass'] == 987 and phase['caller_anchor_state'] == [7000,100,200,-.1,7.5,.8], 'Body/private/caller/configured scope differs')
        for key,name,mu,radius in [('main_body','Sun' if sun else 'MainBody',63,470),('function_body','Sun' if sun else 'FunctionBody',63 if sun else 73,470 if sun else 480)]:
            v = phase[key]
            require(v['name'] == name and v['type'] == spec['type'] and v['mu'] == mu and v['radius'] == radius and v['central'] == spec['central'] and v['user'] == (not sun) and v['global'] and v['automatic'] and not v['local'] and v['naif'] == (10 if sun else 2000001), 'Body literal/type/global metadata differs')
            if not sun: require(v['source'] == 'SPICE', 'User-body source differs')
        require(phase['main_writes'] == ([] if sun else ['Create '+spec['type']+' MainBody;']*4) and phase['function_writes'] == ([] if sun else ['Create '+spec['type']+' FunctionBody;']*3) and phase['main_mu_writes'] == [63]*4 and phase['function_mu_writes'] == ([] if sun else [73]*3), 'Actual body Write differs')
        require([v['path'] for v in phase['identities']] == ['direct-1','direct-2','nested'], 'Direct/nested model context missing')
        for v in phase['identities']:
            require(v['function_created'] == (not sun) and v['declaration_object_pointer'] == v['runtime_solar_body_pointer'] and v['reader_solar_system_pointer'] == v['runtime_solar_system_pointer'] != v['configured_solar_system_pointer'] and v['owned_instances'] == 1, 'Function lookup/model/owned-body identity differs')
            for key in ['declaration_object_pointer','runtime_solar_body_pointer','reader_solar_system_pointer','runtime_solar_system_pointer','configured_solar_system_pointer']:
                require(re.fullmatch('[0-9a-f]+',v[key]) and v[key] != '0', 'Invalid native pointer')
            require(v['fcs_create_commands'] == sum(row['type'] == 'Create' for row in v['fcs_commands']) == 0 and any(row['type'] == 'Write' for row in v['fcs_commands']), 'Recorded declaration-mode FCS differs')


def audit(repo, stage):
    bindings = load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for n,d in bindings.items(): require(Path(n).is_file() and not Path(n).is_symlink() and sha(n) == d, 'Changed regular binding '+n)
    def bound(p):
        p = Path(p); require(str(p) in bindings, 'Unbound artifact '+str(p)); return p
    def capsule(name):
        with tarfile.open(bound(stage/name)) as t:
            require(all(v.isfile() for v in t.getmembers()), 'Nonregular source member')
            return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    visible = capsule('visible-executed-sources-v1.tar.gz')
    before = load(bound(stage/'visible-inputs-before-v1.json'))
    require(before == load(bound(stage/'visible-inputs-after-v1.json')) and before['sources'] == {n:hashlib.sha256(b).hexdigest() for n,b in visible.items()}, 'Visible inputs changed')
    catalog = json.loads(visible['build/linux-gui/src/qtgui/NasaFunctionBodyCases.json']); specs = {v['key']:v for v in catalog['cases']}
    require(set(specs) == POSITIVE|{'Star'} and {v['type'] for v in specs.values()} == {'Star','Planet','Moon','Comet','Asteroid'} and specs['Sun']['creation_mode'] == 'existing-solar-model' and specs['Star']['creation_mode'] == 'custom-create-probe' and {k for k,v in specs.items() if v['creation_mode'] == 'function-side-create'} == POSITIVE-{'Sun'}, 'Body authority/supported route narrowed or custom Star promoted')
    authority = load(bound(stage/'body-creation-authority-v1.json')); originals = {v['key']:v for v in authority['original_rows']}
    require([originals['FRR!E'+str(n)]['text'].split()[-1] for n in range(214,218)] == ['Moon','Asteroid','Comet','Planet'] and not authority['custom_star_creation_qualified'] and not authority['eligibility_exclusion_assigned'] and not authority['qualified_keys'], 'Original four-class authority changed')
    kernel = load(bound(stage/'kernel-identity-before-v1.json'))
    require(kernel['path'] == catalog['kernel'] and kernel['sha256'] == catalog['kernel_sha256'] and sha(kernel['path']) == kernel['sha256'] and not kernel['copied'] and kernel['path'] not in bindings and not any(n.endswith(('.bsp','.bpc','.tpc','.tls')) for n in visible), 'Kernel changed or copied into source bundle')
    old = load(bound(repo/'build/nasa-coverage/coverage95-20261103-function-builtin-scope/visible-inputs-before-v1.json'))['sources']
    for source in REG.values(): require(hashlib.sha256(visible['src/qtgui/tests/'+source]).hexdigest() == old['src/qtgui/tests/'+source], 'Original regression assertions changed')
    versions = [('function-body-scope-headless-v1','headless-executed-sources-v1.tar.gz',{PREFIX+k for k in POSITIVE-{'Sun'}|{'Star'}},set()),('function-body-scope-headless-v2','headless-executed-sources-v2.tar.gz',{PREFIX+k for k in POSITIVE}|set(REG),{PREFIX+'Sun'}|set(REG)),('function-body-scope-headless-v3','headless-executed-sources-v3.tar.gz',{PREFIX+k for k in POSITIVE},{PREFIX+k for k in POSITIVE}),('function-body-scope-visible-v1',None,{PREFIX+k for k in POSITIVE}|set(REG),{PREFIX+k for k in POSITIVE}|set(REG))]
    selected, failures, superseded, exposures = [], [], [], 0
    maps = load(bound(stage/'loaded-images-v1.json'))['processes']; launch = load(bound(stage/'visible-launch-v1.json'))
    require(launch['returncode'] == 0 and launch['inputs_unchanged'] and launch['mapped_processes'] == len(maps) == 7, 'Visible native mapping incomplete')
    for invocation,archive,expected,passed in versions:
        receipt = load(bound(repo/'build/gui-acceptance'/invocation/'results.json')); frozen = capsule(archive) if archive else visible
        require(receipt.get('finished_utc') and receipt['end_reason'] == 'completed-selection' and all(n in frozen and hashlib.sha256(frozen[n]).hexdigest() == d for n,d in receipt['source_hashes'].items()), 'Executed source/terminal state differs')
        active = {v['name']:v for v in receipt['results'] if v['status'] != 'not-run'}; require(set(active) == expected, 'Native selection changed')
        for name,row in active.items():
            good = name in passed; require(row['status'] == ('passed' if good else 'failed') and (row['returncode'] == 0) == good, 'Native status changed')
            log = bound(row['log']); case = log.parent; tests = list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'))
            require(len(tests) == 1 and tests[0].get('name') == name and tests[0].find('skipped') is None and (tests[0].find('failure') is None) == good, 'Exact JUnit differs')
            item = dict(invocation=invocation,test=name)
            if not good:
                diagnostic = 'Body reader creation route missing/duplicated' if invocation.endswith('v2') else 'Cannot set central body for body "MainBody" because body "" cannot be found.' if name == PREFIX+'Star' else 'disallowed object field "MainBody.Mu"'
                require(diagnostic in log.read_text(), 'Original body/fixture failure lost'); failures.append(item); continue
            native = [json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text())]
            require(len(native) == 1 and native[0]['outcome'] == 'passed', 'Native emitted proof differs')
            if name in REG:
                expected_checkpoint = 'function-frame-Automatic' if name == 'QtGui.NasaFunctionFrameAutomatic' else 'body-script-preview-Sun'
                require(native[0]['checkpoint'] == expected_checkpoint, 'Original regression proof identity differs')
                if (case/'proof.json').is_file(): require(load(bound(case/'proof.json')) == native[0], 'Stored original proof differs')
            else:
                require(load(bound(case/'proof.json')) == native[0], 'Stored scope proof differs')
            if invocation.endswith('v2') and name == PREFIX+'Sun': superseded.append(item); continue
            if name not in REG:
                spec = specs[name.removeprefix(PREFIX)]; proof = native[0]; contract(proof,spec)
                require(proof['data_input'] == dict(path=kernel['path'],sha256=kernel['sha256'],copied=False), 'Referenced data identity differs')
                caller = bound(case/'caller.script').read_text(); original = bound(case/'initial.gmf').read_text(); edited = bound(case/'edited.gmf').read_text(); nested = bound(case/'nested.gmf').read_text()
                require(original.replace('PrivateOnly = 103;','PrivateOnly = 41;') == edited and 'Mark = PrivateOnly + Calls;' in edited and '[Mark] = ReadShared();' in nested and not re.search(r'Global[^;]*(?:MainBody|FunctionBody|Sun)',caller+edited+nested), 'Exact body source/edit/automatic syntax differs')
                if spec['key'] == 'Sun': require('Create Star' not in caller+edited and 'Write Sun ' in edited, 'Existing Sun falsely promoted to custom creation')
                else: require('Create '+spec['type']+' MainBody;' in caller and not re.search(r'\bCreate\s+\w+\s+FunctionBody\s*;',caller) and 'Create '+spec['type']+' FunctionBody;' in edited, 'Function declaration location differs')
                prototypes = load(bound(case/'prototypes-before.json'))
                for phase in proof['phases']:
                    tag = str(phase['phase']); require([float(v) for v in bound(case/(tag+'-values.txt')).read_text().split()] == phase['raw_values'] and load(bound(case/(tag+'-prototypes.json'))) == prototypes and all(load(bound(case/(tag+'-invariants.json'))).values()), 'Raw counter/prototype/invariant differs')
                    text = bound(case/(tag+'-messages.txt')).read_text(); main = 'Sun' if spec['key'] == 'Sun' else 'MainBody'
                    for object_name,key in [(main,'main_writes'),('Sun' if spec['key'] == 'Sun' else 'FunctionBody','function_writes')]:
                        require(re.findall(r'^Create '+re.escape(spec['type'])+' '+object_name+r';',text,re.M) == phase[key], 'Raw body constructor/type Write differs')
                    require([float(v) for v in re.findall(r'^'+main+r'\.Mu = ([-+0-9.eE]+);',text,re.M)] == phase['main_mu_writes'], 'Actual shared-body Mu Write differs')
                    if spec['key'] != 'Sun': require([float(v) for v in re.findall(r'^FunctionBody\.Mu = ([-+0-9.eE]+);',text,re.M)] == phase['function_mu_writes'], 'Actual declared-body Mu Write differs')
            if archive is None:
                display = receipt['visible_display']; require(display['start_minimized'] and display['resizable'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'] and receipt['max_screenshots'] == 0 and row['trace']['native_exposure_events'] > 0 and not row['trace']['screenshots'], 'Visible policy differs')
                exposures += row['trace']['native_exposure_events']; traces = list(case.glob('trace-*.jsonl')); require(len(traces) == 1, 'Visible process trace missing'); process = maps[traces[0].stem.removeprefix('trace-')]
                exes = [n for n in row['registered_command'] if n in receipt['binary_hashes']]; require(len(exes) == 1 and process['executable'] == exes[0] and all(before['images'].get(n) == d for n,d in process['images'].items()), 'Mapped image identity differs')
                for n,d in before['images'].items():
                    if n == exes[0] or Path(n).name.startswith(('libGmatBase','libGmatUtil')): require(process['images'].get(n) == d, 'Actual executable/core image unmapped')
            selected.append(item)
    require(len(selected) == 14 and len(failures) == 9 and len(superseded) == 1, 'Body totals differ')
    require('error:' in bound(stage/'build-v1.log').read_text() and bound(stage/'NasaFunctionBodyTests.compile-v1.cpp').is_file(), 'Compile failure/source lost')
    negatives = []; sample = load(bound(repo/'build/gui-acceptance/function-body-scope-visible-v1'/(PREFIX+'Planet')/'proof.json'))
    for label, corrupt in [('wrong-function-body-lookup',lambda p:p['phases'][0]['identities'][0].__setitem__('declaration_object_pointer','wrong')),('configured-model-substitution',lambda p:p['phases'][0]['identities'][0].__setitem__('reader_solar_system_pointer',p['phases'][0]['identities'][0]['configured_solar_system_pointer'])),('configured-body-leak',lambda p:p['phases'][0].__setitem__('configured_function_body_absent',False)),('missing-repeat',lambda p:p['phases'].pop()),('wrong-body-literal',lambda p:p['phases'][0]['function_body'].__setitem__('mu',0))]:
        bad = copy.deepcopy(sample); corrupt(bad)
        try: contract(bad,specs['Planet'])
        except (ValueError,AssertionError): negatives.append(label)
        else: raise ValueError('Accepted corrupt body proof '+label)
    sun = load(bound(repo/'build/gui-acceptance/function-body-scope-visible-v1'/(PREFIX+'Sun')/'proof.json')); sun['creation_mode'] = 'function-side-create'
    try: contract(sun,specs['Sun'])
    except (ValueError,AssertionError): negatives.append('false-custom-Star-creation')
    else: raise ValueError('Existing Sun promoted to custom creation')
    return dict(status='passed-four-NASA-body-declarations-and-existing-Sun-shared-model',qualified_keys=[],selected=selected,retained_GUI_failures=failures,superseded_Sun_pass=superseded,retained_compile_failures=1,new_scope_GUI_passes=10,affected_original_regression_GUI_passes=4,scope_missions=30,function_calls=120,reader_calls=90,actual_solar_model_identity_checks=90,function_body_lookup_identity_checks=90,custom_body_missions=24,existing_Sun_missions=6,Script_body_constructor_lines=168,Script_Mu_values=192,raw_marker_scalars=150,caller_anchor_component_checks=180,visible_mapped_processes=7,visible_exposure_events=exposures,negative_controls=negatives,custom_Star_creation_qualified=False,Star_factory_row_excluded=False,kernel_copies=0,production_changed_files=['src/base/function/Function.cpp','src/base/function/Function.hpp'],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--stage',type=Path,required=True); parser.add_argument('--output',type=Path,required=True); args = parser.parse_args()
    result = audit(Path(__file__).resolve().parents[3],args.stage.resolve()); args.output.write_text(json.dumps(result,indent=2)+'\n'); print(json.dumps(result,indent=2))
