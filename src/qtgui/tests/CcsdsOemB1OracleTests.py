"""Standard-derived positive boundaries and corrupt OEM negative controls.

Synthetic data here tests the independent format oracle, not GMAT dynamics.
Actual output checks are a separate, failure-reporting audit of retained files.
"""
import unittest

import CcsdsOemB1Oracle as oracle


HEADER = ('CCSDS_OEM_VERS = 1.0\nCREATION_DATE = 2000-001T00:00:00\n'
          'ORIGINATOR = Acceptance test\n')


def block(start='2000-01-01T12:00:00', stop='2000-01-01T12:00:10', degree=1):
    return (f'META_START\nOBJECT_NAME = Test object\nOBJECT_ID = 2000-001A\n'
            f'CENTER_NAME = Earth\nREF_FRAME = EME2000\nTIME_SYSTEM = UTC\n'
            f'START_TIME = {start}\nUSEABLE_START_TIME = {start}\n'
            f'USEABLE_STOP_TIME = {stop}\nSTOP_TIME = {stop}\n'
            f'INTERPOLATION = Lagrange\nINTERPOLATION_DEGREE = {degree}\nMETA_STOP\n'
            f'{start} 7000.0 +0.0 -0.000 0.0 7.5e+00 0.0\n'
            f'{stop} 6999.0 75.0 0.0 -0.08 7.5E0 0.0\n')


GOOD = HEADER + block()


class OemB1Contracts(unittest.TestCase):
    def check(self, value=GOOD, **kwargs):
        return oracle.validate(value.encode('ascii'), **kwargs)

    def reject(self, value, clause):
        with self.assertRaises(oracle.ConformanceError) as caught:
            self.check(value)
        self.assertIn(clause, caught.exception.clause)

    def test_baseline_and_all_line_endings(self):
        for ending in ('\n', '\r', '\r\n', '\n\r'):
            with self.subTest(ending=repr(ending)):
                result = self.check(GOOD.replace('\n', ending))
                self.assertEqual(result['status'], 'passed')
                self.assertEqual(result['records'], 2)

    def test_exact_calendar_ordinal_julian_equivalence(self):
        calendar = oracle.epoch('2000-01-01T12:00:00', 1, 'UTC', set())
        self.assertEqual(calendar, oracle.epoch('2000-001T12:00:00', 1, 'UTC', set()))
        self.assertEqual(calendar, oracle.epoch('2451545.0', 1, 'UTC', set()))
        self.assertEqual(self.check(GOOD.replace('2000-01-01T12:00:00', '2451545.0'))['status'], 'passed')

    def test_submillisecond_order_is_not_rounded(self):
        result = self.check(HEADER + block('2000-001T00:00:00.0000001',
                                          '2000-001T00:00:00.0000002'))
        self.assertEqual(result['status'], 'passed')
        self.assertEqual(result['segments'][0]['start'][1], '1E-7')

    def test_leap_second_and_next_civil_day_order(self):
        result = self.check(HEADER + block('2016-12-31T23:59:60.5', '2017-01-01T00:00:00'))
        self.assertEqual(result['status'], 'passed')
        self.assertEqual(result['segments'][0]['start'][1], '86400.5')
        unreviewed = self.check(HEADER + block('2030-12-31T23:59:60', '2031-01-01T00:00:00'))
        self.assertEqual(unreviewed['status'], 'incomplete')
        self.assertIn('Unreviewed UTC leap date: 2030-12-31', unreviewed['gaps'])

    def test_optional_fields_and_whitespace_text_conventions(self):
        value = GOOD
        for line in GOOD.splitlines(True):
            if line.startswith('USEABLE_'):
                value = value.replace(line, '')
        value = '\n\n' + value.replace('Test object', 'Test__object')
        value = value.replace('CENTER_NAME = Earth', '  CENTER_NAME  =  eArTh   ')
        self.assertEqual(self.check(value)['status'], 'passed')

    def test_comments_at_header_metadata_before_and_after_data(self):
        value = GOOD.replace('CREATION_DATE', 'COMMENT header\nCREATION_DATE')
        value = value.replace('OBJECT_NAME', 'COMMENT metadata\nOBJECT_NAME')
        value = value.replace('META_STOP\n', 'META_STOP\nCOMMENT before data\n')
        value += 'COMMENT after data\n'
        self.assertEqual(self.check(value)['comments'], 4)
        self.assertEqual(self.check(value)['status'], 'passed')

    def test_adjacent_and_gapped_segments_preserve_boundary_records(self):
        for start in ('2000-01-01T12:00:10', '2000-01-01T12:00:11'):
            value = GOOD + 'COMMENT between arcs\n' + block(start, '2000-01-01T12:00:20')
            self.assertEqual(self.check(value)['records'], 4)
            self.assertEqual(len(self.check(value)['segments']), 2)

    def test_one_point_constant_and_linear_interpolation(self):
        one = GOOD.replace('INTERPOLATION_DEGREE = 1', 'INTERPOLATION_DEGREE = 0')
        one = one.replace('2000-01-01T12:00:10', '2000-01-01T12:00:00')
        one = '\n'.join(one.splitlines()[:-1]) + '\n'
        self.assertEqual(self.check(one)['records'], 1)
        self.assertEqual(self.check(GOOD.replace('Lagrange', 'Linear'))['status'], 'passed')

    def test_custom_id_requires_explicit_external_icd_profile(self):
        value = GOOD.replace('2000-001A', 'SyntheticId')
        result = self.check(value)
        self.assertEqual(result['status'], 'incomplete')
        self.assertEqual(result['gaps'], ['Non-COSPAR object ID needs an ICD: SYNTHETICID'])
        self.assertEqual(self.check(value, icd_object_ids=['SyntheticId'])['status'], 'passed')

    def test_unknown_interpolation_and_time_system_remain_unverified(self):
        for old, new in [('Lagrange', 'Hermite'), ('Lagrange', 'AgencyPolynomial'),
                         ('TIME_SYSTEM = UTC', 'TIME_SYSTEM = AgencyClock')]:
            with self.subTest(new=new):
                self.assertEqual(self.check(GOOD.replace(old, new))['status'], 'incomplete')
        value = '\n'.join(line for line in GOOD.splitlines() if not line.startswith('INTERPOLATION')) + '\n'
        self.assertEqual(self.check(value)['status'], 'incomplete')

    def test_valid_numeric_boundaries(self):
        for value in ('0', '-0', '+0.0', '123456789012345678', '0.00000000000000001',
                      '1.234567890123456e+100', '4.940656458412465e-324', '-1.797693134862315e308'):
            with self.subTest(value=value):
                self.assertIsInstance(oracle.number(value, 1), float)
        self.assertEqual(oracle.integer('-2147483648', 1), -2147483648)
        self.assertEqual(oracle.integer('+2147483647', 1), 2147483647)

    def test_invalid_numeric_lexical_precision_and_range(self):
        for value in ('NaN', 'inf', '-Infinity', '.5', '-.5', '1.', '1D3', '12.3e4',
                      '1e3', '1.2345678901234567e0', '1234567890123456789', '1.0e309',
                      '1.0e-400', '1.0e2147483648'):
            with self.subTest(value=value):
                with self.assertRaises(oracle.ConformanceError):
                    oracle.number(value, 1)
        for value in ('2147483648', '-2147483649', '1.5'):
            with self.subTest(value=value):
                with self.assertRaises(oracle.ConformanceError):
                    oracle.integer(value, 1)

    def test_invalid_ascii_line_length_and_termination(self):
        self.reject(GOOD.replace('Test object', 'Test\tobject'), '4.3.1(b)')
        self.reject(GOOD + 'COMMENT ' + 'x' * 247 + '\n', '4.3.1(a)')
        self.assertEqual(self.check(GOOD + 'COMMENT ' + 'x' * 246 + '\n')['status'], 'passed')
        self.reject(GOOD.rstrip('\n'), '4.3.1(e)')
        for byte in (0, 31, 127, 128, 255):
            with self.subTest(byte=byte):
                with self.assertRaises(oracle.ConformanceError):
                    oracle.validate(GOOD.encode() + b'COMMENT ' + bytes([byte]) + b'\n')

    def test_header_version_first_line_and_keyword_order(self):
        self.reject('COMMENT too soon\n' + GOOD, '4.3.7(c)')
        self.reject(GOOD.replace('CCSDS_OEM_VERS = 1.0', 'CCSDS_OEM_VERS = 2.0'), '4.3.7(a)')
        self.reject(GOOD.replace('CREATION_DATE = 2000-001T00:00:00\n', ''), 'Table 4-2')
        self.reject(GOOD.replace('ORIGINATOR = Acceptance test', 'ORIGINATOR = __'), '4.3.3')
        self.reject(GOOD.replace('CREATION_DATE', 'creation_date'), 'Table 4-2')
        self.reject(GOOD.replace('ORIGINATOR = Acceptance test', 'ORIGINATOR = A = B'), '4.3.2(b)')

    def test_unknown_duplicate_and_misordered_metadata(self):
        self.reject(GOOD.replace('CENTER_NAME = Earth', 'CENTER_NAME = Earth\nCENTER_NAME = Earth'), '4.3.2')
        self.reject(GOOD.replace('REF_FRAME = EME2000', 'REF_FRAME_EPOCH = 2000-001T00:00:00'), '4.3.7(e)')
        self.reject(GOOD.replace('OBJECT_NAME = Test object\nOBJECT_ID = 2000-001A',
                                 'OBJECT_ID = 2000-001A\nOBJECT_NAME = Test object'), '4.3.2(g)')
        self.reject(GOOD.replace('CENTER_NAME = Earth\n', ''), 'Table 4-3')

    def test_missing_and_nested_metadata_boundaries(self):
        self.reject(HEADER, '4.2')
        self.reject(GOOD.replace('META_START\n', ''), '4.3.2')
        self.reject(GOOD.replace('META_STOP\n', ''), '4.3.7')
        self.reject(GOOD.replace('META_STOP\n', 'META_START\n'), '4.3.7(g)')
        self.reject(HEADER + block().split('2000-01-01T12:00:00 7000.0')[0], '4.2')

    def test_invalid_dates_and_creation_julian_forbidden(self):
        for value in ('2000-367T00:00:00', '2001-366T00:00:00', '2000-000T00:00:00',
                      '2000-02-30T00:00:00', '2000-01-01T24:00:00', '2000-01-01T00:60:00',
                      '2000-01-01T12:00:60', '2000-01-01T12:00:00Z', '2000-1-1T12:00:00'):
            with self.subTest(value=value):
                self.reject(GOOD.replace('2000-01-01T12:00:00', value), 'Table 4')
        self.reject(GOOD.replace('CREATION_DATE = 2000-001T00:00:00', 'CREATION_DATE = 2451545.0'), 'Table 4-2')

    def test_units_extra_columns_and_bad_data_are_rejected(self):
        for old, new in [('7000.0 +0.0', '7000.0 [km] +0.0'), ('7000.0 +0.0', '7000.0'),
                         ('7000.0 +0.0', '7000.0 +0.0 1.0')]:
            with self.subTest(new=new):
                self.reject(GOOD.replace(old, new), '4.3.5')
        self.reject(GOOD.replace('7000.0', 'NaN'), '4.3.3')

    def test_duplicate_decreasing_and_wrong_bound_epochs(self):
        last = GOOD.splitlines()[-1]
        self.reject(GOOD + last + '\n', '4.3.5(c)')
        self.reject(GOOD.replace(last, last.replace('12:00:10', '11:59:59')), '4.3.5(c)')
        self.reject(GOOD.replace('START_TIME = 2000-01-01T12:00:00',
                                 'START_TIME = 2000-01-01T11:59:00'), 'Table 4-3')
        self.reject(GOOD.replace('USEABLE_STOP_TIME = 2000-01-01T12:00:10',
                                 'USEABLE_STOP_TIME = 2000-01-01T12:00:11'), 'Table 4-3')

    def test_comments_inside_data_or_without_delimiter_rejected(self):
        last = GOOD.splitlines()[-1]
        self.reject(GOOD.replace(last, 'COMMENT in data\n' + last), '4.3.1(d)')
        self.reject(GOOD + 'COMMENT\n', '4.3.6')

    def test_cross_segment_object_time_and_overlap_fail(self):
        second = block('2000-01-01T12:00:10', '2000-01-01T12:00:20')
        self.reject(GOOD + second.replace('TIME_SYSTEM = UTC', 'TIME_SYSTEM = TAI'), '4.3.5(d)')
        self.reject(GOOD + second.replace('2000-001A', '2000-001B'), '4.1')
        self.reject(GOOD + block('2000-01-01T12:00:09', '2000-01-01T12:00:20'), '4.3.5(c)')

    def test_short_arc_record_count_not_internal_integrator_count(self):
        # The actual retained ShortArc advertises degree 2 with two OEM rows.
        # Three internal integrator points cannot be used by a file recipient.
        self.reject(GOOD.replace('INTERPOLATION_DEGREE = 1', 'INTERPOLATION_DEGREE = 2'), '4.3.5(f)')
        self.reject(GOOD.replace('INTERPOLATION_DEGREE = 1\n', ''), 'Table 4-3')
        self.reject(GOOD.replace('Lagrange', 'Linear').replace('DEGREE = 1', 'DEGREE = 2'), '4.3.5(f)')
        self.reject(GOOD.replace('DEGREE = 1', 'DEGREE = -1'), '4.3.5(f)')


if __name__ == '__main__':
    unittest.main()
