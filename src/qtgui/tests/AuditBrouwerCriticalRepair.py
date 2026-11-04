"""Audit a bounded critical-longitude repair without awarding whole-row credit."""
import argparse,copy,hashlib,json,math,re,tarfile
from pathlib import Path
import xml.etree.ElementTree as ET
from BuildNasaCoverageExtension import require,sha,load,validate_visible

def independent_cartesian(k):
    a,e,inc,node,peri,nu=k;i,o,w,t=map(math.radians,[inc,node,peri,nu]);p=a*(1-e*e);radius=p/(1+e*math.cos(t));speed=math.sqrt(398600.4418/p)
    P=[math.cos(o)*math.cos(w)-math.sin(o)*math.sin(w)*math.cos(i),math.sin(o)*math.cos(w)+math.cos(o)*math.sin(w)*math.cos(i),math.sin(w)*math.sin(i)]
    Q=[-math.cos(o)*math.sin(w)-math.sin(o)*math.cos(w)*math.cos(i),-math.sin(o)*math.sin(w)+math.cos(o)*math.cos(w)*math.cos(i),math.cos(w)*math.sin(i)]
    return [radius*(P[j]*math.cos(t)+Q[j]*math.sin(t)) for j in range(3)]+[speed*(-P[j]*math.sin(t)+Q[j]*(e+math.cos(t))) for j in range(3)]

def check(proof,case):
    require(proof['outcome']=='passed' and proof['case']==case['key'],'Failed/wrong GUI case')
    original=independent_cartesian(case['keplerian']);rotated=[-original[1],original[0],original[2],-original[4],original[3],original[5]]
    require(all(abs(a-b)<1e-10 for a,b in zip(case['cartesian'],original)) and all(abs(a-b)<1e-10 for a,b in zip(case['rotated_cartesian'],rotated)),'Independent fixture geometry differs')
    require(proof['initial_cartesian']==case['cartesian'] and proof['expected_rotated_cartesian']==case['rotated_cartesian'],'Proof inputs changed')
    require(len(proof['initial_mean_preview'])==6 and proof['new_raan']==math.fmod(proof['initial_mean_preview'][3]+90.,360.),'Wrong actual rotation edit')
    require([v['phase'] for v in proof['phases']]==[0,1],'Missing reopen phase')
    errors=[]
    for phase in proof['phases']:
        require(len(phase['raw'])==6 and all(math.isfinite(v) for v in phase['raw']),'Invalid report')
        error=[abs(a-b) for a,b in zip(phase['raw'],rotated)];require(all(e<=(1e-6 if i<3 else 1e-9) for i,e in enumerate(error)),'Independent rotated state differs');errors.append(error)
    return dict(position_km=max(max(v[:3]) for v in errors),velocity_km_s=max(max(v[3:]) for v in errors))

def audit(repo,stage):
    manifest=load(stage/'evidence-inputs-v1.json')
    for path,digest in manifest['artifact_sha256'].items():require(not Path(path).is_symlink() and sha(path)==digest,'Changed regular artifact '+path)
    def bound(path):path=Path(path);require(str(path) in manifest['artifact_sha256'],'Unbound artifact '+str(path));return path
    before=load(bound(stage/'visible-inputs-before.json'));require(before==load(bound(stage/'visible-inputs-after.json')),'Changed native inputs')
    launch=load(bound(stage/'visible-launch.json'));require(launch['returncode']==0 and launch['inputs_unchanged'],'Visible launch failed')
    with tarfile.open(bound(stage/'executed-sources-v5.tar.gz')) as tar:frozen={m.name:tar.extractfile(m).read() for m in tar.getmembers()}
    require({p:hashlib.sha256(b).hexdigest() for p,b in frozen.items()}==before['sources'],'Executed archive differs')
    preimage=bound(stage/'StateConversionUtil.before.cpp').read_text();old='\t\tdlt1e = 0.0;\n\t\tblghp = 0.0;';new='\t\tdlt1e = 0.0;\n      // Suppress the singular correction, not the mean longitude itself.\n      // Retaining the base angle preserves rotation about the zonal axis.\n\t\tblghp = raandp + aopdp + meanAnom;'
    require(preimage.count(old)==1 and preimage.replace(old,new)==frozen['src/gmatutil/util/StateConversionUtil.cpp'].decode(),'Unexpected numerical changes')
    identities=load(bound(stage/'before-identities.json'));require(identities['src/gmatutil/util/StateConversionUtil.cpp']==hashlib.sha256(preimage.encode()).hexdigest(),'Wrong preimage')
    require(identities['src/qtgui/tests/BrouwerRotationCovarianceTests.cpp']==hashlib.sha256(frozen['src/qtgui/tests/BrouwerRotationCovarianceTests.cpp']).hexdigest(),'Symmetry assertions changed')
    oldtest=ET.parse(bound(stage/'covariance-before-v1.xml')).getroot().findall('testcase');newtest=ET.parse(bound(stage/'covariance-after-v2.xml')).getroot().findall('testcase')
    require(len(oldtest)==len(newtest)==1 and oldtest[0].find('failure') is not None and newtest[0].find('failure') is None,'Before/after regression not established')
    require('BROUWER_ROTATION_RESULT checks=18 failures=6' in oldtest[0].findtext('system-out') and 'BROUWER_ROTATION_RESULT checks=18 failures=0' in newtest[0].findtext('system-out'),'Symmetry check counts differ')
    direct=load(bound(stage/'direct-v2/receipt.json'));previous=load(bound(repo/'build/nasa-coverage/coverage95-20261101/direct-conversion-native-v3/receipt.json'))
    require(previous['proofs'][0]['failed_contexts']==1 and direct['proofs'][0]['failed_contexts']==0 and direct['proofs'][0]['directed_contexts']==238 and direct['proofs'][0]['identity_contexts']==22,'Unchanged direct matrix not repaired')
    for path in ['src/qtgui/tests/NasaDirectOrbitConversionTests.cpp','src/qtgui/tests/NasaDirectOrbitConversionCases.json','src/qtgui/tests/RunDirectOrbitConversionAcceptance.py']:
        require(previous['source_sha256'][path]==direct['source_sha256'][path]==identities[path],'Original direct assertions/reference changed')
    require(direct['status']=='failed' and direct['returncode']==0 and direct['inputs_unchanged'] and direct['failed_independent_mean_reconstructions']==1,'Lost original prototype failure')
    failures=[v for v in load(bound(stage/'direct-v2/independent-reconstruction.json')) if not v['passed']];require(len(failures)==1 and failures[0]['case']=='Elliptic_SphericalAZFPA' and failures[0]['target']=='BrouwerMeanLong','Prototype mismatch scope changed')
    require(any(Path(p).name.startswith('libGmatUtil.') and h==direct['binary_sha256'].get(p) and h!=identities['application/bin/libGmatUtil.so'] for p,h in before['images'].items()),'Fixed runtime image not mapped')
    cases=json.loads(frozen['src/qtgui/tests/NasaBrouwerCriticalRotationCases.json'])['cases'];require([c['keplerian'] for c in cases]==[[9800.,.3,i,88.,18.,101.] for i in [63.2,64.]],'GUI critical inputs differ');selected={'QtGui.NasaBrouwerCritical'+c['key']:c for c in cases}
    mapped=load(bound(stage/'loaded-images.json'))['processes'];summaries={};last=None
    for mode,run in [('headless','brouwer-critical-headless-v5'),('visible','brouwer-critical-visible-v1')]:
        receipt=load(bound(repo/'build/gui-acceptance'/run/'results.json'));require(receipt['mode']==mode and receipt.get('finished_utc') and receipt['end_reason']=='completed-selection','Nonterminal GUI run')
        require(all(before['sources'].get(p)==h for p,h in receipt['source_hashes'].items()),'Wrong GUI source version')
        active={v['name']:v for v in receipt['results'] if v['status']!='not-run'};require(set(active)==set(selected),'Incomplete GUI selection');evidence={}
        for name,row in active.items():
            log=bound(row['log']);root=log.parent;junit=bound(root/'ctest.xml');require(row['status']=='passed' and row['returncode']==0,'GUI case failed')
            tests=ET.parse(junit).getroot().findall('testcase');require(len(tests)==1 and tests[0].attrib['name']==name and tests[0].attrib.get('status')=='run' and tests[0].find('failure') is None and tests[0].find('skipped') is None,'Missing passing JUnit')
            proof=load(bound(root/'proof.json'));matches=re.findall(r'NASA_CHECK (\{[^\n]+\})',log.read_text());require(len(matches)==1 and json.loads(matches[0])==proof,'Proof differs from stdout');summaries[mode+'-'+name]=check(proof,selected[name]);last=(proof,selected[name])
            for phase in proof['phases']:require([float(v) for v in bound(root/f"{phase['phase']}-states.csv").read_text().strip().split(',')]==phase['raw'],'Raw report differs')
            exes=[p for p in row['registered_command'] if p in receipt['binary_hashes']];require(len(exes)==1 and before['images'].get(exes[0])==receipt['binary_hashes'][exes[0]],'GUI executable differs')
            evidence[name]=dict(log=str(log),log_sha256=sha(log),junit=str(junit),junit_sha256=sha(junit))
            if mode=='visible':
                traces=list(root.glob('trace-*.jsonl'));require(traces,'Missing visible trace')
                for trace in traces:
                    process=mapped.get(trace.stem.removeprefix('trace-'));require(process and process['executable']==exes[0],'Missing process mapping')
                    for p,h in before['images'].items():
                        if p==exes[0] or Path(p).name.startswith('libGmat'):require(process['images'].get(p)==h,'Mapped core differs')
        if mode=='visible':validate_visible(receipt,evidence)
    negatives=[]
    for label,mutate in [('wrong-state',lambda p:p['phases'][0]['raw'].__setitem__(0,0)),('missing-reopen',lambda p:p['phases'].pop()),('wrong-node',lambda p:p.update(new_raan=0))]:
        corrupt=copy.deepcopy(last[0]);mutate(corrupt)
        try:check(corrupt,last[1])
        except (ValueError,AssertionError):negatives.append(label)
        else:raise ValueError('Accepted corrupt '+label)
    return dict(status='passed-bounded-repair',qualified_keys=[],symmetry_checks=18,before_symmetry_failures=6,native_direct_contexts=238,native_direct_failures=0,preserved_prototype_failures=1,native_gui_passes=4,raw_gui_states=8,max_errors=summaries,negative_controls=negatives,technical_core_approval=False,scientific_approval=False,full_current_build_qualified=False,full_NASA_qualified=False,limits=['FRR!E13 remains unqualified.','Native critical branch and original MATLAB prototype differ; original failing registration remains enabled.','Rotation symmetry and inverse reconstruction do not establish independent physical averaging.'])

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();result=audit(Path(__file__).resolve().parents[3],a.stage.resolve());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
