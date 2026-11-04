"""Published ERFA references and time/convention negative controls.

Numerical constants: ERFA v2.0.1 src/t_erfa_c.c, t_c2t06a, t_pnm80,
t_gmst82 and t_eqeq94. https://github.com/liberfa/erfa/tree/v2.0.1
These validate the independent reference, not GMAT acceptance coverage.
"""
from pathlib import Path
import tempfile
import unittest

import erfa
import numpy as np
import Fk5StandardsOracle as oracle


class PublishedReferenceTests(unittest.TestCase):
    def test_published_c2t06a_matrix(self):
        actual = erfa.c2t06a(2400000.5, 53736., 2400000.5, 53736., 2.55060238e-7, 1.860359247e-6)
        expected = np.array([[-.1810332128305897282, .9834769806938592296, .6555550962998436505e-4],
                             [-.9834768134136214897, -.1810332203649130832, .5749800844905594110e-3],
                             [.5773474024748545878e-3, .3961816829632690581e-4, .9999998325501747785]])
        self.assertLessEqual(float(np.max(np.abs(actual - expected))), 1e-12)

    def test_published_iau1976_1980_matrix(self):
        actual = erfa.pnm80(2400000.5, 50123.9999)
        expected = np.array([[.9999995831934611169, .8373654045728124011e-3, .3639121916933106191e-3],
                             [-.8373804896118301316e-3, .9999996485439674092, .4130202510421549752e-4],
                             [-.3638774789072144473e-3, -.4160674085851722359e-4, .9999999329310274805]])
        for i in range(3):
            for j in range(3):
                self.assertLessEqual(abs(actual[i, j] - expected[i, j]), 1e-12 if i == j else 1e-14)

    def test_published_greenwich_sidereal_time(self):
        self.assertLessEqual(abs(erfa.gmst82(2400000.5, 53736.) - 1.754174981860675096), 1e-12)

    def test_published_equation_of_equinoxes(self):
        self.assertLessEqual(abs(erfa.eqeq94(2400000.5, 41234.) - .5357758254609256894e-4), 1e-17)

    def test_pinned_reference_library_identity(self):
        self.assertEqual(erfa.__version__, oracle.REQUIRED_ERFA)
        self.assertEqual(erfa.version.erfa_version, '2.0.1')
        self.assertEqual(erfa.version.sofa_version, '20231011')


class TimeAndFrameContracts(unittest.TestCase):
    def setUp(self):
        self.eop = [(51543., .357), (51544., .355), (51545., .353)]
        self.clock = oracle.times(21545., self.eop)

    def test_split_tai_tt_utc_ut1_offsets(self):
        clock = self.clock
        difference = lambda a, b: ((a[0] - b[0]) + (a[1] - b[1])) * 86400
        self.assertAlmostEqual(difference(clock['tt'], clock['tai']), 32.184, delta=5e-7)
        self.assertAlmostEqual(difference(clock['tai'], clock['utc']), 32., delta=5e-7)
        self.assertAlmostEqual(difference(clock['ut1'], clock['utc']), clock['dut1_seconds'], delta=5e-7)
        self.assertEqual(clock['eop_bracket'], [[51544., .355], [51545., .353]])

    def test_eop_interpolation_without_utc_leap_jump(self):
        clock = self.clock
        expected = .355 - .002 * (clock['utc_mjd'] - 51544.)
        self.assertEqual(clock['dut1_seconds'], expected)
        with self.assertRaisesRegex(ValueError, 'leap interval'):
            oracle.times(21545., [(51543., -.1), (51544., -.1), (51545., .9)])

    def test_missing_eop_coverage_and_daily_rows_reject(self):
        with self.assertRaisesRegex(ValueError, 'outside retained EOP'):
            oracle.times(30000., self.eop)
        with self.assertRaisesRegex(ValueError, 'adjacent daily'):
            oracle.times(21545., [(51543., .357), (51546., .351)])
        with self.assertRaisesRegex(ValueError, 'Nonfinite'):
            oracle.times(float('nan'), self.eop)

    def test_post1997_reference_policy_is_explicit(self):
        with self.assertRaisesRegex(ValueError, 'Pre-1997'):
            oracle.reference((2400000.5, 50000.), (2400000.5, 50000.))

    def test_common_terrestrial_polar_motion_cancels(self):
        tt, ut1 = self.clock['tt'], self.clock['ut1']
        expected = oracle.reference(tt, ut1)
        for xp, yp in [(0., 0.), (2.5e-7, -1.8e-6), (-4e-6, 3e-6)]:
            pole = erfa.pom00(xp, yp, erfa.sp00(*tt))
            old = erfa.c2teqx(erfa.pnm80(*tt), erfa.gmst82(*ut1) + erfa.eqeq94(*tt), pole)
            new = erfa.c2t06a(*tt, *ut1, xp, yp)
            self.assertLess(float(np.max(np.abs(old.T @ new - expected))), 8e-16)

    def test_tt_cannot_be_substituted_for_ut1(self):
        correct = oracle.reference(self.clock['tt'], self.clock['ut1'])
        conflated = oracle.reference(self.clock['tt'], self.clock['tt'])
        self.assertGreater(float(np.max(np.abs(correct - conflated))), 1e-11)

    def test_direction_inverse_and_norm(self):
        matrix = oracle.reference(self.clock['tt'], self.clock['ut1'])
        vector = np.array([7000., -1200., 900.])
        self.assertLess(float(np.max(np.abs(matrix.T @ (matrix @ vector) - vector))), 3e-12)
        self.assertAlmostEqual(float(np.linalg.norm(matrix @ vector)), float(np.linalg.norm(vector)), delta=3e-12)
        self.assertGreater(float(np.max(np.abs(matrix @ vector - matrix.T @ vector))), .0001)

    def test_eop_calendar_validation_and_order(self):
        with tempfile.TemporaryDirectory(prefix='gmat-eop-reference-') as directory:
            file = Path(directory) / 'eop.txt'
            file.write_text('2000 1 1 51544 0 0 .355\n2000 1 2 51545 0 0 .353\n')
            self.assertEqual(oracle.read_eop(file), [(51544., .355), (51545., .353)])
            for raw in ['2000 1 1 51543 0 0 .355\n',
                        '2000 1 2 51545 0 0 .353\n2000 1 1 51544 0 0 .355\n',
                        '2000 1 1 51544 0 0 nan\n']:
                file.write_text(raw)
                with self.assertRaises(ValueError):
                    oracle.read_eop(file)


if __name__ == '__main__':
    unittest.main()
