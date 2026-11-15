"""Replay quaternion boundary and actual GUI-rejection evidence without reruns."""
import argparse
import collections
import hashlib
import json
import math
from pathlib import Path
import tarfile

import NasaGuiCoverage as nasa
from AuditAttitudeOffsetScopeEvidence import numbers, require

REPO=Path(__file__).resolve().parents[3]
SHA=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
LOAD=lambda p:json.loads(Path(p).read_text())


def independent_inputs(catalog,original):
    frames={c['key']:c for c in original['cases']}
    q=original['offset_quaternion']
    expected=[('Identity',[0,0,0,1]),('Negative',[-v for v in q]),
              ('Normalized',[7*v for v in q]),('HalfTurn',[1,2,3,0]),
              ('HalfTurnBody',[0,1,0,0]),('HugeFinite',[1e308,-1e308,1e308,-1e308]),
              ('ThresholdIdentity',[0,0,0,1e-10]),('SmallAccepted',[1e-9*v for v in q]),
              ('NegativeIdentity',[0,0,0,-4])]
    require([(c['key'],c['input_quaternion'])for c in catalog['cases']]==expected,
            'Missing/changed quaternion boundary partition')
    require(catalog['invalid_inputs']==['','0 0 0','0 0 0 1 2','0 0 0 0','0 0 0 1e-12',
                                      'nan 0 0 1','inf 0 0 1','-inf 0 0 1','1e309 0 0 1','word 0 0 1'],
            'Missing/changed GUI invalid-input partition')
    for case in catalog['cases']:
        base=frames[case['base_frame_case']]
        require(case['matrix_bound']==base['matrix_bound'] and case['rate_bound_degrees_s']==5e-9
                and case['setup']==base['setup'], 'Original frame bounds/setup changed')
        raw=case['input_quaternion'];scale=max(abs(v)for v in raw)
        normalized=[v/scale/math.hypot(*(w/scale for w in raw))for v in raw]
        require(max(abs(a-b)for a,b in zip(normalized,case['offset_quaternion']))<4e-16,
                'Independent normalization differs')
        require(case['serializes_offset']==(normalized!=[0,0,0,1]), 'Default script-cloaking contract differs')
        vector_norm=math.hypot(*normalized[:3])
        if vector_norm==0:
            rotation=[[int(i==j)for j in range(3)]for i in range(3)]
        else:
            angle=2*math.atan2(vector_norm,normalized[3]);c,s=math.cos(angle),math.sin(angle)
            u=[v/vector_norm for v in normalized[:3]]
            cross=[[0,-u[2],u[1]],[u[2],0,-u[0]],[-u[1],u[0],0]]
            rotation=[[c*(i==j)+(1-c)*u[i]*u[j]-s*cross[i][j]for j in range(3)]for i in range(3)]
        require(max(abs(rotation[i][j]-case['offset_matrix'][i][j])for i in range(3)for j in range(3))<2e-15,
                'Independent axis-angle boundary rotation differs')
        for sample,prior in zip(case['samples'],base['samples']):
            for name in ['epoch','frame_rows','frame_basis_rates','frame_angular_velocity_degrees_s']:
                require(sample[name]==prior[name], 'Retained frame oracle input changed')
            for i in range(3):
                for j in range(3):
                    require(abs(math.fsum(rotation[i][k]*prior['frame_rows'][k][j]for k in range(3))-
                                sample['rows'][i][j])<3e-15, 'Boundary expected basis is not independently composed')
                require(abs(math.fsum(rotation[i][k]*prior['frame_angular_velocity_degrees_s'][k]for k in range(3))-
                            sample['angular_velocity_degrees_s'][i])<3e-15, 'Boundary expected rate differs')


def replay(stage,output):
    artifacts={}
    def pin(p):
        p=Path(p);artifacts[str(p)]=SHA(p);return p
    project=stage/'boundary-build-v2';producer=LOAD(pin(project/'build-receipt-v2.json'))
    require(producer['status']=='passed' and producer['returncodes']==[0,0] and producer['inputs_unchanged']
            and producer['inherited_frontend_source_version']==6 and producer['inherited_core_source_version']==4,
            'Boundary/frontend/core producer conjunction failed')
    sources=LOAD(pin(project/'boundary-source-sha256-v2.json'))
    require(sources==producer['boundary_source_sha256'], 'Boundary compiler closure changed')
    with tarfile.open(pin(project/'boundary-source-v2.tar.gz'))as archive:
        require(set(archive.getnames())==set(sources), 'Boundary source capsule partition changed')
        for name,digest in sources.items():
            require(hashlib.sha256(archive.extractfile(name).read()).hexdigest()==digest
                    and SHA(project/'source'/name)==digest, 'Boundary source bytes changed')
    for name,digest in {**producer['inherited_build_inputs'],**producer['binary_sha256']}.items():
        require(SHA(name)==digest,'Boundary compiler/link/runtime input changed')
    parent=LOAD(pin(stage/'scope-build-receipt-v6-attempt2.json'))
    require(producer['parent_build_receipt_sha256']==SHA(stage/'scope-build-receipt-v6-attempt2.json'),
            'Different inherited frontend producer')
    frozen=Path(parent['frozen_source_root'])
    original=LOAD(pin(frozen/'src/qtgui/tests/AttitudeOffsetScopeCases.json'))
    catalog=LOAD(pin(project/'source/src/qtgui/tests/AttitudeOffsetBoundaryCases.json'))
    require(catalog['source_fixture_sha256']==SHA(frozen/'src/qtgui/tests/AttitudeOffsetScopeCases.json'),
            'Boundary frame inputs are from a different catalog')
    independent_inputs(catalog,original)
    cases={c['key']:c for c in catalog['cases']};covered=set();witnesses=[];rejections=0
    executable=next(p for p in producer['binary_sha256']if Path(p).name=='GmatQtNasaAttitudeOffsetBoundaryTests')
    for run_name,count in [('offset-boundary-normalized-headless-v2',1),('offset-boundary-remaining-headless-v2',8),('offset-boundary-visible-v2',9)]:
        run=LOAD(pin(stage/run_name/'results.json'));maps=LOAD(pin(stage/(run_name+'-loaded-images.json')));mode=run['mode']
        require(maps['returncode']==0 and maps['inputs_unchanged'] and maps['runtime_binary_copies']==0
                and maps['source_version']=='frontend6-core4-boundary2'
                and maps['build_receipt_sha256']==SHA(project/'build-receipt-v2.json'), 'Boundary run/maps/producer failed')
        for name,digest in maps['loaded_project_images'].items():require(SHA(name)==digest,'Mapped boundary image changed')
        selected=[r for r in run['results']if r['status']!='not-run'];require(len(selected)==count,'Omitted/extra boundary case')
        processes=[p for p in maps['processes']if p['executable']==executable]
        require(len(processes)==count,'Missing/extra mapped boundary process')
        if mode=='visible':
            display=run['visible_display'];require(run['resizable'] and display['resizable'] and display['start_minimized']
                and 'Iconic' in display['initial_host_window_state'] and '_NET_WM_STATE_FULLSCREEN'not in display['initial_host_window_state'],
                'Boundary visible desktop was not initially minimized/resizable/nonfullscreen')
        for result in selected:
            work=Path(result['log']).parent;proof=LOAD(pin(work/'proof.json'));key=proof['case'];case=cases[key]
            require((mode,key)not in covered,'Repeated boundary profile/case');covered.add((mode,key))
            require(result['status']=='passed' and result['returncode']==0 and proof['outcome']=='passed'
                    and proof['checkpoint']=='attitude-offset-boundaries' and proof['repeat_runs']==2
                    and proof['actual_popups']==6 and proof['input_quaternion']==case['input_quaternion']
                    and proof['offset_quaternion']==case['offset_quaternion'] and proof['offset_matrix']==case['offset_matrix'],
                    'Boundary native proof is incomplete or substituted')
            require([p for p in nasa.proof_records(result)if p.get('checkpoint')=='attitude-offset-boundaries']==[proof],
                    'Boundary proof differs from actual native log')
            require(LOAD(pin(work/'case.json'))==case and LOAD(pin(work/'checks.json'))==proof['checks'],'Executed boundary inputs/checks differ')
            raw=pin(work/'values.csv').read_bytes();repeated=pin(work/'repeat.values.csv').read_bytes()
            numbers(case,proof,raw,repeated,catalog['input_state'])
            for name in ['initial.script','accepted.script','build.messages.txt','run.messages.txt','repeat.messages.txt','test.log','ctest.xml','result.json']:pin(work/name)
            initial,accepted=(work/'initial.script').read_text(),(work/'accepted.script').read_text()
            require(initial!=accepted and ('Sat.OffsetQuaternion'in accepted)==case['serializes_offset']
                    and initial.split('BeginMissionSequence;',1)[1]==accepted.split('BeginMissionSequence;',1)[1],
                    'Actual normalized/default serialization or mission preservation differs')
            process=[p for p in processes if p['command'][-1]==key];require(len(process)==1,'No unique native boundary witness')
            traces=[json.loads(line)for path in work.glob('trace-*.jsonl')for line in pin(path).read_text().splitlines()]
            if key=='Normalized':
                checks=LOAD(pin(work/'invalid-checks.json'));require(checks==proof['invalid_checks'] and len(checks)==10,'GUI invalid-input results omitted')
                for check,invalid,error in zip(checks,catalog['invalid_inputs'],catalog['invalid_expected_errors']):
                    require(check['input']==invalid and error in check['error'] and all(check[n]for n in
                        ['Apply_rejected','selector_rejected','draft_and_native_isolated','correction_exact']),
                        'Actual GUI rejection/transaction/correction conjunction incomplete')
                counts=collections.Counter((e.get('name'),e.get('event'))for e in traces)
                require(counts['applyPanel','activated']>=11 and counts['attitude_EulerAngleSequence','input']>=12 and counts['attitude_EulerAngleSequence','changed']>=12,
                        'Actual parent Apply/selector activations missing')
                rejections+=20
            else:require(proof['invalid_checks']==[],'Invalid-input evidence attributed to another case')
            if mode=='visible':require(result['requested_visibility']=='visible' and result['trace']['native_exposure_events']>0,
                                       'Boundary has no actual visible native exposure')
            witnesses.append(dict(profile=mode,case=key,pid=process[0]['pid'],proof=str(work/'proof.json')))
    require(covered=={(mode,key)for mode in ['headless','visible']for key in cases},'Incomplete boundary/profile conjunction')
    require(rejections==40,'Expected40 actual Apply/selector rejections across both profiles')
    result=dict(status='passed',contract='fixed-offset-boundaries-and-actual-GUI-atomicity',passed_profile_cases=len(covered),
                checked_epochs=3*len(covered),actual_mission_runs=2*len(covered),actual_GUI_rejections=rejections,
                witnesses=witnesses,artifacts=artifacts,reader_sha256=SHA(__file__),native_reruns=0,new_requirement_credit=0,
                whole_row_qualified=False,scientific_approval=False,technical_core_approval=False,
                full_current_build_qualified=False,full_NASA_qualified=False)
    with output.open('x')as stream:json.dump(result,stream,indent=2);stream.write('\n')
    return result


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,default=REPO/'build/nasa-coverage/coverage95-20261104-attitude-fixed-offset');p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    r=replay(a.stage.resolve(),a.output.resolve());print(json.dumps({k:r[k]for k in ['status','passed_profile_cases','checked_epochs','actual_GUI_rejections','native_reruns','new_requirement_credit']}))
