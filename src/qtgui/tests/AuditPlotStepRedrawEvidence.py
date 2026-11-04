"""Verify the recorded framebuffer comparison results and retained trajectory grids."""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
import re
from AuditPlotStepEphemerisEvidence import check, digest, document
from AuditPlotStepResetEvidence import invocation


CASES=['RedrawLastTwo','RedrawAll','RedrawRetained']


def check_pixels(row, name, phase, counts):
    total=4 if name=='RedrawRetained' else 11
    wanted=total-1 if phase=='final' or name=='RedrawAll' else 2
    check(row['phase']==phase and row['expected_segments']==wanted and row['retained']==total, 'Expected segment count differs')
    check(row['width']>=200 and row['height']>=140 and re.fullmatch(r'[0-9a-f]{64}',row['actual_sha256']), 'Canvas identity differs')
    expected=sorted({0,wanted-1,wanted,min(total-1,wanted+1),total-1})
    check([a['segments'] for a in row['alternatives']]==expected,'Reference inventory differs')
    for a in row['alternatives']:
        equal=a['segments']==wanted
        check(re.fullmatch(r'[0-9a-f]{64}',a['reference_sha256']) and (a['reference_sha256']==row['actual_sha256'])==equal, 'Reference framebuffer hash differs')
        delta=a['different_pixels']
        check(delta==0 if equal else 5<=delta<=row['width']*row['height'],'Pixel comparison differs')
        counts['matching_frames' if equal else 'rejected_alternative_frames']+=1


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    a=parser.parse_args();m=document(a.manifest)
    check(m['auditor_sha256']==digest(Path(__file__)),'Auditor changed')
    for name,sha in m['artifacts'].items():check(digest(Path(name))==sha,'Artifact changed '+name)
    counts=Counter();seen=set();source=a.manifest.parent/'source-v1'
    for run in m['runs']:
        root=Path(run['directory']);mode=run['mode'];case=root/'QtGui.PlotIntegrationSteps'
        result=invocation(root,mode,case.name,counts)
        for path,sha in document(source/'hashes.json').items():check(sha==digest(source/path)==result['source_hashes'][path],'Executed source differs')
        logged=[json.loads(line.split('NASA_CHECK ',1)[1]) for line in (case/'test.log').read_text().splitlines() if 'NASA_CHECK ' in line]
        proof=document(case/'proof.json')
        check(proof['cases']==logged and proof['new_requirement_credit']==0 and [v['case'] for v in logged]==CASES,'Case inventory differs')
        for p in logged:
            name=p['case'];total=4 if name=='RedrawRetained' else 11
            check(p['skip']==9 and p['outcome']=='passed' and document(case/(name+'-proof.json'))==p,'Native proof differs')
            curves=document(case/(name+'-points.json'))
            check(curves==p['outputs'] and [(c['plot'],c['object']) for c in curves]==[(v,o) for v in ('Orb','Map') for o in ('One','Two')],'Final curve inventory differs')
            for c in curves:
                times=list(range(100*(11-total),1001,100)) if c['object']=='One' else []
                check(len(times)==len(c['points']),'Retained point count differs')
                for t,pt in zip(times,c['points']):
                    check(abs(pt['elapsed']-t)<1e-4 and all(math.isfinite(pt[k]) for k in ('x','y','z')),'Final plot epoch differs')
                    if c['plot']=='Orb':
                        angle=t*math.sqrt(398600.4415/7000**3)
                        check(all(abs(pt[k]-v)<4e-5 for k,v in zip(('x','y','z'),(7000*math.cos(angle),7000*math.sin(angle),0))),'Circular trajectory differs')
                    counts['plot_epochs']+=1
            for phase in ('live','final'):
                rows=document(case/(name+'-'+phase+'-pixels.json'))
                check(rows==p['redraw_'+phase] and [r['plot'] for r in rows]==['Orb','Map'],'Saved display witness differs')
                for row in rows:check_pixels(row,name,phase,counts)
            seen.add((mode,name))
    check(seen=={(mode,key) for mode in ('headless','visible') for key in CASES},'Modes missing')
    ledger=document(Path(m['baseline']))
    check(ledger['coverage']['requirements']['verified']==1009 and ledger['coverage']['requirements']['total']==1158 and ledger['remaining_counts']['requirements']==149,'Canonical ledger differs')
    out=dict(bounded_evidence_status='passed',manifest_sha256=digest(a.manifest),new_requirement_credit=0,checked=dict(counts))
    a.output.write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out))


if __name__=='__main__':main()
