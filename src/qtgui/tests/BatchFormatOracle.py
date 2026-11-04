"""Independent STK/Code-500 decoding and circular-orbit acceptance checks.

Wire layout is shared specification input from Code500EphemerisFile.hpp;
no GMAT reader, propagator, interpolation, or measurement code is called.
This is a bounded file/science check, not formal external ICD certification.
"""
import argparse
import datetime as dt
import hashlib
import json
import math
from pathlib import Path
import struct

BASE = dt.datetime(2012, 6, 10, tzinfo=dt.timezone.utc)
DUT_BASE = (BASE - dt.datetime(1957, 9, 18, tzinfo=dt.timezone.utc)).total_seconds() / 864
SENTINEL = 9.999999999999999e15
RECORD = 2800


def check(value, message):
    if not value:
        raise ValueError(message)


def circle(t, fixture):
    r, mu = fixture['radius'], fixture['mu']
    n, v, c = math.sqrt(mu / r**3), math.sqrt(mu / r), math.sqrt(.5)
    return [r * math.cos(n*t), r * math.sin(n*t)*c, r * math.sin(n*t)*c,
            -v * math.sin(n*t), v * math.cos(n*t)*c, v * math.cos(n*t)*c]


def civil(packed, day_of_year, seconds):
    check(math.isfinite(packed) and packed == int(packed), 'Invalid packed date')
    p = int(packed)
    year, month, day = p // 10000, p // 100 % 100, p % 100
    if year < 1900:
        year += 1900
    stamp = dt.datetime(year, month, day, tzinfo=dt.timezone.utc)
    check(day_of_year == stamp.timetuple().tm_yday, 'Date/day-of-year disagreement')
    check(math.isfinite(seconds) and -.002 <= seconds < 86400.002, 'Invalid seconds of day')
    return (stamp - BASE).total_seconds() + seconds


def stk(payload, spec):
    lines = payload.decode('ascii').splitlines()
    check(lines and lines[0].startswith('stk.v.'), 'Missing STK signature')
    meta, rows, boundaries = {}, [], []
    state = 'header'
    for raw in lines[1:]:
        line = raw.strip()
        if not line:
            continue
        if line == 'BEGIN Ephemeris':
            check(state == 'header', 'Repeated STK ephemeris')
        elif line == 'BEGIN SegmentBoundaryTimes':
            check(state == 'header', 'Misplaced STK boundary block')
            state = 'boundary'
        elif line == 'END SegmentBoundaryTimes':
            check(state == 'boundary', 'Misplaced STK boundary end')
            state = 'header'
        elif line == 'EphemerisTimePosVel':
            check(state == 'header', 'Repeated STK data block')
            state = 'data'
        elif line == 'END Ephemeris':
            check(state == 'data', 'Premature STK end')
            state = 'end'
        elif state == 'boundary':
            boundaries.append(float(line))
        elif state == 'data':
            row = list(map(float, line.split()))
            check(len(row) == 7 and all(map(math.isfinite, row)), 'Invalid STK state row')
            rows.append(row)
        else:
            check(state == 'header', 'Data after STK terminator')
            name, value = line.split(None, 1)
            check(name not in meta, 'Repeated STK metadata')
            meta[name] = value.strip()
    check(state == 'end', 'Missing STK terminator')
    check(meta['CentralBody'] == 'Earth' and meta['CoordinateSystem'] == 'J2000', 'STK frame differs')
    check(meta['DistanceUnit'] == spec['distance_unit'], 'STK distance unit differs')
    check(int(meta['NumberOfEphemerisPoints']) == len(rows), 'STK point count differs')
    check(meta['InterpolationMethod'] == 'Lagrange' and int(meta['InterpolationOrder']) == 7,
          'STK interpolation metadata differs')
    epoch = dt.datetime.strptime(meta['ScenarioEpoch'], '%d %b %Y %H:%M:%S.%f').replace(tzinfo=dt.timezone.utc)
    check(abs((epoch - BASE).total_seconds()) < .002, 'STK scenario epoch differs')
    # The guide writes boundaries only for discontinuities. This pass has none.
    check(boundaries == [], 'STK final-pass boundaries differ')
    scale = .001 if spec['distance_unit'] == 'Meters' else 1.
    for row in rows:
        row[1:] = [x * scale for x in row[1:]]
    return rows, dict(format='STK-TimePosVel', metadata=meta, boundaries=boundaries,
                      distance_unit=spec['distance_unit'], sentinel_states=0)


def code500(payload, spec):
    check(len(payload) >= 3*RECORD and len(payload) % RECORD == 0, 'Code500 record length differs')
    endian = {'LittleEndian': '<', 'BigEndian': '>'}[spec['byte_order']]
    def numbers(offset, count=1, data=payload):
        return struct.unpack_from(endian + str(count) + 'd', data, offset)
    def num(offset, data=payload):
        return numbers(offset, data=data)[0]
    check(payload[:8] == b'EPHEM   ', 'Code500 product signature differs')
    check(num(16) == 2., 'Code500 output time system is not UTC')
    check(num(152) == 1. and num(1576) == 0., 'Code500 integration/output center differs')
    check(num(208) == 570918., 'Code500 DUT reference differs')
    check(payload[216:220] == b'2000' and struct.unpack_from(endian+'i', payload, 220)[0] == 4,
          'Code500 frame differs')
    check(num(72) == 20. and abs(num(1552)*864 - 20) < 1e-12, 'Code500 header step differs')
    first = civil(num(24), num(32), num(40))
    last = civil(num(48), num(56), num(64))
    dut_first, dut_last = (num(1536)-DUT_BASE)*864, (num(1544)-DUT_BASE)*864
    check(abs(first-dut_first) < .002 and abs(last-dut_last) < .002,
          'Code500 civil/DUT header time differs')
    rows, records, sentinel_count, ended = [], [], 0, False
    for offset in range(2*RECORD, len(payload), RECORD):
        data = payload[offset:offset+RECORD]
        date, doy, sod, step = numbers(0, 4, data)
        all_sentinel = date == SENTINEL
        if all_sentinel:
            check(all(x == SENTINEL for x in [date, doy, sod, step]), 'Invalid Code500 terminal header')
            t = 0.
        else:
            check(not ended, 'Code500 data record after terminal padding')
            t = civil(date, doy, sod)
            check(step == 20. and abs(num(2440, data)*864-20.) < 1e-12,
                  'Code500 data record step differs')
            check(abs(t - (num(2432, data)-DUT_BASE)*864) < .002,
                  'Code500 civil/DUT data time differs')
        valid = 0
        for i in range(50):
            state = list(numbers(32+i*48, 6, data))
            if any(x == SENTINEL for x in state):
                check(all(x == SENTINEL for x in state), 'Partial Code500 sentinel state')
                ended = True
                sentinel_count += 1
            else:
                check(not ended and not all_sentinel and all(map(math.isfinite, state)),
                      'Code500 finite state after padding or nonfinite state')
                rows.append([t+i*step] + [x*10000 for x in state[:3]] +
                            [x*10000/864 for x in state[3:]])
                valid += 1
        records.append(dict(offset=offset, start=t, valid=valid, terminal=all_sentinel))
    check(ended and sentinel_count > 0, 'Missing Code500 end markers')
    check(abs(first-rows[0][0]) < .002 and abs(last-rows[-1][0]) < .002,
          'Code500 header does not bound final trajectory')
    initial = list(numbers(520, 6))
    initial = [x*10000 for x in initial[:3]] + [x*10000/864 for x in initial[3:]]
    check(all(abs(a-b) < (2e-5 if i < 3 else 2e-8)
              for i, (a,b) in enumerate(zip(initial, rows[0][1:]))), 'Code500 header initial state differs')
    return rows, dict(format='Code-500', byte_order=spec['byte_order'], time_system='UTC',
                      frame='EarthMJ2000Eq', records=records, sentinel_states=sentinel_count,
                      first=first, last=last, dut_first=dut_first, dut_last=dut_last)


def analyze(payload, observations, fixture, spec):
    rows, metadata = (stk if spec['format'] == 'STK-TimePosVel' else code500)(payload, spec)
    obs = []
    for line in observations.splitlines():
        if not line.strip() or line.lstrip().startswith('%'):
            continue
        fields = line.split()
        check(len(fields) == 6 and fields[1] == 'Range_Skin', 'Unexpected observation schema')
        obs.append((float(fields[0])-fixture['utc_mjd'])*86400-fixture['tai_minus_utc'])
    check(len(obs) == 358 and abs(max(obs)-28680) < .002, 'Batch observation extent differs')
    check(len(rows) == 1435, 'Final output point count differs from complete 20s grid')
    errors = [0.]*6
    max_clock_error = 0.
    for i, row in enumerate(rows):
        t = row[0]
        check(abs(t-i*20.) < .002 and (i == 0 or t > rows[i-1][0]),
              'Final output sampling is incomplete, repeated, or nonmonotonic')
        max_clock_error = max(max_clock_error, abs(t-i*20))
        for j, (actual, expected) in enumerate(zip(row[1:], circle(t, fixture))):
            error = abs(actual-expected)
            check(error < fixture['position_tolerance' if j < 3 else 'velocity_tolerance'],
                  'Output contains nonfinal trajectory or differs from independent circular truth')
            errors[j] = max(errors[j], error)
    check(abs(rows[0][0]) < .002 and abs(rows[-1][0]-max(obs)) < .002,
          'Final output omitted initial state or last observation')
    return dict(points=len(rows), observations=len(obs), first=rows[0][0], last=rows[-1][0],
                last_observation=max(obs), component_max_errors=errors,
                max_position_error_km=max(errors[:3]), max_velocity_error_km_s=max(errors[3:]),
                max_clock_error_s=max_clock_error, metadata=metadata, rows=rows,
                file_sha256=hashlib.sha256(payload).hexdigest(), independent_decoder=True,
                independent_closed_form_orbit=True, shared_time_frame_and_simulated_observations=True)


def main():
    parser = argparse.ArgumentParser()
    for name in ['fixture', 'case', 'file', 'observations', 'output']:
        parser.add_argument('--'+name, required=True)
    args = parser.parse_args()
    fixture = json.loads(Path(args.fixture).read_text())
    spec = next(c for c in fixture['cases'] if c['key'] == args.case)
    result = analyze(Path(args.file).read_bytes(), Path(args.observations).read_text(), fixture, spec)
    Path(args.output).write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k != 'rows'}))


if __name__ == '__main__':
    main()
