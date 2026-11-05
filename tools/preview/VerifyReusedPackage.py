"""Reuse a native pass only when its package and production tree match."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import zipfile

root = Path(sys.argv[1])
platform = sys.argv[2] if len(sys.argv) > 2 else 'windows-x64'
assert platform in {'windows-x64', 'macos-arm64', 'macos-x64'}
report = json.loads((root / (platform + '-verification.json')).read_text())
assert report['status'] == 'passed' and report['platform'] == platform
if platform.startswith('macos'):
    assert report['bundled_3ds_reader']['status'] == 'passed'
commit = report['commit']
assert re.fullmatch(r'[a-f0-9]{40}', commit)
subprocess.run(['git', 'fetch', '--no-tags', '--depth=1', 'origin', commit], check=True)
subprocess.run(['git', 'diff', '--quiet', commit, 'HEAD', '--', 'CMakeLists.txt',
                'src', 'plugins', 'application', 'depends', 'build/cmake_modules',
                'build/install'], check=True)
archives = list(root.glob('*' + platform + '.zip'))
assert len(archives) == 1
archive = archives[0]
digest, name = (root / (archive.name + '.sha256')).read_text().strip().split()
assert name == archive.name and hashlib.sha256(archive.read_bytes()).hexdigest() == digest
with zipfile.ZipFile(archive) as package:
    paths = [n for n in package.namelist() if n.endswith('/PACKAGE-VERIFICATION.json')]
    assert len(paths) == 1 and json.loads(package.read(paths[0])) == report
print('Reused exact verified ' + platform + ' package; all production source trees match ' + commit)
