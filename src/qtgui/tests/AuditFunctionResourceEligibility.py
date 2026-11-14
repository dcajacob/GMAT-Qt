"""Audit frozen parameter-role, vector-routing and retained-scope evidence."""
import argparse,copy,difflib,hashlib,json,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import load,require,sha
from ReviewFunctionResourceEligibility import review
META='QtGui.NasaFunctionParameterRoles';VECTOR='QtGui.NasaFunctionVectorResource'
TYPES=['Cartesian','ModEquinoctial','IncomingAsymptote','OutgoingAsymptote','BrouwerMeanShort','BrouwerMeanLong']


def metadata(p):
    require(p['outcome']=='passed' and p['factory_parameter_count']==len(p['rows'])==324 and p['system_parameter_count']==321 and p['user_parameter_count']==3 and p['user_parameters']==['Array','String','Variable'] and p['configured_source_history_unchanged'],'Incomplete native parameter partition')
    require(len({v['factory_type'] for v in p['rows']})==324 and sorted(v['factory_type'] for v in p['rows'] if v['is_system_parameter'])==p['system_parameters'] and sorted(v['factory_type'] for v in p['rows'] if not v['is_system_parameter'])==p['user_parameters'],'Native parameter roles differ')


def vector(p):
    require(p['outcome']=='passed' and p['types']==TYPES and p['missions']==3 and p['function_calls']==6 and p['component_comparisons']==756 and p['file_decisions']==[{'saved':False},{'saved':True}],'Vector workflow scope differs')
    require(len(p['phases'])==3 and [v['phase'] for v in p['phases']]==[0,1,2] and [v['rebuild'] for v in p['phases']]==[True,True,False],'Missing Unicode/no-Build repeat')
    for phase in p['phases']:
        require(len(phase['rows'])==42 and phase['shared_final_x']==8040 and phase['caller_shadow_x']==7000 and phase['caller_keep']==103 and phase['returned']==[41,41],'Vector ownership/sentinel/count differs')
        for i,row in enumerate(phase['rows']):
            require(len(row)==13 and row[0]==[10,20,30,40,30,40,50][i//6],'Vector role/order differs')
            require(all(abs(row[1+c]-row[7+c])<=1e-8 for c in range(6)),'Vector/component routing mismatch')
            if i%6==0:require(abs(row[1]-[7000,8000,7500,8020,7500,8040,8040][i//6])<1e-10,'Cartesian owner is wrong')
    require(p['phases'][0]['rows']==p['phases'][1]['rows']==p['phases'][2]['rows'],'Repeat changed raw rows')


def audit(repo,stage):
    bindings=load(stage/'evidence-inputs-v1.json')['artifact_sha256']
    for name,digest in bindings.items():require(Path(name).is_file() and not Path(name).is_symlink() and sha(name)==digest,'Changed regular input '+name)
    def bound(p):p=Path(p);require(str(p) in bindings,'Unbound '+str(p));return p
    def archive(p):
        with tarfile.open(bound(p)) as t:
            require(all(v.isfile() for v in t.getmembers()),'Nonregular source capsule');return {v.name:t.extractfile(v).read() for v in t.getmembers()}
    visible_sources=archive(stage/'visible-executed-sources.tar.gz');before=load(bound(stage/'visible-inputs-before.json'));require(before==load(bound(stage/'visible-inputs-after.json')) and {p:hashlib.sha256(b).hexdigest() for p,b in visible_sources.items()}==before['sources'],'Visible inputs changed')
    launch=load(bound(stage/'visible-launch.json'));maps=load(bound(stage/'loaded-images.json'))['processes'];require(launch['returncode']==0 and launch['mapped_processes']==2 and len(maps)==2 and launch['inputs_unchanged'],'Visible invocation incomplete')
    versions=[('function-parameter-roles-headless-v1','headless-executed-sources-v1.tar.gz',{META:False}),('function-parameter-roles-headless-v2','headless-executed-sources-v2.tar.gz',{META:False}),('function-parameter-roles-headless-v3','headless-executed-sources-v3.tar.gz',{META:False}),('function-vectors-headless-v1','vector-executed-sources-v1.tar.gz',{VECTOR:False}),('function-eligibility-headless-v4','headless-executed-sources-v4.tar.gz',{META:True,VECTOR:False}),('function-vectors-headless-v5','vector-executed-sources-v5.tar.gz',{VECTOR:False}),('function-vectors-headless-v6','vector-executed-sources-v6.tar.gz',{VECTOR:False}),('function-vectors-headless-v7','vector-executed-sources-v7.tar.gz',{VECTOR:False}),('function-eligibility-headless-v8','headless-executed-sources-v8.tar.gz',{META:True,VECTOR:False}),('function-eligibility-headless-v9','headless-executed-sources-v9.tar.gz',{META:True,VECTOR:False}),('function-vectors-headless-v11','vector-executed-sources-v11.tar.gz',{VECTOR:True}),('function-eligibility-visible-v1',None,{META:True,VECTOR:True})]
    selected=[];failed=[];superseded=[];meta=None;last_vector=None
    for folder,capsule,expected in versions:
        root=repo/'build/gui-acceptance'/folder;r=load(bound(root/'results.json'));require(r.get('finished_utc') and r['end_reason']=='completed-selection','Nonterminal native attempt');frozen=archive(stage/capsule) if capsule else visible_sources
        for name,digest in r['source_hashes'].items():require(name in frozen and hashlib.sha256(frozen[name]).hexdigest()==digest,'Executed source differs '+name)
        active={v['name']:v for v in r['results'] if v['status']!='not-run'};require(set(active)==set(expected),'Changed selected registration')
        for name,passed in expected.items():
            row=active[name];require(row['status']==('passed' if passed else 'failed') and (row['returncode']==0)==passed,'Native status differs');log=bound(row['log']);case=log.parent;tests=list(ET.parse(bound(case/'ctest.xml')).getroot().iter('testcase'));require(len(tests)==1 and tests[0].get('name')==name and tests[0].find('skipped') is None and (tests[0].find('failure') is None and tests[0].find('error') is None)==passed,'Native JUnit differs')
            if not passed:failed.append(dict(invocation=folder,test=name));continue
            emitted=[json.loads(v) for v in re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text())];require(len(emitted)==1,'Native proof missing');p=emitted[0]
            if name==META:
                metadata(p);require(load(bound(case/'native-parameter-roles.json'))==p,'Native role file differs');require(meta is None or meta==p,'Metadata changed between source versions/modes');meta=p
            else:
                vector(p);last_vector=p;require(load(bound(case/'proof.json'))==p and load(bound(case/'phases.json'))==p['phases'],'Raw vector phase proof differs')
                raw=[]
                for phase in p['phases']:
                    b=bound(case/(str(phase['phase'])+'-vectors.txt')).read_text();rows=[[float(v) for v in re.split(r'[,\s]+',line.strip())] for line in b.strip().splitlines()];require(rows==phase['rows'],'Raw report differs from vector proof');raw.append(b)
                require(raw[0]==raw[1]==raw[2],'Raw vector repeat bytes differ')
                for shape in ['5,1','2,3']:require('ERROR' in bound(case/('invalid-'+shape+'-messages.txt')).read_text(),'Missing dimension rejection')
            if capsule is None:
                display=r['visible_display'];require(display['resizable'] and display['start_minimized'] and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN' not in display['initial_host_window_state'] and r['max_screenshots']==0 and row['trace']['native_exposure_events']>0 and not row['trace']['screenshots'],'Wrong visible window policy')
                exes=[v for v in row['registered_command'] if v in r['binary_hashes']];require(len(exes)==1,'Executed image unbound')
                traces=list(case.glob('trace-*.jsonl'));require(len(traces)==1,'Missing visible process trace')
                for trace in traces:
                    process=maps.get(trace.stem.removeprefix('trace-'));require(process and process['executable']==exes[0],'Process map differs')
                    for path,digest in process['images'].items():require(before['images'].get(path)==digest,'Mapped image differs')
                    for path,digest in before['images'].items():
                        if path==exes[0] or Path(path).name.startswith(('libGmatBase','libGmatUtil','libGmatEstimation','libEKF','libStation','libThrustFile')):require(process['images'].get(path)==digest,'Required core/provider image unmapped')
            item=dict(invocation=folder,test=name)
            (superseded if folder in {'function-eligibility-headless-v4','function-eligibility-headless-v8'} else selected).append(item)
    require(len(selected)==4 and len(superseded)==2 and len(failed)==10,'Native attempt totals differ')
    diagnostics=load(bound(repo/'build/gui-acceptance/function-parameter-roles-headless-v3/QtGui.NasaFunctionParameterRoles/failed-parameter-role.json'));require(diagnostics['factory_types']==TYPES[1:] and len(diagnostics['completed_rows'])==319,'Original five unconstructable entries lost')
    current={v['factory_type']:v for v in meta['rows']};require(all(current[v['factory_type']]==v for v in diagnostics['completed_rows']),'Prior 319 native metadata rows changed')
    references=archive(stage/'eligibility-reference-sources-v1.tar.gz');prefix='src/base/factory/ParameterFactory.cpp';old=bound(stage/'ParameterFactory.cpp.before').read_bytes();new=references[prefix];expected=old.replace(b'if (ofType == "ModifiedEquinoctial")',b'if (ofType == "ModifiedEquinoctial" || ofType == "ModEquinoctial")')
    for scalar,scalar_class,whole,whole_class in [('IncomingBVAZI','IncAsymBVAZI','IncomingAsymptote','IncAsymState'),('OutgoingBVAZI','OutAsymBVAZI','OutgoingAsymptote','OutAsymState'),('BrouwerShortMA','BLshortMAP','BrouwerMeanShort','BLshortState'),('BrouwerLongMA','BLlongMADP','BrouwerMeanLong','BLlongState')]:
        prior=('   if (ofType == "'+scalar+'")\n      return new '+scalar_class+'(withName);').encode();expected=expected.replace(prior,prior+('\n   if (ofType == "'+whole+'")\n      return new '+whole_class+'(withName);').encode())
    require(new==expected,'Unexpected factory route/list/scalar change')
    for name in ['Rvec6Var.cpp','Rvec6Var.hpp']:
        original=bound(stage/(name+'.before')).read_text();changed=references['src/base/parameter/'+name].decode();require(not any(v.startswith('- ') for v in difflib.ndiff(original.splitlines(),changed.splitlines())),'An original fixed-vector line changed')
    original=bound(stage/'OrbitRvec6.cpp.before').read_text();changed=references['src/base/parameter/OrbitRvec6.cpp'].decode();expected=original.replace('              objType)\n','              objType),\n     OrbitData(name, typeStr, objType, depObj)\n').replace('bool OrbitRvec6::Initialize()\n{\n','bool OrbitRvec6::Initialize()\n{\n   Rvec6Var::Initialize();\n   SetParameter(this);\n');expected=expected.replace('   return OrbitData::GetRefObjectName(type);\n}\n','   return OrbitData::GetRefObjectName(type);\n}\n\nconst StringArray& OrbitRvec6::GetRefObjectNameArray(const UnsignedInt type)\n{\n   return OrbitData::GetRefObjectNameArray(type);\n}\n');require(changed==expected,'Unexpected orbit-vector formula/ownership change')
    scope=review(repo,stage);require(scope==load(bound(stage/'scope-review-v1.json')) and scope['qualified_keys']==[] and len(scope['rows'])==566 and scope['retained_context_count']==96 and not scope['unbound_assertion_contexts'],'Role/witness review differs')
    for name,digest in scope['retained_artifact_sha256'].items():require(bindings.get(name)==digest,'Retained input unbound')
    negatives=[]
    for label,probe,change,check in [('lost-parameter',meta,lambda p:p['rows'].pop(),metadata),('wrong-vector-component',last_vector,lambda p:p['phases'][0]['rows'][0].__setitem__(1,17),vector),('wrong-owner-context',last_vector,lambda p:p['phases'][0]['rows'][0].__setitem__(0,40),vector),('lost-repeat',last_vector,lambda p:p['phases'].pop(),vector)]:
        p=copy.deepcopy(probe);change(p)
        try:check(p)
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt proof '+label)
    return dict(status='passed-bounded-factory-vector-repair-and-complete-role-review',qualified_keys=[],selected=selected,superseded_metadata_passes=superseded,retained_native_failures=failed,native_parameter_roles=324,system_parameters=321,user_parameters=3,factory_role_rows=566,retained_GUI_contexts=96,resource_types_with_retained_witnesses=28,native_vector_missions=6,native_function_calls=12,raw_report_rows=252,vector_component_comparisons=1512,independent_Cartesian_owner_checks=42,incompatible_array_shape_rejections=4,negative_controls=negatives,production_changed_files=['src/base/factory/ParameterFactory.cpp','src/base/parameter/Rvec6Var.cpp','src/base/parameter/Rvec6Var.hpp','src/base/parameter/OrbitRvec6.cpp','src/base/parameter/OrbitRvec6.hpp'],scientific_approval=False,technical_core_approval=False,full_current_build_qualified=False,full_NASA_qualified=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
