"""Admit the declared central-body binding family from frozen native evidence.

Each declared default body and user-body type needs its own GUI/native
trajectory witness. This contract concerns body-relative propagation capability;
it does not qualify all force models, ephemeris sources or physical domains.
"""
import argparse
import csv
import json
import math
import hashlib
import tarfile
import re
from pathlib import Path

import numpy as np
from scipy.integrate import solve_ivp

import NasaGuiCoverage as nasa
import AuditAcceptanceRecovery as recovery
from AuditCapiAcceptance import load, require, sha
from AuditConstraintScalarAcceptance import passed_junit, validate_visible

SOURCE='src/qtgui/tests/NasaCentralBodyPropagationTests.cpp'
FIXTURE='src/qtgui/tests/NasaCentralBodyPropagationCases.json'
KEY='FRR!E145'
DEFAULTS=['Sun','Mercury','Venus','Earth','Luna','Mars','Jupiter','Saturn','Uranus','Neptune','Pluto']
USER_TYPES=['Moon','Asteroid','Comet','Planet']


def body_records(result):
    """Read this family's original native schema without rewriting receipts.

    The generic NASA reader requires a facets array. These new receipts instead
    carry explicit body/type, clock, raw-state and binding fields. Validate those
    fields here and compare the entire record with the actual saved payload.
    """
    require(result['status']=='passed','Body terminal result is not passing')
    records=[]
    for line in Path(result['log']).read_text().splitlines():
        match=re.fullmatch(r'\s*(?:\d+:\s*)?NASA_CHECK (\{.*\})\s*',line)
        if not match:continue
        record=json.loads(match[1])
        if record.get('checkpoint')=='central-body-propagation' and record.get('outcome')=='passed':
            require({'case','body','type','user','rows','truth','bindings','native_final_state'}<=set(record),'Incomplete original body record schema')
            records.append(record)
    return records


def passing_visible_projection(receipt):
    """Validate passing cases without rewriting a mixed terminal receipt.

    The caller still returns the original receipt and all failed results.
    Whole-family selection checks must replace failures with passing evidence;
    an unrelated failure must not force replay of already observed GUI passes.
    """
    failed = [r for r in receipt['results'] if r['status'] not in ('passed', 'not-run')]
    if not failed:
        return receipt
    return dict(receipt,
                results=[r for r in receipt['results'] if r['status'] in ('passed', 'not-run')],
                visible_validation_scope=dict(passing_cases_only=True,
                    excluded_failed_tests=[r['name'] for r in failed],
                    original_terminal_status_preserved=True))


def read_run(repo,spec,inventory,artifacts,physical_inputs):
    before,after=load(spec['before']),load(spec['after'])
    require(before==after,'Body source/runtime changed during execution')
    reuse=load(spec['reuse']);receipt=load(reuse['receipt'])
    require(sha(reuse['receipt'])==reuse['receipt_sha256'] and receipt.get('finished_utc') and receipt.get('end_reason')=='completed-selection','Changed/nonterminal body receipt')
    sources={};hashes={}
    for field,path in [('reused_source_sha256',reuse['reference']),('changed_source_sha256',reuse['delta'])]:
        require(sha(path)==reuse['reference_sha256' if field.startswith('reused') else 'delta_sha256'],'Body source archive changed')
        with tarfile.open(path) as bundle:
            for name,digest in reuse[field].items():
                require(not Path(name).is_absolute() and '..' not in Path(name).parts and name not in hashes,'Unsafe/duplicate body source member')
                data=bundle.extractfile(name).read()
                require(hashlib.sha256(data).hexdigest()==digest and not data.startswith(b'\x7fELF'),'Changed/non-source body archive member')
                require(before['sources'].get(name,digest)==digest,'Body source disagrees with wider snapshot')
                sources[name]=data;hashes[name]=digest
    other=reuse['recorded_authority_ELF_or_existing_physical_inputs_not_copied']
    require(not set(other)&set(hashes) and {**hashes,**other}==receipt['source_hashes'],'Incomplete body source partition')
    require({p:other[p] for p in inventory['sources']}==inventory['sources'],'Body NASA authority differs')
    for name,digest in other.items():
        if name in inventory['sources']:continue
        if name.startswith('application/data/'):
            require(physical_inputs.get(name)==before['sources'].get(name)==digest,'Executed body physical input not bound before run')
        else:require(before['binaries'].get(str(repo/name))==digest,'Unbound body executable identity')
    results={r['name']:r for r in receipt['results'] if r['status']!='not-run'}
    require(len(results)==sum(r['status']!='not-run' for r in receipt['results']),'Duplicate body case result')
    for name,result in results.items():
        executables=[p for p in result['registered_command'] if p in receipt['binary_hashes']]
        require(len(executables)==1 and before['binaries'].get(executables[0])==receipt['binary_hashes'][executables[0]],'Body executable not bound before run')
        if result['status']!='passed':continue
        junit=Path(result['log']).parent/'ctest.xml'
        require(result['returncode']==0 and passed_junit(junit,name,0),'Passing body result lacks exact successful JUnit')
        require(all(str(p) in artifacts for p in [Path(result['log']),junit]),'Actual body log/JUnit omitted')
    if receipt['mode']=='visible':
        projection=passing_visible_projection(receipt)
        validate_visible(projection,{name:dict(log=result['log'],log_sha256=sha(result['log']),junit=str(Path(result['log']).parent/'ctest.xml'),junit_sha256=sha(Path(result['log']).parent/'ctest.xml')) for name,result in results.items() if result['status']=='passed'})
    else:require(receipt['mode']=='headless','Unknown body execution mode')
    return receipt,results,sources


def numerics(matrix, case, rows, times):
    require(len(rows)==len(times)==4,'Missing central-body epochs')
    gm=case['mu_km3_s2'];radius=case['radius_km'];inc=math.radians(matrix['inclination_degrees'])
    speed=math.sqrt(gm/radius);period=2*math.pi*math.sqrt(radius**3/gm)
    require(times[0]==0 and all(math.isfinite(t) for t in times) and all(a<b for a,b in zip(times,times[1:])),'Invalid body dynamics time vector')
    def derivative(t,state):
        position=state[:3]
        return np.concatenate((state[3:],-gm*position/np.linalg.norm(position)**3))
    initial=[radius,0,0,0,speed*math.cos(inc),speed*math.sin(inc)]
    solution=solve_ivp(derivative,(0,times[-1]),initial,method='DOP853',t_eval=times,rtol=2.3e-14,atol=[1e-11]*3+[1e-13]*3)
    require(solution.success and solution.y.shape==(6,4),'Independent body reference integration failed')
    max_position=max_velocity=0
    for index,(row,t) in enumerate(zip(rows,times)):
        require(len(row)==9 and all(math.isfinite(v) for v in row),'Invalid body raw fields')
        requested=period*matrix['phase_fractions'][index];bound=matrix['elapsed_bound_seconds']
        require(abs(row[1]-requested)<bound and abs((row[0]-matrix['epoch_a1_mjd'])*86400-requested)<bound and abs(t-requested)<bound and abs(t-row[1])<bound and row[8]==41,'Body requested/reported/native clocks or sentinel differ')
        for component in range(6):
            error=abs(row[component+2]-solution.y[component,index])
            require(error<matrix['position_bound_km' if component<3 else 'velocity_bound_km_s'],'Independent Cartesian ODE reference differs')
            if component<3:max_position=max(max_position,error)
            else:max_velocity=max(max_velocity,error)
    return dict(method='independent Cartesian two-body DOP853; no GMAT force/conversion calls',maximum_position_error_km=max_position,maximum_velocity_error_km_s=max_velocity)


def check_case(matrix,case,result,sources,artifacts):
    work=Path(result['log']).parent;p=load(work/'proof.json')
    require(result['status']=='passed' and result['returncode']==0 and p['outcome']=='passed','Unsuccessful body binding witness')
    require(body_records(result)==[p],'Body proof differs from actual native log')
    require(p['case']==case['key'] and p['body']==case['body'] and p['type']==case['type'] and p['user']==case['user'],'Substituted body/class')
    require(json.loads(sources[FIXTURE])==matrix,'Another independent body fixture was executed')
    for name in ['proof.json','results.csv','baseline.script','accepted.script','build.messages.txt','run.messages.txt','test.log','ctest.xml']:
        require(str(work/name) in artifacts,'Essential body evidence omitted: '+name)
    raw=list(csv.reader((work/'results.csv').open()));require(len(raw)==5 and raw[-1]==['42'],'Raw body row/following sentinel omitted')
    rows=[[float(v) for v in row] for row in raw[:4]];require(rows==p['rows'],'Body proof/raw values differ')
    for field in ['mu_km3_s2','radius_km']:require(p[field]==case[field],'Independent body model input changed')
    require(p['inclination_degrees']==matrix['inclination_degrees'] and p['typed_body_entries']==2,'Body geometry/GUI entry omitted')
    code=sources[SOURCE].decode()
    if 'dynamics_elapsed_seconds' in p:
        timing=p['propagation_timing'];require(len(timing)==3 and load(work/'propagation-timing.json')==timing and str(work/'propagation-timing.json') in artifacts,'Actual unrounded propagation timing omitted')
        times=[0.]
        for segment in timing:
            elapsed=segment['propagator_time_seconds'];require(math.isfinite(elapsed) and elapsed>0 and elapsed==segment['ODE_time_seconds'],'Native propagator/ODE durations differ');times.append(times[-1]+elapsed)
        require(times==p['dynamics_elapsed_seconds'] and 'circular(gm,radius,inclination,dynamicsElapsed)' in code,'Dynamics clock/oracle source mismatch')
    else:
        # The first passing Moon receipt used the reported A1 timestamp. It
        # remains valid at its own source version; no known pass is replayed.
        require(case['key']=='UserMoon' and 'circular(gm,radius,inclination,(values[0].toDouble()-epoch)*86400)' in code,'Unreviewed legacy body timebase')
        times=[(r[0]-matrix['epoch_a1_mjd'])*86400 for r in rows]
    gm=case['mu_km3_s2'];radius=case['radius_km'];inc=math.radians(matrix['inclination_degrees']);omega=math.sqrt(gm/radius**3)
    require(abs(p['period_seconds']-2*math.pi/omega)<1e-9,'Body period changed')
    for row,t,truth in zip(rows,times,p['truth']):
        a=omega*t;expected=[radius*math.cos(a),radius*math.sin(a)*math.cos(inc),radius*math.sin(a)*math.sin(inc),-radius*omega*math.sin(a),radius*omega*math.cos(a)*math.cos(inc),radius*omega*math.cos(a)*math.sin(inc)]
        require(len(truth)==6,'Incomplete body analytic truth')
        for c in range(6):
            bound=matrix['position_bound_km' if c<3 else 'velocity_bound_km_s']
            require(abs(truth[c]-expected[c])<bound and abs(row[c+2]-expected[c])<bound,'Independent inclined circular truth differs')
    check=numerics(matrix,case,rows,times)
    require(len(p['native_final_state'])==6,'Actual final native state omitted')
    for c,value in enumerate(p['native_final_state']):require(abs(value-rows[-1][c+2])<(2e-6 if c<3 else 1e-9),'Native/report frame state differs')
    require(p['bindings']==[dict(central_body=case['body'],point_mass=case['body'],contributors=1)]*3,'Actual Propagate force/central-body bindings omitted')
    before=(work/'baseline.script').read_text();after=(work/'accepted.script').read_text()
    import re
    without=lambda s:re.sub(r'^[ \t]*(?:GMAT[ \t]+)?Forces\.[^\n]*\n','',s,flags=re.M)
    require(before!=after and without(before)==without(after) and before.count('Forces.CentralBody = Earth;')==1 and after.count('Forces.CentralBody = '+case['body']+';')==1,'Body GUI transaction changed unrelated source')
    for token in ['resource_CentralBody','panel->discardChanges()','source->undo()','source->redo()','configured()==baseline','configured()==accepted','read(saved)==applied.toUtf8()','GetClone(0)','force->GetBodyName()==body','runtimeBody->IsUserDefined()==user','runtimeBody->GetGravitationalConstant()==gm','converter.Convert','text->isReadOnly()','configured()==stateBefore']:
        require(token in code,'Original body native/GUI assertion omitted: '+token)
    return check


def check_legacy(spec):
    receipt=json.loads(recovery.payload(spec['receipt']));manifest=json.loads(recovery.payload(receipt['snapshot_manifest']))
    require(sha(spec['receipt'])==spec['receipt_sha256'] and recovery.digest(recovery.payload(receipt['snapshot_manifest']))==spec['manifest_sha256'],'Legacy body receipt/snapshot changed')
    result=next(r for r in receipt['results'] if r['name']==spec['claim']['test'])
    recovery.verify_claim(Path(manifest['root']),manifest,receipt,result,spec['claim'])
    require(receipt['mode']==spec['mode'],'Legacy body mode substituted')
    if spec['mode']=='visible':require(result['trace']['native_exposure_events']>0,'Legacy body GUI not exposed')
    proofs=[p for p in nasa.proof_records(result) if p.get('checkpoint')==spec['claim']['checkpoint']];require(len(proofs)==1,'Ambiguous legacy body proof');p=proofs[0]
    if spec['cell']=='UserAsteroid':
        require(p['body']=='Ceres' and p['user_defined'] and p['max_position_error_km']<=p['analytic_position_tolerance_km']==1e-7 and p['max_velocity_error_km_per_s']<=p['analytic_velocity_tolerance_km_per_s']==1e-10,'Legacy asteroid independent trajectory failed')
    else:
        work=Path(result['log']).parent;runtime=load(work/'runtime.json');require(runtime['runtime_CentralBody']==('Earth' if spec['cell']=='Earth' else 'Luna'),'Legacy central-body identity differs')
        rows=list(csv.reader((work/'values.csv').open()));require(rows[-1]==['42'] and [float(v) for v in rows[0]]==runtime['raw'],'Legacy body raw result/sentinel changed')
        truth=runtime['circle_truth'];require(len(truth)==6 and len(runtime['raw']) in (8,9),'Legacy Cartesian truth incomplete')
        require(p['max_position_error_km']<2e-5 and p['max_velocity_error_km_s']<3e-8,'Legacy body analytic trajectory failed')
    return spec['cell'],spec['mode']


def qualify(repo,path,inventory):
    q=load(path);require(q['contract_kind']=='source-versioned-complete-central-body-propagation-family' and q['qualified_keys']==[KEY],'Wrong/partial central-body credit')
    require(q['authority_sha256']==inventory['sources'] and all(q.get(k) is False for k in ['technical_core_approval','full_current_build_qualified','full_NASA_qualified']),'Body authority or qualification scope changed')
    for path,digest in q['artifact_sha256'].items():require(sha(path)==digest,'Changed body artifact: '+path)
    known={r['key']:r for r in inventory['requirements']};review=load(q['review']);r=review['requirement']
    require(r['key']==KEY and r['fingerprint']==known[KEY]['fingerprint'] and r['source_text']==known[KEY]['text'] and r['rationale'],'Original body requirement changed')
    require(review['original_requirement_review']['evidence']==[] and review['original_requirement_review']['fingerprint']==r['fingerprint'],'Original mapped obligation dropped')
    require(review['default_source_keys']==['FRR!E'+str(i) for i in range(184,195)] and review['user_type_source_keys']==['FRR!E'+str(i) for i in range(214,218)],'Declared body inventory truncated')
    for key,record in review['source_rows'].items():require(record==known[key],'Original body inventory source changed')
    require(set(review['source_rows'])==set(review['default_source_keys']+review['user_type_source_keys']),'Missing original body/type row')
    matrix=load(q['fixture']);cases={c['key']:c for c in matrix['cases']};require(len(cases)==12 and len(matrix['phase_fractions'])==4,'Body epochs/cells omitted')
    new_defaults={c['body'] for c in cases.values() if not c['user']};new_users={c['type'] for c in cases.values() if c['user']}
    require(new_defaults==set(DEFAULTS)-{'Earth','Luna'} and new_users==set(USER_TYPES)-{'Asteroid'},'Missing/extra declared body/type')
    require(matrix['position_bound_km']==5e-6 and matrix['velocity_bound_km_s']==1e-8 and matrix['elapsed_bound_seconds']==1e-5 and matrix['phase_fractions']==[0,.05,.125,.25],'Original body oracles/bounds changed')
    require(set(q['selected'])=={'headless','visible'},'Missing body execution mode')
    runs={name:read_run(repo,spec,inventory,q['artifact_sha256'],q['physical_inputs']) for name,spec in q['runs'].items()}
    for name,spec in q['runs'].items():
        before=load(spec['before']);receipt=runs[name][0]
        for path,digest in q['physical_inputs'].items():require(before['sources'].get(path)==digest==receipt['source_hashes'].get(path),'Executed body physical input not bound before run')
    for mode,selections in q['selected'].items():
        require(set(selections)==set(cases),'Missing body/class execution cell')
        for key,selection in selections.items():
            receipt,results,sources=runs[selection['run']];name='QtGui.NativeNasaCentralBodyPropagation'+key
            require(receipt['mode']==mode and selection['test']==name,'Body test/mode substituted');check_case(matrix,cases[key],results[name],sources,q['artifact_sha256'])
    legacy=[check_legacy(s) for s in q['legacy_cells']]
    require(len(legacy)==6 and set(legacy)=={(cell,mode) for cell in ['Earth','Luna','UserAsteroid'] for mode in ['headless','visible']},'Missing/duplicate inherited body cell/mode')
    # Generic name resolution is the mechanism under test, alongside an actual
    # native trajectory witness for every declared body/type. No metadata-only
    # replacement of a missing body cell is accepted.
    for path,token in review['generic_binding_source_guards'].items():require(token in (repo/path).read_text(),'Generic body binding source guard missing')
    require(review['all_force_models_claimed'] is False and review['all_ephemeris_sources_claimed'] is False and review['all_numeric_domains_claimed'] is False,'Body capability expanded into unrelated science')
    return q,[KEY]


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--qualification',type=Path,required=True);a=p.parse_args();_,keys=qualify(a.repo.resolve(),a.qualification,nasa.inventory(a.repo.resolve()));print(json.dumps(dict(status='passed',qualified_keys=keys,new_GUI_or_engine_executions=0)))
