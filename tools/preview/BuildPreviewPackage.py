"""Build, relocate and probe a private tester package on its native OS."""
import argparse
import collections
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
import time
import urllib.request
import zipfile

REPO = Path(__file__).resolve().parents[2]
ROOT = REPO / 'build/preview-package'
LOGS = ROOT / 'logs'
DEPS = ROOT / 'deps'
BUILD_JOBS = str(max(1, min(4, os.cpu_count() or 2)))
PACKAGE_VERSION = None


def run(command, name, *, env=None, cwd=None):
    print(name + ': ' + ' '.join(map(str, command)), flush=True)
    log = LOGS / (name + '.log')
    with log.open('w', encoding='utf-8') as stream:
        process = subprocess.Popen(list(map(str, command)), cwd=cwd or REPO,
                                   env=env, stdout=stream, stderr=subprocess.STDOUT)
        while True:
            try:
                code = process.wait(timeout=45)
                break
            except subprocess.TimeoutExpired:
                # Preserve the complete log and expose bounded live progress so
                # a long native compile/download is distinguishable from a hang.
                with log.open('rb') as current:
                    current.seek(max(0, log.stat().st_size - 2048))
                    tail = current.read().decode('utf-8', errors='replace').splitlines()[-3:]
                print(name + ' still running:\n' + '\n'.join(tail), flush=True)
    if code:
        print((LOGS / (name + '.log')).read_text(errors='replace')[-14000:], flush=True)
        raise RuntimeError(f'{name} failed with exit {code}')


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
        for attempt in range(1, 4):
            name = 'vcpkg-install-' + str(attempt)
            try:
                run([vcpkg / 'vcpkg.exe', 'install', 'xerces-c:x64-windows',
                     'freetype:x64-windows'], name)
                break
            except RuntimeError:
                log = (LOGS / (name + '.log')).read_text(errors='replace')
                if attempt == 3 or not re.search(r'error code 92|HTTP/2 framing', log):
                    raise
                print('Retrying the dependency download after an HTTP/2 stream failure', flush=True)
                time.sleep(10)
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
        options = ['-DBUILD_OSG_APPLICATIONS=OFF', '-DBUILD_OSG_EXAMPLES=OFF', '-DWIN32_USE_MP=ON',
                   '-DBUILD_OSG_PLUGINS_BY_DEFAULT=OFF', '-DBUILD_OSG_PLUGIN_3DS=ON',
                   '-DBUILD_OSG_PLUGIN_OBJ=ON', '-DBUILD_OSG_PLUGIN_OSG=ON',
                   '-DBUILD_OSG_PLUGIN_FREETYPE=ON', '-DDYNAMIC_OPENSCENEGRAPH=ON',
                   '-DDYNAMIC_OPENTHREADS=ON', '-DOSG_MSVC_VERSIONED_DLL=OFF',
                   '-DCMAKE_POLICY_VERSION_MINIMUM=3.5', f'-DCMAKE_INSTALL_PREFIX={osg}',
                   f'-DCMAKE_TOOLCHAIN_FILE={vcpkg}/scripts/buildsystems/vcpkg.cmake']
        sdk_record = osg / 'preview-sdk-sha256.json'
        if sdk_record.exists():
            sdk = json.loads(sdk_record.read_text())
            assert sdk['source_sha256'] == receipts['OpenSceneGraph']
            assert all(hashlib.sha256((osg / n).read_bytes()).hexdigest() == h
                       for n, h in sdk['file_sha256'].items())
            print('Verified cached Windows graphics SDK', flush=True)
        else:
            run(['cmake', '-S', DEPS / 'OpenSceneGraph-OpenSceneGraph-3.6.5',
                 '-B', DEPS / 'osg-build', '-G', 'Visual Studio 17 2022', '-A', 'x64', *options], 'osg-configure')
            run(['cmake', '--build', DEPS / 'osg-build', '--config', 'Release', '--parallel', BUILD_JOBS], 'osg-build')
            run(['cmake', '--install', DEPS / 'osg-build', '--config', 'Release'], 'osg-install')
            sdk_record.write_text(json.dumps({'source_sha256': receipts['OpenSceneGraph'],
                'file_sha256': {p.relative_to(osg).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                                for p in osg.rglob('*') if p.is_file()}}, indent=2) + '\n')
    elif target.startswith('linux'):
        run([sys.executable, '-m', 'pip', 'install', 'aqtinstall==3.3.0'], 'aqt-install')
        run([sys.executable, '-m', 'aqt', 'install-qt', 'linux', 'desktop',
             '6.10.2', 'linux_gcc_64', '-O', DEPS / 'qt'], 'qt-install')
        qt, osg, prefix = DEPS / 'qt/6.10.2/gcc_64', Path('/usr'), Path('/usr')
        archive = DEPS / 'cspice.tar.Z'
        receipts['CSPICE'] = download(
            'https://naif.jpl.nasa.gov/pub/naif/toolkit/C/PC_Linux_GCC_64bit/packages/cspice.tar.Z', archive)
        run(['tar', '-xf', archive, '-C', DEPS], 'cspice-extract')
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


def build(target, qt, osg, prefix, *, extra_options=(), targets=('GmatQt',)):
    build_env = None
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
        # gmatdefs.hpp includes Windows headers for every native plugin. Keep
        # their min/max macros from replacing standard C++ calls (including
        # Yukon), just as the Qt frontend's existing NOMINMAX definition does.
        # CL supplements MSVC's normal flags instead of replacing CMake's
        # exception-handling, runtime-library or release configuration.
        build_env = dict(os.environ)
        build_env['CL'] = (build_env.get('CL', '') + ' /DNOMINMAX /MP' + BUILD_JOBS).strip()
        print('MSVC native build: add /DNOMINMAX and file-level parallel compilation', flush=True)
    elif target.startswith('macos'):
        architecture = 'arm64' if target.endswith('arm64') else 'x86_64'
        options += ['-DCMAKE_OSX_DEPLOYMENT_TARGET=15.0',
                    f'-DCMAKE_OSX_ARCHITECTURES={architecture}']
    options.extend(extra_options)
    run(['cmake', '-S', REPO, '-B', ROOT / 'gmat-build', *options], 'gmat-configure', env=build_env)
    run(['cmake', '--build', ROOT / 'gmat-build', '--config', 'Release',
         '--target', *targets, '--parallel', '1' if target.startswith('windows') else BUILD_JOBS],
        'gmat-build', env=build_env)


def production_tree():
    data = subprocess.check_output(['git', 'ls-tree', '-r', 'HEAD', '--',
        'CMakeLists.txt', 'src', 'plugins', 'application', 'depends',
        'build/cmake_modules', 'build/install'], cwd=REPO)
    return hashlib.sha256(data).hexdigest()


def preserve_native(target, qt, osg, prefix, *, extra_files=()):
    """Retain exact compiled inputs before any subsequent packaging failure."""
    paths = {}
    for root in [REPO / 'application/bin', REPO / 'application/plugins']:
        for path in root.rglob('*'):
            if path.is_file() and (path.suffix in {'.dll', '.dylib', '.so', '.exe'} or '.so.' in path.name or
                                   path.name in {'GmatQt', 'gmat_startup_qt.txt'}):
                paths[path.relative_to(REPO).as_posix()] = path
    for path in extra_files:
        paths[path.relative_to(REPO).as_posix()] = path
    if target.startswith('windows'):
        for root, arcroot in [(qt, qt.relative_to(REPO)), (osg, osg.relative_to(REPO)),
                              (prefix / 'bin', DEPS.relative_to(REPO) / 'package-vcpkg/bin'),
                              (prefix / 'share', DEPS.relative_to(REPO) / 'package-vcpkg/share'),
                              (DEPS / 'OpenSceneGraph-OpenSceneGraph-3.6.5',
                               DEPS.relative_to(REPO) / 'OpenSceneGraph-OpenSceneGraph-3.6.5')]:
            for path in root.rglob('*'):
                relative = path.relative_to(root)
                if not path.is_file() or 'include' in relative.parts or path.suffix in {'.pdb', '.lib', '.a'}:
                    continue
                if root.name.startswith('OpenSceneGraph-') and not (
                    path.name.lower().startswith(('license', 'copying', 'copyright')) or path.name == 'lib3ds.h'):
                    continue
                paths[(arcroot / relative).as_posix()] = path
        prefix = DEPS / 'package-vcpkg'
    metadata = {'platform': target, 'production_tree_sha256': production_tree(),
        'source_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip(),
        'qt': str(qt), 'osg': str(osg), 'prefix': str(prefix),
        'file_sha256': {n: hashlib.sha256(p.read_bytes()).hexdigest() for n,p in paths.items()}}
    context = ROOT / 'native-build-context.json'
    context.write_text(json.dumps(metadata, indent=2) + '\n')
    archive = ROOT / ('native-inputs-' + target + '.tar.gz')
    with tarfile.open(archive, 'w:gz', compresslevel=1, dereference=True) as out:
        for name, path in paths.items():
            out.add(path, arcname=name, recursive=False)
        out.add(context, arcname=context.relative_to(REPO).as_posix(), recursive=False)
    print('Preserved native inputs: ' + str(archive), flush=True)


def restore_native(target):
    archive = ROOT / 'reuse-native' / ('native-inputs-' + target + '.tar.gz')
    if not archive.exists():
        return None
    with tarfile.open(archive) as source:
        metadata = json.load(source.extractfile('build/preview-package/native-build-context.json'))
        assert metadata['platform'] == target and metadata['production_tree_sha256'] == production_tree()
        assert all(m.isfile() and not Path(m.name).is_absolute() and '..' not in Path(m.name).parts
                   for m in source.getmembers())
        source.extractall(REPO, filter='data')
    assert all(hashlib.sha256((REPO / n).read_bytes()).hexdigest() == h
               for n,h in metadata['file_sha256'].items())
    print('Restored exact native binaries; production tree matches ' + metadata['source_commit'], flush=True)
    return metadata


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
    license_root = REPO / 'tools/preview/licenses'
    for name, record in json.loads((license_root / 'SOURCE.json').read_text()).items():
        source = license_root / name
        assert hashlib.sha256(source.read_bytes()).hexdigest() == record['sha256']
        copy_file(source, notices / name)
    roots = {'Qt': qt, 'OpenSceneGraph': osg, 'Xerces-and-dependencies': prefix,
             'CSPICE': DEPS / 'cspice'}
    if sys.platform.startswith('linux'):
        roots = {'Qt': qt, 'CSPICE': DEPS / 'cspice'}
    if os.name == 'nt':
        roots['OpenSceneGraph-source'] = DEPS / 'OpenSceneGraph-OpenSceneGraph-3.6.5'
    elif sys.platform == 'darwin':
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
        'Libraries can be replaced with compatible builds. GMAT source and package recipes: https://github.com/dcajacob/GMAT-Qt\n', encoding='utf-8')


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
        path.chmod(path.stat().st_mode | 0o200)
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
        if path != binary:
            subprocess.run(['codesign', '--force', '--sign', '-', path], check=True)
    run(['codesign', '--force', '--sign', '-', bundle], 'stage-sign')


def stage_linux(payload, qt, osg):
    copy_file(REPO / 'application/bin/GmatQt', payload / 'bin/GmatQt')
    for source in (qt / 'plugins').rglob('*.so'):
        if source.parent.name in {'platforms', 'imageformats', 'iconengines', 'styles',
                                  'xcbglintegrations', 'wayland-shell-integration',
                                  'wayland-graphics-integration-client', 'wayland-decoration-client', 'tls'}:
            copy_file(source, payload / 'qt/plugins' / source.relative_to(qt / 'plugins'))
    for directory in (osg / 'lib/x86_64-linux-gnu').glob('osgPlugins-*'):
        for source in directory.glob('*.so'):
            if source.stem in {'osgdb_3ds', 'osgdb_obj', 'osgdb_osg', 'osgdb_freetype',
                               'osgdb_png', 'osgdb_jpeg', 'osgdb_serializers_osg'}:
                copy_file(source, payload / 'osgPlugins' / source.name)

    def is_elf(path):
        if not path.is_file():
            return False
        with path.open('rb') as stream:
            return stream.read(4) == b'\x7fELF'

    queue = collections.deque(path for path in payload.rglob('*') if is_elf(path))
    visited = set()
    host_libraries = {}
    system_sources = set()
    requirements = set()
    package_libraries = str(payload / 'bin') + ':' + str(payload / 'lib')
    env = dict(os.environ, LD_LIBRARY_PATH=package_libraries + ':' + str(qt / 'lib'))
    while queue:
        binary = queue.popleft()
        if binary in visited:
            continue
        visited.add(binary)
        versions = subprocess.check_output(['objdump', '-T', binary], text=True)
        requirements.update(re.findall(r'GLIBC_(\d+\.\d+)', versions))
        linked = subprocess.check_output(['ldd', binary], text=True, env=env)
        assert 'not found' not in linked, (str(binary), linked)
        for line in linked.splitlines():
            match = re.match(r'\s*(\S+)\s+=>\s+(/\S+)\s+\(', line)
            if not match:
                continue
            soname, name = match.groups()
            source = Path(name)
            if source.is_relative_to(payload):
                continue
            if soname.startswith(('libc.so', 'libm.so', 'libdl.so', 'libpthread.so',
                                  'librt.so', 'libresolv.so', 'libnss_', 'ld-linux')):
                host_libraries[soname] = name
                continue
            destination = payload / 'lib' / soname
            if not destination.exists():
                copy_file(source, destination)
                queue.append(destination)
                if str(source).startswith('/usr/lib/') or str(source).startswith('/lib/'):
                    system_sources.add(source.resolve())
    for binary in visited:
        linked = subprocess.check_output(['ldd', binary], text=True,
                                         env=dict(os.environ, LD_LIBRARY_PATH=package_libraries))
        assert 'not found' not in linked, (str(binary), linked)
    for source in sorted(system_sources):
        owner = subprocess.run(['dpkg-query', '-S', str(source)], capture_output=True, text=True)
        if owner.returncode:
            continue
        package = owner.stdout.split(': ', 1)[0].split(':', 1)[0]
        copyright_file = Path('/usr/share/doc') / package / 'copyright'
        if copyright_file.is_file():
            copy_file(copyright_file, payload / 'THIRD-PARTY-NOTICES' / package / 'copyright')
    runtime = {'host_libraries': host_libraries,
               'minimum_glibc': max(requirements, key=lambda value: tuple(map(int, value.split('.'))))}
    (LOGS / 'linux-runtime.json').write_text(json.dumps(runtime, indent=2) + '\n')
    launcher = '''#!/bin/sh
set -eu
gmat_alpha_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
export LD_LIBRARY_PATH="$gmat_alpha_root/bin:$gmat_alpha_root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$gmat_alpha_root/qt/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="$gmat_alpha_root/qt/plugins/platforms"
export OSG_LIBRARY_PATH="$gmat_alpha_root/osgPlugins"
exec "$gmat_alpha_root/bin/GmatQt" --startup "$gmat_alpha_root/bin/gmat_startup_qt.txt" "$@"
'''
    executable = payload / 'run-gmat'
    executable.write_text(launcher)
    executable.chmod(0o755)
    return executable


def stage_package(target, qt, osg, prefix):
    label = ('GMAT-Qt-' + PACKAGE_VERSION + '-' + target) if PACKAGE_VERSION else ('GMAT-Qt-' + target)
    payload = ROOT / 'staging' / label
    copy_payload(payload)
    collect_notices(payload, qt, osg, prefix)
    suffix = '.dll' if target.startswith('windows') else '.so*' if target.startswith('linux') else '.dylib'
    for path in (REPO / 'application/bin').glob('*' + suffix):
        copy_file(path, payload / 'bin' / path.name)
    for path in (REPO / 'application/plugins').rglob('*' + suffix):
        copy_file(path, payload / 'plugins' / path.relative_to(REPO / 'application/plugins'))
    startup = REPO / 'application/bin/gmat_startup_qt.txt'
    assert not re.search(r'(?im)^PLUGIN\s*=.*(Matlab|Fmincon|VF13ad)', startup.read_text())
    copy_file(startup, payload / 'bin/gmat_startup_qt.txt')
    if target.startswith('linux'):
        return payload, stage_linux(payload, qt, osg)
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
    resources = contents / 'Resources'
    resources.mkdir(parents=True)
    for path in list(payload.iterdir()):
        if path == bundle or path.name == 'output':
            continue
        destination = {'bin': contents / 'MacOS', 'plugins': contents / 'PlugIns'}.get(
            path.name, resources / path.name)
        shutil.move(str(path), destination)
    # main.cpp's existing Mac fallback finds this adjacent startup file. Keep
    # configuration and writable output outside the signed app; all data and
    # documents belong in Resources, never the code locations within Contents.
    startup_path = payload / 'gmat_startup_qt.txt'
    shutil.move(contents / 'MacOS/gmat_startup_qt.txt', startup_path)
    startup_text = re.sub(r'(?m)^ROOT_PATH\s*=.*$', 'ROOT_PATH = ../Resources/', startup_path.read_text())
    startup_path.write_text(re.sub(r'(?m)^OUTPUT_PATH\s*=.*$',
                                 'OUTPUT_PATH = ../../../output/', startup_text)
                            .replace('../plugins/', '../PlugIns/'))
    binary = contents / 'MacOS/GmatQt'
    copy_file(REPO / 'application/bin/GmatQt', binary)
    binary.chmod(0o755)
    (contents / 'Info.plist').write_bytes(plistlib.dumps({
        'CFBundleExecutable': 'GmatQt', 'CFBundleIdentifier': 'com.dcajacob.gmat-qt.preview',
        'CFBundleName': 'GMAT-Qt', 'CFBundlePackageType': 'APPL',
        'CFBundleVersion': '20261114' if PACKAGE_VERSION else '20261104',
        'CFBundleShortVersionString': PACKAGE_VERSION.split('-')[0] if PACKAGE_VERSION else '2026.11.04',
        'LSMinimumSystemVersion': '15.0',
        'NSHighResolutionCapable': True}))
    for directory in (osg / 'lib').glob('osgPlugins-*'):
        for path in directory.iterdir():
            if path.is_file() and path.name.startswith(('osgdb_3ds.', 'osgdb_obj.', 'osgdb_osg.', 'osgdb_freetype.')):
                # OSG's native bundle search includes Contents/PlugIns. A dotted
                # osgPlugins-3.6.5 directory is interpreted as an invalid nested
                # code bundle by macOS signing, so deploy readers directly here.
                copy_file(path, contents / 'PlugIns' / path.name)
    run(['clang++', '-std=c++17', '-arch', 'arm64' if target.endswith('arm64') else 'x86_64',
         '-mmacosx-version-min=15.0', '-Wl,-headerpad_max_install_names',
         '-I' + str(osg / 'include'), '-L' + str(osg / 'lib'),
         REPO / 'tools/preview/ModelReaderProbe.cpp', '-losgDB', '-losg', '-lOpenThreads',
         '-framework', 'Foundation', '-o', contents / 'MacOS/PackageReaderProbe'], 'reader-probe-build')
    run([qt / 'bin/macdeployqt', bundle, '-always-overwrite'], 'macdeployqt')
    qtpaths = next(path for path in [qt / 'bin/qtpaths', qt / 'bin/qtpaths6'] if path.is_file())
    plugin_root = Path(subprocess.check_output([qtpaths, '--query', 'QT_INSTALL_PLUGINS'], text=True).strip())
    offscreen = plugin_root / 'platforms/libqoffscreen.dylib'
    assert offscreen.is_file(), f'Qt offscreen plugin is missing from {plugin_root}'
    copy_file(offscreen, contents / 'PlugIns/platforms' / offscreen.name)
    relocate_macos(bundle)
    return payload, binary


def probe(target, payload, executable):
    root = payload / 'GMAT-Qt.app/Contents/Resources' if target.startswith('macos') else payload
    report = ROOT / 'probe-values.csv'
    saved = ROOT / 'saved-spacecraft.script'
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
Create OrbitView AlphaOrbit;
AlphaOrbit.Add = {{Sat, Earth}};
Create GroundTrackPlot AlphaGround;
AlphaGround.Add = {{Sat}};
Create XYPlot AlphaXY;
AlphaXY.XVariable = Sat.ElapsedSecs;
AlphaXY.YVariables = {{Sat.EarthMJ2000Eq.X}};
BeginMissionSequence;
Propagate P(Sat) {{Sat.ElapsedSecs = 600}};
Report Values Sat.ElapsedSecs Sat.X Sat.Y Sat.Z Sat.VX Sat.VY Sat.VZ;
Save Sat {{Filename = '{saved.as_posix()}'}};
""", encoding='utf-8')
    plan = {'duration_seconds': 600, 'radius_km': radius, 'mu_km3_s2': mu,
            'elapsed_bound_seconds': 2e-5, 'position_bound_km': 2e-5,
            'velocity_bound_km_s': 3e-8, 'oracle': 'analytic circular two-body motion',
            'bounds_reused_from': 'previous native package probe'}
    (LOGS / 'package-probe-plan.json').write_text(json.dumps(plan, indent=2) + '\n')
    env = dict(os.environ, QT_QPA_PLATFORM='offscreen')
    for key in ['QT_PLUGIN_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH', 'DYLD_LIBRARY_PATH', 'DYLD_FRAMEWORK_PATH', 'OSG_LIBRARY_PATH']:
        env.pop(key, None)
    if target.startswith('windows'):
        env['PATH'] = str(executable.parent) + os.pathsep + os.environ.get('SystemRoot', 'C:/Windows') + '/System32'
        # The offscreen font database needs an explicit directory on Windows.
        # The normal windows platform obtains installed fonts through the OS.
        env['QT_QPA_FONTDIR'] = str(Path(os.environ.get('SystemRoot', 'C:/Windows')) / 'Fonts')
    if target.startswith('linux'):
        env.update(QT_QPA_PLATFORM='xcb', LIBGL_ALWAYS_SOFTWARE='1')
    prefix = ['xvfb-run', '-a', '-s', '-screen 0 1600x1000x24'] if target.startswith('linux') else []
    command = prefix + [executable, script, '--run', '--screenshot', LOGS / 'package-probe.png',
                        '--settings-dir', ROOT / 'probe-settings']
    run(command, 'package-probe', env=env, cwd=ROOT)
    if target.startswith('macos'):
        reader = executable.parent / 'PackageReaderProbe'
        run([reader, root.parent, root / 'data/vehicle/models/aura.3ds'], 'model-reader-probe', env=env, cwd=ROOT)
        reader.unlink()
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
    assert 'Create Spacecraft Sat;' in saved.read_text()
    reopened = ROOT / 'reopened-values.csv'
    reopen_script = ROOT / 'reopen.script'
    reopen_script.write_text(saved.read_text() + f"""
Create ReportFile Reopened;
Reopened.Filename = '{reopened.as_posix()}';
Reopened.WriteHeaders = false;
Reopened.FixedWidth = false;
Reopened.Delimiter = ',';
Reopened.Precision = 17;
BeginMissionSequence;
Report Reopened Sat.X Sat.Y Sat.Z Sat.VX Sat.VY Sat.VZ;
""", encoding='utf-8')
    run(prefix + [executable, reopen_script, '--run', '--screenshot', LOGS / 'reopen-probe.png',
                  '--settings-dir', ROOT / 'reopen-settings'], 'reopen-probe', env=env, cwd=ROOT)
    restored_values = list(map(float, reopened.read_text().strip().split(',')))
    assert len(restored_values) == 6 and all(math.isfinite(v) for v in restored_values)
    restored_errors = [abs(a-b) for a,b in zip(restored_values, values[1:])]
    assert max(restored_errors[:3]) < 2e-5 and max(restored_errors[3:]) < 3e-8
    result = {'status': 'passed', 'platform': target, 'native_os': platform.platform(),
              'commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip(),
              'version': PACKAGE_VERSION, 'production_tree_sha256': production_tree(),
              'probe_values': values, 'independent_circle': truth,
              'max_position_error_km': max(errors[:3]), 'max_velocity_error_km_s': max(errors[3:]),
              'save_reopen_errors': restored_errors,
              'plots_created': ['OrbitView', 'GroundTrackPlot', 'XYPlot'],
              'graphics_profile': env['QT_QPA_PLATFORM'],
              'scope': 'Packaged startup, point-mass mission, reports, plot creation and spacecraft save/reopen.',
              'full_regression_run': False, 'full_current_build_qualified': False}
    if target.startswith('linux'):
        result['runtime'] = json.loads((LOGS / 'linux-runtime.json').read_text())
    if target.startswith('macos'):
        result['bundled_3ds_reader'] = {'status': 'passed', 'model': 'data/vehicle/models/aura.3ds',
                                        'lookup': 'Native bundle plugin path only; SDK paths excluded.'}
    (LOGS / 'package-verification.json').write_text(json.dumps(result, indent=2) + '\n')
    copy_file(LOGS / 'package-verification.json', root / 'PACKAGE-VERIFICATION.json')
    # Remove probe outputs from the reusable application bundle before archiving.
    for path in (payload / 'output').glob('*'):
        if path.is_file():
            path.unlink()
    return result


def main():
    global PACKAGE_VERSION
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform', required=True, choices=['linux-x86_64', 'windows-x64', 'macos-arm64', 'macos-x64'])
    parser.add_argument('--version', help='Release version recorded in the package and artifact name')
    args = parser.parse_args()
    target, PACKAGE_VERSION = args.platform, args.version
    if PACKAGE_VERSION and not re.fullmatch(r'[0-9][0-9A-Za-z.\-]*', PACKAGE_VERSION):
        parser.error('Version must contain only numbers, letters, periods and hyphens')
    assert (os.name == 'nt') == target.startswith('windows')
    assert target.startswith('windows') or target.startswith('linux') == sys.platform.startswith('linux')
    LOGS.mkdir(parents=True, exist_ok=True)
    DEPS.mkdir(parents=True, exist_ok=True)
    restored = restore_native(target)
    if restored and target.startswith('windows'):
        qt, osg, prefix = (Path(restored[n]) for n in ['qt', 'osg', 'prefix'])
    else:
        qt, osg, prefix = dependencies(target)
    if not restored:
        build(target, qt, osg, prefix)
        preserve_native(target, qt, osg, prefix)
    payload, executable = stage_package(target, qt, osg, prefix)
    probe(target, payload, executable)
    if sys.platform == 'darwin':
        bundle = payload / 'GMAT-Qt.app'
        run(['codesign', '--force', '--sign', '-', bundle], 'adhoc-sign')
        run(['codesign', '--verify', '--deep', '--strict', bundle], 'verify-signature')
    dist = ROOT / 'dist'
    dist.mkdir(exist_ok=True)
    name = 'GMAT-Qt-' + (PACKAGE_VERSION or 'preview-20261104') + '-' + target
    archive = dist / (name + ('.tar.gz' if target.startswith('linux') else '.zip'))
    if PACKAGE_VERSION:
        copy_file(REPO / 'RELEASE-NOTES.rst', payload / 'RELEASE-NOTES.rst')
    manifest = {str(path.relative_to(payload)): hashlib.sha256(path.read_bytes()).hexdigest()
                for path in sorted(payload.rglob('*')) if path.is_file()}
    (payload / 'PACKAGE-MANIFEST.json').write_text(json.dumps(manifest, indent=2) + '\n')
    if sys.platform == 'darwin':
        # ditto preserves bundle permissions and symlinks.
        run(['ditto', '-c', '-k', '--sequesterRsrc', '--keepParent', payload, archive], 'archive')
    elif target.startswith('linux'):
        with tarfile.open(archive, 'w:gz', dereference=False) as bundle:
            bundle.add(payload, arcname=payload.name)
    else:
        shutil.make_archive(str(archive.with_suffix('')), 'zip', payload.parent, payload.name)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    (dist / (archive.name + '.sha256')).write_text(digest + '  ' + archive.name + '\n')
    copy_file(LOGS / 'package-verification.json', dist / (target + '-verification.json'))
    print('Verified package: ' + str(archive), flush=True)


if __name__ == '__main__':
    main()
