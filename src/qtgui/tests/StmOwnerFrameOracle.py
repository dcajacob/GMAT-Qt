"""High-precision independent Cartesian frame maps and numerical Jacobians.

Uses Decimal scalar kinematics and five-point differences, independently of the
native axes buffers and any production automatic differentiation machinery.
"""
from decimal import Decimal, localcontext
import json
import math
from pathlib import Path

D = lambda value: value if isinstance(value, Decimal) else Decimal(int(value)) if isinstance(value, bool) else Decimal(str(value))
TYPES = ['OrbitSTM', 'OrbitSTMA', 'OrbitSTMB', 'OrbitSTMC', 'OrbitSTMD']
PROVIDERS = ['original', 'clone', 'copy']


def dot(a, b): return sum((x*y for x, y in zip(a, b)), D(0))
def add(a, b): return [x+y for x, y in zip(a, b)]
def sub(a, b): return [x-y for x, y in zip(a, b)]
def scale(a, b): return [x*b for x in a]
def norm(a): return dot(a, a).sqrt()
def unit(a): return scale(a, D(1)/norm(a))
def cross(a, b): return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]
def mul(a, b): return [[dot(row, column) for column in zip(*b)] for row in a]


def inverse(a):
    n = len(a); w = [list(row)+[D(i == j) for j in range(n)] for i, row in enumerate(a)]
    for col in range(n):
        pivot = max(range(col, n), key=lambda r: abs(w[r][col])); w[col], w[pivot] = w[pivot], w[col]
        if abs(w[col][col]) < D('1e-50'): raise ValueError('Singular independent Cartesian Jacobian')
        w[col] = [v/w[col][col] for v in w[col]]
        for row in range(n):
            if row != col:
                value = w[row][col]; w[row] = [x-value*y for x, y in zip(w[row], w[col])]
    return [row[n:] for row in w]


def sincos(angle):
    s, c, st, ct = angle, D(1), angle, D(1)
    for n in range(1, 100):
        st *= -angle*angle/D((2*n)*(2*n+1)); ct *= -angle*angle/D((2*n-1)*(2*n))
        s += st; c += ct
        if abs(st)+abs(ct) < D('1e-75'): break
    return s, c


def circle(mu, radius, time):
    mu, radius, time = D(mu), D(radius), D(time); n = (mu/radius**3).sqrt(); s, c = sincos(n*time)
    return [radius*c, radius*s, D(0), -radius*n*s, radius*n*c, D(0)]


def phi(mu, radius, time):
    mu, radius, time = D(mu), D(radius), D(time); n = (mu/radius**3).sqrt(); u = n*time; s, c = sincos(u)
    z, one = D(0), D(1)
    hill = [[4-3*c,z,z,s/n,2*(1-c)/n,z], [6*(s-u),one,z,-2*(1-c)/n,(4*s-3*u)/n,z],
            [z,z,c,z,z,s/n], [3*n*s,z,z,c,2*s,z], [-6*n*(1-c),z,z,-2*s,4*c-3,z], [z,z,-n*s,z,z,c]]
    initial = [[D(i == j) for j in range(6)] for i in range(6)]; initial[3][1] = n; initial[4][0] = -n
    rotation = [[c,-s,z], [s,c,z], [z,z,one]]; omega = [[z,-n,z], [n,z,z], [z,z,z]]; rate = mul(rotation, omega)
    final = [[z]*6 for _ in range(6)]
    for i in range(3):
        for j in range(3):
            final[i][j] = final[i+3][j+3] = rotation[i][j]; final[i+3][j] = rate[i][j]
    return mul(final, mul(hill, initial))


def acceleration(mu, state):
    r = state[:3]; return scale(r, -D(mu)/norm(r)**3)


def frame_value(owner, guide, spec, mu):
    earth = [D(0)]*6
    primary = guide if spec['primary'] == 'Ref' else earth
    origin = guide if spec['origin'] == 'Ref' else earth
    rp, vp = sub(owner[:3], primary[:3]), sub(owner[3:], primary[3:])
    ap = sub(acceleration(mu, owner), acceleration(mu, guide) if spec['primary'] == 'Ref' else earth[:3])
    n = cross(rp, vp); zu = unit(n); nd = cross(rp, ap); zd = scale(sub(nd, scale(zu, dot(zu, nd))), D(1)/norm(n))
    v, vd = (rp, vp) if spec['x_axis'] == 'R' else (vp, ap)
    xu = unit(v); xd = scale(sub(vd, scale(xu, dot(xu, vd))), D(1)/norm(v))
    yu = cross(zu, xu); yd = add(cross(zd, xu), cross(zu, xd))
    position, velocity = sub(owner[:3], origin[:3]), sub(owner[3:], origin[3:])
    return [dot(e, position) for e in [xu, yu, zu]] + [dot(e, velocity)+dot(ed, position) for e, ed in zip([xu, yu, zu], [xd, yd, zd])]


def jacobian(owner, guide, spec, mu):
    result = [[D(0)]*6 for _ in range(6)]
    for j in range(6):
        h = D('1e-10') if j < 3 else D('1e-13'); samples = []
        for k in [-2, -1, 1, 2]:
            trial = list(owner); trial[j] += k*h; samples.append(frame_value(trial, guide, spec, mu))
        for i in range(6): result[i][j] = (samples[0][i]-8*samples[1][i]+8*samples[2][i]-samples[3][i])/(12*h)
    return result


def expected(plan, spec, time):
    with localcontext() as ctx:
        ctx.prec = 80
        mu, r, ref = plan['mu'], plan['radius'], plan['reference_radius']
        a = jacobian(circle(mu,r,0), circle(mu,ref,0), spec, mu)
        b = jacobian(circle(mu,r,time), circle(mu,ref,time), spec, mu)
        value = mul(b, mul(phi(mu,r,time), inverse(a)))
        condition = max(sum(abs(v) for v in row) for row in a)*max(sum(abs(v) for v in row) for row in inverse(a))
        return [[float(v) for v in row] for row in value], float(condition)


def audit(root, fixture):
    root = Path(root); plan = json.loads(Path(fixture).read_text()); proof = json.loads((root/'proof.json').read_text())
    if (plan['matrix_block_tolerances'] != [2e-8,2e-6,2e-11,2e-8] or
            plan['position_tolerance_km'] != 2e-5 or plan['velocity_tolerance_km_s'] != 2e-8):
        raise ValueError('Frozen owner-frame bounds differ')
    if not math.isfinite(proof['integration_seconds']) or abs(proof['integration_seconds']-60.) > 1e-6:
        raise ValueError('Owner-frame propagation duration differs')
    specs = {c['key']:c for c in plan['cases']}; rows = proof['observations']; issues = []
    if list(specs) != ['OffsetPrimaryRTN','OffsetOriginRTN','OffsetPrimaryVNB','CenteredRTN'] or plan['expected_reads'] != 60:
        issues.append('Frozen frame inventory differs')
    with localcontext() as ctx:
        ctx.prec = 80
        for state_key, matrix_key, radius in [('owner_state','native_stm',plan['radius']),('guide_state','guide_stm',plan['reference_radius'])]:
            truth_state = circle(plan['mu'],radius,proof['integration_seconds']); truth_phi = phi(plan['mu'],radius,proof['integration_seconds'])
            if len(proof[state_key]) != 6 or any(not math.isfinite(v) or abs(v-float(truth_state[i])) > (plan['position_tolerance_km'] if i<3 else plan['velocity_tolerance_km_s']) for i,v in enumerate(proof[state_key])):
                issues.append('Independent circular state differs')
            if (len(proof[matrix_key]) != 6 or any(len(row) != 6 for row in proof[matrix_key]) or
                    any(not math.isfinite(proof[matrix_key][i][j]) or abs(proof[matrix_key][i][j]-float(truth_phi[i][j])) > plan['matrix_block_tolerances'][2*(i//3)+j//3] for i in range(6) for j in range(6))):
                issues.append('Independent circular variational propagation differs')
    if [(v['case'],v['provider'],v['type']) for v in rows] != [(key,p,t) for key in specs for p in PROVIDERS for t in TYPES]:
        issues.append('Owner-frame native read inventory differs')
    expected_values = {key:expected(plan,c,proof['integration_seconds'])[0] for key,c in specs.items() if not c['singular']}
    checks = []
    for row in rows:
        spec = specs[row['case']]; isolated = all(row.get(v,False) for v in ['owner_unchanged','guide_unchanged','cache_atomic','readonly','owner_bound','frame_bound'])
        if spec['singular']:
            passed = row.get('rejected',False) and 'singular' in row.get('diagnostic','').lower() and isolated
            checks.append(dict(case=row['case'],provider=row['provider'],type=row['type'],passed=passed,rejection=True))
            continue
        truth = expected_values[row['case']]; k = TYPES.index(row['type']); size = 6 if k == 0 else 3
        r,c = (0,0) if k == 0 else (3*((k-1)//2),3*((k-1)%2)); actual = row.get('actual',[])
        shape = len(actual)==size and all(len(v)==size for v in actual)
        error = max((abs(actual[i][j]-truth[i+r][j+c]) for i in range(size) for j in range(size)),default=math.inf) if shape else math.inf
        passed = shape and not row.get('rejected',False) and isolated and all(math.isfinite(actual[i][j]) and
                    abs(actual[i][j]-truth[i+r][j+c]) <= plan['matrix_block_tolerances'][2*((i+r)//3)+(j+c)//3]
                    for i in range(size) for j in range(size))
        checks.append(dict(case=row['case'],provider=row['provider'],type=row['type'],passed=passed,rejection=False,max_error=error if math.isfinite(error) else None))
    failed = sum(not v['passed'] for v in checks)
    return dict(status='passed' if not failed and not issues else 'failed',passed=len(checks)-failed,failed=failed,
                native_reads=len(checks),defined_contexts=9,singular_rejections=15,issues=issues,checks=checks,
                native_tests_run_by_replay=0,new_requirement_credit=0,whole_row_qualified=False)
