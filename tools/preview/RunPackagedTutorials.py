"""Build the GUI tutorial observer against an extracted release's native libraries."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import zipfile

import BuildPreviewPackage as builder


def windows_build_environment():
    vswhere = Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
    installation = subprocess.check_output([str(vswhere), '-latest', '-products', '*',
        '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
        '-property', 'installationPath'], text=True).strip()
    setup = Path(installation)/'VC/Auxiliary/Build/vcvars64.bat'
    result = subprocess.check_output(f'"{setup}" >nul && set', shell=True, text=True)
    environment = dict(os.environ)
    for line in result.splitlines():
        if '=' in line and not line.startswith('='):
            name, value = line.split('=', 1)
            environment[name] = value
    return environment


def import_libraries(binary, output, environment):
    output.mkdir()
    for name in ['GmatBase', 'GmatUtil']:
        dll = binary/('lib'+name+'.dll')
        exports = subprocess.check_output(['dumpbin', '/nologo', '/exports', str(dll)],
                                          env=environment, text=True)
        symbols = re.findall(r'^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+(\S+)', exports, re.M)
        assert symbols, dll
        definition = output/(name+'.def')
        definition.write_text('LIBRARY '+dll.name+'\nEXPORTS\n'+'\n'.join(symbols)+'\n')
        subprocess.run(['lib', '/nologo', '/machine:x64', '/def:'+str(definition),
                        '/out:'+str(output/(name+'.lib'))], env=environment, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform', required=True)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--tutorial', action='append')
    parser.add_argument('--output', type=Path, default=builder.REPO/'build/tutorial-release')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    builder.LOGS.mkdir(parents=True, exist_ok=True)
    builder.DEPS.mkdir(parents=True, exist_ok=True)
    workspace = Path(tempfile.mkdtemp(prefix='gmat-tutorial-release-'))
    extracted = workspace/'extracted'
    extracted.mkdir()
    if args.platform.startswith('macos'):
        subprocess.run(['ditto', '-x', '-k', str(args.archive.resolve()), str(extracted)], check=True)
    elif args.platform.startswith('windows'):
        with zipfile.ZipFile(args.archive) as archive:
            archive.extractall(extracted)
    else:
        with tarfile.open(args.archive) as archive:
            archive.extractall(extracted, filter='data')
    payload = next(p for p in extracted.iterdir() if p.is_dir() and p.name.startswith('GMAT-Qt-'))
    for name, digest in json.loads((payload/'PACKAGE-MANIFEST.json').read_text()).items():
        assert hashlib.sha256((payload/name).read_bytes()).hexdigest() == digest, name
    root = payload/'GMAT-Qt.app' if args.platform.startswith('macos') else payload
    binary = root/'Contents/MacOS' if args.platform.startswith('macos') else root/'bin'
    metadata_path = root/'Contents/Resources/PYTHON-API.json' if args.platform.startswith('macos') else root/'PYTHON-API.json'
    metadata = json.loads(metadata_path.read_text())
    qt, osg, prefix = builder.dependencies(args.platform)
    environment = windows_build_environment() if os.name == 'nt' else dict(os.environ)
    environment['GMAT_QT_TUTORIAL_RUNTIME'] = str(binary/('GmatQt.exe' if os.name == 'nt' else 'GmatQt'))
    build = output/'probe-build'
    options = ['-DCMAKE_BUILD_TYPE=Release', '-DGMAT_INSTALLATION='+str(root),
               '-DCSPICE_DIR='+str(builder.DEPS/'cspice'),
               '-DCMAKE_PREFIX_PATH='+str(qt)+';'+str(osg)+';'+str(prefix)]
    if os.name == 'nt':
        imports = output/'import-libraries'
        import_libraries(binary, imports, environment)
        options += ['-G', 'Visual Studio 17 2022', '-A', 'x64', '-DGMAT_IMPORT_LIBRARIES='+str(imports)]
        environment['PATH'] = os.pathsep.join([str(binary), str(qt/'bin'), str(osg/'bin'),
                                              str(prefix/'bin'), environment['PATH']])
        environment['QT_PLUGIN_PATH'] = str(qt/'plugins')
    elif args.platform.startswith('macos'):
        options += ['-DCMAKE_OSX_DEPLOYMENT_TARGET=15.0',
                    '-DCMAKE_OSX_ARCHITECTURES='+('arm64' if args.platform.endswith('arm64') else 'x86_64')]
        environment['DYLD_LIBRARY_PATH'] = str(binary)+':'+str(root/'Contents/Frameworks')
    else:
        environment['LD_LIBRARY_PATH'] = os.pathsep.join([str(binary), str(root/'lib'), str(qt/'lib')])
        environment['QT_PLUGIN_PATH'] = str(qt/'plugins')
        environment['GMAT_QT_TUTORIAL_PLUGIN_PATH'] = str(qt/'plugins')
    builder.run(['cmake', '-S', builder.REPO/'tools/preview/tutorial-probe', '-B', build, *options],
                'tutorial-probe-configure', env=environment)
    builder.run(['cmake', '--build', build, '--config', 'Release', '--parallel', builder.BUILD_JOBS],
                'tutorial-probe-build', env=environment)
    suffix = '.exe' if os.name == 'nt' else ''
    probe = next(build.rglob('GmatQtTutorialProbe'+suffix))
    driver = next(build.rglob('GmatQtTutorialDriverTests'+suffix))
    if os.name == 'nt':
        software = qt/'bin/opengl32sw.dll'
        if software.is_file():
            shutil.copy2(software, probe.parent/'opengl32.dll')
            environment['QT_OPENGL_DLL'] = str(probe.parent/'opengl32.dll')
    builder.run([driver], 'tutorial-semantic-driver', env=dict(environment, QT_QPA_PLATFORM='offscreen'))
    startup = payload/'gmat_startup_qt.txt' if args.platform.startswith('macos') else binary/'gmat_startup_qt.txt'
    desktop = 'isolated-x11' if args.platform.startswith('linux') else 'native'
    command = [sys.executable, builder.REPO/'src/qtgui/tests/RunTutorials.py', '--desktop', desktop,
        '--probe', probe, '--startup', startup, '--pause-ms', '0', '--input-delay-ms', '20',
        '--output', output/'replay']
    selected = args.tutorial or [str(number) for number in range(1, 19)]
    unavailable = []
    if not re.search(r'(?im)^PLUGIN\s*=.*VF13ad', startup.read_text()):
        unavailable = [{'tutorial': '05-optimal-lunar-flyby', 'status': 'unavailable',
                        'reason': 'The standard alpha.3 archive excludes the required VF13ad optimizer plugin.'}]
        selected = [number for number in selected if number not in ['5', '05', '05-optimal-lunar-flyby']]
    for tutorial in selected:
        command += ['--tutorial', tutorial]
    with (output/'replay.log').open('w') as log:
        result = subprocess.run(list(map(str, command)), env=environment, stdout=log, stderr=subprocess.STDOUT)
    receipt = {'platform': args.platform, 'archive': str(args.archive),
        'archive_sha256': hashlib.sha256(args.archive.read_bytes()).hexdigest(),
        'release_source': metadata['source_commit'],
        'harness_source': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=builder.REPO, text=True).strip(),
        'probe_sha256': hashlib.sha256(probe.read_bytes()).hexdigest(), 'exit_code': result.returncode,
        'extracted_root': str(root), 'replay': str(output/'replay/results.json'),
        'observer': 'Matching frontend source linked to the archived GMAT native libraries',
        'desktop': desktop, 'unavailable_tutorials': unavailable, 'full_regression_run': False}
    (output/'verification.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(receipt))
    return result.returncode


if __name__ == '__main__':
    sys.exit(main())
