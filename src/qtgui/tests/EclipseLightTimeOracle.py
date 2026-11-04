"""Independent converged reception light time for bounded eclipse cases.

NAIF abcorr.req defines tau = |T(et-tau)-O(et)|/c in barycentric axes.
Only uncorrected SPICE ephemerides are read here. The iteration, observer
construction, delayed Earth orientation, limb geometry and time roots are
independent of GMAT and SPICE aberration/occultation/GF implementations.
"""
import argparse
import copy
import ctypes as C
import functools
import hashlib
import json
import math
from pathlib import Path

import EclipseGeometryOracle as geometry

C_KM_S = 299792.458


class Reception(geometry.Trajectory):
    def barycentric(self, body, et):
        values, lt = (C.c_double*6)(), C.c_double()
        self.data.lib.spkezr_c(body.encode(), et, b'J2000', b'NONE',
                               b'SOLAR SYSTEM BARYCENTER', values, C.byref(lt))
        self.data.check()
        return list(values)[:3]

    def retarded(self, body, et, observer):
        delay = 0.
        for iteration in range(12):
            displacement = [x-y for x, y in zip(self.barycentric(body, et-delay), observer)]
            next_delay = geometry.norm(displacement)/C_KM_S
            if abs(next_delay-delay) < 1e-11:
                return displacement, next_delay, iteration+1
            delay = next_delay
        raise AssertionError('Independent reception light time did not converge')

    @functools.lru_cache(maxsize=None)
    def apparent(self, time):
        et = self.data.et(time)
        r = self.data.fk5_to_icrf(time, self.state(time)[:3])
        observer = [x+y for x, y in zip(self.barycentric('EARTH', et), r)]
        sun, sun_delay, sun_iterations = self.retarded('SUN', et, observer)
        earth, earth_delay, earth_iterations = self.retarded('EARTH', et, observer)
        # Earth's body frame participates at the reception emission epoch.
        matrix = ((C.c_double*3)*3)()
        self.data.lib.pxform_c(b'J2000', b'ITRF93', et-earth_delay, matrix)
        self.data.check()
        rotation = [list(row) for row in matrix]
        fixed_r = [-geometry.dot(row, earth) for row in rotation]
        fixed_u = [geometry.dot(row, geometry.unit(sun)) for row in rotation]
        beta = math.asin(self.data.sun/geometry.norm(sun))
        assert beta < 2*math.asin(min(self.data.earth)/geometry.norm(earth))
        return dict(margins=geometry.limb_extrema(fixed_r, fixed_u, beta, self.data.earth),
                    sun_delay=sun_delay, earth_delay=earth_delay,
                    sun_iterations=sun_iterations, earth_iterations=earth_iterations)

    @functools.lru_cache(maxsize=None)
    def margins(self, time):
        return self.apparent(time)['margins']


def fixtures(library, root):
    geometry.sphere_contract()
    data = geometry.Data(library, root)
    previous = json.loads((root/'src/qtgui/tests/NasaEclipseResultCases.json').read_text())
    cases = []
    for key, base, stellar in [('AllTypes', 'AllTypes', False),
                                ('Antumbra', 'Antumbra', False),
                                ('StellarIgnored', 'AllTypes', True)]:
        case = copy.deepcopy(next(c for c in previous['cases'] if c['key'] == base))
        orbit = Reception(data, radius=case['radius'], anti=base == 'Antumbra')
        assert orbit.state(0) == case['initial_state'] and orbit.state(geometry.DURATION) == case['final_state']
        events = [dict(e) for e in orbit.intervals() if e['type'] in case['types']]
        totals = geometry.groups(events)
        for i, group in enumerate(totals, 1):
            for event in group['events']:
                event.update(group=i, duration=event['end']-event['start'],
                             total_duration=group['end']-group['start'])
        assert len(events) == len(case['events'])
        shifts, sensitive_shifts = [], []
        for new, old in zip(events, case['events']):
            assert new['type'] == old['type'] and new['body'] == old['body']
            for name in ['start', 'end']:
                shift = new[name]-old[name]
                shifts.append(shift)
                if old[name] not in (0., geometry.DURATION):
                    sensitive_shifts.append(shift)
                else:
                    assert new[name] == old[name]  # confinement endpoint, not an event root
        # 2 ms exceeds millisecond report quantization and microsecond root
        # bracketing; every non-clipped root must separate on/off by at least5x.
        tolerance = .002
        assert sensitive_shifts and min(abs(x) for x in sensitive_shifts) > 5*tolerance
        maximum = max(g['end']-g['start'] for g in totals)
        per_type = {}
        for kind in case['types']:
            matching = [e for e in events if e['type'] == kind]
            longest = max((e['duration'] for e in matching), default=0.)
            participants = sorted({case['target']+'/'+e['body'] for e in matching
                                   if abs(e['duration']-longest) <= 1e-6})
            per_type[kind] = dict(count=len(matching), maximum=longest, participants=participants)
        case.update(key=key, use_light_time=True, use_stellar_aberration=stellar,
                    events=events, total_events=len(totals), per_type=per_type,
                    maximum_duration=maximum,
                    maximum_group=next(i+1 for i, g in enumerate(totals) if g['end']-g['start'] == maximum),
                    time_tolerance=tolerance, duration_tolerance=tolerance,
                    uncorrected_events=case['events'], correction_shifts_seconds=shifts,
                    correction_samples=[dict(seconds=t, **{k:v for k,v in orbit.apparent(t).items()
                                            if k != 'margins'}) for t in [0., 2500., 6500.]])
        cases.append(case)
    assert cases[0]['events'] == cases[2]['events']  # documented stellar-ignore contract
    physical = copy.deepcopy(previous['physical_inputs'])
    assert physical['tt0'] == data.tt0 and physical['earth_radii'] == data.earth and physical['sun_radius'] == data.sun
    for name, digest in physical['kernels'].items():
        assert hashlib.sha256((root/name).read_bytes()).hexdigest() == digest
    return dict(cases=cases, physical_inputs=physical, speed_of_light_km_s=C_KM_S,
                allowed_scientific_spice_calls=previous['allowed_scientific_spice_calls'],
                references=['https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/req/abcorr.html',
                            'https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/spkezr_c.html',
                            'https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/gfoclt_c.html'],
                scope='Independent barycentric fixed-point reception light time, delayed Earth orientation and solar-limb/ellipsoid roots for two selected circular Earth trajectories. Geometric SPICE data only; no native GMAT, SPICE corrected positions or GF/event solver predicts results. Stellar toggle is ignored for eclipse detection as documented by NAIF. Contact transmission/stellar science, arbitrary bodies/orbits/grazing, independent ephemeris/frame truth, native OS input/hardware/platform/stress/private current NASA and full current-build suites remain unqualified.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    value = fixtures(args.library, root)
    Path(__file__).with_name('NasaEclipseLightTimeCases.json').write_text(json.dumps(value, indent=2)+'\n')
    print(json.dumps({c['key']:dict(intervals=len(c['events']), shifts=c['correction_shifts_seconds']) for c in value['cases']}, indent=2))
