"""Execute only the new external-C all-command-summary contract."""
import argparse
from pathlib import Path
import RunCapiAcceptance as base

def fixture(stopped=False,runtime_error=False):
    prefix=base.fixture('basic').split('BeginMissionSequence;')[0]
    prefix+='''Earth.Mu = 398600.4415;
P1.Type = RungeKutta89;
P1.MinStep = 5;
P1.MaxStep = 5;
A.Cd = 2.5;
A.DryMass = 800;
B.Cd = 1.75;
B.DryMass = 950;
Create Variable I Counter;
I = 0;
Counter = 0;
Create ReportFile Values;
Values.Filename = 'cache.csv';
Values.WriteHeaders = false;
Values.FixedWidth = false;
Values.Delimiter = ',';
Values.Precision = 17;
BeginMissionSequence;
'''
    refs='A.ElapsedSecs '+' '.join('A.EarthMJ2000Eq.'+x for x in ('X','Y','Z','VX','VY','VZ'))+' B.ElapsedSecs '+' '.join('B.EarthMJ2000Eq.'+x for x in ('X','Y','Z','VX','VY','VZ'))+' Counter'
    result=prefix+f'''Report 'Before' Values {refs};
GMAT 'ResetCounter' Counter = 0;
For 'OuterLoop' I = 1:1:2;
   If 'BranchChoice' I == 1;
      Propagate 'FirstArc' P1(A) {{A.ElapsedSecs = 10}};
   Else 'Alternate';
      Propagate 'SecondArc' P1(A) {{A.ElapsedSecs = 10}};
   EndIf 'CloseChoice';
EndFor 'CloseLoop';
While 'CounterLoop' Counter < 2;
   GMAT 'Increment' Counter = Counter + 1;
EndWhile 'CloseCounter';
If 'NeverChoice' Counter < 0;
   Propagate 'Unexecuted' P1(A) {{A.ElapsedSecs = 30}};
EndIf 'CloseNever';
BeginScript 'ScriptBlock';
   GMAT 'ScriptValue' Counter = Counter + 40;
EndScript 'CloseScript';
Report 'After' Values {refs};
GMAT 'LateState' A.X = A.X + 1234;
GMAT 'LateMass' A.DryMass = 991;
Report 'Late' Values {refs};
'''
    if runtime_error:return result.replace("   Else 'Alternate';","      GMAT 'BadArgument' Counter = -1;\n      GMAT 'RuntimeError' Counter = sqrt(Counter);\n   Else 'Alternate';")
    return result.replace("   Else 'Alternate';","      Stop 'Halt';\n   Else 'Alternate';") if stopped else result

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--executable',type=Path,required=True);p.add_argument('--metadata-probe',type=Path,required=True);variant=p.add_mutually_exclusive_group();variant.add_argument('--stopped',action='store_true');variant.add_argument('--runtime-error',action='store_true');dest=p.add_mutually_exclusive_group(required=True);dest.add_argument('--output',type=Path);dest.add_argument('--output-root',type=Path);a=p.parse_args()
    import uuid
    mode='runtimeerror' if a.runtime_error else 'stopped' if a.stopped else 'summary';output=a.output or a.output_root/(mode+'-'+uuid.uuid4().hex[:12]);probe=a.metadata_probe.resolve();raise SystemExit(base.run(a.repo.resolve(),a.executable,mode,output,fixture_text=fixture(a.stopped,a.runtime_error),additional_binaries=[probe],extra_arguments=[probe],fixture_assets={'recovery.script':fixture()} if a.stopped or a.runtime_error else None))
