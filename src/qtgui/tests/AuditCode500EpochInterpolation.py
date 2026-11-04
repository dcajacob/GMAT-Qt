"""Independently solve the recorded Code500 cubic spline and check file clocks."""
import argparse
from collections import Counter
from datetime import date
from decimal import Decimal, localcontext
import json
import math
from pathlib import Path


def check(value, message):
    if not value:
        raise ValueError(message)


def D(x):
    return Decimal.from_float(float(x))


def stamp_seconds(parts):
    return D(parts['day'])*86400+D(parts['sec'])+D(parts['frac'])


def polynomial(degree, t):
    r, v = [9000, 100, 300], [-.1, 7.1, .4]
    a = [(i+1)*1e-5 if degree == 3 else 0 for i in range(3)]
    b = [(i-1)*1e-9 if degree == 3 else 0 for i in range(3)]
    return [r[i]+v[i]*t+a[i]*t*t+b[i]*t*t*t for i in range(3)] + [
        v[i]+2*a[i]*t+3*b[i]*t*t for i in range(3)]


def file_day(encoded):
    y = int(encoded)//10000
    return date(1900+y, int(encoded)//100 % 100, int(encoded) % 100)


def encoded_seconds(file, index):
    block, line = divmod(index, 50)
    data, first = file['records'][block], file['records'][0]
    return (Decimal((file_day(data['date'])-file_day(first['date'])).days)*86400 +
            D(data['seconds_of_day'])-D(first['seconds_of_day']) + D(data['step_seconds'])*line)


def coefficients(points):
    # Six independently solved 16-equation systems: two endpoint values for
    # each cubic, C1/C2 continuity at three knots, and equality of the third
    # derivatives at the first and last interior knots (not-a-knot).
    xs = [D(p['x_seconds']) for p in points]
    origin, scale = xs[0], xs[-1]-xs[0]
    u = [(x-origin)/scale for x in xs]
    rows, rhs = [], []

    def basis(x, derivative):
        return ([1, x, x*x, x*x*x] if derivative == 0 else
                [0, 1, 2*x, 3*x*x] if derivative == 1 else
                [0, 0, 2, 6*x] if derivative == 2 else [0, 0, 0, 6])

    def equation(terms, values):
        row = [Decimal(0)]*16
        for segment, x, derivative, sign in terms:
            for i, value in enumerate(basis(x, derivative)):
                row[4*segment+i] += Decimal(value)*sign
        rows.append(row)
        rhs.append([D(v) for v in values])

    for segment in range(4):
        equation([(segment, u[segment], 0, 1)], points[segment]['state'])
        equation([(segment, u[segment+1], 0, 1)], points[segment+1]['state'])
    for knot in range(1, 4):
        for derivative in [1, 2]:
            equation([(knot-1, u[knot], derivative, 1), (knot, u[knot], derivative, -1)], [0]*6)
    for knot in [1, 3]:
        equation([(knot-1, u[knot], 3, 1), (knot, u[knot], 3, -1)], [0]*6)
    augmented = [a+b for a, b in zip(rows, rhs)]
    for col in range(16):
        pivot = max(range(col, 16), key=lambda i: abs(augmented[i][col]))
        check(augmented[pivot][col] != 0, 'Singular independent spline system')
        augmented[col], augmented[pivot] = augmented[pivot], augmented[col]
        value = augmented[col][col]
        augmented[col] = [x/value for x in augmented[col]]
        for row in range(16):
            if row != col:
                factor = augmented[row][col]
                augmented[row] = [a-factor*b for a, b in zip(augmented[row], augmented[col])]
    return origin, scale, xs, [row[16:] for row in augmented]


def audit(proof):
    check(proof['checkpoint'] == 'code500-relative-epoch-interpolation' and
          proof['outcome'] == 'recorded-for-independent-audit', 'Wrong support proof')
    check(proof['position_bound_km'] == 1e-7 and proof['velocity_bound_km_s'] == 1e-10 and
          proof['whole_row_qualified'] is False and proof['new_requirement_credit'] == 0 and
          proof['runtime_or_kernel_copies'] == 0, 'Bounds or qualification gates changed')
    files = {f['key']: f for f in proof['files']}
    check(len(files) == 24 and {(f['system'], f['endian'], f['degree'], f['count']) for f in files.values()} ==
          {(s, e, d, c) for s in ['A1', 'UTC'] for e in [1, 2] for d in [1, 3] for c in [9, 50, 112]},
          'File system/endian/degree/record universe differs')
    counts, cache, seen = Counter(), {}, set()
    with localcontext() as context:
        context.prec = 60
        for f in files.values():
            expected = Decimal(21545)+(Decimal('32.0343817')/86400 if f['system'] == 'UTC' else 0)
            check(abs((Decimal(f['origin_a1_gt'])-expected)*86400) < Decimal('1e-10'),
                  'Authored reference epoch differs')
        for row in proof['observations']:
            f = files[row['file']]
            key = (row['file'], row['direction'], row['api'], row['requested_seconds'])
            check(key not in seen, 'Duplicate query')
            seen.add(key)
            check(row['api'] in {'Real', 'GmatTime', 'RoundedGmatTime'} and row['direction'] in {-1, 1},
                  'Query API/direction differs')
            # ToString rounds a long decimal MJD. Raw day/integer-second/fraction
            # components preserve exact endpoint identities for domain checks.
            x = stamp_seconds(row['query_components'])-stamp_seconds(f['origin_components'])
            check(abs(x-(Decimal(row['query_a1_gt'])-Decimal(f['origin_a1_gt']))*86400) < Decimal('1e-10'),
                  'Serialized clock diagnostic differs from raw clock components')
            # Representable Real endpoints retain their legacy node identity;
            # the precise overload evaluates the actual supplied epoch.
            if row['api'] == 'Real' and row['query_real'] == f['span_start_real']:
                x = encoded_seconds(f, 0)
            elif row['api'] == 'Real' and row['query_real'] == f['span_end_real']:
                x = encoded_seconds(f, f['count']-1)
            outside = x < encoded_seconds(f, 0) or x > encoded_seconds(f, f['count']-1)
            if row['rejected']:
                check(row['exception_kind'] == 'Gmat', 'Unexpected standard-library exception')
                check(outside, 'Native query rejected an epoch inside the encoded file span')
                counts['outside_queries_rejected'] += 1
                continue
            check(not outside, 'Native query silently extrapolated outside the encoded span')
            points = row['spline_inputs']
            check(len(points) == 5 and all(points[i]['x_seconds'] < points[i+1]['x_seconds'] for i in range(4)),
                  'Actual five-point spline input order differs')
            indices = []
            for point in points:
                index = min(range(f['count']), key=lambda i: abs(polynomial(f['degree'], 30*i)[0]-point['state'][0]))
                indices.append(index)
                expected = polynomial(f['degree'], 30*index)
                check(all(abs(a-b) <= (1e-8 if i < 3 else 1e-12)
                          for i, (a, b) in enumerate(zip(point['state'], expected))), 'Encoded state differs from authored polynomial')
                check(abs(D(point['x_seconds'])-encoded_seconds(f, index)) <= Decimal('1e-10'),
                      'Native interpolation lost encoded submicrosecond time bits')
            check(indices == list(range(indices[0], indices[0]+5)), 'Stencil nodes not consecutive')
            signature = json.dumps(points, sort_keys=True)
            if signature not in cache:
                cache[signature] = coefficients(points)
            origin, scale, xs, coeff = cache[signature]
            check(xs[0] <= x <= xs[-1], 'Native stencil does not contain query')
            segment = next(i for i in range(4) if xs[i] <= x <= xs[i+1])
            u = (x-origin)/scale
            expected = [float(sum(coeff[4*segment+j][i]*(Decimal(1) if j == 0 else u**j)
                                  for j in range(4))) for i in range(6)]
            check(len(row['actual']) == 6, 'Output state width differs')
            for i, (a, b) in enumerate(zip(row['actual'], expected)):
                check(math.isfinite(a) and abs(a-b) <= (1e-7 if i < 3 else 1e-10),
                      'Native spline differs from independent 16-equation solve')
                counts['independent_components'] += 1
            counts['successful_queries'] += 1
        counts['files'] = len(files)
        counts['independent_spline_systems'] = len(cache)
        counts['queries'] = len(seen)
        check(counts['queries'] == 1104, 'Query census differs')
    return dict(status='passed-code500-independent-epoch-spline-audit', counts=dict(counts),
                new_requirement_credit=0, whole_row_qualified=False,
                limits='Finite file/clock/endian/record matrix. Shared time conversion and native format reading are explicit inputs;no general format,leap,provider or scientific qualification.')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('proof', type=Path)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    result = audit(json.loads(args.proof.read_text()))
    check(not args.output.exists(), 'Never overwrite an audit')
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
