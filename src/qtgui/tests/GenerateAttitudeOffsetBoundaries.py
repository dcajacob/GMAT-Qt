"""Independent quaternion boundary witnesses over retained rotating-frame inputs."""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path

HERE = Path(__file__).resolve().parent


def generate():
    source = HERE / 'AttitudeOffsetScopeCases.json'
    original = json.loads(source.read_text())
    frames = {c['key']: c for c in original['cases']}
    q = original['offset_quaternion']
    variants = [
        ('Identity', [0, 0, 0, 1], 'GroundStation_Topocentric'),
        ('Negative', [-v for v in q], 'GroundStation_Topocentric'),
        ('Normalized', [7*v for v in q], 'GroundStation_Topocentric'),
        ('HalfTurn', [1, 2, 3, 0], 'GroundStation_Topocentric'),
        ('HalfTurnBody', [0, 1, 0, 0], 'CelestialBody_BodyFixed'),
        ('HugeFinite', [1e308, -1e308, 1e308, -1e308], 'GroundStation_Topocentric'),
        ('ThresholdIdentity', [0, 0, 0, 1e-10], 'GroundStation_Topocentric'),
        ('SmallAccepted', [1e-9*v for v in q], 'GroundStation_Topocentric'),
        ('NegativeIdentity', [0, 0, 0, -4], 'GroundStation_Topocentric'),
    ]
    cases = []
    for key, raw, frame in variants:
        case = copy.deepcopy(frames[frame])
        scale = max(abs(v) for v in raw)
        scaled = [v/scale for v in raw]
        norm = math.sqrt(math.fsum(v*v for v in scaled))
        normalized = [v/norm for v in scaled]
        x, y, z, w = normalized
        rotation = [[1-2*(y*y+z*z), 2*(x*y+z*w), 2*(x*z-y*w)],
                    [2*(x*y-z*w), 1-2*(x*x+z*z), 2*(y*z+x*w)],
                    [2*(x*z+y*w), 2*(y*z-x*w), 1-2*(x*x+y*y)]]
        case.update(key=key, base_frame_case=frame, input_quaternion=raw,
                    offset_quaternion=normalized, offset_matrix=rotation,
                    serializes_offset=normalized != [0, 0, 0, 1],
                    invalid_guards=key == 'Normalized')
        for sample in case['samples']:
            sample['rows'] = [[math.fsum(rotation[i][k]*sample['frame_rows'][k][j]
                                        for k in range(3)) for j in range(3)] for i in range(3)]
            sample['angular_velocity_degrees_s'] = [math.fsum(
                rotation[i][j]*sample['frame_angular_velocity_degrees_s'][j] for j in range(3))
                for i in range(3)]
        cases.append(case)
    invalid = ['', '0 0 0', '0 0 0 1 2', '0 0 0 0', '0 0 0 1e-12',
               'nan 0 0 1', 'inf 0 0 1', '-inf 0 0 1', '1e309 0 0 1', 'word 0 0 1']
    return dict(input_state=original['input_state'], numeric_sentinel=41,
                source_fixture=source.name,
                source_fixture_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                scope='Offset boundaries and actual GUI rejection/atomic correction, conjoined '
                      'with all 68 original source-permitted frame/origin cells. No credit alone.',
                invalid_inputs=invalid, invalid_expected_errors=(["Keep the existing number of columns"]*3 + ["OffsetQuaternion magnitude"]*2 + ["Every cell must contain a finite number"]*5), cases=cases)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    with args.output.open('x') as stream:
        json.dump(generate(), stream, indent=2, ensure_ascii=False)
        stream.write('\n')
