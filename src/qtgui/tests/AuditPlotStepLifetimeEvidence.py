"""Recompute both executions of the three Toggle fixtures from sealed receipts."""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
from xml.etree import ElementTree
from AuditPlotStepEphemerisEvidence import check, digest, document


GRIDS = {'LifetimeToggleGap': ([0,20,30,50], [False,True,False,True], 57),
         'LifetimeInitiallyOff': ([10,30], [False,True], 37),
         'LifetimeFinishOff': ([0,20], [False,True], 30)}


def circle(seconds):
    rate = math.sqrt(398600.4415 / 7000**3)
    angle = rate * seconds
    return [7000*math.cos(angle), 7000*math.sin(angle), 0,
            -7000*rate*math.sin(angle), 7000*rate*math.cos(angle), 0]


def check_run(run, name, counts):
    times, links, stop = GRIDS[name]
    report = run['report']
    check(len(report)==7 and abs(report[0]-stop)<1e-4, 'Wrong final mission time')
    check(all(abs(v-w)<tol for v,w,tol in zip(report[1:], circle(stop), [4e-5]*3+[5e-8]*3)), 'Wrong final Cartesian state')
    counts['report_values'] += 7
    check([p['name'] for p in run['plots']]==['Orb','Map'], 'Wrong plot inventory')
    for plot in run['plots']:
        points = plot['points']
        check(plot['end_of_run'] is True and len(points)==len(times), 'End-of-run state or point count differs')
        if plot['name']=='Map':
            check(plot['active'] is (name!='LifetimeFinishOff'), 'Final activation differs')
        for point, time, connected in zip(points,times,links):
            check(abs(point['epoch']-time)<1e-4 and point['connect'] is connected, 'Gap epoch or line connection differs')
            xyz = [point[k] for k in ('x','y','z')]
            check(all(map(math.isfinite,xyz)), 'Nonfinite curve value')
            if plot['name']=='Orb':
                check(all(abs(v-w)<4e-5 for v,w in zip(xyz,circle(time))), 'Orbit differs from independent circle')
                counts['position_values'] += 3
            counts['epochs_and_connections'] += 1


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    manifest=document(args.manifest)
    for name, sha in manifest['artifacts'].items():
        check(digest(Path(name))==sha, 'Changed artifact '+name)
    check(digest(Path(__file__))==manifest['auditor_sha256'], 'Auditor changed')
    archive=args.manifest.parent/'source-v1'
    counts=Counter()
    seen=set()
    for run in manifest['runs']:
        mode=run['mode'];root=Path(run['directory']);case=root/'QtGui.PlotIntegrationSteps'
        receipt=document(case/'result.json'); result=document(root/'results.json')
        check(receipt['status']=='passed' and receipt['returncode']==0 and receipt['requested_visibility']==mode and not receipt['trace']['screenshots'], 'Invocation differs')
        junit=ElementTree.parse(case/'ctest.xml').findall('.//testcase')
        check(len(junit)==1 and junit[0].get('name')=='QtGui.PlotIntegrationSteps' and all(junit[0].find(k) is None for k in ('failure','error','skipped')), 'JUnit differs')
        for name,sha in document(archive/'hashes.json').items():
            check(digest(archive/name)==result['source_hashes'][name]==sha,'Executed source differs')
        if mode=='visible':
            display=result['visible_display']
            check(display['resizable'] and display['start_minimized'] and 'Iconic' in display['initial_host_window_state'] and 'FULLSCREEN' not in display['initial_host_window_state'], 'Window policy differs')
            check(receipt['trace']['recorded_platforms']==['xcb'] and receipt['trace']['native_exposure_events']>0,'Exposure missing')
            counts['native_exposures']+=receipt['trace']['native_exposure_events']
        proof=document(case/'proof.json')
        logs=[json.loads(line.split('NASA_CHECK ',1)[1]) for line in (case/'test.log').read_text().splitlines() if 'NASA_CHECK ' in line]
        check(logs==proof['cases'] and proof['new_requirement_credit']==0 and [p['case'] for p in logs]==list(GRIDS),'Proof/log differs')
        for p in logs:
            name=p['case']; repeated=document(case/(name+'-runs.json'))
            check(p==document(case/(name+'-proof.json')) and p['outputs']==document(case/(name+'-points.json')),'Final proof differs')
            check(p['skip']==1 and repeated==p['lifetime_runs'] and len(repeated)==2 and repeated[0]==repeated[1], 'Repeat changed outputs')
            for measured in repeated:check_run(measured,name,counts)
            seen.add((mode,name))
    check(seen=={(mode,key) for key in GRIDS for mode in ('headless','visible')}, 'Cases missing')
    baseline=document(Path(manifest['baseline']))
    check(baseline['coverage']['requirements']['verified']==1009 and baseline['coverage']['requirements']['total']==1158 and baseline['remaining_counts']['requirements']==149,'Baseline differs')
    report=dict(bounded_evidence_status='passed',new_requirement_credit=0,manifest_sha256=digest(args.manifest),checked=dict(counts))
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
