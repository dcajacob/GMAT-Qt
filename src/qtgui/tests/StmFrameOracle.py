"""Independent two-end Cartesian frame Jacobians for a prescribed tilted spin."""
from decimal import Decimal
import math


def multiply(a, b):
    return [[sum(x*y for x, y in zip(row, column)) for column in zip(*b)] for row in a]


def rotation(theta, tilt, rate):
    c, s, C, S = math.cos(theta), math.sin(theta), math.cos(tilt), math.sin(tilt)
    return ([[c, -s*C, s*S], [s, c*C, -c*S], [0., S, C]],
            [[-rate*s, -rate*c*C, rate*c*S], [rate*c, -rate*s*C, rate*s*S], [0., 0., 0.]])


def jacobian(theta, tilt, rate, inverse=False):
    r, d = rotation(theta, tilt, rate)
    if not inverse: r, d = list(map(list, zip(*r))), list(map(list, zip(*d)))
    result = [[0.] * 6 for _ in range(6)]
    for i in range(3):
        for j in range(3):
            result[i][j] = result[i+3][j+3] = r[i][j]
            result[i+3][j] = d[i][j]
    return result


def seconds(mjd, base=21545.125):
    return float((Decimal.from_float(mjd) - Decimal.from_float(base)) * 86400)


def expected(phi, theta, tilt, rate, initial, final):
    return multiply(jacobian(theta + rate*final, tilt, rate),
                    multiply(phi, jacobian(theta + rate*initial, tilt, rate, inverse=True)))
