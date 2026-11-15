"""Complete only Python-free startup for a previously exercised range archive."""
import hashlib
import json
from pathlib import Path
import tempfile

from BuildPythonRange import unpack
from QualifyPythonPackage import write_no_python_check


def prepare(archive, output):
    record = output/'verification.json'
    previous = record.read_bytes()
    receipt = json.loads(previous)
    assert receipt['status'] == 'awaiting-without-python-check'
    assert hashlib.sha256(archive.read_bytes()).hexdigest() == receipt['archive_sha256']
    assert all(case['status'] == 'passed' for case in receipt['python_versions'].values())
    assert all(receipt[name]['status'] == 'passed' for name in ['package_checks','hermite_checks','event_checks'])
    payload = unpack(archive, Path(tempfile.mkdtemp(prefix='gmat-range-completion-'))/'extracted')
    for name,digest in json.loads((payload/'PACKAGE-MANIFEST.json').read_text()).items():
        assert hashlib.sha256((payload/name).read_bytes()).hexdigest() == digest
    target = receipt['platform']
    root = payload/'GMAT-Qt.app' if target.startswith('macos') else payload
    receipt['extracted_root'] = str(root)
    receipt['reused_qualification_sha256'] = hashlib.sha256(previous).hexdigest()
    (output/'verification-before-completion.json').write_bytes(previous)
    record.write_text(json.dumps(receipt,indent=2)+'\n')
    write_no_python_check(target, root, payload.parent, output)


if __name__ == '__main__':
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    prepare(args.archive.resolve(),args.output.resolve())
