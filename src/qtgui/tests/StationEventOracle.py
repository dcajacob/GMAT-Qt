"""Derive event selections from the preserved independent, UNCLIPPED roots.

ContactGeometryOracle supplies the geometric circular-orbit/ellipsoid roots.
An interval clipped by a query boundary must never create a station event.
This derivation calls no GMAT event locator, GF search or contact report parser.
"""
import copy
import hashlib
import json
from pathlib import Path


def fixtures():
    original = Path(__file__).with_name('NasaContactResultCases.json')
    source = json.loads(original.read_text())
    full, bounded = source['cases'][0], source['cases'][2]
    cases = []
    combinations = [
        ('RiseOnly', ['StationRise']),
        ('SetOnly', ['StationSet']),
        ('Both', ['StationRise', 'StationSet']),
        ('MixedRise', ['ContactIntervals', 'StationRise']),
        ('MixedSet', ['ContactIntervals', 'StationSet']),
        ('MixedBoth', ['ContactIntervals', 'StationRise', 'StationSet']),
        ('BoundedBoth', ['StationRise', 'StationSet']),
        ('BoundedMixed', ['ContactIntervals', 'StationRise', 'StationSet']),
        ('DarkOnly', ['StationRise', 'StationSet']),
        ('OtherTarget', ['StationRise', 'StationSet']),
    ]
    for key, types in combinations:
        case = copy.deepcopy(bounded if key.startswith('Bounded') else full)
        case.update(key=key, event_types=types)
        if key == 'DarkOnly':
            case['observers'] = ['Dark']
            case['events'] = {'Dark': []}
        if key == 'OtherTarget':
            case['target'] = 'Other'
        events = []
        for observer in case['observers']:
            for contact in full['events'][observer]:
                for column, event_type in [('start', 'StationRise'), ('end', 'StationSet')]:
                    time = contact[column]
                    if event_type in types and case['start'] <= time <= case['end']:
                        # The original full-orbit fixture independently checks
                        # both sides of these roots. Its clipped endpoints do
                        # not enter this derivation.
                        assert time not in (0., 6500.)
                        events.append(dict(observer=observer, event_type=event_type, time=time))
        case['station_events'] = sorted(events, key=lambda event: event['time'])
        cases.append(case)
    result = copy.deepcopy(source)
    result.update(cases=cases, parent_fixture_sha256=hashlib.sha256(original.read_bytes()).hexdigest(),
                  scope='Discrete geometric station rise/set selection, with the inherited contact GUI lifecycle, independent original circle/ellipsoid roots and complete report/viewer checks. Native analytic-kernel tests separately cover root boundaries, tangencies, plateaus and occultation ordering. This does not close the multiple-spacecraft event-locator requirement, arbitrary scientific domains, or platform qualification.')
    return result


if __name__ == '__main__':
    path = Path(__file__).with_name('NasaStationEventCases.json')
    path.write_text(json.dumps(fixtures(), indent=2) + '\n')
    print('Wrote', path.name, 'from existing independent roots; no GUI tests rerun')
