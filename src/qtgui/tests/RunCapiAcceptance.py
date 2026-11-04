"""Run an external C client in a private GMAT engine session (no GUI).

Retain text inputs, exact hashes and logs; never copy runtime images. This
runner awards no NASA credit. AuditCapiAcceptance supplies the separate review.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess
import tarfile
import uuid
import xml.etree.ElementTree as ET

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def fixture(mode):
    text=''
    for name,r,axis in [('A',7000,0),('B',9000,1)]:
        state=[0.]*6;state[axis]=r;state[axis+4]=math.sqrt(398600.4415/r)
        text+=f"Create Spacecraft {name};\n{name}.DateFormat = A1ModJulian;\n{name}.Epoch = '21545';\n{name}.CoordinateSystem = EarthMJ2000Eq;\n{name}.DisplayStateType = Cartesian;\n"
        text+=''.join(f'{name}.{field} = {value:.17g};\n' for field,value in zip(['X','Y','Z','VX','VY','VZ'],state))
    for i in [1,2]:
        text+=f'Create ForceModel FM{i};\nFM{i}.PrimaryBodies = {{}};\nFM{i}.PointMasses = {{Earth}};\nFM{i}.Drag = None;\nFM{i}.SRP = Off;\nCreate Propagator P{i};\nP{i}.FM = FM{i};\nP{i}.InitialStepSize = 10;\nP{i}.Accuracy = 0.000000000001;\n'
    text+='BeginMissionSequence;\n'
    for i,name,seconds in [(1,'A',60),(2,'B',30)]:
        args=f"{name}, 'STM'" if mode in ('stm','epoch') else name
        text+=f"Propagate 'Flight{i}' P{i}({args}) {{{name}.ElapsedSecs = {seconds}}};\n"
    return text

def run(repo,executable,mode,output,epoch_probe=None,fixture_text=None,additional_binaries=(),extra_arguments=(),fixture_assets=None):
    output=output.resolve();output.mkdir(parents=True,exist_ok=False)
    work=output/'session';work.mkdir();(work/'output').mkdir()
    startup=repo/'application/bin/gmat_startup_qt.txt'
    text=startup.read_text()
    for key,value in [('ROOT_PATH',str(repo/'application')+'/'),('OUTPUT_PATH',str(work/'output')+'/'),('PERSONALIZATION_FILE',str(work/'personalization.ini'))]:
        text=re.sub(r'^'+key+r'\s*=.*$',lambda m:key+' = '+value,text,flags=re.M)
    text=re.sub(r'^PLUGIN\s*=\s*(.+?)\s*$',lambda m:'PLUGIN = '+str((startup.parent/m[1]).resolve()),text,flags=re.M)
    plugin=str(repo/'application/bin/libCInterface')
    if plugin not in text:text+='\nPLUGIN = '+plugin+'\n'
    (work/'gmat_startup_file.txt').write_text(text)
    script=work/'acceptance.script';script.write_text(fixture(mode) if fixture_text is None else fixture_text)
    assets=[]
    for name,body in (fixture_assets or {}).items():
        relative=Path(name)
        if relative.is_absolute() or '..' in relative.parts:raise ValueError('Unsafe C API fixture asset')
        path=work/relative;path.parent.mkdir(parents=True,exist_ok=True);path.write_text(body);assets.append(path)
    sources=sorted({p for folder in ('src/base','src/gmatutil','plugins') for p in (repo/folder).rglob('*') if p.is_file() and p.suffix in ('.c','.cpp','.h','.hpp','.inc')}|{repo/'src/qtgui/CMakeLists.txt',*Path(__file__).parent.glob('*Capi*')})
    sources=[p for p in sources if p.is_file()]
    binaries={executable.resolve()}
    if epoch_probe:binaries.add(epoch_probe.resolve())
    binaries.update(Path(p).resolve() for p in additional_binaries)
    for folder in ('application/bin','application/plugins'):
        for p in (repo/folder).rglob('*'):
            if p.is_file() and not p.is_symlink() and p.open('rb').read(4)==b'\x7fELF':binaries.add(p.resolve())
    inputs=[*sources,*binaries,startup,script,work/'gmat_startup_file.txt',*assets]
    before={str(p):sha(p) for p in inputs}
    (output/'inputs-before.json').write_text(json.dumps(before,indent=2)+'\n')
    with tarfile.open(output/'sources.tar.gz','w:gz') as bundle:
        for p in sources:bundle.add(p,arcname=str(p.relative_to(repo)),recursive=False)
    command=[str(executable.resolve()),str(script),mode]
    command.extend(map(str,extra_arguments))
    if mode=='epoch':
        if not epoch_probe:raise ValueError('Epoch mode requires the private read-only epoch probe')
        command.append(str(epoch_probe.resolve()))
    result=subprocess.run(command,cwd=work,capture_output=True,text=True,timeout=60)
    (output/'client.log').write_text(result.stdout+result.stderr)
    after={str(p):sha(p) for p in inputs}
    (output/'inputs-after.json').write_text(json.dumps(after,indent=2)+'\n')
    proofs=[json.loads(line[len('NASA_CHECK '):]) for line in result.stdout.splitlines() if line.startswith('NASA_CHECK ')]
    loaded=sorted({line[len('NASA_LOADED '):] for line in result.stdout.splitlines() if line.startswith('NASA_LOADED ') and line[len('NASA_LOADED '):].startswith(str(repo)+'/')})
    ok=result.returncode==0 and before==after and len(proofs)==1 and proofs[0]['outcome']=='passed' and loaded and set(loaded)<=set(before)
    suite=ET.Element('testsuite',name='CInterface',tests='1',failures=str(int(not ok)))
    case=ET.SubElement(suite,'testcase',name='QtAcceptance.NasaCInterface'+mode.title())
    if not ok:ET.SubElement(case,'failure',message='C API assertion or evidence failure').text=result.stdout+result.stderr
    ET.ElementTree(suite).write(output/'junit.xml',encoding='unicode')
    receipt=dict(mode='native-C-API',command=command,cwd=str(work),returncode=result.returncode,status='passed' if ok else 'failed',inputs_unchanged=before==after,sources={str(p.relative_to(repo)):before[str(p)] for p in sources},binaries={str(p):before[str(p)] for p in binaries},loaded_project_images={p:before[p] for p in loaded if p in before},proofs=proofs,GUI_executions=0,screenshots=0,runtime_copies=0,full_current_build_qualified=False,full_NASA_qualified=False,technical_core_approval=False)
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],mode=mode,assertions=proofs[0]['assertions'] if proofs else 0,output=str(output))))
    return 0 if ok else 1

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3])
    parser.add_argument('--executable',type=Path,required=True)
    parser.add_argument('--mode',choices=('basic','stm','epoch'),required=True)
    parser.add_argument('--epoch-probe',type=Path)
    destination=parser.add_mutually_exclusive_group(required=True)
    destination.add_argument('--output',type=Path)
    destination.add_argument('--output-root',type=Path)
    args=parser.parse_args()
    output=args.output or args.output_root/(args.mode+'-'+uuid.uuid4().hex[:12])
    raise SystemExit(run(args.repo.resolve(),args.executable,args.mode,output,args.epoch_probe))
