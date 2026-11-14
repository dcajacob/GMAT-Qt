"""Legal null-marker contexts, with paired plain/grouped native execution."""
import argparse
import hashlib
import json
from pathlib import Path


def cases():
    origin = Path(__file__).resolve().parent/'mission-command-names/Propagate.json'
    original = json.loads(origin.read_text())
    setup, body = original['script'].split('BeginMissionSequence;')
    rows = body.strip().splitlines()
    propagation = [line for line in rows if line.startswith('Propagate ')]
    report = next(line for line in rows if line.startswith("Report 'Witness' Values "))
    assert len(propagation) == 2
    result = []
    for marker in ['NoOp', 'BeginMissionSequence', 'PrepareMissionSequence']:
        commands = ('Count = 3;\n'+propagation[0]+'\n'+marker+';\n'
                    'Count = Count * 7;\n'+propagation[1]+'\n'+report+'\n')
        result.append(dict(family=marker, command=marker,
            script=setup+'BeginMissionSequence;\n'+commands,
            expected_tail=[21, 1, 0, 0], velocity_increment=[0, 0, 0],
            marker_plain_control=True, duplicate_starter=marker!='NoOp',
            fixture_origin='mission-command-names/Propagate.json',
            fixture_origin_sha256=hashlib.sha256(origin.read_bytes()).hexdigest(),
            oracle=('NoOp preserves arithmetic and two ten-second circle arcs in both native sequences.' if marker=='NoOp' else
                    'Interpreter::FinalPass rejects more than one sequence starter; plain and grouped duplicate placement must both reject. This does not resolve the legal unique-starter context.')))
    result.append(dict(family='Create', command='Create',
        script=setup+'BeginMissionSequence;\nCount = 3;\nCreate Variable Inside;\nCount = 21;\n'+report+'\n',
        unsupported_definition=True, fixture_origin='mission-command-names/Propagate.json',
        fixture_origin_sha256=hashlib.sha256(origin.read_bytes()).hexdigest(),
        oracle='The native interpreter prohibits object definitions after the mission boundary outside a function; both contexts must reject for that exact rule. Function-local Create remains covered by recorded GmatFunction execution.'))
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for case in cases():
        path = args.output_dir/(case['family']+'.json')
        if path.exists():
            raise SystemExit('Refusing to overwrite '+str(path))
        path.write_text(json.dumps(case, indent=2)+'\n')
