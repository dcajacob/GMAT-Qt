"""Build, relocate and probe a private tester package on its native OS."""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import plistlib
import re
import shutil
import subprocess
import sys
import tarfile
import urllib.request
import zipfile

REPO = Path(__file__).resolve().parents[2]
ROOT = REPO / 'build/preview-package'
LOGS = ROOT / 'logs'
DEPS = ROOT / 'deps'


def run(command, name, *, env=None, cwd=None):
    print(name + ': ' + ' '.join(map(str, command)), flush=True)
    with (LOGS / (name + '.log')).open('w', encoding='utf-8') as stream:
        result = subprocess.run(list(map(str, command)), cwd=cwd or REPO,
                                env=env, stdout=stream, stderr=subprocess.STDOUT)
    if result.returncode:
        print((LOGS / (name + '.log')).read_text(errors='replace')[-14000:], flush=True)
        raise RuntimeError(f'{name} failed with exit {result.returncode}')


def download(url, target):
    with urllib.request.urlopen(url, timeout=180) as response, target.open('wb') as out:
        shutil.copyfileobj(response, out)
    return hashlib.sha256(target.read_bytes()).hexdigest()


def copy_file(source, target):
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)


def dependencies(target):
    receipts = {}
    if target.startswith('windows'):
        run([sys.executable, '-m', 'pip', 'install', 'aqtinstall==3.3.0'], 'aqt-install')
        run([sys.executable, '-m', 'aqt', 'install-qt', 'windows', 'desktop',
             '6.10.2', 'win64_msvc2022_64', '-O', DEPS / 'qt'], 'qt-install')
        qt = DEPS / 'qt/6.10.2/msvc2022_64'
        vcpkg = Path(os.environ['VCPKG_INSTALLATION_ROOT'])
        run([vcpkg / 'vcpkg.exe', 'install', 'xerces-c:x64-windows',
             'freetype:x64-windows'], 'vcpkg-install')
        prefix = vcpkg / 'installed/x64-windows'
        toolkit = 'PC_Windows_VisualC_64bit'
        archive = DEPS / 'cspice.zip'
        receipts['CSPICE'] = download(f'https://naif.jpl.nasa.gov/pub/naif/toolkit/C/{toolkit}/packages/cspice.zip', archive)
        with zipfile.ZipFile(archive) as source:
            source.extractall(DEPS)
        osg_archive = DEPS / 'osg.tar.gz'
        receipts['OpenSceneGraph'] = download('https://github.com/openscenegraph/OpenSceneGraph/archive/refs/tags/OpenSceneGraph-3.6.5.tar.gz', osg_archive)
        with tarfile.open(osg_archive) as source:
            source.extractall(DEPS, filter='data')
        osg = DEPS / 'osg'
        options = ['-DBUILD_OSG_APPLICATIONS=OFF', '-DBUILD_OSG_EXAMPLES=OFF',
                   '-DBUILD_OSG_PLUGINS_BY_DEFAULT=OFF', '-DBUILD_OSG_PLUGIN_3DS=ON',
                   '-DBUILD_OSG_PLUGIN_OBJ=ON', '-DBUILD_OSG_PLUGIN_OSG=ON',
                   '-DBUILD_OSG_PLUGIN_FREETYPE=ON', '-DDYNAMIC_OPENSCENEGRAPH=ON',
                   '-DDYNAMIC_OPENTHREADS=ON', '-DOSG_MSVC_VERSIONED_DLL=OFF',
                   '-DCMAKE_POLICY_VERSION_MINIMUM=3.5', f'-DCMAKE_INSTALL_PREFIX={osg}',
                   f'-DCMAKE_TOOLCHAIN_FILE={vcpkg}/scripts/buildsystems/vcpkg.cmake']
        run(['cmake', '-S', DEPS / 'OpenSceneGraph-OpenSceneGraph-3.6.5',
             '-B', DEPS / 'osg-build', '-G', 'Visual Studio 17 2022', '-A', 'x64', *options], 'osg-configure')
        run(['cmake', '--build', DEPS / 'osg-build', '--config', 'Release', '--parallel', '2'], 'osg-build')
        run(['cmake', '--install', DEPS / 'osg-build', '--config', 'Release'], 'osg-install')
    else:
        run(['brew', 'install', 'qt', 'open-scene-graph', 'xerces-c'], 'brew-install')
        brew = lambda formula: Path(subprocess.check_output(['brew', '--prefix', formula], text=True).strip())
        qt, osg, prefix = brew('qt'), brew('open-scene-graph'), brew('xerces-c')
        toolkit = 'MacM1_OSX_clang_64bit' if target.endswith('arm64') else 'MacIntel_OSX_AppleC_64bit'
        archive = DEPS / 'cspice.tar.Z'
        receipts['CSPICE'] = download(f'https://naif.jpl.nasa.gov/pub/naif/toolkit/C/{toolkit}/packages/cspice.tar.Z', archive)
        run(['tar', '-xf', archive, '-C', DEPS], 'cspice-extract')
    (LOGS / 'dependency-downloads.json').write_text(json.dumps(receipts, indent=2) + '\n')
    return qt, osg, prefix


def build(target, qt, osg, prefix):
    options = ['-DCMAKE_BUILD_TYPE=Release', '-DGMAT_INCLUDE_QT_GUI=ON',
               '-DGMAT_INCLUDE_GUI=OFF', '-DGMAT_QT_BUILD_TESTS=OFF',
               '-DGMAT_INCLUDE_API=OFF', '-DPLUGIN_MATLABINTERFACE=OFF',
               '-DPLUGIN_FMINCONOPTIMIZER=OFF', '-DPLUGIN_PYTHONINTERFACE=OFF',
               '-DPLUGIN_EXTERNALFORCEMODEL=OFF', '-DPLUGIN_OVTOOFI=OFF',
               '-DPLUGIN_CINTERFACE=OFF', '-DGMAT_QT_EXTERNAL_PLUGINS=',
               f'-DCSPICE_DIR={DEPS / "cspice"}',
               f'-DCMAKE_PREFIX_PATH={qt};{osg};{prefix}']
    if target.startswith('windows'):
        options += ['-G', 'Visual Studio 17 2022', '-A', 'x64']
    else:
        architecture = 'arm64' if target.endswith('arm64') else 'x86_64'
        options += ['-DCMAKE_OSX_DEPLOYMENT_TARGET=15.0',
                    f'-DCMAKE_OSX_ARCHITECTURES={architecture}']
    run(['cmake', '-S', REPO, '-B', ROOT / 'gmat-build', *options], 'gmat-configure')
    run(['cmake', '--build', ROOT / 'gmat-build', '--config', 'Release',
         '--target', 'GmatQt', '--parallel', '2'], 'gmat-build')


def copy_payload(destination):
    # Copy only Git-owned runtime data and examples, never local outputs or SDKs.
    paths = subprocess.check_output(['git', 'ls-files', '-z', '--', 'application'], cwd=REPO).decode().split('\0')
    for name in paths:
        if not name:
            continue
        relative = Path(name).relative_to('application')
        if relative.parts[0] in {'data', 'samples', 'userfunctions', 'userincludes', 'extras'}:
            copy_file(REPO / name, destination / relative)
    for folder in ['output', 'plugins', 'bin', 'utilities', 'matlab', 'docs/help']:
        (destination / folder).mkdir(parents=True, exist_ok=True)
    copy_file(REPO / 'License.txt', destination / 'License.txt')
    copy_file(REPO / 'TESTING.rst', destination / 'TESTING.rst')


def collect_notices(destination, qt, osg, prefix):
    notices = destination / 'THIRD-PARTY-NOTICES'
    notices.mkdir(parents=True)
    download('https://www.gnu.org/licenses/lgpl-3.0.txt', notices / 'LGPL-3.0.txt')
    download('https://www.gnu.org/licenses/gpl-3.0.txt', notices / 'GPL-3.0.txt')
    roots = {'Qt': qt, 'OpenSceneGraph': osg, 'Xerces-and-dependencies': prefix,
             'CSPICE': DEPS / 'cspice'}
    if os.name == 'nt':
        roots['OpenSceneGraph-source'] = DEPS / 'OpenSceneGraph-OpenSceneGraph-3.6.5'
    else:
        cellar = Path(subprocess.check_output(['brew', '--cellar'], text=True).strip())
        for formula in cellar.iterdir():
            if formula.is_dir():
                for keg in formula.iterdir():
                    if keg.is_dir():
                        roots['Homebrew-' + formula.name + '-' + keg.name] = keg
    accepted = ('license', 'licence', 'copying', 'copyright', 'notice')
    for label, root in roots.items():
        if not root.exists():
            continue
        candidates = list(root.glob('*'))
        for folder in ['LICENSES', 'Licenses', 'share/licenses', 'share/doc', 'share', '.brew']:
            if (root / folder).is_dir():
                candidates += list((root / folder).rglob('*'))
        if label == 'OpenSceneGraph-source':
            candidates += list((root / 'src/osgPlugins/3ds/lib3ds').glob('*.h'))
        for path in set(candidates):
            if path.is_file() and (path.name.lower().startswith(accepted) or
                                   path.parent.name == '.brew' or
                                   (label == 'OpenSceneGraph-source' and path.name == 'lib3ds.h')):
                copy_file(path, notices / label / path.relative_to(root))
    (notices / 'README.txt').write_text(
        'This preview dynamically links Qt, OpenSceneGraph and Xerces-C.\n'
        'Installed license notices and Homebrew build recipes are retained here.\n'
        'Qt source and licensing: https://code.qt.io/cgit/qt/ and https://www.qt.io/licensing/open-source-lgpl-obligations\n'
        'Windows Qt version: 6.10.2. Mac Qt version and patches are identified by the bundled Homebrew recipe.\n'
        'OpenSceneGraph source: https://github.com/openscenegraph/OpenSceneGraph/tree/OpenSceneGraph-3.6.5\n'
        'Xerces-C source: https://xerces.apache.org/xerces-c/\n'
        'CSPICE toolkit, source and rules: https://naif.jpl.nasa.gov/naif/toolkit.html and https://naif.jpl.nasa.gov/naif/rules.html\n'
        'Libraries can be replaced with compatible builds. GMAT source and package recipes are in the private GMAT-Qt repository.\n', encoding='utf-8')


def macho(path):
    if not path.is_file():
        return False
    with path.open('rb') as stream:
        return stream.read(4) in {b'\xcf\xfa\xed\xfe', b'\xfe\xed\xfa\xcf', b'\xca\xfe\xba\xbe'}


def relocate_macos(bundle):
    frameworks = bundle / 'Contents/Frameworks'
    frameworks.mkdir(exist_ok=True)
    binary = bundle / 'Contents/MacOS/GmatQt'
    queue = [p for p in bundle.rglob('*') if macho(p)]
    done = set()
    copied_sources = {}
    while queue:
        path = queue.pop()
        if path in done:
            continue
        done.add(path)
        libraries = subprocess.check_output(['otool', '-L', path], text=True).splitlines()[1:]
        identities = subprocess.check_output(['otool', '-D', path], text=True).splitlines()[1:]
        for line in libraries:
            old = line.strip().split(' (compatibility version', 1)[0]
            if old in identities:
                continue
            if old.startswith(('/System/', '/usr/lib/', '@executable_path/../Frameworks/')):
                continue
            if old.startswith('@loader_path/'):
                source = path.parent / old[len('@loader_path/'):]
            elif old.startswith('@rpath/'):
                # Qt's deploy tool resolves its framework references first.
                candidates = [frameworks / old[len('@rpath/'):], path.parent / old[len('@rpath/'):],
                              binary.parent / old[len('@rpath/'):]]
                source = next((p for p in candidates if p.exists()), None)
                if source is None:
                    raise RuntimeError(f'Unresolved runtime dependency {old} in {path}')
            else:
                source = Path(old)
            if not source.exists():
                raise RuntimeError(f'Missing runtime dependency {old} in {path}')
            if source.resolve() == path.resolve():
                continue
            try:
                relative = source.resolve().relative_to(frameworks.resolve())
                replacement = '@executable_path/../Frameworks/' + str(relative)
            except ValueError:
                target = frameworks / source.name
                identity = hashlib.sha256(source.read_bytes()).hexdigest()
                if target.exists():
                    if target in copied_sources:
                        assert copied_sources[target] == identity, f'Dependency name collision: {source}'
                    elif target.read_bytes() != source.read_bytes():
                        # macdeployqt may already have relocated/stripped this
                        # library. Its link UUID survives those path changes.
                        uuids = lambda p: set(re.findall(r'\buuid\s+([A-Fa-f0-9-]+)',
                            subprocess.check_output(['otool', '-l', p], text=True)))
                        source_uuids = uuids(source)
                        assert source_uuids and source_uuids == uuids(target), f'Dependency name collision: {source}'
                    copied_sources[target] = identity
                else:
                    copy_file(source, target)
                    copied_sources[target] = identity
                    target.chmod(0o755)
                    queue.append(target)
                replacement = '@executable_path/../Frameworks/' + source.name
            run(['install_name_tool', '-change', old, replacement, path], 'relocate-' + str(len(done)) + '-' + str(len(libraries)) + '-' + hashlib.sha256(old.encode()).hexdigest()[:10])
        if path.suffix in {'.dylib', '.so'}:
            subprocess.run(['install_name_tool', '-id', '@executable_path/../Frameworks/' + path.name, path], check=True)
    for path in sorted(done, key=lambda p: len(p.parts), reverse=True):
        subprocess.run(['codesign', '--force', '--sign', '-', path], check=True)


def stage_package(target, qt, osg, prefix):
    payload = ROOT / 'staging' / ('GMAT-Qt-' + target)
    copy_payload(payload)
    collect_notices(payload, qt, osg, prefix)
    suffix = '.dll' if target.startswith('windows') else '.dylib'
    for path in (REPO / 'application/bin').glob('*' + suffix):
        copy_file(path, payload / 'bin' / path.name)
    for path in (REPO / 'application/plugins').rglob('*' + suffix):
        copy_file(path, payload / 'plugins' / path.relative_to(REPO / 'application/plugins'))
    startup = REPO / 'application/bin/gmat_startup_qt.txt'
    assert not re.search(r'(?im)^PLUGIN\s*=.*(Matlab|Fmincon|VF13ad)', startup.read_text())
    copy_file(startup, payload / 'bin/gmat_startup_qt.txt')
    if target.startswith('windows'):
        copy_file(REPO / 'application/bin/GmatQt.exe', payload / 'bin/GmatQt.exe')
        for directory in [osg / 'bin', prefix / 'bin']:
            for path in directory.glob('*.dll'):
                copy_file(path, payload / 'bin' / path.name)
        for directory in osg.rglob('osgPlugins-*'):
            if directory.is_dir() and 'debug' not in directory.parts:
                shutil.copytree(directory, payload / 'bin' / directory.name, dirs_exist_ok=True)
        run([qt / 'bin/windeployqt.exe', '--release', '--no-translations',
             '--compiler-runtime', payload / 'bin/GmatQt.exe'], 'windeployqt')
        # The normal desktop platform remains qwindows. Retain the optional
        # offscreen plugin too so the packaged-binary mission probe is portable.
        for path in (qt / 'plugins/platforms').glob('*offscreen*.dll'):
            copy_file(path, payload / 'bin/platforms' / path.name)
        return payload, payload / 'bin/GmatQt.exe'
    bundle = payload / 'GMAT-Qt.app'
    contents = bundle / 'Contents'
    contents.mkdir(parents=True)
    for path in list(payload.iterdir()):
        if path == bundle or path.name == 'output':
            continue
        shutil.move(str(path), contents / ('MacOS' if path.name == 'bin' else path.name))
    startup_path = contents / 'MacOS/gmat_startup_qt.txt'
    startup_path.write_text(re.sub(r'(?m)^OUTPUT_PATH\s*=.*$',
                                 'OUTPUT_PATH = ../../../output/', startup_path.read_text()))
    binary = contents / 'MacOS/GmatQt'
    copy_file(REPO / 'application/bin/GmatQt', binary)
    binary.chmod(0o755)
    (contents / 'Info.plist').write_bytes(plistlib.dumps({
        'CFBundleExecutable': 'GmatQt', 'CFBundleIdentifier': 'com.dcajacob.gmat-qt.preview',
        'CFBundleName': 'GMAT-Qt', 'CFBundlePackageType': 'APPL',
        'CFBundleShortVersionString': '2026.11.04', 'LSMinimumSystemVersion': '15.0',
        'NSHighResolutionCapable': True}))
    for directory in (osg / 'lib').glob('osgPlugins-*'):
        for path in directory.iterdir():
            if path.is_file() and path.name.startswith(('osgdb_3ds.', 'osgdb_obj.', 'osgdb_osg.', 'osgdb_freetype.')):
                copy_file(path, contents / 'MacOS' / directory.name / path.name)
    run([qt / 'bin/macdeployqt', bundle, '-always-overwrite'], 'macdeployqt')
    for path in (qt / 'plugins/platforms').glob('*offscreen*.dylib'):
        copy_file(path, contents / 'PlugIns/platforms' / path.name)
    relocate_macos(bundle)
    return payload, binary


def probe(target, payload, executable):
    root = payload if target.startswith('windows') else payload / 'GMAT-Qt.app/Contents'
    report = ROOT / 'probe-values.csv'
    radius, mu = 7000., 398600.4415
    script = ROOT / 'probe.script'
    script.write_text(f"""Create Spacecraft Sat;
Sat.DateFormat = A1ModJulian;
Sat.Epoch = '21545';
Sat.CoordinateSystem = EarthMJ2000Eq;
Sat.DisplayStateType = Cartesian;
Sat.X = {radius};
Sat.Y = 0;
Sat.Z = 0;
Sat.VX = 0;
Sat.VY = {math.sqrt(mu / radius):.17g};
Sat.VZ = 0;
Create ForceModel FM;
FM.PrimaryBodies = {{}};
FM.PointMasses = {{Earth}};
FM.Drag = None;
FM.SRP = Off;
Create Propagator P;
P.FM = FM;
P.Accuracy = 1e-12;
P.MinStep = 0;
P.MaxStep = 5;
Create ReportFile Values;
Values.Filename = '{report.as_posix()}';
Values.WriteHeaders = false;
Values.FixedWidth = false;
Values.Delimiter = ',';
Values.Precision = 17;
BeginMissionSequence;
Propagate P(Sat) {{Sat.ElapsedSecs = 600}};
Report Values Sat.ElapsedSecs Sat.X Sat.Y Sat.Z Sat.VX Sat.VY Sat.VZ;
""", encoding='utf-8')
    env = dict(os.environ, QT_QPA_PLATFORM='offscreen')
    for key in ['QT_PLUGIN_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH', 'DYLD_LIBRARY_PATH', 'DYLD_FRAMEWORK_PATH', 'OSG_LIBRARY_PATH']:
        env.pop(key, None)
    if target.startswith('windows'):
        env['PATH'] = str(executable.parent) + os.pathsep + os.environ.get('SystemRoot', 'C:/Windows') + '/System32'
    run([executable, script, '--run', '--screenshot', LOGS / 'package-probe.png',
         '--settings-dir', ROOT / 'probe-settings'], 'package-probe', env=env, cwd=ROOT)
    rows = report.read_text().strip().splitlines()
    assert len(rows) == 1
    values = list(map(float, rows[0].split(',')))
    assert len(values) == 7 and all(math.isfinite(v) for v in values)
    assert abs(values[0] - 600) < 2e-5
    omega = math.sqrt(mu / radius ** 3)
    c, s = math.cos(omega * values[0]), math.sin(omega * values[0])
    truth = [radius*c, radius*s, 0., -radius*omega*s, radius*omega*c, 0.]
    errors = [abs(a-b) for a,b in zip(values[1:], truth)]
    assert max(errors[:3]) < 2e-5 and max(errors[3:]) < 3e-8
    result = {'status': 'passed', 'platform': target, 'native_os': platform.platform(),
              'commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip(),
              'probe_values': values, 'independent_circle': truth,
              'max_position_error_km': max(errors[:3]), 'max_velocity_error_km_s': max(errors[3:]),
              'scope': 'Packaged executable native startup and point-mass mission; not full GUI or NASA qualification.'}
    (LOGS / 'package-verification.json').write_text(json.dumps(result, indent=2) + '\n')
    copy_file(LOGS / 'package-verification.json', root / 'PACKAGE-VERIFICATION.json')
    # Remove probe outputs from the reusable application bundle before archiving.
    for path in (payload / 'output').glob('*'):
        if path.is_file():
            path.unlink()
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform', required=True, choices=['windows-x64', 'macos-arm64', 'macos-x64'])
    target = parser.parse_args().platform
    assert (os.name == 'nt') == target.startswith('windows')
    assert target.startswith('windows') or sys.platform == 'darwin'
    LOGS.mkdir(parents=True, exist_ok=True)
    DEPS.mkdir(parents=True, exist_ok=True)
    qt, osg, prefix = dependencies(target)
    build(target, qt, osg, prefix)
    payload, executable = stage_package(target, qt, osg, prefix)
    probe(target, payload, executable)
    if sys.platform == 'darwin':
        bundle = payload / 'GMAT-Qt.app'
        run(['codesign', '--force', '--deep', '--sign', '-', bundle], 'adhoc-sign')
        run(['codesign', '--verify', '--deep', '--strict', bundle], 'verify-signature')
    dist = ROOT / 'dist'
    dist.mkdir(exist_ok=True)
    archive = dist / ('GMAT-Qt-preview-20261104-' + target + '.zip')
    if sys.platform == 'darwin':
        # ditto preserves bundle permissions and symlinks.
        run(['ditto', '-c', '-k', '--sequesterRsrc', '--keepParent', payload, archive], 'archive')
    else:
        shutil.make_archive(str(archive.with_suffix('')), 'zip', payload.parent, payload.name)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    (dist / (archive.name + '.sha256')).write_text(digest + '  ' + archive.name + '\n')
    copy_file(LOGS / 'package-verification.json', dist / (target + '-verification.json'))
    print('Verified package: ' + str(archive), flush=True)


if __name__ == '__main__':
    main()
