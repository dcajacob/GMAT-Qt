"""Run isolated native numeric measured-Goal policy and solver-result checks."""
import argparse,os,tempfile
from pathlib import Path
from RunNestedSolverValidation import run
CASES=['VariableDefault','VariableFalse','LiteralDefault','LiteralFalse','TrueFraction','TrueScientific','TrueInconsistent','TrueOverflow']
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('binary',type=Path);p.add_argument('startup',type=Path);a=p.parse_args();output=os.environ.get('GMAT_QT_ACCEPTANCE_DIR')
    if output:raise SystemExit(run(a.binary,a.startup,Path(output),CASES))
    with tempfile.TemporaryDirectory(prefix='gmat-achieve-goal-') as temp:raise SystemExit(run(a.binary,a.startup,Path(temp),CASES))
