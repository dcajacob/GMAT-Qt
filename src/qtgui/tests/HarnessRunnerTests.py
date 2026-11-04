"""Negative acceptance-receipt and source-preservation checks; no GMAT run."""
from pathlib import Path
import tempfile
import unittest

import RunHarnessAcceptance as runner


class HarnessReceiptContracts(unittest.TestCase):
    def test_only_one_successful_exact_test_can_pass(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'ctest.xml'
            good='<testsuites><testsuite tests="1" failures="0"><testcase name="QtAcceptance.Unit"/></testsuite></testsuites>'
            path.write_text(good)
            self.assertTrue(runner.passed_junit(path,'QtAcceptance.Unit',0))
            self.assertFalse(runner.passed_junit(path,'QtAcceptance.Unit',1))
            self.assertFalse(runner.passed_junit(path,'Different.Test',0))
            for text in ['<testsuites/>','<broken>',
                         good.replace('<testcase name="QtAcceptance.Unit"/>','<testcase name="QtAcceptance.Unit"><failure/></testcase>'),
                         good.replace('<testcase name="QtAcceptance.Unit"/>','<testcase name="QtAcceptance.Unit"><skipped/></testcase>'),
                         good.replace('failures="0"','errors="1"'),
                         good.replace('failures="0"','disabled="1"'),
                         good.replace('<testcase name="QtAcceptance.Unit"/>','<testcase name="QtAcceptance.Unit"/><testcase name="other"/>')]:
                path.write_text(text)
                with self.subTest(text=text):
                    self.assertFalse(runner.passed_junit(path,'QtAcceptance.Unit',0))
            self.assertFalse(runner.passed_junit(Path(directory)/'missing.xml','QtAcceptance.Unit',0))

    def test_archive_preserves_only_matching_text_and_hashes_authority_data(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);repo=root/'repo';repo.mkdir();output=root/'out';output.mkdir()
            (repo/'case.py').write_text('assert 2+3==5\n')
            (repo/'authority.xlsx').write_bytes(b'\x00\xffunit-authority')
            hashes={name:runner.sha(repo/name) for name in ['case.py','authority.xlsx']}
            result=runner.preserve_text_inputs(repo,output,hashes,{'authority.xlsx'})
            self.assertEqual((output/'sources/case.py').read_bytes(),(repo/'case.py').read_bytes())
            self.assertFalse((output/'sources/authority.xlsx').exists())
            self.assertEqual(result['preserved_source_sha256'],{'case.py':hashes['case.py']})
            self.assertEqual(result['authority_inputs_not_copied'],{'authority.xlsx':hashes['authority.xlsx']})
            self.assertTrue(result['source_archive_created_after_execution'])

    def test_changed_and_escaping_sources_cannot_be_archived(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);repo=root/'repo';repo.mkdir();output=root/'out';output.mkdir()
            (repo/'case.py').write_text('before')
            before=runner.sha(repo/'case.py');(repo/'case.py').write_text('after')
            with self.assertRaisesRegex(ValueError,'changed execution input'):
                runner.preserve_text_inputs(repo,output,{'case.py':before},set())
            for i,source in enumerate(['../escape.py','/tmp/escape.py']):
                destination=root/('bad'+str(i));destination.mkdir()
                with self.assertRaisesRegex(ValueError,'Unsafe source archive'):
                    runner.preserve_text_inputs(repo,destination,{source:'0'*64},set())


if __name__=='__main__':
    unittest.main()
