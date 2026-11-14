"""Independent SI drag law and fifth-order integration of retained GUI outputs.

Environment inputs (Earth mu and native angular-velocity endpoints) are shared,
explicit inputs. This does not qualify Earth orientation or empirical density.
The C++ gate uses RK4; this audit uses a separate fifth-order tableau.
"""
import json
import math
from pathlib import Path


def force(state, omega, cd, area, mass, rho=1e-8):
    r, v = state[:3], state[3:]
    cross = (omega[1]*r[2]-omega[2]*r[1],
             omega[2]*r[0]-omega[0]*r[2],
             omega[0]*r[1]-omega[1]*r[0])
    relative = [1000*(a-b) for a, b in zip(v, cross)]
    speed = math.sqrt(sum(x*x for x in relative))
    return [-0.5*rho*cd*area*speed*x/mass/1000 for x in relative]


def integrate(initial, seconds, mu, omega_start, omega_final, cd, area, mass,
              steps=250):
    state = list(initial)
    h = seconds/steps
    c = [0, 1/5, 3/10, 4/5, 8/9, 1]
    a = [[], [1/5], [3/40, 9/40], [44/45, -56/15, 32/9],
         [19372/6561, -25360/2187, 64448/6561, -212/729],
         [9017/3168, -355/33, 46732/5247, 49/176, -5103/18656]]
    b = [35/384, 0, 500/1113, 125/192, -2187/6784, 11/84]
    def derivative(s, t):
        omega = [x+(y-x)*t/seconds for x, y in zip(omega_start, omega_final)]
        drag = force(s, omega, cd, area, mass)
        radius = math.sqrt(sum(x*x for x in s[:3]))
        gravity = [-mu*x/radius**3 if mu else 0 for x in s[:3]]
        return s[3:]+[g+d for g, d in zip(gravity, drag)]
    for n in range(steps):
        k = []
        for j in range(6):
            sample = [state[i]+h*sum(a[j][m]*k[m][i] for m in range(j))
                      for i in range(6)]
            k.append(derivative(sample, (n+c[j])*h))
        state = [state[i]+h*sum(b[j]*k[j][i] for j in range(6)) for i in range(6)]
    return state


def self_check():
    # Straight-line drag, no gravity/rotation: closed-form v=v0/(1+k*v0*t).
    state = [7000, 0, 0, 2, 0, 0]
    cd, area, mass, t = 2.2, 20, 1000, 10
    k = 500*1e-8*cd*area/mass
    result = integrate(state, t, 0, [0]*3, [0]*3, cd, area, mass)
    expected = [7000+math.log1p(k*2*t)/k, 0, 0, 2/(1+k*2*t), 0, 0]
    for i, (x, y) in enumerate(zip(result, expected)):
        assert abs(x-y) <= (1e-9 if i < 3 else 1e-12), (result, expected)
    return dict(closed_form_straight_line_drag_passed=True)


def audit(observations):
    validation = self_check()
    records = []
    assert len(observations) == 10
    for sample in observations:
        assert sample['density_kg_m3'] == 1e-8
        mass = sum(sample[k] for k in ['dry_mass_kg', 'fuel_mass_kg', 'second_tank_mass_kg'])
        assert mass == sample['total_mass_kg'] == sample['report'][1]
        assert abs(sample['report'][2]-10) <= 1e-12 and sample['report'][-1] == 41
        actual = sample['report'][3:9]
        args = [sample['initial_state'], sample['seconds'], sample['mu_km3_s2'],
                sample['omega_start_rad_s'], sample['omega_final_rad_s'],
                sample['cd'], sample['area_m2'], mass]
        coarse = integrate(*args, steps=125)
        expected = integrate(*args, steps=250)
        errors = [abs(a-b) for a, b in zip(actual, expected)]
        refinement = [abs(a-b) for a, b in zip(coarse, expected)]
        acceleration = force(actual, sample['omega_final_rad_s'], sample['cd'], sample['area_m2'], mass)
        magnitude = math.sqrt(sum(x*x for x in acceleration))
        acceleration_errors = [abs(a-b) for a, b in zip(acceleration, sample['native_acceleration_km_s2'])]
        assert all(math.isfinite(x) for x in actual+expected+acceleration)
        assert all(e <= (1e-6 if i < 3 else 1e-9) for i, e in enumerate(errors)), (sample['trial'], errors)
        assert all(e <= (1e-9 if i < 3 else 1e-12) for i, e in enumerate(refinement))
        assert all(e <= 1e-12*magnitude+1e-18 for e in acceleration_errors)
        records.append(dict(trial=sample['trial'],independent_state=expected,
                            errors=errors,refinement_errors=refinement,
                            independent_acceleration=acceleration,
                            acceleration_errors=acceleration_errors))
    return dict(status='passed',algorithm='Dormand-Prince fifth-order; separate from native and C++ RK4',
                self_check=validation,trials=records,checked_state_components=60,
                checked_acceleration_components=30,independent_environment_truth=False,
                empirical_atmosphere_qualified=False)


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('observations', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.write_text(json.dumps(audit(json.loads(args.observations.read_text())), indent=2)+'\n')
