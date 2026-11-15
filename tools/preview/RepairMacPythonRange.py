"""Relocate an existing multi-Python Mac archive to its canonical engine copies."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

from BuildPythonRange import unpack


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    archives = list(args.input.rglob('GMAT-Qt-*.zip'))
    assert len(archives) == 1, archives
    archive = archives[0].resolve()
    old_hash = hashlib.sha256(archive.read_bytes()).hexdigest()
    assert archive.with_name(archive.name+'.sha256').read_text().split() == [old_hash, archive.name]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    payload = unpack(archive, output/'staging')
    root = payload/'GMAT-Qt.app'
    manifest = json.loads((payload/'PACKAGE-MANIFEST.json').read_text())
    for name,digest in manifest.items():
        assert hashlib.sha256((payload/name).read_bytes()).hexdigest() == digest
    frameworks = root/'Contents/Frameworks'
    canonical = {name:next(frameworks.glob('lib'+name+'.R2026a.dylib'))
                 for name in ['GmatBase','GmatUtil','Station','GmatEstimation']}
    binaries = [root/'Contents/MacOS/GmatQt', *list((root/'Contents/Resources/gmatpy').rglob('*.so'))]
    changes = []
    for binary in binaries:
        dependencies = subprocess.check_output(['otool','-L',str(binary)],text=True).splitlines()[1:]
        for line in dependencies:
            old = line.strip().split(' (',1)[0]
            for name, path in canonical.items():
                if Path(old).name.startswith('lib'+name) and old.endswith('.dylib'):
                    new = '@loader_path/'+os.path.relpath(path,binary.parent)
                    if old != new:
                        subprocess.run(['install_name_tool','-change',old,new,str(binary)],check=True)
                        changes.append({'file':binary.relative_to(payload).as_posix(),'before':old,'after':new})
        subprocess.run(['codesign','--force','--sign','-',str(binary)],check=True)
    subprocess.run(['codesign','--force','--sign','-',str(root)],check=True)
    subprocess.run(['codesign','--verify','--deep','--strict',str(root)],check=True)
    metadata_path = root/'Contents/Resources/PYTHON-API.json'
    metadata = json.loads(metadata_path.read_text())
    metadata['relocation_source_commit'] = subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()
    metadata['relocated_archive_sha256'] = old_hash
    metadata_path.write_text(json.dumps(metadata,indent=2)+'\n')
    subprocess.run(['codesign','--force','--sign','-',str(root)],check=True)
    subprocess.run(['codesign','--verify','--deep','--strict',str(root)],check=True)
    rebuilt = {p.relative_to(payload).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
               for p in payload.rglob('*') if p.is_file() and p.name!='PACKAGE-MANIFEST.json'}
    allowed = {b.relative_to(payload).as_posix() for b in binaries}
    allowed.add(metadata_path.relative_to(payload).as_posix())
    allowed.add('GMAT-Qt.app/Contents/_CodeSignature/CodeResources')
    assert all(rebuilt[name]==digest or name in allowed for name,digest in manifest.items())
    (payload/'PACKAGE-MANIFEST.json').write_text(json.dumps(rebuilt,indent=2)+'\n')
    dist = output/'dist'
    dist.mkdir()
    destination = dist/archive.name
    subprocess.run(['ditto','-c','-k','--sequesterRsrc','--keepParent',str(payload),str(destination)],check=True)
    digest = hashlib.sha256(destination.read_bytes()).hexdigest()
    destination.with_name(destination.name+'.sha256').write_text(digest+'  '+destination.name+'\n')
    (dist/'relocation.json').write_text(json.dumps({'source_archive_sha256':old_hash,
        'archive_sha256':digest,'changes':changes,'native_code_recompiled':False},indent=2)+'\n')


if __name__ == '__main__':
    main()
