#!/usr/bin/env python3
"""Extract a private Openbox for tutorial replay (Debian/Ubuntu x86-64).

Downloads distribution packages into the build tree. Does not install packages,
change the user's window manager, or write desktop configuration.
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess


def main():
    repo = Path(__file__).resolve().parents[3]
    root = repo / 'build/tools/tutorial-desktop'
    root.mkdir(parents=True, exist_ok=True)
    packages = ['openbox', 'libobrender32', 'libobt2', 'libimlib2t64']
    for command in ['apt-get', 'dpkg-deb']:
        if not shutil.which(command):
            raise SystemExit('This helper requires Debian/Ubuntu packaging; alternatively supply --wm explicitly.')
    subprocess.run(['apt-get', '-o', 'Acquire::Retries=0', '-o', 'Acquire::http::Timeout=20',
                    'download', *packages], cwd=root, check=True)
    extracted = root / 'extracted'
    for package in sorted(root.glob('*.deb')):
        subprocess.run(['dpkg-deb', '-x', str(package), str(extracted)], check=True)
    libraries = extracted / 'usr/lib/x86_64-linux-gnu'
    binary = extracted / 'usr/bin/openbox'
    environment = dict(os.environ, LD_LIBRARY_PATH=str(libraries))
    version = subprocess.run([str(binary), '--version'], env=environment, capture_output=True,
                             text=True, check=True).stdout.strip()
    receipt = {'version': version, 'packages': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                               for p in sorted(root.glob('*.deb'))}}
    (root / 'packages.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(version + '\nPrivate desktop ready: ' + str(root))


if __name__ == '__main__':
    main()
