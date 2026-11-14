"""Replay bounded convergence-output repairs; the whole E406 conjunction stays separate."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from RunEphemerisConvergence import audit as branch_check


def load(path): return json.loads(Path(path).read_text())
def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def need(ok, message):
    if not ok: raise ValueError(message)


def audit(stage):
    stage = Path(stage)
    first = branch_check(stage/'native-v2', stage/'frozen-plan-v1.json')
    final = branch_check(stage/'native-v3', stage/'initial-guess-repair-v1.json')
    need(first['passed'] == 5 and first['failed'] == 1 and
         [c for c in first['checks'] if not c['passed']] ==
         [dict(case='NestedDC',phase='initial_guess',passed=False,rows=25,
               failures=['Output differs from independent final circular orbit'])],
         'Original initial-epoch failure or five passing controls changed')
    need(final['status'] == 'passed' and final['passed'] == 1, 'Initial-epoch repair failed')
    branch = [c for c in first['checks'] if c['passed']] + final['checks']
    need(len({(c['case'],c['phase']) for c in branch}) == 6, 'Incomplete branch control census')
    old = stage.parents[1]/'gui-acceptance/terminal-ephemeris-visible-v1'
    identical = []
    for case in ['NestedDC', 'NestedYukon']:
        for phase, index in [('exhausted',0), ('recovered',1)]:
            before = old/('QtGui.NasaTerminalEphemeris'+case)/(str(index)+'-trials.csv')
            after = stage/'native-v2'/(case+'-'+phase)/'Trials.csv'
            need(before.read_bytes() == after.read_bytes(), 'A solver/diagnostic trajectory changed')
            identical.append(dict(case=case,phase=phase,sha256=sha(before)))
    batch = stage/'batch-dependent-v1/QtGui.NasaBatchOutputEdgeIterationLimit'
    result = load(batch/'result.json'); proof = load(batch/'proof.json')
    need(result['status'] == 'passed' and result['returncode'] == 0 and proof['outcome'] == 'passed',
         'Affected batch test failed')
    need(len(proof['phases']) == 3, 'Batch recovery census changed')
    for i, phase in enumerate(proof['phases']):
        need(phase['phase'] == i and phase['outcome'] == 'Completed', 'Batch phase did not complete')
        rows = phase['rows']
        if i == 0:
            need(not rows and phase['final_publications'] == 0 and
                 phase['terminal_status'] == 'Exceed maximum number of iterations',
                 'Unconverged batch output escaped')
        else:
            need(len(rows) == phase['final_publications'] == 1435 and phase['terminal_status'].startswith('Meet '),
                 'Converged batch recovery missing')
            mu, radius = 398600.4415, 7000.
            rate, speed, tilt = math.sqrt(mu/radius**3), math.sqrt(mu/radius), math.sqrt(.5)
            for n, row in enumerate(rows):
                t = 20*n; angle = rate*t
                wanted = [t,radius*math.cos(angle),radius*math.sin(angle)*tilt,radius*math.sin(angle)*tilt,
                          -speed*math.sin(angle),speed*math.cos(angle)*tilt,speed*math.cos(angle)*tilt]
                need(len(row) == 7 and all(math.isfinite(v) and abs(v-wanted[j]) <=
                     (1e-4 if j == 0 else 2e-5 if j < 4 else 2e-8) for j,v in enumerate(row)),
                     'Batch trajectory differs from frozen circular orbit')
    return dict(status='passed',selected_branch_contexts=6,reused_branch_contexts=5,
                newly_fixed_initial_epoch_contexts=1,dependent_batch_contexts=3,
                byte_identical_prior_diagnostic_reports=identical,original_initial_epoch_failure_retained=True,
                original_nonconverged_output_receipts_retained=True,original_assertions_changed=0,
                native_tests_run_by_replay=0,new_requirement_credit=0,whole_row_qualified=False,
                full_current_build_qualified=False,full_regression=False)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('stage',type=Path);p.add_argument('--output',type=Path)
    a=p.parse_args();result=audit(a.stage)
    if a.output:
        with a.output.open('x') as f:json.dump(result,f,indent=2);f.write('\n')
    print(json.dumps(result))
