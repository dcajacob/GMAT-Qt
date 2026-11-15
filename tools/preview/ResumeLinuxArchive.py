"""Complete missing Linux checks on the existing alpha archive."""
import hashlib
import json
from pathlib import Path
import shutil

import BuildPreviewPackage as builder
from QualifyPythonPackage import qualify


def main():
    archive, = (builder.ROOT / 'dist').glob('*linux-x86_64.tar.gz')
    checksum = archive.with_name(archive.name + '.sha256').read_text().split()
    assert checksum == [hashlib.sha256(archive.read_bytes()).hexdigest(), archive.name]
    qualification = builder.LOGS / 'archive-qualification'
    previous_path = qualification / 'package-verification.json'
    previous = json.loads(previous_path.read_text())
    assert previous['status'] == 'passed' and previous['platform'] == 'linux-x86_64'
    retained = builder.LOGS / 'first-archive-qualification'
    assert not retained.exists()
    shutil.copytree(qualification, retained)
    # The existing archive and package pass are reused. The missing Hermite,
    # event, MCP and Python-free checks run on a new extraction.
    shutil.rmtree(qualification)
    qualify('linux-x86_64', archive, previous_package=previous)
    receipt_path = qualification / 'verification.json'
    receipt = json.loads(receipt_path.read_text())
    receipt['reused_package_pass'] = {
        'archive_sha256': checksum[0],
        'receipt_sha256': hashlib.sha256((retained / 'package-verification.json').read_bytes()).hexdigest(),
        'previous_source': previous['commit'],
        'previous_receipt': 'first-archive-qualification/package-verification.json',
    }
    receipt_path.write_text(json.dumps(receipt, indent=2)+'\n')


if __name__ == '__main__':
    main()
