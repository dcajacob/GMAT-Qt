"""Serialization and negative controls for the independent batch file oracle."""
import copy
import json
from pathlib import Path
import struct
import unittest

import BatchFormatOracle as oracle

FIXTURE = json.loads(Path(__file__).with_name('NasaBatchFormatCases.json').read_text())
OBSERVATIONS = '\n'.join(f'{FIXTURE["utc_mjd"]+(i*28680/357+34)/86400:.16f} Range_Skin 1 A B 1' for i in range(358))


def stk(spec):
    scale = 1000 if spec['distance_unit'] == 'Meters' else 1
    lines = ['stk.v.10.0', 'BEGIN Ephemeris', 'NumberOfEphemerisPoints 1435',
             'ScenarioEpoch 10 Jun 2012 00:00:00.000', 'CentralBody Earth',
             'CoordinateSystem J2000', 'InterpolationMethod Lagrange', 'InterpolationOrder 7',
             'DistanceUnit '+spec['distance_unit']]
    lines += ['EphemerisTimePosVel']
    lines += [' '.join(map(str, [i*20]+[scale*x for x in oracle.circle(i*20, FIXTURE)])) for i in range(1435)]
    return ('\n'.join(lines+['END Ephemeris'])+'\n').encode()


def code500(spec):
    order = '<' if spec['byte_order'] == 'LittleEndian' else '>'
    data = bytearray(31*oracle.RECORD)
    def put(offset, *values):
        struct.pack_into(order+str(len(values))+'d', data, offset, *values)
    data[:8] = b'EPHEM   '
    data[216:220] = b'2000'
    struct.pack_into(order+'i', data, 220, 4)
    for offset, value in {16:2, 24:1120610, 32:162, 40:0, 48:1120610, 56:162,
                          64:28680, 72:20, 152:1, 208:570918, 1536:oracle.DUT_BASE,
                          1544:oracle.DUT_BASE+28680/864, 1552:20/864, 1576:0}.items():
        put(offset, value)
    def units(state):
        return [x/10000 for x in state[:3]]+[x*864/10000 for x in state[3:]]
    put(520, *units(oracle.circle(0, FIXTURE)))
    for record in range(29):
        start = record*1000
        offset = (2+record)*oracle.RECORD
        put(offset, 1120610, 162, start, 20)
        put(offset+2432, oracle.DUT_BASE+start/864, 20/864)
        for i in range(50):
            t = start+i*20
            put(offset+32+i*48, *(units(oracle.circle(t, FIXTURE)) if t <= 28680 else [oracle.SENTINEL]*6))
    return bytes(data)


class Contract(unittest.TestCase):
    def analyze(self, payload, spec):
        return oracle.analyze(payload, OBSERVATIONS, FIXTURE, spec)

    def test_all_four_encodings_and_units(self):
        for spec in FIXTURE['cases']:
            with self.subTest(spec=spec['key']):
                result = self.analyze((stk if spec['format']=='STK-TimePosVel' else code500)(spec), spec)
                self.assertEqual(result['points'], 1435)
                self.assertLess(result['max_position_error_km'], 2e-11)
                self.assertLess(result['max_velocity_error_km_s'], 2e-14)

    def test_wrong_units_and_frame_rejected(self):
        spec = FIXTURE['cases'][1]
        for old, new in [(b'DistanceUnit Meters', b'DistanceUnit Kilometers'),
                         (b'CoordinateSystem J2000', b'CoordinateSystem Fixed')]:
            with self.assertRaises(ValueError):
                self.analyze(stk(spec).replace(old,new), spec)

    def test_shifted_orbit_and_missing_grid_rejected(self):
        spec = FIXTURE['cases'][0]
        lines = stk(spec).decode().splitlines()
        start = lines.index('EphemerisTimePosVel')+1
        changed = lines.copy()
        fields = changed[start+900].split(); fields[1] = str(float(fields[1])+.001)
        changed[start+900] = ' '.join(fields)
        for mutation in [changed, lines[:start+900]+lines[start+901:]]:
            with self.assertRaises(ValueError):
                self.analyze('\n'.join(mutation).encode(), spec)

    def test_iteration_segment_and_clock_rejected(self):
        spec = FIXTURE['cases'][0]
        for old, new in [(b'\nEphemerisTimePosVel',b'\nBEGIN SegmentBoundaryTimes\n0\n20\nEND SegmentBoundaryTimes\nEphemerisTimePosVel'),
                         (b'10 Jun 2012 00:00:00.000', b'10 Jun 2012 00:00:01.000')]:
            with self.assertRaises(ValueError):
                self.analyze(stk(spec).replace(old,new), spec)

    def test_wrong_byte_order_and_truncation_rejected(self):
        spec = FIXTURE['cases'][2]
        other = copy.deepcopy(spec); other['byte_order'] = 'BigEndian'
        with self.assertRaises(ValueError):
            self.analyze(code500(spec), other)
        with self.assertRaises(ValueError):
            self.analyze(code500(spec)[:-8], spec)

    def test_partial_sentinel_and_finite_after_padding_rejected(self):
        spec = FIXTURE['cases'][2]
        original = code500(spec)
        for index in [35*6, 36*6]:
            changed = bytearray(original)
            struct.pack_into('<d',changed,30*oracle.RECORD+32+index*8,0.)
            with self.assertRaises(ValueError):
                self.analyze(bytes(changed), spec)

    def test_dut_and_header_state_rejected(self):
        spec = FIXTURE['cases'][3]
        for offset, value in [(1536,oracle.DUT_BASE+1), (520,7.)]:
            changed = bytearray(code500(spec)); struct.pack_into('>d',changed,offset,value)
            with self.assertRaises(ValueError):
                self.analyze(bytes(changed),spec)


if __name__ == '__main__':
    unittest.main()
