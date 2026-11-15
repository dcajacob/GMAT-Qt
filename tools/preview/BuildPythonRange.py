"""Add CPython 3.12-3.15 bindings to an existing qualified native archive."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import tarfile
import zipfile

import BuildPreviewPackage as builder

VERSIONS = ('3.12', '3.13', '3.14', '3.15')


def run(command, log):
    with log.open('w', encoding='utf-8') as stream:
        subprocess.run(list(map(str, command)), stdout=stream, stderr=subprocess.STDOUT, check=True)


def unpack(archive, destination):
    destination.mkdir(parents=True)
    if archive.name.endswith('.tar.gz'):
        with tarfile.open(archive) as stream:
            stream.extractall(destination, filter='data')
    elif platform.system() == 'Darwin':
        subprocess.run(['ditto', '-x', '-k', str(archive), str(destination)], check=True)
    else:
        with zipfile.ZipFile(archive) as stream:
            stream.extractall(destination)
    return next(p for p in destination.iterdir() if p.is_dir() and p.name.startswith('GMAT-Qt-'))


def libraries(payload, target):
    root = payload/'GMAT-Qt.app/Contents' if target.startswith('macos') else payload
    found = {}
    for native in ('GmatUtil', 'GmatBase', 'Station', 'GmatEstimation'):
        pattern = 'lib'+native+'*.dylib' if target.startswith('macos') else 'lib'+native+('.dll' if target.startswith('windows') else '.so*')
        candidates = [p for p in root.rglob(pattern) if p.is_file()]
        # Linux dependency copies have directory-relative RPATHs. Link the
        # canonical runtime copy from bin or plugins, as the loader does.
        assert candidates, native
        directory = 'bin' if native in ('GmatUtil', 'GmatBase') else 'plugins'
        preferred = [p for p in candidates if p.parent.name == directory]
        found[native] = sorted(preferred or candidates, key=lambda p: len(str(p)))[0]
    return found


def windows_import_library(native, dll, folder):
    listing = subprocess.check_output(['dumpbin', '/exports', str(dll)], text=True)
    symbols = re.findall(r'^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+(\S+)', listing, re.M)
    assert symbols, dll
    definition = folder/(native+'.def')
    definition.write_text('LIBRARY '+dll.name+'\nEXPORTS\n'+'\n'.join(symbols)+'\n')
    output = folder/(native+'.lib')
    run(['lib', '/nologo', '/machine:x64', '/def:'+str(definition), '/out:'+str(output)], folder/(native+'-imports.log'))
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform', required=True)
    parser.add_argument('--archive-dir', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cspice-include', type=Path, required=True)
    parser.add_argument('--gui-prefix', help='Qt, OpenSceneGraph and Xerces prefixes for the affected GUI rebuild')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    archive, = [p for p in args.archive_dir.iterdir() if p.name.endswith(('.tar.gz', '.zip'))]
    original_hash = hashlib.sha256(archive.read_bytes()).hexdigest()
    assert archive.with_name(archive.name+'.sha256').read_text().split() == [original_hash, archive.name]
    payload = unpack(archive.resolve(), output/'staging')
    original_manifest = json.loads((payload/'PACKAGE-MANIFEST.json').read_text())
    for name, digest in original_manifest.items():
        assert hashlib.sha256((payload/name).read_bytes()).hexdigest() == digest, name
    root = payload/'GMAT-Qt.app' if args.platform.startswith('macos') else payload
    api_root = root/'Contents/Resources/gmatpy' if args.platform.startswith('macos') else root/'bin/gmatpy'
    metadata_path = root/'Contents/Resources/PYTHON-API.json' if args.platform.startswith('macos') else root/'PYTHON-API.json'
    metadata = json.loads(metadata_path.read_text())
    original_metadata = dict(metadata)
    native_libraries = libraries(payload, args.platform)
    options = []
    for native, path in native_libraries.items():
        options.append(f'-DGMAT_LIBRARY_{native}={path}')
        if args.platform.startswith('windows'):
            options.append(f'-DGMAT_IMPLIB_{native}={windows_import_library(native, path, output)}')
    versions = {}
    for version in VERSIONS:
        interpreter = Path(os.environ['GMAT_PYTHON_'+version.replace('.', '')]).resolve()
        info = json.loads(subprocess.check_output([str(interpreter), '-c',
            'import json,sys,sysconfig; print(json.dumps({"version":sys.version,"root":sys.base_prefix,"minor":list(sys.version_info[:2]),"free_threaded":bool(sysconfig.get_config_var("Py_GIL_DISABLED"))}))'], text=True))
        assert info['minor'] == list(map(int, version.split('.'))) and not info['free_threaded'], info
        module_dir = api_root/('_py'+version.replace('.', ''))
        if module_dir.exists():
            shutil.rmtree(module_dir)
        module_dir.mkdir()
        shutil.copy2(builder.REPO/'application/bin/gmatpy/__init__.specific.py', module_dir/'__init__.py')
        folder = output/('build-'+version)
        command = ['cmake', '-S', builder.REPO/'tools/preview/python-bindings', '-B', folder,
            '-DCMAKE_BUILD_TYPE=Release', f'-DGMAT_SOURCE={builder.REPO}', f'-DGMAT_OUTPUT={module_dir}',
            f'-DGMAT_PYTHON_VERSION={version}', f'-DPython3_EXECUTABLE={interpreter}',
            f'-DPython3_ROOT_DIR={info["root"]}', f'-DCSPICE_INCLUDE_DIR={args.cspice_include.resolve()}', *options]
        if args.platform.startswith('windows'):
            command += ['-G', 'Visual Studio 17 2022', '-A', 'x64']
        elif args.platform.startswith('macos'):
            command += ['-DCMAKE_OSX_DEPLOYMENT_TARGET=15.0',
                        '-DCMAKE_OSX_ARCHITECTURES='+('arm64' if args.platform.endswith('arm64') else 'x86_64')]
        print('Building CPython '+version+' wrappers', flush=True)
        run(command, output/('configure-'+version+'.log'))
        run(['cmake', '--build', folder, '--config', 'Release', '--parallel', str(min(os.cpu_count() or 2, 3))],
            output/('build-'+version+'.log'))
        for generated in module_dir.glob('*_wrap.*'):
            generated.unlink()
        for module in ('gmat', 'station', 'navigation'):
            assert (module_dir/(module+'_py.py')).is_file()
            assert len(list(module_dir.glob('_'+module+'_py.*'))) == 1
        versions[version] = info
    if args.gui_prefix:
        gui_build = output/'gui-build'
        command = ['cmake', '-S', builder.REPO/'tools/preview/archived-gui', '-B', gui_build,
            '-DCMAKE_BUILD_TYPE=Release', f'-DCSPICE_DIR={args.cspice_include.resolve().parent}',
            f'-DCMAKE_PREFIX_PATH={args.gui_prefix}', *options]
        if args.platform.startswith('windows'):
            command += ['-G', 'Visual Studio 17 2022', '-A', 'x64']
        elif args.platform.startswith('macos'):
            command += ['-DCMAKE_OSX_DEPLOYMENT_TARGET=15.0',
                        '-DCMAKE_OSX_ARCHITECTURES='+('arm64' if args.platform.endswith('arm64') else 'x86_64')]
        run(command, output/'gui-configure.log')
        run(['cmake', '--build', gui_build, '--config', 'Release', '--target', 'GmatQt',
             '--parallel', '3'], output/'gui-build.log')
        executable = 'GmatQt.exe' if args.platform.startswith('windows') else 'GmatQt'
        candidates = list(gui_build.rglob(executable))
        assert len(candidates) == 1, candidates
        destination = root/'Contents/MacOS'/executable if args.platform.startswith('macos') else root/'bin'/executable
        shutil.copy2(candidates[0], destination)
        if args.platform.startswith('macos'):
            for line in subprocess.check_output(['otool', '-L', str(destination)], text=True).splitlines()[1:]:
                old = line.strip().split(' (', 1)[0]
                if old.startswith(('/usr/lib/', '/System/')):
                    continue
                suffix = old[old.index('/Qt')+1:] if '/Qt' in old and '.framework/' in old else Path(old).name
                matches = [p for p in root.rglob(Path(old).name) if p.is_file() and p.as_posix().endswith(suffix)]
                assert matches, ('Unpackaged GUI dependency', old)
                dependency = min(matches, key=lambda p: len(str(p)))
                replacement = '@executable_path/'+os.path.relpath(dependency, destination.parent)
                subprocess.run(['install_name_tool', '-change', old, replacement, str(destination)], check=True)
        metadata['gui_source_commit'] = subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip()
    metadata.update(python_versions=list(VERSIONS), python_builds=versions,
                    bindings_source_commit=subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip(),
                    build_flavor='api-py312-315-v1',
                    base_archive={'sha256': original_hash, 'python_api': original_metadata})
    metadata_path.write_text(json.dumps(metadata, indent=2)+'\n')
    shutil.copy2(builder.REPO/'application/bin/gmatpy/__init__.py', api_root/'__init__.py')
    shutil.copy2(builder.REPO/'RELEASE-NOTES.rst', payload/'RELEASE-NOTES.rst')
    for document in ('README.rst', 'GMAT-Qt-Differences.rst'):
        shutil.copy2(builder.REPO/document, payload/document)
    for name, digest in original_manifest.items():
        if ('/gmatpy/' in '/'+name or name.endswith(('PYTHON-API.json', 'RELEASE-NOTES.rst', '_CodeSignature/CodeResources'))
                or (args.gui_prefix and Path(name).name in ('GmatQt', 'GmatQt.exe'))):
            continue
        assert hashlib.sha256((payload/name).read_bytes()).hexdigest() == digest, name
    if args.platform.startswith('macos'):
        # Relocate wrapper imports to the frameworks already in this signed app.
        for extension in api_root.rglob('*.so'):
            dependencies = subprocess.check_output(['otool', '-L', extension], text=True).splitlines()[1:]
            for line in dependencies:
                old = line.strip().split(' (', 1)[0]
                for native, path in native_libraries.items():
                    if Path(old).name.startswith('lib'+native) and Path(old).name.endswith('.dylib'):
                        replacement = '@loader_path/'+os.path.relpath(path, extension.parent)
                        subprocess.run(['install_name_tool', '-change', old, replacement, str(extension)], check=True)
            subprocess.run(['codesign', '--force', '--sign', '-', str(extension)], check=True)
        subprocess.run(['codesign', '--force', '--sign', '-', str(root)], check=True)
        subprocess.run(['codesign', '--verify', '--deep', '--strict', str(root)], check=True)
    manifest = {p.relative_to(payload).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in sorted(payload.rglob('*')) if p.is_file() and p.name != 'PACKAGE-MANIFEST.json'}
    (payload/'PACKAGE-MANIFEST.json').write_text(json.dumps(manifest, indent=2)+'\n')
    dist = output/'dist'
    dist.mkdir()
    destination = dist/archive.name
    if args.platform.startswith('linux'):
        with tarfile.open(destination, 'w:gz', dereference=False) as stream:
            stream.add(payload, arcname=payload.name)
    elif args.platform.startswith('macos'):
        subprocess.run(['ditto', '-c', '-k', '--sequesterRsrc', '--keepParent', str(payload), str(destination)], check=True)
    else:
        shutil.make_archive(str(destination.with_suffix('')), 'zip', payload.parent, payload.name)
    digest = hashlib.sha256(destination.read_bytes()).hexdigest()
    destination.with_name(destination.name+'.sha256').write_text(digest+'  '+destination.name+'\n')
    receipt = {'status':'built', 'platform':args.platform, 'base_archive_sha256':original_hash,
        'archive_sha256':digest, 'source_commit':metadata['bindings_source_commit'],
        'native_source_commit':metadata['source_commit'], 'python_versions':versions,
        'engine_rebuilt':False, 'desktop_rebuilt':bool(args.gui_prefix), 'native_libraries':{n:hashlib.sha256(p.read_bytes()).hexdigest() for n,p in native_libraries.items()}}
    (dist/'python-range-build.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps({'status':'built', 'archive':str(destination), 'sha256':digest}), flush=True)


if __name__ == '__main__':
    main()
