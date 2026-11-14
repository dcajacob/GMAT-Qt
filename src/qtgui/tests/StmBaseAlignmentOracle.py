"""Independent two-end STM algebra at literal ICRF table nodes.

Alignment is reconstructed with unit quaternions, not native converter output.
The native Cartesian contract treats the base-alignment rate as zero.
"""
import math
from StmFrameOracle import multiply, rotation


def transpose(value):
    return list(map(list, zip(*value)))


def inverse(value):
    n = len(value)
    work = [list(row) + [float(i == j) for j in range(n)] for i, row in enumerate(value)]
    for j in range(n):
        pivot = max(range(j, n), key=lambda i: abs(work[i][j]))
        work[j], work[pivot] = work[pivot], work[j]
        scale = work[j][j]
        if not math.isfinite(scale) or abs(scale) < 1e-15:
            raise ValueError('Singular independent Cartesian frame Jacobian')
        work[j] = [v / scale for v in work[j]]
        for i in range(n):
            if i != j:
                scale = work[i][j]
                work[i] = [a - scale*b for a, b in zip(work[i], work[j])]
    return [row[n:] for row in work]


def alignment(vector):
    angle = math.sqrt(sum(v*v for v in vector))
    w = math.cos(angle / 2)
    x, y, z = [-v * math.sin(angle / 2) / angle for v in vector]
    return [[1-2*(y*y+z*z), 2*(x*y-z*w), 2*(x*z+y*w)],
            [2*(x*y+z*w), 1-2*(x*x+z*z), 2*(y*z-x*w)],
            [2*(x*z-y*w), 2*(y*z+x*w), 1-2*(x*x+y*y)]]


def axis(spec, seconds):
    if spec['kind'] == 'ICRF':
        return rotation(0., 0., 0.)
    return rotation(spec['theta'] + spec['rate']*seconds, spec['tilt'], spec['rate'])


def jacobian(spec, endpoint):
    source, target = spec['input_axes'], spec['output_axes']
    seconds = spec[endpoint + '_seconds']
    a, ad = axis(source, seconds)
    b, bd = axis(target, seconds)
    q = [[float(i == j) for j in range(3)] for i in range(3)]
    if source['base'] != target['base']:
        q = alignment(spec[endpoint + '_rotation_vector'])
        if source['base'] == 'ICRF':
            q = transpose(q)
    r = multiply(transpose(b), multiply(q, a))
    left = multiply(transpose(bd), multiply(q, a))
    right = multiply(transpose(b), multiply(q, ad))
    result = [[0.] * 6 for _ in range(6)]
    for i in range(3):
        for j in range(3):
            result[i][j] = result[i+3][j+3] = r[i][j]
            result[i+3][j] = left[i][j] + right[i][j]
    return result


def expected(spec):
    return multiply(jacobian(spec, 'final'), multiply(spec['native_stm'], inverse(jacobian(spec, 'initial'))))
