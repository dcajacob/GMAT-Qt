"""Check actual iterative output against convergence and independent circular states."""
import argparse
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import re
import tempfile
import RunStmFrameReference as runtime


def audit(root, fixture):
    root = Path(root); plan = json.loads(Path(fixture).read_text())
    proof = json.loads((root/'proof.json').read_text())
    expected = [(c['key'], p['key']) for c in plan['cases'] for p in c['phases']]
    assert [(r['case'], r['phase']) for r in proof['observations']] == expected
    assert plan['control_tolerance'] == 1e-6 and plan['position_tolerance_km'] == 2e-5
    assert plan['velocity_tolerance_km_s'] == 2e-8 and plan['epoch_tolerance_seconds'] == .002
    checks = []
    start = datetime(2000,1,1,12,tzinfo=timezone.utc)
    for row in proof['observations']:
        spec = next(c for c in plan['cases'] if c['key'] == row['case'])
        phase = next(p for p in spec['phases'] if p['key'] == row['phase'])
        failures = []
        if row['run_status'] != 1: failures.append('Mission did not complete')
        if abs(row['U']-phase['expected_u']) > 1e-6 or abs(row['V']-phase['expected_v']) > 1e-6:
            failures.append('Independent solver/control expectation differs')
        if 'expected_outer_converged' in phase:
            states = [s for s in row['solvers'] if s['name'] == spec['outer']]
            if len(states) != 1 or states[0]['converged'] != phase['expected_outer_converged']:
                failures.append('Actual outer solver convergence differs')
            if 'expected_native_status' in phase and (len(states) != 1 or states[0]['status'] != phase['expected_native_status']):
                failures.append('Required native solver status differs')
            if 'accepted_native_statuses' in phase and (len(states) != 1 or states[0]['status'] not in phase['accepted_native_statuses']):
                failures.append('No accepted public convergence status')
            if phase.get('physical_tolerances_enabled') and (len(states) != 1 or not states[0].get('physical_tolerances_enabled')):
                failures.append('Physical-tolerance setting did not reach the native optimizer')
        path = Path(row['ephemeris']); samples = []
        if path.exists():
            for line in path.read_text().splitlines():
                if not re.match(r'^\d{4}-\d\d-\d\dT',line): continue
                fields = line.split(); assert len(fields) == 7
                stamp=datetime.fromisoformat(fields[0]).replace(tzinfo=timezone.utc)
                samples.append([(stamp-start).total_seconds()]+[float(v) for v in fields[1:]])
        if len(samples) != phase['expected_rows']: failures.append('Unconverged data emitted or expected output missing')
        radius = 7000+100*phase['expected_u']+10*phase['expected_v']
        rate = math.sqrt(plan['reference_mu']/radius**3); speed = math.sqrt(plan['reference_mu']/radius)
        for i, sample in enumerate(samples):
            t = sample[0]; a=rate*t
            want=[radius*math.cos(a),radius*math.sin(a),0.,-speed*math.sin(a),speed*math.cos(a),0.]
            if abs(t-5*i) > .002 or any(not math.isfinite(v) or abs(v-want[k]) > (2e-5 if k<3 else 2e-8) for k,v in enumerate(sample[1:])):
                failures.append('Output differs from independent final circular orbit');break
        trials=Path(row['trials'])
        if not trials.exists() or not trials.read_text().strip(): failures.append('Diagnostic trial report lost')
        checks.append(dict(case=row['case'],phase=row['phase'],passed=not failures,rows=len(samples),failures=failures))
    failed = sum(not c['passed'] for c in checks)
    return dict(status='failed' if failed else 'passed',passed=len(checks)-failed,failed=failed,
                reads=len(checks),contexts=len(checks),native_missions=proof['native_missions'],checks=checks,
                native_tests_run_by_replay=0,new_requirement_credit=0)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('binary',type=Path);p.add_argument('startup',type=Path);p.add_argument('fixture',type=Path);p.add_argument('--output',type=Path);a=p.parse_args()
    runtime.audit=audit
    if a.output:raise SystemExit(runtime.run(a.binary,a.startup,a.fixture,a.output))
    with tempfile.TemporaryDirectory(prefix='gmat-converged-ephem-') as temporary:raise SystemExit(runtime.run(a.binary,a.startup,a.fixture,temporary))
