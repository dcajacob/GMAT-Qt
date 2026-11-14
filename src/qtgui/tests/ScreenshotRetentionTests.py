import json
from pathlib import Path
import struct
import tempfile
import unittest

import ScreenshotRetention as retention
import AcceptanceSnapshot as snapshot


class RetentionContracts(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.case = self.root/'QtGui.Example'
        (self.case/'screenshots').mkdir(parents=True)
        self.frame = self.case/'screenshots/frame.png'
        self.frame.write_bytes(b'\x89PNG\r\n\x1a\n'+b'\x00\x00\x00\rIHDR'+struct.pack('>II', 3, 4))
        self.oracle = self.case/'direct-pixel-oracle.png'
        self.oracle.write_bytes(self.frame.read_bytes())
        log = self.case/'test.log';log.write_text('PASS original assertions\n')
        junit = self.case/'ctest.xml';junit.write_text('<testsuite tests="1" failures="0"><testcase name="QtGui.Example" status="run"/></testsuite>')
        self.result = dict(name='QtGui.Example', status='passed', returncode=0, log=str(log), junit=str(junit),
                           log_sha256=retention.sha(log), junit_sha256=retention.sha(junit),
                           trace=dict(screenshots=[str(self.frame), str(self.frame), str(self.oracle)]))
        self.receipt = self.root/'results.json'
        self.save()

    def save(self):
        self.receipt.write_text(json.dumps(dict(finished_utc='completed', snapshot_unchanged=True, results=[self.result])))

    def test_pass_prunes_only_observer_frame_and_preserves_receipt_oracle_and_reuse(self):
        original = self.receipt.read_bytes()
        report = retention.prune_receipt(self.receipt)
        self.assertEqual(report['files'], 1)
        self.assertFalse(self.frame.exists())
        self.assertTrue(self.oracle.exists())
        self.assertEqual(self.receipt.read_bytes(), original)
        record = retention.retained_capture(self.frame)
        self.assertFalse(record['pixels_available'])
        self.assertEqual(record['png_size'], [3, 4])
        self.assertEqual(retention.prune_receipt(self.receipt)['files'], 0)
        self.assertTrue(retention.passing_case(self.result))
        case = dict(name=self.result['name'], command=['example'])
        profile = dict(visible=True)
        self.result.update(snapshot_id='frozen', execution_profile=profile,
                           case_fingerprint=snapshot.case_fingerprint(case))
        self.assertTrue(snapshot.reusable(self.result, case, 'frozen', profile))

    def test_failed_tampered_and_protected_cases_keep_frames(self):
        self.result['status'] = 'failed';self.save()
        self.assertEqual(retention.prune_receipt(self.receipt)['files'], 0)
        self.result['status'] = 'passed';self.save()
        self.assertEqual(retention.prune_receipt(self.receipt, protected={self.case})['files'], 0)
        Path(self.result['log']).write_text('changed log')
        self.assertEqual(retention.prune_receipt(self.receipt)['files'], 0)
        self.assertTrue(self.frame.exists())

    def test_changed_original_receipt_invalidates_pruning_attestation(self):
        retention.prune_receipt(self.receipt)
        self.result['status'] = 'failed';self.save()
        with self.assertRaisesRegex(ValueError, 'identity'):
            retention.retained_capture(self.frame)

    def test_incomplete_run_missing_junit_and_external_symlink_are_preserved(self):
        self.receipt.write_text(json.dumps(dict(results=[self.result])))
        self.assertEqual(retention.prune_receipt(self.receipt)['files'], 0)
        self.save()
        junit = Path(self.result['junit'])
        original = junit.read_bytes();junit.unlink()
        self.assertEqual(retention.prune_receipt(self.receipt)['files'], 0)
        junit.write_bytes(original)
        self.frame.unlink();self.frame.symlink_to(self.oracle)
        self.assertEqual(retention.prune_receipt(self.receipt)['files'], 0)
        self.assertTrue(self.oracle.exists())


if __name__ == '__main__':
    unittest.main()
