"""Supplementary fixed-pole Brouwer frame witnesses, frozen before execution.

The retained NASA mean-to-osculating model supplies only the Earth-equatorial
force correction. An independent geometric rotation supplies the mean ellipse
in the ecliptic basis. No native conversion output supplies an expectation.
"""
from pathlib import Path
import hashlib
import json
import math
from BrouwerPrototypeOracle import adapt, cartesian

ROOT = Path(__file__).resolve().parents[3]
OBLIQUITY = math.radians(84381.448 / 3600)


def cross(a, b):
    return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def rotate_mean(elements, angle):
    a, e, inc, node, peri, mean = elements
    i, om, w = map(math.radians, [inc, node, peri])
    p = [math.cos(om)*math.cos(w)-math.sin(om)*math.sin(w)*math.cos(i),
         math.sin(om)*math.cos(w)+math.cos(om)*math.sin(w)*math.cos(i),
         math.sin(w)*math.sin(i)]
    h = [math.sin(om)*math.sin(i), -math.cos(om)*math.sin(i), math.cos(i)]
    def rotate(v):
        return [v[0], math.cos(angle)*v[1]-math.sin(angle)*v[2],
                math.sin(angle)*v[1]+math.cos(angle)*v[2]]
    p, h = rotate(p), rotate(h)
    n = [-h[1], h[0], 0]
    norm = math.sqrt(dot(n, n))
    n = [x/norm for x in n]
    return [a, e, math.degrees(math.atan2(norm, h[2])),
            math.degrees(math.atan2(n[1], n[0])) % 360,
            math.degrees(math.atan2(dot(cross(n, p), h), dot(n, p))) % 360, mean]


def generate():
    mu, mean_eq = 398600.4418, [10800, .17, 47, 112, 38, 143]
    mean_ec = rotate_mean(mean_eq, -OBLIQUITY)
    restored = rotate_mean(mean_ec, OBLIQUITY)
    assert max(abs(x-y) for x, y in zip(restored, mean_eq)) < 1e-12
    cases = []
    oracle_rows = []
    for kind in ["Short", "Long"]:
        reference = ROOT / ("prototype/StateConv/BROLYD_short.m" if kind == "Short" else "prototype/StateConv/BROLYD.m")
        forward, _, _ = adapt(reference)
        initial = cartesian(forward(mean_eq), mu)
        setup = "Earth.Mu = 398600.4418;\nSat.CoordinateSystem = EarthMJ2000Eq;\nSat.DisplayStateType = Cartesian;\n"
        setup += "".join(f"Sat.{name} = {value:.17g};\n" for name, value in zip(["X", "Y", "Z", "VX", "VY", "VZ"], initial))
        fields = ["SMA", "ECC", "INC", "RAAN", "AOP", "MA"]
        report = " ".join([f"Sat.EarthMJ2000Ec.Brouwer{kind}{field}" for field in fields]
                          + [f"Sat.EarthMJ2000Eq.{field}" for field in ["X", "Y", "Z", "VX", "VY", "VZ"]])
        for index in range(-1, 6):
            expected_mean = list(mean_ec)
            if index >= 0:
                expected_mean[index] += [30, .01, 1, 2, 3, 4][index]
            expected_eq = rotate_mean(expected_mean, OBLIQUITY)
            expected_cart = cartesian(forward(expected_eq), mu)
            key = f"Brouwer{kind}Ecliptic" + ("Read" if index < 0 else "Write"+fields[index])
            case = dict(key=key, setup=setup, origin="Earth", body_case="Earth",
                        expected_origin_type="Planet", mu=mu, parameter="BrouwerMean"+kind,
                        orbital_case=True, read_only=False, native_guard_checks=False,
                        destination="R" if index < 0 else f"Sat.EarthMJ2000Ec.Brouwer{kind}{fields[index]}",
                        before="3.25" if index < 0 else format(mean_ec[index], ".17g"),
                        expression="1" if index < 0 else format(expected_mean[index], ".17g"),
                        report=report, expected=expected_mean+expected_cart,
                        invalid=["AbsentNASA + 1", "1 +"],
                        absolute_tolerance=1e-6, relative_tolerance=2e-12,
                        column_absolute_tolerances=[1e-6, 1e-9, 1e-6, 1e-6, 1e-6, 1e-6]+[1e-6]*3+[1e-9]*3)
            cases.append(case)
            oracle_rows.append(dict(key=key, mean_equatorial=expected_eq,
                                    mean_ecliptic=expected_mean, cartesian_equatorial=expected_cart))
    inputs = ["prototype/StateConv/BROLYD.m", "prototype/StateConv/BROLYD_short.m",
              "src/qtgui/tests/BrouwerPrototypeOracle.py", "src/qtgui/tests/BrouwerFrameOracle.py"]
    return dict(schema_version=1, obliquity_arcsec=84381.448,
                oracle="Retained NASA zonal model in EarthMJ2000Eq; independent periapsis/normal rotation for the mean ellipse; independent Kepler/Cartesian reconstruction.",
                reference_sha256={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in inputs},
                oracle_rows=oracle_rows, cases=cases,
                scope="Two mean models, one nonsingular orbit in two fixed inertial bases, six independent scalar writes each, actual GUI transactions and persistence. No all-frame/domain or independent physical averaging qualification.")


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    with args.output.open("x") as stream:
        json.dump(generate(), stream, indent=2)
        stream.write("\n")
