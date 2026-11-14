"""Audit finite-burn plot samples against prescribed thrust and mass-flow solutions.

This adds bounded evidence to the preceding sealed consumer campaign. It does
not admit any NASA row or claim full runtime-image/core qualification.
"""
import argparse
import gzip
import hashlib
import json
import math
from decimal import Decimal, localcontext
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
    counts = dict(epochs=0, cartesian_values=0, velocity_values=0, geographic_values=0, report_values=0, native_exposures=0)
    def truth(key,time,measured_stop=None):
        # High precision closed-form integration, independent of the native
        # journal, sampled output and C++ long-double oracle implementation.
        with localcontext() as context:
            context.prec=60
            D=Decimal;t=D(str(time));electric='Electric' in key
            force=D('0.5') if electric else D(100);isp=D(2500 if electric else 300)
            decrement=not key.endswith('Fixed');restart=key.endswith('Restart')
            mass=D(1000);x=D(7000);vx=D(0);stop=(D(str(measured_stop)) if measured_stop is not None else D('37.123')) if key.endswith('Nonlinear') else D(37)
            bounds=[D(0),D(10),D(27),D(50)] if restart else [D(0),stop,stop+13]
            for i,(lo,hi) in enumerate(zip(bounds,bounds[1:])):
                dt=max(D(0),min(t,hi)-lo)
                if not dt:continue
                firing=i!=1 if restart else i==0
                x+=vx*dt
                if firing:
                    if decrement:
                        rate=force/(isp*10);endmass=mass-rate*dt
                        ratio=(endmass/mass).ln();exhaust=isp/100
                        x+=exhaust*(dt+endmass/rate*ratio);vx-=exhaust*ratio;mass=endmass
                    else:
                        acceleration=force/(mass*1000)
                        x+=acceleration*dt*dt/2;vx+=acceleration*dt
            return [float(v) for v in (x,D(300)+t,D(500)+D('.2')*t,vx,D(1),D('.2'),mass-900,mass)]
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
            if key in ('NonlinearABM','NonlinearABMQuarter'):
                stop=37.123 if key=='NonlinearABM' else 37.25
                grid=[0,*range(2,37,2),stop]
                require(p['skip']==1 and p['burn_report']==[] and p['solver_states']==[], 'Affected ABM context differs')
                require([(v['plot'],v['object']) for v in p['outputs']]==[('Orb','One'),('Orb','Two'),('Map','One'),('Map','Two')], 'Affected ABM curve inventory differs')
                for curve in p['outputs']:
                    expected=grid if curve['object']=='One' else []
                    require(len(curve['points'])==len(expected), 'Affected ABM sample count differs')
                    for point,t in zip(curve['points'],expected):
                        require(abs(point['elapsed']-t)<1e-4, 'Affected ABM literal epoch differs');counts['epochs']+=1
                        require(all(math.isfinite(point[k]) for k in ('x','y','z')), 'Affected ABM coordinate nonfinite')
                        if curve['plot']=='Orb':
                            angle=math.sqrt(398600.4415/7000.**3)*t
                            xyz=(7000*math.cos(angle),7000*math.sin(angle),0)
                            require(all(abs(point[k]-v)<4e-5 for k,v in zip(('x','y','z'),xyz)), 'Affected ABM circle differs');counts['cartesian_values']+=3
                continue
            require(key in manifest['expected_cases'] and p['skip']==(0 if key.endswith('Zero') else 1), 'Unexpected burn case/selection')
            require(p['solver_states']==[] and not p['live_membership'] and p['retention_limit']==1000, 'Unexpected solver/retention context')
            stop=37.123 if key.endswith('Nonlinear') else 37
            times=[0,10,27,50] if key.endswith('Restart') else [0,10,stop,stop+13]
            raw=bound(case/(key+'-report.csv')).read_text().strip().splitlines()
            rows=[[float(v) for v in line.split(',')] for line in raw]
            require(rows==p['burn_report'] and len(rows)==4,'Raw burn report/proof mismatch')
            measured_stop=rows[2][0] if key.endswith('Nonlinear') else None
            for row,t in zip(rows,times):
                expected=[t,*truth(key,row[0] if measured_stop is not None else t,measured_stop)]
                require(len(row)==9,'Burn report schema mismatch')
                for i,(value,wanted) in enumerate(zip(row,expected)):
                    tolerance=1e-4 if i==0 else 2e-6 if i<=3 else 2e-9 if i<=6 else 1e-7
                    require(math.isfinite(value) and abs(value-wanted)<tolerance,'Independent thrust/mass-flow report mismatch')
                    counts['report_values']+=1
            if measured_stop is not None:
                diagnostic=load(case/(key+'-stop-diagnostics.json'))
                require(diagnostic['burn_report']==rows and diagnostic['outputs']==p['outputs'], 'Nonlinear diagnostics differ from proof')
                target=truth(key,37.123)[0]
                require(abs(diagnostic['target_x']-target)<2e-12 and diagnostic['requested_stop_tolerance']==1e-10, 'Nonlinear target/tolerance changed')
                require(abs(rows[2][1]-target)<=1e-10 and abs(diagnostic['position_stop_residual']-(rows[2][1]-diagnostic['target_x']))<1e-15, 'Nonlinear stop missed requested tolerance')
                require('outside of the stopping tolerance' not in bound(case/(key+'-messages.txt')).read_text(), 'Native stop warning remains')
            grid=[0,10,20,30,37,47,50] if key.endswith('Zero') else [0,20,stop,stop+13]
            require([(v['plot'],v['object']) for v in p['outputs']]==[('Orb','One'),('Orb','Two'),('Map','One'),('Map','Two')],'Final curve inventory differs')
            for curve in p['outputs']:
                expected=grid if curve['object']=='One' else []
                require(len(curve['points'])==len(expected),'Burn sampled count differs from literal primitive-step grid')
                for point,t in zip(curve['points'],expected):
                    require(abs(point['elapsed']-t)<1e-4,'Burn sampled epoch differs');counts['epochs']+=1
                    require(all(math.isfinite(point[k]) for k in ('x','y','z')),'Nonfinite burn plot value')
                    state=truth(key,point['elapsed'] if measured_stop is not None else t,measured_stop)
                    if curve['plot']=='Orb':
                        require(all(abs(point[k]-v)<2e-6 for k,v in zip(('x','y','z'),state[:3])),'Independent burn position mismatch');counts['cartesian_values']+=3
                        require(len(point['velocity'])==3 and all(math.isfinite(v) and abs(v-wanted)<2e-9 for v,wanted in zip(point['velocity'],state[3:6])),'Independent burn velocity mismatch');counts['velocity_values']+=3
                    else:
                        lon=math.degrees(math.atan2(state[1],state[0]));lat=math.degrees(math.atan2(state[2],math.hypot(state[0],state[1])))
                        require(abs(point['x']-lon)<2e-6 and abs(point['y']-lat)<2e-6,'Independent burn geographic mismatch');counts['geographic_values']+=2
    require(seen['headless']==seen['visible']==set(manifest['expected_cases']), 'Visible/unattended matrix incomplete')
    output={'bounded_evidence_status':'passed','new_requirement_credit':0,'requirements':{'verified':1009,'total':1158,'remaining':149,'needed_for_95_percent':92},'checked':counts,'runs':manifest['runs'],'manifest_sha256':sha(repo/a.manifest),'full_current_build_qualified':False,'full_NASA_qualified':False,'open_gates':manifest['open_gates']}
    a.output.write_text(json.dumps(output,indent=2)+'\n');print(json.dumps({'status':'passed',**counts,'new_requirement_credit':0}))

if __name__=='__main__':main()
