"""Exercise actual console plotting exceptions with independent mission results.

No GUI credit or whole-product parity is inferred here. Positive GUI delivery
must be conjoined separately from its existing source-versioned evidence.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import subprocess
import tarfile
import time
import xml.etree.ElementTree as ET

from RunCapiAcceptance import fixture

CASES = ['none', 'xy', 'orbit', 'ground', 'all', 'invalid-xy', 'invalid-orbit', 'invalid-ground']
PLOTS = {
 'xy': [('XYPlot','ConsoleXY', '''ConsoleXY.XVariable = A.ElapsedSecs;
ConsoleXY.YVariables = {A.EarthMJ2000Eq.X, A.EarthMJ2000Eq.Y};
ConsoleXY.ShowGrid = true;
''')],
 'orbit': [('OrbitView','ConsoleOrbit', '''ConsoleOrbit.Add = {A, B, Earth};
ConsoleOrbit.CoordinateSystem = EarthMJ2000Eq;
ConsoleOrbit.EnableStars = Off;
''')],
 'ground': [('GroundTrack','ConsoleGround', '''ConsoleGround.Add = {A, B};
ConsoleGround.CentralBody = Earth;
'''), ('GroundTrackPlot','ConsoleLegacyGround', '''ConsoleLegacyGround.Add = {A, B};
ConsoleLegacyGround.CentralBody = Earth;
''')]}


def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def write(p, v): p.write_text(json.dumps(v, indent=2)+'\n')
def require(value, message):
    if not value: raise ValueError(message)


def mission(case):
    groups = list(PLOTS) if case == 'all' else [case.removeprefix('invalid-')] if case != 'none' else []
    selected = [r for g in groups for r in PLOTS[g]]
    text = fixture('basic').split('BeginMissionSequence;')[0]
    text += 'Earth.Mu = 398600.4415;\nP1.MinStep = 2;\nP1.MaxStep = 2;\nP2.MinStep = 2;\nP2.MaxStep = 2;\n'
    text += ''.join('Create '+kind+' '+name+';\n'+properties for kind, name, properties in selected)
    if case == 'invalid-xy': text = text.replace('A.EarthMJ2000Eq.Y', 'A.NoSuchParameter')
    if case == 'invalid-orbit': text = text.replace('ConsoleOrbit.CoordinateSystem = EarthMJ2000Eq;', 'ConsoleOrbit.CoordinateSystem = MissingFrame;')
    if case == 'invalid-ground': text = text.replace('ConsoleGround.CentralBody = Earth;', 'ConsoleGround.CentralBody = MissingBody;')
    text += '''Create Variable Shared;
Shared = 17;
Create ReportFile Results;
Results.Filename = 'states.csv';
Results.WriteHeaders = false;
Results.FixedWidth = false;
Results.Delimiter = ',';
Results.Precision = 17;
BeginMissionSequence;
'''
    refs = ' '.join(s+'.ElapsedSecs '+' '.join(s+'.EarthMJ2000Eq.'+f for f in ('X','Y','Z','VX','VY','VZ')) for s in ('A','B'))+' Shared'
    text += f"Report 'Before' Results {refs};\nPropagate 'ArcA' P1(A) {{A.ElapsedSecs = 20}};\nReport 'AfterA' Results {refs};\nPropagate 'ArcB' P2(B) {{B.ElapsedSecs = 10}};\nReport 'AfterB' Results {refs};\n"
    if selected: text += 'Write '+' '.join(name for _,name,_ in selected)+' {Style = Script};\n'
    return text, [(kind, name) for kind,name,_ in selected]


def check_states(path):
    import csv
    rows = [[float(x) for x in row] for row in csv.reader(path.open())]
    require(len(rows) == 3 and all(len(r) == 15 for r in rows), 'Console must report all three complete result rows')
    checks = 0
    for row, times in zip(rows, [(0,0),(20,0),(20,10)]):
        for sat, (radius, seconds) in enumerate(zip([7000,9000], times)):
            w=math.sqrt(398600.4415/radius**3)
            state=[radius*math.cos(w*seconds),radius*math.sin(w*seconds),0,-radius*w*math.sin(w*seconds),radius*w*math.cos(w*seconds),0] if sat==0 else [0,radius*math.cos(w*seconds),radius*math.sin(w*seconds),0,-radius*w*math.sin(w*seconds),radius*w*math.cos(w*seconds)]
            expected=[seconds,*state]
            for j, value in enumerate(expected):
                require(math.isfinite(row[sat*7+j]) and abs(row[sat*7+j]-value) <= (1e-4 if j==0 else 2e-5 if j<4 else 4e-8), 'Independent console circular state failed')
                checks += 1
        require(row[14] == 17, 'Shared non-graphical mission result changed'); checks += 1
    return rows, checks


def run(repo, output, cases):
    require(set(cases) <= set(CASES) and len(cases)==len(set(cases)), 'Unknown/duplicate console case')
    output=output.resolve();output.mkdir(parents=True,exist_ok=False)
    executable=(repo/'application/bin/GmatConsole-R2026a').resolve();startup=repo/'application/bin/gmat_startup_qt.txt'
    sources={p for folder in ['src/base','src/gmatutil','plugins','src/console'] for p in (repo/folder).rglob('*') if p.is_file() and p.suffix in ('.c','.cpp','.h','.hpp','.inc')}
    sources |= {Path(__file__).resolve(),repo/'src/qtgui/tests/RunCapiAcceptance.py',repo/'src/qtgui/CMakeLists.txt'}
    binaries={executable}
    for folder in ['application/bin','application/plugins']:
        for p in (repo/folder).rglob('*'):
            if p.is_file() and not p.is_symlink():
                with p.open('rb') as f:
                    if f.read(4)==b'\x7fELF':binaries.add(p.resolve())
    fixture_inputs=[]
    selections={}
    for case in cases:
        work=output/case;work.mkdir();(work/'output').mkdir()
        text=startup.read_text()
        for key,value in [('ROOT_PATH',str(repo/'application')+'/'),('OUTPUT_PATH',str(work/'output')+'/'),('PERSONALIZATION_FILE',str(work/'personalization.ini'))]:text=re.sub(r'^'+key+r'\s*=.*$',lambda m:key+' = '+value,text,flags=re.M)
        text=re.sub(r'^PLUGIN\s*=\s*(.+?)\s*$',lambda m:'PLUGIN = '+str((startup.parent/m[1]).resolve()),text,flags=re.M)
        start=work/'startup.txt';start.write_text(text);body,selections[case]=mission(case);script=work/'mission.script';script.write_text(body);fixture_inputs += [start,script]
    inputs=[*sorted(sources),*sorted(binaries),startup,*fixture_inputs,Path('/etc/os-release')]
    before={str(p):sha(p) for p in inputs};write(output/'inputs-before.json',before)
    with tarfile.open(output/'sources.tar.gz','w:gz') as b:
        for p in sorted(sources):b.add(p,arcname=str(p.relative_to(repo)),recursive=False)
    env=dict(os.environ)
    for key in ['DISPLAY','WAYLAND_DISPLAY','XAUTHORITY','WAYLAND_SOCKET']:env.pop(key,None)
    results=[]
    for case in cases:
        work=output/case;command=[str(executable),'--startup_file',str(work/'startup.txt'),'--run',str(work/'mission.script'),'--exit'];images=set();log=work/'console.log';error=None
        with log.open('w') as stream:
            process=subprocess.Popen(command,cwd=work,env=env,stdin=subprocess.DEVNULL,stdout=stream,stderr=subprocess.STDOUT,text=True)
            started=time.monotonic()
            while process.poll() is None:
                try:
                    for line in Path(f'/proc/{process.pid}/maps').read_text().splitlines():
                        fields=line.split()
                        if len(fields)>=6 and fields[-1].startswith('/') and Path(fields[-1]).exists(): images.add(str(Path(fields[-1]).resolve()))
                except (FileNotFoundError,ProcessLookupError):pass
                if time.monotonic()-started>30:process.kill();process.wait();error='Console timeout';break
                time.sleep(.005)
        content=log.read_text();invalid=case.startswith('invalid-');rows=[];checks=0
        try:
            require(error is None,error)
            require(images and str(executable) in images and not any('libQt' in Path(p).name or 'libX11' in Path(p).name for p in images), 'Console process was unobserved or loaded GUI libraries')
            project={p:before[p] for p in images if p.startswith(str(repo)+'/')}
            require(project and all(p in before for p in project), 'Unbound loaded console images')
            if invalid:
                require(process.returncode != 0 and re.search('Exception|ERROR|error|failed',content), 'Invalid plot configuration was silently accepted')
                require(not (work/'output/states.csv').exists(), 'Invalid plot configuration unexpectedly executed mission')
            else:
                require(process.returncode == 0 and 'Mission run completed' in content, 'Console mission did not finish successfully')
                rows,checks=check_states(work/'output/states.csv')
                for kind,name in selections[case]:
                    expression=r'Create '+('(?:GroundTrack|GroundTrackPlot)' if kind.startswith('GroundTrack') else kind)+' '+name+r';'
                    require(re.search(expression,content), 'Console did not serialize the actual configured plot object: '+name)
                if (output/'none/output/states.csv').exists():require((work/'output/states.csv').read_bytes()==(output/'none/output/states.csv').read_bytes(),'Plot exceptions changed ordinary mission results')
            status='passed'
        except (ValueError,OSError,KeyError) as failure:status='failed';error=str(failure);project={p:before[p] for p in images if p in before and p.startswith(str(repo)+'/')}
        result=dict(case=case,status=status,error=error,command=command,cwd=str(work),returncode=process.returncode,configured_plots=selections[case],loaded_project_images=project,loaded_GUI_images=[p for p in images if 'libQt' in Path(p).name or 'libX11' in Path(p).name],display_variables_removed=True,independent_numeric_fields=checks,rows=rows,log_sha256=sha(log))
        results.append(result);write(output/'partial-results.json',results)
    after={str(p):sha(p) for p in inputs};write(output/'inputs-after.json',after)
    ok=before==after and all(r['status']=='passed' for r in results)
    suite=ET.Element('testsuite',name='ConsolePlotExceptions',tests=str(len(results)),failures=str(sum(r['status']!='passed' for r in results)))
    for r in results:
        c=ET.SubElement(suite,'testcase',name='QtAcceptance.ConsolePlot.'+r['case'])
        if r['status']!='passed':ET.SubElement(c,'failure',message=r['error'] or 'Console acceptance failed')
    ET.ElementTree(suite).write(output/'junit.xml',encoding='unicode')
    receipt=dict(mode='native-console',status='passed' if ok else 'failed',inputs_unchanged=before==after,sources={str(p.relative_to(repo)):before[str(p)] for p in sources},binaries={str(p):before[str(p)] for p in binaries},host_os_release=Path('/etc/os-release').read_text(),results=results,GUI_executions=0,screenshots=0,runtime_copies=0,full_current_build_qualified=False,full_NASA_qualified=False,technical_core_approval=False)
    write(output/'receipt.json',receipt);print(json.dumps(dict(status=receipt['status'],cases=[{k:r[k] for k in ['case','status','error','returncode']} for r in results],output=str(output))))
    return not ok


if __name__=='__main__':
    import uuid
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);dest=p.add_mutually_exclusive_group(required=True);dest.add_argument('--output',type=Path);dest.add_argument('--output-root',type=Path);p.add_argument('--case',choices=CASES,action='append');a=p.parse_args();output=a.output or a.output_root/uuid.uuid4().hex[:12];raise SystemExit(run(a.repo.resolve(),output,a.case or CASES))
