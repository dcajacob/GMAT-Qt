"""Replay real MATLAB Fmincon ephemeris evidence against a frozen bounded optimum."""
import argparse
import csv
import json
import math
from pathlib import Path


def need(ok, message):
    if not ok:
        raise ValueError(message)


def audit(root, fixture):
    root = Path(root); plan = json.loads(Path(fixture).read_text())
    proof = json.loads((root/'proof.json').read_text())
    need(plan['mu'] == 398600.4415 and plan['position_bound_km'] == 2e-5 and
         plan['velocity_bound_km_s'] == 2e-8 and plan['control_bound'] == 1e-6 and
         plan['epoch_bound_seconds'] == .002, 'Frozen independent bounds differ')
    need([p['key'] for p in proof['phases']] == [p['key'] for p in plan['phases']], 'Phase census differs')
    results = []
    for row, expected in zip(proof['phases'], plan['phases']):
        problems = []
        if not row['completed'] or not row['trials_present']:
            problems.append('Mission or diagnostic report missing')
        trials = list(csv.reader((root/(row['key']+'-trials.csv')).read_text().splitlines()))
        if not trials:
            problems.append('Actual diagnostic report empty')
        for trial in trials:
            if len(trial) != 3:
                problems.append('Diagnostic report shape differs');break
            x, cost, keep = map(float,trial)
            if not all(math.isfinite(v) for v in (x,cost,keep)) or abs(cost-(x-2)**2) > 1e-8 or keep != 41:
                problems.append('Trial report violates the independent quadratic/sentinel');break
        if not math.isfinite(row['alpha']) or row['alpha'] < -1e-6 or row['alpha'] > 1+1e-6:
            problems.append('Native control is outside the bounded problem')
        if not math.isfinite(row['cost']) or abs(row['cost']-(row['alpha']-2)**2) > 1e-8:
            problems.append('Cost differs from independent quadratic')
        if 'expected_converged' in expected:
            if row['converged'] != expected['expected_converged'] or not row['actual_matlab_exit_flag']:
                problems.append('Native/MATLAB convergence status missing or wrong')
            if (row['exit_flag'] > 0) != expected['expected_converged']:
                problems.append('Actual MATLAB exit flag disagrees')
        if 'expected_alpha' in expected:
            if abs(row['alpha']-expected['expected_alpha']) > 1e-6 or abs(row['cost']-expected['expected_cost']) > 1e-6:
                problems.append('Result differs from independent constrained optimum or initial guess')
        if len(row['rows']) != expected['expected_rows']:
            problems.append('Unconverged output escaped or accepted output missing')
        radius = 7000+100*expected.get('expected_alpha',row['alpha'])
        rate, speed = math.sqrt(plan['mu']/radius**3), math.sqrt(plan['mu']/radius)
        for i, values in enumerate(row['rows']):
            if len(values) != 7:
                problems.append('Wrong ephemeris state width');break
            t=values[0];angle=rate*t
            truth=[radius*math.cos(angle),radius*math.sin(angle),0.,-speed*math.sin(angle),speed*math.cos(angle),0.]
            if abs(t-5*i) > .002 or any(not math.isfinite(v) or abs(v-truth[j]) > (2e-5 if j<3 else 2e-8) for j,v in enumerate(values[1:])):
                problems.append('Ephemeris differs from independent circle/epoch');break
        results.append(dict(phase=row['key'],passed=not problems,problems=problems,rows=len(row['rows'])))
    return dict(status='passed' if all(p['passed'] for p in results) else 'failed',
                native_missions=len(results),phases=results,provider='actual MATLAB FminconOptimizer',
                native_tests_run_by_replay=0,new_requirement_credit=0)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('root',type=Path);p.add_argument('fixture',type=Path);p.add_argument('--output',type=Path)
    a=p.parse_args();result=audit(a.root,a.fixture)
    if a.output:
        with a.output.open('x') as f:json.dump(result,f,indent=2);f.write('\n')
    print(json.dumps(result));raise SystemExit(result['status'] != 'passed')
