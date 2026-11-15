"""Package saved Windows Python bindings and rebuild only the affected GUI."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile

import BuildPreviewPackage as builder
from BuildPythonRange import libraries, run, unpack, windows_import_library


def main():
    task = builder.REPO/'build/windows-finish'
    output = task/'result'
    output.mkdir(parents=True, exist_ok=True)
    archive, = (task/'base').glob('*.zip')
    payload = unpack(archive.resolve(), output/'staging')
    original_manifest = json.loads((payload/'PACKAGE-MANIFEST.json').read_text())
    for name,digest in original_manifest.items():
        assert hashlib.sha256((payload/name).read_bytes()).hexdigest()==digest
    saved, = (task/'saved').rglob('gmatpy')
    api = payload/'bin/gmatpy'
    shutil.rmtree(api)
    shutil.copytree(saved, api)
    for version in ('3.12','3.13','3.14','3.15'):
        folder = api/('_py'+version.replace('.',''))
        assert all((folder/(name+'_py.py')).is_file() and (folder/('_'+name+'_py.pyd')).is_file()
                   for name in ['gmat','station','navigation'])
    qt = task/'qt/6.10.2/msvc2022_64'
    run([sys.executable,'-m','pip','install','aqtinstall==3.3.0'],output/'aqt.log')
    run([sys.executable,'-m','aqt','install-qt','windows','desktop','6.10.2','win64_msvc2022_64','-O',task/'qt'],output/'qt.log')
    osg_archive = task/'osg.tar.gz'
    builder.download('https://github.com/openscenegraph/OpenSceneGraph/archive/refs/tags/OpenSceneGraph-3.6.5.tar.gz',osg_archive)
    with tarfile.open(osg_archive) as stream:
        stream.extractall(task,filter='data')
    osg_source=task/'OpenSceneGraph-OpenSceneGraph-3.6.5'
    osg_build=task/'osg-headers'
    run(['cmake','-S',osg_source,'-B',osg_build,'-G','Visual Studio 17 2022','-A','x64',
         '-DCMAKE_POLICY_VERSION_MINIMUM=3.5','-DBUILD_OSG_APPLICATIONS=OFF','-DBUILD_OSG_EXAMPLES=OFF'],output/'osg-headers.log')
    includes=task/'osg/include'
    shutil.copytree(osg_source/'include',includes)
    shutil.copytree(osg_build/'include',includes,dirs_exist_ok=True)
    import_folder=task/'imports'
    import_folder.mkdir()
    options=[]
    for native,path in libraries(payload,'windows-x64').items():
        options += [f'-DGMAT_LIBRARY_{native}={path}',f'-DGMAT_IMPLIB_{native}={windows_import_library(native,path,import_folder)}']
    for module in ['osg','osgDB','osgViewer','osgGA','osgUtil','osgText','OpenThreads']:
        imported=windows_import_library(module,payload/'bin'/(module+'.dll'),import_folder)
        options += [f'-D{module.upper()}_INCLUDE_DIR={includes}',f'-D{module.upper()}_LIBRARY_RELEASE={imported}']
    cspice=task/'cspice.zip'
    builder.download('https://naif.jpl.nasa.gov/pub/naif/toolkit/C/PC_Windows_VisualC_64bit/packages/cspice.zip',cspice)
    import zipfile
    with zipfile.ZipFile(cspice) as stream:stream.extractall(task)
    gui=task/'gui'
    run(['cmake','-S',builder.REPO/'tools/preview/archived-gui','-B',gui,'-G','Visual Studio 17 2022','-A','x64',
         '-DCMAKE_BUILD_TYPE=Release',f'-DCMAKE_PREFIX_PATH={qt}',f'-DCSPICE_DIR={task/"cspice"}',*options],output/'gui-configure.log')
    run(['cmake','--build',gui,'--config','Release','--target','GmatQt','--parallel','3'],output/'gui-build.log')
    shutil.copy2(gui/'Release/GmatQt.exe',payload/'bin/GmatQt.exe')
    source=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()
    metadata_path=payload/'PYTHON-API.json'
    metadata=json.loads(metadata_path.read_text())
    original=dict(metadata)
    metadata.update(python_versions=['3.12','3.13','3.14','3.15'],build_flavor='api-py312-315-v1',
        bindings_source_commit=os.environ['GMAT_SAVED_BINDING_SOURCE'],gui_source_commit=source,
        base_archive={'sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'python_api':original})
    metadata_path.write_text(json.dumps(metadata,indent=2)+'\n')
    for name in ['README.rst','RELEASE-NOTES.rst','GMAT-Qt-Differences.rst']:
        shutil.copy2(builder.REPO/name,payload/name)
    for name,digest in original_manifest.items():
        normalized=name.replace('\\','/')
        if '/gmatpy/' in '/'+normalized or normalized in ['bin/GmatQt.exe','PYTHON-API.json','RELEASE-NOTES.rst']:
            continue
        assert hashlib.sha256((payload/name).read_bytes()).hexdigest()==digest,name
    manifest={p.relative_to(payload).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
              for p in payload.rglob('*') if p.is_file() and p.name!='PACKAGE-MANIFEST.json'}
    (payload/'PACKAGE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
    dist=output/'dist';dist.mkdir()
    destination=dist/archive.name
    shutil.make_archive(str(destination.with_suffix('')),'zip',payload.parent,payload.name)
    destination.with_name(destination.name+'.sha256').write_text(hashlib.sha256(destination.read_bytes()).hexdigest()+'  '+destination.name+'\n')
    (dist/'build-reuse.json').write_text(json.dumps({'binding_source':metadata['bindings_source_commit'],
        'gui_source':source,'native_source':metadata['source_commit'],'reused_binding_files':{
            p.relative_to(saved).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in saved.rglob('*') if p.is_file()}},indent=2)+'\n')


if __name__=='__main__':main()
