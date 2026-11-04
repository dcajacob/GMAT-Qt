"""Closed-form point-mass circle fixtures; no GMAT/native imports.

Retain all expected OEM samples, including both sides of a discontinuity.
Assignment changes phase; an inertial impulse reverses velocity exactly.
"""
import datetime
import json
import math
from pathlib import Path

MU = 398600.4415
RADIUS = 7000.0
RATE = math.sqrt(MU/RADIUS**3)
SPEED = math.sqrt(MU/RADIUS)
CARTESIAN = ['X', 'Y', 'Z', 'VX', 'VY', 'VZ']
BASE = datetime.datetime(2000, 1, 1, 12, tzinfo=datetime.timezone.utc)


def state(time, phase=0.0, direction=1):
    angle = phase+direction*RATE*time
    return [RADIUS*math.cos(angle), RADIUS*math.sin(angle), 0.0,
            -direction*SPEED*math.sin(angle), direction*SPEED*math.cos(angle), 0.0]


def samples(start, end, step, phase=0.0, direction=1, include_terminal=True):
    times = list(range(start, end+1, step))
    if include_terminal and times[-1] != end:
        times.append(end)
    return [[t]+state(t, phase, direction) for t in times]


def fixture(key):
    duration = 600 if key == 'FixedAligned' else 10 if key == 'ShortArc' else 605
    raw = key == 'RawSteps'
    step = 5 if raw else 20
    segments = [samples(0, duration, step)]
    mission = "Propagate 'Entire arc' PropA(Sat) {Sat.ElapsedSecs = %d};\n" % duration
    user = key == 'UserInterval'
    if user:
        # FinalEpoch bounds a regular requested window; it does not force an
        # extra off-grid point. Native propagation termination is tested separately.
        segments = [samples(37, 558, step, include_terminal=False)]
    elif key in ['AssignmentBoundary', 'ManeuverBoundary', 'DynamicsBoundary']:
        mission = "Propagate 'Before boundary' PropA(Sat) {Sat.ElapsedSecs = 300};\n"
        if key == 'AssignmentBoundary':
            phase, direction = math.pi/3, 1
            changed = state(300, phase, direction)
            mission += ''.join('Sat.%s = %.17g;\n' % (name, value) for name, value in zip(CARTESIAN, changed))
        elif key == 'ManeuverBoundary':
            phase, direction = 600*RATE, -1
            mission += "Maneuver 'Reverse velocity' Reverse(Sat);\n"
        else:
            phase, direction = 0, 1
        mission += "Propagate 'After boundary' %s(Sat) {Sat.ElapsedSecs = 305};\n" % ('PropB' if key == 'DynamicsBoundary' else 'PropA')
        segments = [samples(0, 300, step), samples(300, 605, step, phase, direction)]
    elif key == 'ToggleGap':
        # These deliberately inverted controls are corrected through the GUI.
        mission = "Propagate 'Before gap' PropA(Sat) {Sat.ElapsedSecs = 200};\nToggle 'Gap off' Export On;\nPropagate 'Disabled arc' PropA(Sat) {Sat.ElapsedSecs = 200};\nToggle 'Gap on' Export Off;\nPropagate 'After gap' PropA(Sat) {Sat.ElapsedSecs = 205};\n"
        segments = [samples(0, 200, step), samples(400, 605, step)]
    phase = math.pi/3 if key == 'AssignmentBoundary' else 600*RATE if key == 'ManeuverBoundary' else 0
    direction = -1 if key == 'ManeuverBoundary' else 1
    return dict(key=key, duration=duration, mu=MU, radius=RADIUS, speed=SPEED,
                sampling='IntegratorSteps' if raw else '20', order=7,
                user_interval=user, initial_epoch='01 Jan 2000 12:00:37.000' if user else 'InitialSpacecraftEpoch',
                final_epoch='01 Jan 2000 12:09:18.000' if user else 'FinalSpacecraftEpoch',
                mission=mission, expected_segments=segments,
                expected_final=[duration]+state(duration, phase, direction)+[9],
                expected_degree=2 if key == 'ShortArc' else 7,
                reverse_delta_v=[-2*x for x in state(300)[3:]],
                position_tolerance=2e-5, velocity_tolerance=2e-8)


KEYS = ['FixedAligned', 'FixedRemainder', 'RawSteps', 'UserInterval',
        'AssignmentBoundary', 'ManeuverBoundary', 'DynamicsBoundary', 'ToggleGap', 'ShortArc']


if __name__ == '__main__':
    p = Path(__file__).with_name('NasaEphemerisSamplingCases.json')
    p.write_text(json.dumps(dict(cases=[fixture(k) for k in KEYS],
                 scope='Selected circular Earth/OEM all-record sampling, discontinuities and guide-specific order fallback; no complete CCSDS conformance, arbitrary states/frames, covariance, binary formats or full current-build/platform qualification.'), indent=2)+'\n')
    print('Wrote9 independent complete-OEM fixtures:', p)
