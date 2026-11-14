"""Exercise the existing orbit drag assertions on the native OpenGL path."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile

import BuildPreviewPackage as builder


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform', required=True)
    parser.add_argument('--test-executable', type=Path)
    parser.add_argument('--output', type=Path, default=builder.LOGS / 'native-desktop')
    args=parser.parse_args()
    output=args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    project=output / 'probe-source'
    project.mkdir(exist_ok=True)
    shutil.copy2(Path(__file__).with_name('NativeGraphicsProbe.cpp'),project / 'probe.cpp')
    libraries='opengl32 gdi32 user32' if args.platform.startswith('windows') else (
        '"-framework OpenGL"' if args.platform.startswith('macos') else 'GL X11')
    (project / 'CMakeLists.txt').write_text(
        'cmake_minimum_required(VERSION 3.16)\nproject(NativeGraphicsProbe LANGUAGES CXX)\n'
        'add_executable(NativeGraphicsProbe probe.cpp)\n'
        f'target_link_libraries(NativeGraphicsProbe PRIVATE {libraries})\n')
    build=output / 'probe-build'
    with (output / 'build.log').open('w') as log:
        subprocess.run(['cmake','-S',str(project),'-B',str(build)],stdout=log,stderr=subprocess.STDOUT,check=True)
        subprocess.run(['cmake','--build',str(build),'--config','Release'],stdout=log,stderr=subprocess.STDOUT,check=True)
    filename='NativeGraphicsProbe.exe' if args.platform.startswith('windows') else 'NativeGraphicsProbe'
    probe=next(path for path in build.rglob(filename) if path.is_file())
    result=subprocess.run([str(probe)],text=True,capture_output=True)
    (output / 'context.log').write_text(result.stdout+result.stderr)
    graphics=json.loads(result.stdout.strip().splitlines()[-1]) if result.stdout.strip() else {'available':False}
    renderer=graphics.get('renderer','').lower()
    graphics['software_renderer']=any(name in renderer for name in (
        'llvmpipe','softpipe','software rasterizer','gdi generic','basic render','swiftshader'))
    graphics['qpa']='windows' if args.platform.startswith('windows') else (
        'cocoa' if args.platform.startswith('macos') else 'xcb')
    receipt={'platform':args.platform,'graphics':graphics,'test':'QtGui.OrbitDragDirection',
             'assertions':'Unmodified src/qtgui/tests/SummaryDragTests.cpp',
             'status':'graphics-unavailable','full_regression_run':False}
    # Qt's OpenGL widget needs an OpenGL 2.0-capable context.
    supported=graphics['available'] and not graphics.get('version','1').startswith('1.')
    if args.platform.startswith('windows') and not supported:
        context=json.loads((builder.ROOT / 'native-build-context.json').read_text())
        software=Path(context['qt']) / 'bin/opengl32sw.dll'
        graphics['qt_software_fallback_available']=software.is_file()
        supported=software.is_file()
        if supported:
            graphics['qt_software_fallback_sha256']=hashlib.sha256(software.read_bytes()).hexdigest()
    if supported:
        executable=args.test_executable
        if executable is None:
            archive=builder.ROOT / 'reuse-drag' / ('native-inputs-'+args.platform+'.tar.gz')
            with tarfile.open(archive) as source:
                metadata=json.load(source.extractfile('build/preview-package/native-build-context.json'))
                suffix='GmatQtSummaryDragTests'+('.exe' if args.platform.startswith('windows') else '')
                matches=[member for member in source.getmembers() if member.isfile() and member.name.endswith('/'+suffix)]
                assert len(matches)==1
                member=matches[0]
                data=source.extractfile(member).read()
                assert hashlib.sha256(data).hexdigest()==metadata['file_sha256'][member.name]
                executable=output / suffix
                executable.write_bytes(data)
                executable.chmod(0o755)
            receipt['test_source_commit']=metadata['source_commit']
        environment=dict(os.environ,QT_QPA_PLATFORM=graphics['qpa'],GMAT_QT_ACCEPTANCE_DIR=str(output))
        environment['QT_LOGGING_RULES']='qt.qpa.gl=true'
        environment['PATH']=str(builder.REPO / 'application/bin')+os.pathsep+environment.get('PATH','')
        if args.platform.startswith('windows'):
            context=json.loads((builder.ROOT / 'native-build-context.json').read_text())
            environment['PATH']=os.pathsep.join([str(Path(context['qt']) / 'bin'),str(Path(context['osg']) / 'bin'),
                str(Path(context['prefix']) / 'bin'),environment['PATH']])
            environment['QT_PLUGIN_PATH']=str(Path(context['qt']) / 'plugins')
            if graphics.get('qt_software_fallback_available'):
                # OSG imports opengl32 directly. Use one Mesa module for both
                # OSG and Qt so their current contexts refer to the same driver.
                driver=output / 'opengl32.dll'
                shutil.copy2(software,driver)
                environment['QT_OPENGL_DLL']=str(driver)
                environment['QT_OPENGL']='desktop'
                fallback_probe=output / 'MesaGraphicsProbe.exe'
                shutil.copy2(probe,fallback_probe)
                fallback=subprocess.run([str(fallback_probe)],text=True,capture_output=True)
                (output / 'fallback-context.log').write_text(fallback.stdout+fallback.stderr)
                if fallback.stdout.strip():
                    graphics['fallback']=json.loads(fallback.stdout.strip().splitlines()[-1])
        elif args.platform.startswith('linux'):
            qt=builder.DEPS / 'qt/6.10.2/gcc_64'
            environment['LD_LIBRARY_PATH']=os.pathsep.join([str(builder.REPO / 'application/bin'),str(qt / 'lib'),environment.get('LD_LIBRARY_PATH','')])
        result=subprocess.run([str(executable.resolve()),'drag'],env=environment,text=True,
                              stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=120)
        (output / 'orbit-drag.log').write_text(result.stdout)
        receipt['status']='passed' if result.returncode==0 else 'failed'
        receipt['test_exit']=result.returncode
        receipt['executable_sha256']=hashlib.sha256(executable.read_bytes()).hexdigest()
    (output / 'verification.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt,indent=2))
    return 1 if receipt['status']=='failed' else 0


if __name__=='__main__':
    raise SystemExit(main())
