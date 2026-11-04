#!/usr/bin/env python3
"""Read solver acceptance evidence without weakening endpoint comparisons."""
import argparse
import json
import re
from pathlib import Path

NUMBER=r'[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?'
GOAL=re.compile(r'([A-Za-z]\w*(?:\.\w+)+)\s+Desired:\s*('+NUMBER+r')\s+Achieved:\s*('+NUMBER+r')\s+Variance:\s*('+NUMBER+r')')

def analyze(text,tolerances):
    accepted=[]
    # Perturbed passes are not convergence evidence. Use the last nominal goal
    # values immediately before each explicit target-converged message.
    for block in text.split('*** The Targeter converged!')[:-1]:
        final={m[1]:{'goal':m[1],'desired':float(m[2]),'achieved':float(m[3]),'residual':abs(float(m[4]))} for m in GOAL.finditer(block)}
        for name,row in final.items():
            row['tolerance']=tolerances.get(name)
            row['within_tolerance']=row['tolerance'] is not None and row['residual']<=row['tolerance']
        accepted.append(list(final.values()))
    return {'converged_targets':len(accepted),'accepted_goals':accepted,
            'all_accepted_goals_within_tolerance':bool(accepted) and all(rows and all(r['within_tolerance'] for r in rows) for rows in accepted),
            'note':'Goal acceptance is separate from equality with the reference endpoint; this does not change endpoint tolerances.'}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case',type=Path);parser.add_argument('--tolerances',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();table=json.loads(args.tolerances.read_text());messages=[]
    for p in sorted(args.case.glob('stage-*/isolated-*/state/messages.txt')):
        text=p.read_text();text=text.rsplit('Running mission...',1)[-1];messages.append(text)
    result=analyze('\n'.join(messages),table);args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(result['converged_targets'],result['all_accepted_goals_within_tolerance'])

if __name__=='__main__':main()
