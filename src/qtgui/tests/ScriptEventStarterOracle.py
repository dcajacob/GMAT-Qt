"""Unique root starter contexts; preparation must not execute following commands."""
import argparse
import hashlib
import json
from pathlib import Path


def cases():
    origin = Path(__file__).resolve().parent/'mission-command-names/Propagate.json'
    old = json.loads(origin.read_text())
    setup, body = old['script'].split('BeginMissionSequence;')
    lines = body.strip().splitlines()
    propagation = [line for line in lines if line.startswith('Propagate ')]
    report = next(line for line in lines if line.startswith("Report 'Witness' Values "))
    assert len(propagation) == 2
    result = []
    for marker in ['BeginMissionSequence', 'PrepareMissionSequence']:
        commands = 'Count = 3;\n'+propagation[0]+'\nCount = Count * 7;\n'+propagation[1]+'\n'+report+'\n'
        result.append(dict(family=marker, command=marker,
            script=setup+marker+';\n'+commands,
            expected_tail=[21, 1, 0, 0], velocity_increment=[0, 0, 0],
            marker_plain_control=True, unique_starter=True,
            expected_prepared=marker=='PrepareMissionSequence',
            fixture_origin='mission-command-names/Propagate.json',
            fixture_origin_sha256=hashlib.sha256(origin.read_bytes()).hexdigest(),
            oracle=('Begin executes the following scalar/orbit mission in plain and grouped contexts.' if marker=='BeginMissionSequence' else
                    'Prepare initializes the sandbox without executing following assignments/propagation/report; unchanged initial state and no report must agree with native filename loading.')))
    return result


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output-dir', type=Path, required=True)
    a = p.parse_args()
    a.output_dir.mkdir(parents=True, exist_ok=True)
    for case in cases():
        path = a.output_dir/(case['family']+'.json')
        if path.exists():
            raise SystemExit('Refusing to overwrite '+str(path))
        path.write_text(json.dumps(case, indent=2)+'\n')
