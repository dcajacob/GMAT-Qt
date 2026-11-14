"""Independent ERFA/SOFA frame comparison; no GMAT time or frame API.

IAU1976/1980 equinox reduction and IAU2006/2000A CIO reduction share a
terrestrial frame. Their composition follows NASA's CoordinateSystem guide.
The shipped GMAT table is the observation, never the expected reference.
Disagreement is diagnostic until model/time/data provenance is reconciled.
"""
import argparse
import bisect
import hashlib
import json
import math
from pathlib import Path
import warnings

import erfa
import numpy as np

GMAT_JD_OFFSET = 2430000.0
A1_MINUS_TAI_SECONDS = 0.0343817
REQUIRED_ERFA = '2.0.1.5'


def need(ok, message):
    if not ok:
        raise ValueError(message)


def read_eop(path):
    rows = []
    for line in Path(path).read_text().splitlines():
        parts = line.split()
        if len(parts) < 7 or not parts[0].isdigit() or len(parts[0]) != 4:
            continue
        year, month, day = map(int, parts[:3])
        _, calendar_mjd = erfa.cal2jd(year, month, day)
        mjd, dut1 = float(parts[3]), float(parts[6])
        need(mjd == calendar_mjd and math.isfinite(dut1), 'Invalid EOP date/UT1 input')
        rows.append((mjd, dut1))
    need(len(rows) > 1 and all(a[0] < b[0] for a, b in zip(rows, rows[1:])),
         'EOP rows are missing, unordered or duplicate')
    return rows


def times(a1_mjd, eop):
    need(math.isfinite(a1_mjd), 'Nonfinite input epoch')
    # Use split dates throughout. Combining a full JD first loses precision
    # at Earth-rotation rates and must not be used to fit the GMAT table.
    tai = (GMAT_JD_OFFSET, a1_mjd - A1_MINUS_TAI_SECONDS / 86400)
    with warnings.catch_warnings():
        warnings.simplefilter('error', erfa.ErfaWarning)
        tt = erfa.taitt(*tai)
        utc = erfa.taiutc(*tai)
    utc_mjd = (utc[0] - 2400000.5) + utc[1]
    need(eop[0][0] <= utc_mjd <= eop[-1][0], 'Epoch outside retained EOP coverage')
    index = max(0, min(len(eop) - 2, bisect.bisect_right([r[0] for r in eop], utc_mjd) - 1))
    left, right = eop[index:index + 2]
    need(right[0] - left[0] == 1, 'Missing adjacent daily EOP input')
    # Do not interpolate a UTC leap jump as an ordinary continuous DUT1 value.
    # A dedicated leap-interval interpolation contract is outside this probe.
    need(abs(right[1] - left[1]) < .5, 'UTC leap interval requires separate EOP treatment')
    fraction = (utc_mjd - left[0]) / (right[0] - left[0])
    dut1 = left[1] + fraction * (right[1] - left[1])
    with warnings.catch_warnings():
        warnings.simplefilter('error', erfa.ErfaWarning)
        ut1 = erfa.utcut1(*utc, dut1)
    return dict(tai=list(tai), tt=list(tt), utc=list(utc), ut1=list(ut1), utc_mjd=utc_mjd,
                dut1_seconds=dut1, eop_bracket=[list(left), list(right)])


def reference(tt, ut1):
    """Return ICRF-to-FK5 rotation through one common terrestrial system.

    Common polar motion, including its locator, cancels in the composition.
    The legacy equation of equinoxes uses TT; sidereal time uses UT1. This
    targets post-1997 dates where NASA uses the IAU1994 complementary terms.
    """
    need(tt[0] + tt[1] > 2450449.5, 'Pre-1997 NASA equation-of-equinoxes policy is separate')
    celestial_to_true = erfa.pnm80(*tt)
    gast = erfa.gmst82(*ut1) + erfa.eqeq94(*tt)
    legacy = erfa.c2teqx(celestial_to_true, gast, np.eye(3))
    modern = erfa.c2tcio(erfa.c2i06a(*tt), erfa.era00(*ut1), np.eye(3))
    result = legacy.T @ modern
    need(np.all(np.isfinite(result)) and np.max(np.abs(result @ result.T - np.eye(3))) < 2e-15
         and abs(np.linalg.det(result) - 1) < 2e-15, 'Invalid composed reference rotation')
    return result


def compare(table, epoch, eop):
    # Existing pure-Python table interpolation is an observation decoder. It
    # supplies no expected frame, ERFA values, correction or fitted parameter.
    from Fk5DynamicsOracle import rotation
    clock = times(epoch, eop)
    expected = reference(clock['tt'], clock['ut1'])
    actual = np.array([rotation(table, epoch, unit) for unit in np.eye(3)]).T
    matrix_error = float(np.max(np.abs(actual - expected)))
    vectors = [np.array(v, dtype=float) for v in ([7000, -1200, 900], [1200, 7800, -2400],
                                                [-3300, 400, 9200])]
    errors = [float(np.max(np.abs((actual - expected) @ vector))) for vector in vectors]
    contrasts = [float(np.max(np.abs((expected - expected.T) @ vector))) for vector in vectors]
    return dict(epoch_a1_mjd=epoch, clock=clock, independent_matrix=expected.tolist(),
                observed_table_matrix=actual.tolist(), maximum_matrix_difference=matrix_error,
                vectors=[v.tolist() for v in vectors], maximum_position_differences_km=errors,
                wrong_transpose_contrasts_km=contrasts,
                comparison_status='diagnostic discrepancy' if matrix_error > 2e-14 else 'numerical agreement',
                no_correction_or_tolerance_fitted=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--table', type=Path, required=True)
    parser.add_argument('--eop', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--epoch', type=float, action='append', required=True)
    args = parser.parse_args()
    need(erfa.__version__ == REQUIRED_ERFA, 'Use the pinned PyERFA dependency')
    table_bytes, eop_bytes = args.table.read_bytes(), args.eop.read_bytes()
    table = [list(map(float, line.split(','))) for line in table_bytes.decode().splitlines() if line.strip()]
    need(all(len(row) == 4 and all(map(math.isfinite, row)) for row in table)
         and all(a[0] < b[0] for a, b in zip(table, table[1:])), 'Invalid observed rotation table')
    eop = read_eop(args.eop)
    observations = [compare(table, epoch, eop) for epoch in args.epoch]
    report = dict(status='independent-reference-diagnostic; no NASA credit',
                  versions=dict(pyerfa=erfa.__version__, erfa=erfa.version.erfa_version,
                                sofa=erfa.version.sofa_version),
                  inputs={str(args.table): hashlib.sha256(table_bytes).hexdigest(),
                          str(args.eop): hashlib.sha256(eop_bytes).hexdigest()},
                  observations=observations, new_requirement_credit=[], core_changes=0,
                  limits=['Comparison of two stated theoretical reductions, not observed astrometric qualification',
                          'GMAT table generator/time/model provenance still requires reconciliation',
                          'No fitted bias, residual subtraction or relaxed existing assertion',
                          'Post-1997, retained EOP coverage and ordinary UTC day intervals only',
                          'Full force/body/frame conjunction for FRR!E144 remains open'])
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(status=report['status'], epochs=len(observations),
                         maximum_matrix_difference=max(o['maximum_matrix_difference'] for o in observations))))
    # A completed diagnostic is not silently green when the models disagree.
    # This exit code does not classify the discrepancy as a GMAT defect.
    return 2 if any(o['comparison_status'] == 'diagnostic discrepancy' for o in observations) else 0


if __name__ == '__main__':
    raise SystemExit(main())
