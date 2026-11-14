"""Explain the retained three-sample failure without changing its acceptance bounds."""
import csv
from datetime import datetime
import math
from pathlib import Path


def diagnostic(root, case):
    root = Path(root)
    nodes = [list(map(float,row)) for row in csv.reader((root/'raw-inputs.csv').read_text().splitlines())]
    assert len(nodes) == 3 and all(len(row)==7 for row in nodes)
    origin = nodes[0][0]*86400
    x = [row[0]*86400-origin for row in nodes]
    assert x[0] == 0 and abs(x[1]-5)<1e-6 and abs(x[2]-10)<1e-6
    rows = []
    baseline = datetime(2000,1,1,12)
    for line in (root/'mission.oem').read_text().splitlines():
        fields = line.split()
        if fields and fields[0][:4].isdigit():
            rows.append([(datetime.fromisoformat(fields[0])-baseline).total_seconds()]+
                        list(map(float,fields[1:])))
    expected = case['expected_segments'][0]
    assert len(rows)==len(expected)==2
    observations = []
    for row, wanted in zip(rows,expected):
        assert row[0]==wanted[0]
        candidates = []
        for offset in range(-3,4):
            query = (origin+row[0]+offset*math.ulp(origin))-origin
            predicted = [sum(node[j+1]*math.prod((query-x[k])/(x[i]-x[k])
                         for k in range(3) if k!=i) for i,node in enumerate(nodes)) for j in range(6)]
            candidates.append(dict(clock_ULP_offset=offset,
                position_residual_km=max(abs(a-b) for a,b in zip(row[1:4],predicted[:3])),
                velocity_residual_km_s=max(abs(a-b) for a,b in zip(row[4:7],predicted[3:]))))
        observations.append(dict(epoch_seconds=row[0],
            strict_position_error_km=max(abs(a-b) for a,b in zip(row[1:4],wanted[1:4])),
            strict_velocity_error_km_s=max(abs(a-b) for a,b in zip(row[4:7],wanted[4:7])),
            independent_quadratic_candidates=candidates))
    position = max(o['strict_position_error_km'] for o in observations)
    velocity = max(o['strict_velocity_error_km_s'] for o in observations)
    assert position>case['position_tolerance'] and velocity>case['velocity_tolerance']
    warning = [line for line in (root/'messages.txt').read_text().splitlines()
               if 'order will be reduced to 2' in line]
    assert len(warning)==1
    return dict(status='strict-circular-failure-retained;diagnostic-only',native_input_samples=3,
                native_clock_ULP_seconds=math.ulp(origin),
                position_bound_km=case['position_tolerance'],velocity_bound_km_s=case['velocity_tolerance'],
                max_position_error_km=position,max_velocity_error_km_s=velocity,
                observations=observations,native_order_reduction_warning=warning[0],
                inference='Independent quadratic interpolation of the three actual native inputs reproduces the output at nearby representable query clocks. This explains reduced-order truncation;it does not qualify the unchanged circular-state bounds.',
                assertions_relaxed=0,new_requirement_credit=0,science_qualified=False)
