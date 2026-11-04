"""Reject duplicate, partial, stale and invisible extension proofs; no NASA credit."""
import copy
from pathlib import Path
import tempfile
import unittest
from unittest import mock
import BuildNasaCoverageExtension as extension


class ExtensionAdmissionContracts(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix='gmat-extension-guard-');self.addCleanup(self.temp.cleanup)
        root=Path(self.temp.name);self.log=root/'test.log';self.log.write_text('NASA_CHECK synthetic\n')
        self.junit=root/'ctest.xml';self.junit.write_text('<testsuites><testsuite tests="1" failures="0"><testcase name="QtGui.Synthetic"/></testsuite></testsuites>')
        self.cases={'QtGui.Synthetic':dict(log=str(self.log),log_sha256=extension.sha(self.log),junit=str(self.junit),junit_sha256=extension.sha(self.junit))}
        self.receipt=dict(mode='visible',max_screenshots=0,visible_display=dict(resizable=True,start_minimized=True,initial_host_window_state='Iconic'),results=[dict(name='QtGui.Synthetic',status='passed',returncode=0,log=str(self.log),trace=dict(native_exposure_events=1,screenshots=[]))])

    def test_duplicate_already_counted_and_unknown_rows_reject(self):
        for incoming in (['new','new'],['old'],['unknown']):
            with self.subTest(incoming=incoming),self.assertRaises(ValueError):
                extension.merge_keys({'old'},incoming,{'old','new'})
        self.assertEqual(extension.merge_keys({'old'},['new'],{'old','new'}),{'old','new'})

    def test_failed_skipped_timeout_and_nonzero_never_admit(self):
        for status in ('failed','skipped','timeout','not-run'):
            r=copy.deepcopy(self.receipt);r['results'][0]['status']=status
            with self.subTest(status=status),self.assertRaises(ValueError):extension.validate_visible(r,self.cases)
        r=copy.deepcopy(self.receipt);r['results'][0]['returncode']=1
        with self.assertRaises(ValueError):extension.validate_visible(r,self.cases)

    def test_missing_extra_and_unexposed_cases_reject(self):
        for results in ([],self.receipt['results']+[{**self.receipt['results'][0],'name':'other'}]):
            r={**self.receipt,'results':results}
            with self.assertRaises(ValueError):extension.validate_visible(r,self.cases)
        r=copy.deepcopy(self.receipt);r['results'][0]['trace']['native_exposure_events']=0
        with self.assertRaises(ValueError):extension.validate_visible(r,self.cases)

    def test_headless_fullscreen_nonresizable_nonminimized_and_screenshots_reject(self):
        for field,value in [('resizable',False),('start_minimized',False),('initial_host_window_state','Iconic _NET_WM_STATE_FULLSCREEN')]:
            r=copy.deepcopy(self.receipt);r['visible_display'][field]=value
            with self.subTest(field=field),self.assertRaises(ValueError):extension.validate_visible(r,self.cases)
        for field,value in [('mode','headless'),('max_screenshots',1)]:
            with self.assertRaises(ValueError):extension.validate_visible({**self.receipt,field:value},self.cases)
        r=copy.deepcopy(self.receipt);r['results'][0]['trace']['screenshots']=['unexpected.png']
        with self.assertRaises(ValueError):extension.validate_visible(r,self.cases)

    def test_changed_log_and_junit_reject(self):
        for path in (self.log,self.junit):
            old=path.read_bytes();path.write_bytes(old+b' altered')
            with self.assertRaises(ValueError):extension.validate_visible(self.receipt,self.cases)
            path.write_bytes(old)

    def test_missing_wrong_case_and_skipped_junit_reject(self):
        for text in ('<testsuites/>','<testsuites><testsuite><testcase name="other"/></testsuite></testsuites>',
                     '<testsuites><testsuite><testcase name="QtGui.Synthetic"><skipped/></testcase></testsuite></testsuites>'):
            self.junit.write_text(text);cases=copy.deepcopy(self.cases);cases['QtGui.Synthetic']['junit_sha256']=extension.sha(self.junit)
            with self.assertRaises(ValueError):extension.validate_visible(self.receipt,cases)


if __name__=='__main__':unittest.main()
