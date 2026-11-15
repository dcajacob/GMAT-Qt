"""Freeze GUI placement inputs from existing executed command fixtures."""
from pathlib import Path
import hashlib,json
ROOT=Path(__file__).resolve().parent
POSITIONS=['Empty','Many','BlockBefore','BlockAfter','EndBefore','EndAfter','BranchAppend','LeafBefore','LeafAfter']
PARENTS=['For','While','Target','Optimize']
MISSING_FOR=['Assignment','ScriptEvent','CallGmatFunction','CallMatlabFunction','RunSimulator','RunEstimator','Stop']

def load(relative):
 p=ROOT/relative;return json.loads(p.read_text()),str(p.relative_to(ROOT.parents[2])),hashlib.sha256(p.read_bytes()).hexdigest()

def generate():
 refs,refpath,refhash=load('NasaCommandReferenceCases.json');refmap={v['key']:v for v in refs['cases']}
 families=['Optimize','Target','If','While','For','BeginFiniteBurn','EndFiniteBurn','Maneuver','Assignment','Toggle','PenUp','PenDown','Propagate','Report','Save','ScriptEvent','RunSimulator','RunEstimator','CallGmatFunction','CallMatlabFunction','Stop']
 rows=[]
 for family in families:
  sourcepath,sourcehash=refpath,refhash
  if family in refmap: spec=refmap[family]
  elif family in ['RunSimulator','RunEstimator']:spec,sourcepath,sourcehash=load('command-operation-navigation/'+family+'.json')
  elif family=='Assignment':spec,sourcepath,sourcehash=load('command-operation-control/Assignment.json')
  elif family=='CallMatlabFunction':spec,sourcepath,sourcehash=load('MatlabCommandOperationCase.json')
  elif family=='CallGmatFunction':spec,sourcepath,sourcehash=load('script-event-commands/CallGmatFunction.json')
  else:spec,sourcepath,sourcehash=load('mission-command-names/Propagate.json')
  prefix=spec['script'].split('BeginMissionSequence;',1)[0]
  prefix+='Create Variable TreeI TreeSeen TreeControl TreeCost;\nTreeI = 0;\nTreeSeen = 0;\nTreeControl = 1;\nTreeCost = 0;\n'
  prefix+="Create DifferentialCorrector ContextDC;\nContextDC.ShowProgress = false;\nContextDC.ReportFile = '@ROOT@/context-dc.txt';\nCreate Yukon ContextOpt;\nContextOpt.ShowProgress = false;\nContextOpt.ReportFile = '@ROOT@/context-opt.txt';\nBeginMissionSequence;\n"
  statements={
   'Assignment':"'$PROBE$' Count = 2 * U + 1;",'ScriptEvent':"BeginScript '$PROBE$';\nCount = Count + 3;\nEndScript;",
   'CallGmatFunction':"'$PROBE$' [Cost] = Accept(U);",'CallMatlabFunction':"'$PROBE$' [Cost] = Accept(U);",
   'RunSimulator':"RunSimulator '$PROBE$' Sim;",'RunEstimator':"RunEstimator '$PROBE$' EKF;",'Stop':"Stop '$PROBE$';",
   'Propagate':"Propagate '$PROBE$' P(Sat) {Sat.ElapsedSecs = 10};",'Maneuver':"Maneuver '$PROBE$' Kick(Sat);",
   'BeginFiniteBurn':"BeginFiniteBurn '$PROBE$' Burn(Sat);",'EndFiniteBurn':"EndFiniteBurn '$PROBE$' Burn(Sat);",
   'PenUp':"PenUp '$PROBE$' Trace;",'PenDown':"PenDown '$PROBE$' Trace;",'Toggle':"Toggle '$PROBE$' Trace Off;",
   'Report':"Report '$PROBE$' Extra Quantity;",'Save':"Save '$PROBE$' Quantity;",
   'For':"For '$PROBE$' I = 1:1:2;\nCount = Count + 1;\nEndFor;",
   'While':"While '$PROBE$' I < 2;\nI = I + 1;\nEndWhile;",
   'If':"If '$PROBE$' U > 0;\nCount = Count + 1;\nElse;\nCount = Count - 1;\nEndIf;",
   'Target':"Target '$PROBE$' DC {SolveMode = RunInitialGuess, ExitMode = DiscardAndContinue, ShowProgressWindow = false};\nVary DC(U = 1, {Perturbation = 0.01, Lower = -10, Upper = 10, MaxStep = 1});\nAchieve DC(U = 1, {Tolerance = 1e-6});\nEndTarget;",
   'Optimize':"Optimize '$PROBE$' Opt {SolveMode = RunInitialGuess, ExitMode = DiscardAndContinue, ShowProgressWindow = false};\nVary Opt(U = 1, {Perturbation = 0.01, Lower = -10, Upper = 10, MaxStep = 1});\nCost = U^2;\nMinimize Opt(Cost);\nEndOptimize;"}
  native={'Assignment':'GMAT','ScriptEvent':'BeginScript'}.get(family,family)
  template={'ScriptEvent':'Script event','CallGmatFunction':'Call function','CallMatlabFunction':'Call MATLAB function'}.get(family,family)
  files={v['name']:v['text'] for v in spec.get('support_files',[])}
  for parent in PARENTS:
   outer={'For':"For 'Outer' TreeI = 1:1:2;\n",'While':"While 'Outer' TreeI < 2;\n",'Target':"Target 'Outer' ContextDC {SolveMode = RunInitialGuess, ExitMode = DiscardAndContinue, ShowProgressWindow = false};\n",'Optimize':"Optimize 'Outer' ContextOpt {SolveMode = RunInitialGuess, ExitMode = DiscardAndContinue, ShowProgressWindow = false};\n"}[parent]
   start=outer
   if parent=='While':start+='TreeI = TreeI + 1;\n'
   if parent in ['Target','Optimize']:
    solver='ContextDC' if parent=='Target' else 'ContextOpt'
    start+=f'Vary {solver}(TreeControl = 1, {{Perturbation = 0.01, Lower = -10, Upper = 10, MaxStep = 1}});\n'
   leaf="'Leaf' TreeSeen = TreeSeen + 1;\n"
   body=start+"If 'Inner' TreeControl > 0;\n"+leaf+"EndIf;\n"
   if parent=='Target':body+='Achieve ContextDC(TreeControl = 1, {Tolerance = 1e-6});\n'
   if parent=='Optimize':body+='TreeCost = TreeControl^2;\nMinimize ContextOpt(TreeCost);\n'
   closing='End'+parent+';\n';body+=closing+"'Tail' TreeSeen = TreeSeen + 1;\n"
   rows.append(dict(key=family+parent,family=family,outer_context=parent,prefix=prefix,body=body,outer_anchor=outer,leaf_anchor=leaf,closing_anchor=closing,closing_type='End'+parent,positions=POSITIONS if parent=='For' else ['BlockBefore','BlockAfter','EndBefore','EndAfter','BranchAppend'],statement=statements[family],native_type=native,template=template,files=files,source_fixture=sourcepath,source_fixture_sha256=sourcehash,run_missing_by_default=parent!='For' or family in MISSING_FOR))
 return dict(status='frozen-GUI-creation-expectations',scope='GUI creation, configuration, source/history and native graph only; reuse independent execution evidence separately.',cases=rows)

if __name__=='__main__':
 p=ROOT/'MissionCreationPositionCases.json';p.write_text(json.dumps(generate(),indent=2)+'\n');print('Wrote',len(generate()['cases']),'bounded context cases')
