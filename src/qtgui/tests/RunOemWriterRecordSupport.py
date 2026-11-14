"""Compare native pending-record counts with independently parsed serialized OEM rows."""
from datetime import datetime, timedelta
from decimal import Decimal
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from CcsdsOemB1Oracle import validate


def audit(root):
    proof = json.loads((root/'writer-proof.json').read_text())
    expected = dict(initial_count=0, duplicate_count=3, copy_count=0, assignment_count=0,
                    clone_count=0, after_duplicate_write_count=0, after_explicit_clear_count=0,
                    first_chunk_count=1000, complete_segment_count=1003, after_long_write_count=0,
                    next_segment_count=1, after_single_write_count=0)
    assert proof == expected, (proof, expected)
    origin = datetime(2000, 1, 1, 12)

    def epoch(seconds):
        return (origin+timedelta(seconds=seconds)).isoformat(timespec='milliseconds')

    files = []
    cases = [('duplicate-epochs.oem', [[(0, 3), (.001, 5), (1, 6)]]),
             ('accumulated-and-reset.oem', [[(i*.5, i+1) for i in range(1003)], [(600, 2001)]])]
    for name, segments in cases:
        raw = (root/name).read_bytes()
        result = validate(raw, icd_object_ids=['GMAT-Qt-OEM-record-support'])
        assert result['status'] == 'passed', result
        blocks = re.split(r'^META_STOP\s*$', raw.decode('ascii'), flags=re.M)[1:]
        assert len(blocks) == len(segments)
        for block, expected_rows in zip(blocks, segments):
            rows = [line.split() for line in block.splitlines() if re.match(r'^\d{4}-\d{2}-\d{2}T', line)]
            assert len(rows) == len(expected_rows), (name, len(rows), len(expected_rows))
            for row, (seconds, marker) in zip(rows, expected_rows):
                assert row[0] == epoch(seconds), (row[0], epoch(seconds))
                assert list(map(Decimal, row[1:])) == list(map(Decimal, [1000+marker, 2*marker, 3*marker, 4, 5, 6]))
        files.append(dict(file=name, independent_B1=result))
    return dict(status='passed', files=files, native_counts=proof, exact_states_and_epochs=1007,
                new_requirement_credit=0, whole_row_qualified=False)


def run(binary, startup, root):
    root.mkdir(parents=True, exist_ok=True)
    with (root/'writer-native.log').open('wb') as log:
        code = subprocess.call([str(binary), str(startup)], cwd=startup.parent,
            env=dict(os.environ, GMAT_QT_ACCEPTANCE_DIR=str(root)), stdout=log, stderr=subprocess.STDOUT)
    if code:
        print((root/'writer-native.log').read_text(errors='replace')[-4000:])
        return code
    result = audit(root)
    (root/'writer-independent-audit.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(dict(status=result['status'], exact_states_and_epochs=result['exact_states_and_epochs'])))
    return 0


def main():
    binary, startup = (Path(p).resolve() for p in sys.argv[1:])
    output = os.environ.get('GMAT_QT_ACCEPTANCE_DIR')
    if output:
        return run(binary, startup, Path(output).resolve())
    with tempfile.TemporaryDirectory(prefix='gmat-oem-record-support-') as folder:
        return run(binary, startup, Path(folder))


if __name__ == '__main__':
    raise SystemExit(main())
