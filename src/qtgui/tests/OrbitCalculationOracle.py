"""Independent hyperbolic acceptance fixtures, using vector/conic geometry.

No GMAT, native conversion utility or ephemeris is imported. The incoming
impact vector follows angular momentum conservation, B = S cross h / v_inf.
Outgoing setters preserve conic size, shape, true anomaly and B-plane azimuth.
See NASA CalculationObjects.tex and the Spacecraft Orbit State guide.
"""
import json
import math
from pathlib import Path

from OrbitalParameterOracle import MU, CARTESIAN, conic

ELEMENTS = [-18000.0, 1.5, 35.0, 40.0, 25.0, 55.0]
OBLIQUITY = math.radians(84381.448 / 3600)


def dot(a, b):
    return math.fsum(x*y for x, y in zip(a, b))


def cross(a, b):
    return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]


def norm(a):
    return math.sqrt(dot(a, a))


def unit(a):
    return [x/norm(a) for x in a]


def rotate(state, frame, inverse=False):
    if frame == 'EarthMJ2000Eq':
        return list(state)
    assert frame == 'EarthMJ2000Ec'
    c, s = math.cos(OBLIQUITY), math.sin(OBLIQUITY) * (-1 if inverse else 1)
    result = []
    for i in [0, 3]:
        x, y, z = state[i:i+3]
        result += [x, c*y+s*z, -s*y+c*z]
    return result


def geometry(state, mu=MU):
    r, v = state[:3], state[3:]
    h = cross(r, v)
    ev = [(dot(v, v)-mu/norm(r))*x/mu-dot(r, v)*y/mu for x, y in zip(r, v)]
    e = norm(ev)
    a = 1/(2/norm(r)-dot(v, v)/mu)
    assert a < 0 and e > 1
    p, normal = unit(ev), unit(h)
    q = cross(normal, p)
    k = math.sqrt(1-1/e**2)
    incoming = [x/e+k*y for x, y in zip(p, q)]
    outgoing = [-x/e+k*y for x, y in zip(p, q)]
    impact = [x/math.sqrt(-mu/a) for x in cross(incoming, h)]
    t = unit(cross(incoming, [0, 0, 1]))
    rr = cross(incoming, t)
    bt, br = dot(impact, t), dot(impact, rr)
    east = unit(cross([0, 0, 1], outgoing))
    north = cross(outgoing, east)
    bdir = cross(normal, outgoing)
    azimuth = math.atan2(dot(bdir, east), -dot(bdir, north))
    anomaly = math.atan2(dot(r, q), dot(r, p))
    assert abs(dot(impact, incoming)) < 1e-10
    assert abs(norm(impact)-abs(a)*math.sqrt(e*e-1)) < 1e-9
    return dict(a=a, e=e, anomaly=anomaly, azimuth=azimuth,
                BdotT=bt, BdotR=br, BVectorAngle=math.degrees(math.atan2(br, bt)),
                BVectorMag=norm(impact), OutgoingRHA=math.degrees(math.atan2(outgoing[1], outgoing[0])) % 360,
                OutgoingDHA=math.degrees(math.asin(outgoing[2])))


def set_outgoing(state, parameter, value, mu=MU):
    g = geometry(state, mu)
    rha = math.radians(value if parameter == 'OutgoingRHA' else g['OutgoingRHA'])
    dha = math.radians(value if parameter == 'OutgoingDHA' else g['OutgoingDHA'])
    s = [math.cos(dha)*math.cos(rha), math.cos(dha)*math.sin(rha), math.sin(dha)]
    east = unit(cross([0, 0, 1], s))
    north = cross(s, east)
    bdir = [math.sin(g['azimuth'])*x-math.cos(g['azimuth'])*y for x, y in zip(east, north)]
    normal = cross(s, bdir)
    p = [-x/g['e']-math.sqrt(1-1/g['e']**2)*y for x, y in zip(s, bdir)]
    q = cross(normal, p)
    semilatus = g['a']*(1-g['e']**2)
    nu = g['anomaly']
    radius = semilatus/(1+g['e']*math.cos(nu))
    result = [radius*(math.cos(nu)*x+math.sin(nu)*y) for x, y in zip(p, q)]
    result += [math.sqrt(mu/semilatus)*(-math.sin(nu)*x+(g['e']+math.cos(nu))*y) for x, y in zip(p, q)]
    check = geometry(result, mu)
    for name in ['a', 'e', 'anomaly', 'azimuth']:
        assert abs(check[name]-g[name]) < (1e-9 if name == 'a' else 1e-12)
    assert abs(check[parameter]-value) < 1e-11
    return result


def calculate(case):
    initial = conic(case['initial_elements'], case['mu'])
    local = rotate(initial, case['frame'])
    parameter = case['parameter']
    if case['read_only']:
        scalar = geometry(local, case['mu'])[parameter]
        # Both the actual Assignment RHS result and a direct parameter read.
        return [scalar, scalar] + initial
    changed = set_outgoing(local, parameter, case['assigned_value'], case['mu'])
    return [case['assigned_value']] + rotate(changed, case['frame'], inverse=True)


def fixtures():
    state = conic(ELEMENTS)
    setup = 'Sat.DateFormat = TAIModJulian;\nSat.Epoch = 21545;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\n'
    setup += ''.join('Sat.%s = %.17g;\n' % (name, value) for name, value in zip(CARTESIAN, state))
    setup += 'Sat.DisplayStateType = OutgoingAsymptote;\n'
    rows = dict(BdotT=529, BdotR=530, BVectorAngle=531, BVectorMag=532, OutgoingRHA=547, OutgoingDHA=548)
    cases = []
    for parameter, row in rows.items():
        for suffix, frame in [('Eq', 'EarthMJ2000Eq'), ('Ec', 'EarthMJ2000Ec')]:
            readonly = parameter.startswith('B')
            token = 'Sat.'+frame+'.'+parameter
            case = dict(key=parameter+suffix, parameter=parameter, frame=frame, mu=MU,
                        initial_elements=ELEMENTS, representation='OutgoingAsymptote', orbital_case=True,
                        setup=setup, destination='R' if readonly else token, before='0' if readonly else token,
                        expression=token if readonly else '220' if parameter == 'OutgoingRHA' else '-25',
                        report=('R ' if readonly else '')+token+' '+' '.join('Sat.EarthMJ2000Eq.'+c for c in CARTESIAN),
                        invalid=['AbsentNASA + 1', '1 +'], absolute_tolerance=2e-8, relative_tolerance=2e-12,
                        column_absolute_tolerances=([2e-8]*2 if readonly else [1e-9])+[2e-8]*3+[2e-11]*3,
                        requirements=['FRR!E'+str(row)], read_only=readonly)
            if readonly:
                case['reject_destination'] = token
            else:
                case['assigned_value'] = 220.0 if parameter == 'OutgoingRHA' else -25.0
            case['expected'] = calculate(case)
            cases.append(case)
    return dict(cases=cases, reference_sources=['doc/SystemDocs/MathematicalSpecification/CalculationObjects.tex',
                'doc/help/src/System_CalculationParameters.xml', 'doc/help/src/Resource_SpacecraftOrbitState.xml'],
                scope='Selected nonsingular Earth hyperbola, equatorial/ecliptic inertial parameter frames; full six-component Cartesian witnesses. B-plane RHS results and readonly rejection; outgoing RHA/DHA setters. No rotating frame, arbitrary body/domain, degeneracy, physical-invalid/runtime-error or platform/full-build qualification.')


if __name__ == '__main__':
    path = Path(__file__).with_name('NasaOrbitCalculationCases.json')
    path.write_text(json.dumps(fixtures(), indent=2)+'\n')
    print('Wrote 12 independent vector-geometry fixtures:', path)
