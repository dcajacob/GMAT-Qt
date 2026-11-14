"""Compare optimizer terminator spellings in native solver and branch scopes."""
import argparse,os,tempfile
from pathlib import Path
from RunNestedSolverValidation import run

CASES=[prefix+context for context in ['Direct','If','TargetInside','OptimizeInside'] for prefix in ['Canonical','Worksheet']]+['WrongTargetEndOptimize','WrongIfEndTarget','OrphanEndTarget']

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('binary',type=Path);p.add_argument('startup',type=Path);a=p.parse_args();output=os.environ.get('GMAT_QT_ACCEPTANCE_DIR')
    if output:raise SystemExit(run(a.binary,a.startup,Path(output),CASES))
    with tempfile.TemporaryDirectory(prefix='gmat-optimizer-end-') as temp:raise SystemExit(run(a.binary,a.startup,Path(temp),CASES))
