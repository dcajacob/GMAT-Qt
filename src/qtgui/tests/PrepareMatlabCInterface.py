"""Generate MATLAB's C-interface prototype/thunk in an isolated build directory."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--matlab', type=Path, required=True)
    p.add_argument('--library', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    repo = Path(__file__).resolve().parents[3]
    header = repo/'plugins/CInterfacePlugin/src/plugin/CInterfaceFunctions.h'
    definitions = repo/'plugins/CInterfacePlugin/src/include/GmatCFunc_defs.hpp'
    sha = lambda f: hashlib.sha256(Path(f).read_bytes()).hexdigest()
    inputs = {str(f.resolve()):sha(f) for f in [Path(__file__), header, definitions, a.matlab, a.library]}
    output = a.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    receipt = output/'wrapper-receipt.json'
    if receipt.exists():
        old = json.loads(receipt.read_text())
        if old['inputs'] == inputs and all(Path(f).is_file() and sha(f) == h for f,h in old['generated'].items()):
            print('Reusing unchanged MATLAB C-interface wrapper')
            return 0
        raise RuntimeError('Wrapper inputs changed; use a fresh output directory')
    quote = lambda t: "'"+str(t).replace("'", "''")+"'"
    script = output/'prepare_wrapper.m'
    assert not script.exists(), 'Retain the failed preparation; use a fresh directory'
    script.write_text("% Generated build driver; no acceptance credit.\ncd("+quote(output)+");\n"
        "[missing, warnings] = loadlibrary("+quote(a.library.absolute())+", "+quote(header)+
        ", 'mfilename', 'interfacewrapper', 'includepath', "+quote(definitions.parent)+");\n"
        "assert(libisloaded('libCInterface') && isempty(missing));\n"
        "f = libfunctions('libCInterface');\n"
        "assert(all(ismember({'StartGmat','LoadScript','RunScript'},f)));\n"
        "disp(jsonencode(struct('missing',{missing},'warnings',warnings,'functions',{f})));\n"
        "unloadlibrary('libCInterface'); assert(~libisloaded('libCInterface'));\n")
    with (output/'prepare-wrapper.log').open('xb') as log:
        code = subprocess.call([str(a.matlab), '-batch', 'run('+quote(script)+')'], stdout=log, stderr=subprocess.STDOUT)
    if code:
        return code
    generated = {str(f):sha(f) for f in sorted(output.iterdir()) if f.is_file() and f.name not in ['wrapper-receipt.json','prepare-wrapper.log']}
    assert output/'interfacewrapper.m' in [Path(f) for f in generated]
    receipt.write_text(json.dumps(dict(status='generated', inputs=inputs, generated=generated,
        generated_thunks=[f for f in generated if Path(f).suffix in ['.so','.dll','.dylib']],
        runtime_binary_copies=0, new_requirement_credit=0), indent=2)+'\n')
    print('Generated MATLAB C-interface wrapper')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
