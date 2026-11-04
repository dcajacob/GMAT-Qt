"""Require native Code500 observations and their independent numerical audit."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def run(binary, startup, root):
    log=root/'native.log'
    with log.open('wb') as stream:
        code=subprocess.call([binary,startup],stdout=stream,stderr=subprocess.STDOUT,
            env=dict(os.environ,GMAT_QT_ACCEPTANCE_DIR=str(root)))
    if code:
        print(log.read_text(errors='replace')[-4000:])
        return code
    from AuditCode500EpochInterpolation import audit
    result=audit(json.loads((root/'proof.json').read_text()))
    (root/'independent-audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))
    return 0


def main():
    if len(sys.argv)!=3:
        raise SystemExit('Supply native binary and startup file')
    binary,startup=map(lambda p:str(Path(p).resolve()),sys.argv[1:])
    output=os.environ.get('GMAT_QT_ACCEPTANCE_DIR')
    if output:
        root=Path(output);root.mkdir(parents=True,exist_ok=True)
        return run(binary,startup,root)
    with tempfile.TemporaryDirectory(prefix='gmat-code500-contract-') as folder:
        return run(binary,startup,Path(folder))


if __name__=='__main__':
    raise SystemExit(main())
