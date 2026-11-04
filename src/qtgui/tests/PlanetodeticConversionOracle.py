"""Six missing GUI edges, with independent spheroid and local sky geometry.

Retain all four earlier spheroid cases. Existing conversion edges and tests are
not regenerated. These references do not qualify the whole FRR!E13 family.
"""
import json
import math
from pathlib import Path
import PlanetodeticOracle as planet


def fixture():
    root = Path(__file__).resolve().parent
    meta = {c['key']: dict(names=c['names'], units=c['units'])
            for c in json.loads((root/'NasaOrbitDefinitionCases.json').read_text())['cases']}
    types = ['Cartesian', 'SphericalAZFPA', 'SphericalRADEC', 'Planetodetic']
    cases = []
    for sample in planet.cases():
        cart = sample['cartesian']; r, v = cart[:3], cart[3:]
        mag = lambda x: math.sqrt(sum(a*a for a in x))
        R, V = mag(r), mag(v)
        lon, lat = math.atan2(r[1], r[0]), math.asin(r[2]/R)
        north = [-math.sin(lat)*math.cos(lon), -math.sin(lat)*math.sin(lon), math.cos(lat)]
        east = [-math.sin(lon), math.cos(lon), 0]
        dot = lambda a, b: sum(x*y for x, y in zip(a, b))
        az = math.degrees(math.atan2(dot(v, east), dot(v, north))) % 360
        fpa = math.degrees(math.acos(dot(r, v)/(R*V)))
        states = {'Cartesian': cart,
            'SphericalAZFPA': [R, math.degrees(lon)%360, math.degrees(lat), V, az, fpa],
            'SphericalRADEC': [R, math.degrees(lon)%360, math.degrees(lat), V,
                math.degrees(math.atan2(v[1], v[0]))%360, math.degrees(math.asin(v[2]/V))],
            'Planetodetic': sample['values']}
        metadata = {**meta, 'Planetodetic': dict(names=sample['names'], units=sample['units'])}
        for source in types:
            targets = [t for t in types if t != source] if source == 'Planetodetic' else ['Planetodetic']
            cases.append(dict(key=sample['key']+'_'+source, source=source, shape=sample['key'],
                flattening=sample['flattening'], equatorial_radius=sample['equatorial_radius'],
                mu=398600.4418, cartesian=cart, expected_pairs=len(targets),
                targets=[dict(key=t, values=states[t], **metadata[t]) for t in targets]))
    return dict(schema_version=1,
        scope='Six previously unexecuted directed Planetodetic/Cartesian/spherical GUI edges on four spheroids in a body-fixed Earth frame; existing98fixtureedges not replayed. Conic/inertial cross-frame, other14type pairs, singularities, model/averaging/convention and wholeFRR13 gates remain open.',
        position_tolerance_km=1e-6, velocity_tolerance_km_s=1e-9,
        displayed_field_tolerance=1e-6, cases=cases)


if __name__ == '__main__':
    import argparse
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args(); a.output.write_text(json.dumps(fixture(), indent=2)+'\n')
