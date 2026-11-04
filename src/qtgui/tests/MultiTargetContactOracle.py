"""Independent, target-specific contact and rise/set roots for distinct circles.

Uses the preserved station algebra and hashed orientation/time inputs. No GMAT
locator, GF search, ephemeris manager or event output supplies expected results.
Orbital phase changes target position only; Earth orientation retains real time.
"""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import ContactGeometryOracle as contact


class PhasedCircle(contact.Circle):
    def __init__(self, data, phase):
        super().__init__(data)
        self.phase = phase

    def state(self, time, phase=None):
        return super().state(time, self.phase if phase is None else phase)


def fixtures(library, root):
    original = root/'src/qtgui/tests/NasaContactResultCases.json'
    result = copy.deepcopy(json.loads(original.read_text()))
    data = contact.common.Data(library, root)
    phases = dict(Sat=0., Other=math.pi/2, Third=math.pi, Excluded=3*math.pi/2)
    orbits = {name: PhasedCircle(data, phase) for name, phase in phases.items()}
    full = {name: {s['name']: orbit.intervals(s) for s in result['stations']}
            for name, orbit in orbits.items()}
    assert full['Sat'] != full['Other'] != full['Third']
    states = {name: orbit.state(0) for name, orbit in orbits.items()}
    final = {name: orbit.state(contact.DURATION) for name, orbit in orbits.items()}
    selections = [
        ('PairRise', ['Sat', 'Other'], ['StationRise'], False, False),
        ('TripletBoth', ['Sat', 'Other', 'Third'], ['StationRise', 'StationSet'], False, False),
        ('ReverseSet', ['Third', 'Sat', 'Other'], ['StationSet'], False, False),
        ('SingletonOther', ['Other'], ['StationRise', 'StationSet'], False, False),
        ('BoundedMixed', ['Sat', 'Other', 'Third'], ['ContactIntervals', 'StationRise', 'StationSet'], True, False),
        ('PairIntervals', ['Sat', 'Other'], ['ContactIntervals'], False, False),
        ('PairMixed', ['Sat', 'Other'], ['ContactIntervals', 'StationRise', 'StationSet'], False, False),
        ('DarkOnly', ['Other', 'Third'], ['StationRise', 'StationSet'], False, True),
    ]
    cases = []
    for key, targets, types, bounded, dark in selections:
        case = copy.deepcopy(result['cases'][0])
        start, end = (800., 3200.) if bounded else (0., float(contact.DURATION))
        observers = ['Dark'] if dark else ['North', 'East', 'Dark']
        events, station_events = {}, {}
        for name in targets:
            events[name] = {s['name']: orbits[name].intervals(s, start, end)
                            for s in result['stations'] if s['name'] in observers}
            roots = []
            for observer in observers:
                for interval in full[name][observer]:
                    for column, kind in [('start', 'StationRise'), ('end', 'StationSet')]:
                        time = interval[column]
                        if kind in types and start <= time <= end and time not in (0., contact.DURATION):
                            station = next(s for s in result['stations'] if s['name'] == observer)
                            assert orbits[name].margin(time-.01, station)*orbits[name].margin(time+.01, station) < 0
                            roots.append(dict(observer=observer, event_type=kind, time=time))
            station_events[name] = sorted(roots, key=lambda row: row['time'])
        case.update(key=key, target=targets[0], targets=targets, additional_targets=targets[1:],
                    event_types=types, observers=observers, events=events, station_events=station_events,
                    spacecraft_states=states, spacecraft_final_states=final,
                    initial_state=states[targets[0]], other_state=states['Other'],
                    final_state=final[targets[0]], start=start, end=end, bounded=bounded)
        cases.append(case)
    result.update(cases=cases, phases=phases,
                  parent_fixture_sha256=hashlib.sha256(original.read_bytes()).hexdigest(),
                  oracle_dependencies={str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
                     for p in [Path(__file__).resolve(), original.with_name('ContactGeometryOracle.py'),
                               original.with_name('EclipseGeometryOracle.py')]},
                  scope='User-selected one, two or three distinct spacecraft; rise-only, set-only, both, contact intervals and mixed reports; selected observer membership, explicit no-events, bounded roots, independent final states, real GUI lifecycle, report target identity and cached-result clone ownership. Uses geometric Earth circles with existing hashed shape/time/orientation inputs. Broader trajectories, light-time, FOV and third-body multi-target combinations are not inferred from this fixture; existing separately qualified options retain their recorded scope.')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    value = fixtures(args.library, root)
    Path(__file__).with_name('NasaMultiTargetContactCases.json').write_text(json.dumps(value, indent=2)+'\n')
    print(json.dumps({c['key']: {n: len(c['station_events'][n]) for n in c['targets']} for c in value['cases']}))
