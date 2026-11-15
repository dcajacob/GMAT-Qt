"""Add only missing function-context witnesses, retaining original effect oracles."""
from pathlib import Path
import copy,hashlib,json,math,re
ROOT=Path(__file__).resolve().parent
FAMILIES=['ClearPlot','PenUp','PenDown','MarkPoint','CommandEcho','NestedEvent','UpdateDynamicData','Set','Save','While','Optimize','Stop','CallPythonFunction','NoOp']
def cases():
 for family in FAMILIES:
  origin=next(p for folder in ['script-event-commands','script-event-backends','mission-command-names','script-event-markers'] for p in [ROOT/folder/(family+'.json')]if p.exists())
  original=json.loads(origin.read_text());spec=copy.deepcopy(original)
  setup,body=original['script'].split('BeginMissionSequence;',1)
  names=[]
  for declaration in re.findall(r'^Create\s+\w+\s+([^;]+);',setup,re.M):
   names+=declaration.split()
  assert all(re.fullmatch(r'[A-Za-z][A-Za-z0-9_]*',n)for n in names)
  shared='Global '+' '.join(names)+';\n'
  if family=='Optimize':
   assert body.count('Minimize Opt(Cost);')==1
   body=body.replace('Minimize Opt(Cost);','NonlinearConstraint Opt(U >= 0);\nMinimize Opt(Cost);')
  spec.update(function_body='function [answer] = MatrixFunction()\nCreate Variable answer PrivateFunctionSentinel;\nPrivateFunctionSentinel = 77;\n'+shared+'BeginMissionSequence;'+body+'\nanswer = 42;\n',
   main_script=setup+'\nCreate Variable Returned CallerAfter;\nReturned = -999;\nCallerAfter = 41;\nCreate GmatFunction MatrixFunction;\nMatrixFunction.FunctionPath = \'@FUNCTION@\';\nBeginMissionSequence;\n'+shared+'[Returned] = MatrixFunction();\nCallerAfter = 42;\n',
   context='Function',global_names=names,required_function_types=[original['command']]+(['NonlinearConstraint','Minimize','Vary','EndOptimize']if family=='Optimize'else[]),
   supplementary_origin=str(origin.relative_to(ROOT.parents[2])),supplementary_origin_sha256=hashlib.sha256(origin.read_bytes()).hexdigest(),
   column_tolerances=[1e-4]+[2e-5]*11)
  radius=7000.;mu=398600.4415;speed=math.sqrt(mu/radius);angle=math.sqrt(mu/radius**3)*20
  spec['expected_report']=[20,radius*math.cos(angle),radius*math.sin(angle),0,-speed*math.sin(angle),speed*math.cos(angle),0,41]+original['expected_tail']
  assert len(spec['expected_report'])==12
  for key in ['expected_tail','velocity_increment','expected_extra_rows','extra_tolerances']:
   if key in original:assert spec[key]==original[key]
  yield family,spec
if __name__=='__main__':
 destination=ROOT/'function-command-matrix';destination.mkdir(exist_ok=True)
 count=0
 for key,spec in cases():
  path=destination/(key+'.json')
  if path.exists():assert json.loads(path.read_text())==spec
  else:
   with path.open('x')as f:json.dump(spec,f,indent=2);f.write('\n')
   count+=1
 print(json.dumps(dict(created=count,total=len(FAMILIES),native_tests_run=0)))
