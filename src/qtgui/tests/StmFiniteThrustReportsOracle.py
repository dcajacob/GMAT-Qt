"""Check native matrix reports across a finite-burn shutdown."""
from decimal import localcontext
import json
import math
from pathlib import Path
import re

import StmSrpOracle as common
import StmFiniteThrustOracle


def audit(root, fixture):
    root = Path(root)
    plan = json.loads(Path(fixture).read_text())
    live = StmFiniteThrustOracle.audit(root, fixture)
    proof = json.loads((root / 'proof.json').read_text())
    initial = json.loads((root / 'initial-states.json').read_text())
    inputs = json.loads((root / 'frame-sun-inputs.json').read_text())
    samples = json.loads((root / 'post-report-samples.json').read_text())
    phases = ['Powered', 'Coast', 'Restarted', 'FinalCoast'] if plan.get('restart_reports') else ['Powered', 'Coast']
    common.require([sample['phase'] for sample in samples] == [1, 0]*(len(phases)//2),
                   'Powered and coast report phases differ')
    reads = report_entries = 0
    with localcontext() as context:
        context.prec = 80
        for phase, sample in zip(phases, samples):
            current_plan = dict(common.time_dependent_plan(plan, proof['integration_seconds']),
                                thrust_newtons=plan['thrust_newtons'] if sample['phase'] else 0)
            common.require(sample['owner_state'] == proof['owner_state'] and
                sample['guide_state'] == proof['guide_state'] and
                sample['owner_unchanged'] and sample['guide_unchanged'],
                'Post-propagation reports changed the endpoint state')
            acceleration = common.acceleration(current_plan, sample['owner_state'], inputs['final'])
            common.require(all(abs(a-b.real) < 1e-13 for a,b in zip(sample['acceleration'], acceleration)),
                           'Post-propagation frame acceleration differs from the active burn state')
            truths = {}
            for spec in plan['cases']:
                start = common.jacobian(plan, spec, initial['Sat'], initial['Ref'], inputs['initial'])
                end = common.jacobian(current_plan, spec, sample['owner_state'], sample['guide_state'], inputs['final'])
                flow = [[common.D(value) for value in row] for row in proof['native_stm']]
                truths[spec['key']] = common.mul(end, common.mul(flow, common.inverse(start)))
            common.require([(row['case'], row['type']) for row in sample['reads']] ==
                [(spec['key'], kind) for spec in plan['cases'] for kind in common.TYPES],
                'Post-propagation matrix inventory differs')
            for row in sample['reads']:
                common.require(row['readonly'], 'Post-propagation matrix became writable')
                index = common.TYPES.index(row['type'])
                size = 6 if index == 0 else 3
                first_row, first_col = (0, 0) if index == 0 else (3*((index-1)//2), 3*((index-1)%2))
                report = root / (phase + row['case'] + row['type'] + '.txt')
                values = [float(value.replace('D','E').replace('d','e'))
                    for value in re.findall(r'[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[EeDd][-+]?\d+)?', report.read_text())]
                common.require(len(values) == size*size, 'Native matrix report cardinality differs')
                actual = row['actual']
                common.require(len(actual) == size and all(len(line) == size for line in actual),
                               'Post-propagation matrix shape differs')
                for i in range(size):
                    for j in range(size):
                        expected = float(truths[row['case']][i+first_row][j+first_col])
                        bound = plan['matrix_block_tolerances'][2*((i+first_row)//3)+(j+first_col)//3]
                        common.require(math.isfinite(values[i*size+j]) and
                            abs(actual[i][j]-expected) <= bound and abs(values[i*size+j]-expected) <= bound,
                            'Post-propagation matrix or native report differs from the independent frame differential')
                        report_entries += 1
                reads += 1
    return dict(status='passed', live_native_reads=live['native_reads'],
                post_propagation_reads=reads, native_report_entries=report_entries,
                phases=phases, native_tests_run_by_replay=0,
                new_requirement_credit=0, whole_row_qualified=False)
