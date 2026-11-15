"""Audit native shared-Formation copy facts without whole-resource credit."""
from pathlib import Path
import hashlib,json,math,tarfile

load=lambda p:json.loads(Path(p).read_text())
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(v,message):
    if not v:raise ValueError(message)

def check_proof(proof):
    mode=proof['mode'];shared=mode=='Shared'
    need(mode in ['Exclusive','Shared'] and proof['status']=='passed' and proof['sharing_allowed']==shared,
         'Wrong clone mode or result')
    need(proof['original_preserved'] and proof['cancel_and_rejection_atomic'] and proof['new_requirement_credit']==0,
         'Clone preservation/guard obligation is missing')
    need([x['phase']for x in proof['runs']]==(['shared-copy','independent-copy']if shared else []),'Clone consumer runs missing')
    for result,active in zip(proof['runs'],['One','Two']):
        need(result['active']==active and len(result['raw'])==len(result['independent_expected'])==33,'Clone result shape changed')
        expected=[];limits=[]
        for name,radius,phase,plane in [('One',7000,0,0),('Two',9000,.4,1),('Three',11000,-.3,2),('Witness',13000,.8,0)]:
            seconds=60 if name==active else 0;omega=math.sqrt(398600.4415/radius**3);a=phase+seconds*omega
            x=radius*math.cos(a);y=radius*math.sin(a);vx=-radius*omega*math.sin(a);vy=radius*omega*math.cos(a)
            state=[x,y,0,vx,vy,0]if plane==0 else [0,x,y,0,vx,vy]if plane==1 else[x,0,y,vx,0,vy]
            expected.extend([21545+seconds/86400,seconds,*state]);limits.extend([1e-10,3e-5,2e-5,2e-5,2e-5,3e-8,3e-8,3e-8])
        expected.append(41);limits.append(0)
        need(all(math.isfinite(x)and abs(x-y)<=d for x,y,d in zip(result['raw'],expected,limits)),'Clone result differs from independent circular orbit')
        need(all(abs(x-y)<1e-8 for x,y in zip(result['independent_expected'],expected)),'Frozen expected values do not match independent orbit')

def audit(stage):
    stage=Path(stage);versions={}
    for version in ['before-v1','before-v2','diagnostic-v1','controlled-v1','controlled-v2']:
        ids=load(stage/('source-sha256-'+version+'.json'));build=load(stage/('build-'+version+'.json'))
        need(build['status']=='passed' and build['inputs_unchanged'] and build['returncode']==0 and build['source_sha256']==ids,'Formation clone build guard failed')
        need(sha(stage/('build-'+version+'.log'))==build['log_sha256'],'Formation clone build log changed')
        with tarfile.open(stage/('sources-'+version+'.tar.gz'))as archive:
            need(set(archive.getnames())==set(ids),'Clone source archive incomplete')
            for name,h in ids.items():need(hashlib.sha256(archive.extractfile(name).read()).hexdigest()==h,'Clone archived source changed')
        versions[version]=build
    baseline=versions['before-v1']
    for version,build in versions.items():
        need(all(build['binary_sha256'].get(n)==h for n,h in baseline['binary_sha256'].items()if Path(n).name.startswith('lib')),'Native libraries changed during clone witness development')
        changed={n for n,h in baseline['source_sha256'].items()if build['source_sha256'][n]!=h}
        need(changed <= {'src/qtgui/CMakeLists.txt','src/qtgui/tests/FormationCloneScopeTests.cpp'},'Production code changed during clone witness development')
    groups=[('headless','before-v2','all',False),('headless','diagnostic-v1','shared',False),
            ('headless','controlled-v1','shared',False),('headless','controlled-v2','shared',True),
            ('visible','controlled-v2','all',True)]
    found=set();passed=failed=missions=0
    for profile,version,selection,complete in groups:
        key='gui-'+profile+'-'+version+'-'+selection;root=stage/key;receipt=load(stage/(key+'-receipt.json'));batch=load(root/'results.json')
        need(receipt['inputs_unchanged'] and receipt['returncode']==(0 if complete else 1),'Clone historical run result changed')
        need(all(receipt['source_sha256'].get(n)==h for n,h in versions[version]['source_sha256'].items()),'Clone execution source differs from build')
        for name,h in versions[version]['binary_sha256'].items():
            if Path(name).name=='GmatQt':continue
            matches=[p for p,v in receipt['image_sha256'].items()if v==h and Path(p).parent==Path(name).parent and
                     (p==name or Path(p).name.startswith(Path(name).name+'.'))]
            need(len(matches)==1,'Clone execution image differs from build')
        need(batch['end_reason']=='completed-selection' and batch['mode']==profile,'Clone selection/profile incomplete')
        rows=[x for x in batch['results']if x['status']!='not-run']
        need(len(rows)==(2 if selection=='all'else 1),'Clone selected-case count changed')
        for row in rows:
            run=root/row['name'];need(row==load(run/'result.json'),'Clone case/aggregate result mismatch')
            mode=row['name'].removeprefix('QtGui.NativeFormationClone')
            if row['status']!='passed':
                need(mode=='Shared' and row['status']=='failed' and row['returncode']==8,'Unexpected clone failure')
                message='Cloned Formation independent member edit failed'if version=='controlled-v1'else'Cloning changed the original Formation or spacecraft'
                need(message in (run/'test.log').read_text(),'Original clone failure lost');failed+=1;continue
            need(row['returncode']==0 and row['requested_visibility']==profile,'Clone successful profile differs')
            if profile=='visible':need(row['trace']['native_exposure_events']>0,'No visible clone exposure')
            proof=load(run/'proof.json');check_proof(proof);need(proof['mode']==mode and proof['native_platform']=='xcb','Clone platform/mode differs')
            emitted=[json.loads(x.split('NASA_CHECK ',1)[1])for x in (run/'test.log').read_text().splitlines()if'NASA_CHECK 'in x]
            need(emitted==[proof],'Clone proof differs from actual output')
            for before in run.glob('preservation-*-before.txt'):
                need(before.read_bytes()==before.with_name(before.name.replace('-before','-after')).read_bytes(),'Original changed in accepted clone case')
            if mode=='Shared':
                need((run/'cloned.script').read_bytes()==(run/'expected-cloned.script').read_bytes(),'Cloned source differs')
                for result in proof['runs']:
                    raw=[float(x)for x in (run/(result['phase']+'.csv')).read_text().strip().split(',')]
                    need(raw==result['raw'],'Clone raw CSV changed')
                    script=(run/(result['phase']+'.script')).read_text()
                    need('P(Twin)'in script,'Clone consumer did not propagate the copy')
                missions+=len(proof['runs'])
            else:need('already belongs to formation'in(run/'exclusive-refusal.txt').read_text(),'Exclusive sharing diagnostic missing')
            found.add((profile,mode));passed+=1
    need(found=={(p,m)for p in ['headless','visible']for m in ['Shared','Exclusive']}and passed==4 and failed==3 and missions==4,'Clone evidence conjunction incomplete')
    return dict(status='passed',successful_GUI_cases=passed,retained_failed_witness_cases=failed,independent_native_missions=missions,
      shared_copy_and_independent_member_edit=True,exclusive_and_one_sided_sharing_rejected=True,
      original_preserved=True,production_changes=0,original_assertions_changed=0,new_requirement_credit=0,
      whole_row_qualified=False,source_versioned_only=True,native_tests_run_by_replay=0)
