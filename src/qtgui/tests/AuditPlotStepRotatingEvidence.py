"""Audit rotating-frame samples against independent geometric position derivatives."""
import argparse,json,math
from xml.etree import ElementTree
from collections import Counter
from pathlib import Path
from AuditPlotStepEphemerisEvidence import check,digest,document
from AuditPlotStepResetEvidence import invocation
CASES=['RotatingOriginForward','RotatingOriginBackward','RotatingEarthForward','RotatingEarthBackward','RotatingVelocityForward','RotatingVelocityBackward','RotatingPairForward','RotatingPairBackward','RotatingPairThinned']
def cart(n,t):
 r,p=(7000,0) if n==0 else (9000,.4);w=math.sqrt(398600.4415/r**3);a=p+w*t
 x,y,vx,vy=r*math.cos(a),r*math.sin(a),-r*w*math.sin(a),r*w*math.cos(a)
 return (x,y,0,vx,vy,0) if n==0 else (0,x,y,0,vx,vy)
def sub(a,b):return [x-y for x,y in zip(a,b)]
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def unit(a):return [x/math.sqrt(dot(a,a)) for x in a]
def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def position(name,n,t):
 one,two=cart(0,t),cart(1,t);rel=sub(two,one) if 'Pair' in name else two
 r,v=rel[:3],rel[3:];z=unit(cross(r,v));x=unit(v if 'Velocity' in name else r);y=cross(z,x)
 p=cart(n,t)[:3]
 if 'Earth' not in name:p=sub(p,two[:3])
 return [dot(axis,p) for axis in (x,y,z)]
def state(name,n,t):
 h=.25;lo2,lo,hi,hi2=[position(name,n,t+k*h) for k in (-2,-1,1,2)]
 return position(name,n,t)+[(a-8*b+8*c-d)/(12*h) for a,b,c,d in zip(lo2,lo,hi,hi2)]
def check_case(p,counts):
 name=p['case'];check(name in CASES and p['outcome']=='passed','Unexpected rotating case')
 thin='Thinned' in name;pair='Pair' in name;sign=-1 if 'Backward' in name else 1
 check(p['skip']==(3 if thin else 0),'Incorrect cadence')
 check([(x['plot'],x['object']) for x in p['outputs']]==[(v,o) for v in ('Orb','Map') for o in ('One','Two')],'Wrong curve inventory')
 for curve in p['outputs']:
  n=curve['object']=='Two';orb=curve['plot']=='Orb'
  times=([0,10,20] if n else [0]) if thin else ([0,10,20,23] if not n or (pair and orb) else [0,3,6,9,10,13,16,19,20,23])
  check(len(curve['points'])==len(times),'Wrong point count')
  for i,(point,t) in enumerate(zip(curve['points'],times)):
   t*=sign;connect=(i==3) if pair and orb and n and not thin else i>0
   check(abs(point['elapsed']-t)<1e-4 and point['connect'] is connect,'Wrong epoch/continuity');counts['epochs_and_connections']+=1
   xyz=[point[k] for k in ('x','y','z')];check(all(map(math.isfinite,xyz)),'Nonfinite plot')
   if orb:
    wanted=state(name,n,t);check(all(abs(a-b)<4e-5 for a,b in zip(xyz,wanted)),'Rotating position differs')
    check(len(point['velocity'])==3 and all(abs(a-b)<5e-8 for a,b in zip(point['velocity'],wanted[3:])),'Rotating velocity differs');counts['cartesian_components']+=6
    if 'Earth' not in name and n:check(xyz+point['velocity']==[0]*6,'Self-relative state not exactly zero');counts['self_zero_states']+=1

def main():
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('--manifest',type=Path,required=True);a.add_argument('--output',type=Path,required=True);args=a.parse_args();m=document(args.manifest)
 check(digest(Path(__file__))==m['auditor_sha256'],'Auditor differs')
 for name,h in m['artifacts'].items():check(digest(Path(name))==h,'Artifact differs '+name)
 counts=Counter();seen=set()
 for run in m['runs']:
  root=Path(run['directory']);case=root/'QtGui.PlotIntegrationSteps';mode=run['mode'];result=invocation(root,mode,case.name,counts)
  archive=args.manifest.parent/run['version']
  for name,h in document(archive/'hashes.json').items():check(digest(archive/name)==h==result['source_hashes'][name],'Executed source differs '+name)
  p=document(case/'proof.json');logs=[json.loads(line.split('NASA_CHECK ',1)[1]) for line in (case/'test.log').read_text().splitlines() if 'NASA_CHECK ' in line]
  check(p['cases']==logs and p['new_requirement_credit']==0 and [x['case'] for x in logs]==run['cases'],'Native case inventory differs')
  for p in logs:
   name=p['case'];check(document(case/(name+'-proof.json'))==p and document(case/(name+'-points.json'))==p['outputs'],'Raw data differs')
   if name in CASES:
    if run['version']=='source-v3':
     check(name=='RotatingOriginForward' and p['rotating_context_checks']==['coeval-generic-force-preserved','stale-epoch-force-rejected','same-epoch-changed-state-force-rejected'],'Force input scope checks differ')
     counts['force_scope_checks']+=3
    check_case(p,counts);seen.add((mode,name))
    messages=(case/(name+'-run-messages.txt')).read_text();expected=2 if 'Pair' in name and 'Thinned' not in name else 0
    check(messages.count("has no coherent state for frame 'RotatingFrame'")==expected,'Wrong missing-reference warnings');counts['missing_reference_warnings']+=expected
   else:
    prior=document(Path(m['regressions'][name][mode]));check(p['outputs']==prior['outputs'],'Regression changed '+name);counts['matched_regression_outputs']+=1
 check(seen=={(mode,name) for mode in ('visible','headless') for name in CASES},'Missing rotating matrix')
 tests=ElementTree.parse(m['acceleration_regressions']).findall('.//testcase')
 check({t.get('name') for t in tests}=={'QtGui.ObjectAccelerationDerivativeContract','QtAcceptance.AccelerationContextForce','QtAcceptance.AccelerationContextLifetime','QtAcceptance.AccelerationHandoff'} and all(t.find(tag) is None for t in tests for tag in ('error','failure','skipped')),'Acceleration regression matrix differs')
 counts['acceleration_regressions']=len(tests)
 for failed in m['failed_runs']:
  receipt=document(Path(failed)/'QtGui.PlotIntegrationSteps/result.json');check(receipt['status']=='failed' and receipt['returncode']!=0,'Failed invocation was relabeled')
 counts['failed_invocations_retained']=len(m['failed_runs'])
 ledger=document(Path(m['baseline']));check(ledger['coverage']['requirements']==m['coverage'],'Ledger changed')
 result={'bounded_evidence_status':'passed','new_requirement_credit':0,'checked':dict(counts),'manifest_sha256':digest(args.manifest)}
 args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()
