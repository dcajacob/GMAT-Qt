"""Independent nonsingular elliptic six-representation reference equations.

Uses only scalar geometry and the Kepler eccentric-anomaly equation. Does not
import GMAT or call StateConversionUtil. Scope: body-centered inertial axes,
positive SMA, 0 < ECC < 1, and 0 < INC < 180 degrees; true-anomaly conics.
"""
import math

REPRESENTATIONS = ('Cartesian', 'Keplerian', 'ModifiedKeplerian', 'Equinoctial',
                   'SphericalAZFPA', 'SphericalRADEC')


def representations(elements, mu):
    a, e, inc, node, peri, anomaly = elements
    if not (a > 0 and 0 < e < 1 and 0 < inc < 180 and mu > 0):
        raise ValueError('Reference requires a nonsingular elliptic orbit')
    i, om, w, nu = map(math.radians, [inc, node, peri, anomaly])
    cn, sn, ci, si, cw, sw = math.cos(om), math.sin(om), math.cos(i), math.sin(i), math.cos(w), math.sin(w)
    p_axis = [cn*cw-sn*sw*ci, sn*cw+cn*sw*ci, sw*si]
    q_axis = [-cn*sw-sn*cw*ci, -sn*sw+cn*cw*ci, cw*si]
    p = a*(1-e*e)
    radius, speed_scale = p/(1+e*math.cos(nu)), math.sqrt(mu/p)
    position = [radius*(p_axis[k]*math.cos(nu)+q_axis[k]*math.sin(nu)) for k in range(3)]
    velocity = [speed_scale*(-p_axis[k]*math.sin(nu)+q_axis[k]*(e+math.cos(nu))) for k in range(3)]
    speed = math.sqrt(sum(v*v for v in velocity))
    ra = math.atan2(position[1],position[0])
    dec = math.asin(position[2]/radius)
    north = [-math.sin(dec)*math.cos(ra), -math.sin(dec)*math.sin(ra), math.cos(dec)]
    east = [-math.sin(ra), math.cos(ra), 0.]
    az = math.degrees(math.atan2(sum(x*y for x,y in zip(velocity,east)),
                                sum(x*y for x,y in zip(velocity,north)))) % 360
    fpa = math.degrees(math.acos(sum(x*y for x,y in zip(position,velocity))/(radius*speed)))
    eccentric = math.atan2(math.sqrt(1-e*e)*math.sin(nu), e+math.cos(nu))
    mean = eccentric-e*math.sin(eccentric)
    return dict(Cartesian=position+velocity, Keplerian=list(elements),
        ModifiedKeplerian=[a*(1-e),a*(1+e),inc,node,peri,anomaly],
        Equinoctial=[a,e*math.sin(om+w),e*math.cos(om+w),math.tan(i/2)*math.sin(om),
                     math.tan(i/2)*math.cos(om),math.degrees(om+w+mean)%360],
        SphericalAZFPA=[radius,math.degrees(ra)%360,math.degrees(dec),speed,az,fpa],
        SphericalRADEC=[radius,math.degrees(ra)%360,math.degrees(dec),speed,
                        math.degrees(math.atan2(velocity[1],velocity[0]))%360,
                        math.degrees(math.asin(velocity[2]/speed))])


def fixture(definitions):
    """Reuse existing independently defined physical orbits; add new pair edges."""
    metadata = {c['key']:dict(names=c['names'],units=c['units']) for c in definitions['cases']}
    result = []
    for old in definitions['cases']:
        expected = representations(old['canonical_keplerian'],old['mu'])
        for a,b in zip(expected['Cartesian'],old['expected_cartesian']):
            if abs(a-b) > 1e-10:
                raise ValueError('Existing independent Cartesian reference differs')
        result.append(dict(key=old['key'],mu=old['mu'],keplerian=old['canonical_keplerian'],
            cartesian=expected['Cartesian'],targets=[dict(key=k,values=expected[k],**metadata[k])
                for k in REPRESENTATIONS if k!=old['key']]))
    return dict(schema_version=1,scope='30 directed GUI pairs among six representations; six distinct elliptic orbits,EarthMJ2000Eq,true anomaly only; all-frame/singular/hyperbolic/other-representation parent scope remains open',
                position_tolerance_km=1e-6,velocity_tolerance_km_s=1e-9,
                displayed_field_tolerance=1e-6,cases=result)


if __name__ == '__main__':
    import argparse
    import json
    from pathlib import Path
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--definitions',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    args = p.parse_args()
    args.output.write_text(json.dumps(fixture(json.loads(args.definitions.read_text())),indent=2)+'\n')
