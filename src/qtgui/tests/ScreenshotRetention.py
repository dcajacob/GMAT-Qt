"""Discard automatic observer frames after passing checks; retain test oracles."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import struct
import xml.etree.ElementTree as ET

SIDECAR = 'observer-screenshot-retention.json'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def fingerprint(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def passing_case(result):
    if result.get('status') != 'passed' or result.get('returncode') != 0 or not result.get('log'):
        return False
    log = Path(result['log'])
    junit = Path(result.get('junit', str(log.parent/'ctest.xml')))
    try:
        if not log.is_file():
            return False
        for key, path in [('log', log), ('junit', junit)]:
            if result.get(key+'_sha256') and sha(path) != result[key+'_sha256']:
                return False
        document = ET.parse(junit).getroot()
        cases = document.findall('.//testcase')
        return (len(cases) == 1 and cases[0].get('name') == result['name']
                and cases[0].get('status') in (None, 'run', 'passed', 'pass')
                and not any(document.find('.//'+tag) is not None for tag in ('failure', 'error', 'skipped'))
                and all(int(suite.get(key, '0')) == 0 for suite in document.iter('testsuite')
                        for key in ('failures', 'errors', 'skipped', 'disabled')))
    except (OSError, ValueError, ET.ParseError):
        return False


def prune_receipt(path, allowed_root=None, protected=()):
    path = Path(path).resolve()
    receipt = json.loads(path.read_text())
    if not receipt.get('finished_utc') or receipt.get('snapshot_unchanged') is False:
        return dict(files=0, bytes=0)
    allowed_root = Path(allowed_root or path.parent).resolve()
    original_sha = sha(path)
    total = dict(files=0, bytes=0)
    for result in receipt.get('results', []):
        if not passing_case(result):
            continue
        directory = Path(result['log']).parent.resolve()
        if not directory.is_relative_to(allowed_root) or directory in protected:
            continue
        images = directory/'screenshots'
        if images.is_symlink():
            continue
        removed = []
        for image in sorted(set(result.get('trace', {}).get('screenshots', []))):
            image = Path(image)
            if (image.parent.resolve() != images or image.is_symlink()
                    or image.suffix != '.png' or not image.is_file()):
                continue
            with image.open('rb') as stream:
                header = stream.read(24)
            if len(header) != 24 or header[:8] != b'\x89PNG\r\n\x1a\n':
                continue
            dimensions = struct.unpack('>II', header[16:24])
            if not all(dimensions):
                continue
            removed.append(dict(path='screenshots/'+image.name, sha256=sha(image),
                                bytes=image.stat().st_size, png_size=list(dimensions)))
        if not removed:
            continue
        # Record before removal. Original receipts, logs, traces, JUnit and direct
        # visual assertion images outside screenshots/ are never rewritten.
        record = dict(schema_version=1, policy='discard-automatic-observer-frames-after-pass',
                      pruned_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                      original_receipt=str(path), original_receipt_sha256=original_sha,
                      test=result['name'], original_result_sha256=fingerprint(result),
                      removed=removed, direct_visual_oracles_preserved=True)
        sidecar = directory/SIDECAR
        temporary = sidecar.with_suffix('.tmp')
        temporary.write_text(json.dumps(record, indent=2)+'\n')
        temporary.replace(sidecar)
        for item in removed:
            image = directory/item['path']
            if sha(image) != item['sha256']:
                raise ValueError('Observer frame changed during retention: '+str(image))
            image.unlink()
            total['files'] += 1
            total['bytes'] += item['bytes']
    return total


def retained_capture(path):
    """Return an explicit pruning attestation, never pretend pixels still exist."""
    path = Path(path)
    sidecar = path.parent.parent/SIDECAR
    record = json.loads(sidecar.read_text())
    receipt_path = Path(record['original_receipt'])
    if sha(receipt_path) != record['original_receipt_sha256']:
        raise ValueError('Pruning receipt identity changed')
    receipt = json.loads(receipt_path.read_text())
    matches = [r for r in receipt['results'] if r['name'] == record['test']
               and fingerprint(r) == record['original_result_sha256']]
    if len(matches) != 1 or not passing_case(matches[0]):
        raise ValueError('Pruned frame has no unchanged passing result')
    if str(path) not in matches[0].get('trace', {}).get('screenshots', []):
        raise ValueError('Frame was not in the passing execution trace')
    entries = [e for e in record['removed'] if e['path'] == 'screenshots/'+path.name]
    if len(entries) != 1 or not all(entries[0]['png_size']):
        raise ValueError('Missing pruning record')
    return dict(entries[0], storage_state='pruned-after-pass', pixels_available=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    args = parser.parse_args()
    receipts = sorted(args.root.glob('*/results.json'))
    protected = set()
    for path in receipts:
        data = json.loads(path.read_text())
        for result in data.get('results', []):
            if result.get('log') and result.get('status') in ('failed', 'interrupted', 'timed-out', 'timeout'):
                protected.add(Path(result['log']).parent.resolve())
    total = dict(receipts=0, files=0, bytes=0)
    for path in receipts:
        result = prune_receipt(path, args.root, protected)
        total['receipts'] += 1
        for key in ('files', 'bytes'):
            total[key] += result[key]
    print(json.dumps(total, indent=2))
