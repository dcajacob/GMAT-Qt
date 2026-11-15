"""Audit the missing harmonic-field provider against the frozen monopole truth."""
import json
from pathlib import Path
from StmOwnerFrameOracle import audit as owner_audit


def audit(root, fixture):
    root = Path(root)
    plan = json.loads(Path(fixture).read_text())
    force = json.loads((root / 'force-provider.json').read_text())
    if (force['type'] != plan['force_provider'] or force['body'] != 'Earth' or
            force['mu'] != plan['mu'] or force['degree'] != plan['degree'] or
            force['order'] != plan['order'] or force['stm_limit'] != plan['stm_limit'] or
            Path(force['potential']).resolve() != (root / 'STM-monopole.cof').resolve()):
        raise ValueError('Executed harmonic force provider or potential differs')
    lines = (root / 'STM-monopole.cof').read_text().splitlines()
    header = next(row.split() for row in lines if row.startswith('POTFIELD'))
    if (int(header[1]) != 2 or int(header[2]) != 2 or int(header[3]) != 1 or
            float(header[4]) != plan['mu'] * 1e9 or
            float(header[5]) != plan['potential_radius_km'] * 1000):
        raise ValueError('Independent potential header/constant check differs')
    coefficients = [row.split() for row in lines if row.startswith('RECOEF')]
    if ([(int(row[1]), int(row[2])) for row in coefficients] != [(2, m) for m in range(3)] or
            any(float(v) != 0 for row in coefficients for v in row[3:])):
        raise ValueError('Synthetic monopole fixture has nonzero or missing coefficients')
    result = owner_audit(root, fixture)
    result['force_provider'] = 'GravityField'
    result['force_degree'] = 0
    result['nonzero_harmonic_degree_qualified'] = False
    return result
