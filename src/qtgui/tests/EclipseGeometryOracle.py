"""Independent eclipse geometry and time roots for selected acceptance cases.

CSPICE reads only ephemerides, orientation, physical radii and time systems.
No GMAT library, occultation classifier or geometry finder is called. Rays
on the solar limb are tested against the terrestrial ellipsoid. Extrema on
that limb use golden-section searches; time transitions use bisection.
"""
import argparse
import bisect
import ctypes as C
import functools
import hashlib
import json
import math
from pathlib import Path

MU = 398600.4415
DURATION = 6500
FRAME_TABLE = 'application/data/icrf/ICRF_Table.txt'
KERNELS = [
    'application/data/time/SPICELeapSecondKernel.tls',
    'application/data/planetary_coeff/SPICEPlanetaryConstantsKernel.tpc',
    'application/data/planetary_ephem/spk/DE421AllPlanets.bsp',
    'application/data/planetary_coeff/SPICEEarthPredictedKernel.bpc',
    'application/data/planetary_coeff/SPICEEarthCurrentKernel.bpc',
    'application/data/planetary_coeff/earth_latest_high_prec.bpc',
]


def dot(a, b):
    return math.fsum(x*y for x, y in zip(a, b))


def cross(a, b):
    return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]


def norm(v):
    return math.sqrt(dot(v, v))


def unit(v):
    return [x/norm(v) for x in v]


class Data:
    """Thin reader. The allowed scientific calls deliberately exclude GF."""
    def __init__(self, library, root):
        self.lib = C.CDLL(str(Path(library).resolve()))
        self.lib.erract_c.argtypes = [C.c_char_p, C.c_int, C.c_char_p]
        self.lib.errprt_c.argtypes = [C.c_char_p, C.c_int, C.c_char_p]
        self.lib.erract_c(b'SET', 0, b'RETURN')
        self.lib.errprt_c(b'SET', 0, b'NONE')
        self.lib.furnsh_c.argtypes = [C.c_char_p]
        self.lib.str2et_c.argtypes = [C.c_char_p, C.POINTER(C.c_double)]
        self.lib.unitim_c.argtypes = [C.c_double, C.c_char_p, C.c_char_p]
        self.lib.unitim_c.restype = C.c_double
        self.lib.spkezr_c.argtypes = [C.c_char_p, C.c_double, C.c_char_p, C.c_char_p, C.c_char_p, C.POINTER(C.c_double), C.POINTER(C.c_double)]
        self.lib.pxform_c.argtypes = [C.c_char_p, C.c_char_p, C.c_double, C.POINTER(C.c_double*3)]
        self.lib.bodvrd_c.argtypes = [C.c_char_p, C.c_char_p, C.c_int, C.POINTER(C.c_int), C.POINTER(C.c_double)]
        self.lib.kclear_c()
        # Shared frame-definition data, not a native GMAT conversion call.
        self.frame_rows = [[float(x) for x in line.split(',')]
                           for line in (root/FRAME_TABLE).read_text().splitlines()]
        self.frame_epochs = [row[0] for row in self.frame_rows]
        for name in KERNELS:
            self.lib.furnsh_c(str(root/name).encode())
            self.check()
        et = C.c_double()
        self.lib.str2et_c(b'2000 JAN 01 12:00:00 UTC', C.byref(et))
        self.check()
        self.tt0 = self.lib.unitim_c(et.value, b'ET', b'TDT')
        self.check()
        self.earth = self.radii('EARTH')
        sun = self.radii('SUN')
        assert max(sun)-min(sun) < 1e-10, 'This oracle requires the selected spherical Sun PCK'
        self.sun = sun[0]

    def check(self):
        if self.lib.failed_c():
            buffer = C.create_string_buffer(2048)
            self.lib.getmsg_c(b'LONG', len(buffer), buffer)
            self.lib.reset_c()
            raise RuntimeError(buffer.value.decode())

    def et(self, seconds):
        value = self.lib.unitim_c(self.tt0+seconds, b'TDT', b'ET')
        self.check()
        return value

    def radii(self, body):
        size, values = C.c_int(), (C.c_double*3)()
        self.lib.bodvrd_c(body.encode(), b'RADII', 3, C.byref(size), values)
        self.check()
        assert size.value == 3
        return list(values)

    def sun_state(self, seconds):
        result, lt = (C.c_double*6)(), C.c_double()
        self.lib.spkezr_c(b'SUN', self.et(seconds), b'J2000', b'NONE', b'EARTH', result, C.byref(lt))
        self.check()
        return list(result)

    def rotation(self, seconds):
        matrix = ((C.c_double*3)*3)()
        self.lib.pxform_c(b'J2000', b'ITRF93', self.et(seconds), matrix)
        self.check()
        return [list(row) for row in matrix]

    def fk5_to_icrf(self, seconds, vector):
        """Passive Rodrigues rotation from tabulated Euler-vector data.

        Table epochs use GMAT A1 modified Julian days. At the selected
        UTC epoch, TAI-UTC=32s and A1-TAI=.0343817s. Ten surrounding daily
        records define the documented ninth-degree interpolation. This
        shares the orientation input/convention, not the GMAT routine.
        """
        epoch = 21545+(32.0343817+seconds)/86400
        index = bisect.bisect_left(self.frame_epochs, epoch)
        rows = self.frame_rows[index-5:index+5]
        assert len(rows) == 10 and rows[0][0] < epoch < rows[-1][0]
        weights = [math.prod((epoch-b[0])/(a[0]-b[0]) for b in rows if b is not a)
                   for a in rows]
        rv = [math.fsum(w*row[j+1] for w, row in zip(weights, rows)) for j in range(3)]
        angle = norm(rv)
        axis = unit(rv)
        axial, crossed = dot(axis, vector), cross(axis, vector)
        return [math.cos(angle)*x-math.sin(angle)*y+(1-math.cos(angle))*axial*z
                for x, y, z in zip(vector, crossed, axis)]


def limb_extrema(r, sun_direction, sun_angle, radii):
    """Stable ray discriminant D.D - |R cross D|^2, in scaled axes."""
    rr = [x/a for x, a in zip(r, radii)]
    u = sun_direction
    east = unit(cross(u, [0, 0, 1] if abs(u[2]) < .9 else [1, 0, 0]))
    north = cross(u, east)

    def bilinear(a, b):
        a, b = [x/s for x, s in zip(a, radii)], [x/s for x, s in zip(b, radii)]
        return dot(a, b)-dot(cross(rr, a), cross(rr, b))

    c, s = math.cos(sun_angle), math.sin(sun_angle)
    uu, ee, nn = bilinear(u, u), bilinear(east, east), bilinear(north, north)
    coefficients = [c*c*uu+s*s*(ee+nn)/2, 2*c*s*bilinear(u, east),
                    2*c*s*bilinear(u, north), s*s*(ee-nn)/2, s*s*bilinear(east, north)]

    def value(angle):
        return math.fsum([coefficients[0], coefficients[1]*math.cos(angle),
                         coefficients[2]*math.sin(angle), coefficients[3]*math.cos(2*angle),
                         coefficients[4]*math.sin(2*angle)])

    values = [value(i*math.tau/64) for i in range(64)]

    def extremum(maximize):
        index = (max if maximize else min)(range(64), key=lambda i: values[i])
        a, b = (index-1)*math.tau/64, (index+1)*math.tau/64
        ratio = (math.sqrt(5)-1)/2
        x, y = b-ratio*(b-a), a+ratio*(b-a)
        fx, fy = value(x), value(y)
        for _ in range(48):
            if (fx < fy) == maximize:
                a, x, fx = x, y, fy
                y = a+ratio*(b-a)
                fy = value(y)
            else:
                b, y, fy = y, x, fx
                x = b-ratio*(b-a)
                fx = value(x)
        return (max if maximize else min)(fx, fy)

    forward = dot(rr, [x/a for x, a in zip(u, radii)])
    return extremum(False), extremum(True), uu, forward


def classify(margins):
    low, high, centre, forward = margins
    if forward >= 0:
        return None
    if low >= 0:
        return 'Umbra'
    if high >= 0:
        return 'Penumbra'
    if centre >= 0:
        return 'Antumbra'
    return None


def sphere_contract():
    r, a, beta = [-7000, 0, 0], 6400, .0046
    alpha = math.asin(a/7000)
    for separation, expected in [(0, 'Umbra'), (alpha-2*beta, 'Umbra'),
                                 (alpha-beta/2, 'Penumbra'), (alpha+beta/2, 'Penumbra'),
                                 (alpha+2*beta, None)]:
        u = [math.cos(separation), math.sin(separation), 0]
        assert classify(limb_extrema(r, u, beta, [a]*3)) == expected


class Trajectory:
    def __init__(self, data, radius=7000, phase=0, no_events=False, anti=False):
        self.data, self.radius, self.phase = data, radius, phase
        sun = unit(data.sun_state(0)[:3])
        vertical = unit([z-dot([0, 0, 1], sun)*s for z, s in zip([0, 0, 1], sun)])
        horizontal = cross(sun, vertical)
        self.p, self.q = (vertical, horizontal) if no_events else ([-x for x in sun] if anti else sun, vertical)
        self.rate, self.speed = math.sqrt(MU/radius**3), math.sqrt(MU/radius)

    def state(self, time):
        angle = self.phase+self.rate*time
        return [self.radius*(math.cos(angle)*p+math.sin(angle)*q) for p, q in zip(self.p, self.q)]+[
            self.speed*(-math.sin(angle)*p+math.cos(angle)*q) for p, q in zip(self.p, self.q)]

    @functools.lru_cache(maxsize=None)
    def margins(self, time):
        # The closed-form state and GUI inputs are FK5/MJ2000Eq; the Sun
        # ephemeris and terrestrial orientation returned by CSPICE are ICRF.
        r = self.data.fk5_to_icrf(time, self.state(time)[:3])
        sun = [x-y for x, y in zip(self.data.sun_state(time)[:3], r)]
        beta = math.asin(self.data.sun/norm(sun))
        # In the selected annular case, a fully contained Earth necessarily
        # covers the solar centre; this excludes more general disk geometries.
        assert beta < 2*math.asin(min(self.data.earth)/self.radius)
        rotation = self.data.rotation(time)
        fixed_r = [dot(row, r) for row in rotation]
        fixed_u = [dot(row, unit(sun)) for row in rotation]
        return limb_extrema(fixed_r, fixed_u, beta, self.data.earth)

    def intervals(self, start=0, end=DURATION):
        grid = list(range(int(start), int(end)+1, 30))
        if grid[-1] != end:
            grid.append(end)
        boundaries = [float(start), float(end)]
        for component in range(4):
            for a, b in zip(grid, grid[1:]):
                fa, fb = self.margins(a)[component], self.margins(b)[component]
                if fa*fb >= 0:
                    continue
                left, right = float(a), float(b)
                for _ in range(40):
                    if right-left < 2e-6:
                        break
                    mid = (left+right)/2
                    fm = self.margins(mid)[component]
                    if fa*fm > 0:
                        left, fa = mid, fm
                    else:
                        right = mid
                boundaries.append((left+right)/2)
        boundaries.sort()
        unique = []
        for time in boundaries:
            if not unique or time-unique[-1] > 1e-5:
                unique.append(time)
        result = []
        for a, b in zip(unique, unique[1:]):
            kind = classify(self.margins((a+b)/2))
            if kind is None:
                continue
            if result and result[-1]['type'] == kind and abs(result[-1]['end']-a) < 1e-5:
                result[-1]['end'] = b
            else:
                result.append(dict(start=a, end=b, type=kind, body='Earth'))
        return result


def groups(events):
    result = []
    for event in events:
        if not result or event['start']-result[-1]['end'] > .001:
            result.append(dict(start=event['start'], end=event['end'], events=[event]))
        else:
            result[-1]['end'] = max(result[-1]['end'], event['end'])
            result[-1]['events'].append(event)
    return result


def fixtures(library, root):
    sphere_contract()
    data = Data(library, root)
    nominal, alternate = Trajectory(data), Trajectory(data, phase=math.pi/6)
    far, clear = Trajectory(data, radius=2e6, anti=True), Trajectory(data, no_events=True)
    configurations = [('Umbra', nominal, ['Umbra']), ('Penumbra', nominal, ['Penumbra']),
                      ('Mixed', nominal, ['Umbra', 'Penumbra']),
                      ('AllTypes', nominal, ['Umbra', 'Penumbra', 'Antumbra']),
                      ('Antumbra', far, ['Antumbra']), ('NoEvents', clear, ['Umbra', 'Penumbra', 'Antumbra']),
                      ('Alternate', alternate, ['Umbra', 'Penumbra']),
                      ('WriteOff', nominal, ['Umbra', 'Penumbra']), ('Bounded', nominal, ['Umbra', 'Penumbra'])]
    cases = []
    for key, trajectory, types in configurations:
        start, end = (2500, 3500) if key == 'Bounded' else (0, DURATION)
        events = [dict(e) for e in trajectory.intervals(start, end) if e['type'] in types]
        totals = groups(events)
        for i, group in enumerate(totals, 1):
            for event in group['events']:
                event.update(group=i, duration=event['end']-event['start'], total_duration=group['end']-group['start'])
        maximum = max((g['end']-g['start'] for g in totals), default=0)
        index = next((i+1 for i, g in enumerate(totals) if g['end']-g['start'] == maximum), 0)
        target = 'Other' if key == 'Alternate' else 'Sat'
        per_type = {}
        for kind in types:
            matching = [e for e in events if e['type'] == kind]
            longest = max((e['duration'] for e in matching), default=0)
            participants = sorted({target+'/'+e['body'] for e in matching if abs(e['duration']-longest) <= 1e-6})
            per_type[kind] = dict(count=len(matching), maximum=longest, participants=participants)
        cases.append(dict(key=key, types=types, target=target, per_type=per_type,
                          radius=trajectory.radius, mu=MU, duration=DURATION,
                          initial_state=trajectory.state(0), final_state=trajectory.state(DURATION),
                          other_state=nominal.state(0), write_report=key != 'WriteOff',
                          bounded=key == 'Bounded', events=events, total_events=len(totals),
                          maximum_duration=maximum, maximum_group=index,
                          time_tolerance=.02, duration_tolerance=.03))
    assert len(cases[0]['events']) == 1 and len(cases[1]['events']) == 2
    assert len(cases[2]['events']) == 3 and cases[4]['events'] and not cases[5]['events']
    return dict(cases=cases, physical_inputs=dict(tt0=data.tt0, earth_radii=data.earth,
                sun_radius=data.sun, kernels={name: hashlib.sha256((root/name).read_bytes()).hexdigest() for name in KERNELS+[FRAME_TABLE]}),
                allowed_scientific_spice_calls=['str2et_c', 'unitim_c', 'spkezr_c', 'pxform_c', 'bodvrd_c'],
                scope='Independent ray/ellipsoid geometry and bisection for selected prescribed circular Earth orbits; SPICE supplies shared ephemeris, shape, terrestrial orientation and time inputs only. FK5/ICRF conversion uses the shared hashed Euler-vector data with independent polynomial interpolation and Rodrigues rotation. No GMAT/event/GF function is called; independent frame/ephemeris truth, general light-time, arbitrary bodies/orbits, all annular geometries, native OS input/platform/stress and full current-build qualification remain open.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--library', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    value = fixtures(args.library, root)
    path = Path(__file__).with_name('NasaEclipseResultCases.json')
    path.write_text(json.dumps(value, indent=2)+'\n')
    print(json.dumps({c['key']: [(e['type'], e['start'], e['end']) for e in c['events']] for c in value['cases']}, indent=2))
