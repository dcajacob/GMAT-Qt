"""Audit new-mode transient solver membership and regular redraw/retention receipts.

This adds bounded evidence to the preceding sealed consumer campaign. It does
not admit any NASA row or claim full runtime-image/core qualification.
"""
import argparse
import gzip
import hashlib
import json
import math
from pathlib import Path
import xml.etree.ElementTree as ET


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def require(value, why):
    if not value:
        raise ValueError(why)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    ap.add_argument('--manifest', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    a = ap.parse_args(); repo = a.repo.resolve()
    manifest = json.loads((repo/a.manifest).read_text())
    require(manifest['auditor_sha256'] == sha(Path(__file__)), 'Auditor identity changed')
    for path, digest in manifest['artifacts'].items():
        require(sha(repo/path) == digest, 'Artifact changed: '+path)
    def bound(path):
        p = Path(path)
        if not p.is_absolute(): p = repo/p
        if not p.exists() and Path(str(p)+'.gz').exists(): p = Path(str(p)+'.gz')
        require(str(p.relative_to(repo)) in manifest['artifacts'], 'Unbound artifact: '+str(p))
        return p
    def load(path):
        p = bound(path)
        return json.loads(gzip.decompress(p.read_bytes()) if p.suffix == '.gz' else p.read_bytes())
    prior = load(manifest['prior_audit'])
    require(prior['bounded_evidence_status']=='passed' and prior['new_requirement_credit']==0, 'Prior proof not sealed')
    require(prior['manifest_sha256']==sha(bound(manifest['prior_manifest'])), 'Prior audit/manifest mismatch')
    baseline = load(manifest['baseline'])
    require(baseline['coverage']['requirements']['verified']==1009 and baseline['coverage']['requirements']['total']==1158 and baseline['remaining_counts']['requirements']==149, 'Canonical denominator/coverage changed')
    counts = dict(epochs=0, circular_position_values=0, live_observations=0, cadence_samples=0, regular_refreshes=0, native_exposures=0)
    def check_points(plot, actual, expected):
        require(len(actual)==len(expected), 'Membership/retention length differs')
        for p,(seconds,solver) in zip(actual,expected):
            require(abs(p.get('seconds',p.get('elapsed',-1e10))-seconds)<1e-4, 'Independent epoch differs')
            if 'solver' in p: require(p['solver']==solver, 'Trial/accepted classification differs')
            require(all(math.isfinite(p[k]) for k in ('x','y','z')), 'Nonfinite coordinate')
            counts['epochs']+=1
            if plot=='Orb':
                angle=math.sqrt(398600.4415/7000.**3)*seconds
                xyz=(7000*math.cos(angle),7000*math.sin(angle),0)
                require(all(abs(p[k]-v)<4e-5 for k,v in zip(('x','y','z'),xyz)), 'Independent circle differs')
                counts['circular_position_values']+=3
    seen={'headless':set(),'visible':set()}
    for run in manifest['runs']:
        root=repo/run['directory']; case=root/'QtGui.PlotIntegrationSteps'
        result=load(root/'results.json'); receipt=load(case/'result.json')
        require(receipt['status']=='passed' and receipt['returncode']==0 and receipt['requested_visibility']==run['mode'], 'Native invocation failed/mode differs')
        tests=ET.parse(bound(case/'ctest.xml')).getroot().findall('.//testcase')
        require(len(tests)==1 and tests[0].get('name')=='QtGui.PlotIntegrationSteps' and all(tests[0].find(tag) is None for tag in ('failure','error','skipped')), 'JUnit differs')
        require(receipt['trace']['screenshots']==[], 'Screenshots retained')
        if run['mode']=='visible':
            display=result['visible_display']
            require(display['resizable'] and display['start_minimized'] and 'Iconic' in display['initial_host_window_state'] and 'FULLSCREEN' not in display['initial_host_window_state'], 'Window policy differs')
            require(receipt['trace']['recorded_platforms']==['xcb'] and receipt['trace']['native_exposure_events']>0, 'Native exposure absent')
            counts['native_exposures']+=receipt['trace']['native_exposure_events']
        for path,digest in run['sources'].items():
            require(sha(bound(manifest['source_archives'][digest]))==digest, 'Source archive differs')
            if path in result['source_hashes']:require(result['source_hashes'][path]==digest, 'Executed Qt source differs')
        proof=load(case/'proof.json')
        checks=[json.loads(line.split('NASA_CHECK ',1)[1]) for line in bound(case/'test.log').read_text().splitlines() if 'NASA_CHECK ' in line]
        require(checks==proof['cases'] and proof['outcome']=='passed' and proof['new_requirement_credit']==0, 'Proof/log/credit differs')
        require([p['case'] for p in checks]==run['cases'], 'Case inventory differs')
        for p in checks:
            key=p['case'];seen[run['mode']].add(key)
            require(load(case/(key+'-proof.json'))==p and load(case/(key+'-points.json'))==p['outputs'], 'Raw/per-case evidence differs')
            if key=='CadenceRetention':
                data=load(case/(key+'-observations.json'))
                samples=data['samples'];raw=[]
                for sample in samples:
                    step=sample['raw_step'];require(0<=step<=60 and abs(sample['raw_seconds']-10*step)<1e-4, 'Cadence raw time differs')
                    if not raw or raw[-1]!=step:raw.append(step)
                    collected=1+step//2;require(sample['expected_collected']==collected, 'Cadence collection differs')
                    require([m['plot'] for m in sample['models']]==['Orb','Map'], 'Missing cadence consumer')
                    for m in sample['models']:
                        require(m['frame']==collected,'Cadence frame differs')
                        check_points(m['plot'],m['retained'],[(20*i,False) for i in range(max(0,collected-10),collected)])
                    counts['cadence_samples']+=1
                require(raw==list(range(61)), 'Cadence raw grid incomplete')
                for plot,freq in [('Orb',3),('Map',4)]:
                    frames=[r['frame'] for r in data['regular_refreshes'] if r['plot']==plot]
                    require(frames==list(range(freq,32,freq)), 'Independent intermediate redraw grid differs')
                require(all(r['classification']=='regular-publication' and not r['end_of_run'] for r in data['regular_refreshes']), 'Forced refresh incorrectly credited')
                require(all(r['classification'] in ('native-flush','initial-or-end-of-run') for r in data['forced_refreshes']), 'Unclassified forced refresh')
                counts['regular_refreshes']+=len(data['regular_refreshes'])
                require(p['cadence_retention']['collected']==31 and p['cadence_retention']['retained']==10 and p['cadence_retention']['publications']==len(samples) and p['cadence_retention']['regular_refreshes']==17, 'Cadence summary differs')
                final=[(t,False) for t in range(420,601,20)]
            else:
                mode=next(m for m in ('All','Current','None') if key.endswith(m))
                states=p['solver_states'];finished=[all(x==10014 for x in s) if isinstance(s,list) else s==10014 for s in states]
                require(len(states)>=3 and finished.count(True)==1 and finished[-1] and p['accepted_passes']==1 and p['trial_passes']==len(states)-1, 'Solver-state oracle incomplete')
                require(load(case/(key+'-solver-states.json'))==states, 'Solver states differ')
                phases=load(case/(key+'-live-phases.json')); observations=load(case/(key+'-live-membership.json'))
                require(len(phases)==len(states)+2 and phases[0]['phase']=='before' and phases[-1]['phase']=='after', 'Phase inventory differs')
                for i,phase in enumerate(phases):
                    grid=[0,10] if i==0 else [37,47,50] if i==len(phases)-1 else [10,20,30,37]
                    require(phase['raw_seconds']==grid, 'Literal raw solver grid differs')
                    if 0<i<len(phases)-1:require(phase['pass']==i and phase['accepted']==finished[i-1] and phase['phase']=='solver', 'Solver phase/state partition differs')
                for row in observations:
                    phase=row['phase'];at=row['pass'];is_accepted=row['accepted']
                    if phase=='solver':require(1<=at<=len(states) and is_accepted==finished[at-1], 'Live observation solver state differs')
                    else:require(at==0 and is_accepted, 'Regular phase incorrectly marked as trial')
                    for plot in row['plots']:
                        name=plot['plot'];require(name in ('Orb','Map'), 'Unknown live consumer')
                        if phase=='before':possibilities=[[(0,False)]]
                        elif phase=='after':
                            all_points=[(0,False)]+[(t,not accepted) for accepted in finished for t in (20,37)]
                            base=all_points if mode=='All' else [(0,False),(20,False),(37,False)]
                            possibilities=[base,base+[(50,False)]]
                        else:
                            prior_points=[(0,False)]
                            if mode=='All':prior_points += [(t,not accepted) for accepted in finished[:at-1] for t in (20,37)]
                            current=[(t,not is_accepted) for t in (20,37)] if mode!='None' or is_accepted else []
                            possibilities=[prior_points+current[:n] for n in range(len(current)+1)]
                        wanted=[(r['seconds'],r['solver']) for r in plot['expected']]
                        require(wanted in possibilities, 'Live membership not a legal independently derived phase prefix')
                        if row['boundary']=='completed':require(wanted==possibilities[-1], 'Final phase prefix incomplete')
                        check_points(name,plot['actual'],wanted)
                    counts['live_observations']+=1
                require(p['live_membership']['observation_count']==len(observations) and p['live_membership']['phase_count']==len(phases) and p['live_membership']['flushes']>=3, 'Live summary differs')
                # Native source additionally asserts membership at each exact raw
                # publication and flush. Offline audit validates all saved phase
                # prefixes and values; it does not invent an unrecorded timestamp.
                final=([(0,False)]+[(t,not accepted) for accepted in finished for t in (20,37)]+[(50,False)]) if mode=='All' else [(0,False),(20,False),(37,False),(50,False)]
            require([(r['plot'],r['object']) for r in p['outputs']]==[('Orb','One'),('Orb','Two'),('Map','One'),('Map','Two')], 'Final curve inventory differs')
            for curve in p['outputs']:check_points(curve['plot'],curve['points'],final if curve['object']=='One' else [])
    require(seen['headless']==seen['visible']==set(manifest['expected_cases']), 'Visible/unattended matrix incomplete')
    output={'bounded_evidence_status':'passed','new_requirement_credit':0,'requirements':{'verified':1009,'total':1158,'remaining':149,'needed_for_95_percent':92},'checked':counts,'runs':manifest['runs'],'manifest_sha256':sha(repo/a.manifest),'full_current_build_qualified':False,'full_NASA_qualified':False,'open_gates':manifest['open_gates']}
    a.output.write_text(json.dumps(output,indent=2)+'\n');print(json.dumps({'status':'passed',**counts,'new_requirement_credit':0}))

if __name__=='__main__':main()
