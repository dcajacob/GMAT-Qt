"""Audit plotted positions, velocities and line continuity at a same-epoch assignment."""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
from xml.etree import ElementTree
from AuditPlotStepEphemerisEvidence import check, digest, document


CASES = {'ResetCollected', 'ResetSkipped', 'ResetPeer'}


def check_case(proof, counts):
    name = proof['case']
    check(name in CASES and proof['skip']==1 and proof['outcome']=='passed', 'Unexpected reset case')
    curves = proof['outputs']
    check([(c['plot'],c['object']) for c in curves]==[(p,o) for p in ('Orb','Map') for o in ('One','Two')], 'Curve inventory differs')
    for curve in curves:
        peer = curve['object']=='Two'
        if peer:
            grid = [0,20,40] if name=='ResetPeer' else []
            restart = None
        else:
            grid = [0,10,30] if name=='ResetSkipped' else [0,20,20,40]
            restart = 1 if name=='ResetSkipped' else 2
        points = curve['points']
        check(len(points)==len(grid),'Point count differs')
        for i,(point,t) in enumerate(zip(points,grid)):
            check(abs(point['elapsed']-t)<1e-4, 'Epoch differs')
            check(point['connect'] is (i>0 and i!=restart), 'Curve connection differs')
            counts['epochs_and_connections'] += 1
            radius = 2000 if peer else 1500
            rate = math.sqrt(1000/radius**3)
            advanced = t+2000 if restart is not None and i>=restart else t
            angle = (.9 if peer else .7)+advanced*rate
            c,s = math.cos(angle),math.sin(angle)
            tilt = math.sqrt(3)/2
            if curve['plot']=='Orb':
                xyz = [radius*c,radius*s*tilt,radius*s/2]
                velocity = [-radius*rate*s,radius*rate*c*tilt,radius*rate*c/2]
                check(all(abs(point[k]-w)<4e-5 for k,w in zip(('x','y','z'),xyz)), 'Cartesian position differs')
                check(len(point['velocity'])==3 and all(abs(v-w)<5e-8 for v,w in zip(point['velocity'],velocity)), 'Cartesian velocity differs')
                counts['cartesian_components'] += 6
            else:
                lon = math.degrees(math.atan2(s*tilt,c))
                lat = math.degrees(math.asin(s/2))
                check(abs(point['x']-lon)<2e-6 and abs(point['y']-lat)<2e-6,'Geographic position differs')
                counts['geographic_components'] += 2


def invocation(path, mode, test, counts):
    case = path/test
    receipt = document(case/'result.json')
    check(receipt['status']=='passed' and receipt['returncode']==0 and receipt['requested_visibility']==mode, 'Native mode/status differs')
    junit = ElementTree.parse(case/'ctest.xml').findall('.//testcase')
    check(len(junit)==1 and junit[0].get('name')==test and all(junit[0].find(tag) is None for tag in ('error','failure','skipped')),'JUnit differs')
    check(not receipt['trace']['screenshots'], 'Unexpected screenshot')
    result = document(path/'results.json')
    if mode=='visible':
        display=result['visible_display']
        check(display['resizable'] and display['start_minimized'] and 'Iconic' in display['initial_host_window_state'] and 'FULLSCREEN' not in display['initial_host_window_state'],'Host window differs')
        check(receipt['trace']['recorded_platforms']==['xcb'] and receipt['trace']['native_exposure_events']>0,'No visible exposure')
        counts['native_exposures']+=receipt['trace']['native_exposure_events']
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    m=document(args.manifest)
    check(digest(Path(__file__))==m['auditor_sha256'],'Auditor differs')
    for path,sha in m['artifacts'].items():check(digest(Path(path))==sha,'Artifact differs '+path)
    counts=Counter()
    coverage=set()
    for run in m['runs']:
        root=Path(run['directory']);case=root/'QtGui.PlotIntegrationSteps';mode=run['mode']
        result=invocation(root,mode,case.name,counts)
        source=args.manifest.parent/run['version']
        for path,sha in document(source/'hashes.json').items():
            check(digest(source/path)==sha==result['source_hashes'][path], 'Executed source differs '+path)
        proof=document(case/'proof.json')
        logged=[json.loads(line.split('NASA_CHECK ',1)[1]) for line in (case/'test.log').read_text().splitlines() if 'NASA_CHECK ' in line]
        check(proof['cases']==logged and proof['new_requirement_credit']==0 and [p['case'] for p in logged]==run['cases'],'Case inventory differs')
        for p in logged:
            name=p['case'];check(document(case/(name+'-proof.json'))==p and document(case/(name+'-points.json'))==p['outputs'],'Raw curve/proof differs')
            if name=='LiveSolverCurrent':
                original=document(Path(m['prior_solver_proof'][mode]))
                for k in ('outputs','solver_states','trial_passes','accepted_passes','live_membership'):
                    check(p[k]==original[k],'Affected solver output differs '+k)
                counts['matched_solver_runs']+=1
            else:
                check_case(p, counts)
                if run['version']=='source-v4':coverage.add((mode,name))
    check(coverage=={(mode,name) for mode in ('visible','headless') for name in CASES},'Final cases missing')
    invocation(Path(m['availability_run']), 'headless','QtGui.OrbitDataAvailability',counts)
    ledger=document(Path(m['baseline']))
    check(ledger['coverage']['requirements']['verified']==1009 and ledger['coverage']['requirements']['total']==1158 and ledger['remaining_counts']['requirements']==149,'Canonical ledger differs')
    out=dict(bounded_evidence_status='passed',new_requirement_credit=0,checked=dict(counts),manifest_sha256=digest(args.manifest))
    args.output.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))


if __name__=='__main__':main()
