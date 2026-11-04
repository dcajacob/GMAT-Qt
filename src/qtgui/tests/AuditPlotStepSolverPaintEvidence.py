"""Check the saved solver arcs, framebuffer counterfactuals and native invocations."""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
import re
from AuditPlotStepEphemerisEvidence import check, digest, document
from AuditPlotStepResetEvidence import invocation

from AuditPlotStepRedrawEvidence import CASES as REGULAR_CASES, check_pixels

FAMILIES = ('Solver', 'Optimizer', 'Nested', 'Mixed')
MODES = ('All', 'Current', 'None')
CASES = {'Paint'+family+mode for family in FAMILIES for mode in MODES}


def verify_phase(case, proof, phase, counts):
    name = proof['case']
    rows = document(case/(name+'-'+phase+'-pixels.json'))
    key = 'solver_flush_pixels' if phase=='flush' else 'redraw_'+phase
    check(rows==proof[key] and [row['plot'] for row in rows]==['Orb','Map'], 'Framebuffer inventory differs')
    for row in rows:
        plot=row['plot']
        if phase=='final':
            epochs=[1000,1200,1400,1600,1800,2000]
            trials=[False]*6
            links=[False]+[True]*5
        elif name.endswith('None') or (name.endswith('Current') and phase=='live' and plot=='Orb'):
            epochs=[0,200,400];trials=[False]*3;links=[False,True,True]
        else:
            epochs=[200,400,600,800,1000,1200]
            trials=[False]*2+[True]*4;links=[False,True,False,True,True,True]
        arc=document(case/(name+'-'+phase+'-'+plot+'-arcs.json'))
        check(len(arc)==len(epochs)==row['retained'], 'Arc length differs')
        for point, epoch, trial, link in zip(arc,epochs,trials,links):
            check(abs(point['seconds']-epoch)<1e-4 and point['trial'] is trial and point['connect'] is link, 'Solver arc differs')
            counts['epoch_trial_connections']+=1
        wanted=len(arc)-1 if phase=='final' else 2
        check(row['phase']==phase and row['expected_segments']==wanted and row['width']>=200 and row['height']>=140, 'Bad phase, viewport or redraw window')
        limits=sorted({0,wanted-1,wanted,min(len(arc)-1,wanted+1),len(arc)-1})
        check([a['segments'] for a in row['alternatives']]==limits, 'Counterfactual inventory differs')
        for a in row['alternatives']:
            same=a['segments']==wanted
            check(re.fullmatch('[0-9a-f]{64}',a['reference_sha256']) and (a['reference_sha256']==row['actual_sha256'])==same,'Frame hash differs')
            check(a['different_pixels']==0 if same else 5<=a['different_pixels']<=row['width']*row['height'],'Pixel count differs')
            counts['matching_frames' if same else 'rejected_alternatives']+=1


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args();manifest=document(args.manifest)
    check(digest(Path(__file__))==manifest['auditor_sha256'],'Auditor changed')
    for path, sha in manifest['artifacts'].items():
        check(digest(Path(path))==sha, 'Artifact changed '+path)
    seen=set();regressions=set();counts=Counter()
    for run in manifest['runs']:
        root=Path(run['directory']);mode=run['mode'];case=root/'QtGui.PlotIntegrationSteps'
        result=invocation(root,mode,case.name,counts)
        version=args.manifest.parent/run['version']
        for path,sha in document(version/'hashes.json').items():
            check(digest(version/path)==sha==result['source_hashes'][path], 'Executed source differs '+path)
        registered=document(case/'proof.json')
        logged=[json.loads(line.split('NASA_CHECK ',1)[1]) for line in (case/'test.log').read_text().splitlines() if 'NASA_CHECK ' in line]
        check(registered['new_requirement_credit']==0 and registered['cases']==logged, 'Logged/native proof mismatch')
        for p in logged:
            name=p['case']
            if name in REGULAR_CASES:
                previous=document(Path(manifest['regular_baselines'][mode])/(name+'-proof.json'))
                check(p==document(case/(name+'-proof.json')) and p['outputs']==previous['outputs'], 'Regular curve regression differs')
                for phase in ('live','final'):
                    for row in p['redraw_'+phase]:
                        check_pixels(row,name,phase,Counter())
                check((name,mode) not in regressions,'Repeated regular regression')
                regressions.add((name,mode))
                continue
            check(p==document(case/(name+'-proof.json')) and p['outcome']=='passed' and name in CASES and (name,mode) not in seen, 'Case identity differs')
            seen.add((name,mode))
            check(p['trial_passes']>=2 and p['accepted_passes']==1 and len(p['solver_states'])==p['trial_passes']+1, 'Missing native solver states')
            check(p['solver_states']==document(case/(name+'-solver-states.json')) and p['skip']==1 and p['retention_limit']==6, 'Sampling or solver evidence differs')
            width=2 if name.startswith(('PaintNested','PaintMixed')) else 1
            states=[v if isinstance(v,list) else [v] for v in p['solver_states']]
            check(all(len(v)==width for v in states) and all(x==10014 for x in states[-1])
                  and all(any(x!=10014 for x in v) for v in states[:-1]),'Native solver pass classification differs')
            for phase in ['live','final']+(['flush'] if name.endswith('Current') else []):
                verify_phase(case,p,phase,counts)
            curves=document(case/(name+'-points.json'))
            check(curves==p['outputs'] and [(c['plot'],c['object']) for c in curves]==[(v,o) for v in ('Orb','Map') for o in ('One','Two')], 'Final curve inventory differs')
            for c in curves:
                grid=[1000,1200,1400,1600,1800,2000] if c['object']=='One' else []
                check(len(c['points'])==len(grid),'Final count differs')
                for pt,t in zip(c['points'],grid):
                    xyz=[pt[k] for k in ('x','y','z')]
                    check(abs(pt['elapsed']-t)<1e-4 and all(map(math.isfinite,xyz)), 'Final epoch differs')
                    if c['plot']=='Orb':
                        a=t*math.sqrt(398600.4415/7000**3)
                        check(all(abs(x-y)<4e-5 for x,y in zip(xyz,[7000*math.cos(a),7000*math.sin(a),0])), 'Cartesian orbit differs')
    check(seen=={(name,mode) for name in CASES for mode in ('headless','visible')},'Incomplete two-mode solver matrix')
    check(regressions=={(n,m) for n in REGULAR_CASES for m in ('headless','visible')},'Missing regular regressions')
    counts['matched_regular_regressions']=len(regressions)
    ledger=document(Path(manifest['baseline']))
    check(ledger['coverage']['requirements']['verified']==1009 and ledger['coverage']['requirements']['total']==1158 and ledger['remaining_counts']['requirements']==149,'Canonical baseline differs')
    out=dict(bounded_evidence_status='passed',manifest_sha256=digest(args.manifest),new_requirement_credit=0,checked=dict(counts))
    args.output.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))


if __name__=='__main__':
    main()
