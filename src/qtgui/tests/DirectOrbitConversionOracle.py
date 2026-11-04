"""Previously untested direct native dispatch, using retained reference math.

This supplies scalar conversion/dispatch evidence. Brouwer expectations use
retained NASA prototype forward reconstruction, not
physical averaging truth. Elliptic asymptotes use the declared apoapsis-ray
extension. Frame/origin, singularity and whole FRR!E13 gates remain open.
"""
import hashlib
import json
import math
from pathlib import Path

import OrbitExtendedConversionOracle as extended
import AsymptoteEllipticOracle as asymptote
import HyperbolicConversionOracle as hyperbolic
import BrouwerPrototypeOracle as brouwer

ROOT = Path(__file__).resolve().parents[3]
TYPES = ['Cartesian', 'Keplerian', 'ModifiedKeplerian', 'SphericalAZFPA',
         'SphericalRADEC', 'Equinoctial', 'ModifiedEquinoctial',
         'AlternateEquinoctial', 'Delaunay', 'Planetodetic',
         'OutgoingAsymptote', 'IncomingAsymptote', 'BrouwerMeanShort', 'BrouwerMeanLong']


def mean_anomaly(kep):
    e, nu = kep[1], math.radians(kep[5])
    E = math.atan2(math.sqrt(1-e*e)*math.sin(nu), e+math.cos(nu))
    return math.degrees(E-e*math.sin(E)) % 360


def true_elements(osc):
    e = osc[1]
    E = brouwer.eccentric(math.radians(osc[5]), e)
    nu = math.degrees(math.atan2(math.sqrt(1-e*e)*math.sin(E), math.cos(E)-e)) % 360
    return [*osc[:5], nu]


def spherical_planetodetic(states):
    radius, lon, lat, speed, azimuth, radial_fpa = states['SphericalAZFPA']
    return [radius, lon, lat, speed, azimuth, 90-radial_fpa]


def fixture():
    base = json.loads((ROOT/'src/qtgui/tests/NasaOrbitExtendedConversionCases.json').read_text())
    orbits = {c['key']: c['keplerian'] for c in base['cases']}
    orbits.update({c['key']: c['canonical_keplerian'] for c in asymptote.fixture()['cases']})
    orbits['Planetodetic'] = [14500., .23, 53., 183., 69., 94.]
    functions = {kind: brouwer.adapt(ROOT/'prototype/StateConv'/name)[0] for kind, name in
                 [('BrouwerMeanShort', 'BROLYD_short.m'), ('BrouwerMeanLong', 'BROLYD.m')]}
    literal_mean = [10800., .17, 47., 112., 38., 143.]
    for kind, forward in functions.items():
        orbits[kind] = true_elements(forward(literal_mean))
    cases = []
    for source in TYPES:
        mu = 398600.4418
        kep = orbits[source]
        states = extended.representations(kep, mu)
        states.update({k: v for k, v in asymptote.representations(kep, mu).items()
                       if k in ('IncomingAsymptote', 'OutgoingAsymptote')})
        states['Planetodetic'] = spherical_planetodetic(states)
        for kind in functions:
            if source == kind: states[kind] = list(literal_mean)
        cases.append(dict(key='Elliptic_'+source, domain='elliptic', source=source,
                          mu=mu, flattening=0., equatorial_radius=6378.,
                          input=states[source], cartesian=states['Cartesian'],
                          targets=[dict(type=t, values=states.get(t),
                                        oracle='prototype-forward-reconstruction' if t.startswith('Brouwer') else 'independent-scalar-fields')
                                   for t in TYPES if t != source]))
    hyper = json.loads((ROOT/'src/qtgui/tests/NasaOrbitHyperbolicConversionCases.json').read_text())
    for case in hyper['cases']:
        states = hyperbolic.representations(case['keplerian'], case['mu'])
        cases.append(dict(key='Hyperbolic_'+case['key'], domain='hyperbolic', source=case['key'],
                          mu=case['mu'], flattening=0., equatorial_radius=6378.,
                          input=states[case['key']], cartesian=states['Cartesian'],
                          targets=[dict(type=t, values=states[t]) for t in hyperbolic.ALL if t != case['key']]))
    references = ['src/qtgui/tests/'+name for name in ['DirectOrbitConversionOracle.py',
                  'OrbitConversionOracle.py', 'OrbitExtendedConversionOracle.py',
                  'AsymptoteEllipticOracle.py', 'HyperbolicConversionOracle.py',
                  'BrouwerPrototypeOracle.py', 'NasaOrbitExtendedConversionCases.json',
                  'NasaOrbitHyperbolicConversionCases.json']]
    references += ['prototype/StateConv/BROLYD.m', 'prototype/StateConv/BROLYD_short.m']
    return dict(schema_version=1, scope=__doc__, types=TYPES,
                reference_sha256={p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in references},
                position_tolerance_km=1e-6, velocity_tolerance_km_s=1e-9,
                field_tolerance=1e-6, cases=cases)


if __name__ == '__main__':
    import argparse
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args(); args.output.write_text(json.dumps(fixture(), indent=2)+'\n')
