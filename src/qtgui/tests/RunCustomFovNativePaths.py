"""Check shared CustomFOV paths through native objects and the actual console."""
import argparse,json,os,re,subprocess,tempfile,time
from pathlib import Path
from AcceptanceRuntime import isolate_startup
from RunNestedSolverValidation import run as native_run,sha

CASES=['Default','Absolute','Alias','Missing']
MASK='ConeClockAngles\n20 0\n20 90\n20 180\n20 270\n'

def run(probe,console,startup,output):
    output=output.resolve();output.mkdir(parents=True,exist_ok=True)
    native_status=native_run(probe,startup,output/'native',CASES)
    results=[]
    for name in ['Absolute','Alias','Missing']:
        case=output/('console-'+name);case.mkdir();private=isolate_startup(startup,case/'runtime'/'bin')
        masks=case/'masks';masks.mkdir();(masks/'native-mask.txt').write_text(MASK)
        text=re.sub(r'^FOV_DATA_PATH\s*=.*$','',private.read_text(),flags=re.M)
        text+='\nACCEPTANCE_FOV_PATH = '+str(masks)+'/\nFOV_DATA_PATH = '+(str(masks)+'/' if name=='Absolute' else 'ACCEPTANCE_FOV_PATH/')+'\n';private.write_text(text)
        leaf='missing-mask.txt' if name=='Missing' else 'native-mask.txt'
        script=case/'mission.script';script.write_text("Create CustomFOV Mask OtherMask;\nMask.FOVFileName = '"+leaf+"';\nOtherMask.FOVFileName = '"+leaf+"';\nCreate Spacecraft Sat;\nCreate Imager Camera OtherCamera;\nCamera.FieldOfView = Mask;\nOtherCamera.FieldOfView = OtherMask;\nSat.AddHardware = {Camera, OtherCamera};\nCreate Variable Keep;\nKeep = 41;\nCreate ReportFile Result;\nResult.Filename = 'result.csv';\nResult.WriteHeaders = false;\nResult.Precision = 17;\nBeginMissionSequence;\nReport Result Keep;\n")
        command=[str(console.resolve()),'--startup_file',str(private),'--run',str(script),'--exit'];env=dict(os.environ)
        for key in ['DISPLAY','WAYLAND_DISPLAY','XAUTHORITY','WAYLAND_SOCKET']:env.pop(key,None)
        mapped=set();last_maps='';start=time.monotonic()
        with (case/'console.log').open('w') as log:
            process=subprocess.Popen(command,cwd=private.parent,env=env,stdout=log,stderr=subprocess.STDOUT)
            while process.poll() is None and time.monotonic()-start<60:
                try:last_maps=(Path('/proc')/str(process.pid)/'maps').read_text()
                except FileNotFoundError:pass
                for line in last_maps.splitlines():
                    fields=line.split(None,5)
                    if len(fields)==6 and fields[5].startswith('/'):mapped.add(fields[5])
                time.sleep(.005)
            if process.poll() is None:process.kill();process.wait();code=124
            else:code=process.wait()
        (case/'native-maps.txt').write_text(last_maps);images={}
        for filename in sorted(mapped):
            path=Path(filename)
            if path.is_file():
                with path.open('rb') as f:is_elf=f.read(4)==b'\x7fELF'
                if is_elf:images[filename]=sha(path)
        (case/'native-image-identities.json').write_text(json.dumps(images,indent=2)+'\n')
        gui=[p for p in images if re.search(r'/(?:libQt[56](?:Widgets|Gui|Quick)|libwx)',p)]
        report=private.parent/'output'/'result.csv';content=report.read_text().strip() if report.exists() else None
        passed=bool(images and str(console.resolve()) in images and not gui and ((code!=0 and code!=124 and content is None and re.search(r'Exception|ERROR|error|failed',(case/'console.log').read_text())) if name=='Missing' else (code==0 and content=='41')))
        row=dict(case=name,command=command,returncode=code,passed=passed,report=content,display_variables_removed=True,loaded_GUI_images=gui,loaded_ELF_images=len(images),binary_sha256=sha(console),startup_sha256=sha(private),script_sha256=sha(script),runtime_image_copies=0)
        (case/'result.json').write_text(json.dumps(row,indent=2)+'\n');results.append(row);print(json.dumps(row),flush=True)
    receipt=dict(passed=native_status==0 and all(r['passed'] for r in results),native_cases=4,native_passed=json.loads((output/'native/results.json').read_text())['cases'],console_cases=results,new_requirement_credit=0,full_console_parity=False)
    (output/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    return 0 if receipt['passed'] else 1

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('probe',type=Path);p.add_argument('console',type=Path);p.add_argument('startup',type=Path);a=p.parse_args();output=os.environ.get('GMAT_QT_ACCEPTANCE_DIR')
    if output:raise SystemExit(run(a.probe,a.console,a.startup,Path(output)))
    with tempfile.TemporaryDirectory(prefix='gmat-native-fov-') as temp:raise SystemExit(run(a.probe,a.console,a.startup,Path(temp)))
